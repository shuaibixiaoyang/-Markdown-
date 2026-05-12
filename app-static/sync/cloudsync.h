// 文件说明：app-static\sync\cloudsync.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef CLOUDSYNC_H
#define CLOUDSYNC_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QMap>
#include <QUrl>
#include <QFile>

/**
 * @brief 云同步管理器
 *
 * 功能：
 * - WebDAV 同步（坚果云、NextCloud 等）
 * - iCloud 同步（macOS）
 * - 本地文件夹同步
 * - 冲突检测和解决
 * - 自动同步
 * - 同步历史
 */
class CloudSync : public QObject
{
    Q_OBJECT

public:
    // 同步服务类型
    enum class ServiceType {
        WebDAV,         // 通用 WebDAV（坚果云、NextCloud 等）
        iCloud,         // iCloud Drive
        LocalFolder     // 本地文件夹同步
    };
    Q_ENUM(ServiceType)

    // 同步状态
    enum class SyncState {
        Idle,           // 空闲
        Syncing,        // 正在同步
        Uploading,      // 上传中
        Downloading,    // 下载中
        Error,          // 错误
        Paused          // 暂停
    };
    Q_ENUM(SyncState)

    // 冲突解决策略
    enum class ConflictPolicy {
        AskUser,        // 询问用户
        KeepLocal,      // 保留本地
        KeepRemote,     // 保留远程
        KeepBoth,       // 都保留
        KeepNewer       // 保留较新的
    };
    Q_ENUM(ConflictPolicy)

    // 文件同步状态
    enum class FileState {
        Synced,         // 已同步
        Modified,       // 本地已修改
        RemoteModified, // 远程已修改
        Conflict,       // 冲突
        New,            // 新文件
        Deleted,        // 已删除
        Uploading,      // 上传中
        Downloading,    // 下载中
        Error           // 错误
    };
    Q_ENUM(FileState)

    // 服务配置
    struct ServiceConfig {
        ServiceType type;
        QString name;           // 显示名称
        QString serverUrl;      // 服务器地址 (WebDAV)
        QString username;
        QString password;       // 或 token
        QString remotePath;     // 远程路径
        QString localPath;      // 本地路径
        bool autoSync;          // 自动同步
        int syncInterval;       // 同步间隔（秒）
        ConflictPolicy conflictPolicy;
        bool verifySslCertificates;      // 验证 SSL 证书
        bool resumeTransfer;             // 启用断点续传
        QString pinnedCertificateSha256; // 证书指纹（可选）
        QStringList excludePatterns;  // 排除模式

        ServiceConfig()
            : type(ServiceType::WebDAV)
            , autoSync(true)
            , syncInterval(300)  // 5分钟
            , conflictPolicy(ConflictPolicy::AskUser)
            , verifySslCertificates(true)
            , resumeTransfer(true)
        {}
    };

    // 文件信息
    struct FileInfo {
        QString path;           // 相对路径
        QString fullPath;       // 完整路径
        qint64 size;
        QDateTime modifiedTime;
        QString etag;           // ETag (用于检测变化)
        QString md5;            // MD5 校验和
        bool isDirectory;
        FileState state;

        FileInfo() : size(0), isDirectory(false), state(FileState::Synced) {}
    };

    // 同步任务
    struct SyncTask {
        QString localPath;
        QString remotePath;
        bool isUpload;          // true: 上传, false: 下载
        qint64 totalBytes;
        qint64 transferredBytes;
        QDateTime startTime;

        SyncTask() : isUpload(true), totalBytes(0), transferredBytes(0) {}
    };

    // 冲突信息
    struct ConflictInfo {
        QString path;
        FileInfo localFile;
        FileInfo remoteFile;
        QDateTime detectedTime;
    };

    // 同步历史记录
    struct SyncRecord {
        QString path;
        QString action;         // upload, download, delete, conflict
        QDateTime time;
        bool success;
        QString message;

        SyncRecord() : success(true) {}
    };

    explicit CloudSync(QObject *parent = nullptr);
    ~CloudSync();

    // 服务管理
    bool addService(const ServiceConfig &config);
    bool removeService(const QString &name);
    bool updateService(const ServiceConfig &config);
    ServiceConfig getService(const QString &name) const;
    QVector<ServiceConfig> getAllServices() const;

    // 连接测试
    bool testConnection(const ServiceConfig &config);

    // 同步操作
    void startSync(const QString &serviceName = QString());
    void stopSync();
    void pauseSync();
    void resumeSync();
    SyncState syncState() const { return m_state; }

    // 文件操作
    QVector<FileInfo> getLocalFiles(const QString &serviceName);
    QVector<FileInfo> getRemoteFiles(const QString &serviceName);
    FileState getFileState(const QString &serviceName, const QString &path);

    // 单文件同步
    bool uploadFile(const QString &serviceName, const QString &localPath);
    bool downloadFile(const QString &serviceName, const QString &remotePath);
    bool deleteRemoteFile(const QString &serviceName, const QString &remotePath);

    // 冲突处理
    QVector<ConflictInfo> getConflicts() const;
    bool resolveConflict(const QString &path, ConflictPolicy policy);
    void setDefaultConflictPolicy(ConflictPolicy policy);

    // 同步历史
    QVector<SyncRecord> getSyncHistory(int maxCount = 100) const;
    void clearSyncHistory();

    // 排除规则
    void addExcludePattern(const QString &serviceName, const QString &pattern);
    void removeExcludePattern(const QString &serviceName, const QString &pattern);
    bool isExcluded(const QString &serviceName, const QString &path) const;

    // 配置持久化
    bool saveConfig(const QString &filePath);
    bool loadConfig(const QString &filePath);

    // 错误信息
    QString lastError() const { return m_lastError; }

signals:
    void serviceAdded(const QString &name);
    void serviceRemoved(const QString &name);
    void stateChanged(SyncState state);
    void syncStarted(const QString &serviceName);
    void syncProgress(const QString &serviceName, int percent, const QString &message);
    void syncCompleted(const QString &serviceName, int uploaded, int downloaded);
    void syncFailed(const QString &serviceName, const QString &error);
    void fileStateChanged(const QString &path, FileState state);
    void conflictDetected(const ConflictInfo &conflict);
    void transferProgress(const QString &path, qint64 transferred, qint64 total);
    void errorOccurred(const QString &error);

private slots:
    void onAutoSyncTimer();
    void onNetworkReply();
    void onUploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
    // WebDAV 操作
    bool webdavPropfind(const ServiceConfig &config, const QString &path, QVector<FileInfo> &files);
    bool webdavGet(const ServiceConfig &config, const QString &remotePath, const QString &localPath);
    bool webdavPut(const ServiceConfig &config, const QString &localPath, const QString &remotePath);
    bool webdavDelete(const ServiceConfig &config, const QString &remotePath);
    bool webdavMkcol(const ServiceConfig &config, const QString &path);
    qint64 webdavRemoteFileSize(const ServiceConfig &config, const QString &remotePath);

    // iCloud 操作 (macOS)
    bool icloudSync(const ServiceConfig &config);
    QString icloudContainerPath();

    // 本地文件夹同步
    bool localFolderSync(const ServiceConfig &config);

    // 同步逻辑
    void performSync(const ServiceConfig &config);
    void compareAndSync(const ServiceConfig &config,
                        const QVector<FileInfo> &localFiles,
                        const QVector<FileInfo> &remoteFiles,
                        int &uploaded, int &downloaded);
    bool shouldUpload(const FileInfo &local, const FileInfo &remote);
    bool shouldDownload(const FileInfo &local, const FileInfo &remote);

    // 冲突检测
    bool detectConflict(const FileInfo &local, const FileInfo &remote);
    void handleConflict(const ConflictInfo &conflict);

    // 辅助函数
    QString calculateMD5(const QString &filePath);
    QByteArray createBasicAuthHeader(const QString &username, const QString &password);
    QVector<FileInfo> parseWebDavResponse(const QByteArray &xml, const QString &basePath);
    void addSyncRecord(const QString &path, const QString &action, bool success, const QString &message = QString());
    QString getRelativePath(const QString &fullPath, const QString &basePath);
    bool prepareWebDavRequest(const ServiceConfig &config, QNetworkRequest &request);
    void attachSslErrorHandler(QNetworkReply *reply, const ServiceConfig &config, const QString &operation);
    static QString normalizeFingerprint(const QString &fingerprint);

    QMap<QString, ServiceConfig> m_services;
    QNetworkAccessManager *m_networkManager;
    QTimer *m_autoSyncTimer;
    SyncState m_state;
    QString m_lastError;

    // 当前同步状态
    QString m_currentService;
    QVector<SyncTask> m_taskQueue;
    SyncTask m_currentTask;

    // 冲突列表
    QVector<ConflictInfo> m_conflicts;
    ConflictPolicy m_defaultConflictPolicy;

    // 同步历史
    QVector<SyncRecord> m_syncHistory;
    int m_maxHistorySize;

    // 文件状态缓存
    QMap<QString, QMap<QString, FileInfo>> m_fileStateCache;
};

#endif // CLOUDSYNC_H

