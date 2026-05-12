// 文件说明：app-static\security\securedocument.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "securedocument.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QRandomGenerator>

// ==================== SecureDocument ====================

SecureDocument::SecureDocument(QObject *parent)
    : QObject(parent)
    , m_idleTimer(new QTimer(this))
    , m_expirationTimer(new QTimer(this))
    , m_encryption(new FileEncryption(this))
{
    m_info.state = State::Closed;
    m_info.openCount = 0;
    m_info.isEncrypted = false;
    m_info.accessMode = AccessMode::PasswordOnly;

    m_idleTimer->setSingleShot(true);
    connect(m_idleTimer, &QTimer::timeout, this, &SecureDocument::onIdleTimeout);

    m_expirationTimer->setInterval(1000);  // 每秒检查
    connect(m_expirationTimer, &QTimer::timeout, this, &SecureDocument::checkExpiration);
}

// 函数说明：销毁 SecureDocument 对象，释放本模块持有的资源。
SecureDocument::~SecureDocument()
{
    if (m_info.state == State::Open) {
        close();
    }
    clearSecureMemory();
}

// 函数说明：创建 SecureDocument 需要的对象、记录或输出内容。
bool SecureDocument::create(const QString &filePath,
                            const QString &password,
                            AccessMode accessMode,
                            const SelfDestructOptions &selfDestruct)
{
    if (m_info.state != State::Closed) {
        m_lastError = tr("请先关闭当前文档");
        return false;
    }

    m_info.filePath = filePath;
    m_info.originalName = QFileInfo(filePath).fileName();
    m_info.originalSize = 0;
    m_info.createdAt = QDateTime::currentDateTime();
    m_info.lastAccessedAt = m_info.createdAt;
    m_info.openCount = 1;
    m_info.state = State::Open;
    m_info.isEncrypted = true;
    m_info.accessMode = accessMode;
    m_info.selfDestruct = selfDestruct;

    m_password = password;
    m_content.clear();

    startIdleTimer();
    if (selfDestruct.enabled) {
        startExpirationChecker();
    }

    emit stateChanged(State::Open);
    return true;
}

// 函数说明：打开 SecureDocument 对应的文件、资源或功能入口。
bool SecureDocument::open(const QString &filePath, const QString &password)
{
    if (m_info.state != State::Closed) {
        m_lastError = tr("请先关闭当前文档");
        return false;
    }

    // 检查文件是否存在
    if (!QFile::exists(filePath)) {
        m_lastError = tr("文件不存在");
        return false;
    }

    m_info.state = State::Opening;
    emit stateChanged(State::Opening);

    // 检查是否是加密文件
    if (!FileEncryption::isEncryptedFile(filePath)) {
        // 普通文件，直接读取
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_lastError = tr("无法打开文件");
            m_info.state = State::Closed;
            emit stateChanged(State::Closed);
            return false;
        }

        m_content = QString::fromUtf8(file.readAll());
        file.close();

        m_info.filePath = filePath;
        m_info.originalName = QFileInfo(filePath).fileName();
        m_info.originalSize = m_content.size();
        m_info.lastAccessedAt = QDateTime::currentDateTime();
        m_info.openCount++;
        m_info.state = State::Open;
        m_info.isEncrypted = false;

        emit stateChanged(State::Open);
        return true;
    }

    // 加密文件
    QByteArray plainData;

    // 读取元数据检查过期
    FileEncryption::EncryptionMetadata metadata;
    m_encryption->readMetadata(filePath, metadata);

    if (FileEncryption::isExpired(metadata)) {
        m_lastError = tr("文档已过期");
        m_info.state = State::Expired;
        emit stateChanged(State::Expired);
        emit documentExpired();

        // 如果设置了过期删除
        if (m_info.selfDestruct.deleteOnExpire) {
            performSelfDestruct();
        }
        return false;
    }

    // 检查最大打开次数
    if (m_info.selfDestruct.maxOpenCount > 0 && 
        m_info.openCount >= m_info.selfDestruct.maxOpenCount) {
        m_lastError = tr("已达到最大打开次数");
        m_info.state = State::Expired;
        emit stateChanged(State::Expired);
        
        if (m_info.selfDestruct.deleteOnExpire) {
            performSelfDestruct();
        }
        return false;
    }

    // 读取加密文件内容到内存
    QFile encFile(filePath);
    if (!encFile.open(QIODevice::ReadOnly)) {
        m_lastError = tr("无法读取文件");
        m_info.state = State::Closed;
        emit stateChanged(State::Closed);
        return false;
    }

    QByteArray encData = encFile.readAll();
    encFile.close();

    FileEncryption::ErrorCode code = m_encryption->decryptData(encData, plainData, password);

    if (code != FileEncryption::ErrorCode::Success) {
        m_lastError = m_encryption->lastErrorString();
        m_info.state = State::Closed;
        emit stateChanged(State::Closed);
        return false;
    }

    m_content = QString::fromUtf8(plainData);
    m_password = password;

    m_info.filePath = filePath;
    m_info.originalName = metadata.originalFileName;
    m_info.originalSize = metadata.originalSize;
    m_info.createdAt = metadata.createdAt;
    m_info.lastAccessedAt = QDateTime::currentDateTime();
    m_info.openCount++;
    m_info.state = State::Open;
    m_info.isEncrypted = true;
    m_info.selfDestruct.enabled = metadata.expiresAt.isValid();
    m_info.selfDestruct.expiresAt = metadata.expiresAt;

    startIdleTimer();
    if (m_info.selfDestruct.enabled) {
        startExpirationChecker();
    }

    emit stateChanged(State::Open);
    return true;
}

// 函数说明：保存 SecureDocument 当前状态，保证用户修改可以持久化。
bool SecureDocument::save()
{
    if (m_info.state != State::Open) {
        m_lastError = tr("文档未打开");
        return false;
    }

    if (!m_info.isEncrypted) {
        // 保存为普通文件
        QFile file(m_info.filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            m_lastError = tr("无法保存文件");
            return false;
        }

        file.write(m_content.toUtf8());
        file.close();
        return true;
    }

    // 保存为加密文件
    QByteArray plainData = m_content.toUtf8();
    QByteArray encData;

    FileEncryption::EncryptionOptions options;
    options.expiresAt = m_info.selfDestruct.expiresAt;
    options.requireBiometric = (m_info.accessMode == AccessMode::BiometricOnly ||
                                m_info.accessMode == AccessMode::PasswordAndBiometric);

    FileEncryption::ErrorCode code = m_encryption->encryptData(
        plainData, encData, m_password, options);

    if (code != FileEncryption::ErrorCode::Success) {
        m_lastError = m_encryption->lastErrorString();
        return false;
    }

    QFile file(m_info.filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        m_lastError = tr("无法保存文件");
        return false;
    }

    file.write(encData);
    file.close();

    m_info.originalSize = plainData.size();
    return true;
}

// 函数说明：保存 SecureDocument 当前状态，保证用户修改可以持久化。
bool SecureDocument::saveAs(const QString &newPath, const QString &newPassword)
{
    QString oldPath = m_info.filePath;
    QString oldPassword = m_password;

    m_info.filePath = newPath;
    if (!newPassword.isEmpty()) {
        m_password = newPassword;
    }

    if (!save()) {
        m_info.filePath = oldPath;
        m_password = oldPassword;
        return false;
    }

    return true;
}

// 函数说明：关闭 SecureDocument 相关窗口或资源，并处理必要的保存确认。
bool SecureDocument::close()
{
    if (m_info.state == State::Closed) {
        return true;
    }

    stopIdleTimer();
    stopExpirationChecker();
    clearSecureMemory();

    m_info.state = State::Closed;
    emit stateChanged(State::Closed);
    return true;
}

// 函数说明：实现 SecureDocument::lock 的核心逻辑，供当前模块调用。
bool SecureDocument::lock()
{
    if (m_info.state != State::Open) {
        return false;
    }

    // 清除内存中的内容
    m_content.fill('0');
    m_content.clear();

    m_info.state = State::Locked;
    emit stateChanged(State::Locked);
    return true;
}

// 函数说明：实现 SecureDocument::unlock 的核心逻辑，供当前模块调用。
bool SecureDocument::unlock(const QString &password)
{
    if (m_info.state != State::Locked) {
        return false;
    }

    if (!verifyPassword(password)) {
        m_lastError = tr("密码错误");
        return false;
    }

    // 重新从文件读取
    return open(m_info.filePath, password);
}

// 函数说明：设置 SecureDocument 的运行参数，并触发必要的界面或数据刷新。
void SecureDocument::setContent(const QString &content)
{
    if (m_info.state != State::Open) {
        return;
    }

    m_content = content;
    onActivityDetected();
    emit contentChanged();
}

// 函数说明：设置 SecureDocument 的运行参数，并触发必要的界面或数据刷新。
void SecureDocument::setSelfDestructOptions(const SelfDestructOptions &options)
{
    m_info.selfDestruct = options;

    if (options.enabled) {
        startExpirationChecker();
    } else {
        stopExpirationChecker();
    }
}

// 函数说明：判断 SecureDocument 当前是否满足指定状态。
bool SecureDocument::isExpiringSoon(int warningMinutes) const
{
    if (!m_info.selfDestruct.enabled || !m_info.selfDestruct.expiresAt.isValid()) {
        return false;
    }

    int remaining = remainingSeconds();
    return remaining > 0 && remaining <= warningMinutes * 60;
}

// 函数说明：实现 SecureDocument::remainingSeconds 的核心逻辑，供当前模块调用。
int SecureDocument::remainingSeconds() const
{
    if (!m_info.selfDestruct.expiresAt.isValid()) {
        return -1;  // 永不过期
    }

    return QDateTime::currentDateTime().secsTo(m_info.selfDestruct.expiresAt);
}

// 函数说明：实现 SecureDocument::changePassword 的核心逻辑，供当前模块调用。
bool SecureDocument::changePassword(const QString &oldPassword, const QString &newPassword)
{
    if (!verifyPassword(oldPassword)) {
        m_lastError = tr("原密码错误");
        return false;
    }

    m_password = newPassword;
    return save();
}

// 函数说明：实现 SecureDocument::verifyPassword 的核心逻辑，供当前模块调用。
bool SecureDocument::verifyPassword(const QString &password) const
{
    // 使用恒定时间比较防止时序攻击
    if (password.length() != m_password.length()) {
        // 仍然进行完整比较以保持恒定时间
        volatile int dummy = 0;
        for (int i = 0; i < qMax(password.length(), m_password.length()); ++i) {
            dummy ^= 1;
        }
        return false;
    }

    volatile int diff = 0;
    for (int i = 0; i < password.length(); ++i) {
        diff |= (password[i].unicode() ^ m_password[i].unicode());
    }

    return diff == 0;
}

// 函数说明：实现 SecureDocument::authenticateWithBiometric 的核心逻辑，供当前模块调用。
void SecureDocument::authenticateWithBiometric(std::function<void(bool)> callback)
{
    BiometricAuth::instance().authenticate(
        tr("解锁文档: %1").arg(m_info.originalName),
        [callback](BiometricAuth::AuthResult result, const QString &) {
            if (callback) {
                callback(result == BiometricAuth::AuthResult::Success);
            }
        }
    );
}

// 函数说明：响应 SecureDocument 收到的信号或异步回调，并更新界面状态。
void SecureDocument::onIdleTimeout()
{
    if (m_info.selfDestruct.idleTimeoutMinutes > 0) {
        lock();
        emit idleTimeout();
    }
}

// 函数说明：实现 SecureDocument::checkExpiration 的核心逻辑，供当前模块调用。
void SecureDocument::checkExpiration()
{
    if (!m_info.selfDestruct.enabled) {
        return;
    }

    int remaining = remainingSeconds();

    if (remaining <= 0) {
        m_info.state = State::Expired;
        emit stateChanged(State::Expired);
        emit documentExpired();

        if (m_info.selfDestruct.deleteOnExpire) {
            performSelfDestruct();
        }
    } else if (remaining <= 300) {  // 5分钟警告
        emit expirationWarning(remaining);
    }
}

// 函数说明：响应 SecureDocument 收到的信号或异步回调，并更新界面状态。
void SecureDocument::onActivityDetected()
{
    // 重置空闲计时器
    if (m_info.selfDestruct.idleTimeoutMinutes > 0) {
        startIdleTimer();
    }
}

// 函数说明：启动 SecureDocument 的异步任务、会话或后台流程。
void SecureDocument::startIdleTimer()
{
    if (m_info.selfDestruct.idleTimeoutMinutes > 0) {
        m_idleTimer->start(m_info.selfDestruct.idleTimeoutMinutes * 60 * 1000);
    }
}

// 函数说明：停止 SecureDocument 正在运行的任务或会话。
void SecureDocument::stopIdleTimer()
{
    m_idleTimer->stop();
}

// 函数说明：启动 SecureDocument 的异步任务、会话或后台流程。
void SecureDocument::startExpirationChecker()
{
    if (m_info.selfDestruct.enabled && m_info.selfDestruct.expiresAt.isValid()) {
        m_expirationTimer->start();
    }
}

// 函数说明：停止 SecureDocument 正在运行的任务或会话。
void SecureDocument::stopExpirationChecker()
{
    m_expirationTimer->stop();
}

// 函数说明：实现 SecureDocument::performSelfDestruct 的核心逻辑，供当前模块调用。
void SecureDocument::performSelfDestruct()
{
    close();

    bool success;
    if (m_info.selfDestruct.secureDelete) {
        success = FileEncryption::secureDelete(m_info.filePath);
    } else {
        success = QFile::remove(m_info.filePath);
    }

    if (success) {
        emit documentDestroyed();
    }
}

// 函数说明：清空 SecureDocument 保存的临时状态或缓存数据。
void SecureDocument::clearSecureMemory()
{
    // 安全清除内存中的敏感数据
    if (!m_content.isEmpty()) {
        m_content.fill('0');
        m_content.clear();
        m_content.squeeze();
    }

    if (!m_password.isEmpty()) {
        m_password.fill('0');
        m_password.clear();
        m_password.squeeze();
    }
}


// ==================== SecureDocumentCache ====================

SecureDocumentCache& SecureDocumentCache::instance()
{
    static SecureDocumentCache instance;
    return instance;
}

// 函数说明：构造 SecureDocumentCache 对象，初始化本模块需要的状态、界面和资源。
SecureDocumentCache::SecureDocumentCache(QObject *parent)
    : QObject(parent)
{
    // 使用临时目录
    m_cacheDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + 
                 "/cutemarked_secure_cache";
    
    QDir().mkpath(m_cacheDir);

    // 生成临时加密密钥
    m_cacheKey = FileEncryption::generatePassword(32);
}

// 函数说明：销毁 SecureDocumentCache 对象，释放本模块持有的资源。
SecureDocumentCache::~SecureDocumentCache()
{
    clearAllCache();
}

// 函数说明：实现 SecureDocumentCache::cacheDocument 的核心逻辑，供当前模块调用。
QString SecureDocumentCache::cacheDocument(const QString &docId, const QByteArray &content)
{
    QString cachePath = m_cacheDir + "/" + 
        QCryptographicHash::hash(docId.toUtf8(), QCryptographicHash::Sha256).toHex() + 
        ".cache";

    FileEncryption enc;
    QByteArray encData;
    enc.encryptData(content, encData, m_cacheKey);

    QFile file(cachePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(encData);
        file.close();
        m_cacheMap[docId] = cachePath;
        return cachePath;
    }

    return QString();
}

// 函数说明：读取 SecureDocumentCache 当前保存的状态或计算结果。
QByteArray SecureDocumentCache::getCachedDocument(const QString &docId)
{
    if (!m_cacheMap.contains(docId)) {
        return QByteArray();
    }

    QString cachePath = m_cacheMap[docId];
    QFile file(cachePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }

    QByteArray encData = file.readAll();
    file.close();

    FileEncryption enc;
    QByteArray plainData;
    enc.decryptData(encData, plainData, m_cacheKey);

    return plainData;
}

// 函数说明：从 SecureDocumentCache 管理的数据集合中移除指定内容。
bool SecureDocumentCache::removeCachedDocument(const QString &docId)
{
    if (!m_cacheMap.contains(docId)) {
        return false;
    }

    QString cachePath = m_cacheMap[docId];
    bool success = FileEncryption::secureDelete(cachePath);
    m_cacheMap.remove(docId);
    return success;
}

// 函数说明：清空 SecureDocumentCache 保存的临时状态或缓存数据。
void SecureDocumentCache::clearAllCache()
{
    for (const QString &path : m_cacheMap.values()) {
        FileEncryption::secureDelete(path);
    }
    m_cacheMap.clear();

    // 清理整个缓存目录
    QDir cacheDir(m_cacheDir);
    for (const QString &file : cacheDir.entryList(QDir::Files)) {
        FileEncryption::secureDelete(cacheDir.absoluteFilePath(file));
    }

    emit cacheCleared();
}

// 函数说明：设置 SecureDocumentCache 的运行参数，并触发必要的界面或数据刷新。
void SecureDocumentCache::setCacheDirectory(const QString &path)
{
    clearAllCache();
    m_cacheDir = path;
    QDir().mkpath(m_cacheDir);
}

// 函数说明：实现 SecureDocumentCache::cacheSize 的核心逻辑，供当前模块调用。
qint64 SecureDocumentCache::cacheSize() const
{
    qint64 totalSize = 0;
    QDir cacheDir(m_cacheDir);
    for (const QFileInfo &info : cacheDir.entryInfoList(QDir::Files)) {
        totalSize += info.size();
    }
    return totalSize;
}

