// 文件说明：app-static\security\fileencryption.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef FILEENCRYPTION_H
#define FILEENCRYPTION_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QFile>
#include <QDateTime>
#include <QFileInfo>

/**
 * @brief 文件加密模块 - AES-256-GCM 透明加密
 * 
 * 特性：
 * - AES-256-GCM 认证加密（防篡改）
 * - PBKDF2 密钥派生（防暴力破解）
 * - 随机 IV/Salt（防重放攻击）
 * - 文件头包含元数据（版本、算法、过期时间等）
 * 
 * 加密文件格式：
 * [Magic: 8 bytes][Version: 2 bytes][Flags: 2 bytes]
 * [Salt: 32 bytes][IV: 12 bytes][Tag: 16 bytes]
 * [Metadata Length: 4 bytes][Encrypted Metadata]
 * [Encrypted Content]
 */
class FileEncryption : public QObject
{
    Q_OBJECT

public:
    // 加密算法
    enum class Algorithm {
        AES_256_GCM,    // 推荐：认证加密
        AES_256_CBC     // 兼容模式
    };
    Q_ENUM(Algorithm)

    // 错误码
    enum class ErrorCode {
        Success = 0,
        FileNotFound,
        FileReadError,
        FileWriteError,
        InvalidPassword,
        InvalidFormat,
        DecryptionFailed,
        IntegrityError,     // 文件被篡改
        Expired,            // 文件已过期
        OpenSSLError,
        Unknown
    };
    Q_ENUM(ErrorCode)

    // 加密元数据
    struct EncryptionMetadata {
        QString originalFileName;
        qint64 originalSize;
        QDateTime createdAt;
        QDateTime expiresAt;        // 过期时间，空表示永不过期
        QString hint;               // 密码提示
        bool requireBiometric;      // 是否需要生物特征验证
        QByteArray customData;      // 自定义数据
    };

    // 加密选项
    struct EncryptionOptions {
        Algorithm algorithm;
        int pbkdf2Iterations;
        QDateTime expiresAt;
        QString hint;
        bool requireBiometric;
        
        EncryptionOptions()
            : algorithm(Algorithm::AES_256_GCM)
            , pbkdf2Iterations(100000)
            , requireBiometric(false)
        {}
    };

    explicit FileEncryption(QObject *parent = nullptr);
    ~FileEncryption();

    // 文件加密/解密
    ErrorCode encryptFile(const QString &inputPath, 
                          const QString &outputPath,
                          const QString &password,
                          const EncryptionOptions &options = EncryptionOptions());

    ErrorCode decryptFile(const QString &inputPath,
                          const QString &outputPath,
                          const QString &password);

    // 内存数据加密/解密
    ErrorCode encryptData(const QByteArray &plainData,
                          QByteArray &encryptedData,
                          const QString &password,
                          const EncryptionOptions &options = EncryptionOptions());

    ErrorCode decryptData(const QByteArray &encryptedData,
                          QByteArray &plainData,
                          const QString &password);

    // 检查文件是否已加密
    static bool isEncryptedFile(const QString &filePath);
    static bool isEncryptedData(const QByteArray &data);

    // 读取加密文件元数据（不需要密码）
    ErrorCode readMetadata(const QString &filePath, EncryptionMetadata &metadata);
    ErrorCode readMetadata(const QByteArray &data, EncryptionMetadata &metadata);

    // 检查文件是否过期
    static bool isExpired(const EncryptionMetadata &metadata);

    // 安全删除文件（多次覆写）
    static bool secureDelete(const QString &filePath, int passes = 3);

    // 生成随机密码
    static QString generatePassword(int length = 16);

    // 获取最后错误信息
    QString lastErrorString() const { return m_lastError; }

signals:
    void progressChanged(int percent);
    void encryptionCompleted(ErrorCode code);
    void decryptionCompleted(ErrorCode code);

private:
    // 密钥派生
    QByteArray deriveKey(const QString &password, 
                         const QByteArray &salt,
                         int iterations = 100000);

    // AES-GCM 加密/解密
    bool aesGcmEncrypt(const QByteArray &plaintext,
                       const QByteArray &key,
                       const QByteArray &iv,
                       const QByteArray &aad,
                       QByteArray &ciphertext,
                       QByteArray &tag);

    bool aesGcmDecrypt(const QByteArray &ciphertext,
                       const QByteArray &key,
                       const QByteArray &iv,
                       const QByteArray &aad,
                       const QByteArray &tag,
                       QByteArray &plaintext);

    // 生成随机字节
    static QByteArray generateRandomBytes(int length);

    // 序列化/反序列化元数据
    QByteArray serializeMetadata(const EncryptionMetadata &metadata);
    bool deserializeMetadata(const QByteArray &data, EncryptionMetadata &metadata);

    QString m_lastError;

    // 魔数和版本
    static constexpr char MAGIC[] = "CMEENCR\0";  // CuteMarkEd Encrypted
    static constexpr quint16 VERSION = 1;
};

#endif // FILEENCRYPTION_H

