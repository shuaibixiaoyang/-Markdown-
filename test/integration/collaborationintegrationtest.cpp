// 文件说明：test\integration\collaborationintegrationtest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
/*
 * Integration tests for collaboration transport and recovery protocol.
 */
#include "collaborationintegrationtest.h"

#include <QElapsedTimer>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QSslError>
#include <QSslSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include "compat/websocketcompat.h"

#include "collaboration/collaborationserver.h"
#include "collaboration/crdtdocument.h"

namespace {

QString tryStartServer(Collaboration::CollaborationServer &server, int timeoutMs = 5000)
{
    Q_UNUSED(timeoutMs);
    QSignalSpy errorSpy(&server, &Collaboration::CollaborationServer::error);
    if (server.start(0)) {
        return QString();
    }

    QString reason = QStringLiteral("unknown error");
    if (!errorSpy.isEmpty()) {
        const QList<QVariant> args = errorSpy.takeLast();
        if (!args.isEmpty()) {
            reason = args.first().toString();
        }
    }

    return reason;
}

class SpyReader
{
public:
    explicit SpyReader(QSignalSpy *spy)
        : m_spy(spy)
        , m_index(0)
    {
    }

    bool waitForType(const QString &type, QJsonObject *message, int timeoutMs = 5000)
    {
        QElapsedTimer timer;
        timer.start();

        while (timer.elapsed() < timeoutMs) {
            while (m_index < m_spy->count()) {
                const QList<QVariant> args = m_spy->at(m_index++);
                if (args.isEmpty()) {
                    continue;
                }

                const QJsonDocument doc = QJsonDocument::fromJson(args.first().toString().toUtf8());
                if (!doc.isObject()) {
                    continue;
                }

                const QJsonObject obj = doc.object();
                if (obj.value(QStringLiteral("type")).toString() == type) {
                    if (message) {
                        *message = obj;
                    }
                    return true;
                }
            }
            QTest::qWait(10);
        }

        return false;
    }

private:
    QSignalSpy *m_spy;
    int m_index;
};

void sendJson(QWebSocket &socket, const QJsonObject &obj)
{
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    socket.sendTextMessage(QString::fromUtf8(payload));
}

bool connectSocket(QWebSocket &socket, const QUrl &url, int timeoutMs = 5000)
{
    QSignalSpy connectedSpy(&socket, &QWebSocket::connected);
    socket.open(url);
    return connectedSpy.wait(timeoutMs);
}

QJsonObject makeJoinMessage(const QString &sessionId,
                            const QString &userName,
                            const QString &resumeUserId = QString(),
                            qint64 lastOpSeq = -1)
{
    QJsonObject joinMsg;
    joinMsg[QStringLiteral("type")] = QStringLiteral("join");
    joinMsg[QStringLiteral("sessionId")] = sessionId;
    joinMsg[QStringLiteral("userName")] = userName;
    joinMsg[QStringLiteral("userEmail")] = QStringLiteral("%1@example.test").arg(userName.toLower());
    if (!resumeUserId.isEmpty()) {
        joinMsg[QStringLiteral("resumeUserId")] = resumeUserId;
    }
    joinMsg[QStringLiteral("lastOpSeq")] = lastOpSeq;
    return joinMsg;
}

void sendOperation(QWebSocket &socket, const Collaboration::Operation &operation)
{
    QJsonObject msg;
    msg[QStringLiteral("type")] = QStringLiteral("operation");
    msg[QStringLiteral("operation")] = operation.toJson();
    sendJson(socket, msg);
}

QString writePemFile(const QDir &dir, const QString &name, const QByteArray &content)
{
    const QString path = dir.filePath(name);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return QString();
    }
    file.write(content);
    file.close();
    return path;
}

} // namespace

// 函数说明：实现 CollaborationIntegrationTest::disconnectReconnectReceivesIncrementalDelta 的核心逻辑，供当前模块调用。
void CollaborationIntegrationTest::disconnectReconnectReceivesIncrementalDelta()
{
    Collaboration::CollaborationServer server;
    const QString startError = tryStartServer(server);
    if (!startError.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("Local WebSocket listener unavailable in this environment: %1")
                         .arg(startError)));
        return;
    }

    const QString sessionId = server.createSession(QStringLiteral("recovery-doc"),
                                                   QStringLiteral("Host"));
    const QUrl serverUrl(server.serverUrl());
    QVERIFY(serverUrl.isValid());

    QWebSocket aliceSocket;
    QSignalSpy aliceMessageSpy(&aliceSocket, &QWebSocket::textMessageReceived);
    QVERIFY2(connectSocket(aliceSocket, serverUrl), "Alice failed to connect");
    SpyReader aliceReader(&aliceMessageSpy);

    sendJson(aliceSocket, makeJoinMessage(sessionId, QStringLiteral("Alice")));
    QJsonObject aliceJoined;
    QVERIFY2(aliceReader.waitForType(QStringLiteral("joined"), &aliceJoined),
             "Alice did not receive joined response");
    const QString aliceUserId = aliceJoined.value(QStringLiteral("userId")).toString();
    QVERIFY(!aliceUserId.isEmpty());

    Collaboration::CrdtDocument aliceGenerator(QStringLiteral("alice-site"));
    sendOperation(aliceSocket, aliceGenerator.localInsert(aliceGenerator.length(), QLatin1Char('A')));

    QJsonObject aliceOpAck;
    QVERIFY2(aliceReader.waitForType(QStringLiteral("operation"), &aliceOpAck),
             "Alice did not receive operation ack");
    QCOMPARE(aliceOpAck.value(QStringLiteral("opSeq")).toInteger(), 1);

    QSignalSpy aliceDisconnectedSpy(&aliceSocket, &QWebSocket::disconnected);
    aliceSocket.close();
    QVERIFY2(aliceDisconnectedSpy.wait(5000), "Alice failed to disconnect");

    QWebSocket bobSocket;
    QSignalSpy bobMessageSpy(&bobSocket, &QWebSocket::textMessageReceived);
    QVERIFY2(connectSocket(bobSocket, serverUrl), "Bob failed to connect");
    SpyReader bobReader(&bobMessageSpy);

    sendJson(bobSocket, makeJoinMessage(sessionId, QStringLiteral("Bob")));
    QJsonObject bobJoined;
    QVERIFY2(bobReader.waitForType(QStringLiteral("joined"), &bobJoined),
             "Bob did not receive joined response");
    const QString bobUserId = bobJoined.value(QStringLiteral("userId")).toString();
    QVERIFY(!bobUserId.isEmpty());

    Collaboration::CrdtDocument bobGenerator(QStringLiteral("bob-site"));
    sendOperation(bobSocket, bobGenerator.localInsert(bobGenerator.length(), QLatin1Char('B')));

    QJsonObject bobOpAck;
    QVERIFY2(bobReader.waitForType(QStringLiteral("operation"), &bobOpAck),
             "Bob did not receive operation ack");
    QCOMPARE(bobOpAck.value(QStringLiteral("opSeq")).toInteger(), 2);

    QWebSocket resumedAliceSocket;
    QSignalSpy resumedAliceMessageSpy(&resumedAliceSocket, &QWebSocket::textMessageReceived);
    QVERIFY2(connectSocket(resumedAliceSocket, serverUrl), "Reconnected Alice failed to connect");
    SpyReader resumedAliceReader(&resumedAliceMessageSpy);

    sendJson(resumedAliceSocket,
             makeJoinMessage(sessionId, QStringLiteral("Alice"), aliceUserId, 1));

    QJsonObject resumedJoin;
    QVERIFY2(resumedAliceReader.waitForType(QStringLiteral("joined"), &resumedJoin),
             "Reconnected Alice did not receive joined response");

    QVERIFY(resumedJoin.value(QStringLiteral("resumed")).toBool());
    QVERIFY(!resumedJoin.value(QStringLiteral("fullState")).toBool());

    const QJsonArray deltaOps = resumedJoin.value(QStringLiteral("operations")).toArray();
    QCOMPARE(deltaOps.size(), 1);
    const QJsonObject delta = deltaOps.first().toObject();
    QCOMPARE(delta.value(QStringLiteral("opSeq")).toInteger(), 2);
    QCOMPARE(delta.value(QStringLiteral("userId")).toString(), bobUserId);
}

// 函数说明：处理云同步操作，把本地文档状态同步到配置的远端。
void CollaborationIntegrationTest::syncFallsBackToFullStateWhenOperationLogHasGap()
{
    Collaboration::CollaborationServer server;
    const QString startError = tryStartServer(server);
    if (!startError.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("Local WebSocket listener unavailable in this environment: %1")
                         .arg(startError)));
        return;
    }

    const QString sessionId = server.createSession(QStringLiteral("gap-doc"),
                                                   QStringLiteral("Host"));
    const QUrl serverUrl(server.serverUrl());

    QWebSocket socket;
    QSignalSpy messageSpy(&socket, &QWebSocket::textMessageReceived);
    QVERIFY2(connectSocket(socket, serverUrl), "Client failed to connect");
    SpyReader reader(&messageSpy);

    sendJson(socket, makeJoinMessage(sessionId, QStringLiteral("BulkWriter")));
    QJsonObject joined;
    QVERIFY2(reader.waitForType(QStringLiteral("joined"), &joined),
             "Client did not receive joined response");

    Collaboration::CrdtDocument generator(QStringLiteral("bulk-site"));
    const int totalOps = 2050; // MaxOperationLogEntries=2000, force gap.
    for (int i = 0; i < totalOps; ++i) {
        const QChar ch = QLatin1Char(static_cast<char>('a' + (i % 26)));
        sendOperation(socket, generator.localInsert(generator.length(), ch));
    }

    QVERIFY(server.document(sessionId) != nullptr);
    QTRY_COMPARE_WITH_TIMEOUT(server.document(sessionId)->length(), totalOps, 20000);

    QJsonObject syncMsg;
    syncMsg[QStringLiteral("type")] = QStringLiteral("sync");
    syncMsg[QStringLiteral("fromOpSeq")] = 1;
    sendJson(socket, syncMsg);

    QJsonObject syncResponse;
    QVERIFY2(reader.waitForType(QStringLiteral("sync_response"), &syncResponse, 10000),
             "Client did not receive sync_response");

    QVERIFY(syncResponse.value(QStringLiteral("fullState")).toBool());
    QVERIFY(syncResponse.contains(QStringLiteral("documentState")));
    QVERIFY(syncResponse.value(QStringLiteral("documentState")).toArray().size() >= totalOps);
    QVERIFY(syncResponse.value(QStringLiteral("serverOpSeq")).toInteger() >= totalOps);
}

// 函数说明：实现 CollaborationIntegrationTest::wssHandshakeAndJoinWorksWithTlsEnabled 的核心逻辑，供当前模块调用。
void CollaborationIntegrationTest::wssHandshakeAndJoinWorksWithTlsEnabled()
{
    if (!QSslSocket::supportsSsl()) {
        QSKIP("Qt SSL backend is unavailable on this runtime.");
    }

    static const QByteArray kTestCertPem =
        "-----BEGIN CERTIFICATE-----\n"
        "MIIDCTCCAfGgAwIBAgIUD56d6hk2/j4WMOOFL/aXlT9YGNUwDQYJKoZIhvcNAQEL\n"
        "BQAwFDESMBAGA1UEAwwJbG9jYWxob3N0MB4XDTI2MDMwNDA4NDEyMVoXDTI2MDMw\n"
        "NjA4NDEyMVowFDESMBAGA1UEAwwJbG9jYWxob3N0MIIBIjANBgkqhkiG9w0BAQEF\n"
        "AAOCAQ8AMIIBCgKCAQEA5O2kVLIVIVAm38xRzbkCHdpO0Bucu9vhOP1uxq9qsyBW\n"
        "BMbDBaN4vp8pbV/jp7wYr3nFMgPrsUvNjkgUQCN7z8iSQT2OmGY+G27z12uQOhql\n"
        "8lawS3NZ03WqXHL5AFO3tUxmAt6YNu1AuCimNh3QPXyCELcb0uuGBBSKZ7zUPC4P\n"
        "HT6Rf/mFX+ZdVve3Djgm8mzwhQ9pQvg7sj9ay6H3m2m55V8l6j49YJ63vzbgKwF2\n"
        "d7NsfjfMqC6d4sIJOUEcfKaZpmOq6oaVVFbl9aBpSngWXjdw/i/s1Hj/OVo/fo6J\n"
        "sYe4yzEb0R0UQERtMa70uG1XCPlaROUcHIGDvC0c+QIDAQABo1MwUTAdBgNVHQ4E\n"
        "FgQUDdOXofoUQoqdMOP7tm7GJQlbJ3UwHwYDVR0jBBgwFoAUDdOXofoUQoqdMOP7\n"
        "tm7GJQlbJ3UwDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAy1C6\n"
        "W7/anC5zN0QKb0r5zzDh2utIyPqYAzSEIdsr7EtH6nAwkvuP8tctAT0r6r7rZE25\n"
        "HEIku6G1fLG9XDeLBQvL8lh5eHn8v89K0HhHMY8Ss2/EC1Jx8MboaVH+AN9ugzjK\n"
        "HLzp39kKKgloJGsIlA3Ve2m3NJ5AQRrvdaWUn5f8wPIIa1yVWVlZaqJewmopcHTD\n"
        "g83KtOpzL12aQqj8HqrMIMDqdZIX2hqhzpMwHdv8wlQhZAvkWieCnCMEztpXIZXw\n"
        "wSL8/a3WZKxMFpsWKyWgNE1uKeBmwuDHw2dOv8rntgyql/zI+o2fey2zfu8+YoXT\n"
        "M4axNLgtpRXJ5Faw0w==\n"
        "-----END CERTIFICATE-----\n";

    static const QByteArray kTestKeyPem =
        "-----BEGIN PRIVATE KEY-----\n"
        "MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDk7aRUshUhUCbf\n"
        "zFHNuQId2k7QG5y72+E4/W7Gr2qzIFYExsMFo3i+nyltX+OnvBivecUyA+uxS82O\n"
        "SBRAI3vPyJJBPY6YZj4bbvPXa5A6GqXyVrBLc1nTdapccvkAU7e1TGYC3pg27UC4\n"
        "KKY2HdA9fIIQtxvS64YEFIpnvNQ8Lg8dPpF/+YVf5l1W97cOOCbybPCFD2lC+Duy\n"
        "P1rLofebabnlXyXqPj1gnre/NuArAXZ3s2x+N8yoLp3iwgk5QRx8ppmmY6rqhpVU\n"
        "VuX1oGlKeBZeN3D+L+zUeP85Wj9+jomxh7jLMRvRHRRARG0xrvS4bVcI+VpE5Rwc\n"
        "gYO8LRz5AgMBAAECggEAAxtsmw+ttL2LQYda5uFnc3NGlZGCX5Rc6rKZ4c14s8Sx\n"
        "FjCCbh2pYmDzpX3jL6Agb6VF9WdTOB1QxWkaCULSB+Bvh74npM7icengGmDl3LwG\n"
        "25KsouGGaxXSQZ333jbErAvE6K4Pv9MZiUZX9y0Rafw6IbLyiDrVH1IODNllwglR\n"
        "a80OdO3Bm0byIIGFbUj3bxFHvOoIDS6C51EA6xSuEKgOXfxik8LgHyaEL8rTp1dY\n"
        "autZEPC62QGRGLD5s3gvaUWMNeS1mleOtgU4PcAWAx3oR0L4fLyDLzVgeuKt4uqv\n"
        "SGJNowanENhOpfpXXuweHH30kNjdwHG1z87rx/579wKBgQD+c9bcJAX6i0h5plBu\n"
        "G0L1JIOYIoVfN/QiCCkJ1nLxISAlqoao1BAcTZdEx9R3C4q5GgftBFG0u2rX2OrH\n"
        "dbZF3hmmfsMxUddJYqfAZ+rz1seg23YW9beT06EyuTcuS+/S1WtQIFfZ+KJq66jr\n"
        "lsdaUbO5Z19pMdPDFFDxCye7awKBgQDmUhBJSzKPkdTjINqIdxbyxBCK1pcADXG6\n"
        "c5NMePgDW+Owtc75FlZ0h4yavPTq35JwKKNMvXJ1P60mz0CfmmtbvYBKqeFgHHO0\n"
        "01dKsEt42N/5/rnw7PwvEIKy9Fz7Jzs4o4h4+jLhk5ZK2W5daPlv6z0f6yeDC3Oz\n"
        "1AfSwHxmKwKBgEZ0CI/XoZgnrJ+SPz7daYK643ziQg+FTKGHpOVGbXj6dQ440yQ3\n"
        "42YSzcmLkvaLSZPK81rbEUx7gC/Xrdoga6GYx31kJ+OmB3gYSt5pZ5Kwa4HMgjwF\n"
        "ORlDRaTnx7GX8QVtdlMvQWPnBgGY0qK4kuYdLSguySo1U672FxtGvW+VAoGBANsL\n"
        "AOIz2gogr06zWCKg3/pR7UmdfE5YeujQMi5wfa70HT9aKVVLoT2CDH69ZlBaAHMa\n"
        "svOw2MIZpRtb0CH6QlAlkXVwyx8U8BXxSPuHXr+3wouHbl9rgwtfsG1xaVySmwfq\n"
        "v0gO64UNT8ovr36270M5fhB1HEbNBWTpKeeNmMGxAoGAMerDRPkD+Bv3qvr3uZHm\n"
        "kzFEuQ3ZGa+O0Z8alx4NLmGJYjd31mkO9DUS3p55PWmR4cbfAQkY5+2CTQ5PTxDQ\n"
        "qs905KF1SsmeJH/KlUuc4lj1gmOExTlGu95SMShAoQYT/mN6Ef/spBOaP9+eqa2u\n"
        "RwfXFYGgRO5C8uXArigj6Lo=\n"
        "-----END PRIVATE KEY-----\n";

    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Failed to create temporary directory for TLS material");

    const QDir tempPath(tempDir.path());
    const QString certPath = writePemFile(tempPath, QStringLiteral("cert.pem"), kTestCertPem);
    const QString keyPath = writePemFile(tempPath, QStringLiteral("key.pem"), kTestKeyPem);
    QVERIFY2(!certPath.isEmpty(), "Failed to write test certificate");
    QVERIFY2(!keyPath.isEmpty(), "Failed to write test private key");

    Collaboration::CollaborationServer server;
    QVERIFY(server.enableTls(certPath, keyPath));
    const QString startError = tryStartServer(server);
    if (!startError.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("Local WebSocket listener unavailable in this environment: %1")
                         .arg(startError)));
        return;
    }
    QVERIFY(server.serverUrl().startsWith(QStringLiteral("wss://")));

    const QString sessionId = server.createSession(QStringLiteral("secure-doc"),
                                                   QStringLiteral("Host"));

    QWebSocket socket;
    QObject::connect(&socket, &QWebSocket::sslErrors, &socket,
                     [&socket](const QList<QSslError> &errors) {
        socket.ignoreSslErrors(errors);
    });

    QSignalSpy messageSpy(&socket, &QWebSocket::textMessageReceived);
    QVERIFY2(connectSocket(socket, QUrl(server.serverUrl())), "WSS client failed to connect");
    SpyReader reader(&messageSpy);

    sendJson(socket, makeJoinMessage(sessionId, QStringLiteral("SecureUser")));

    QJsonObject joined;
    QVERIFY2(reader.waitForType(QStringLiteral("joined"), &joined),
             "WSS client did not receive joined response");
    QCOMPARE(joined.value(QStringLiteral("sessionId")).toString(), sessionId);
    QVERIFY(!joined.value(QStringLiteral("userId")).toString().isEmpty());
}

