// 文件说明：app-static\sync\cloudsync.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "cloudsync.h"

#include <QDir>
#include <QFileInfo>
#include <QDirIterator>
#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QXmlStreamReader>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSaveFile>
#include <QStandardPaths>
#include <QSettings>
#include <QDebug>
#include <QEventLoop>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslError>
#include <QSslSocket>
#include <QSysInfo>

#ifdef Q_OS_MAC
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace {

const QString kEncryptedPrefix = QStringLiteral("enc:v1:");

QByteArray deriveSecretKey()
{
    QByteArray seed = QSysInfo::machineUniqueId();
    if (seed.isEmpty()) {
        seed = QSysInfo::machineHostName().toUtf8();
    }
    seed += QCoreApplication::organizationName().toUtf8();
    seed += QCoreApplication::applicationName().toUtf8();
    return QCryptographicHash::hash(seed, QCryptographicHash::Sha256);
}

QByteArray streamCipherXor(const QByteArray &input, const QByteArray &key, const QByteArray &nonce)
{
    QByteArray output(input.size(), '\0');
    QByteArray keystream;
    keystream.reserve(input.size());

    quint32 counter = 0;
    while (keystream.size() < input.size()) {
        const QByteArray material = key + nonce + QByteArray::number(counter++);
        keystream += QCryptographicHash::hash(material, QCryptographicHash::Sha256);
    }

    for (int i = 0; i < input.size(); ++i) {
        output[i] = input[i] ^ keystream[i];
    }
    return output;
}

QString encryptSecret(const QString &plainText)
{
    if (plainText.isEmpty()) {
        return QString();
    }

    QByteArray nonce(16, '\0');
    for (int i = 0; i < nonce.size(); ++i) {
        nonce[i] = static_cast<char>(QRandomGenerator::global()->generate() & 0xFF);
    }

    const QByteArray cipher = streamCipherXor(plainText.toUtf8(), deriveSecretKey(), nonce);
    const QByteArray payload = nonce + cipher;
    const QByteArray encoded = payload.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    return kEncryptedPrefix + QString::fromUtf8(encoded);
}

QString decryptSecret(const QString &storedText)
{
    if (storedText.isEmpty()) {
        return QString();
    }

    if (storedText.startsWith(kEncryptedPrefix)) {
        const QByteArray encoded = storedText.mid(kEncryptedPrefix.size()).toUtf8();
        const QByteArray payload = QByteArray::fromBase64(
            encoded, QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
        if (payload.size() <= 16) {
            return QString();
        }

        const QByteArray nonce = payload.left(16);
        const QByteArray cipher = payload.mid(16);
        return QString::fromUtf8(streamCipherXor(cipher, deriveSecretKey(), nonce));
    }

    const QByteArray legacy = QByteArray::fromBase64(
        storedText.toUtf8(), QByteArray::AbortOnBase64DecodingErrors);
    if (!legacy.isEmpty()) {
        return QString::fromUtf8(legacy);
    }

    // 兼容极旧格式（明文）。
    return storedText;
}

bool isSuccessfulHttpStatus(const int statusCode)
{
    return statusCode >= 200 && statusCode < 300;
}

qint64 parseUnsatisfiedRangeTotal(const QByteArray &contentRangeHeader)
{
    const QByteArray header = contentRangeHeader.trimmed().toLower();
    if (!header.startsWith(QByteArrayLiteral("bytes */"))) {
        return -1;
    }

    const int slash = header.indexOf('/');
    if (slash < 0 || slash + 1 >= header.size()) {
        return -1;
    }

    bool ok = false;
    const qint64 total = header.mid(slash + 1).trimmed().toLongLong(&ok);
    return ok ? total : -1;
}

QString replyFailureMessage(QNetworkReply *reply)
{
    if (!reply) {
        return QObject::tr("网络请求失败");
    }

    const QString sslError = reply->property("sslErrorMessage").toString();
    if (!sslError.isEmpty()) {
        return sslError;
    }

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode > 0) {
        return QObject::tr("HTTP %1: %2").arg(statusCode).arg(reply->errorString());
    }
    return reply->errorString();
}

} // namespace

// 函数说明：构造 CloudSync 对象，初始化本模块需要的状态、界面和资源。
CloudSync::CloudSync(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_autoSyncTimer(new QTimer(this))
    , m_state(SyncState::Idle)
    , m_defaultConflictPolicy(ConflictPolicy::AskUser)
    , m_maxHistorySize(1000)
{
    connect(m_autoSyncTimer, &QTimer::timeout,
            this, &CloudSync::onAutoSyncTimer);
}

// 函数说明：销毁 CloudSync 对象，释放本模块持有的资源。
CloudSync::~CloudSync()
{
    stopSync();
}

// 函数说明：向 CloudSync 管理的数据集合中添加一项内容。
bool CloudSync::addService(const ServiceConfig &config)
{
    if (config.name.isEmpty()) {
        m_lastError = tr("服务名称不能为空");
        return false;
    }

    if (m_services.contains(config.name)) {
        m_lastError = tr("服务 '%1' 已存在").arg(config.name);
        return false;
    }

    m_services[config.name] = config;

    // 创建本地目录
    QDir localDir(config.localPath);
    if (!localDir.exists()) {
        localDir.mkpath(".");
    }

    emit serviceAdded(config.name);

    // 启动自动同步
    if (config.autoSync && !m_autoSyncTimer->isActive()) {
        m_autoSyncTimer->start(config.syncInterval * 1000);
    }

    return true;
}

// 函数说明：从 CloudSync 管理的数据集合中移除指定内容。
bool CloudSync::removeService(const QString &name)
{
    if (!m_services.contains(name)) {
        m_lastError = tr("服务 '%1' 不存在").arg(name);
        return false;
    }

    m_services.remove(name);
    m_fileStateCache.remove(name);

    emit serviceRemoved(name);

    // 检查是否还需要自动同步
    bool needAutoSync = false;
    for (const ServiceConfig &config : m_services) {
        if (config.autoSync) {
            needAutoSync = true;
            break;
        }
    }
    if (!needAutoSync) {
        m_autoSyncTimer->stop();
    }

    return true;
}

// 函数说明：刷新 CloudSync 的内部状态，并同步到相关界面。
bool CloudSync::updateService(const ServiceConfig &config)
{
    if (!m_services.contains(config.name)) {
        return addService(config);
    }

    m_services[config.name] = config;
    return true;
}

// 函数说明：读取 CloudSync 当前保存的状态或计算结果。
CloudSync::ServiceConfig CloudSync::getService(const QString &name) const
{
    return m_services.value(name);
}

// 函数说明：读取 CloudSync 当前保存的状态或计算结果。
QVector<CloudSync::ServiceConfig> CloudSync::getAllServices() const
{
    QVector<ServiceConfig> services;
    for (const ServiceConfig &config : m_services) {
        services.append(config);
    }
    return services;
}

// 函数说明：实现 CloudSync::testConnection 的核心逻辑，供当前模块调用。
bool CloudSync::testConnection(const ServiceConfig &config)
{
    if (config.type == ServiceType::WebDAV) {
        QVector<FileInfo> files;
        return webdavPropfind(config, config.remotePath, files);
    } else if (config.type == ServiceType::LocalFolder) {
        QDir dir(config.remotePath);
        return dir.exists();
    } else if (config.type == ServiceType::iCloud) {
        QString containerPath = icloudContainerPath();
        return !containerPath.isEmpty() && QDir(containerPath).exists();
    }

    return false;
}

// 函数说明：启动 CloudSync 的异步任务、会话或后台流程。
void CloudSync::startSync(const QString &serviceName)
{
    if (m_state == SyncState::Syncing) {
        return;
    }

    m_state = SyncState::Syncing;
    emit stateChanged(m_state);

    if (serviceName.isEmpty()) {
        // 同步所有服务
        for (const ServiceConfig &config : m_services) {
            performSync(config);
        }
    } else {
        if (m_services.contains(serviceName)) {
            performSync(m_services[serviceName]);
        }
    }

    m_state = SyncState::Idle;
    emit stateChanged(m_state);
}

// 函数说明：停止 CloudSync 正在运行的任务或会话。
void CloudSync::stopSync()
{
    if (m_state != SyncState::Syncing) {
        return;
    }

    m_taskQueue.clear();
    m_state = SyncState::Idle;
    emit stateChanged(m_state);
}

// 函数说明：实现 CloudSync::pauseSync 的核心逻辑，供当前模块调用。
void CloudSync::pauseSync()
{
    if (m_state == SyncState::Syncing) {
        m_state = SyncState::Paused;
        emit stateChanged(m_state);
    }
}

// 函数说明：实现 CloudSync::resumeSync 的核心逻辑，供当前模块调用。
void CloudSync::resumeSync()
{
    if (m_state == SyncState::Paused) {
        m_state = SyncState::Syncing;
        emit stateChanged(m_state);
    }
}

// 函数说明：实现 CloudSync::performSync 的核心逻辑，供当前模块调用。
void CloudSync::performSync(const ServiceConfig &config)
{
    emit syncStarted(config.name);

    m_currentService = config.name;

    // 验证本地路径是否存在
    if (!QDir(config.localPath).exists()) {
        m_lastError = tr("本地路径不存在: %1").arg(config.localPath);
        emit syncFailed(config.name, m_lastError);
        return;
    }

    // 验证远程路径（本地文件夹同步时检查目录）
    if (config.type == ServiceType::LocalFolder && !QDir(config.remotePath).exists()) {
        m_lastError = tr("远程路径不存在: %1").arg(config.remotePath);
        emit syncFailed(config.name, m_lastError);
        return;
    }

    // 验证 WebDAV 配置
    if (config.type == ServiceType::WebDAV && config.serverUrl.isEmpty()) {
        m_lastError = tr("WebDAV 服务器地址未配置");
        emit syncFailed(config.name, m_lastError);
        return;
    }

    // 获取本地和远程文件列表
    emit syncProgress(config.name, 5, tr("正在获取文件列表..."));
    QCoreApplication::processEvents();

    QVector<FileInfo> localFiles = getLocalFiles(config.name);
    QVector<FileInfo> remoteFiles = getRemoteFiles(config.name);

    emit syncProgress(config.name, 10, tr("正在比较文件（本地 %1 个，远程 %2 个）...")
                      .arg(localFiles.size()).arg(remoteFiles.size()));
    QCoreApplication::processEvents();

    // 比较并同步
    int uploaded = 0;
    int downloaded = 0;
    compareAndSync(config, localFiles, remoteFiles, uploaded, downloaded);

    emit syncProgress(config.name, 100, tr("同步完成"));
    emit syncCompleted(config.name, uploaded, downloaded);
}

// 函数说明：读取 CloudSync 当前保存的状态或计算结果。
QVector<CloudSync::FileInfo> CloudSync::getLocalFiles(const QString &serviceName)
{
    QVector<FileInfo> files;

    if (!m_services.contains(serviceName)) {
        return files;
    }

    ServiceConfig config = m_services[serviceName];
    QDir baseDir(config.localPath);

    QDirIterator it(config.localPath,
                    QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QString fullPath = it.next();
        QString relativePath = baseDir.relativeFilePath(fullPath);

        if (isExcluded(serviceName, relativePath)) {
            continue;
        }

        QFileInfo fileInfo(fullPath);

        FileInfo info;
        info.path = relativePath;
        info.fullPath = fullPath;
        info.size = fileInfo.size();
        info.modifiedTime = fileInfo.lastModified();
        info.isDirectory = fileInfo.isDir();

        if (!info.isDirectory) {
            info.md5 = calculateMD5(fullPath);
        }

        files.append(info);
    }

    return files;
}

// 函数说明：读取 CloudSync 当前保存的状态或计算结果。
QVector<CloudSync::FileInfo> CloudSync::getRemoteFiles(const QString &serviceName)
{
    QVector<FileInfo> files;

    if (!m_services.contains(serviceName)) {
        return files;
    }

    ServiceConfig config = m_services[serviceName];

    if (config.type == ServiceType::WebDAV) {
        webdavPropfind(config, config.remotePath, files);
    } else if (config.type == ServiceType::LocalFolder) {
        // 对于本地文件夹，远程就是另一个本地路径
        QDir baseDir(config.remotePath);
        QDirIterator it(config.remotePath,
                        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);

        while (it.hasNext()) {
            QString fullPath = it.next();
            QString relativePath = baseDir.relativeFilePath(fullPath);

            if (isExcluded(serviceName, relativePath)) {
                continue;
            }

            QFileInfo fileInfo(fullPath);

            FileInfo info;
            info.path = relativePath;
            info.fullPath = fullPath;
            info.size = fileInfo.size();
            info.modifiedTime = fileInfo.lastModified();
            info.isDirectory = fileInfo.isDir();

            if (!info.isDirectory) {
                info.md5 = calculateMD5(fullPath);
            }

            files.append(info);
        }
    }

    return files;
}

// 函数说明：读取 CloudSync 当前保存的状态或计算结果。
CloudSync::FileState CloudSync::getFileState(const QString &serviceName, const QString &path)
{
    if (m_fileStateCache.contains(serviceName) &&
        m_fileStateCache[serviceName].contains(path)) {
        return m_fileStateCache[serviceName][path].state;
    }
    return FileState::Synced;
}

// 函数说明：实现 CloudSync::compareAndSync 的核心逻辑，供当前模块调用。
void CloudSync::compareAndSync(const ServiceConfig &config,
                                const QVector<FileInfo> &localFiles,
                                const QVector<FileInfo> &remoteFiles,
                                int &uploaded, int &downloaded)
{
    QMap<QString, FileInfo> localMap;
    QMap<QString, FileInfo> remoteMap;

    for (const FileInfo &f : localFiles) {
        localMap[f.path] = f;
    }
    for (const FileInfo &f : remoteFiles) {
        remoteMap[f.path] = f;
    }

    // 本地文件夹同步的复制辅助函数（先删除再复制，解决 QFile::copy 不能覆盖的问题）
    auto localFolderCopy = [](const QString &src, const QString &dst) -> bool {
        QDir().mkpath(QFileInfo(dst).path());
        if (QFile::exists(dst)) {
            QFile::remove(dst);
        }
        return QFile::copy(src, dst);
    };

    // 检查本地文件
    for (auto it = localMap.begin(); it != localMap.end(); ++it) {
        const QString &path = it.key();
        FileInfo &local = it.value();

        if (remoteMap.contains(path)) {
            // 文件在两边都存在
            const FileInfo &remote = remoteMap[path];

            if (detectConflict(local, remote)) {
                // 冲突
                ConflictInfo conflict;
                conflict.path = path;
                conflict.localFile = local;
                conflict.remoteFile = remote;
                conflict.detectedTime = QDateTime::currentDateTime();

                handleConflict(conflict);
            } else if (shouldUpload(local, remote)) {
                // 上传
                bool success = false;
                if (config.type == ServiceType::WebDAV) {
                    success = webdavPut(config, local.fullPath, config.remotePath + "/" + path);
                } else if (config.type == ServiceType::LocalFolder) {
                    success = localFolderCopy(local.fullPath, config.remotePath + "/" + path);
                }
                addSyncRecord(path, "upload", success);
                if (success) ++uploaded;
            } else if (shouldDownload(local, remote)) {
                // 下载
                bool success = false;
                if (config.type == ServiceType::WebDAV) {
                    success = webdavGet(config, config.remotePath + "/" + path, local.fullPath);
                } else if (config.type == ServiceType::LocalFolder) {
                    success = localFolderCopy(config.remotePath + "/" + path, local.fullPath);
                }
                addSyncRecord(path, "download", success);
                if (success) ++downloaded;
            }
        } else {
            // 本地新文件，上传
            if (!local.isDirectory) {
                bool success = false;
                if (config.type == ServiceType::WebDAV) {
                    success = webdavPut(config, local.fullPath, config.remotePath + "/" + path);
                } else if (config.type == ServiceType::LocalFolder) {
                    success = localFolderCopy(local.fullPath, config.remotePath + "/" + path);
                }
                addSyncRecord(path, "upload", success);
                if (success) ++uploaded;
            }
        }
    }

    // 检查远程新文件
    for (auto it = remoteMap.begin(); it != remoteMap.end(); ++it) {
        const QString &path = it.key();
        const FileInfo &remote = it.value();

        if (!localMap.contains(path)) {
            // 远程新文件，下载
            if (!remote.isDirectory) {
                bool success = false;
                QString localPath = config.localPath + "/" + path;

                if (config.type == ServiceType::WebDAV) {
                    QDir().mkpath(QFileInfo(localPath).path());
                    success = webdavGet(config, config.remotePath + "/" + path, localPath);
                } else if (config.type == ServiceType::LocalFolder) {
                    success = localFolderCopy(remote.fullPath, localPath);
                }
                addSyncRecord(path, "download", success);
                if (success) ++downloaded;
            }
        }
    }
}

// 函数说明：实现 CloudSync::shouldUpload 的核心逻辑，供当前模块调用。
bool CloudSync::shouldUpload(const FileInfo &local, const FileInfo &remote)
{
    // 如果 MD5 不同且本地更新
    if (local.md5 != remote.md5 && local.modifiedTime > remote.modifiedTime) {
        return true;
    }
    return false;
}

// 函数说明：实现 CloudSync::shouldDownload 的核心逻辑，供当前模块调用。
bool CloudSync::shouldDownload(const FileInfo &local, const FileInfo &remote)
{
    // 如果 MD5 不同且远程更新
    if (local.md5 != remote.md5 && remote.modifiedTime > local.modifiedTime) {
        return true;
    }
    return false;
}

// 函数说明：实现 CloudSync::detectConflict 的核心逻辑，供当前模块调用。
bool CloudSync::detectConflict(const FileInfo &local, const FileInfo &remote)
{
    // 如果 MD5 不同且修改时间相近（5分钟内），可能是冲突
    if (local.md5 != remote.md5) {
        qint64 timeDiff = qAbs(local.modifiedTime.secsTo(remote.modifiedTime));
        if (timeDiff < 300) {  // 5分钟
            return true;
        }
    }
    return false;
}

// 函数说明：处理 CloudSync 接收到的事件、请求或用户操作。
void CloudSync::handleConflict(const ConflictInfo &conflict)
{
    m_conflicts.append(conflict);
    emit conflictDetected(conflict);

    // 根据默认策略处理
    if (m_defaultConflictPolicy != ConflictPolicy::AskUser) {
        resolveConflict(conflict.path, m_defaultConflictPolicy);
    }
}

// 函数说明：实现 CloudSync::resolveConflict 的核心逻辑，供当前模块调用。
bool CloudSync::resolveConflict(const QString &path, ConflictPolicy policy)
{
    // 查找冲突
    int conflictIndex = -1;
    for (int i = 0; i < m_conflicts.size(); ++i) {
        if (m_conflicts[i].path == path) {
            conflictIndex = i;
            break;
        }
    }

    if (conflictIndex < 0) {
        return false;
    }

    ConflictInfo conflict = m_conflicts[conflictIndex];

    switch (policy) {
        case ConflictPolicy::KeepLocal:
            // 上传本地版本覆盖远程
            uploadFile(m_currentService, conflict.localFile.fullPath);
            break;

        case ConflictPolicy::KeepRemote:
            // 下载远程版本覆盖本地
            downloadFile(m_currentService, conflict.remoteFile.path);
            break;

        case ConflictPolicy::KeepBoth: {
            // 重命名本地文件，下载远程文件
            QString conflictPath = conflict.localFile.fullPath;
            QString baseName = QFileInfo(conflictPath).baseName();
            QString suffix = QFileInfo(conflictPath).suffix();
            QString dir = QFileInfo(conflictPath).path();

            QString newPath = dir + "/" + baseName + "_conflict_" +
                              QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") +
                              "." + suffix;
            QFile::rename(conflictPath, newPath);
            downloadFile(m_currentService, conflict.remoteFile.path);
            break;
        }

        case ConflictPolicy::KeepNewer:
            if (conflict.localFile.modifiedTime > conflict.remoteFile.modifiedTime) {
                uploadFile(m_currentService, conflict.localFile.fullPath);
            } else {
                downloadFile(m_currentService, conflict.remoteFile.path);
            }
            break;

        default:
            return false;
    }

    m_conflicts.removeAt(conflictIndex);
    return true;
}

// 函数说明：实现 CloudSync::uploadFile 的核心逻辑，供当前模块调用。
bool CloudSync::uploadFile(const QString &serviceName, const QString &localPath)
{
    if (!m_services.contains(serviceName)) {
        return false;
    }

    ServiceConfig config = m_services[serviceName];
    QString relativePath = getRelativePath(localPath, config.localPath);

    emit fileStateChanged(relativePath, FileState::Uploading);

    bool success = false;
    if (config.type == ServiceType::WebDAV) {
        success = webdavPut(config, localPath, config.remotePath + "/" + relativePath);
    } else if (config.type == ServiceType::LocalFolder) {
        QString remotePath = config.remotePath + "/" + relativePath;
        QDir().mkpath(QFileInfo(remotePath).path());
        if (QFile::exists(remotePath)) {
            QFile::remove(remotePath);
        }
        success = QFile::copy(localPath, remotePath);
    }

    emit fileStateChanged(relativePath, success ? FileState::Synced : FileState::Error);
    addSyncRecord(relativePath, "upload", success);

    return success;
}

// 函数说明：实现 CloudSync::downloadFile 的核心逻辑，供当前模块调用。
bool CloudSync::downloadFile(const QString &serviceName, const QString &remotePath)
{
    if (!m_services.contains(serviceName)) {
        return false;
    }

    ServiceConfig config = m_services[serviceName];
    QString localPath = config.localPath + "/" + remotePath;

    emit fileStateChanged(remotePath, FileState::Downloading);

    QDir().mkpath(QFileInfo(localPath).path());

    bool success = false;
    if (config.type == ServiceType::WebDAV) {
        success = webdavGet(config, config.remotePath + "/" + remotePath, localPath);
    } else if (config.type == ServiceType::LocalFolder) {
        if (QFile::exists(localPath)) {
            QFile::remove(localPath);
        }
        success = QFile::copy(config.remotePath + "/" + remotePath, localPath);
    }

    emit fileStateChanged(remotePath, success ? FileState::Synced : FileState::Error);
    addSyncRecord(remotePath, "download", success);

    return success;
}

// 函数说明：删除 CloudSync 管理的指定数据或资源。
bool CloudSync::deleteRemoteFile(const QString &serviceName, const QString &remotePath)
{
    if (!m_services.contains(serviceName)) {
        return false;
    }

    ServiceConfig config = m_services[serviceName];

    bool success = false;
    if (config.type == ServiceType::WebDAV) {
        success = webdavDelete(config, config.remotePath + "/" + remotePath);
    } else if (config.type == ServiceType::LocalFolder) {
        success = QFile::remove(config.remotePath + "/" + remotePath);
    }

    addSyncRecord(remotePath, "delete", success);
    return success;
}

// 函数说明：实现 CloudSync::normalizeFingerprint 的核心逻辑，供当前模块调用。
QString CloudSync::normalizeFingerprint(const QString &fingerprint)
{
    QString normalized = fingerprint.trimmed().toLower();
    normalized.remove(QRegularExpression(QStringLiteral("[^0-9a-f]")));
    return normalized;
}

// 函数说明：实现 CloudSync::prepareWebDavRequest 的核心逻辑，供当前模块调用。
bool CloudSync::prepareWebDavRequest(const ServiceConfig &config, QNetworkRequest &request)
{
    request.setRawHeader("Authorization", createBasicAuthHeader(config.username, config.password));

    const QUrl url = request.url();
    const bool https = url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0;
    if (!https) {
        if (config.verifySslCertificates) {
            m_lastError = tr("启用 SSL 证书验证时必须使用 HTTPS 地址: %1").arg(url.toString());
            return false;
        }
        return true;
    }

    QSslConfiguration sslConfig = request.sslConfiguration();
    if (sslConfig.isNull()) {
        sslConfig = QSslConfiguration::defaultConfiguration();
    }
    sslConfig.setProtocol(QSsl::SecureProtocols);
    sslConfig.setPeerVerifyMode(config.verifySslCertificates
                                    ? QSslSocket::VerifyPeer
                                    : QSslSocket::VerifyNone);
    request.setSslConfiguration(sslConfig);
    return true;
}

// 函数说明：实现 CloudSync::attachSslErrorHandler 的核心逻辑，供当前模块调用。
void CloudSync::attachSslErrorHandler(QNetworkReply *reply, const ServiceConfig &config, const QString &operation)
{
    if (!reply) {
        return;
    }

    connect(reply, &QNetworkReply::sslErrors, this,
            [this, reply, config, operation](const QList<QSslError> &errors) {
                if (errors.isEmpty()) {
                    return;
                }

                if (!config.verifySslCertificates) {
                    reply->ignoreSslErrors();
                    return;
                }

                const QString pinned = normalizeFingerprint(config.pinnedCertificateSha256);
                if (!pinned.isEmpty()) {
                    const QSslCertificate cert = reply->sslConfiguration().peerCertificate();
                    const QString actual = normalizeFingerprint(
                        QString::fromLatin1(cert.digest(QCryptographicHash::Sha256).toHex()));
                    if (!actual.isEmpty() && actual == pinned) {
                        reply->ignoreSslErrors();
                        return;
                    }
                    const QString msg = tr("%1 证书指纹不匹配（预期: %2，实际: %3）")
                                            .arg(operation, pinned, actual);
                    reply->setProperty("sslErrorMessage", msg);
                    m_lastError = msg;
                    return;
                }

                QStringList details;
                details.reserve(errors.size());
                for (const QSslError &error : errors) {
                    details.append(error.errorString());
                }
                const QString msg = tr("%1 SSL 证书验证失败: %2")
                                        .arg(operation, details.join(QStringLiteral("; ")));
                reply->setProperty("sslErrorMessage", msg);
                m_lastError = msg;
            });
}

// WebDAV 操作
bool CloudSync::webdavPropfind(const ServiceConfig &config, const QString &path, QVector<FileInfo> &files)
{
    QUrl url(config.serverUrl + path);
    QNetworkRequest request(url);
    if (!prepareWebDavRequest(config, request)) {
        return false;
    }
    request.setRawHeader("Depth", "1");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml");

    QString body = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                   "<propfind xmlns=\"DAV:\">\n"
                   "  <prop>\n"
                   "    <getlastmodified/>\n"
                   "    <getcontentlength/>\n"
                   "    <getetag/>\n"
                   "    <resourcetype/>\n"
                   "  </prop>\n"
                   "</propfind>";

    QNetworkReply *reply = m_networkManager->sendCustomRequest(request, "PROPFIND", body.toUtf8());
    attachSslErrorHandler(reply, config, tr("PROPFIND 请求"));

    // 同步等待
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        m_lastError = replyFailureMessage(reply);
        reply->deleteLater();
        return false;
    }

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (!isSuccessfulHttpStatus(statusCode)) {
        m_lastError = tr("PROPFIND 失败，HTTP %1").arg(statusCode);
        reply->deleteLater();
        return false;
    }

    QByteArray response = reply->readAll();
    reply->deleteLater();

    files = parseWebDavResponse(response, path);
    return true;
}

// 函数说明：实现 CloudSync::webdavGet 的核心逻辑，供当前模块调用。
bool CloudSync::webdavGet(const ServiceConfig &config, const QString &remotePath, const QString &localPath)
{
    QUrl url(config.serverUrl + remotePath);
    QNetworkRequest request(url);
    if (!prepareWebDavRequest(config, request)) {
        return false;
    }

    const QFileInfo localInfo(localPath);
    const qint64 existingBytes = (config.resumeTransfer && localInfo.exists()) ? localInfo.size() : 0;
    if (existingBytes > 0) {
        request.setRawHeader("Range",
                             QByteArray("bytes=") + QByteArray::number(existingBytes) + "-");
    }

    QNetworkReply *reply = m_networkManager->get(request);
    attachSslErrorHandler(reply, config, tr("GET 下载请求"));

    connect(reply, &QNetworkReply::downloadProgress,
            this, &CloudSync::onDownloadProgress);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 416 && existingBytes > 0) {
            const qint64 remoteTotal = parseUnsatisfiedRangeTotal(reply->rawHeader("Content-Range"));
            reply->deleteLater();
            if (remoteTotal > 0 && remoteTotal == existingBytes) {
                return true;
            }

            // Range 不可满足时，降级为全量下载，避免把不一致的本地分片误判为成功。
            ServiceConfig fallbackConfig = config;
            fallbackConfig.resumeTransfer = false;
            return webdavGet(fallbackConfig, remotePath, localPath);
        }
        m_lastError = replyFailureMessage(reply);
        reply->deleteLater();
        return false;
    }

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (!isSuccessfulHttpStatus(statusCode)) {
        m_lastError = tr("GET 下载失败，HTTP %1").arg(statusCode);
        reply->deleteLater();
        return false;
    }

    const QByteArray body = reply->readAll();
    reply->deleteLater();

    QFile file(localPath);
    QIODevice::OpenMode mode = QIODevice::WriteOnly;
    if (statusCode == 206 && existingBytes > 0) {
        mode |= QIODevice::Append;
    } else {
        mode |= QIODevice::Truncate;
    }

    if (!file.open(mode)) {
        m_lastError = tr("无法写入文件: %1").arg(localPath);
        return false;
    }
    file.write(body);
    file.close();
    return true;
}

// 函数说明：实现 CloudSync::webdavPut 的核心逻辑，供当前模块调用。
bool CloudSync::webdavPut(const ServiceConfig &config, const QString &localPath, const QString &remotePath)
{
    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = tr("无法读取文件: %1").arg(localPath);
        return false;
    }

    QUrl url(config.serverUrl + remotePath);
    const qint64 totalSize = file.size();

    qint64 resumeOffset = 0;
    if (config.resumeTransfer) {
        const qint64 remoteSize = webdavRemoteFileSize(config, remotePath);
        if (remoteSize == totalSize && totalSize > 0) {
            file.close();
            return true;
        }
        if (remoteSize > 0 && remoteSize < totalSize) {
            resumeOffset = remoteSize;
        }
    }

    auto performPut = [&](qint64 offset, bool useContentRange) -> bool {
        if (offset > 0 && !file.seek(offset)) {
            m_lastError = tr("无法定位上传偏移: %1").arg(offset);
            return false;
        }
        if (offset == 0) {
            file.seek(0);
        }

        QNetworkRequest request(url);
        if (!prepareWebDavRequest(config, request)) {
            return false;
        }
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
        request.setHeader(QNetworkRequest::ContentLengthHeader, totalSize - offset);
        if (useContentRange) {
            request.setRawHeader(
                "Content-Range",
                QByteArray("bytes ") + QByteArray::number(offset) + "-" +
                    QByteArray::number(totalSize - 1) + "/" + QByteArray::number(totalSize));
        }

        QNetworkReply *reply = m_networkManager->put(request, &file);
        attachSslErrorHandler(reply, config, tr("PUT 上传请求"));
        connect(reply, &QNetworkReply::uploadProgress,
                this, &CloudSync::onUploadProgress);

        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool success = reply->error() == QNetworkReply::NoError && isSuccessfulHttpStatus(statusCode);
        if (!success) {
            m_lastError = replyFailureMessage(reply);
            if (m_lastError.isEmpty()) {
                m_lastError = tr("PUT 上传失败，HTTP %1").arg(statusCode);
            }
        }
        reply->deleteLater();
        return success;
    };

    bool success = false;
    if (resumeOffset > 0) {
        success = performPut(resumeOffset, true);
        if (!success) {
            qWarning() << "WebDAV 断点续传失败，降级为全量上传:" << m_lastError;
            success = performPut(0, false);
        }
    } else {
        success = performPut(0, false);
    }

    file.close();
    return success;
}

// 函数说明：实现 CloudSync::webdavDelete 的核心逻辑，供当前模块调用。
bool CloudSync::webdavDelete(const ServiceConfig &config, const QString &remotePath)
{
    QUrl url(config.serverUrl + remotePath);
    QNetworkRequest request(url);
    if (!prepareWebDavRequest(config, request)) {
        return false;
    }

    QNetworkReply *reply = m_networkManager->deleteResource(request);
    attachSslErrorHandler(reply, config, tr("DELETE 请求"));

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    bool success = (reply->error() == QNetworkReply::NoError) && isSuccessfulHttpStatus(statusCode);
    if (!success) {
        m_lastError = replyFailureMessage(reply);
    }

    reply->deleteLater();
    return success;
}

// 函数说明：实现 CloudSync::webdavMkcol 的核心逻辑，供当前模块调用。
bool CloudSync::webdavMkcol(const ServiceConfig &config, const QString &path)
{
    QUrl url(config.serverUrl + path);
    QNetworkRequest request(url);
    if (!prepareWebDavRequest(config, request)) {
        return false;
    }

    QNetworkReply *reply = m_networkManager->sendCustomRequest(request, "MKCOL");
    attachSslErrorHandler(reply, config, tr("MKCOL 请求"));

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    bool success = (reply->error() == QNetworkReply::NoError) && isSuccessfulHttpStatus(statusCode);
    if (!success) {
        m_lastError = replyFailureMessage(reply);
    }

    reply->deleteLater();
    return success;
}

// 函数说明：实现 CloudSync::webdavRemoteFileSize 的核心逻辑，供当前模块调用。
qint64 CloudSync::webdavRemoteFileSize(const ServiceConfig &config, const QString &remotePath)
{
    QUrl url(config.serverUrl + remotePath);
    QNetworkRequest request(url);
    if (!prepareWebDavRequest(config, request)) {
        return -1;
    }

    QNetworkReply *reply = m_networkManager->head(request);
    attachSslErrorHandler(reply, config, tr("HEAD 请求"));

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode == 404) {
        reply->deleteLater();
        return 0;
    }

    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return -1;
    }

    const QVariant length = reply->header(QNetworkRequest::ContentLengthHeader);
    reply->deleteLater();
    if (!length.isValid()) {
        return -1;
    }
    return length.toLongLong();
}

// 函数说明：实现 CloudSync::icloudContainerPath 的核心逻辑，供当前模块调用。
QString CloudSync::icloudContainerPath()
{
#ifdef Q_OS_MAC
    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    path = path + "/Mobile Documents/com~apple~CloudDocs";
    return path;
#else
    return QString();
#endif
}

// 函数说明：读取 CloudSync 当前保存的状态或计算结果。
QVector<CloudSync::ConflictInfo> CloudSync::getConflicts() const
{
    return m_conflicts;
}

// 函数说明：设置 CloudSync 的运行参数，并触发必要的界面或数据刷新。
void CloudSync::setDefaultConflictPolicy(ConflictPolicy policy)
{
    m_defaultConflictPolicy = policy;
}

// 函数说明：读取 CloudSync 当前保存的状态或计算结果。
QVector<CloudSync::SyncRecord> CloudSync::getSyncHistory(int maxCount) const
{
    if (maxCount >= m_syncHistory.size()) {
        return m_syncHistory;
    }
    return m_syncHistory.mid(m_syncHistory.size() - maxCount);
}

// 函数说明：清空 CloudSync 保存的临时状态或缓存数据。
void CloudSync::clearSyncHistory()
{
    m_syncHistory.clear();
}

// 函数说明：向 CloudSync 管理的数据集合中添加一项内容。
void CloudSync::addExcludePattern(const QString &serviceName, const QString &pattern)
{
    if (m_services.contains(serviceName)) {
        m_services[serviceName].excludePatterns.append(pattern);
    }
}

// 函数说明：从 CloudSync 管理的数据集合中移除指定内容。
void CloudSync::removeExcludePattern(const QString &serviceName, const QString &pattern)
{
    if (m_services.contains(serviceName)) {
        m_services[serviceName].excludePatterns.removeAll(pattern);
    }
}

// 函数说明：判断 CloudSync 当前是否满足指定状态。
bool CloudSync::isExcluded(const QString &serviceName, const QString &path) const
{
    if (!m_services.contains(serviceName)) {
        return false;
    }

    const ServiceConfig &config = m_services[serviceName];
    for (const QString &pattern : config.excludePatterns) {
        QRegularExpression regex(QRegularExpression::wildcardToRegularExpression(pattern));
        if (regex.match(path).hasMatch()) {
            return true;
        }
    }

    // 默认排除模式
    static QStringList defaultExcludes = {
        ".*",           // 隐藏文件
        "*.tmp",
        "*.bak",
        "~*",
        "Thumbs.db",
        ".DS_Store",
        "*.swp"
    };

    for (const QString &pattern : defaultExcludes) {
        QRegularExpression regex(QRegularExpression::wildcardToRegularExpression(pattern));
        if (regex.match(QFileInfo(path).fileName()).hasMatch()) {
            return true;
        }
    }

    return false;
}

// 函数说明：保存 CloudSync 当前状态，保证用户修改可以持久化。
bool CloudSync::saveConfig(const QString &filePath)
{
    QJsonArray servicesArray;

    for (const ServiceConfig &config : m_services) {
        QJsonObject obj;
        obj["name"] = config.name;
        obj["type"] = static_cast<int>(config.type);
        obj["serverUrl"] = config.serverUrl;
        obj["username"] = config.username;
        obj["password"] = encryptSecret(config.password);
        obj["remotePath"] = config.remotePath;
        obj["localPath"] = config.localPath;
        obj["autoSync"] = config.autoSync;
        obj["syncInterval"] = config.syncInterval;
        obj["conflictPolicy"] = static_cast<int>(config.conflictPolicy);
        obj["verifySslCertificates"] = config.verifySslCertificates;
        obj["resumeTransfer"] = config.resumeTransfer;
        obj["pinnedCertificateSha256"] = config.pinnedCertificateSha256;
        obj["excludePatterns"] = QJsonArray::fromStringList(config.excludePatterns);
        servicesArray.append(obj);
    }

    QJsonObject root;
    root["services"] = servicesArray;
    root["defaultConflictPolicy"] = static_cast<int>(m_defaultConflictPolicy);

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    const QByteArray payload = QJsonDocument(root).toJson();
    if (file.write(payload) != payload.size()) {
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        return false;
    }

#if defined(Q_OS_UNIX)
    QFile::setPermissions(filePath, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
#endif

    return true;
}

// 函数说明：加载 CloudSync 需要的数据、配置或外部资源。
bool CloudSync::loadConfig(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull()) {
        return false;
    }

    QJsonObject root = doc.object();
    m_defaultConflictPolicy = static_cast<ConflictPolicy>(root["defaultConflictPolicy"].toInt());
    m_services.clear();
    m_fileStateCache.clear();
    m_conflicts.clear();

    QJsonArray servicesArray = root["services"].toArray();
    bool allLoaded = true;
    for (const QJsonValue &val : servicesArray) {
        QJsonObject obj = val.toObject();

        ServiceConfig config;
        config.name = obj["name"].toString();
        config.type = static_cast<ServiceType>(obj["type"].toInt());
        config.serverUrl = obj["serverUrl"].toString();
        config.username = obj["username"].toString();
        config.password = decryptSecret(obj["password"].toString());
        config.remotePath = obj["remotePath"].toString();
        config.localPath = obj["localPath"].toString();
        config.autoSync = obj["autoSync"].toBool();
        config.syncInterval = obj["syncInterval"].toInt();
        config.conflictPolicy = static_cast<ConflictPolicy>(obj["conflictPolicy"].toInt());
        config.verifySslCertificates = obj.contains("verifySslCertificates")
                                           ? obj["verifySslCertificates"].toBool()
                                           : true;
        config.resumeTransfer = obj.contains("resumeTransfer")
                                    ? obj["resumeTransfer"].toBool()
                                    : true;
        config.pinnedCertificateSha256 = obj["pinnedCertificateSha256"].toString();

        QJsonArray patterns = obj["excludePatterns"].toArray();
        for (const QJsonValue &p : patterns) {
            config.excludePatterns.append(p.toString());
        }

        if (!addService(config)) {
            allLoaded = false;
        }
    }

    return allLoaded;
}

// 函数说明：响应 CloudSync 收到的信号或异步回调，并更新界面状态。
void CloudSync::onAutoSyncTimer()
{
    if (m_state == SyncState::Idle) {
        startSync();
    }
}

// 函数说明：响应 CloudSync 收到的信号或异步回调，并更新界面状态。
void CloudSync::onNetworkReply()
{
    // 处理网络响应
}

// 函数说明：响应 CloudSync 收到的信号或异步回调，并更新界面状态。
void CloudSync::onUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    if (!m_currentTask.localPath.isEmpty()) {
        emit transferProgress(m_currentTask.localPath, bytesSent, bytesTotal);
    }
}

// 函数说明：响应 CloudSync 收到的信号或异步回调，并更新界面状态。
void CloudSync::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (!m_currentTask.remotePath.isEmpty()) {
        emit transferProgress(m_currentTask.remotePath, bytesReceived, bytesTotal);
    }
}

// 函数说明：实现 CloudSync::calculateMD5 的核心逻辑，供当前模块调用。
QString CloudSync::calculateMD5(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }

    QCryptographicHash hash(QCryptographicHash::Md5);
    if (hash.addData(&file)) {
        return QString::fromLatin1(hash.result().toHex());
    }

    return QString();
}

// 函数说明：创建 CloudSync 需要的对象、记录或输出内容。
QByteArray CloudSync::createBasicAuthHeader(const QString &username, const QString &password)
{
    QString credentials = username + ":" + password;
    return "Basic " + credentials.toUtf8().toBase64();
}

// 函数说明：解析输入内容，转换为 CloudSync 后续处理使用的数据结构。
QVector<CloudSync::FileInfo> CloudSync::parseWebDavResponse(const QByteArray &xml, const QString &basePath)
{
    QVector<FileInfo> files;
    QXmlStreamReader reader(xml);

    FileInfo currentFile;
    QString currentElement;

    while (!reader.atEnd()) {
        reader.readNext();

        if (reader.isStartElement()) {
            currentElement = reader.name().toString();

            if (currentElement == "response") {
                currentFile = FileInfo();
            }
        } else if (reader.isEndElement()) {
            if (reader.name() == "response" && !currentFile.path.isEmpty()) {
                // 过滤掉基础路径本身
                if (currentFile.path != basePath && currentFile.path != basePath + "/") {
                    currentFile.path = currentFile.path.mid(basePath.length());
                    if (currentFile.path.startsWith('/')) {
                        currentFile.path = currentFile.path.mid(1);
                    }
                    if (!currentFile.path.isEmpty()) {
                        files.append(currentFile);
                    }
                }
            }
        } else if (reader.isCharacters() && !reader.isWhitespace()) {
            QString text = reader.text().toString();

            if (currentElement == "href") {
                currentFile.path = QUrl::fromPercentEncoding(text.toUtf8());
            } else if (currentElement == "getlastmodified") {
                currentFile.modifiedTime = QDateTime::fromString(text, Qt::RFC2822Date);
            } else if (currentElement == "getcontentlength") {
                currentFile.size = text.toLongLong();
            } else if (currentElement == "getetag") {
                currentFile.etag = text;
            } else if (currentElement == "collection") {
                currentFile.isDirectory = true;
            }
        }
    }

    return files;
}

// 函数说明：向 CloudSync 管理的数据集合中添加一项内容。
void CloudSync::addSyncRecord(const QString &path, const QString &action, bool success, const QString &message)
{
    SyncRecord record;
    record.path = path;
    record.action = action;
    record.time = QDateTime::currentDateTime();
    record.success = success;
    record.message = message;

    m_syncHistory.append(record);

    // 限制历史记录大小
    while (m_syncHistory.size() > m_maxHistorySize) {
        m_syncHistory.removeFirst();
    }
}

// 函数说明：读取 CloudSync 当前保存的状态或计算结果。
QString CloudSync::getRelativePath(const QString &fullPath, const QString &basePath)
{
    if (fullPath.startsWith(basePath)) {
        QString rel = fullPath.mid(basePath.length());
        if (rel.startsWith('/')) rel = rel.mid(1);
        return rel;
    }
    return fullPath;
}

