// 文件说明：app-static\security\securitymanager.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef SECURITYMANAGER_H
#define SECURITYMANAGER_H

#include <QObject>
#include <QWidget>
#include <QDialog>
#include <QString>
#include <QStringList>
#include <QMap>

#include "fileencryption.h"
#include "biometricauth.h"
#include "securedocument.h"

class QSettings;
class QPushButton;

/**
 * @brief 安全管理器 - 统一管理加密、认证和安全策略
 * 
 * 功能：
 * - 加密/解密文件
 * - 管理受保护的文件夹
 * - 生物特征认证集成
 * - 安全策略配置
 * - 加密文档的快速访问
 */
class SecurityManager : public QObject
{
    Q_OBJECT

public:
    // 安全级别
    enum class SecurityLevel {
        None,           // 无保护
        Password,       // 密码保护
        Biometric,      // 生物特征
        TwoFactor       // 双因素（密码 + 生物特征）
    };
    Q_ENUM(SecurityLevel)

    // 受保护文件夹配置
    struct ProtectedFolder {
        QString path;
        SecurityLevel level;
        bool autoLock;          // 离开时自动锁定
        int lockTimeoutMinutes; // 锁定超时
        bool requireBiometric;  // 需要生物特征
    };

    // 安全设置
    struct SecuritySettings {
        SecurityLevel defaultLevel = SecurityLevel::Password;
        int pbkdf2Iterations = 100000;
        bool rememberPassword = false;
        int passwordTimeout = 15;   // 分钟
        bool enableBiometric = true;
        bool secureDelete = true;
        int secureDeletePasses = 3;
    };

    explicit SecurityManager(QObject *parent = nullptr);
    ~SecurityManager();

    // 单例访问
    static SecurityManager& instance();

    // 文件加密操作
    bool encryptFile(const QString &filePath, 
                     const QString &password,
                     const FileEncryption::EncryptionOptions &options = FileEncryption::EncryptionOptions());
    
    bool decryptFile(const QString &filePath, 
                     const QString &password,
                     const QString &outputPath = QString());
    
    bool decryptFileWithBiometric(const QString &filePath,
                                   const QString &outputPath = QString());

    // 检查文件状态
    bool isFileEncrypted(const QString &filePath) const;
    bool isFileExpired(const QString &filePath) const;
    
    // 获取加密文件信息
    FileEncryption::EncryptionMetadata getFileMetadata(const QString &filePath) const;

    // 受保护文件夹管理
    void addProtectedFolder(const ProtectedFolder &folder);
    void removeProtectedFolder(const QString &path);
    QList<ProtectedFolder> protectedFolders() const;
    bool isPathProtected(const QString &path) const;
    ProtectedFolder getProtectedFolderConfig(const QString &path) const;

    // 文件夹锁定/解锁
    bool lockFolder(const QString &path);
    bool unlockFolder(const QString &path, const QString &password);
    bool unlockFolderWithBiometric(const QString &path);
    bool isFolderLocked(const QString &path) const;

    // 生物特征认证
    bool isBiometricAvailable() const;
    BiometricAuth::Availability biometricAvailability() const;
    void authenticateBiometric(const QString &reason, 
                               std::function<void(bool)> callback);

    // 安全设置
    SecuritySettings settings() const { return m_settings; }
    void setSettings(const SecuritySettings &settings);
    void saveSettings();
    void loadSettings();

    // 密码管理（安全存储）
    bool savePassword(const QString &key, const QString &password);
    QString loadPassword(const QString &key) const;
    bool removePassword(const QString &key);
    void clearSavedPasswords();

    // 安全删除
    bool secureDeleteFile(const QString &filePath);
    bool secureDeleteFolder(const QString &folderPath);

    // 获取错误信息
    QString lastError() const { return m_lastError; }

signals:
    // 文件操作
    void fileEncrypted(const QString &filePath);
    void fileDecrypted(const QString &filePath);
    void fileExpired(const QString &filePath);

    // 文件夹操作
    void folderLocked(const QString &path);
    void folderUnlocked(const QString &path);

    // 认证
    void authenticationRequired(const QString &path);
    void authenticationSucceeded();
    void authenticationFailed(const QString &reason);

    // 进度
    void progressChanged(int percent);

private:
    void initializeKeychain();
    QString deriveStorageKey() const;

    SecuritySettings m_settings;
    QMap<QString, ProtectedFolder> m_protectedFolders;
    QStringList m_lockedFolders;
    QString m_lastError;
    
    FileEncryption *m_encryption;
    QSettings *m_settingsStore;
};


/**
 * @brief 加密文件对话框
 */
class QDialog;
class QLineEdit;
class QCheckBox;
class QDateTimeEdit;
class QSpinBox;
class QComboBox;
class QDialogButtonBox;
class QLabel;
class QProgressBar;

class EncryptFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EncryptFileDialog(const QString &filePath, QWidget *parent = nullptr);

    QString password() const;
    FileEncryption::EncryptionOptions options() const;
    SecureDocument::SelfDestructOptions selfDestructOptions() const;

private slots:
    void onPasswordChanged();
    void onSelfDestructToggled(bool enabled);
    void validateInput();

private:
    void setupUI();
    int calculatePasswordStrength(const QString &password) const;

    QString m_filePath;
    QLineEdit *m_passwordEdit;
    QLineEdit *m_confirmEdit;
    QLabel *m_strengthLabel;
    QProgressBar *m_strengthBar;
    QLineEdit *m_hintEdit;
    QCheckBox *m_biometricCheck;
    QCheckBox *m_selfDestructCheck;
    QDateTimeEdit *m_expiresEdit;
    QSpinBox *m_maxOpenCountSpin;
    QSpinBox *m_idleTimeoutSpin;
    QCheckBox *m_secureDeleteCheck;
    QDialogButtonBox *m_buttonBox;
};


/**
 * @brief 解密文件对话框
 */
class DecryptFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DecryptFileDialog(const QString &filePath, QWidget *parent = nullptr);

    QString password() const;
    bool useBiometric() const;

signals:
    void biometricRequested();

private slots:
    void onBiometricClicked();
    void onBiometricResult(bool success);

private:
    void setupUI();
    void loadFileInfo();

    QString m_filePath;
    QLabel *m_fileInfoLabel;
    QLabel *m_hintLabel;
    QLineEdit *m_passwordEdit;
    QPushButton *m_biometricBtn;
    QLabel *m_expirationLabel;
    QDialogButtonBox *m_buttonBox;
    bool m_useBiometric;
};

#endif // SECURITYMANAGER_H

