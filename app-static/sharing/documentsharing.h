// 文件说明：app-static\sharing\documentsharing.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef DOCUMENTSHARING_H
#define DOCUMENTSHARING_H

#include <QObject>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QDateTime>
#include <QTimer>
#include <QUuid>
#include <QHostAddress>

/**
 * @brief 文档分享管理器
 *
 * 功能：
 * - 生成临时分享链接
 * - 本地 HTTP 服务器托管文档
 * - 分享链接有效期管理
 * - 访问统计
 * - 密码保护分享
 */
class DocumentSharing : public QObject
{
    Q_OBJECT

public:
    // 分享信息
    struct ShareInfo {
        QString id;                 // 分享 ID
        QString title;              // 文档标题
        QString htmlContent;        // HTML 内容
        QString markdownContent;    // Markdown 内容
        QString password;           // 访问密码（可选）
        QDateTime createTime;       // 创建时间
        QDateTime expireTime;       // 过期时间
        int maxViews;               // 最大访问次数 (-1 表示无限)
        int currentViews;           // 当前访问次数
        bool allowDownload;         // 允许下载
        bool isActive;              // 是否激活

        ShareInfo()
            : maxViews(-1)
            , currentViews(0)
            , allowDownload(true)
            , isActive(true)
        {}
    };

    // 分享配置
    struct ShareConfig {
        int port;                   // 服务器端口
        int defaultExpireMinutes;   // 默认过期时间（分钟）
        int defaultMaxViews;        // 默认最大访问次数
        bool requirePassword;       // 默认需要密码
        QString customCss;          // 自定义 CSS

        ShareConfig()
            : port(8080)
            , defaultExpireMinutes(60)
            , defaultMaxViews(-1)
            , requirePassword(false)
        {}
    };

    explicit DocumentSharing(QObject *parent = nullptr);
    ~DocumentSharing();

    // 服务器管理
    bool startServer(int port = 0);
    void stopServer();
    bool isServerRunning() const;
    int serverPort() const;
    QString serverAddress() const;

    // 分享管理
    QString createShare(const QString &title,
                       const QString &htmlContent,
                       const QString &markdownContent = QString(),
                       int expireMinutes = -1,
                       const QString &password = QString());
    bool updateShare(const QString &shareId, const ShareInfo &info);
    bool deleteShare(const QString &shareId);
    bool extendShare(const QString &shareId, int additionalMinutes);

    // 分享信息
    ShareInfo getShareInfo(const QString &shareId) const;
    QList<ShareInfo> getAllShares() const;
    QList<ShareInfo> getActiveShares() const;
    QString getShareUrl(const QString &shareId) const;

    // 统计
    int getTotalViews() const;
    int getActiveShareCount() const;

    // 配置
    void setConfig(const ShareConfig &config);
    ShareConfig config() const { return m_config; }

    // 清理
    void cleanupExpiredShares();

    // 错误信息
    QString lastError() const { return m_lastError; }

signals:
    void serverStarted(int port);
    void serverStopped();
    void shareCreated(const QString &shareId, const QString &url);
    void shareAccessed(const QString &shareId);
    void shareExpired(const QString &shareId);
    void shareDeleted(const QString &shareId);
    void errorOccurred(const QString &error);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();
    void onCleanupTimer();

private:
    void handleHttpRequest(QTcpSocket *socket, const QByteArray &request);
    void sendHttpResponse(QTcpSocket *socket, int statusCode,
                         const QString &contentType, const QByteArray &content);
    void sendSharePage(QTcpSocket *socket, const ShareInfo &share);
    void sendPasswordPage(QTcpSocket *socket, const QString &shareId);
    void sendErrorPage(QTcpSocket *socket, int statusCode, const QString &message);
    void sendDownloadFile(QTcpSocket *socket, const ShareInfo &share);

    QString parseRequestPath(const QByteArray &request);
    QMap<QString, QString> parseQueryParams(const QString &path);
    QMap<QString, QString> parsePostData(const QByteArray &request);
    QString generateShareId();
    QString generateShareHtml(const ShareInfo &share);
    QString getLocalIpAddress() const;

    QTcpServer *m_server;
    QMap<QString, ShareInfo> m_shares;
    QMap<QTcpSocket*, QByteArray> m_pendingRequests;
    QTimer *m_cleanupTimer;
    ShareConfig m_config;
    QString m_lastError;
};

#endif // DOCUMENTSHARING_H

