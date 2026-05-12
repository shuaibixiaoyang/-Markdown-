// 文件说明：test\integration\datavisualizationpanelintegrationtest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
/*
 * Integration tests for DataVisualizationPanel async flow and config compatibility.
 */
#include "datavisualizationpanelintegrationtest.h"

#include <QtTest>

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>

#include <datalocation.h>
#include <datavisualization/datavisualizationpanel.h>
#include <datavisualization/databaseconnector.h>

namespace {

void clearConnectorState()
{
    DatabaseConnector &connector = DatabaseConnector::instance();
    const QStringList names = connector.connectionNames();
    for (const QString &name : names) {
        connector.removeConnection(name);
    }
    connector.setDefaultQueryTimeoutMs(DatabaseConnector::DefaultTimeoutMs);
}

class FileRestoreGuard
{
public:
    explicit FileRestoreGuard(const QString &path)
        : m_path(path)
    {
        QFile file(path);
        if (file.exists() && file.open(QIODevice::ReadOnly)) {
            m_hasOriginal = true;
            m_originalData = file.readAll();
        }
    }

    ~FileRestoreGuard()
    {
        if (m_hasOriginal) {
            QFile file(m_path);
            if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                file.write(m_originalData);
            }
        } else {
            QFile::remove(m_path);
        }
    }

private:
    QString m_path;
    bool m_hasOriginal = false;
    QByteArray m_originalData;
};

QString writeLegacyConnectionArrayJson(const QString &path)
{
    QJsonObject legacyConn;
    legacyConn[QStringLiteral("name")] = QStringLiteral("legacy_conn");
    legacyConn[QStringLiteral("type")] = static_cast<int>(DatabaseConnector::DatabaseType::SQLite);
    legacyConn[QStringLiteral("host")] = QString();
    legacyConn[QStringLiteral("port")] = 0;
    legacyConn[QStringLiteral("database")] = QStringLiteral("legacy.db");
    legacyConn[QStringLiteral("username")] = QStringLiteral("legacy_user");
    legacyConn[QStringLiteral("password")] = QStringLiteral("legacy_pass");
    legacyConn[QStringLiteral("options")] = QJsonObject();

    QJsonArray legacyArray;
    legacyArray.append(legacyConn);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return file.errorString();
    }
    file.write(QJsonDocument(legacyArray).toJson(QJsonDocument::Compact));
    return QString();
}

QString createSqliteFixture(const QString &dbPath)
{
    const QString setupConnName = QStringLiteral("datavis_panel_integration_setup_conn");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), setupConnName);
        db.setDatabaseName(dbPath);
        if (!db.open()) {
            const QString error = db.lastError().text();
            db = QSqlDatabase();
            QSqlDatabase::removeDatabase(setupConnName);
            return error;
        }

        QSqlQuery query(db);
        if (!query.exec(QStringLiteral("CREATE TABLE users(id INTEGER PRIMARY KEY, name TEXT)")) ||
            !query.exec(QStringLiteral("INSERT INTO users(id, name) VALUES (1, 'Alice')")) ||
            !query.exec(QStringLiteral("INSERT INTO users(id, name) VALUES (2, 'Bob')"))) {
            const QString error = query.lastError().text();
            db.close();
            db = QSqlDatabase();
            QSqlDatabase::removeDatabase(setupConnName);
            return error;
        }

        db.close();
    }

    QSqlDatabase::removeDatabase(setupConnName);
    return QString();
}

bool waitForSpyCount(QSignalSpy &spy, int expectedCount, int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();

    while (spy.count() < expectedCount) {
        const qint64 elapsed = timer.elapsed();
        if (elapsed >= timeoutMs) {
            return false;
        }

        const int remaining = timeoutMs - static_cast<int>(elapsed);
        if (!spy.wait(qMin(remaining, 200))) {
            continue;
        }
    }

    return true;
}

} // namespace

// 函数说明：实现 DataVisualizationPanelIntegrationTest::init 的核心逻辑，供当前模块调用。
void DataVisualizationPanelIntegrationTest::init()
{
    clearConnectorState();
}

// 函数说明：实现 DataVisualizationPanelIntegrationTest::cleanup 的核心逻辑，供当前模块调用。
void DataVisualizationPanelIntegrationTest::cleanup()
{
    clearConnectorState();
}

// 函数说明：实现 DataVisualizationPanelIntegrationTest::apiErrorAndTimeoutAlwaysReleasePending 的核心逻辑，供当前模块调用。
void DataVisualizationPanelIntegrationTest::apiErrorAndTimeoutAlwaysReleasePending()
{
    QTcpServer hangingServer;
    bool canListen = hangingServer.listen(QHostAddress::LocalHost, 0);
    if (!canListen) {
        canListen = hangingServer.listen(QHostAddress::AnyIPv4, 0);
    }
    if (!canListen) {
        QSKIP("Local TCP listener is unavailable in this environment.");
    }

    DataVisualizationPanel panel;

    QList<QTcpSocket *> hangingSockets;
    connect(&hangingServer, &QTcpServer::newConnection, this, [&]() {
        while (hangingServer.hasPendingConnections()) {
            QTcpSocket *socket = hangingServer.nextPendingConnection();
            hangingSockets.append(socket);
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    });

    QTcpServer probeServer;
    bool canProbe = probeServer.listen(QHostAddress::LocalHost, 0);
    if (!canProbe) {
        canProbe = probeServer.listen(QHostAddress::AnyIPv4, 0);
    }
    if (!canProbe) {
        QSKIP("Cannot allocate a probe TCP port for refused-connection scenario.");
    }
    const quint16 closedPort = probeServer.serverPort();
    probeServer.close();

    const QString markdown = QString(
        "```json {render: \"table\", source: \"api://http://127.0.0.1:%1/refused\", timeout: 150}\n"
        "{}\n"
        "```\n\n"
        "```json {render: \"table\", source: \"api://http://127.0.0.1:%2/hang\", timeout: 150}\n"
        "{}\n"
        "```")
        .arg(closedPort)
        .arg(hangingServer.serverPort());

    QSignalSpy refreshedSpy(&panel, &DataVisualizationPanel::dataRefreshed);
    panel.setMarkdownContent(markdown);
    panel.setMarkdownContent(markdown);

    QVERIFY2(waitForSpyCount(refreshedSpy, 1, 8000), "API refresh did not finish in time");

    // 如果 pending 没有归零，refreshAll 会因为 m_isRefreshing 为 true 而提前返回。
    panel.refreshAll();
    QVERIFY2(waitForSpyCount(refreshedSpy, 2, 8000), "Second API refresh did not finish in time");

    for (QTcpSocket *socket : hangingSockets) {
        if (socket) {
            socket->disconnectFromHost();
            socket->deleteLater();
        }
    }
}

// 函数说明：实现 DataVisualizationPanelIntegrationTest::sqlCacheHitAndVariableChangeInvalidatesCache 的核心逻辑，供当前模块调用。
void DataVisualizationPanelIntegrationTest::sqlCacheHitAndVariableChangeInvalidatesCache()
{
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
        QSKIP("QSQLITE driver is not available");
    }

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString dbPath = dir.filePath(QStringLiteral("datavis-cache.db"));
    const QString setupError = createSqliteFixture(dbPath);
    QVERIFY2(setupError.isEmpty(), qPrintable(setupError));

    DataVisualizationPanel panel;
    DatabaseConnector::ConnectionConfig config;
    config.name = QStringLiteral("datavis_cache_conn");
    config.type = DatabaseConnector::DatabaseType::SQLite;
    config.database = dbPath;
    config.port = 0;
    panel.addDatabaseConnection(config);

    panel.variableManager()->setVariable(QStringLiteral("uid"), 1,
                                         VariableManager::VariableType::Number);

    const QString markdown = QStringLiteral(
        "```sql {render: \"table\", connection: \"datavis_cache_conn\"}\n"
        "SELECT name FROM users WHERE id = $uid\n"
        "```");

    QSignalSpy refreshedSpy(&panel, &DataVisualizationPanel::dataRefreshed);
    QSignalSpy querySpy(&DatabaseConnector::instance(), &DatabaseConnector::queryCompleted);

    panel.setMarkdownContent(markdown);
    panel.setMarkdownContent(markdown);
    QVERIFY2(waitForSpyCount(refreshedSpy, 1, 8000), "Initial SQL refresh did not finish in time");
    QVERIFY2(querySpy.count() >= 1, "Initial SQL query should hit connector");

    refreshedSpy.clear();
    querySpy.clear();

    panel.refreshAll();
    QVERIFY2(waitForSpyCount(refreshedSpy, 1, 5000), "Cached SQL refresh did not finish in time");
    QCOMPARE(querySpy.count(), 0);

    refreshedSpy.clear();
    querySpy.clear();
    panel.variableManager()->setVariable(QStringLiteral("uid"), 2,
                                         VariableManager::VariableType::Number);
    panel.refreshAll();
    QVERIFY2(waitForSpyCount(refreshedSpy, 1, 8000), "SQL refresh after variable change did not finish in time");
    QVERIFY2(querySpy.count() >= 1,
             "Changing bound variable should invalidate SQL cache and re-query DB");
}

// 函数说明：实现 DataVisualizationPanelIntegrationTest::upgradesLegacyConnectionConfigViaDialogLoadSave 的核心逻辑，供当前模块调用。
void DataVisualizationPanelIntegrationTest::upgradesLegacyConnectionConfigViaDialogLoadSave()
{
    const QString configPath = QDir(DataLocation::writableLocation())
        .filePath(QStringLiteral("datavis-connections.json"));
    const QString configDir = QFileInfo(configPath).absolutePath();
    if (configDir.isEmpty() || !QDir().mkpath(configDir)) {
        QSKIP("AppDataLocation is not writable in this environment.");
    }

    FileRestoreGuard restoreGuard(configPath);
    QFile::remove(configPath);

    const QString writeError = writeLegacyConnectionArrayJson(configPath);
    QVERIFY2(writeError.isEmpty(), qPrintable(writeError));

    DatabaseConnectionDialog dialog;
    dialog.loadConnections();

    DatabaseConnector &connector = DatabaseConnector::instance();
    QVERIFY(connector.hasConnection(QStringLiteral("legacy_conn")));

    dialog.saveConnections();

    QFile upgradedFile(configPath);
    QVERIFY(upgradedFile.open(QIODevice::ReadOnly));
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(upgradedFile.readAll(), &parseError);
    QVERIFY(parseError.error == QJsonParseError::NoError);
    QVERIFY(doc.isObject());

    const QJsonObject root = doc.object();
    QVERIFY(root.contains(QStringLiteral("defaultQueryTimeoutMs")));
    QVERIFY(root.value(QStringLiteral("connections")).isArray());

    const QJsonArray connections = root.value(QStringLiteral("connections")).toArray();
    QVERIFY(!connections.isEmpty());
    QCOMPARE(connections.first().toObject().value(QStringLiteral("name")).toString(),
             QStringLiteral("legacy_conn"));
}

