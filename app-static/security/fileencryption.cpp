// 文件说明：app-static\security\fileencryption.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "fileencryption.h"

#include <QFile>
#include <QDataStream>
#include <QDateTime>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageAuthenticationCode>

#ifndef NO_OPENSSL
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#endif

// 常量定义
static const int SALT_LENGTH = 32;
static const int IV_LENGTH = 12;      // GCM 推荐 12 字节
static const int TAG_LENGTH = 16;     // GCM 认证标签
static const int KEY_LENGTH = 32;     // AES-256

// 函数说明：构造 FileEncryption 对象，初始化本模块需要的状态、界面和资源。
FileEncryption::FileEncryption(QObject *parent)
    : QObject(parent)
{
    // OpenSSL 3.0+ 自动初始化，无需手动调用
    // 保留空构造函数以便未来扩展
}

// 函数说明：销毁 FileEncryption 对象，释放本模块持有的资源。
FileEncryption::~FileEncryption()
{
    // OpenSSL 3.0+ 自动清理，无需手动调用
    // EVP_cleanup() 和 ERR_free_strings() 已弃用
}

// 函数说明：实现 FileEncryption::encryptFile 的核心逻辑，供当前模块调用。
FileEncryption::ErrorCode FileEncryption::encryptFile(
    const QString &inputPath,
    const QString &outputPath,
    const QString &password,
    const EncryptionOptions &options)
{
    // 读取输入文件
    QFile inputFile(inputPath);
    if (!inputFile.exists()) {
        m_lastError = tr("文件不存在: %1").arg(inputPath);
        return ErrorCode::FileNotFound;
    }

    if (!inputFile.open(QIODevice::ReadOnly)) {
        m_lastError = tr("无法读取文件: %1").arg(inputPath);
        return ErrorCode::FileReadError;
    }

    QByteArray plainData = inputFile.readAll();
    inputFile.close();

    // 准备元数据
    EncryptionMetadata metadata;
    metadata.originalFileName = QFileInfo(inputPath).fileName();
    metadata.originalSize = plainData.size();
    metadata.createdAt = QDateTime::currentDateTime();
    metadata.expiresAt = options.expiresAt;
    metadata.hint = options.hint;
    metadata.requireBiometric = options.requireBiometric;

    // 生成随机 salt 和 IV
    QByteArray salt = generateRandomBytes(SALT_LENGTH);
    QByteArray iv = generateRandomBytes(IV_LENGTH);

    // 派生密钥
    QByteArray key = deriveKey(password, salt, options.pbkdf2Iterations);

    // 序列化元数据
    QByteArray metadataBytes = serializeMetadata(metadata);

    // 加密元数据
    QByteArray encryptedMetadata, metadataTag;
    QByteArray metadataIv = generateRandomBytes(IV_LENGTH);
    if (!aesGcmEncrypt(metadataBytes, key, metadataIv, QByteArray(), 
                       encryptedMetadata, metadataTag)) {
        m_lastError = tr("元数据加密失败");
        return ErrorCode::OpenSSLError;
    }

    // 加密文件内容
    QByteArray encryptedData, contentTag;
    if (!aesGcmEncrypt(plainData, key, iv, QByteArray(), encryptedData, contentTag)) {
        m_lastError = tr("内容加密失败");
        return ErrorCode::OpenSSLError;
    }

    emit progressChanged(50);

    // 写入输出文件
    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        m_lastError = tr("无法写入文件: %1").arg(outputPath);
        return ErrorCode::FileWriteError;
    }

    QDataStream stream(&outputFile);
    stream.setByteOrder(QDataStream::BigEndian);

    // 写入文件头
    stream.writeRawData(MAGIC, 8);
    stream << VERSION;
    quint16 flags = static_cast<quint16>(options.algorithm);
    stream << flags;

    // 写入加密参数
    stream.writeRawData(salt.constData(), SALT_LENGTH);
    stream.writeRawData(iv.constData(), IV_LENGTH);
    stream.writeRawData(contentTag.constData(), TAG_LENGTH);

    // 写入元数据
    stream.writeRawData(metadataIv.constData(), IV_LENGTH);
    stream.writeRawData(metadataTag.constData(), TAG_LENGTH);
    quint32 metadataLen = encryptedMetadata.size();
    stream << metadataLen;
    stream.writeRawData(encryptedMetadata.constData(), encryptedMetadata.size());

    // 写入加密内容
    stream.writeRawData(encryptedData.constData(), encryptedData.size());

    outputFile.close();

    emit progressChanged(100);
    emit encryptionCompleted(ErrorCode::Success);

    return ErrorCode::Success;
}

// 函数说明：实现 FileEncryption::decryptFile 的核心逻辑，供当前模块调用。
FileEncryption::ErrorCode FileEncryption::decryptFile(
    const QString &inputPath,
    const QString &outputPath,
    const QString &password)
{
    QFile inputFile(inputPath);
    if (!inputFile.exists()) {
        m_lastError = tr("文件不存在: %1").arg(inputPath);
        return ErrorCode::FileNotFound;
    }

    if (!inputFile.open(QIODevice::ReadOnly)) {
        m_lastError = tr("无法读取文件: %1").arg(inputPath);
        return ErrorCode::FileReadError;
    }

    QByteArray fileData = inputFile.readAll();
    inputFile.close();

    QByteArray plainData;
    ErrorCode code = decryptData(fileData, plainData, password);

    if (code != ErrorCode::Success) {
        return code;
    }

    // 写入输出文件
    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        m_lastError = tr("无法写入文件: %1").arg(outputPath);
        return ErrorCode::FileWriteError;
    }

    outputFile.write(plainData);
    outputFile.close();

    emit decryptionCompleted(ErrorCode::Success);
    return ErrorCode::Success;
}

// 函数说明：实现 FileEncryption::encryptData 的核心逻辑，供当前模块调用。
FileEncryption::ErrorCode FileEncryption::encryptData(
    const QByteArray &plainData,
    QByteArray &encryptedData,
    const QString &password,
    const EncryptionOptions &options)
{
    // 生成随机 salt 和 IV
    QByteArray salt = generateRandomBytes(SALT_LENGTH);
    QByteArray iv = generateRandomBytes(IV_LENGTH);

    // 派生密钥
    QByteArray key = deriveKey(password, salt, options.pbkdf2Iterations);

    // 准备元数据
    EncryptionMetadata metadata;
    metadata.originalSize = plainData.size();
    metadata.createdAt = QDateTime::currentDateTime();
    metadata.expiresAt = options.expiresAt;
    metadata.hint = options.hint;
    metadata.requireBiometric = options.requireBiometric;

    QByteArray metadataBytes = serializeMetadata(metadata);

    // 加密元数据
    QByteArray encryptedMetadata, metadataTag;
    QByteArray metadataIv = generateRandomBytes(IV_LENGTH);
    if (!aesGcmEncrypt(metadataBytes, key, metadataIv, QByteArray(), 
                       encryptedMetadata, metadataTag)) {
        m_lastError = tr("元数据加密失败");
        return ErrorCode::OpenSSLError;
    }

    // 加密内容
    QByteArray ciphertext, contentTag;
    if (!aesGcmEncrypt(plainData, key, iv, QByteArray(), ciphertext, contentTag)) {
        m_lastError = tr("内容加密失败");
        return ErrorCode::OpenSSLError;
    }

    // 组装加密数据
    QDataStream stream(&encryptedData, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    stream.writeRawData(MAGIC, 8);
    stream << VERSION;
    quint16 flags = static_cast<quint16>(options.algorithm);
    stream << flags;

    stream.writeRawData(salt.constData(), SALT_LENGTH);
    stream.writeRawData(iv.constData(), IV_LENGTH);
    stream.writeRawData(contentTag.constData(), TAG_LENGTH);

    stream.writeRawData(metadataIv.constData(), IV_LENGTH);
    stream.writeRawData(metadataTag.constData(), TAG_LENGTH);
    quint32 metadataLen = encryptedMetadata.size();
    stream << metadataLen;
    stream.writeRawData(encryptedMetadata.constData(), encryptedMetadata.size());

    stream.writeRawData(ciphertext.constData(), ciphertext.size());

    return ErrorCode::Success;
}

// 函数说明：实现 FileEncryption::decryptData 的核心逻辑，供当前模块调用。
FileEncryption::ErrorCode FileEncryption::decryptData(
    const QByteArray &encryptedData,
    QByteArray &plainData,
    const QString &password)
{
    if (!isEncryptedData(encryptedData)) {
        m_lastError = tr("不是有效的加密文件格式");
        return ErrorCode::InvalidFormat;
    }

    QDataStream stream(encryptedData);
    stream.setByteOrder(QDataStream::BigEndian);

    // 读取文件头
    char magic[8];
    stream.readRawData(magic, 8);

    quint16 version, flags;
    stream >> version >> flags;

    if (version > VERSION) {
        m_lastError = tr("不支持的文件版本");
        return ErrorCode::InvalidFormat;
    }

    // 读取加密参数
    QByteArray salt(SALT_LENGTH, 0);
    QByteArray iv(IV_LENGTH, 0);
    QByteArray contentTag(TAG_LENGTH, 0);

    stream.readRawData(salt.data(), SALT_LENGTH);
    stream.readRawData(iv.data(), IV_LENGTH);
    stream.readRawData(contentTag.data(), TAG_LENGTH);

    // 读取元数据
    QByteArray metadataIv(IV_LENGTH, 0);
    QByteArray metadataTag(TAG_LENGTH, 0);
    stream.readRawData(metadataIv.data(), IV_LENGTH);
    stream.readRawData(metadataTag.data(), TAG_LENGTH);

    quint32 metadataLen;
    stream >> metadataLen;

    QByteArray encryptedMetadata(metadataLen, 0);
    stream.readRawData(encryptedMetadata.data(), metadataLen);

    // 计算剩余数据长度（加密内容）
    int headerSize = 8 + 2 + 2 + SALT_LENGTH + IV_LENGTH + TAG_LENGTH + 
                     IV_LENGTH + TAG_LENGTH + 4 + metadataLen;
    QByteArray ciphertext = encryptedData.mid(headerSize);

    // 派生密钥
    QByteArray key = deriveKey(password, salt);

    // 解密元数据
    QByteArray metadataBytes;
    if (!aesGcmDecrypt(encryptedMetadata, key, metadataIv, QByteArray(), 
                       metadataTag, metadataBytes)) {
        m_lastError = tr("密码错误或文件已损坏");
        return ErrorCode::InvalidPassword;
    }

    EncryptionMetadata metadata;
    if (!deserializeMetadata(metadataBytes, metadata)) {
        m_lastError = tr("元数据解析失败");
        return ErrorCode::InvalidFormat;
    }

    // 检查是否过期
    if (isExpired(metadata)) {
        m_lastError = tr("文件已过期");
        return ErrorCode::Expired;
    }

    emit progressChanged(50);

    // 解密内容
    if (!aesGcmDecrypt(ciphertext, key, iv, QByteArray(), contentTag, plainData)) {
        m_lastError = tr("文件已被篡改或密码错误");
        return ErrorCode::IntegrityError;
    }

    emit progressChanged(100);
    return ErrorCode::Success;
}

// 函数说明：判断 FileEncryption 当前是否满足指定状态。
bool FileEncryption::isEncryptedFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    char magic[8];
    if (file.read(magic, 8) != 8) {
        return false;
    }

    return memcmp(magic, MAGIC, 8) == 0;
}

// 函数说明：判断 FileEncryption 当前是否满足指定状态。
bool FileEncryption::isEncryptedData(const QByteArray &data)
{
    if (data.size() < 8) {
        return false;
    }
    return memcmp(data.constData(), MAGIC, 8) == 0;
}

// 函数说明：读取 FileEncryption 的配置或数据，并同步到运行时状态。
FileEncryption::ErrorCode FileEncryption::readMetadata(
    const QString &filePath, 
    EncryptionMetadata &metadata)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return ErrorCode::FileReadError;
    }

    QByteArray data = file.readAll();
    return readMetadata(data, metadata);
}

// 函数说明：读取 FileEncryption 的配置或数据，并同步到运行时状态。
FileEncryption::ErrorCode FileEncryption::readMetadata(
    const QByteArray &data, 
    EncryptionMetadata &metadata)
{
    if (!isEncryptedData(data)) {
        return ErrorCode::InvalidFormat;
    }

    // 元数据是加密的，无法在没有密码的情况下读取完整内容
    // 这里只能读取基本信息（如文件头中的标志位）
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);

    char magic[8];
    stream.readRawData(magic, 8);

    quint16 version, flags;
    stream >> version >> flags;

    // 可以从 flags 中提取一些信息
    metadata.requireBiometric = (flags & 0x8000) != 0;

    return ErrorCode::Success;
}

// 函数说明：判断 FileEncryption 当前是否满足指定状态。
bool FileEncryption::isExpired(const EncryptionMetadata &metadata)
{
    if (!metadata.expiresAt.isValid()) {
        return false;  // 没有设置过期时间
    }
    return QDateTime::currentDateTime() > metadata.expiresAt;
}

// 函数说明：实现 FileEncryption::secureDelete 的核心逻辑，供当前模块调用。
bool FileEncryption::secureDelete(const QString &filePath, int passes)
{
    QFile file(filePath);
    if (!file.exists()) {
        return true;
    }

    qint64 fileSize = file.size();

    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    // 多次覆写
    for (int pass = 0; pass < passes; ++pass) {
        file.seek(0);
        
        QByteArray pattern;
        switch (pass % 3) {
            case 0:
                pattern = QByteArray(4096, 0x00);  // 全0
                break;
            case 1:
                pattern = QByteArray(4096, 0xFF);  // 全1
                break;
            case 2:
                pattern = generateRandomBytes(4096);  // 随机
                break;
        }

        qint64 written = 0;
        while (written < fileSize) {
            qint64 toWrite = qMin(qint64(4096), fileSize - written);
            file.write(pattern.left(toWrite));
            written += toWrite;
        }

        file.flush();
    }

    file.close();

    // 删除文件
    return QFile::remove(filePath);
}

// 函数说明：根据当前数据生成 FileEncryption 需要的输出结果。
QString FileEncryption::generatePassword(int length)
{
    const QString chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
    
    QString password;
    password.reserve(length);

    for (int i = 0; i < length; ++i) {
        int index = QRandomGenerator::global()->bounded(chars.length());
        password.append(chars[index]);
    }

    return password;
}

// 函数说明：实现 FileEncryption::deriveKey 的核心逻辑，供当前模块调用。
QByteArray FileEncryption::deriveKey(
    const QString &password,
    const QByteArray &salt,
    int iterations)
{
    QByteArray key(KEY_LENGTH, 0);
    QByteArray passwordBytes = password.toUtf8();

#ifdef NO_OPENSSL
    // Qt-based PBKDF2-HMAC-SHA256 implementation
    QByteArray U = salt + QByteArray(4, 0);
    U[salt.size() + 3] = 1;  // block counter = 1

    QByteArray hmac = QMessageAuthenticationCode::hash(
        U, passwordBytes, QCryptographicHash::Sha256);
    QByteArray result = hmac;

    for (int i = 1; i < iterations; ++i) {
        hmac = QMessageAuthenticationCode::hash(
            hmac, passwordBytes, QCryptographicHash::Sha256);
        for (int j = 0; j < result.size(); ++j) {
            result[j] = result[j] ^ hmac[j];
        }
    }
    key = result.left(KEY_LENGTH);
#else
    PKCS5_PBKDF2_HMAC(
        passwordBytes.constData(), passwordBytes.size(),
        reinterpret_cast<const unsigned char*>(salt.constData()), salt.size(),
        iterations,
        EVP_sha256(),
        KEY_LENGTH,
        reinterpret_cast<unsigned char*>(key.data())
    );
#endif

    return key;
}

// 函数说明：实现 FileEncryption::aesGcmEncrypt 的核心逻辑，供当前模块调用。
bool FileEncryption::aesGcmEncrypt(
    const QByteArray &plaintext,
    const QByteArray &key,
    const QByteArray &iv,
    const QByteArray &aad,
    QByteArray &ciphertext,
    QByteArray &tag)
{
#ifdef NO_OPENSSL
    // Qt-based authenticated encryption using HMAC-SHA256 stream cipher
    // Generate keystream using HMAC(key, iv + counter)
    ciphertext.resize(plaintext.size());
    QByteArray counterBlock = iv;

    int offset = 0;
    quint32 counter = 0;
    while (offset < plaintext.size()) {
        // Build counter block: iv + 4-byte counter
        QByteArray counterBytes(4, 0);
        counterBytes[0] = (counter >> 24) & 0xFF;
        counterBytes[1] = (counter >> 16) & 0xFF;
        counterBytes[2] = (counter >> 8) & 0xFF;
        counterBytes[3] = counter & 0xFF;

        QByteArray block = QMessageAuthenticationCode::hash(
            iv + counterBytes, key, QCryptographicHash::Sha256);

        int blockLen = qMin(block.size(), plaintext.size() - offset);
        for (int i = 0; i < blockLen; ++i) {
            ciphertext[offset + i] = plaintext[offset + i] ^ block[i];
        }
        offset += blockLen;
        ++counter;
    }

    // Generate authentication tag: HMAC(key, aad + ciphertext)
    QByteArray tagInput = aad + ciphertext;
    tag = QMessageAuthenticationCode::hash(
        tagInput, key, QCryptographicHash::Sha256).left(TAG_LENGTH);

    return true;
#else
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return false;
    }

    bool success = true;
    int len = 0;
    int ciphertextLen = 0;

    // 初始化加密
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        success = false;
        goto cleanup;
    }

    // 设置 IV 长度
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv.size(), nullptr) != 1) {
        success = false;
        goto cleanup;
    }

    // 设置密钥和 IV
    if (EVP_EncryptInit_ex(ctx, nullptr, nullptr,
            reinterpret_cast<const unsigned char*>(key.constData()),
            reinterpret_cast<const unsigned char*>(iv.constData())) != 1) {
        success = false;
        goto cleanup;
    }

    // 添加 AAD（如果有）
    if (!aad.isEmpty()) {
        if (EVP_EncryptUpdate(ctx, nullptr, &len,
                reinterpret_cast<const unsigned char*>(aad.constData()),
                aad.size()) != 1) {
            success = false;
            goto cleanup;
        }
    }

    // 加密数据
    ciphertext.resize(plaintext.size() + 16);
    if (EVP_EncryptUpdate(ctx,
            reinterpret_cast<unsigned char*>(ciphertext.data()), &len,
            reinterpret_cast<const unsigned char*>(plaintext.constData()),
            plaintext.size()) != 1) {
        success = false;
        goto cleanup;
    }
    ciphertextLen = len;

    // 完成加密
    if (EVP_EncryptFinal_ex(ctx,
            reinterpret_cast<unsigned char*>(ciphertext.data()) + len, &len) != 1) {
        success = false;
        goto cleanup;
    }
    ciphertextLen += len;
    ciphertext.resize(ciphertextLen);

    // 获取认证标签
    tag.resize(TAG_LENGTH);
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_LENGTH,
            reinterpret_cast<void*>(tag.data())) != 1) {
        success = false;
        goto cleanup;
    }

cleanup:
    EVP_CIPHER_CTX_free(ctx);
    return success;
#endif
}

// 函数说明：实现 FileEncryption::aesGcmDecrypt 的核心逻辑，供当前模块调用。
bool FileEncryption::aesGcmDecrypt(
    const QByteArray &ciphertext,
    const QByteArray &key,
    const QByteArray &iv,
    const QByteArray &aad,
    const QByteArray &tag,
    QByteArray &plaintext)
{
#ifdef NO_OPENSSL
    // Qt-based authenticated decryption
    // Verify authentication tag first
    QByteArray tagInput = aad + ciphertext;
    QByteArray expectedTag = QMessageAuthenticationCode::hash(
        tagInput, key, QCryptographicHash::Sha256).left(TAG_LENGTH);

    if (expectedTag != tag) {
        return false;  // Authentication failed
    }

    // Decrypt using same keystream
    plaintext.resize(ciphertext.size());
    int offset = 0;
    quint32 counter = 0;
    while (offset < ciphertext.size()) {
        QByteArray counterBytes(4, 0);
        counterBytes[0] = (counter >> 24) & 0xFF;
        counterBytes[1] = (counter >> 16) & 0xFF;
        counterBytes[2] = (counter >> 8) & 0xFF;
        counterBytes[3] = counter & 0xFF;

        QByteArray block = QMessageAuthenticationCode::hash(
            iv + counterBytes, key, QCryptographicHash::Sha256);

        int blockLen = qMin(block.size(), ciphertext.size() - offset);
        for (int i = 0; i < blockLen; ++i) {
            plaintext[offset + i] = ciphertext[offset + i] ^ block[i];
        }
        offset += blockLen;
        ++counter;
    }

    return true;
#else
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return false;
    }

    bool success = true;
    int len = 0;
    int plaintextLen = 0;

    // 初始化解密
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        success = false;
        goto cleanup;
    }

    // 设置 IV 长度
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv.size(), nullptr) != 1) {
        success = false;
        goto cleanup;
    }

    // 设置密钥和 IV
    if (EVP_DecryptInit_ex(ctx, nullptr, nullptr,
            reinterpret_cast<const unsigned char*>(key.constData()),
            reinterpret_cast<const unsigned char*>(iv.constData())) != 1) {
        success = false;
        goto cleanup;
    }

    // 添加 AAD（如果有）
    if (!aad.isEmpty()) {
        if (EVP_DecryptUpdate(ctx, nullptr, &len,
                reinterpret_cast<const unsigned char*>(aad.constData()),
                aad.size()) != 1) {
            success = false;
            goto cleanup;
        }
    }

    // 解密数据
    plaintext.resize(ciphertext.size());
    if (EVP_DecryptUpdate(ctx,
            reinterpret_cast<unsigned char*>(plaintext.data()), &len,
            reinterpret_cast<const unsigned char*>(ciphertext.constData()),
            ciphertext.size()) != 1) {
        success = false;
        goto cleanup;
    }
    plaintextLen = len;

    // 设置认证标签
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_LENGTH,
            const_cast<char*>(tag.constData())) != 1) {
        success = false;
        goto cleanup;
    }

    // 验证并完成解密
    if (EVP_DecryptFinal_ex(ctx,
            reinterpret_cast<unsigned char*>(plaintext.data()) + len, &len) != 1) {
        // 认证失败 - 文件被篡改或密码错误
        success = false;
        goto cleanup;
    }
    plaintextLen += len;
    plaintext.resize(plaintextLen);

cleanup:
    EVP_CIPHER_CTX_free(ctx);
    return success;
#endif
}

// 函数说明：根据当前数据生成 FileEncryption 需要的输出结果。
QByteArray FileEncryption::generateRandomBytes(int length)
{
    QByteArray bytes(length, 0);
#ifdef NO_OPENSSL
    QRandomGenerator *rng = QRandomGenerator::global();
    for (int i = 0; i < length; ++i) {
        bytes[i] = static_cast<char>(rng->bounded(256));
    }
#else
    RAND_bytes(reinterpret_cast<unsigned char*>(bytes.data()), length);
#endif
    return bytes;
}

// 函数说明：实现 FileEncryption::serializeMetadata 的核心逻辑，供当前模块调用。
QByteArray FileEncryption::serializeMetadata(const EncryptionMetadata &metadata)
{
    QJsonObject obj;
    obj["originalFileName"] = metadata.originalFileName;
    obj["originalSize"] = metadata.originalSize;
    obj["createdAt"] = metadata.createdAt.toString(Qt::ISODate);
    if (metadata.expiresAt.isValid()) {
        obj["expiresAt"] = metadata.expiresAt.toString(Qt::ISODate);
    }
    obj["hint"] = metadata.hint;
    obj["requireBiometric"] = metadata.requireBiometric;
    if (!metadata.customData.isEmpty()) {
        obj["customData"] = QString::fromLatin1(metadata.customData.toBase64());
    }

    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

// 函数说明：实现 FileEncryption::deserializeMetadata 的核心逻辑，供当前模块调用。
bool FileEncryption::deserializeMetadata(
    const QByteArray &data, 
    EncryptionMetadata &metadata)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        return false;
    }

    QJsonObject obj = doc.object();
    metadata.originalFileName = obj["originalFileName"].toString();
    metadata.originalSize = obj["originalSize"].toVariant().toLongLong();
    metadata.createdAt = QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODate);
    if (obj.contains("expiresAt")) {
        metadata.expiresAt = QDateTime::fromString(obj["expiresAt"].toString(), Qt::ISODate);
    }
    metadata.hint = obj["hint"].toString();
    metadata.requireBiometric = obj["requireBiometric"].toBool();
    if (obj.contains("customData")) {
        metadata.customData = QByteArray::fromBase64(
            obj["customData"].toString().toLatin1());
    }

    return true;
}

