// 文件说明：app-static\security\biometricauth_mac.mm
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "biometricauth.h"

#include <QDebug>
#include <QtConcurrent>

#import <LocalAuthentication/LocalAuthentication.h>
#import <Foundation/Foundation.h>

/**
 * @brief macOS Touch ID / Face ID 生物特征认证实现
 */
class MacBiometricAuth : public BiometricAuth
{
    Q_OBJECT

public:
    explicit MacBiometricAuth(QObject *parent = nullptr)
        : BiometricAuth(parent)
        , m_context(nil)
    {
    }

    ~MacBiometricAuth() override
    {
        if (m_context) {
            [m_context release];
            m_context = nil;
        }
    }

    Availability checkAvailability() const override
    {
        LAContext *context = [[LAContext alloc] init];
        NSError *error = nil;
        
        BOOL canEvaluate = [context canEvaluatePolicy:LAPolicyDeviceOwnerAuthenticationWithBiometrics
                                                error:&error];
        
        [context release];
        
        if (canEvaluate) {
            return Availability::Available;
        }
        
        if (error) {
            switch (error.code) {
                case LAErrorBiometryNotAvailable:
                    return Availability::NotAvailable;
                case LAErrorBiometryNotEnrolled:
                    return Availability::NotConfigured;
                case LAErrorBiometryLockout:
                    return Availability::TemporaryUnavailable;
                default:
                    return Availability::Unknown;
            }
        }
        
        return Availability::Unknown;
    }

    QList<AuthType> supportedAuthTypes() const override
    {
        QList<AuthType> types;
        
        LAContext *context = [[LAContext alloc] init];
        NSError *error = nil;
        
        if ([context canEvaluatePolicy:LAPolicyDeviceOwnerAuthenticationWithBiometrics
                                 error:&error]) {
            // 检查生物特征类型
            if (@available(macOS 10.13.2, *)) {
                switch (context.biometryType) {
                    case LABiometryTypeTouchID:
                        types << AuthType::Fingerprint;
                        break;
                    case LABiometryTypeFaceID:
                        types << AuthType::FaceRecognition;
                        break;
                    default:
                        break;
                }
            } else {
                // 旧版本只有 Touch ID
                types << AuthType::Fingerprint;
            }
        }
        
        [context release];
        
        types << AuthType::Password;  // 总是支持密码回退
        return types;
    }

protected:
    void platformAuthenticate(const QString &reason) override
    {
        // 创建新的认证上下文
        if (m_context) {
            [m_context release];
        }
        m_context = [[LAContext alloc] init];
        
        // 设置本地化回退按钮
        m_context.localizedFallbackTitle = @"使用密码";
        
        NSString *reasonStr = reason.toNSString();
        
        // 在后台线程执行认证
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            [m_context evaluatePolicy:LAPolicyDeviceOwnerAuthenticationWithBiometrics
                      localizedReason:reasonStr
                                reply:^(BOOL success, NSError *error) {
                AuthResult result;
                QString message;
                
                if (success) {
                    result = AuthResult::Success;
                    message = tr("认证成功");
                } else if (error) {
                    switch (error.code) {
                        case LAErrorUserCancel:
                            result = AuthResult::Canceled;
                            message = tr("用户取消");
                            break;
                        case LAErrorUserFallback:
                            // 用户选择使用密码
                            result = AuthResult::Failed;
                            message = tr("请使用密码");
                            break;
                        case LAErrorAuthenticationFailed:
                            result = AuthResult::Failed;
                            message = tr("认证失败");
                            break;
                        case LAErrorBiometryLockout:
                            result = AuthResult::Lockout;
                            message = tr("生物特征已锁定，请使用密码");
                            break;
                        case LAErrorSystemCancel:
                            result = AuthResult::Canceled;
                            message = tr("系统取消");
                            break;
                        default:
                            result = AuthResult::Error;
                            message = QString::fromNSString(error.localizedDescription);
                            break;
                    }
                } else {
                    result = AuthResult::Failed;
                    message = tr("未知错误");
                }
                
                // 在主线程发送结果
                dispatch_async(dispatch_get_main_queue(), ^{
                    emitResult(result, message);
                });
            }];
        });
    }

private:
    LAContext *m_context;
};

// macOS 平台工厂函数
BiometricAuth* createPlatformBiometricAuth(QObject *parent)
{
    return new MacBiometricAuth(parent);
}

#include "biometricauth_mac.moc"

