// 文件说明：app-static\datavisualization\databaseconnector.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "databaseconnector.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlField>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QSharedPointer>
#include <QRegularExpression>
#include <QThread>
#include <QTimer>

namespace {

bool isSingleStatementSql(const QString &sql)
{
    QString normalized = sql.trimmed();
    if (normalized.isEmpty()) {
        return false;
    }

    // 允许末尾单个分号，但拒绝多语句执行（最基础的注入防护）。
    if (normalized.endsWith(QLatin1Char(';'))) {
        normalized.chop(1);
        normalized = normalized.trimmed();
    }
    return !normalized.contains(QLatin1Char(';'));
}

void applyStatementTimeout(QSqlDatabase &db,
                           DatabaseConnector::DatabaseType type,
                           int timeoutMs)
{
    if (timeoutMs <= 0 || !db.isOpen()) {
        return;
    }

    QSqlQuery timeoutQuery(db);
    switch (type) {
        case DatabaseConnector::DatabaseType::SQLite:
            timeoutQuery.exec(QStringLiteral("PRAGMA busy_timeout=%1").arg(timeoutMs));
            break;
        case DatabaseConnector::DatabaseType::PostgreSQL:
            timeoutQuery.exec(QStringLiteral("SET statement_timeout = %1").arg(timeoutMs));
            break;
        case DatabaseConnector::DatabaseType::MySQL:
            timeoutQuery.exec(QStringLiteral("SET SESSION MAX_EXECUTION_TIME=%1").arg(timeoutMs));
            break;
        default:
            break;
    }
}

DatabaseConnector::QueryResult runPreparedQuery(QSqlDatabase &db,
                                                const QString &sql,
                                                const QVariantList &params,
                                                int timeoutMs)
{
    DatabaseConnector::QueryResult result;
    result.success = false;
    result.affectedRows = 0;
    result.executionTimeMs = 0;

    if (!isSingleStatementSql(sql)) {
        result.errorMessage = QObject::tr("SQL 必须是单条语句，禁止多语句执行");
        return result;
    }

    QElapsedTimer timer;
    timer.start();

    QSqlQuery query(db);
    query.setForwardOnly(true);

    if (!query.prepare(sql)) {
        result.errorMessage = query.lastError().text();
        result.executionTimeMs = timer.elapsed();
        return result;
    }

    for (int i = 0; i < params.size(); ++i) {
        query.bindValue(i, params.at(i));
    }

    if (!query.exec()) {
        result.errorMessage = query.lastError().text();
        result.executionTimeMs = timer.elapsed();
        return result;
    }

    QSqlRecord record = query.record();
    for (int i = 0; i < record.count(); ++i) {
        result.columns.append(record.fieldName(i));
    }

    while (query.next()) {
        QVector<QVariant> row;
        row.reserve(record.count());
        for (int i = 0; i < record.count(); ++i) {
            row.append(query.value(i));
        }
        result.rows.append(row);
    }

    result.success = true;
    result.affectedRows = query.numRowsAffected();
    result.executionTimeMs = timer.elapsed();
    if (timeoutMs > 0 && result.executionTimeMs > timeoutMs) {
        result.success = false;
        result.rows.clear();
        result.errorMessage = QObject::tr("查询超时（>%1 ms）").arg(timeoutMs);
    }

    return result;
}

} // namespace

// 函数说明：构造 DatabaseConnector 对象，初始化本模块需要的状态、界面和资源。
DatabaseConnector::DatabaseConnector(QObject *parent)
    : QObject(parent)
    , m_defaultQueryTimeoutMs(DefaultTimeoutMs)
{
}

// 函数说明：销毁 DatabaseConnector 对象，释放本模块持有的资源。
DatabaseConnector::~DatabaseConnector()
{
    // 关闭所有连接
    for (const QString &name : m_connections.keys()) {
        closeConnection(name);
    }
}

// 函数说明：实现 DatabaseConnector::instance 的核心逻辑，供当前模块调用。
DatabaseConnector& DatabaseConnector::instance()
{
    static DatabaseConnector instance;
    return instance;
}

// 函数说明：向 DatabaseConnector 管理的数据集合中添加一项内容。
bool DatabaseConnector::addConnection(const ConnectionConfig &config)
{
    if (config.name.isEmpty()) {
        return false;
    }
    
    // 如果已存在，先移除
    if (m_configs.contains(config.name)) {
        removeConnection(config.name);
    }
    
    m_configs[config.name] = config;
    return true;
}

// 函数说明：从 DatabaseConnector 管理的数据集合中移除指定内容。
bool DatabaseConnector::removeConnection(const QString &name)
{
    if (!m_configs.contains(name)) {
        return false;
    }
    
    closeConnection(name);
    m_configs.remove(name);
    return true;
}

// 函数说明：检查 DatabaseConnector 是否具备对应的数据或能力。
bool DatabaseConnector::hasConnection(const QString &name) const
{
    return m_configs.contains(name);
}

// 函数说明：实现 DatabaseConnector::connectionNames 的核心逻辑，供当前模块调用。
QStringList DatabaseConnector::connectionNames() const
{
    return m_configs.keys();
}

// 函数说明：读取 DatabaseConnector 当前保存的状态或计算结果。
DatabaseConnector::ConnectionConfig DatabaseConnector::getConnectionConfig(const QString &name) const
{
    return m_configs.value(name);
}

// 函数说明：打开 DatabaseConnector 对应的文件、资源或功能入口。
bool DatabaseConnector::openConnection(const QString &name)
{
    if (!m_configs.contains(name)) {
        emit connectionError(name, tr("Connection '%1' not found").arg(name));
        return false;
    }
    
    const ConnectionConfig &config = m_configs[name];
    QString connectionId = generateConnectionId(name);
    
    // 如果已打开，直接返回
    if (m_connections.contains(name) && m_connections[name].isOpen()) {
        return true;
    }
    
    // 创建数据库连接
    QString driver = databaseTypeToDriver(config.type);
    QSqlDatabase db = QSqlDatabase::addDatabase(driver, connectionId);
    
    switch (config.type) {
        case DatabaseType::SQLite:
            db.setDatabaseName(config.database);
            break;
            
        case DatabaseType::MySQL:
        case DatabaseType::PostgreSQL:
            db.setHostName(config.host);
            db.setPort(config.port > 0 ? config.port : 
                      (config.type == DatabaseType::MySQL ? 3306 : 5432));
            db.setDatabaseName(config.database);
            db.setUserName(config.username);
            db.setPassword(config.password);
            break;
            
        case DatabaseType::ODBC:
            db.setDatabaseName(config.database); // DSN
            db.setUserName(config.username);
            db.setPassword(config.password);
            break;
            
        default:
            emit connectionError(name, tr("Unknown database type"));
            return false;
    }
    
    // 应用额外选项
    for (auto it = config.options.begin(); it != config.options.end(); ++it) {
        db.setConnectOptions(it.key() + "=" + it.value());
    }
    
    // 尝试打开连接
    if (!db.open()) {
        QString error = db.lastError().text();
        emit connectionError(name, error);
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectionId);
        return false;
    }
    
    m_connections[name] = db;
    emit connectionOpened(name);
    return true;
}

// 函数说明：关闭 DatabaseConnector 相关窗口或资源，并处理必要的保存确认。
bool DatabaseConnector::closeConnection(const QString &name)
{
    if (!m_connections.contains(name)) {
        return false;
    }
    
    QSqlDatabase db = m_connections[name];
    QString connectionId = db.connectionName();
    
    if (db.isOpen()) {
        db.close();
    }
    
    m_connections.remove(name);
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionId);
    
    emit connectionClosed(name);
    return true;
}

// 函数说明：判断 DatabaseConnector 当前是否满足指定状态。
bool DatabaseConnector::isConnectionOpen(const QString &name) const
{
    if (!m_connections.contains(name)) {
        return false;
    }
    return m_connections[name].isOpen();
}

// 函数说明：实现 DatabaseConnector::lastError 的核心逻辑，供当前模块调用。
QString DatabaseConnector::lastError(const QString &name) const
{
    if (!m_connections.contains(name)) {
        return tr("Connection not found");
    }
    return m_connections[name].lastError().text();
}

// 函数说明：实现 DatabaseConnector::executeQuery 的核心逻辑，供当前模块调用。
DatabaseConnector::QueryResult DatabaseConnector::executeQuery(const QString &connectionName,
                                                               const QString &sql)
{
    return executeQuery(connectionName, sql, QVariantList(), m_defaultQueryTimeoutMs);
}

// 函数说明：实现 DatabaseConnector::executeQuery 的核心逻辑，供当前模块调用。
DatabaseConnector::QueryResult DatabaseConnector::executeQuery(const QString &connectionName,
                                                               const QString &sql,
                                                               int timeoutMs)
{
    return executeQuery(connectionName, sql, QVariantList(), timeoutMs);
}

// 函数说明：实现 DatabaseConnector::executeQuery 的核心逻辑，供当前模块调用。
DatabaseConnector::QueryResult DatabaseConnector::executeQuery(const QString &connectionName,
                                                               const QString &sql,
                                                               const QVariantList &params)
{
    return executeQuery(connectionName, sql, params, m_defaultQueryTimeoutMs);
}

// 函数说明：实现 DatabaseConnector::executeQuery 的核心逻辑，供当前模块调用。
DatabaseConnector::QueryResult DatabaseConnector::executeQuery(const QString &connectionName,
                                                               const QString &sql,
                                                               const QVariantList &params,
                                                               int timeoutMs)
{
    QueryResult result;
    result.success = false;
    result.affectedRows = 0;
    result.executionTimeMs = 0;

    // 检查连接
    if (!isConnectionOpen(connectionName)) {
        if (!openConnection(connectionName)) {
            result.errorMessage = tr("Failed to open connection '%1'").arg(connectionName);
            return result;
        }
    }

    QSqlDatabase &db = m_connections[connectionName];
    applyStatementTimeout(db, m_configs.value(connectionName).type, timeoutMs);
    return runPreparedQuery(db, sql, params, timeoutMs);
}

// 函数说明：实现 DatabaseConnector::executeQueryAsync 的核心逻辑，供当前模块调用。
void DatabaseConnector::executeQueryAsync(const QString &connectionName,
                                          const QString &sql,
                                          const QString &blockId)
{
    executeQueryAsync(connectionName, sql, QVariantList(), blockId, m_defaultQueryTimeoutMs);
}

// 函数说明：实现 DatabaseConnector::executeQueryAsync 的核心逻辑，供当前模块调用。
void DatabaseConnector::executeQueryAsync(const QString &connectionName,
                                          const QString &sql,
                                          const QString &blockId,
                                          int timeoutMs)
{
    executeQueryAsync(connectionName, sql, QVariantList(), blockId, timeoutMs);
}

// 函数说明：实现 DatabaseConnector::executeQueryAsync 的核心逻辑，供当前模块调用。
void DatabaseConnector::executeQueryAsync(const QString &connectionName,
                                          const QString &sql,
                                          const QVariantList &params,
                                          const QString &blockId)
{
    executeQueryAsync(connectionName, sql, params, blockId, m_defaultQueryTimeoutMs);
}

// 函数说明：实现 DatabaseConnector::executeQueryAsync 的核心逻辑，供当前模块调用。
void DatabaseConnector::executeQueryAsync(const QString &connectionName,
                                          const QString &sql,
                                          const QVariantList &params,
                                          const QString &blockId,
                                          int timeoutMs)
{
    if (!m_configs.contains(connectionName)) {
        QueryResult result;
        result.success = false;
        result.errorMessage = tr("Connection '%1' not found").arg(connectionName);
        emit queryCompleted(blockId, result);
        return;
    }

    const ConnectionConfig config = m_configs.value(connectionName);

    // 使用 QtConcurrent 在后台线程执行
    QFuture<QueryResult> future = QtConcurrent::run([=]() {
        // 注意：需要在线程中创建新的数据库连接
        // 因为 QSqlDatabase 不能跨线程使用
        QString threadConnId = QString("%1_thread_%2")
            .arg(connectionName)
            .arg(reinterpret_cast<quintptr>(QThread::currentThread()));

        // 在线程中创建连接
        QSqlDatabase db = QSqlDatabase::addDatabase(
            databaseTypeToDriver(config.type), threadConnId);

        switch (config.type) {
            case DatabaseType::SQLite:
                db.setDatabaseName(config.database);
                break;
            case DatabaseType::MySQL:
            case DatabaseType::PostgreSQL:
                db.setHostName(config.host);
                db.setPort(config.port > 0 ? config.port :
                           (config.type == DatabaseType::MySQL ? 3306 : 5432));
                db.setDatabaseName(config.database);
                db.setUserName(config.username);
                db.setPassword(config.password);
                break;
            case DatabaseType::ODBC:
                db.setDatabaseName(config.database);
                db.setUserName(config.username);
                db.setPassword(config.password);
                break;
            default:
                break;
        }

        for (auto it = config.options.begin(); it != config.options.end(); ++it) {
            db.setConnectOptions(it.key() + "=" + it.value());
        }

        QueryResult result;
        result.success = false;

        if (!db.open()) {
            result.errorMessage = db.lastError().text();
            db = QSqlDatabase();
            QSqlDatabase::removeDatabase(threadConnId);
            return result;
        }

        applyStatementTimeout(db, config.type, timeoutMs);
        result = runPreparedQuery(db, sql, params, timeoutMs);

        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(threadConnId);

        return result;
    });

    // 监听完成
    QFutureWatcher<QueryResult> *watcher = new QFutureWatcher<QueryResult>(this);
    QTimer *timeoutTimer = new QTimer(this);
    timeoutTimer->setSingleShot(true);
    auto completed = QSharedPointer<bool>::create(false);

    if (timeoutMs > 0) {
        connect(timeoutTimer, &QTimer::timeout, this, [=]() {
            if (*completed) {
                return;
            }
            *completed = true;
            QueryResult timeoutResult;
            timeoutResult.success = false;
            timeoutResult.executionTimeMs = timeoutMs;
            timeoutResult.errorMessage = tr("查询超时（>%1 ms）").arg(timeoutMs);
            emit queryCompleted(blockId, timeoutResult);
        });
        timeoutTimer->start(timeoutMs);
    }

    connect(watcher, &QFutureWatcher<QueryResult>::finished, this, [=]() {
        if (*completed) {
            watcher->deleteLater();
            timeoutTimer->deleteLater();
            return;
        }
        *completed = true;
        timeoutTimer->stop();
        QueryResult result = watcher->result();
        emit queryCompleted(blockId, result);
        watcher->deleteLater();
        timeoutTimer->deleteLater();
    });

    watcher->setFuture(future);
}

// 函数说明：设置 DatabaseConnector 的运行参数，并触发必要的界面或数据刷新。
void DatabaseConnector::setDefaultQueryTimeoutMs(int timeoutMs)
{
    if (timeoutMs < NoTimeoutMs) {
        m_defaultQueryTimeoutMs = NoTimeoutMs;
        return;
    }
    m_defaultQueryTimeoutMs = timeoutMs;
}

// 函数说明：实现 DatabaseConnector::databaseTypeToDriver 的核心逻辑，供当前模块调用。
QString DatabaseConnector::databaseTypeToDriver(DatabaseType type)
{
    switch (type) {
        case DatabaseType::SQLite: return "QSQLITE";
        case DatabaseType::MySQL: return "QMYSQL";
        case DatabaseType::PostgreSQL: return "QPSQL";
        case DatabaseType::ODBC: return "QODBC";
        default: return QString();
    }
}

// 函数说明：实现 DatabaseConnector::driverToDatabaseType 的核心逻辑，供当前模块调用。
DatabaseConnector::DatabaseType DatabaseConnector::driverToDatabaseType(const QString &driver)
{
    if (driver == "QSQLITE") return DatabaseType::SQLite;
    if (driver == "QMYSQL") return DatabaseType::MySQL;
    if (driver == "QPSQL") return DatabaseType::PostgreSQL;
    if (driver == "QODBC") return DatabaseType::ODBC;
    return DatabaseType::Unknown;
}

// 函数说明：读取 DatabaseConnector 当前保存的状态或计算结果。
QVector<DatabaseConnector::ColumnInfo> DatabaseConnector::getColumnInfo(const QueryResult &result)
{
    QVector<ColumnInfo> info;
    
    // 从第一行数据推断类型
    for (int i = 0; i < result.columns.size(); ++i) {
        ColumnInfo col;
        col.name = result.columns[i];
        
        if (!result.rows.isEmpty() && i < result.rows[0].size()) {
            QVariant value = result.rows[0][i];
            col.type = value.type();
            col.typeName = value.typeName();
            col.isNumeric = (value.type() == QVariant::Int ||
                            value.type() == QVariant::Double ||
                            value.type() == QVariant::LongLong);
        } else {
            col.type = QVariant::String;
            col.typeName = "QString";
            col.isNumeric = false;
        }
        
        info.append(col);
    }
    
    return info;
}

// 函数说明：保存 DatabaseConnector 当前状态，保证用户修改可以持久化。
bool DatabaseConnector::saveConnections(const QString &filePath)
{
    if (filePath.isEmpty()) {
        return false;
    }

    QFileInfo fileInfo(filePath);
    QDir dir = fileInfo.dir();
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        return false;
    }

    QJsonArray connectionsArray;
    
    for (const ConnectionConfig &config : m_configs) {
        QJsonObject obj;
        obj["name"] = config.name;
        obj["type"] = static_cast<int>(config.type);
        obj["host"] = config.host;
        obj["port"] = config.port;
        obj["database"] = config.database;
        obj["username"] = config.username;
        // 注意：密码不应明文保存，实际应用中需要加密
        obj["password"] = config.password;
        
        QJsonObject options;
        for (auto it = config.options.begin(); it != config.options.end(); ++it) {
            options[it.key()] = it.value();
        }
        obj["options"] = options;
        
        connectionsArray.append(obj);
    }

    QJsonObject root;
    root["version"] = 1;
    root["defaultQueryTimeoutMs"] = m_defaultQueryTimeoutMs;
    root["connections"] = connectionsArray;

    QJsonDocument doc(root);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(doc.toJson());
    file.close();
    
    return true;
}

// 函数说明：加载 DatabaseConnector 需要的数据、配置或外部资源。
bool DatabaseConnector::loadConnections(const QString &filePath)
{
    if (filePath.isEmpty()) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        return false;
    }

    QJsonArray connectionsArray;
    if (doc.isObject()) {
        QJsonObject root = doc.object();
        if (root.contains("defaultQueryTimeoutMs")) {
            setDefaultQueryTimeoutMs(root["defaultQueryTimeoutMs"].toInt(m_defaultQueryTimeoutMs));
        }
        connectionsArray = root["connections"].toArray();
    } else if (doc.isArray()) {
        // 兼容旧格式：根节点直接是连接数组。
        connectionsArray = doc.array();
    } else {
        return false;
    }

    const QStringList names = m_configs.keys();
    for (const QString &name : names) {
        removeConnection(name);
    }
    
    for (const QJsonValue &value : connectionsArray) {
        if (!value.isObject()) {
            continue;
        }

        QJsonObject obj = value.toObject();
        
        ConnectionConfig config;
        config.name = obj["name"].toString();
        config.type = static_cast<DatabaseType>(obj["type"].toInt());
        config.host = obj["host"].toString();
        config.port = obj["port"].toInt();
        config.database = obj["database"].toString();
        config.username = obj["username"].toString();
        config.password = obj["password"].toString();
        
        QJsonObject options = obj["options"].toObject();
        for (auto it = options.begin(); it != options.end(); ++it) {
            config.options[it.key()] = it.value().toString();
        }

        if (!config.name.isEmpty()) {
            addConnection(config);
        }
    }
    
    return true;
}

// 函数说明：根据当前数据生成 DatabaseConnector 需要的输出结果。
QString DatabaseConnector::generateConnectionId(const QString &baseName) const
{
    return QString("cutemarked_%1_%2")
        .arg(baseName)
        .arg(reinterpret_cast<quintptr>(QCoreApplication::instance()));
}

