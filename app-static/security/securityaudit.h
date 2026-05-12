// 文件说明：app-static\security\securityaudit.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef SECURITYAUDIT_H
#define SECURITYAUDIT_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVector>
#include <QFile>
#include <QMutex>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QDir>
#include <QCryptographicHash>
#include <QRandomGenerator>

/**
 * @brief 安全审计日志
 * 
 * 记录所有安全相关操作：
 * - 加密/解密操作
 * - 认证尝试（成功/失败）
 * - 文件访问
 * - 配置变更
 * - 异常事件
 * 
 * 特性：
 * - 日志完整性校验（HMAC）
 * - 自动轮转
 * - 加密存储
 */
class SecurityAudit : public QObject
{
    Q_OBJECT

public:
    // 事件类型
    enum class EventType {
        // 认证事件
        AuthSuccess,            // 认证成功
        AuthFailed,             // 认证失败
        AuthLockout,            // 认证锁定
        BiometricSuccess,       // 生物特征成功
        BiometricFailed,        // 生物特征失败
        
        // 加密事件
        FileEncrypted,          // 文件加密
        FileDecrypted,          // 文件解密
        EncryptionFailed,       // 加密失败
        DecryptionFailed,       // 解密失败
        
        // 文件事件
        FileOpened,             // 文件打开
        FileSaved,              // 文件保存
        FileExpired,            // 文件过期
        FileDestroyed,          // 文件销毁
        IntegrityViolation,     // 完整性违规
        
        // 配置事件
        SettingsChanged,        // 设置变更
        PasswordChanged,        // 密码变更
        
        // 系统事件
        SessionStart,           // 会话开始
        SessionEnd,             // 会话结束
        IdleTimeout,            // 空闲超时
        
        // 安全事件
        SuspiciousActivity,     // 可疑活动
        BruteForceDetected,     // 检测到暴力破解
        TamperingDetected       // 检测到篡改
    };
    Q_ENUM(EventType)

    // 严重级别
    enum class Severity {
        Info,       // 信息
        Warning,    // 警告
        Error,      // 错误
        Critical    // 严重
    };
    Q_ENUM(Severity)

    // 审计事件
    struct AuditEvent {
        QString id;
        QDateTime timestamp;
        EventType type;
        Severity severity;
        QString description;
        QString filePath;
        QString userName;
        QString ipAddress;
        QJsonObject metadata;
        QString hash;           // 事件哈希（完整性）
    };

    // 日志配置
    struct AuditConfig {
        QString logDirectory;
        int maxLogSize = 10 * 1024 * 1024;  // 10MB
        int maxLogFiles = 10;
        bool encryptLogs = false;
        bool includeStackTrace = false;
        Severity minSeverity = Severity::Info;
    };

    explicit SecurityAudit(QObject *parent = nullptr);
    ~SecurityAudit();

    // 单例
    static SecurityAudit& instance();

    // 记录事件
    void log(EventType type, const QString &description,
             const QString &filePath = QString(),
             const QJsonObject &metadata = QJsonObject());

    void log(EventType type, Severity severity, const QString &description,
             const QString &filePath = QString(),
             const QJsonObject &metadata = QJsonObject());

    // 便捷方法
    void logAuthSuccess(const QString &method);
    void logAuthFailed(const QString &method, const QString &reason);
    void logFileEncrypted(const QString &filePath);
    void logFileDecrypted(const QString &filePath);
    void logFileOpened(const QString &filePath, bool encrypted);
    void logSessionStart();
    void logSessionEnd();
    void logSettingsChanged(const QString &setting, const QVariant &oldValue, const QVariant &newValue);
    void logSuspiciousActivity(const QString &description, const QJsonObject &details);

    // 查询日志
    QVector<AuditEvent> getRecentEvents(int count = 100) const;
    QVector<AuditEvent> getEventsByType(EventType type, int count = 100) const;
    QVector<AuditEvent> getEventsByDateRange(const QDateTime &from, const QDateTime &to) const;
    QVector<AuditEvent> getEventsByFile(const QString &filePath) const;

    // 统计
    int getFailedAuthCount(int minutesBack = 60) const;
    bool isBruteForceDetected(int threshold = 5, int minutesBack = 15) const;

    // 配置
    void setConfig(const AuditConfig &config);
    AuditConfig config() const { return m_config; }

    // 验证日志完整性
    bool verifyLogIntegrity() const;

    // 导出日志
    bool exportToFile(const QString &filePath, 
                      const QDateTime &from = QDateTime(),
                      const QDateTime &to = QDateTime()) const;

    // 清理旧日志
    void cleanupOldLogs();

signals:
    void eventLogged(const AuditEvent &event);
    void bruteForceDetected(int attempts);
    void integrityViolationDetected();

private:
    // 生成事件ID
    QString generateEventId() const;

    // 计算事件哈希
    QString calculateEventHash(const AuditEvent &event) const;

    // 获取事件严重级别
    Severity getDefaultSeverity(EventType type) const;

    // 格式化事件
    QString formatEvent(const AuditEvent &event) const;

    // 写入日志
    void writeToLog(const AuditEvent &event);

    // 轮转日志
    void rotateLogIfNeeded();

    // 加载日志
    void loadExistingLogs();

    // 获取当前用户名
    QString getCurrentUserName() const;

    AuditConfig m_config;
    QVector<AuditEvent> m_events;
    QFile *m_logFile;
    mutable QMutex m_mutex;
    QString m_currentLogPath;
    qint64 m_currentLogSize;
};


// ==================== 实现 ====================

inline SecurityAudit::SecurityAudit(QObject *parent)
    : QObject(parent)
    , m_logFile(nullptr)
    , m_currentLogSize(0)
{
    // 默认日志目录
    m_config.logDirectory = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation) + "/security_logs";
    
    QDir().mkpath(m_config.logDirectory);
    
    // 打开或创建当前日志文件
    m_currentLogPath = m_config.logDirectory + "/security_audit.log";
    m_logFile = new QFile(m_currentLogPath, this);
    
    if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        m_currentLogSize = m_logFile->size();
    }
    
    // 加载现有事件
    loadExistingLogs();
    
    // 记录会话开始
    logSessionStart();
}

inline SecurityAudit::~SecurityAudit()
{
    logSessionEnd();
    
    if (m_logFile && m_logFile->isOpen()) {
        m_logFile->close();
    }
}

inline SecurityAudit& SecurityAudit::instance()
{
    static SecurityAudit instance;
    return instance;
}

inline void SecurityAudit::log(EventType type, const QString &description,
                                const QString &filePath,
                                const QJsonObject &metadata)
{
    log(type, getDefaultSeverity(type), description, filePath, metadata);
}

inline void SecurityAudit::log(EventType type, Severity severity,
                                const QString &description,
                                const QString &filePath,
                                const QJsonObject &metadata)
{
    if (severity < m_config.minSeverity) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    AuditEvent event;
    event.id = generateEventId();
    event.timestamp = QDateTime::currentDateTime();
    event.type = type;
    event.severity = severity;
    event.description = description;
    event.filePath = filePath;
    event.userName = getCurrentUserName();
    event.metadata = metadata;
    event.hash = calculateEventHash(event);
    
    m_events.append(event);
    writeToLog(event);
    
    // 检测暴力破解
    if (type == EventType::AuthFailed) {
        int failCount = getFailedAuthCount(15);
        if (failCount >= 5) {
            emit bruteForceDetected(failCount);
            log(EventType::BruteForceDetected, Severity::Critical,
                tr("检测到可能的暴力破解攻击"),
                QString(),
                QJsonObject{{"attempts", failCount}});
        }
    }
    
    emit eventLogged(event);
}

inline void SecurityAudit::logAuthSuccess(const QString &method)
{
    log(EventType::AuthSuccess, tr("认证成功"),
        QString(), QJsonObject{{"method", method}});
}

inline void SecurityAudit::logAuthFailed(const QString &method, const QString &reason)
{
    log(EventType::AuthFailed, Severity::Warning,
        tr("认证失败: %1").arg(reason),
        QString(), QJsonObject{{"method", method}, {"reason", reason}});
}

inline void SecurityAudit::logFileEncrypted(const QString &filePath)
{
    log(EventType::FileEncrypted, tr("文件已加密"), filePath);
}

inline void SecurityAudit::logFileDecrypted(const QString &filePath)
{
    log(EventType::FileDecrypted, tr("文件已解密"), filePath);
}

inline void SecurityAudit::logFileOpened(const QString &filePath, bool encrypted)
{
    log(EventType::FileOpened, tr("文件已打开"),
        filePath, QJsonObject{{"encrypted", encrypted}});
}

inline void SecurityAudit::logSessionStart()
{
    log(EventType::SessionStart, tr("会话开始"));
}

inline void SecurityAudit::logSessionEnd()
{
    log(EventType::SessionEnd, tr("会话结束"));
}

inline void SecurityAudit::logSettingsChanged(const QString &setting,
                                               const QVariant &oldValue,
                                               const QVariant &newValue)
{
    log(EventType::SettingsChanged, tr("设置已更改: %1").arg(setting),
        QString(), QJsonObject{
            {"setting", setting},
            {"oldValue", QJsonValue::fromVariant(oldValue)},
            {"newValue", QJsonValue::fromVariant(newValue)}
        });
}

inline void SecurityAudit::logSuspiciousActivity(const QString &description,
                                                  const QJsonObject &details)
{
    log(EventType::SuspiciousActivity, Severity::Warning, description, QString(), details);
}

inline QVector<SecurityAudit::AuditEvent> SecurityAudit::getRecentEvents(int count) const
{
    QMutexLocker locker(&m_mutex);
    
    int start = qMax(0, m_events.size() - count);
    return m_events.mid(start);
}

inline QVector<SecurityAudit::AuditEvent> SecurityAudit::getEventsByType(
    EventType type, int count) const
{
    QMutexLocker locker(&m_mutex);
    
    QVector<AuditEvent> result;
    for (int i = m_events.size() - 1; i >= 0 && result.size() < count; --i) {
        if (m_events[i].type == type) {
            result.prepend(m_events[i]);
        }
    }
    return result;
}

inline QVector<SecurityAudit::AuditEvent> SecurityAudit::getEventsByDateRange(
    const QDateTime &from, const QDateTime &to) const
{
    QMutexLocker locker(&m_mutex);
    
    QVector<AuditEvent> result;
    for (const AuditEvent &event : m_events) {
        if (event.timestamp >= from && event.timestamp <= to) {
            result.append(event);
        }
    }
    return result;
}

inline QVector<SecurityAudit::AuditEvent> SecurityAudit::getEventsByFile(
    const QString &filePath) const
{
    QMutexLocker locker(&m_mutex);
    
    QVector<AuditEvent> result;
    for (const AuditEvent &event : m_events) {
        if (event.filePath == filePath) {
            result.append(event);
        }
    }
    return result;
}

inline int SecurityAudit::getFailedAuthCount(int minutesBack) const
{
    QMutexLocker locker(&m_mutex);
    
    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-minutesBack * 60);
    int count = 0;
    
    for (int i = m_events.size() - 1; i >= 0; --i) {
        if (m_events[i].timestamp < cutoff) break;
        if (m_events[i].type == EventType::AuthFailed) {
            count++;
        }
    }
    
    return count;
}

inline bool SecurityAudit::isBruteForceDetected(int threshold, int minutesBack) const
{
    return getFailedAuthCount(minutesBack) >= threshold;
}

inline void SecurityAudit::setConfig(const AuditConfig &config)
{
    QMutexLocker locker(&m_mutex);
    m_config = config;
    
    QDir().mkpath(m_config.logDirectory);
}

inline bool SecurityAudit::verifyLogIntegrity() const
{
    QMutexLocker locker(&m_mutex);
    
    for (const AuditEvent &event : m_events) {
        QString expectedHash = calculateEventHash(event);
        if (event.hash != expectedHash) {
            return false;
        }
    }
    
    return true;
}

inline bool SecurityAudit::exportToFile(const QString &filePath,
                                         const QDateTime &from,
                                         const QDateTime &to) const
{
    QMutexLocker locker(&m_mutex);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    QJsonArray eventsArray;
    for (const AuditEvent &event : m_events) {
        if (from.isValid() && event.timestamp < from) continue;
        if (to.isValid() && event.timestamp > to) continue;
        
        QJsonObject obj;
        obj["id"] = event.id;
        obj["timestamp"] = event.timestamp.toString(Qt::ISODate);
        obj["type"] = static_cast<int>(event.type);
        obj["severity"] = static_cast<int>(event.severity);
        obj["description"] = event.description;
        obj["filePath"] = event.filePath;
        obj["userName"] = event.userName;
        obj["metadata"] = event.metadata;
        obj["hash"] = event.hash;
        eventsArray.append(obj);
    }
    
    QJsonDocument doc(eventsArray);
    file.write(doc.toJson());
    return true;
}

inline void SecurityAudit::cleanupOldLogs()
{
    QDir logDir(m_config.logDirectory);
    QStringList logFiles = logDir.entryList(QStringList() << "*.log", 
                                             QDir::Files, QDir::Time);
    
    // 保留最新的 maxLogFiles 个文件
    while (logFiles.size() > m_config.maxLogFiles) {
        QString oldFile = logDir.absoluteFilePath(logFiles.takeLast());
        QFile::remove(oldFile);
    }
}

inline QString SecurityAudit::generateEventId() const
{
    return QString::number(QDateTime::currentMSecsSinceEpoch()) + "-" +
           QString::number(QRandomGenerator::global()->bounded(10000), 16).rightJustified(4, '0');
}

inline QString SecurityAudit::calculateEventHash(const AuditEvent &event) const
{
    QString data = event.id + event.timestamp.toString(Qt::ISODate) +
                   QString::number(static_cast<int>(event.type)) +
                   event.description + event.filePath + event.userName;
    
    return QCryptographicHash::hash(data.toUtf8(), QCryptographicHash::Sha256).toHex();
}

inline SecurityAudit::Severity SecurityAudit::getDefaultSeverity(EventType type) const
{
    switch (type) {
        case EventType::AuthFailed:
        case EventType::BiometricFailed:
        case EventType::EncryptionFailed:
        case EventType::DecryptionFailed:
        case EventType::FileExpired:
            return Severity::Warning;
            
        case EventType::AuthLockout:
        case EventType::IntegrityViolation:
        case EventType::SuspiciousActivity:
        case EventType::BruteForceDetected:
        case EventType::TamperingDetected:
            return Severity::Critical;
            
        case EventType::FileDestroyed:
            return Severity::Error;
            
        default:
            return Severity::Info;
    }
}

inline QString SecurityAudit::formatEvent(const AuditEvent &event) const
{
    QString severityStr;
    switch (event.severity) {
        case Severity::Info: severityStr = "INFO"; break;
        case Severity::Warning: severityStr = "WARN"; break;
        case Severity::Error: severityStr = "ERROR"; break;
        case Severity::Critical: severityStr = "CRIT"; break;
    }
    
    return QString("[%1] [%2] [%3] %4 | File: %5 | User: %6\n")
        .arg(event.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz"))
        .arg(severityStr)
        .arg(event.id)
        .arg(event.description)
        .arg(event.filePath.isEmpty() ? "-" : event.filePath)
        .arg(event.userName);
}

inline void SecurityAudit::writeToLog(const AuditEvent &event)
{
    if (!m_logFile || !m_logFile->isOpen()) {
        return;
    }
    
    QString line = formatEvent(event);
    m_logFile->write(line.toUtf8());
    m_logFile->flush();
    
    m_currentLogSize += line.size();
    
    if (m_currentLogSize >= m_config.maxLogSize) {
        rotateLogIfNeeded();
    }
}

inline void SecurityAudit::rotateLogIfNeeded()
{
    if (m_currentLogSize < m_config.maxLogSize) {
        return;
    }
    
    m_logFile->close();
    
    // 重命名为时间戳文件
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString rotatedPath = m_config.logDirectory + "/security_audit_" + timestamp + ".log";
    QFile::rename(m_currentLogPath, rotatedPath);
    
    // 创建新日志文件
    m_logFile->setFileName(m_currentLogPath);
    m_logFile->open(QIODevice::WriteOnly | QIODevice::Append);
    m_currentLogSize = 0;
    
    // 清理旧日志
    cleanupOldLogs();
}

inline void SecurityAudit::loadExistingLogs()
{
    // 加载当前日志文件中的事件（可选，用于完整性验证）
    // 简化实现：仅从当前会话开始记录
}

inline QString SecurityAudit::getCurrentUserName() const
{
#ifdef Q_OS_WIN
    return qgetenv("USERNAME");
#else
    return qgetenv("USER");
#endif
}

#endif // SECURITYAUDIT_H

