// 文件说明：test\integration\cloudsyncintegrationtest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
/*
 * Integration tests for CloudSync WebDAV security and resume transfer.
 */
#include "cloudsyncintegrationtest.h"

#include <QtTest>

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QMap>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <functional>

#include <sync/cloudsync.h>
#include <datalocation.h>

namespace {

QString cloudSyncConfigPathForApp()
{
    return QDir(DataLocation::writableLocation())
        .filePath(QStringLiteral("cloud-sync-config.json"));
}

class ScopedFileBackup
{
public:
    explicit ScopedFileBackup(const QString &path)
        : m_path(path)
    {
        QFile file(path);
        if (file.exists()) {
            m_hadOriginal = true;
            if (file.open(QIODevice::ReadOnly)) {
                m_originalContent = file.readAll();
            }
        }
    }

    ~ScopedFileBackup()
    {
        if (m_hadOriginal) {
            QDir().mkpath(QFileInfo(m_path).absolutePath());
            QFile file(m_path);
            if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                file.write(m_originalContent);
            }
            return;
        }
        QFile::remove(m_path);
    }

private:
    QString m_path;
    bool m_hadOriginal = false;
    QByteArray m_originalContent;
};

CloudSync::ServiceConfig makeWebDavService(const QString &name,
                                           const QString &serverUrl,
                                           const QString &localPath)
{
    CloudSync::ServiceConfig config;
    config.name = name;
    config.type = CloudSync::ServiceType::WebDAV;
    config.serverUrl = serverUrl;
    config.username = QStringLiteral("user");
    config.password = QStringLiteral("pass");
    config.remotePath = QStringLiteral("/notes");
    config.localPath = localPath;
    config.verifySslCertificates = true;
    config.resumeTransfer = true;
    return config;
}

CloudSync::ServiceConfig findServiceByName(const QVector<CloudSync::ServiceConfig> &services,
                                           const QString &name)
{
    for (const CloudSync::ServiceConfig &service : services) {
        if (service.name == name) {
            return service;
        }
    }
    return CloudSync::ServiceConfig();
}

bool renameServiceWithRollback(CloudSync &sync,
                               const QString &oldName,
                               const CloudSync::ServiceConfig &newConfig,
                               const std::function<void()> &beforeAddStep)
{
    const CloudSync::ServiceConfig oldConfig = sync.getService(oldName);
    if (oldConfig.name.isEmpty()) {
        return false;
    }

    if (!sync.removeService(oldName)) {
        return false;
    }

    if (beforeAddStep) {
        beforeAddStep();
    }

    if (sync.addService(newConfig)) {
        return true;
    }

    // 回滚到旧配置（与主窗口编辑逻辑保持一致）。
    sync.addService(oldConfig);
    return false;
}

bool listenLocal(QTcpServer &server)
{
    if (server.listen(QHostAddress::LocalHost, 0)) {
        return true;
    }
    return server.listen(QHostAddress::AnyIPv4, 0);
}

QMap<QByteArray, QByteArray> parseHeaders(const QByteArray &requestHeader)
{
    QMap<QByteArray, QByteArray> headers;
    const QList<QByteArray> lines = requestHeader.split('\n');
    for (int i = 1; i < lines.size(); ++i) {
        const QByteArray line = lines.at(i).trimmed();
        if (line.isEmpty()) {
            break;
        }
        const int colon = line.indexOf(':');
        if (colon <= 0) {
            continue;
        }
        const QByteArray key = line.left(colon).trimmed().toLower();
        const QByteArray value = line.mid(colon + 1).trimmed();
        headers.insert(key, value);
    }
    return headers;
}

QByteArray readHttpRequest(QTcpSocket *socket, int timeoutMs = 5000)
{
    QByteArray data;
    QElapsedTimer timer;
    timer.start();

    while (!data.contains("\r\n\r\n")) {
        if (socket->bytesAvailable() == 0 && !socket->waitForReadyRead(50)) {
            if (timer.elapsed() > timeoutMs) {
                return QByteArray();
            }
            continue;
        }
        data += socket->readAll();
        if (timer.elapsed() > timeoutMs) {
            return QByteArray();
        }
    }

    const int headerEnd = data.indexOf("\r\n\r\n");
    const QByteArray headerPart = data.left(headerEnd + 4);
    const QMap<QByteArray, QByteArray> headers = parseHeaders(headerPart);
    const qint64 contentLength = headers.value("content-length").toLongLong();
    const qint64 targetSize = headerEnd + 4 + qMax<qint64>(0, contentLength);

    while (data.size() < targetSize) {
        if (!socket->waitForReadyRead(50)) {
            if (timer.elapsed() > timeoutMs) {
                break;
            }
            continue;
        }
        data += socket->readAll();
    }

    return data;
}

} // namespace

// 函数说明：保存 CloudSyncIntegrationTest 当前状态，保证用户修改可以持久化。
void CloudSyncIntegrationTest::saveConfigEncryptsWebDavCredentials()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    CloudSync sync;
    CloudSync::ServiceConfig config;
    config.name = QStringLiteral("secure_webdav");
    config.type = CloudSync::ServiceType::WebDAV;
    config.serverUrl = QStringLiteral("https://dav.example.com");
    config.username = QStringLiteral("alice");
    config.password = QStringLiteral("SecretPass!123");
    config.remotePath = QStringLiteral("/notes");
    config.localPath = dir.filePath(QStringLiteral("local"));
    config.verifySslCertificates = true;
    config.resumeTransfer = true;
    QVERIFY(sync.addService(config));

    const QString cfgPath = dir.filePath(QStringLiteral("cloudsync.json"));
    QVERIFY(sync.saveConfig(cfgPath));

    QFile cfgFile(cfgPath);
    QVERIFY(cfgFile.open(QIODevice::ReadOnly));
    const QByteArray raw = cfgFile.readAll();
    cfgFile.close();

    QVERIFY(!raw.contains(config.password.toUtf8()));
    QVERIFY(raw.contains("enc:v1:"));

    CloudSync loaded;
    QVERIFY(loaded.loadConfig(cfgPath));
    const QVector<CloudSync::ServiceConfig> services = loaded.getAllServices();
    QCOMPARE(services.size(), 1);
    QCOMPARE(services.first().password, config.password);
    QCOMPARE(services.first().verifySslCertificates, true);
    QCOMPARE(services.first().resumeTransfer, true);
}

// 函数说明：启动 CloudSyncIntegrationTest 的异步任务、会话或后台流程。
void CloudSyncIntegrationTest::startupConfigPathRestoresServiceList()
{
    const QString configPath = cloudSyncConfigPathForApp();
    ScopedFileBackup backup(configPath);

    const QDir dataDir(DataLocation::writableLocation());
    if ((!dataDir.exists() && !QDir().mkpath(dataDir.absolutePath())) || !dataDir.exists()) {
        QSKIP("AppDataLocation is not writable in this environment.");
    }

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString localRoot = dir.filePath(QStringLiteral("local"));
    QVERIFY(QDir().mkpath(localRoot));

    CloudSync writer;
    CloudSync::ServiceConfig s1 = makeWebDavService(
        QStringLiteral("startup-a"), QStringLiteral("https://dav.example.com"), localRoot + "/a");
    CloudSync::ServiceConfig s2 = makeWebDavService(
        QStringLiteral("startup-b"), QStringLiteral("https://dav2.example.com"), localRoot + "/b");
    s2.verifySslCertificates = false;
    s2.resumeTransfer = false;
    s2.pinnedCertificateSha256 = QStringLiteral("AA:BB:CC");

    QVERIFY(writer.addService(s1));
    QVERIFY(writer.addService(s2));
    QVERIFY(writer.saveConfig(configPath));

    CloudSync startup;
    QVERIFY(startup.loadConfig(configPath));
    const QVector<CloudSync::ServiceConfig> services = startup.getAllServices();
    QCOMPARE(services.size(), 2);
    QVERIFY(!findServiceByName(services, QStringLiteral("startup-a")).name.isEmpty());
    QVERIFY(!findServiceByName(services, QStringLiteral("startup-b")).name.isEmpty());
}

// 函数说明：处理编辑命令，将用户操作转换为 Markdown 文本变更。
void CloudSyncIntegrationTest::editedServicePersistsSslAndResumeAcrossRestart()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString configPath = dir.filePath(QStringLiteral("cloud-sync.json"));
    const QString localRoot = dir.filePath(QStringLiteral("local"));
    QVERIFY(QDir().mkpath(localRoot));

    CloudSync sync;
    CloudSync::ServiceConfig original = makeWebDavService(
        QStringLiteral("edit-me"), QStringLiteral("https://dav.example.com"), localRoot + "/service");
    original.pinnedCertificateSha256.clear();
    QVERIFY(sync.addService(original));

    CloudSync::ServiceConfig edited = original;
    edited.serverUrl = QStringLiteral("https://dav-edited.example.com");
    edited.verifySslCertificates = false;
    edited.resumeTransfer = false;
    edited.pinnedCertificateSha256 = QStringLiteral("11:22:33:44");
    QVERIFY(sync.updateService(edited));
    QVERIFY(sync.saveConfig(configPath));

    CloudSync reloaded;
    QVERIFY(reloaded.loadConfig(configPath));
    const QVector<CloudSync::ServiceConfig> services = reloaded.getAllServices();
    QCOMPARE(services.size(), 1);

    const CloudSync::ServiceConfig persisted = findServiceByName(services, QStringLiteral("edit-me"));
    QCOMPARE(persisted.serverUrl, edited.serverUrl);
    QCOMPARE(persisted.verifySslCertificates, edited.verifySslCertificates);
    QCOMPARE(persisted.resumeTransfer, edited.resumeTransfer);
    QCOMPARE(persisted.pinnedCertificateSha256, edited.pinnedCertificateSha256);
}

// 函数说明：实现 CloudSyncIntegrationTest::renameRollbackKeepsOriginalWhenTargetNameConflicts 的核心逻辑，供当前模块调用。
void CloudSyncIntegrationTest::renameRollbackKeepsOriginalWhenTargetNameConflicts()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString localRoot = dir.filePath(QStringLiteral("local"));
    QVERIFY(QDir().mkpath(localRoot));

    CloudSync sync;
    CloudSync::ServiceConfig alpha = makeWebDavService(
        QStringLiteral("alpha"), QStringLiteral("https://dav-alpha.example.com"), localRoot + "/alpha");
    alpha.verifySslCertificates = false;
    alpha.resumeTransfer = false;
    alpha.pinnedCertificateSha256 = QStringLiteral("AA:AA");

    QVERIFY(sync.addService(alpha));

    CloudSync::ServiceConfig target = alpha;
    target.name = QStringLiteral("beta");
    target.serverUrl = QStringLiteral("https://dav-beta.example.com");

    const bool renamed = renameServiceWithRollback(
        sync, alpha.name, target, [&]() {
            CloudSync::ServiceConfig conflict = makeWebDavService(
                QStringLiteral("beta"),
                QStringLiteral("https://conflict.example.com"),
                localRoot + "/beta");
            QVERIFY(sync.addService(conflict));
        });
    QVERIFY(!renamed);

    const QVector<CloudSync::ServiceConfig> services = sync.getAllServices();
    QCOMPARE(services.size(), 2);

    const CloudSync::ServiceConfig alphaAfter = findServiceByName(services, QStringLiteral("alpha"));
    QVERIFY(!alphaAfter.name.isEmpty());
    QCOMPARE(alphaAfter.serverUrl, alpha.serverUrl);
    QCOMPARE(alphaAfter.verifySslCertificates, alpha.verifySslCertificates);
    QCOMPARE(alphaAfter.resumeTransfer, alpha.resumeTransfer);
    QCOMPARE(alphaAfter.pinnedCertificateSha256, alpha.pinnedCertificateSha256);

    const CloudSync::ServiceConfig betaAfter = findServiceByName(services, QStringLiteral("beta"));
    QVERIFY(!betaAfter.name.isEmpty());
    QCOMPARE(betaAfter.serverUrl, QStringLiteral("https://conflict.example.com"));
}

// 函数说明：实现 CloudSyncIntegrationTest::webdavRejectsHttpWhenSslVerificationEnabled 的核心逻辑，供当前模块调用。
void CloudSyncIntegrationTest::webdavRejectsHttpWhenSslVerificationEnabled()
{
    CloudSync sync;

    CloudSync::ServiceConfig config;
    config.name = QStringLiteral("ssl_required");
    config.type = CloudSync::ServiceType::WebDAV;
    config.serverUrl = QStringLiteral("http://127.0.0.1:12345");
    config.username = QStringLiteral("u");
    config.password = QStringLiteral("p");
    config.remotePath = QStringLiteral("/");
    config.localPath = QStringLiteral("/tmp");
    config.verifySslCertificates = true;

    QVERIFY(!sync.testConnection(config));
    QVERIFY(sync.lastError().contains(QStringLiteral("HTTPS")));
}

// 函数说明：实现 CloudSyncIntegrationTest::webdavDownloadSupportsRangeResume 的核心逻辑，供当前模块调用。
void CloudSyncIntegrationTest::webdavDownloadSupportsRangeResume()
{
    QTcpServer server;
    if (!listenLocal(server)) {
        QSKIP("Local TCP listener is unavailable");
    }

    const QByteArray payload =
        QByteArrayLiteral("abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    QByteArray observedRangeHeader;

    connect(&server, &QTcpServer::newConnection, this, [&]() {
        while (server.hasPendingConnections()) {
            QTcpSocket *socket = server.nextPendingConnection();
            const QByteArray request = readHttpRequest(socket);
            const int headerEnd = request.indexOf("\r\n\r\n");
            const QByteArray headerPart = request.left(headerEnd + 4);
            const QMap<QByteArray, QByteArray> headers = parseHeaders(headerPart);
            observedRangeHeader = headers.value("range");

            int start = 0;
            if (!observedRangeHeader.isEmpty()) {
                const QByteArray prefix = QByteArrayLiteral("bytes=");
                if (observedRangeHeader.startsWith(prefix)) {
                    start = observedRangeHeader.mid(prefix.size()).split('-').value(0).toInt();
                }
            }
            const QByteArray body = payload.mid(start);
            const bool partial = start > 0;

            QByteArray response = partial
                                      ? QByteArrayLiteral("HTTP/1.1 206 Partial Content\r\n")
                                      : QByteArrayLiteral("HTTP/1.1 200 OK\r\n");
            response += QByteArrayLiteral("Content-Length: ") + QByteArray::number(body.size()) +
                        QByteArrayLiteral("\r\n");
            if (partial) {
                response += QByteArrayLiteral("Content-Range: bytes ") + QByteArray::number(start) +
                            QByteArrayLiteral("-") + QByteArray::number(payload.size() - 1) +
                            QByteArrayLiteral("/") + QByteArray::number(payload.size()) +
                            QByteArrayLiteral("\r\n");
            }
            response += QByteArrayLiteral("Connection: close\r\n\r\n");
            response += body;

            socket->write(response);
            socket->waitForBytesWritten(3000);
            socket->disconnectFromHost();
            socket->deleteLater();
        }
    });

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString localRoot = dir.filePath(QStringLiteral("local"));
    QVERIFY(QDir().mkpath(localRoot));
    const QString localFile = localRoot + QStringLiteral("/file.txt");

    const int existingBytes = 20;
    QFile partial(localFile);
    QVERIFY(partial.open(QIODevice::WriteOnly | QIODevice::Truncate));
    partial.write(payload.left(existingBytes));
    partial.close();

    CloudSync sync;
    CloudSync::ServiceConfig config;
    config.name = QStringLiteral("download_resume");
    config.type = CloudSync::ServiceType::WebDAV;
    config.serverUrl = QStringLiteral("http://127.0.0.1:%1").arg(server.serverPort());
    config.username = QStringLiteral("user");
    config.password = QStringLiteral("pass");
    config.remotePath = QStringLiteral("/remote");
    config.localPath = localRoot;
    config.verifySslCertificates = false;
    config.resumeTransfer = true;
    QVERIFY(sync.addService(config));

    QVERIFY(sync.downloadFile(config.name, QStringLiteral("file.txt")));

    QFile result(localFile);
    QVERIFY(result.open(QIODevice::ReadOnly));
    QCOMPARE(result.readAll(), payload);
    QVERIFY(observedRangeHeader.contains(QByteArray::number(existingBytes)));
}

// 函数说明：实现 CloudSyncIntegrationTest::webdavDownloadFallsBackToFullAfterUnsatisfiedRange 的核心逻辑，供当前模块调用。
void CloudSyncIntegrationTest::webdavDownloadFallsBackToFullAfterUnsatisfiedRange()
{
    QTcpServer server;
    if (!listenLocal(server)) {
        QSKIP("Local TCP listener is unavailable");
    }

    const QByteArray remotePayload = QByteArrayLiteral("NEWER-REMOTE");
    const int staleLocalBytes = 20;
    QByteArray firstRangeHeader;
    QByteArray secondRangeHeader;
    int requestCount = 0;

    connect(&server, &QTcpServer::newConnection, this, [&]() {
        while (server.hasPendingConnections()) {
            QTcpSocket *socket = server.nextPendingConnection();
            const QByteArray request = readHttpRequest(socket);
            const int headerEnd = request.indexOf("\r\n\r\n");
            const QByteArray headerPart = request.left(headerEnd + 4);
            const QMap<QByteArray, QByteArray> headers = parseHeaders(headerPart);
            const QByteArray rangeHeader = headers.value("range");

            ++requestCount;
            QByteArray response;
            if (requestCount == 1) {
                firstRangeHeader = rangeHeader;
                response = QByteArrayLiteral("HTTP/1.1 416 Range Not Satisfiable\r\n");
                response += QByteArrayLiteral("Content-Range: bytes */") +
                            QByteArray::number(remotePayload.size()) + QByteArrayLiteral("\r\n");
                response += QByteArrayLiteral("Content-Length: 0\r\n");
                response += QByteArrayLiteral("Connection: close\r\n\r\n");
            } else {
                secondRangeHeader = rangeHeader;
                response = QByteArrayLiteral("HTTP/1.1 200 OK\r\n");
                response += QByteArrayLiteral("Content-Length: ") +
                            QByteArray::number(remotePayload.size()) + QByteArrayLiteral("\r\n");
                response += QByteArrayLiteral("Connection: close\r\n\r\n");
                response += remotePayload;
            }

            socket->write(response);
            socket->waitForBytesWritten(3000);
            socket->disconnectFromHost();
            socket->deleteLater();
        }
    });

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString localRoot = dir.filePath(QStringLiteral("local"));
    QVERIFY(QDir().mkpath(localRoot));
    const QString localFile = localRoot + QStringLiteral("/file.txt");

    QFile stale(localFile);
    QVERIFY(stale.open(QIODevice::WriteOnly | QIODevice::Truncate));
    stale.write(QByteArray(staleLocalBytes, 'X'));
    stale.close();

    CloudSync sync;
    CloudSync::ServiceConfig config;
    config.name = QStringLiteral("download_fallback");
    config.type = CloudSync::ServiceType::WebDAV;
    config.serverUrl = QStringLiteral("http://127.0.0.1:%1").arg(server.serverPort());
    config.username = QStringLiteral("user");
    config.password = QStringLiteral("pass");
    config.remotePath = QStringLiteral("/remote");
    config.localPath = localRoot;
    config.verifySslCertificates = false;
    config.resumeTransfer = true;
    QVERIFY(sync.addService(config));

    QVERIFY(sync.downloadFile(config.name, QStringLiteral("file.txt")));

    QCOMPARE(requestCount, 2);
    QVERIFY(firstRangeHeader.contains(QByteArray::number(staleLocalBytes)));
    QVERIFY(secondRangeHeader.isEmpty());

    QFile result(localFile);
    QVERIFY(result.open(QIODevice::ReadOnly));
    QCOMPARE(result.readAll(), remotePayload);
}

// 函数说明：实现 CloudSyncIntegrationTest::webdavUploadSupportsContentRangeResume 的核心逻辑，供当前模块调用。
void CloudSyncIntegrationTest::webdavUploadSupportsContentRangeResume()
{
    QTcpServer server;
    if (!listenLocal(server)) {
        QSKIP("Local TCP listener is unavailable");
    }

    const QByteArray payload = QByteArrayLiteral("UPLOAD-RESUME-CONTENT-ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    const int remoteExisting = 10;
    QByteArray observedContentRange;
    QByteArray observedUploadBody;
    int headCount = 0;
    int putCount = 0;

    connect(&server, &QTcpServer::newConnection, this, [&]() {
        while (server.hasPendingConnections()) {
            QTcpSocket *socket = server.nextPendingConnection();
            const QByteArray request = readHttpRequest(socket);
            const int headerEnd = request.indexOf("\r\n\r\n");
            const QByteArray headerPart = request.left(headerEnd + 4);
            const QByteArray body = request.mid(headerEnd + 4);
            const int eol = headerPart.indexOf('\n');
            const QByteArray requestLine = (eol >= 0 ? headerPart.left(eol) : headerPart).trimmed();
            const QByteArray method = requestLine.split(' ').value(0);
            const QMap<QByteArray, QByteArray> headers = parseHeaders(headerPart);

            QByteArray response;
            if (method == "HEAD") {
                ++headCount;
                response = QByteArrayLiteral("HTTP/1.1 200 OK\r\n");
                response += QByteArrayLiteral("Content-Length: ") +
                            QByteArray::number(remoteExisting) + QByteArrayLiteral("\r\n");
                response += QByteArrayLiteral("Connection: close\r\n\r\n");
            } else if (method == "PUT") {
                ++putCount;
                observedContentRange = headers.value("content-range");
                observedUploadBody = body;
                response = QByteArrayLiteral("HTTP/1.1 201 Created\r\n");
                response += QByteArrayLiteral("Content-Length: 0\r\n");
                response += QByteArrayLiteral("Connection: close\r\n\r\n");
            } else {
                response = QByteArrayLiteral("HTTP/1.1 405 Method Not Allowed\r\n");
                response += QByteArrayLiteral("Content-Length: 0\r\n");
                response += QByteArrayLiteral("Connection: close\r\n\r\n");
            }

            socket->write(response);
            socket->waitForBytesWritten(3000);
            socket->disconnectFromHost();
            socket->deleteLater();
        }
    });

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString localRoot = dir.filePath(QStringLiteral("local"));
    QVERIFY(QDir().mkpath(localRoot));
    const QString localFile = localRoot + QStringLiteral("/file.txt");

    QFile file(localFile);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(payload);
    file.close();

    CloudSync sync;
    CloudSync::ServiceConfig config;
    config.name = QStringLiteral("upload_resume");
    config.type = CloudSync::ServiceType::WebDAV;
    config.serverUrl = QStringLiteral("http://127.0.0.1:%1").arg(server.serverPort());
    config.username = QStringLiteral("user");
    config.password = QStringLiteral("pass");
    config.remotePath = QStringLiteral("/remote");
    config.localPath = localRoot;
    config.verifySslCertificates = false;
    config.resumeTransfer = true;
    QVERIFY(sync.addService(config));

    QVERIFY(sync.uploadFile(config.name, localFile));

    QCOMPARE(headCount, 1);
    QCOMPARE(putCount, 1);
    QVERIFY(observedContentRange.contains("bytes 10-"));
    QCOMPARE(observedUploadBody, payload.mid(remoteExisting));
}

