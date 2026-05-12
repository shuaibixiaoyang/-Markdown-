// 文件说明：app-static\security\biometricauth.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef BIOMETRICAUTH_H
#define BIOMETRICAUTH_H

#include <QObject>
#include <QString>
#include <functional>

/**
 * @brief 生物特征认证抽象层
 * 
 * 跨平台支持：
 * - Windows: Windows Hello (指纹/面部/PIN)
 * - macOS: Touch ID / Face ID
 * - Linux: 不支持（回退到密码）
 * 
 * 使用方式：
 * BiometricAuth::instance().authenticate("解锁文档", [](bool success) {
 *     if (success) {
 *         // 认证成功
 *     }
 * });
 */
class BiometricAuth : public QObject
{
    Q_OBJECT

public:
    // 认证类型
    enum class AuthType {
        None,           // 不支持
        Fingerprint,    // 指纹
        FaceRecognition,// 面部识别
        Iris,           // 虹膜
        PIN,            // Windows Hello PIN
        Password        // 回退到密码
    };
    Q_ENUM(AuthType)

    // 可用性状态
    enum class Availability {
        Available,          // 可用
        NotAvailable,       // 硬件不支持
        NotConfigured,      // 未配置生物特征
        NotAllowed,         // 策略不允许
        TemporaryUnavailable, // 临时不可用
        Unknown
    };
    Q_ENUM(Availability)

    // 认证结果
    enum class AuthResult {
        Success,
        Failed,
        Canceled,
        Timeout,
        Lockout,        // 多次失败锁定
        NotAvailable,
        Error
    };
    Q_ENUM(AuthResult)

    // 回调类型
    using AuthCallback = std::function<void(AuthResult result, const QString &message)>;

    explicit BiometricAuth(QObject *parent = nullptr);
    virtual ~BiometricAuth();

    // 单例访问
    static BiometricAuth& instance();

    // 检查生物特征认证是否可用
    virtual Availability checkAvailability() const;

    // 获取支持的认证类型
    virtual QList<AuthType> supportedAuthTypes() const;

    // 获取首选认证类型
    virtual AuthType preferredAuthType() const;

    // 是否有硬件支持
    virtual bool isHardwareAvailable() const;

    // 是否已注册生物特征
    virtual bool isEnrolled() const;

    // 执行认证（异步）
    virtual void authenticate(const QString &reason, AuthCallback callback);

    // 执行认证（同步，阻塞）
    virtual AuthResult authenticateSync(const QString &reason, int timeoutMs = 30000);

    // 取消正在进行的认证
    virtual void cancelAuthentication();

    // 获取平台名称
    static QString platformName();

signals:
    // 认证完成
    void authenticationCompleted(AuthResult result, const QString &message);

    // 认证取消
    void authenticationCanceled();

protected:
    // 平台特定实现（由子类覆盖）
    virtual void platformAuthenticate(const QString &reason);

    // 发送结果
    void emitResult(AuthResult result, const QString &message = QString());

    AuthCallback m_currentCallback;
    bool m_isAuthenticating;
};


/**
 * @brief 创建平台特定的生物特征认证实例
 */
BiometricAuth* createPlatformBiometricAuth(QObject *parent = nullptr);

#endif // BIOMETRICAUTH_H

