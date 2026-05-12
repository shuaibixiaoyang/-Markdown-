// 文件说明：app-static\datavisualization\databaseconnector.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef DATABASECONNECTOR_H
#define DATABASECONNECTOR_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>
#include <QMap>
#include <QSqlDatabase>
#include <QSqlError>
#include <QTimer>

/**
 * @brief 数据库连接器 - 管理数据库连接并执行查询
 * 
 * 支持多种数据库驱动:
 * - SQLite
 * - MySQL / MariaDB
 * - PostgreSQL
 * - ODBC
 */
class DatabaseConnector : public QObject
{
    Q_OBJECT

public:
    static const int NoTimeoutMs = 0;
    static const int DefaultTimeoutMs = 10000;

    // 数据库类型
    enum class DatabaseType {
        SQLite,
        MySQL,
        PostgreSQL,
        ODBC,
        Unknown
    };
    Q_ENUM(DatabaseType)

    // 连接配置
    struct ConnectionConfig {
        QString name;           // 连接名称
        DatabaseType type;      // 数据库类型
        QString host;           // 主机地址
        int port;               // 端口号
        QString database;       // 数据库名/文件路径
        QString username;       // 用户名
        QString password;       // 密码
        QMap<QString, QString> options;  // 额外选项
    };

    // 查询结果
    struct QueryResult {
        bool success;
        QString errorMessage;
        QStringList columns;                    // 列名
        QVector<QVector<QVariant>> rows;        // 数据行
        int affectedRows;
        qint64 executionTimeMs;
    };

    // 列元信息
    struct ColumnInfo {
        QString name;
        QString typeName;
        int type;
        bool isNumeric;
    };

    explicit DatabaseConnector(QObject *parent = nullptr);
    ~DatabaseConnector();

    // 单例访问
    static DatabaseConnector& instance();

    // 连接管理
    bool addConnection(const ConnectionConfig &config);
    bool removeConnection(const QString &name);
    bool hasConnection(const QString &name) const;
    QStringList connectionNames() const;
    ConnectionConfig getConnectionConfig(const QString &name) const;

    // 连接状态
    bool openConnection(const QString &name);
    bool closeConnection(const QString &name);
    bool isConnectionOpen(const QString &name) const;
    QString lastError(const QString &name) const;

    // 查询执行
    QueryResult executeQuery(const QString &connectionName, const QString &sql);
    QueryResult executeQuery(const QString &connectionName, const QString &sql,
                             int timeoutMs);

    // 参数化查询（防止 SQL 注入）
    QueryResult executeQuery(const QString &connectionName, const QString &sql,
                             const QVariantList &params);
    QueryResult executeQuery(const QString &connectionName, const QString &sql,
                             const QVariantList &params, int timeoutMs);

    // 异步查询
    void executeQueryAsync(const QString &connectionName, const QString &sql,
                           const QString &blockId);
    void executeQueryAsync(const QString &connectionName, const QString &sql,
                           const QString &blockId, int timeoutMs);

    // 异步参数化查询（防止 SQL 注入）
    void executeQueryAsync(const QString &connectionName, const QString &sql,
                           const QVariantList &params, const QString &blockId);
    void executeQueryAsync(const QString &connectionName, const QString &sql,
                           const QVariantList &params, const QString &blockId,
                           int timeoutMs);

    // 查询超时配置
    int defaultQueryTimeoutMs() const { return m_defaultQueryTimeoutMs; }
    void setDefaultQueryTimeoutMs(int timeoutMs);

    // 工具方法
    static QString databaseTypeToDriver(DatabaseType type);
    static DatabaseType driverToDatabaseType(const QString &driver);
    static QVector<ColumnInfo> getColumnInfo(const QueryResult &result);

    // 保存/加载连接配置
    bool saveConnections(const QString &filePath);
    bool loadConnections(const QString &filePath);

signals:
    // 异步查询完成
    void queryCompleted(const QString &blockId, const QueryResult &result);
    
    // 连接状态变化
    void connectionOpened(const QString &name);
    void connectionClosed(const QString &name);
    void connectionError(const QString &name, const QString &error);

private:
    QMap<QString, ConnectionConfig> m_configs;
    QMap<QString, QSqlDatabase> m_connections;
    int m_defaultQueryTimeoutMs;
    
    QString generateConnectionId(const QString &baseName) const;
};

#endif // DATABASECONNECTOR_H

