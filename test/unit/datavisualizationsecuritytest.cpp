// 文件说明：test\unit\datavisualizationsecuritytest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "datavisualizationsecuritytest.h"

#include <QtTest>
#include <QSqlDatabase>
#include <QTemporaryDir>

#include <datavisualization/databaseconnector.h>
#include <datavisualization/variablemanager.h>

namespace {

void clearAllConnections()
{
    DatabaseConnector &connector = DatabaseConnector::instance();
    const QStringList names = connector.connectionNames();
    for (const QString &name : names) {
        connector.removeConnection(name);
    }
}

} // namespace

// 函数说明：实现 DataVisualizationSecurityTest::init 的核心逻辑，供当前模块调用。
void DataVisualizationSecurityTest::init()
{
    clearAllConnections();
    DatabaseConnector::instance().setDefaultQueryTimeoutMs(DatabaseConnector::DefaultTimeoutMs);
}

// 函数说明：实现 DataVisualizationSecurityTest::cleanup 的核心逻辑，供当前模块调用。
void DataVisualizationSecurityTest::cleanup()
{
    clearAllConnections();
    DatabaseConnector::instance().setDefaultQueryTimeoutMs(DatabaseConnector::DefaultTimeoutMs);
}

// 函数说明：实现 DataVisualizationSecurityTest::sqlSubstitutionUsesBindParams 的核心逻辑，供当前模块调用。
void DataVisualizationSecurityTest::sqlSubstitutionUsesBindParams()
{
    VariableManager manager;
    manager.setVariable(QStringLiteral("name"), QStringLiteral("Alice"),
                        VariableManager::VariableType::String);
    manager.setVariable(QStringLiteral("min_age"), 18, VariableManager::VariableType::Number);

    QVariantList params;
    const QString sql = manager.substituteForSQL(
        QStringLiteral("SELECT * FROM users WHERE name='$name' AND age >= $min_age"),
        &params);

    QCOMPARE(sql, QStringLiteral("SELECT * FROM users WHERE name=? AND age >= ?"));
    QCOMPARE(params.size(), 2);
    QCOMPARE(params.at(0).toString(), QStringLiteral("Alice"));
    QCOMPARE(params.at(1).toInt(), 18);
}

// 函数说明：实现 DataVisualizationSecurityTest::listVariableExpansionUsesBindParams 的核心逻辑，供当前模块调用。
void DataVisualizationSecurityTest::listVariableExpansionUsesBindParams()
{
    VariableManager manager;
    QVariantList ids;
    ids << 1 << 2 << 3;
    manager.setVariable(QStringLiteral("ids"), ids, VariableManager::VariableType::List);

    QVariantList params;
    const QString sql = manager.substituteForSQL(
        QStringLiteral("SELECT * FROM orders WHERE id IN ($ids)"),
        &params);

    QCOMPARE(sql, QStringLiteral("SELECT * FROM orders WHERE id IN (?,?,?)"));
    QCOMPARE(params.size(), 3);
    QCOMPARE(params.at(0).toInt(), 1);
    QCOMPARE(params.at(1).toInt(), 2);
    QCOMPARE(params.at(2).toInt(), 3);
}

// 函数说明：实现 DataVisualizationSecurityTest::databaseConnectorRejectsMultiStatementSql 的核心逻辑，供当前模块调用。
void DataVisualizationSecurityTest::databaseConnectorRejectsMultiStatementSql()
{
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
        QSKIP("QSQLITE driver is not available");
    }

    DatabaseConnector &connector = DatabaseConnector::instance();

    DatabaseConnector::ConnectionConfig config;
    config.name = QStringLiteral("unit_sqlite");
    config.type = DatabaseConnector::DatabaseType::SQLite;
    config.database = QStringLiteral(":memory:");
    config.port = 0;
    QVERIFY(connector.addConnection(config));

    const DatabaseConnector::QueryResult okResult = connector.executeQuery(
        config.name, QStringLiteral("SELECT 1"));
    QVERIFY2(okResult.success, qPrintable(okResult.errorMessage));

    const DatabaseConnector::QueryResult invalidResult = connector.executeQuery(
        config.name, QStringLiteral("SELECT 1; SELECT 2"));
    QVERIFY(!invalidResult.success);
    QVERIFY(invalidResult.errorMessage.contains(QStringLiteral("单条语句")));
}

// 函数说明：实现 DataVisualizationSecurityTest::databaseConnectorPersistsTimeoutAndConnections 的核心逻辑，供当前模块调用。
void DataVisualizationSecurityTest::databaseConnectorPersistsTimeoutAndConnections()
{
    DatabaseConnector &connector = DatabaseConnector::instance();
    connector.setDefaultQueryTimeoutMs(4321);

    DatabaseConnector::ConnectionConfig config;
    config.name = QStringLiteral("persisted_conn");
    config.type = DatabaseConnector::DatabaseType::SQLite;
    config.database = QStringLiteral("example.db");
    config.port = 0;
    QVERIFY(connector.addConnection(config));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString filePath = dir.filePath(QStringLiteral("datavis-connections.json"));

    QVERIFY(connector.saveConnections(filePath));

    connector.setDefaultQueryTimeoutMs(123);
    clearAllConnections();

    QVERIFY(connector.loadConnections(filePath));
    QCOMPARE(connector.defaultQueryTimeoutMs(), 4321);
    QVERIFY(connector.hasConnection(config.name));

    const DatabaseConnector::ConnectionConfig loaded = connector.getConnectionConfig(config.name);
    QCOMPARE(loaded.type, config.type);
    QCOMPARE(loaded.database, config.database);
}

