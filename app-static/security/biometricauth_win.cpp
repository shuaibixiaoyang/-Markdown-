// 文件说明：app-static\security\biometricauth_win.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifdef Q_OS_WIN

#include "biometricauth.h"

#include <QDebug>
#include <QCoreApplication>

// Windows 头文件
#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Security.Credentials.UI.h>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Security::Credentials::UI;

/**
 * @brief Windows Hello 生物特征认证实现
 */
class WindowsHelloBiometricAuth : public BiometricAuth
{
    Q_OBJECT

public:
    explicit WindowsHelloBiometricAuth(QObject *parent = nullptr)
        : BiometricAuth(parent)
    {
        // 初始化 WinRT
        winrt::init_apartment();
    }

    ~WindowsHelloBiometricAuth() override
    {
        winrt::uninit_apartment();
    }

    Availability checkAvailability() const override
    {
        try {
            auto result = UserConsentVerifier::CheckAvailabilityAsync().get();
            
            switch (result) {
                case UserConsentVerifierAvailability::Available:
                    return Availability::Available;
                case UserConsentVerifierAvailability::DeviceNotPresent:
                    return Availability::NotAvailable;
                case UserConsentVerifierAvailability::NotConfiguredForUser:
                    return Availability::NotConfigured;
                case UserConsentVerifierAvailability::DisabledByPolicy:
                    return Availability::NotAllowed;
                case UserConsentVerifierAvailability::DeviceBusy:
                    return Availability::TemporaryUnavailable;
                default:
                    return Availability::Unknown;
            }
        } catch (...) {
            return Availability::Unknown;
        }
    }

    QList<AuthType> supportedAuthTypes() const override
    {
        QList<AuthType> types;
        
        if (checkAvailability() == Availability::Available) {
            // Windows Hello 可能支持多种类型，但 API 不区分
            types << AuthType::Fingerprint
                  << AuthType::FaceRecognition
                  << AuthType::PIN;
        }
        
        types << AuthType::Password;  // 总是支持密码回退
        return types;
    }

protected:
    void platformAuthenticate(const QString &reason) override
    {
        // 在后台线程执行 WinRT 异步操作
        QtConcurrent::run([this, reason]() {
            try {
                // 将 QString 转换为 winrt::hstring
                std::wstring reasonW = reason.toStdWString();
                hstring message(reasonW);

                auto result = UserConsentVerifier::RequestVerificationAsync(message).get();

                AuthResult authResult;
                QString resultMessage;

                switch (result) {
                    case UserConsentVerificationResult::Verified:
                        authResult = AuthResult::Success;
                        resultMessage = tr("认证成功");
                        break;
                    case UserConsentVerificationResult::DeviceNotPresent:
                        authResult = AuthResult::NotAvailable;
                        resultMessage = tr("设备不可用");
                        break;
                    case UserConsentVerificationResult::NotConfiguredForUser:
                        authResult = AuthResult::NotAvailable;
                        resultMessage = tr("未配置 Windows Hello");
                        break;
                    case UserConsentVerificationResult::DisabledByPolicy:
                        authResult = AuthResult::NotAvailable;
                        resultMessage = tr("被策略禁用");
                        break;
                    case UserConsentVerificationResult::DeviceBusy:
                        authResult = AuthResult::Error;
                        resultMessage = tr("设备忙");
                        break;
                    case UserConsentVerificationResult::RetriesExhausted:
                        authResult = AuthResult::Lockout;
                        resultMessage = tr("尝试次数过多");
                        break;
                    case UserConsentVerificationResult::Canceled:
                        authResult = AuthResult::Canceled;
                        resultMessage = tr("用户取消");
                        break;
                    default:
                        authResult = AuthResult::Failed;
                        resultMessage = tr("认证失败");
                        break;
                }

                // 在主线程发送结果
                QMetaObject::invokeMethod(this, [this, authResult, resultMessage]() {
                    emitResult(authResult, resultMessage);
                }, Qt::QueuedConnection);

            } catch (const winrt::hresult_error &e) {
                QString errorMsg = QString::fromWCharArray(e.message().c_str());
                QMetaObject::invokeMethod(this, [this, errorMsg]() {
                    emitResult(AuthResult::Error, errorMsg);
                }, Qt::QueuedConnection);
            } catch (...) {
                QMetaObject::invokeMethod(this, [this]() {
                    emitResult(AuthResult::Error, tr("未知错误"));
                }, Qt::QueuedConnection);
            }
        });
    }
};

// Windows 平台工厂函数
BiometricAuth* createPlatformBiometricAuth(QObject *parent)
{
    return new WindowsHelloBiometricAuth(parent);
}

#include "biometricauth_win.moc"

#endif // Q_OS_WIN

