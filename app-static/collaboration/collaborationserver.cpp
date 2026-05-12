// 文件说明：app-static\collaboration\collaborationserver.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "collaborationserver.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QUuid>
#include <QDateTime>
#include <QHostAddress>
#include <QRandomGenerator>
#include <QFile>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslSocket>

namespace Collaboration {

const QList<QColor> CollaborationServer::s_userColors = {
    QColor("#FF6B6B"), QColor("#4ECDC4"), QColor("#45B7D1"), QColor("#96CEB4"),
    QColor("#FFEAA7"), QColor("#DDA0DD"), QColor("#98D8C8"), QColor("#F7DC6F"),
    QColor("#BB8FCE"), QColor("#85C1E9"), QColor("#F8B500"), QColor("#00CED1")
};

// 函数说明：构造 CollaborationServer 对象，初始化本模块需要的状态、界面和资源。
CollaborationServer::CollaborationServer(QObject *parent)
    : QObject(parent)
    , m_server(nullptr)
    , m_heartbeatTimer(new QTimer(this))
    , m_port(0)
    , m_secureMode(false)
{
    connect(m_heartbeatTimer, &QTimer::timeout, this, &CollaborationServer::heartbeat);
}

// 函数说明：销毁 CollaborationServer 对象，释放本模块持有的资源。
CollaborationServer::~CollaborationServer()
{
    stop();
}

// 函数说明：启动 CollaborationServer 的异步任务、会话或后台流程。
bool CollaborationServer::start(quint16 port)
{
    if (m_server && m_server->isListening()) {
        return true;
    }

    const auto mode = m_secureMode ? QWebSocketServer::SecureMode
                                   : QWebSocketServer::NonSecureMode;
    m_server = new QWebSocketServer(QStringLiteral("CuteMarkEd Collaboration"),
                                     mode, this);

    if (m_secureMode) {
        m_server->setSslConfiguration(m_sslConfiguration);
    }

    if (m_server->listen(QHostAddress::Any, port)) {
        m_port = m_server->serverPort();

        connect(m_server, &QWebSocketServer::newConnection,
                this, &CollaborationServer::onNewConnection);

        m_heartbeatTimer->start(30000); // Heartbeat every 30 seconds

        emit serverStarted(m_port);
        return true;
    }

    emit error(tr("Failed to start server: %1").arg(m_server->errorString()));
    delete m_server;
    m_server = nullptr;
    return false;
}

// 函数说明：停止 CollaborationServer 正在运行的任务或会话。
void CollaborationServer::stop()
{
    m_heartbeatTimer->stop();

    // Close all sessions
    QStringList sessionIds = m_sessions.keys();
    for (const QString &sessionId : sessionIds) {
        closeSession(sessionId);
    }

    if (m_server) {
        m_server->close();
        delete m_server;
        m_server = nullptr;
    }

    m_port = 0;
    emit serverStopped();
}

// 函数说明：判断 CollaborationServer 当前是否满足指定状态。
bool CollaborationServer::isRunning() const
{
    return m_server && m_server->isListening();
}

// 函数说明：实现 CollaborationServer::port 的核心逻辑，供当前模块调用。
quint16 CollaborationServer::port() const
{
    return m_port;
}

// 函数说明：实现 CollaborationServer::serverUrl 的核心逻辑，供当前模块调用。
QString CollaborationServer::serverUrl() const
{
    if (!isRunning()) return QString();
    const QString scheme = m_secureMode ? QStringLiteral("wss") : QStringLiteral("ws");
    return QStringLiteral("%1://localhost:%2").arg(scheme).arg(m_port);
}

// 函数说明：实现 CollaborationServer::enableTls 的核心逻辑，供当前模块调用。
bool CollaborationServer::enableTls(const QString &certificateFile,
                                    const QString &privateKeyFile,
                                    const QByteArray &privateKeyPassphrase)
{
    if (isRunning()) {
        emit error(tr("Cannot enable TLS while server is running"));
        return false;
    }

    QFile certFile(certificateFile);
    if (!certFile.open(QIODevice::ReadOnly)) {
        emit error(tr("Failed to open TLS certificate: %1").arg(certificateFile));
        return false;
    }
    const QSslCertificate cert(&certFile, QSsl::Pem);
    certFile.close();
    if (cert.isNull()) {
        emit error(tr("Invalid TLS certificate: %1").arg(certificateFile));
        return false;
    }

    QFile keyFile(privateKeyFile);
    if (!keyFile.open(QIODevice::ReadOnly)) {
        emit error(tr("Failed to open TLS private key: %1").arg(privateKeyFile));
        return false;
    }
    const QByteArray keyData = keyFile.readAll();
    keyFile.close();

    QSslKey privateKey(keyData, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey, privateKeyPassphrase);
    if (privateKey.isNull()) {
        privateKey = QSslKey(keyData, QSsl::Ec, QSsl::Pem, QSsl::PrivateKey, privateKeyPassphrase);
    }
    if (privateKey.isNull()) {
        emit error(tr("Invalid TLS private key: %1").arg(privateKeyFile));
        return false;
    }

    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setLocalCertificate(cert);
    config.setPrivateKey(privateKey);
    config.setPeerVerifyMode(QSslSocket::VerifyNone);

    m_sslConfiguration = config;
    m_secureMode = true;
    return true;
}

// 函数说明：实现 CollaborationServer::disableTls 的核心逻辑，供当前模块调用。
void CollaborationServer::disableTls()
{
    if (isRunning()) {
        emit error(tr("Cannot disable TLS while server is running"));
        return;
    }

    m_secureMode = false;
    m_sslConfiguration = QSslConfiguration();
}

// 函数说明：创建 CollaborationServer 需要的对象、记录或输出内容。
QString CollaborationServer::createSession(const QString &documentId, const QString &hostName)
{
    Session session;
    session.id = generateSessionId();
    session.documentId = documentId;
    session.document = new CrdtDocument(this);
    session.createdAt = QDateTime::currentMSecsSinceEpoch();

    // Create host user (will be fully populated when host connects)
    session.hostId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    m_sessions[session.id] = session;

    emit sessionCreated(session.id);
    return session.id;
}

// 函数说明：关闭 CollaborationServer 相关窗口或资源，并处理必要的保存确认。
void CollaborationServer::closeSession(const QString &sessionId)
{
    if (!m_sessions.contains(sessionId)) return;

    Session &session = m_sessions[sessionId];

    // Notify all users and disconnect them
    QJsonObject closeMsg;
    closeMsg["type"] = "session_closed";
    closeMsg["sessionId"] = sessionId;
    broadcast(sessionId, closeMsg);

    // Close all sockets in this session
    for (auto &user : session.users) {
        if (user.socket) {
            m_socketToSession.remove(user.socket);
            m_socketToUser.remove(user.socket);
            user.socket->close();
        }
    }

    delete session.document;
    m_sessions.remove(sessionId);

    emit sessionClosed(sessionId);
}

// 函数说明：检查 CollaborationServer 是否具备对应的数据或能力。
bool CollaborationServer::hasSession(const QString &sessionId) const
{
    return m_sessions.contains(sessionId);
}

// 函数说明：设置 CollaborationServer 的运行参数，并触发必要的界面或数据刷新。
void CollaborationServer::setDocument(const QString &sessionId, CrdtDocument *doc)
{
    if (!m_sessions.contains(sessionId)) return;

    Session &session = m_sessions[sessionId];
    if (session.document) {
        disconnect(session.document, nullptr, this, nullptr);
        if (session.document->parent() == this) {
            delete session.document;
        }
    }
    session.document = doc;

    if (!session.document) {
        return;
    }

    connect(session.document, &CrdtDocument::operationGenerated, this,
            [this, sessionId](const Operation &op) {
        if (!m_sessions.contains(sessionId)) {
            return;
        }

        Session &currentSession = m_sessions[sessionId];
        const qint64 opSeq = currentSession.nextOperationSeq++;
        const QString hostUserId = currentSession.hostId.isEmpty()
            ? QStringLiteral("host")
            : currentSession.hostId;
        const QJsonObject opJson = op.toJson();

        appendOperationLog(currentSession, opSeq, hostUserId, opJson);
        emit operationReceived(sessionId, op);

        QJsonObject broadcastMsg;
        broadcastMsg["type"] = "operation";
        broadcastMsg["operation"] = opJson;
        broadcastMsg["userId"] = hostUserId;
        broadcastMsg["opSeq"] = opSeq;
        broadcast(sessionId, broadcastMsg);
    });
}

// 函数说明：实现 CollaborationServer::document 的核心逻辑，供当前模块调用。
CrdtDocument* CollaborationServer::document(const QString &sessionId) const
{
    if (!m_sessions.contains(sessionId)) return nullptr;
    return m_sessions[sessionId].document;
}

// 函数说明：实现 CollaborationServer::users 的核心逻辑，供当前模块调用。
QList<User> CollaborationServer::users(const QString &sessionId) const
{
    if (!m_sessions.contains(sessionId)) return QList<User>();
    return m_sessions[sessionId].users.values();
}

// 函数说明：设置 CollaborationServer 的运行参数，并触发必要的界面或数据刷新。
void CollaborationServer::setUserPermission(const QString &sessionId, const QString &userId, Permission perm)
{
    if (!m_sessions.contains(sessionId)) return;

    Session &session = m_sessions[sessionId];
    if (!session.users.contains(userId)) return;

    session.users[userId].permission = perm;
    if (session.knownUsers.contains(userId)) {
        session.knownUsers[userId].permission = perm;
    }

    // Notify user of permission change
    QJsonObject msg;
    msg["type"] = "permission_changed";
    msg["userId"] = userId;
    msg["permission"] = static_cast<int>(perm);
    broadcast(sessionId, msg);
}

// 函数说明：实现 CollaborationServer::kickUser 的核心逻辑，供当前模块调用。
void CollaborationServer::kickUser(const QString &sessionId, const QString &userId)
{
    if (!m_sessions.contains(sessionId)) return;

    Session &session = m_sessions[sessionId];
    if (!session.users.contains(userId)) return;

    User user = session.users[userId];
    if (user.socket) {
        QJsonObject msg;
        msg["type"] = "kicked";
        msg["reason"] = "You have been removed from the session";
        sendTo(user.socket, msg);

        m_socketToSession.remove(user.socket);
        m_socketToUser.remove(user.socket);
        user.socket->close();
    }

    session.users.remove(userId);
    session.knownUsers.remove(userId);
    emit userLeft(sessionId, user);

    // Notify others
    QJsonObject notification;
    notification["type"] = "user_left";
    notification["user"] = user.toJson();
    broadcast(sessionId, notification);
}

// 函数说明：实现 CollaborationServer::chatHistory 的核心逻辑，供当前模块调用。
QList<ChatMessage> CollaborationServer::chatHistory(const QString &sessionId) const
{
    if (!m_sessions.contains(sessionId)) return QList<ChatMessage>();
    return m_sessions[sessionId].chatHistory;
}

// 函数说明：响应 CollaborationServer 收到的信号或异步回调，并更新界面状态。
void CollaborationServer::onNewConnection()
{
    QWebSocket *socket = m_server->nextPendingConnection();

    connect(socket, &QWebSocket::textMessageReceived,
            this, &CollaborationServer::onTextMessage);
    connect(socket, &QWebSocket::binaryMessageReceived,
            this, &CollaborationServer::onBinaryMessage);
    connect(socket, &QWebSocket::disconnected,
            this, &CollaborationServer::onDisconnected);
    connect(socket, &QWebSocket::errorOccurred,
            this, &CollaborationServer::onError);
}

// 函数说明：响应 CollaborationServer 收到的信号或异步回调，并更新界面状态。
void CollaborationServer::onTextMessage(const QString &message)
{
    QWebSocket *socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) return;

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        sendError(socket, "Invalid JSON message");
        return;
    }

    processMessage(socket, doc.object());
}

// 函数说明：响应 CollaborationServer 收到的信号或异步回调，并更新界面状态。
void CollaborationServer::onBinaryMessage(const QByteArray &message)
{
    // Binary messages not used in this implementation
    Q_UNUSED(message);
}

// 函数说明：响应 CollaborationServer 收到的信号或异步回调，并更新界面状态。
void CollaborationServer::onDisconnected()
{
    QWebSocket *socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) return;

    QString sessionId = m_socketToSession.value(socket);
    QString userId = m_socketToUser.value(socket);

    if (!sessionId.isEmpty() && m_sessions.contains(sessionId)) {
        Session &session = m_sessions[sessionId];

        if (session.users.contains(userId)) {
            User user = session.users.take(userId);
            user.socket = nullptr;
            session.knownUsers[userId] = user;

            emit userLeft(sessionId, user);

            // Notify others
            QJsonObject msg;
            msg["type"] = "user_left";
            msg["user"] = user.toJson();
            broadcast(sessionId, msg);
        }
    }

    m_socketToSession.remove(socket);
    m_socketToUser.remove(socket);
    socket->deleteLater();
}

// 函数说明：响应 CollaborationServer 收到的信号或异步回调，并更新界面状态。
void CollaborationServer::onError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    QWebSocket *socket = qobject_cast<QWebSocket*>(sender());
    if (socket) {
        emit this->error(socket->errorString());
    }
}

// 函数说明：实现 CollaborationServer::heartbeat 的核心逻辑，供当前模块调用。
void CollaborationServer::heartbeat()
{
    QJsonObject pingMsg;
    pingMsg["type"] = "ping";
    pingMsg["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    for (const QString &sessionId : m_sessions.keys()) {
        broadcast(sessionId, pingMsg);
    }
}

// 函数说明：处理 CollaborationServer 的核心业务数据，并输出处理结果。
void CollaborationServer::processMessage(QWebSocket *socket, const QJsonObject &msg)
{
    QString type = msg["type"].toString();

    if (type == "join") {
        handleJoin(socket, msg);
    } else if (type == "leave") {
        handleLeave(socket, msg);
    } else if (type == "operation") {
        handleOperation(socket, msg);
    } else if (type == "cursor") {
        handleCursor(socket, msg);
    } else if (type == "selection") {
        handleSelection(socket, msg);
    } else if (type == "chat") {
        handleChat(socket, msg);
    } else if (type == "sync") {
        handleSync(socket, msg);
    } else if (type == "permission") {
        handlePermission(socket, msg);
    } else if (type == "kick") {
        handleKick(socket, msg);
    } else if (type == "pong") {
        // Heartbeat response, connection is alive
    } else {
        sendError(socket, "Unknown message type: " + type);
    }
}

// 函数说明：处理 CollaborationServer 接收到的事件、请求或用户操作。
void CollaborationServer::handleJoin(QWebSocket *socket, const QJsonObject &msg)
{
    QString sessionId = msg["sessionId"].toString();
    QString userName = msg["userName"].toString();
    QString userEmail = msg["userEmail"].toString();
    QString resumeUserId = msg["resumeUserId"].toString();
    const qint64 lastOpSeq = msg.contains("lastOpSeq") ? msg["lastOpSeq"].toInteger() : -1;

    if (!m_sessions.contains(sessionId)) {
        sendError(socket, "Session not found");
        return;
    }

    Session &session = m_sessions[sessionId];

    User user;
    bool resumed = false;

    if (!resumeUserId.isEmpty() && session.knownUsers.contains(resumeUserId)) {
        user = session.knownUsers.value(resumeUserId);
        resumed = true;

        if (!userName.isEmpty()) {
            user.name = userName;
        }
        if (!userEmail.isEmpty()) {
            user.email = userEmail;
        }
    } else {
        user.id = resumeUserId.isEmpty()
            ? QUuid::createUuid().toString(QUuid::WithoutBraces)
            : resumeUserId;
        user.name = userName;
        user.email = userEmail;
        user.color = generateUserColor();
        user.permission = Permission::Edit;
        user.cursorPosition = 0;
        user.selectionStart = 0;
        user.selectionEnd = 0;
        user.isHost = session.users.isEmpty() && session.knownUsers.isEmpty();
    }

    user.socket = socket;
    if (user.isHost || session.hostId == user.id) {
        session.hostId = user.id;
        user.isHost = true;
        user.permission = Permission::Owner;
    }

    if (session.users.contains(user.id)) {
        QWebSocket *oldSocket = session.users[user.id].socket;
        if (oldSocket && oldSocket != socket) {
            m_socketToSession.remove(oldSocket);
            m_socketToUser.remove(oldSocket);
            oldSocket->close();
        }
    }

    session.users[user.id] = user;
    session.knownUsers[user.id] = user;
    m_socketToSession[socket] = sessionId;
    m_socketToUser[socket] = user.id;

    emit userJoined(sessionId, user);

    // Send join confirmation with state snapshot or incremental replay.
    QJsonObject response;
    response["type"] = "joined";
    response["sessionId"] = sessionId;
    response["userId"] = user.id;
    response["user"] = user.toJson();
    response["resumed"] = resumed;
    response["serverOpSeq"] = currentServerOpSeq(session);

    // Include all current users
    QJsonArray usersArray;
    for (const User &u : session.users) {
        usersArray.append(u.toJson());
    }
    response["users"] = usersArray;

    bool fullState = true;
    if (resumed && lastOpSeq >= 0) {
        bool hasGap = false;
        const QJsonArray deltaOps = operationDelta(session, lastOpSeq, &hasGap);
        if (!hasGap) {
            response["operations"] = deltaOps;
            fullState = false;
        }
    }

    response["fullState"] = fullState;
    if (fullState && session.document) {
        response["documentState"] = session.document->getState();
    }

    // Include chat history
    QJsonArray chatArray;
    for (const ChatMessage &m : session.chatHistory) {
        chatArray.append(m.toJson());
    }
    response["chatHistory"] = chatArray;

    sendTo(socket, response);

    // Notify others of new user
    QJsonObject notification;
    notification["type"] = "user_joined";
    notification["user"] = user.toJson();
    broadcast(sessionId, notification, socket);
}

// 函数说明：处理 CollaborationServer 接收到的事件、请求或用户操作。
void CollaborationServer::handleLeave(QWebSocket *socket, const QJsonObject &msg)
{
    Q_UNUSED(msg);
    socket->close();
}

// 函数说明：处理 CollaborationServer 接收到的事件、请求或用户操作。
void CollaborationServer::handleOperation(QWebSocket *socket, const QJsonObject &msg)
{
    QString sessionId = m_socketToSession.value(socket);
    QString userId = m_socketToUser.value(socket);

    if (sessionId.isEmpty() || !m_sessions.contains(sessionId)) {
        sendError(socket, "Not in a session");
        return;
    }

    Session &session = m_sessions[sessionId];
    User *user = findUserBySocket(sessionId, socket);

    if (!user || user->permission < Permission::Edit) {
        sendError(socket, "No edit permission");
        return;
    }

    // Parse and apply operation
    Operation op = Operation::fromJson(msg["operation"].toObject());

    if (session.document) {
        session.document->applyOperation(op);
    }

    const qint64 opSeq = session.nextOperationSeq++;
    const QJsonObject operationObj = msg["operation"].toObject();
    appendOperationLog(session, opSeq, userId, operationObj);

    emit operationReceived(sessionId, op);

    // Broadcast to all users, including sender.
    // Sender uses this to confirm latest server op sequence after reconnect.
    QJsonObject broadcastMsg;
    broadcastMsg["type"] = "operation";
    broadcastMsg["operation"] = operationObj;
    broadcastMsg["userId"] = userId;
    broadcastMsg["opSeq"] = opSeq;
    broadcast(sessionId, broadcastMsg);
}

// 函数说明：处理 CollaborationServer 接收到的事件、请求或用户操作。
void CollaborationServer::handleCursor(QWebSocket *socket, const QJsonObject &msg)
{
    QString sessionId = m_socketToSession.value(socket);

    if (sessionId.isEmpty() || !m_sessions.contains(sessionId)) return;

    User *user = findUserBySocket(sessionId, socket);
    if (!user) return;

    int position = msg["position"].toInt();
    user->cursorPosition = position;

    emit cursorMoved(sessionId, user->id, position);

    // Broadcast cursor update
    QJsonObject broadcastMsg;
    broadcastMsg["type"] = "cursor";
    broadcastMsg["userId"] = user->id;
    broadcastMsg["position"] = position;
    broadcast(sessionId, broadcastMsg, socket);
}

// 函数说明：处理 CollaborationServer 接收到的事件、请求或用户操作。
void CollaborationServer::handleSelection(QWebSocket *socket, const QJsonObject &msg)
{
    QString sessionId = m_socketToSession.value(socket);

    if (sessionId.isEmpty() || !m_sessions.contains(sessionId)) return;

    User *user = findUserBySocket(sessionId, socket);
    if (!user) return;

    int start = msg["start"].toInt();
    int end = msg["end"].toInt();
    user->selectionStart = start;
    user->selectionEnd = end;

    emit selectionChanged(sessionId, user->id, start, end);

    // Broadcast selection update
    QJsonObject broadcastMsg;
    broadcastMsg["type"] = "selection";
    broadcastMsg["userId"] = user->id;
    broadcastMsg["start"] = start;
    broadcastMsg["end"] = end;
    broadcast(sessionId, broadcastMsg, socket);
}

// 函数说明：处理 CollaborationServer 接收到的事件、请求或用户操作。
void CollaborationServer::handleChat(QWebSocket *socket, const QJsonObject &msg)
{
    QString sessionId = m_socketToSession.value(socket);

    if (sessionId.isEmpty() || !m_sessions.contains(sessionId)) return;

    User *user = findUserBySocket(sessionId, socket);
    if (!user) return;

    ChatMessage chatMsg;
    chatMsg.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    chatMsg.senderId = user->id;
    chatMsg.senderName = user->name;
    chatMsg.content = msg["content"].toString();
    chatMsg.timestamp = QDateTime::currentMSecsSinceEpoch();

    Session &session = m_sessions[sessionId];
    session.chatHistory.append(chatMsg);

    // Keep only last 100 messages
    while (session.chatHistory.size() > 100) {
        session.chatHistory.removeFirst();
    }

    emit chatMessageReceived(sessionId, chatMsg);

    // Broadcast chat message
    QJsonObject broadcastMsg;
    broadcastMsg["type"] = "chat";
    broadcastMsg["message"] = chatMsg.toJson();
    broadcast(sessionId, broadcastMsg);
}

// 函数说明：处理 CollaborationServer 接收到的事件、请求或用户操作。
void CollaborationServer::handleSync(QWebSocket *socket, const QJsonObject &msg)
{
    QString sessionId = m_socketToSession.value(socket);

    if (sessionId.isEmpty() || !m_sessions.contains(sessionId)) return;

    Session &session = m_sessions[sessionId];
    const qint64 fromOpSeq = msg.contains("fromOpSeq") ? msg["fromOpSeq"].toInteger() : -1;

    QJsonObject response;
    response["type"] = "sync_response";
    response["serverOpSeq"] = currentServerOpSeq(session);

    bool hasGap = false;
    const QJsonArray deltaOps = operationDelta(session, fromOpSeq, &hasGap);

    if (!hasGap) {
        response["fullState"] = false;
        response["operations"] = deltaOps;
    } else {
        response["fullState"] = true;
        if (session.document) {
            response["documentState"] = session.document->getState();
        }
    }

    if (!response.contains("documentState") && session.document && fromOpSeq < 0) {
        // Initial sync requests without sequence baseline receive full state.
        response["fullState"] = true;
        response["documentState"] = session.document->getState();
    }

    sendTo(socket, response);
}

// 函数说明：处理 CollaborationServer 接收到的事件、请求或用户操作。
void CollaborationServer::handlePermission(QWebSocket *socket, const QJsonObject &msg)
{
    QString sessionId = m_socketToSession.value(socket);

    if (sessionId.isEmpty() || !m_sessions.contains(sessionId)) return;

    User *user = findUserBySocket(sessionId, socket);
    if (!user || user->permission != Permission::Owner) {
        sendError(socket, "Only owner can change permissions");
        return;
    }

    QString targetUserId = msg["targetUserId"].toString();
    Permission newPerm = static_cast<Permission>(msg["permission"].toInt());

    setUserPermission(sessionId, targetUserId, newPerm);
}

// 函数说明：处理 CollaborationServer 接收到的事件、请求或用户操作。
void CollaborationServer::handleKick(QWebSocket *socket, const QJsonObject &msg)
{
    QString sessionId = m_socketToSession.value(socket);
    if (sessionId.isEmpty() || !m_sessions.contains(sessionId)) return;

    User *user = findUserBySocket(sessionId, socket);
    if (!user || user->permission != Permission::Owner) {
        sendError(socket, "Only owner can kick users");
        return;
    }

    const QString targetUserId = msg["targetUserId"].toString();
    if (targetUserId.isEmpty()) {
        sendError(socket, "Invalid target user");
        return;
    }
    kickUser(sessionId, targetUserId);
}

// 函数说明：实现 CollaborationServer::broadcast 的核心逻辑，供当前模块调用。
void CollaborationServer::broadcast(const QString &sessionId, const QJsonObject &msg, QWebSocket *exclude)
{
    if (!m_sessions.contains(sessionId)) return;

    QByteArray data = QJsonDocument(msg).toJson(QJsonDocument::Compact);

    for (const User &user : m_sessions[sessionId].users) {
        if (user.socket && user.socket != exclude && user.socket->isValid()) {
            user.socket->sendTextMessage(QString::fromUtf8(data));
        }
    }
}

// 函数说明：实现 CollaborationServer::sendTo 的核心逻辑，供当前模块调用。
void CollaborationServer::sendTo(QWebSocket *socket, const QJsonObject &msg)
{
    if (!socket || !socket->isValid()) return;

    QByteArray data = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    socket->sendTextMessage(QString::fromUtf8(data));
}

// 函数说明：实现 CollaborationServer::sendError 的核心逻辑，供当前模块调用。
void CollaborationServer::sendError(QWebSocket *socket, const QString &error)
{
    QJsonObject msg;
    msg["type"] = "error";
    msg["message"] = error;
    sendTo(socket, msg);
}

// 函数说明：根据当前数据生成 CollaborationServer 需要的输出结果。
QString CollaborationServer::generateSessionId() const
{
    // Generate a short, human-readable session ID
    QString chars = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    QString id;
    for (int i = 0; i < 6; ++i) {
        id += chars[QRandomGenerator::global()->bounded(chars.length())];
    }
    return id;
}

// 函数说明：根据当前数据生成 CollaborationServer 需要的输出结果。
QColor CollaborationServer::generateUserColor() const
{
    return s_userColors[QRandomGenerator::global()->bounded(s_userColors.size())];
}

// 函数说明：实现 CollaborationServer::findSessionForSocket 的核心逻辑，供当前模块调用。
QString CollaborationServer::findSessionForSocket(QWebSocket *socket) const
{
    return m_socketToSession.value(socket);
}

// 函数说明：实现 CollaborationServer::findUserBySocket 的核心逻辑，供当前模块调用。
User* CollaborationServer::findUserBySocket(const QString &sessionId, QWebSocket *socket)
{
    if (!m_sessions.contains(sessionId)) return nullptr;

    QString userId = m_socketToUser.value(socket);
    if (userId.isEmpty()) return nullptr;

    Session &session = m_sessions[sessionId];
    if (!session.users.contains(userId)) return nullptr;

    return &session.users[userId];
}

// 函数说明：实现 CollaborationServer::currentServerOpSeq 的核心逻辑，供当前模块调用。
qint64 CollaborationServer::currentServerOpSeq(const Session &session) const
{
    return qMax<qint64>(0, session.nextOperationSeq - 1);
}

// 函数说明：实现 CollaborationServer::operationDelta 的核心逻辑，供当前模块调用。
QJsonArray CollaborationServer::operationDelta(const Session &session, qint64 fromOpSeq, bool *hasGap) const
{
    if (hasGap) {
        *hasGap = false;
    }

    QJsonArray delta;
    if (fromOpSeq < 0) {
        if (hasGap) {
            *hasGap = true;
        }
        return delta;
    }

    if (fromOpSeq > currentServerOpSeq(session)) {
        if (hasGap) {
            *hasGap = true;
        }
        return delta;
    }

    if (session.operationLog.isEmpty()) {
        return delta;
    }

    const qint64 earliestSeq = session.operationLog.first().value(QStringLiteral("seq")).toInteger();
    if (fromOpSeq + 1 < earliestSeq) {
        if (hasGap) {
            *hasGap = true;
        }
        return delta;
    }

    for (const QJsonObject &entry : session.operationLog) {
        const qint64 seq = entry.value(QStringLiteral("seq")).toInteger();
        if (seq <= fromOpSeq) {
            continue;
        }

        QJsonObject deltaOp;
        deltaOp["opSeq"] = seq;
        deltaOp["userId"] = entry.value(QStringLiteral("userId")).toString();
        deltaOp["operation"] = entry.value(QStringLiteral("operation")).toObject();
        delta.append(deltaOp);
    }
    return delta;
}

// 函数说明：实现 CollaborationServer::appendOperationLog 的核心逻辑，供当前模块调用。
void CollaborationServer::appendOperationLog(Session &session, qint64 seq, const QString &userId, const QJsonObject &operation)
{
    QJsonObject entry;
    entry["seq"] = seq;
    entry["userId"] = userId;
    entry["operation"] = operation;
    session.operationLog.append(entry);

    while (session.operationLog.size() > MaxOperationLogEntries) {
        session.operationLog.removeFirst();
    }
}

} // namespace Collaboration

