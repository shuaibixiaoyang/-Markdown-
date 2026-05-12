// 文件说明：app-static\security\fileintegrity.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef FILEINTEGRITY_H
#define FILEINTEGRITY_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QFileSystemWatcher>
#include <QMap>
#include <QDateTime>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QStandardPaths>

/**
 * @brief 文件完整性监控器
 * 
 * 功能：
 * - 计算和验证文件哈希
 * - 监控文件变化
 * - 检测篡改
 * - 自动备份
 */
class FileIntegrityMonitor : public QObject
{
    Q_OBJECT

public:
    // 完整性状态
    enum class IntegrityStatus {
        Valid,          // 完整
        Modified,       // 已修改
        Corrupted,      // 损坏
        Missing,        // 文件丢失
        Unknown         // 未知
    };
    Q_ENUM(IntegrityStatus)

    // 文件信息
    struct FileInfo {
        QString path;
        QString sha256Hash;
        QString sha512Hash;
        qint64 size;
        QDateTime lastModified;
        QDateTime lastVerified;
        IntegrityStatus status;
        bool isEncrypted;
    };

    explicit FileIntegrityMonitor(QObject *parent = nullptr);
    ~FileIntegrityMonitor();

    // 单例
    static FileIntegrityMonitor& instance();

    // 添加文件监控
    bool addFile(const QString &filePath);
    bool removeFile(const QString &filePath);
    
    // 验证文件完整性
    IntegrityStatus verifyFile(const QString &filePath);
    bool verifyAllFiles();

    // 获取文件信息
    FileInfo getFileInfo(const QString &filePath) const;
    QStringList monitoredFiles() const;

    // 更新文件哈希（文件被合法修改后调用）
    bool updateFileHash(const QString &filePath);

    // 计算文件哈希
    static QString calculateSHA256(const QString &filePath);
    static QString calculateSHA512(const QString &filePath);
    static QString calculateHash(const QByteArray &data, 
                                  QCryptographicHash::Algorithm algo = QCryptographicHash::Sha256);

    // 导出/导入完整性数据库
    bool exportDatabase(const QString &filePath) const;
    bool importDatabase(const QString &filePath);

    // 设置验证间隔
    void setAutoVerifyInterval(int seconds);

signals:
    void fileModified(const QString &filePath);
    void fileCorrupted(const QString &filePath);
    void fileMissing(const QString &filePath);
    void integrityVerified(const QString &filePath, IntegrityStatus status);
    void suspiciousActivityDetected(const QString &filePath, const QString &details);

private slots:
    void onFileChanged(const QString &path);
    void onDirectoryChanged(const QString &path);
    void performPeriodicVerification();

private:
    void loadDatabase();
    void saveDatabase();
    QString getDatabasePath() const;

    QFileSystemWatcher *m_watcher;
    QMap<QString, FileInfo> m_files;
    QTimer *m_verifyTimer;
    QString m_databasePath;
};


// ==================== 实现 ====================

inline FileIntegrityMonitor::FileIntegrityMonitor(QObject *parent)
    : QObject(parent)
    , m_watcher(new QFileSystemWatcher(this))
    , m_verifyTimer(new QTimer(this))
{
    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &FileIntegrityMonitor::onFileChanged);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &FileIntegrityMonitor::onDirectoryChanged);
    connect(m_verifyTimer, &QTimer::timeout,
            this, &FileIntegrityMonitor::performPeriodicVerification);

    loadDatabase();
}

inline FileIntegrityMonitor::~FileIntegrityMonitor()
{
    saveDatabase();
}

inline FileIntegrityMonitor& FileIntegrityMonitor::instance()
{
    static FileIntegrityMonitor instance;
    return instance;
}

inline bool FileIntegrityMonitor::addFile(const QString &filePath)
{
    if (!QFile::exists(filePath)) {
        return false;
    }

    FileInfo info;
    info.path = filePath;
    info.sha256Hash = calculateSHA256(filePath);
    info.sha512Hash = calculateSHA512(filePath);

    QFileInfo fileInfo(filePath);
    info.size = fileInfo.size();
    info.lastModified = fileInfo.lastModified();
    info.lastVerified = QDateTime::currentDateTime();
    info.status = IntegrityStatus::Valid;
    info.isEncrypted = false;  // 由调用者设置

    m_files[filePath] = info;
    m_watcher->addPath(filePath);

    saveDatabase();
    return true;
}

inline bool FileIntegrityMonitor::removeFile(const QString &filePath)
{
    if (!m_files.contains(filePath)) {
        return false;
    }

    m_files.remove(filePath);
    m_watcher->removePath(filePath);

    saveDatabase();
    return true;
}

inline FileIntegrityMonitor::IntegrityStatus FileIntegrityMonitor::verifyFile(
    const QString &filePath)
{
    if (!m_files.contains(filePath)) {
        return IntegrityStatus::Unknown;
    }

    if (!QFile::exists(filePath)) {
        m_files[filePath].status = IntegrityStatus::Missing;
        emit fileMissing(filePath);
        return IntegrityStatus::Missing;
    }

    QString currentHash = calculateSHA256(filePath);
    FileInfo &info = m_files[filePath];

    if (currentHash != info.sha256Hash) {
        // 进一步验证 SHA512
        QString currentHash512 = calculateSHA512(filePath);
        if (currentHash512 != info.sha512Hash) {
            info.status = IntegrityStatus::Modified;
            emit fileModified(filePath);
            emit suspiciousActivityDetected(filePath, 
                tr("文件哈希不匹配，可能被篡改"));
        }
    } else {
        info.status = IntegrityStatus::Valid;
    }

    info.lastVerified = QDateTime::currentDateTime();
    emit integrityVerified(filePath, info.status);

    return info.status;
}

inline bool FileIntegrityMonitor::verifyAllFiles()
{
    bool allValid = true;

    for (const QString &path : m_files.keys()) {
        IntegrityStatus status = verifyFile(path);
        if (status != IntegrityStatus::Valid) {
            allValid = false;
        }
    }

    return allValid;
}

inline FileIntegrityMonitor::FileInfo FileIntegrityMonitor::getFileInfo(
    const QString &filePath) const
{
    return m_files.value(filePath);
}

inline QStringList FileIntegrityMonitor::monitoredFiles() const
{
    return m_files.keys();
}

inline bool FileIntegrityMonitor::updateFileHash(const QString &filePath)
{
    if (!QFile::exists(filePath)) {
        return false;
    }

    if (!m_files.contains(filePath)) {
        return addFile(filePath);
    }

    FileInfo &info = m_files[filePath];
    info.sha256Hash = calculateSHA256(filePath);
    info.sha512Hash = calculateSHA512(filePath);

    QFileInfo fileInfo(filePath);
    info.size = fileInfo.size();
    info.lastModified = fileInfo.lastModified();
    info.lastVerified = QDateTime::currentDateTime();
    info.status = IntegrityStatus::Valid;

    saveDatabase();
    return true;
}

inline QString FileIntegrityMonitor::calculateSHA256(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    
    // 分块读取大文件
    const int CHUNK_SIZE = 1024 * 1024;  // 1MB
    while (!file.atEnd()) {
        hash.addData(file.read(CHUNK_SIZE));
    }

    return hash.result().toHex();
}

inline QString FileIntegrityMonitor::calculateSHA512(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }

    QCryptographicHash hash(QCryptographicHash::Sha512);
    
    const int CHUNK_SIZE = 1024 * 1024;
    while (!file.atEnd()) {
        hash.addData(file.read(CHUNK_SIZE));
    }

    return hash.result().toHex();
}

inline QString FileIntegrityMonitor::calculateHash(const QByteArray &data,
                                                    QCryptographicHash::Algorithm algo)
{
    return QCryptographicHash::hash(data, algo).toHex();
}

inline bool FileIntegrityMonitor::exportDatabase(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QJsonArray filesArray;
    for (const FileInfo &info : m_files) {
        QJsonObject obj;
        obj["path"] = info.path;
        obj["sha256"] = info.sha256Hash;
        obj["sha512"] = info.sha512Hash;
        obj["size"] = info.size;
        obj["lastModified"] = info.lastModified.toString(Qt::ISODate);
        obj["lastVerified"] = info.lastVerified.toString(Qt::ISODate);
        obj["isEncrypted"] = info.isEncrypted;
        filesArray.append(obj);
    }

    QJsonDocument doc(filesArray);
    file.write(doc.toJson());
    return true;
}

inline bool FileIntegrityMonitor::importDatabase(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonArray filesArray = doc.array();

    for (const QJsonValue &val : filesArray) {
        QJsonObject obj = val.toObject();
        
        FileInfo info;
        info.path = obj["path"].toString();
        info.sha256Hash = obj["sha256"].toString();
        info.sha512Hash = obj["sha512"].toString();
        info.size = obj["size"].toVariant().toLongLong();
        info.lastModified = QDateTime::fromString(
            obj["lastModified"].toString(), Qt::ISODate);
        info.lastVerified = QDateTime::fromString(
            obj["lastVerified"].toString(), Qt::ISODate);
        info.isEncrypted = obj["isEncrypted"].toBool();
        info.status = IntegrityStatus::Unknown;

        m_files[info.path] = info;
        m_watcher->addPath(info.path);
    }

    return true;
}

inline void FileIntegrityMonitor::setAutoVerifyInterval(int seconds)
{
    if (seconds <= 0) {
        m_verifyTimer->stop();
    } else {
        m_verifyTimer->start(seconds * 1000);
    }
}

inline void FileIntegrityMonitor::onFileChanged(const QString &path)
{
    if (!m_files.contains(path)) {
        return;
    }

    // 短暂延迟后验证（让文件写入完成）
    QTimer::singleShot(500, this, [this, path]() {
        verifyFile(path);
    });
}

inline void FileIntegrityMonitor::onDirectoryChanged(const QString &path)
{
    Q_UNUSED(path)
    // 检查目录下的文件
}

inline void FileIntegrityMonitor::performPeriodicVerification()
{
    verifyAllFiles();
}

inline void FileIntegrityMonitor::loadDatabase()
{
    QString dbPath = getDatabasePath();
    if (QFile::exists(dbPath)) {
        importDatabase(dbPath);
    }
}

inline void FileIntegrityMonitor::saveDatabase()
{
    exportDatabase(getDatabasePath());
}

inline QString FileIntegrityMonitor::getDatabasePath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + 
           "/file_integrity.json";
}

#endif // FILEINTEGRITY_H

