// 文件说明：app-static\sharing\documentsharing.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "documentsharing.h"

#include <QNetworkInterface>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>

// 函数说明：构造 DocumentSharing 对象，初始化本模块需要的状态、界面和资源。
DocumentSharing::DocumentSharing(QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_cleanupTimer(new QTimer(this))
{
    connect(m_server, &QTcpServer::newConnection,
            this, &DocumentSharing::onNewConnection);

    m_cleanupTimer->setInterval(60000);  // 每分钟检查一次
    connect(m_cleanupTimer, &QTimer::timeout,
            this, &DocumentSharing::onCleanupTimer);
}

// 函数说明：销毁 DocumentSharing 对象，释放本模块持有的资源。
DocumentSharing::~DocumentSharing()
{
    stopServer();
}

// 函数说明：启动 DocumentSharing 的异步任务、会话或后台流程。
bool DocumentSharing::startServer(int port)
{
    if (m_server->isListening()) {
        return true;
    }

    int targetPort = (port > 0) ? port : m_config.port;

    if (!m_server->listen(QHostAddress::Any, targetPort)) {
        // 尝试自动选择端口
        if (!m_server->listen(QHostAddress::Any, 0)) {
            m_lastError = tr("无法启动服务器: %1").arg(m_server->errorString());
            emit errorOccurred(m_lastError);
            return false;
        }
    }

    m_cleanupTimer->start();
    emit serverStarted(m_server->serverPort());
    return true;
}

// 函数说明：停止 DocumentSharing 正在运行的任务或会话。
void DocumentSharing::stopServer()
{
    if (m_server->isListening()) {
        m_server->close();
        m_cleanupTimer->stop();
        emit serverStopped();
    }
}

// 函数说明：判断 DocumentSharing 当前是否满足指定状态。
bool DocumentSharing::isServerRunning() const
{
    return m_server->isListening();
}

// 函数说明：实现 DocumentSharing::serverPort 的核心逻辑，供当前模块调用。
int DocumentSharing::serverPort() const
{
    return m_server->serverPort();
}

// 函数说明：实现 DocumentSharing::serverAddress 的核心逻辑，供当前模块调用。
QString DocumentSharing::serverAddress() const
{
    if (!m_server->isListening()) {
        return QString();
    }
    return QString("http://%1:%2").arg(getLocalIpAddress()).arg(m_server->serverPort());
}

// 函数说明：创建 DocumentSharing 需要的对象、记录或输出内容。
QString DocumentSharing::createShare(const QString &title,
                                     const QString &htmlContent,
                                     const QString &markdownContent,
                                     int expireMinutes,
                                     const QString &password)
{
    if (!m_server->isListening()) {
        if (!startServer()) {
            return QString();
        }
    }

    ShareInfo share;
    share.id = generateShareId();
    share.title = title;
    share.htmlContent = htmlContent;
    share.markdownContent = markdownContent;
    share.password = password;
    share.createTime = QDateTime::currentDateTime();

    int expire = (expireMinutes > 0) ? expireMinutes : m_config.defaultExpireMinutes;
    share.expireTime = share.createTime.addSecs(expire * 60);

    share.maxViews = m_config.defaultMaxViews;
    share.currentViews = 0;
    share.allowDownload = true;
    share.isActive = true;

    m_shares[share.id] = share;

    QString url = getShareUrl(share.id);
    emit shareCreated(share.id, url);

    return share.id;
}

// 函数说明：刷新 DocumentSharing 的内部状态，并同步到相关界面。
bool DocumentSharing::updateShare(const QString &shareId, const ShareInfo &info)
{
    if (!m_shares.contains(shareId)) {
        m_lastError = tr("分享不存在: %1").arg(shareId);
        return false;
    }

    m_shares[shareId] = info;
    return true;
}

// 函数说明：删除 DocumentSharing 管理的指定数据或资源。
bool DocumentSharing::deleteShare(const QString &shareId)
{
    if (!m_shares.contains(shareId)) {
        m_lastError = tr("分享不存在: %1").arg(shareId);
        return false;
    }

    m_shares.remove(shareId);
    emit shareDeleted(shareId);
    return true;
}

// 函数说明：实现 DocumentSharing::extendShare 的核心逻辑，供当前模块调用。
bool DocumentSharing::extendShare(const QString &shareId, int additionalMinutes)
{
    if (!m_shares.contains(shareId)) {
        m_lastError = tr("分享不存在: %1").arg(shareId);
        return false;
    }

    m_shares[shareId].expireTime = m_shares[shareId].expireTime.addSecs(additionalMinutes * 60);
    m_shares[shareId].isActive = true;
    return true;
}

// 函数说明：读取 DocumentSharing 当前保存的状态或计算结果。
DocumentSharing::ShareInfo DocumentSharing::getShareInfo(const QString &shareId) const
{
    return m_shares.value(shareId);
}

// 函数说明：读取 DocumentSharing 当前保存的状态或计算结果。
QList<DocumentSharing::ShareInfo> DocumentSharing::getAllShares() const
{
    return m_shares.values();
}

// 函数说明：读取 DocumentSharing 当前保存的状态或计算结果。
QList<DocumentSharing::ShareInfo> DocumentSharing::getActiveShares() const
{
    QList<ShareInfo> result;
    QDateTime now = QDateTime::currentDateTime();

    for (const ShareInfo &share : m_shares) {
        if (share.isActive && share.expireTime > now) {
            result.append(share);
        }
    }

    return result;
}

// 函数说明：读取 DocumentSharing 当前保存的状态或计算结果。
QString DocumentSharing::getShareUrl(const QString &shareId) const
{
    if (!m_server->isListening() || !m_shares.contains(shareId)) {
        return QString();
    }

    return QString("%1/share/%2").arg(serverAddress()).arg(shareId);
}

// 函数说明：读取 DocumentSharing 当前保存的状态或计算结果。
int DocumentSharing::getTotalViews() const
{
    int total = 0;
    for (const ShareInfo &share : m_shares) {
        total += share.currentViews;
    }
    return total;
}

// 函数说明：读取 DocumentSharing 当前保存的状态或计算结果。
int DocumentSharing::getActiveShareCount() const
{
    return getActiveShares().count();
}

// 函数说明：设置 DocumentSharing 的运行参数，并触发必要的界面或数据刷新。
void DocumentSharing::setConfig(const ShareConfig &config)
{
    m_config = config;
}

// 函数说明：实现 DocumentSharing::cleanupExpiredShares 的核心逻辑，供当前模块调用。
void DocumentSharing::cleanupExpiredShares()
{
    QDateTime now = QDateTime::currentDateTime();
    QStringList expiredIds;

    for (auto it = m_shares.begin(); it != m_shares.end(); ++it) {
        if (it.value().expireTime <= now) {
            it.value().isActive = false;
            expiredIds.append(it.key());
        }
    }

    for (const QString &id : expiredIds) {
        emit shareExpired(id);
    }
}

// 函数说明：响应 DocumentSharing 收到的信号或异步回调，并更新界面状态。
void DocumentSharing::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead,
                this, &DocumentSharing::onReadyRead);
        connect(socket, &QTcpSocket::disconnected,
                this, &DocumentSharing::onClientDisconnected);
        m_pendingRequests[socket] = QByteArray();
    }
}

// 函数说明：响应 DocumentSharing 收到的信号或异步回调，并更新界面状态。
void DocumentSharing::onReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    m_pendingRequests[socket].append(socket->readAll());

    // 检查是否收到完整的 HTTP 请求
    if (m_pendingRequests[socket].contains("\r\n\r\n")) {
        handleHttpRequest(socket, m_pendingRequests[socket]);
        m_pendingRequests.remove(socket);
    }
}

// 函数说明：响应 DocumentSharing 收到的信号或异步回调，并更新界面状态。
void DocumentSharing::onClientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        m_pendingRequests.remove(socket);
        socket->deleteLater();
    }
}

// 函数说明：响应 DocumentSharing 收到的信号或异步回调，并更新界面状态。
void DocumentSharing::onCleanupTimer()
{
    cleanupExpiredShares();
}

// 函数说明：处理 DocumentSharing 接收到的事件、请求或用户操作。
void DocumentSharing::handleHttpRequest(QTcpSocket *socket, const QByteArray &request)
{
    QString path = parseRequestPath(request);
    QMap<QString, QString> params = parseQueryParams(path);

    // 移除查询参数
    int queryPos = path.indexOf('?');
    if (queryPos > 0) {
        path = path.left(queryPos);
    }

    if (path.startsWith("/share/")) {
        QString shareId = path.mid(7);  // 移除 "/share/"

        if (!m_shares.contains(shareId)) {
            sendErrorPage(socket, 404, tr("分享链接不存在或已过期"));
            return;
        }

        ShareInfo &share = m_shares[shareId];

        // 检查是否过期
        if (share.expireTime <= QDateTime::currentDateTime()) {
            share.isActive = false;
            sendErrorPage(socket, 410, tr("分享链接已过期"));
            return;
        }

        // 检查访问次数
        if (share.maxViews > 0 && share.currentViews >= share.maxViews) {
            sendErrorPage(socket, 410, tr("分享链接已达到最大访问次数"));
            return;
        }

        // 检查密码
        if (!share.password.isEmpty()) {
            QString providedPassword = params.value("password");
            if (providedPassword.isEmpty()) {
                // POST 请求中的密码
                QMap<QString, QString> postData = parsePostData(request);
                providedPassword = postData.value("password");
            }

            if (providedPassword != share.password) {
                sendPasswordPage(socket, shareId);
                return;
            }
        }

        // 更新访问次数
        share.currentViews++;
        emit shareAccessed(shareId);

        // 检查是否下载请求
        if (params.contains("download")) {
            sendDownloadFile(socket, share);
        } else {
            sendSharePage(socket, share);
        }
    } else if (path == "/" || path.isEmpty()) {
        sendErrorPage(socket, 200, tr("CuteMarkEd 文档分享服务"));
    } else {
        sendErrorPage(socket, 404, tr("页面不存在"));
    }
}

// 函数说明：实现 DocumentSharing::sendHttpResponse 的核心逻辑，供当前模块调用。
void DocumentSharing::sendHttpResponse(QTcpSocket *socket, int statusCode,
                                       const QString &contentType, const QByteArray &content)
{
    QString statusText;
    switch (statusCode) {
        case 200: statusText = "OK"; break;
        case 404: statusText = "Not Found"; break;
        case 410: statusText = "Gone"; break;
        default: statusText = "Error"; break;
    }

    QByteArray response;
    response.append(QString("HTTP/1.1 %1 %2\r\n").arg(statusCode).arg(statusText).toUtf8());
    response.append(QString("Content-Type: %1; charset=utf-8\r\n").arg(contentType).toUtf8());
    response.append(QString("Content-Length: %1\r\n").arg(content.size()).toUtf8());
    response.append("Connection: close\r\n");
    response.append("\r\n");
    response.append(content);

    socket->write(response);
    socket->flush();
    socket->disconnectFromHost();
}

// 函数说明：实现 DocumentSharing::sendSharePage 的核心逻辑，供当前模块调用。
void DocumentSharing::sendSharePage(QTcpSocket *socket, const ShareInfo &share)
{
    QString html = generateShareHtml(share);
    sendHttpResponse(socket, 200, "text/html", html.toUtf8());
}

// 函数说明：实现 DocumentSharing::sendPasswordPage 的核心逻辑，供当前模块调用。
void DocumentSharing::sendPasswordPage(QTcpSocket *socket, const QString &shareId)
{
    QString html = QString(R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>需要密码 - CuteMarkEd</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            background: #f5f5f5;
        }
        .container {
            background: white;
            padding: 40px;
            border-radius: 8px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            text-align: center;
        }
        h1 { color: #333; margin-bottom: 20px; }
        form { margin-top: 20px; }
        input[type="password"] {
            padding: 12px;
            font-size: 16px;
            border: 1px solid #ddd;
            border-radius: 4px;
            width: 250px;
        }
        button {
            padding: 12px 24px;
            font-size: 16px;
            background: #007bff;
            color: white;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            margin-left: 10px;
        }
        button:hover { background: #0056b3; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔒 需要密码</h1>
        <p>此文档受密码保护，请输入密码访问。</p>
        <form method="post" action="/share/%1">
            <input type="password" name="password" placeholder="请输入密码" required>
            <button type="submit">访问</button>
        </form>
    </div>
</body>
</html>
)").arg(shareId);

    sendHttpResponse(socket, 200, "text/html", html.toUtf8());
}

// 函数说明：实现 DocumentSharing::sendErrorPage 的核心逻辑，供当前模块调用。
void DocumentSharing::sendErrorPage(QTcpSocket *socket, int statusCode, const QString &message)
{
    QString html = QString(R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>%1 - CuteMarkEd</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            background: #f5f5f5;
        }
        .container {
            background: white;
            padding: 40px;
            border-radius: 8px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            text-align: center;
        }
        h1 { color: #333; }
        p { color: #666; }
    </style>
</head>
<body>
    <div class="container">
        <h1>%2</h1>
        <p>%3</p>
    </div>
</body>
</html>
)").arg(statusCode == 200 ? "CuteMarkEd" : QString::number(statusCode))
  .arg(statusCode == 200 ? "CuteMarkEd 文档分享" : "出错了")
  .arg(message);

    sendHttpResponse(socket, statusCode, "text/html", html.toUtf8());
}

// 函数说明：实现 DocumentSharing::sendDownloadFile 的核心逻辑，供当前模块调用。
void DocumentSharing::sendDownloadFile(QTcpSocket *socket, const ShareInfo &share)
{
    if (!share.allowDownload) {
        sendErrorPage(socket, 403, tr("此文档不允许下载"));
        return;
    }

    QByteArray content = share.markdownContent.isEmpty() ?
                         share.htmlContent.toUtf8() :
                         share.markdownContent.toUtf8();

    QString filename = share.title.isEmpty() ? "document" : share.title;
    filename.replace(QRegularExpression("[^\\w\\-]"), "_");
    filename += share.markdownContent.isEmpty() ? ".html" : ".md";

    QByteArray response;
    response.append("HTTP/1.1 200 OK\r\n");
    response.append("Content-Type: application/octet-stream\r\n");
    response.append(QString("Content-Disposition: attachment; filename=\"%1\"\r\n").arg(filename).toUtf8());
    response.append(QString("Content-Length: %1\r\n").arg(content.size()).toUtf8());
    response.append("Connection: close\r\n");
    response.append("\r\n");
    response.append(content);

    socket->write(response);
    socket->flush();
    socket->disconnectFromHost();
}

// 函数说明：解析输入内容，转换为 DocumentSharing 后续处理使用的数据结构。
QString DocumentSharing::parseRequestPath(const QByteArray &request)
{
    QStringList lines = QString::fromUtf8(request).split("\r\n");
    if (lines.isEmpty()) return QString();

    QStringList parts = lines[0].split(' ');
    if (parts.size() < 2) return QString();

    return QUrl::fromPercentEncoding(parts[1].toUtf8());
}

// 函数说明：解析输入内容，转换为 DocumentSharing 后续处理使用的数据结构。
QMap<QString, QString> DocumentSharing::parseQueryParams(const QString &path)
{
    QMap<QString, QString> params;
    int queryPos = path.indexOf('?');
    if (queryPos < 0) return params;

    QString queryString = path.mid(queryPos + 1);
    QStringList pairs = queryString.split('&');

    for (const QString &pair : pairs) {
        int eqPos = pair.indexOf('=');
        if (eqPos > 0) {
            QString key = QUrl::fromPercentEncoding(pair.left(eqPos).toUtf8());
            QString value = QUrl::fromPercentEncoding(pair.mid(eqPos + 1).toUtf8());
            params[key] = value;
        }
    }

    return params;
}

// 函数说明：解析输入内容，转换为 DocumentSharing 后续处理使用的数据结构。
QMap<QString, QString> DocumentSharing::parsePostData(const QByteArray &request)
{
    QMap<QString, QString> data;

    int bodyStart = request.indexOf("\r\n\r\n");
    if (bodyStart < 0) return data;

    QString body = QString::fromUtf8(request.mid(bodyStart + 4));
    QStringList pairs = body.split('&');

    for (const QString &pair : pairs) {
        int eqPos = pair.indexOf('=');
        if (eqPos > 0) {
            QString key = QUrl::fromPercentEncoding(pair.left(eqPos).toUtf8());
            QString value = QUrl::fromPercentEncoding(pair.mid(eqPos + 1).toUtf8());
            data[key] = value;
        }
    }

    return data;
}

// 函数说明：根据当前数据生成 DocumentSharing 需要的输出结果。
QString DocumentSharing::generateShareId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

// 函数说明：根据当前数据生成 DocumentSharing 需要的输出结果。
QString DocumentSharing::generateShareHtml(const ShareInfo &share)
{
    QString css = m_config.customCss.isEmpty() ? R"(
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            line-height: 1.6;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
            background: #fff;
        }
        .header {
            border-bottom: 1px solid #eee;
            padding-bottom: 10px;
            margin-bottom: 20px;
        }
        .title { margin: 0; color: #333; }
        .meta { color: #999; font-size: 14px; }
        .content { color: #333; }
        .footer {
            margin-top: 40px;
            padding-top: 20px;
            border-top: 1px solid #eee;
            text-align: center;
            color: #999;
            font-size: 12px;
        }
        .download-btn {
            display: inline-block;
            padding: 8px 16px;
            background: #007bff;
            color: white;
            text-decoration: none;
            border-radius: 4px;
            margin-top: 10px;
        }
        .download-btn:hover { background: #0056b3; }
        pre { background: #f5f5f5; padding: 15px; overflow-x: auto; }
        code { background: #f5f5f5; padding: 2px 5px; }
        blockquote { border-left: 4px solid #ddd; margin: 0; padding-left: 20px; color: #666; }
        img { max-width: 100%; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background: #f5f5f5; }
    )" : m_config.customCss;

    QString downloadButton;
    if (share.allowDownload) {
        downloadButton = QString(R"(
            <a href="?download=1" class="download-btn">📥 下载文档</a>
        )");
    }

    QString html = QString(R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>%1 - CuteMarkEd</title>
    <style>%2</style>
</head>
<body>
    <div class="header">
        <h1 class="title">%1</h1>
        <div class="meta">分享于 %3 · 有效期至 %4</div>
    </div>
    <div class="content">
        %5
    </div>
    <div class="footer">
        %6
        <p>由 CuteMarkEd 生成</p>
    </div>
</body>
</html>
)").arg(share.title.isEmpty() ? tr("未命名文档") : share.title)
  .arg(css)
  .arg(share.createTime.toString("yyyy-MM-dd HH:mm"))
  .arg(share.expireTime.toString("yyyy-MM-dd HH:mm"))
  .arg(share.htmlContent)
  .arg(downloadButton);

    return html;
}

// 函数说明：读取 DocumentSharing 当前保存的状态或计算结果。
QString DocumentSharing::getLocalIpAddress() const
{
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();

    for (const QHostAddress &address : addresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol &&
            address != QHostAddress::LocalHost) {
            return address.toString();
        }
    }

    return "127.0.0.1";
}

