// 文件说明：app-static\security\biometricauth.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "biometricauth.h"

#include <QEventLoop>
#include <QTimer>

// 函数说明：构造 BiometricAuth 对象，初始化本模块需要的状态、界面和资源。
BiometricAuth::BiometricAuth(QObject *parent)
    : QObject(parent)
    , m_isAuthenticating(false)
{
}

// 函数说明：销毁 BiometricAuth 对象，释放本模块持有的资源。
BiometricAuth::~BiometricAuth()
{
    if (m_isAuthenticating) {
        cancelAuthentication();
    }
}

// 函数说明：实现 BiometricAuth::instance 的核心逻辑，供当前模块调用。
BiometricAuth& BiometricAuth::instance()
{
    static BiometricAuth *instance = createPlatformBiometricAuth();
    return *instance;
}

// 函数说明：实现 BiometricAuth::checkAvailability 的核心逻辑，供当前模块调用。
BiometricAuth::Availability BiometricAuth::checkAvailability() const
{
    // 基类默认实现
    return Availability::NotAvailable;
}

// 函数说明：实现 BiometricAuth::supportedAuthTypes 的核心逻辑，供当前模块调用。
QList<BiometricAuth::AuthType> BiometricAuth::supportedAuthTypes() const
{
    return { AuthType::Password };
}

// 函数说明：实现 BiometricAuth::preferredAuthType 的核心逻辑，供当前模块调用。
BiometricAuth::AuthType BiometricAuth::preferredAuthType() const
{
    QList<AuthType> types = supportedAuthTypes();
    if (types.isEmpty()) {
        return AuthType::None;
    }
    
    // 优先级：指纹 > 面部 > PIN > 密码
    if (types.contains(AuthType::Fingerprint)) {
        return AuthType::Fingerprint;
    }
    if (types.contains(AuthType::FaceRecognition)) {
        return AuthType::FaceRecognition;
    }
    if (types.contains(AuthType::PIN)) {
        return AuthType::PIN;
    }
    
    return types.first();
}

// 函数说明：判断 BiometricAuth 当前是否满足指定状态。
bool BiometricAuth::isHardwareAvailable() const
{
    Availability avail = checkAvailability();
    return avail == Availability::Available || 
           avail == Availability::NotConfigured;
}

// 函数说明：判断 BiometricAuth 当前是否满足指定状态。
bool BiometricAuth::isEnrolled() const
{
    return checkAvailability() == Availability::Available;
}

// 函数说明：实现 BiometricAuth::authenticate 的核心逻辑，供当前模块调用。
void BiometricAuth::authenticate(const QString &reason, AuthCallback callback)
{
    if (m_isAuthenticating) {
        if (callback) {
            callback(AuthResult::Error, tr("认证已在进行中"));
        }
        return;
    }

    m_currentCallback = callback;
    m_isAuthenticating = true;

    // 检查可用性
    Availability avail = checkAvailability();
    if (avail != Availability::Available) {
        emitResult(AuthResult::NotAvailable, 
                   tr("生物特征认证不可用"));
        return;
    }

    // 调用平台特定实现
    platformAuthenticate(reason);
}

// 函数说明：实现 BiometricAuth::authenticateSync 的核心逻辑，供当前模块调用。
BiometricAuth::AuthResult BiometricAuth::authenticateSync(
    const QString &reason, 
    int timeoutMs)
{
    AuthResult result = AuthResult::Error;
    QString message;

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    connect(&timer, &QTimer::timeout, &loop, [&]() {
        result = AuthResult::Timeout;
        cancelAuthentication();
        loop.quit();
    });

    authenticate(reason, [&](AuthResult r, const QString &msg) {
        result = r;
        message = msg;
        loop.quit();
    });

    timer.start(timeoutMs);
    loop.exec();
    timer.stop();

    return result;
}

// 函数说明：实现 BiometricAuth::cancelAuthentication 的核心逻辑，供当前模块调用。
void BiometricAuth::cancelAuthentication()
{
    if (m_isAuthenticating) {
        m_isAuthenticating = false;
        emit authenticationCanceled();
        
        if (m_currentCallback) {
            m_currentCallback(AuthResult::Canceled, tr("用户取消"));
            m_currentCallback = nullptr;
        }
    }
}

// 函数说明：实现 BiometricAuth::platformName 的核心逻辑，供当前模块调用。
QString BiometricAuth::platformName()
{
#ifdef Q_OS_WIN
    return "Windows Hello";
#elif defined(Q_OS_MAC)
    return "Touch ID";
#elif defined(Q_OS_LINUX)
    return "Linux";
#else
    return "Unknown";
#endif
}

// 函数说明：实现 BiometricAuth::platformAuthenticate 的核心逻辑，供当前模块调用。
void BiometricAuth::platformAuthenticate(const QString &reason)
{
    Q_UNUSED(reason)
    // 基类默认实现：不支持
    emitResult(AuthResult::NotAvailable, tr("平台不支持生物特征认证"));
}

// 函数说明：实现 BiometricAuth::emitResult 的核心逻辑，供当前模块调用。
void BiometricAuth::emitResult(AuthResult result, const QString &message)
{
    m_isAuthenticating = false;
    
    emit authenticationCompleted(result, message);
    
    if (m_currentCallback) {
        m_currentCallback(result, message);
        m_currentCallback = nullptr;
    }
}

// 默认工厂函数（非特定平台，或 MinGW 不支持 WinRT）
#if (!defined(Q_OS_WIN) && !defined(Q_OS_MACOS) && !defined(Q_OS_DARWIN) && !defined(Q_OS_APPLE)) || (defined(Q_OS_WIN) && !defined(HAS_WINRT_BIOMETRIC))
BiometricAuth* createPlatformBiometricAuth(QObject *parent)
{
    return new BiometricAuth(parent);
}
#endif

