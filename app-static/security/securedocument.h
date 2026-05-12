// 文件说明：app-static\security\securedocument.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef SECUREDOCUMENT_H
#define SECUREDOCUMENT_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QTimer>

#include "fileencryption.h"
#include "biometricauth.h"

/**
 * @brief 安全文档管理器 - 自毁模式与加密文档管理
 * 
 * 功能：
 * - 加密文档的打开/保存/关闭
 * - 自毁模式（过期自动删除）
 * - 访问控制（密码 + 生物特征）
 * - 安全缓存管理
 */
class SecureDocument : public QObject
{
    Q_OBJECT

public:
    // 文档状态
    enum class State {
        Closed,
        Opening,
        Open,
        Locked,
        Expired
    };
    Q_ENUM(State)

    // 访问模式
    enum class AccessMode {
        PasswordOnly,       // 仅密码
        BiometricOnly,      // 仅生物特征
        PasswordOrBiometric,// 密码或生物特征
        PasswordAndBiometric// 密码和生物特征（双因素）
    };
    Q_ENUM(AccessMode)

    // 自毁选项
    struct SelfDestructOptions {
        bool enabled;
        QDateTime expiresAt;
        int maxOpenCount;
        int idleTimeoutMinutes;
        bool deleteOnExpire;
        bool secureDelete;
        
        SelfDestructOptions()
            : enabled(false)
            , maxOpenCount(0)
            , idleTimeoutMinutes(0)
            , deleteOnExpire(true)
            , secureDelete(true)
        {}
    };

    // 文档信息
    struct DocumentInfo {
        QString filePath;
        QString originalName;
        qint64 originalSize;
        QDateTime createdAt;
        QDateTime lastAccessedAt;
        int openCount;
        State state;
        bool isEncrypted;
        AccessMode accessMode;
        SelfDestructOptions selfDestruct;
    };

    explicit SecureDocument(QObject *parent = nullptr);
    ~SecureDocument();

    // 文档操作
    bool create(const QString &filePath, 
                const QString &password,
                AccessMode accessMode = AccessMode::PasswordOnly,
                const SelfDestructOptions &selfDestruct = SelfDestructOptions());

    bool open(const QString &filePath, const QString &password);
    bool save();
    bool saveAs(const QString &newPath, const QString &newPassword = QString());
    bool close();
    bool lock();
    bool unlock(const QString &password);

    // 内容访问
    QString content() const { return m_content; }
    void setContent(const QString &content);

    // 文档信息
    DocumentInfo info() const { return m_info; }
    State state() const { return m_info.state; }
    bool isOpen() const { return m_info.state == State::Open; }
    bool isLocked() const { return m_info.state == State::Locked; }
    bool isExpired() const { return m_info.state == State::Expired; }

    // 自毁设置
    void setSelfDestructOptions(const SelfDestructOptions &options);
    SelfDestructOptions selfDestructOptions() const { return m_info.selfDestruct; }

    // 检查是否即将过期
    bool isExpiringSoon(int warningMinutes = 5) const;
    QDateTime expiresAt() const { return m_info.selfDestruct.expiresAt; }
    int remainingSeconds() const;

    // 修改密码
    bool changePassword(const QString &oldPassword, const QString &newPassword);

    // 验证密码
    bool verifyPassword(const QString &password) const;

    // 生物特征认证
    void authenticateWithBiometric(std::function<void(bool)> callback);

    // 获取错误信息
    QString lastError() const { return m_lastError; }

signals:
    // 状态变化
    void stateChanged(State state);
    void contentChanged();
    
    // 自毁警告
    void expirationWarning(int remainingSeconds);
    void documentExpired();
    void documentDestroyed();
    
    // 空闲超时
    void idleTimeout();
    
    // 认证请求
    void authenticationRequired();

private slots:
    void onIdleTimeout();
    void checkExpiration();
    void onActivityDetected();

private:
    void startIdleTimer();
    void stopIdleTimer();
    void startExpirationChecker();
    void stopExpirationChecker();
    void performSelfDestruct();
    void clearSecureMemory();

    DocumentInfo m_info;
    QString m_content;
    QString m_password;  // 加密存储在内存中
    QString m_lastError;
    
    QTimer *m_idleTimer;
    QTimer *m_expirationTimer;
    
    FileEncryption *m_encryption;
};


/**
 * @brief 安全文档缓存管理器
 * 
 * 管理临时解密的文件缓存，确保：
 * - 缓存文件加密存储
 * - 应用退出时安全清理
 * - 定期清理过期缓存
 */
class SecureDocumentCache : public QObject
{
    Q_OBJECT

public:
    static SecureDocumentCache& instance();

    // 添加到缓存
    QString cacheDocument(const QString &docId, const QByteArray &content);
    
    // 从缓存获取
    QByteArray getCachedDocument(const QString &docId);
    
    // 移除缓存
    bool removeCachedDocument(const QString &docId);
    
    // 清理所有缓存
    void clearAllCache();
    
    // 设置缓存目录
    void setCacheDirectory(const QString &path);
    
    // 获取缓存大小
    qint64 cacheSize() const;

signals:
    void cacheCleared();

private:
    explicit SecureDocumentCache(QObject *parent = nullptr);
    ~SecureDocumentCache();

    QString m_cacheDir;
    QMap<QString, QString> m_cacheMap;
    QString m_cacheKey;  // 用于加密缓存的临时密钥
};

#endif // SECUREDOCUMENT_H

