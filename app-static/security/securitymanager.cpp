// 文件说明：app-static\security\securitymanager.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "securitymanager.h"

#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QGroupBox>
#include <QMessageBox>

#ifdef Q_OS_MAC
#include <Security/Security.h>
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincred.h>
#endif

// ==================== SecurityManager ====================

SecurityManager::SecurityManager(QObject *parent)
    : QObject(parent)
    , m_encryption(new FileEncryption(this))
    , m_settingsStore(new QSettings("CuteMarkEd", "Security", this))
{
    loadSettings();
    initializeKeychain();

    connect(m_encryption, &FileEncryption::progressChanged,
            this, &SecurityManager::progressChanged);
}

// 函数说明：销毁 SecurityManager 对象，释放本模块持有的资源。
SecurityManager::~SecurityManager()
{
    saveSettings();
}

// 函数说明：实现 SecurityManager::instance 的核心逻辑，供当前模块调用。
SecurityManager& SecurityManager::instance()
{
    static SecurityManager instance;
    return instance;
}

// 函数说明：实现 SecurityManager::encryptFile 的核心逻辑，供当前模块调用。
bool SecurityManager::encryptFile(const QString &filePath,
                                   const QString &password,
                                   const FileEncryption::EncryptionOptions &options)
{
    if (!QFile::exists(filePath)) {
        m_lastError = tr("文件不存在");
        return false;
    }

    // 生成输出路径
    QString outputPath = filePath + ".encrypted";
    
    FileEncryption::EncryptionOptions opts = options;
    opts.pbkdf2Iterations = m_settings.pbkdf2Iterations;

    FileEncryption::ErrorCode code = m_encryption->encryptFile(
        filePath, outputPath, password, opts);

    if (code != FileEncryption::ErrorCode::Success) {
        m_lastError = m_encryption->lastErrorString();
        return false;
    }

    // 如果设置了安全删除原文件
    if (m_settings.secureDelete) {
        secureDeleteFile(filePath);
    } else {
        QFile::remove(filePath);
    }

    // 重命名加密文件
    QFile::rename(outputPath, filePath);

    emit fileEncrypted(filePath);
    return true;
}

// 函数说明：实现 SecurityManager::decryptFile 的核心逻辑，供当前模块调用。
bool SecurityManager::decryptFile(const QString &filePath,
                                   const QString &password,
                                   const QString &outputPath)
{
    if (!isFileEncrypted(filePath)) {
        m_lastError = tr("文件未加密");
        return false;
    }

    QString outPath = outputPath.isEmpty() ? filePath + ".decrypted" : outputPath;

    FileEncryption::ErrorCode code = m_encryption->decryptFile(
        filePath, outPath, password);

    if (code == FileEncryption::ErrorCode::Expired) {
        m_lastError = tr("文件已过期");
        emit fileExpired(filePath);
        
        // 自动删除过期文件
        if (m_settings.secureDelete) {
            secureDeleteFile(filePath);
        }
        return false;
    }

    if (code != FileEncryption::ErrorCode::Success) {
        m_lastError = m_encryption->lastErrorString();
        return false;
    }

    emit fileDecrypted(filePath);
    return true;
}

// 函数说明：实现 SecurityManager::decryptFileWithBiometric 的核心逻辑，供当前模块调用。
bool SecurityManager::decryptFileWithBiometric(const QString &filePath,
                                                 const QString &outputPath)
{
    if (!isBiometricAvailable()) {
        m_lastError = tr("生物特征认证不可用");
        return false;
    }

    // 检查文件是否需要生物特征
    FileEncryption::EncryptionMetadata metadata;
    m_encryption->readMetadata(filePath, metadata);

    if (!metadata.requireBiometric) {
        m_lastError = tr("此文件不需要生物特征认证");
        return false;
    }

    // 从安全存储中获取密码
    QString storageKey = QString("file:%1").arg(QFileInfo(filePath).absoluteFilePath());
    QString password = loadPassword(storageKey);

    if (password.isEmpty()) {
        m_lastError = tr("未找到存储的密码，请使用密码解密");
        return false;
    }

    // 执行生物特征认证
    BiometricAuth::AuthResult authResult = BiometricAuth::instance().authenticateSync(
        tr("解密文件: %1").arg(QFileInfo(filePath).fileName()));

    if (authResult != BiometricAuth::AuthResult::Success) {
        m_lastError = tr("生物特征认证失败");
        return false;
    }

    // 使用存储的密码解密文件
    return decryptFile(filePath, password, outputPath);
}

// 函数说明：判断 SecurityManager 当前是否满足指定状态。
bool SecurityManager::isFileEncrypted(const QString &filePath) const
{
    return FileEncryption::isEncryptedFile(filePath);
}

// 函数说明：判断 SecurityManager 当前是否满足指定状态。
bool SecurityManager::isFileExpired(const QString &filePath) const
{
    if (!isFileEncrypted(filePath)) {
        return false;
    }

    FileEncryption::EncryptionMetadata metadata;
    m_encryption->readMetadata(filePath, metadata);
    return FileEncryption::isExpired(metadata);
}

// 函数说明：读取 SecurityManager 当前保存的状态或计算结果。
FileEncryption::EncryptionMetadata SecurityManager::getFileMetadata(const QString &filePath) const
{
    FileEncryption::EncryptionMetadata metadata;
    m_encryption->readMetadata(filePath, metadata);
    return metadata;
}

// 函数说明：向 SecurityManager 管理的数据集合中添加一项内容。
void SecurityManager::addProtectedFolder(const ProtectedFolder &folder)
{
    m_protectedFolders[folder.path] = folder;
    saveSettings();
}

// 函数说明：从 SecurityManager 管理的数据集合中移除指定内容。
void SecurityManager::removeProtectedFolder(const QString &path)
{
    m_protectedFolders.remove(path);
    m_lockedFolders.removeAll(path);
    saveSettings();
}

// 函数说明：实现 SecurityManager::protectedFolders 的核心逻辑，供当前模块调用。
QList<SecurityManager::ProtectedFolder> SecurityManager::protectedFolders() const
{
    return m_protectedFolders.values();
}

// 函数说明：判断 SecurityManager 当前是否满足指定状态。
bool SecurityManager::isPathProtected(const QString &path) const
{
    for (const ProtectedFolder &folder : m_protectedFolders) {
        if (path.startsWith(folder.path)) {
            return true;
        }
    }
    return false;
}

// 函数说明：读取 SecurityManager 当前保存的状态或计算结果。
SecurityManager::ProtectedFolder SecurityManager::getProtectedFolderConfig(const QString &path) const
{
    for (const ProtectedFolder &folder : m_protectedFolders) {
        if (path.startsWith(folder.path)) {
            return folder;
        }
    }
    return ProtectedFolder();
}

// 函数说明：实现 SecurityManager::lockFolder 的核心逻辑，供当前模块调用。
bool SecurityManager::lockFolder(const QString &path)
{
    if (!m_protectedFolders.contains(path)) {
        return false;
    }

    if (!m_lockedFolders.contains(path)) {
        m_lockedFolders.append(path);
    }

    emit folderLocked(path);
    return true;
}

// 函数说明：实现 SecurityManager::unlockFolder 的核心逻辑，供当前模块调用。
bool SecurityManager::unlockFolder(const QString &path, const QString &password)
{
    if (!m_lockedFolders.contains(path)) {
        return true;  // 已经解锁
    }

    // 验证密码
    QString storageKey = QString("folder:%1").arg(path);
    QString storedPassword = loadPassword(storageKey);

    // 如果没有存储密码，则设置新密码
    if (storedPassword.isEmpty()) {
        if (!password.isEmpty()) {
            savePassword(storageKey, password);
        }
        m_lockedFolders.removeAll(path);
        emit folderUnlocked(path);
        return true;
    }

    // 验证密码是否匹配
    if (password != storedPassword) {
        m_lastError = tr("密码错误");
        return false;
    }

    m_lockedFolders.removeAll(path);
    emit folderUnlocked(path);
    return true;
}

// 函数说明：实现 SecurityManager::unlockFolderWithBiometric 的核心逻辑，供当前模块调用。
bool SecurityManager::unlockFolderWithBiometric(const QString &path)
{
    if (!isBiometricAvailable()) {
        return false;
    }

    ProtectedFolder folder = getProtectedFolderConfig(path);
    if (!folder.requireBiometric) {
        return false;
    }

    // 同步认证
    BiometricAuth::AuthResult result = BiometricAuth::instance().authenticateSync(
        tr("解锁文件夹: %1").arg(QFileInfo(path).fileName()));

    if (result == BiometricAuth::AuthResult::Success) {
        m_lockedFolders.removeAll(path);
        emit folderUnlocked(path);
        return true;
    }

    return false;
}

// 函数说明：判断 SecurityManager 当前是否满足指定状态。
bool SecurityManager::isFolderLocked(const QString &path) const
{
    return m_lockedFolders.contains(path);
}

// 函数说明：判断 SecurityManager 当前是否满足指定状态。
bool SecurityManager::isBiometricAvailable() const
{
    return BiometricAuth::instance().checkAvailability() == 
           BiometricAuth::Availability::Available;
}

// 函数说明：实现 SecurityManager::biometricAvailability 的核心逻辑，供当前模块调用。
BiometricAuth::Availability SecurityManager::biometricAvailability() const
{
    return BiometricAuth::instance().checkAvailability();
}

// 函数说明：实现 SecurityManager::authenticateBiometric 的核心逻辑，供当前模块调用。
void SecurityManager::authenticateBiometric(const QString &reason,
                                             std::function<void(bool)> callback)
{
    BiometricAuth::instance().authenticate(reason,
        [this, callback](BiometricAuth::AuthResult result, const QString &message) {
            bool success = (result == BiometricAuth::AuthResult::Success);
            if (success) {
                emit authenticationSucceeded();
            } else {
                emit authenticationFailed(message);
            }
            if (callback) {
                callback(success);
            }
        });
}

// 函数说明：设置 SecurityManager 的运行参数，并触发必要的界面或数据刷新。
void SecurityManager::setSettings(const SecuritySettings &settings)
{
    m_settings = settings;
    saveSettings();
}

// 函数说明：保存 SecurityManager 当前状态，保证用户修改可以持久化。
void SecurityManager::saveSettings()
{
    m_settingsStore->beginGroup("Security");
    m_settingsStore->setValue("defaultLevel", static_cast<int>(m_settings.defaultLevel));
    m_settingsStore->setValue("pbkdf2Iterations", m_settings.pbkdf2Iterations);
    m_settingsStore->setValue("rememberPassword", m_settings.rememberPassword);
    m_settingsStore->setValue("passwordTimeout", m_settings.passwordTimeout);
    m_settingsStore->setValue("enableBiometric", m_settings.enableBiometric);
    m_settingsStore->setValue("secureDelete", m_settings.secureDelete);
    m_settingsStore->setValue("secureDeletePasses", m_settings.secureDeletePasses);
    m_settingsStore->endGroup();

    // 保存受保护文件夹
    m_settingsStore->beginWriteArray("ProtectedFolders");
    int i = 0;
    for (const ProtectedFolder &folder : m_protectedFolders) {
        m_settingsStore->setArrayIndex(i++);
        m_settingsStore->setValue("path", folder.path);
        m_settingsStore->setValue("level", static_cast<int>(folder.level));
        m_settingsStore->setValue("autoLock", folder.autoLock);
        m_settingsStore->setValue("lockTimeout", folder.lockTimeoutMinutes);
        m_settingsStore->setValue("requireBiometric", folder.requireBiometric);
    }
    m_settingsStore->endArray();
}

// 函数说明：加载 SecurityManager 需要的数据、配置或外部资源。
void SecurityManager::loadSettings()
{
    m_settingsStore->beginGroup("Security");
    m_settings.defaultLevel = static_cast<SecurityLevel>(
        m_settingsStore->value("defaultLevel", 1).toInt());
    m_settings.pbkdf2Iterations = m_settingsStore->value("pbkdf2Iterations", 100000).toInt();
    m_settings.rememberPassword = m_settingsStore->value("rememberPassword", false).toBool();
    m_settings.passwordTimeout = m_settingsStore->value("passwordTimeout", 15).toInt();
    m_settings.enableBiometric = m_settingsStore->value("enableBiometric", true).toBool();
    m_settings.secureDelete = m_settingsStore->value("secureDelete", true).toBool();
    m_settings.secureDeletePasses = m_settingsStore->value("secureDeletePasses", 3).toInt();
    m_settingsStore->endGroup();

    // 加载受保护文件夹
    int size = m_settingsStore->beginReadArray("ProtectedFolders");
    for (int i = 0; i < size; ++i) {
        m_settingsStore->setArrayIndex(i);
        ProtectedFolder folder;
        folder.path = m_settingsStore->value("path").toString();
        folder.level = static_cast<SecurityLevel>(
            m_settingsStore->value("level", 1).toInt());
        folder.autoLock = m_settingsStore->value("autoLock", true).toBool();
        folder.lockTimeoutMinutes = m_settingsStore->value("lockTimeout", 5).toInt();
        folder.requireBiometric = m_settingsStore->value("requireBiometric", false).toBool();
        m_protectedFolders[folder.path] = folder;
    }
    m_settingsStore->endArray();
}

// 函数说明：保存 SecurityManager 当前状态，保证用户修改可以持久化。
bool SecurityManager::savePassword(const QString &key, const QString &password)
{
#ifdef Q_OS_MAC
    // 使用 macOS 钥匙串
    QByteArray keyUtf8 = key.toUtf8();
    QByteArray passUtf8 = password.toUtf8();

    SecKeychainItemRef itemRef = nullptr;
    OSStatus status = SecKeychainFindGenericPassword(
        nullptr,
        strlen("CuteMarkEd"),
        "CuteMarkEd",
        keyUtf8.size(),
        keyUtf8.constData(),
        nullptr,
        nullptr,
        &itemRef
    );

    if (status == errSecSuccess && itemRef) {
        // 密码已存在，更新它
        status = SecKeychainItemModifyAttributesAndData(
            itemRef,
            nullptr,
            passUtf8.size(),
            passUtf8.constData()
        );
        CFRelease(itemRef);
        return status == errSecSuccess;
    }

    // 密码不存在，新增
    status = SecKeychainAddGenericPassword(
        nullptr,
        strlen("CuteMarkEd"),
        "CuteMarkEd",
        keyUtf8.size(),
        keyUtf8.constData(),
        passUtf8.size(),
        passUtf8.constData(),
        nullptr
    );
    return status == errSecSuccess;
#elif defined(Q_OS_WIN)
    // 使用 Windows 凭据管理器
    CREDENTIALW cred = { 0 };
    cred.Type = CRED_TYPE_GENERIC;
    std::wstring target = (QString("CuteMarkEd:") + key).toStdWString();
    cred.TargetName = const_cast<wchar_t*>(target.c_str());
    cred.CredentialBlobSize = password.toUtf8().size();
    cred.CredentialBlob = reinterpret_cast<LPBYTE>(
        const_cast<char*>(password.toUtf8().constData()));
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;

    return CredWriteW(&cred, 0);
#else
    // Linux: 使用加密的本地存储
    // 简化实现，实际应该使用 libsecret 或类似库
    m_settingsStore->setValue("passwords/" + key, 
        password.toUtf8().toBase64());
    return true;
#endif
}

// 函数说明：加载 SecurityManager 需要的数据、配置或外部资源。
QString SecurityManager::loadPassword(const QString &key) const
{
#ifdef Q_OS_MAC
    UInt32 passwordLength = 0;
    void *passwordData = nullptr;

    OSStatus status = SecKeychainFindGenericPassword(
        nullptr,
        strlen("CuteMarkEd"),
        "CuteMarkEd",
        key.toUtf8().size(),
        key.toUtf8().constData(),
        &passwordLength,
        &passwordData,
        nullptr
    );

    if (status == errSecSuccess && passwordData) {
        QString password = QString::fromUtf8(
            static_cast<char*>(passwordData), passwordLength);
        SecKeychainItemFreeContent(nullptr, passwordData);
        return password;
    }
    return QString();
#elif defined(Q_OS_WIN)
    PCREDENTIALW cred = nullptr;
    std::wstring target = (QString("CuteMarkEd:") + key).toStdWString();

    if (CredReadW(target.c_str(), CRED_TYPE_GENERIC, 0, &cred)) {
        QString password = QString::fromUtf8(
            reinterpret_cast<char*>(cred->CredentialBlob),
            cred->CredentialBlobSize);
        CredFree(cred);
        return password;
    }
    return QString();
#else
    QByteArray encoded = m_settingsStore->value("passwords/" + key).toByteArray();
    return QString::fromUtf8(QByteArray::fromBase64(encoded));
#endif
}

// 函数说明：从 SecurityManager 管理的数据集合中移除指定内容。
bool SecurityManager::removePassword(const QString &key)
{
#ifdef Q_OS_MAC
    SecKeychainItemRef itemRef = nullptr;
    OSStatus status = SecKeychainFindGenericPassword(
        nullptr,
        strlen("CuteMarkEd"),
        "CuteMarkEd",
        key.toUtf8().size(),
        key.toUtf8().constData(),
        nullptr,
        nullptr,
        &itemRef
    );

    if (status == errSecSuccess && itemRef) {
        status = SecKeychainItemDelete(itemRef);
        CFRelease(itemRef);
        return status == errSecSuccess;
    }
    return false;
#elif defined(Q_OS_WIN)
    std::wstring target = (QString("CuteMarkEd:") + key).toStdWString();
    return CredDeleteW(target.c_str(), CRED_TYPE_GENERIC, 0);
#else
    m_settingsStore->remove("passwords/" + key);
    return true;
#endif
}

// 函数说明：清空 SecurityManager 保存的临时状态或缓存数据。
void SecurityManager::clearSavedPasswords()
{
    m_settingsStore->beginGroup("passwords");
    m_settingsStore->remove("");
    m_settingsStore->endGroup();
}

// 函数说明：实现 SecurityManager::secureDeleteFile 的核心逻辑，供当前模块调用。
bool SecurityManager::secureDeleteFile(const QString &filePath)
{
    return FileEncryption::secureDelete(filePath, m_settings.secureDeletePasses);
}

// 函数说明：实现 SecurityManager::secureDeleteFolder 的核心逻辑，供当前模块调用。
bool SecurityManager::secureDeleteFolder(const QString &folderPath)
{
    QDir dir(folderPath);
    if (!dir.exists()) {
        return true;
    }

    // 递归删除所有文件
    for (const QString &file : dir.entryList(QDir::Files)) {
        if (!secureDeleteFile(dir.absoluteFilePath(file))) {
            return false;
        }
    }

    // 递归删除子目录
    for (const QString &subdir : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (!secureDeleteFolder(dir.absoluteFilePath(subdir))) {
            return false;
        }
    }

    // 删除空目录
    return dir.rmdir(folderPath);
}

// 函数说明：执行 SecurityManager 的启动初始化流程，把延后加载的功能接入主界面。
void SecurityManager::initializeKeychain()
{
    // 平台特定的密钥链初始化
}

// 函数说明：实现 SecurityManager::deriveStorageKey 的核心逻辑，供当前模块调用。
QString SecurityManager::deriveStorageKey() const
{
    // 派生用于本地存储加密的密钥
    // 基于机器特定信息
    return QString();
}


// ==================== EncryptFileDialog ====================

EncryptFileDialog::EncryptFileDialog(const QString &filePath, QWidget *parent)
    : QDialog(parent)
    , m_filePath(filePath)
{
    setWindowTitle(tr("加密文件"));
    setupUI();
}

// 函数说明：初始化 EncryptFileDialog 的 setupUI 相关界面、动作或服务连接。
void EncryptFileDialog::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // 文件信息
    QFileInfo fileInfo(m_filePath);
    QLabel *fileLabel = new QLabel(
        tr("文件: %1 (%2)")
            .arg(fileInfo.fileName())
            .arg(QLocale().formattedDataSize(fileInfo.size())),
        this);
    layout->addWidget(fileLabel);

    // 密码设置
    QGroupBox *passwordGroup = new QGroupBox(tr("密码设置"), this);
    QFormLayout *passwordLayout = new QFormLayout(passwordGroup);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(tr("输入密码"));
    connect(m_passwordEdit, &QLineEdit::textChanged,
            this, &EncryptFileDialog::onPasswordChanged);
    passwordLayout->addRow(tr("密码:"), m_passwordEdit);

    m_confirmEdit = new QLineEdit(this);
    m_confirmEdit->setEchoMode(QLineEdit::Password);
    m_confirmEdit->setPlaceholderText(tr("确认密码"));
    connect(m_confirmEdit, &QLineEdit::textChanged,
            this, &EncryptFileDialog::validateInput);
    passwordLayout->addRow(tr("确认:"), m_confirmEdit);

    // 密码强度
    QHBoxLayout *strengthLayout = new QHBoxLayout();
    m_strengthLabel = new QLabel(tr("强度:"), this);
    strengthLayout->addWidget(m_strengthLabel);
    m_strengthBar = new QProgressBar(this);
    m_strengthBar->setRange(0, 100);
    m_strengthBar->setTextVisible(false);
    m_strengthBar->setFixedHeight(8);
    strengthLayout->addWidget(m_strengthBar);
    passwordLayout->addRow(strengthLayout);

    m_hintEdit = new QLineEdit(this);
    m_hintEdit->setPlaceholderText(tr("可选，帮助您记住密码"));
    passwordLayout->addRow(tr("密码提示:"), m_hintEdit);

    layout->addWidget(passwordGroup);

    // 生物特征
    if (BiometricAuth::instance().isHardwareAvailable()) {
        m_biometricCheck = new QCheckBox(
            tr("允许使用 %1 解锁").arg(BiometricAuth::platformName()), this);
        layout->addWidget(m_biometricCheck);
    } else {
        m_biometricCheck = nullptr;
    }

    // 自毁设置
    QGroupBox *selfDestructGroup = new QGroupBox(tr("自毁设置"), this);
    QVBoxLayout *selfDestructLayout = new QVBoxLayout(selfDestructGroup);

    m_selfDestructCheck = new QCheckBox(tr("启用自毁模式"), this);
    connect(m_selfDestructCheck, &QCheckBox::toggled,
            this, &EncryptFileDialog::onSelfDestructToggled);
    selfDestructLayout->addWidget(m_selfDestructCheck);

    QFormLayout *selfDestructForm = new QFormLayout();

    m_expiresEdit = new QDateTimeEdit(QDateTime::currentDateTime().addDays(7), this);
    m_expiresEdit->setCalendarPopup(true);
    m_expiresEdit->setEnabled(false);
    selfDestructForm->addRow(tr("过期时间:"), m_expiresEdit);

    m_maxOpenCountSpin = new QSpinBox(this);
    m_maxOpenCountSpin->setRange(0, 1000);
    m_maxOpenCountSpin->setSpecialValueText(tr("无限制"));
    m_maxOpenCountSpin->setEnabled(false);
    selfDestructForm->addRow(tr("最大打开次数:"), m_maxOpenCountSpin);

    m_idleTimeoutSpin = new QSpinBox(this);
    m_idleTimeoutSpin->setRange(0, 60);
    m_idleTimeoutSpin->setSuffix(tr(" 分钟"));
    m_idleTimeoutSpin->setSpecialValueText(tr("不锁定"));
    m_idleTimeoutSpin->setEnabled(false);
    selfDestructForm->addRow(tr("空闲超时:"), m_idleTimeoutSpin);

    m_secureDeleteCheck = new QCheckBox(tr("过期后安全删除（多次覆写）"), this);
    m_secureDeleteCheck->setChecked(true);
    m_secureDeleteCheck->setEnabled(false);

    selfDestructLayout->addLayout(selfDestructForm);
    selfDestructLayout->addWidget(m_secureDeleteCheck);
    layout->addWidget(selfDestructGroup);

    // 按钮
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttonBox->button(QDialogButtonBox::Ok)->setText(tr("加密"));
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(m_buttonBox);

    setMinimumWidth(400);
}

// 函数说明：实现 EncryptFileDialog::password 的核心逻辑，供当前模块调用。
QString EncryptFileDialog::password() const
{
    return m_passwordEdit->text();
}

// 函数说明：实现 EncryptFileDialog::options 的核心逻辑，供当前模块调用。
FileEncryption::EncryptionOptions EncryptFileDialog::options() const
{
    FileEncryption::EncryptionOptions opts;
    opts.hint = m_hintEdit->text();
    opts.requireBiometric = m_biometricCheck && m_biometricCheck->isChecked();
    
    if (m_selfDestructCheck->isChecked()) {
        opts.expiresAt = m_expiresEdit->dateTime();
    }
    
    return opts;
}

// 函数说明：实现 EncryptFileDialog::selfDestructOptions 的核心逻辑，供当前模块调用。
SecureDocument::SelfDestructOptions EncryptFileDialog::selfDestructOptions() const
{
    SecureDocument::SelfDestructOptions opts;
    opts.enabled = m_selfDestructCheck->isChecked();
    
    if (opts.enabled) {
        opts.expiresAt = m_expiresEdit->dateTime();
        opts.maxOpenCount = m_maxOpenCountSpin->value();
        opts.idleTimeoutMinutes = m_idleTimeoutSpin->value();
        opts.secureDelete = m_secureDeleteCheck->isChecked();
        opts.deleteOnExpire = true;
    }
    
    return opts;
}

// 函数说明：响应 EncryptFileDialog 收到的信号或异步回调，并更新界面状态。
void EncryptFileDialog::onPasswordChanged()
{
    int strength = calculatePasswordStrength(m_passwordEdit->text());
    m_strengthBar->setValue(strength);
    
    if (strength < 33) {
        m_strengthBar->setStyleSheet("QProgressBar::chunk { background: #f44336; }");
        m_strengthLabel->setText(tr("强度: 弱"));
    } else if (strength < 66) {
        m_strengthBar->setStyleSheet("QProgressBar::chunk { background: #ff9800; }");
        m_strengthLabel->setText(tr("强度: 中"));
    } else {
        m_strengthBar->setStyleSheet("QProgressBar::chunk { background: #4caf50; }");
        m_strengthLabel->setText(tr("强度: 强"));
    }
    
    validateInput();
}

// 函数说明：响应 EncryptFileDialog 收到的信号或异步回调，并更新界面状态。
void EncryptFileDialog::onSelfDestructToggled(bool enabled)
{
    m_expiresEdit->setEnabled(enabled);
    m_maxOpenCountSpin->setEnabled(enabled);
    m_idleTimeoutSpin->setEnabled(enabled);
    m_secureDeleteCheck->setEnabled(enabled);
}

// 函数说明：实现 EncryptFileDialog::validateInput 的核心逻辑，供当前模块调用。
void EncryptFileDialog::validateInput()
{
    QString password = m_passwordEdit->text();
    QString confirm = m_confirmEdit->text();
    
    bool valid = !password.isEmpty() && 
                 password == confirm &&
                 password.length() >= 8;
    
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

// 函数说明：实现 EncryptFileDialog::calculatePasswordStrength 的核心逻辑，供当前模块调用。
int EncryptFileDialog::calculatePasswordStrength(const QString &password) const
{
    if (password.isEmpty()) return 0;
    
    int score = 0;
    
    // 长度
    score += qMin(password.length() * 4, 40);
    
    // 字符类型
    bool hasLower = false, hasUpper = false, hasDigit = false, hasSpecial = false;
    for (const QChar &c : password) {
        if (c.isLower()) hasLower = true;
        else if (c.isUpper()) hasUpper = true;
        else if (c.isDigit()) hasDigit = true;
        else hasSpecial = true;
    }
    
    if (hasLower) score += 10;
    if (hasUpper) score += 15;
    if (hasDigit) score += 15;
    if (hasSpecial) score += 20;
    
    return qMin(score, 100);
}


// ==================== DecryptFileDialog ====================

DecryptFileDialog::DecryptFileDialog(const QString &filePath, QWidget *parent)
    : QDialog(parent)
    , m_filePath(filePath)
    , m_useBiometric(false)
{
    setWindowTitle(tr("解密文件"));
    setupUI();
    loadFileInfo();
}

// 函数说明：初始化 DecryptFileDialog 的 setupUI 相关界面、动作或服务连接。
void DecryptFileDialog::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    m_fileInfoLabel = new QLabel(this);
    layout->addWidget(m_fileInfoLabel);

    m_hintLabel = new QLabel(this);
    m_hintLabel->setStyleSheet("color: #666; font-style: italic;");
    layout->addWidget(m_hintLabel);

    m_expirationLabel = new QLabel(this);
    layout->addWidget(m_expirationLabel);

    QFormLayout *formLayout = new QFormLayout();
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(tr("输入密码"));
    formLayout->addRow(tr("密码:"), m_passwordEdit);
    layout->addLayout(formLayout);

    // 生物特征按钮
    if (BiometricAuth::instance().isHardwareAvailable()) {
        m_biometricBtn = new QPushButton(
            tr("使用 %1").arg(BiometricAuth::platformName()), this);
        connect(m_biometricBtn, &QPushButton::clicked,
                this, &DecryptFileDialog::onBiometricClicked);
        layout->addWidget(m_biometricBtn);
    } else {
        m_biometricBtn = nullptr;
    }

    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttonBox->button(QDialogButtonBox::Ok)->setText(tr("解密"));
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(m_buttonBox);

    setMinimumWidth(350);
}

// 函数说明：加载 DecryptFileDialog 需要的数据、配置或外部资源。
void DecryptFileDialog::loadFileInfo()
{
    QFileInfo fileInfo(m_filePath);
    m_fileInfoLabel->setText(tr("文件: %1").arg(fileInfo.fileName()));

    FileEncryption::EncryptionMetadata metadata;
    FileEncryption().readMetadata(m_filePath, metadata);

    if (!metadata.hint.isEmpty()) {
        m_hintLabel->setText(tr("提示: %1").arg(metadata.hint));
        m_hintLabel->show();
    } else {
        m_hintLabel->hide();
    }

    if (metadata.expiresAt.isValid()) {
        int remaining = QDateTime::currentDateTime().secsTo(metadata.expiresAt);
        if (remaining <= 0) {
            m_expirationLabel->setText(tr("⚠️ 文件已过期"));
            m_expirationLabel->setStyleSheet("color: #f44336;");
        } else if (remaining < 3600) {
            m_expirationLabel->setText(
                tr("⚠️ 将在 %1 分钟后过期").arg(remaining / 60));
            m_expirationLabel->setStyleSheet("color: #ff9800;");
        } else {
            m_expirationLabel->setText(
                tr("过期时间: %1").arg(metadata.expiresAt.toString()));
        }
        m_expirationLabel->show();
    } else {
        m_expirationLabel->hide();
    }

    // 如果需要生物特征但不可用，禁用按钮
    if (metadata.requireBiometric && m_biometricBtn) {
        if (!BiometricAuth::instance().isEnrolled()) {
            m_biometricBtn->setEnabled(false);
            m_biometricBtn->setToolTip(tr("需要先配置生物特征"));
        }
    }
}

// 函数说明：实现 DecryptFileDialog::password 的核心逻辑，供当前模块调用。
QString DecryptFileDialog::password() const
{
    return m_passwordEdit->text();
}

// 函数说明：实现 DecryptFileDialog::useBiometric 的核心逻辑，供当前模块调用。
bool DecryptFileDialog::useBiometric() const
{
    return m_useBiometric;
}

// 函数说明：响应 DecryptFileDialog 收到的信号或异步回调，并更新界面状态。
void DecryptFileDialog::onBiometricClicked()
{
    emit biometricRequested();
    
    BiometricAuth::instance().authenticate(
        tr("解密文件"),
        [this](BiometricAuth::AuthResult result, const QString &) {
            onBiometricResult(result == BiometricAuth::AuthResult::Success);
        }
    );
}

// 函数说明：响应 DecryptFileDialog 收到的信号或异步回调，并更新界面状态。
void DecryptFileDialog::onBiometricResult(bool success)
{
    if (success) {
        m_useBiometric = true;
        accept();
    } else {
        QMessageBox::warning(this, tr("认证失败"), 
            tr("生物特征认证失败，请使用密码"));
    }
}

