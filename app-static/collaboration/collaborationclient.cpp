// 文件说明：app-static\collaboration\collaborationclient.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "collaborationclient.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QSslConfiguration>
#include <QStringList>

namespace Collaboration {

// 函数说明：构造 CollaborationClient 对象，初始化本模块需要的状态、界面和资源。
CollaborationClient::CollaborationClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
    , m_reconnectTimer(new QTimer(this))
    , m_state(ConnectionState::Disconnected)
    , m_document(nullptr)
    , m_lastServerOpSeq(-1)
    , m_ignoreSslErrors(false)
    , m_manualDisconnect(false)
    , m_reconnectAttempts(0)
{
    const QByteArray insecureSsl = qgetenv("CUTEMARKED_COLLAB_INSECURE_SSL");
    m_ignoreSslErrors = (insecureSsl == "1" || insecureSsl.compare("true", Qt::CaseInsensitive) == 0);

    connect(m_socket, &QWebSocket::connected,
            this, &CollaborationClient::onConnected);
    connect(m_socket, &QWebSocket::disconnected,
            this, &CollaborationClient::onDisconnected);
    connect(m_socket, &QWebSocket::textMessageReceived,
            this, &CollaborationClient::onTextMessage);
    connect(m_socket, &QWebSocket::errorOccurred,
            this, &CollaborationClient::onError);
    connect(m_socket, &QWebSocket::sslErrors,
            this, &CollaborationClient::onSslErrors);

    connect(m_reconnectTimer, &QTimer::timeout,
            this, &CollaborationClient::onReconnectTimer);
}

// 函数说明：销毁 CollaborationClient 对象，释放本模块持有的资源。
CollaborationClient::~CollaborationClient()
{
    disconnect();
}

// 函数说明：实现 CollaborationClient::connectToSession 的核心逻辑，供当前模块调用。
void CollaborationClient::connectToSession(const QUrl &serverUrl, const QString &sessionId,
                                           const QString &userName, const QString &userEmail)
{
    if (m_state == ConnectionState::Connected || m_state == ConnectionState::Connecting) {
        disconnect();
    }

    m_manualDisconnect = false;
    m_serverUrl = normalizeServerUrl(serverUrl);
    m_sessionId = sessionId;
    m_userName = userName;
    m_userEmail = userEmail;
    m_reconnectAttempts = 0;
    m_users.clear();

    setState(ConnectionState::Connecting);
    m_socket->open(m_serverUrl);
}

// 函数说明：实现 CollaborationClient::disconnect 的核心逻辑，供当前模块调用。
void CollaborationClient::disconnect()
{
    m_manualDisconnect = true;
    m_reconnectTimer->stop();
    m_reconnectAttempts = 0;
    m_lastServerOpSeq = -1;

    if (m_socket->isValid()) {
        // Send leave message
        QJsonObject msg;
        msg["type"] = "leave";
        sendMessage(msg);

        m_socket->close();
    }

    m_users.clear();
    m_chatHistory.clear();
    m_userId.clear();
    m_sessionId.clear();
    m_pendingOperations.clear();

    setState(ConnectionState::Disconnected);
    emit leftSession();
}

// 函数说明：判断 CollaborationClient 当前是否满足指定状态。
bool CollaborationClient::isConnected() const
{
    return m_state == ConnectionState::Connected;
}

// 函数说明：设置 CollaborationClient 的运行参数，并触发必要的界面或数据刷新。
void CollaborationClient::setDocument(CrdtDocument *doc)
{
    m_document = doc;
}

// 函数说明：实现 CollaborationClient::sendOperation 的核心逻辑，供当前模块调用。
void CollaborationClient::sendOperation(const Operation &op)
{
    if (!isConnected()) {
        if (m_state == ConnectionState::Connecting || m_state == ConnectionState::Reconnecting) {
            queuePendingOperation(op);
        }
        return;
    }

    QJsonObject msg;
    msg["type"] = "operation";
    msg["operation"] = op.toJson();
    sendMessage(msg);
}

// 函数说明：实现 CollaborationClient::sendCursorPosition 的核心逻辑，供当前模块调用。
void CollaborationClient::sendCursorPosition(int position)
{
    if (!isConnected()) return;

    QJsonObject msg;
    msg["type"] = "cursor";
    msg["position"] = position;
    sendMessage(msg);
}

// 函数说明：实现 CollaborationClient::sendSelection 的核心逻辑，供当前模块调用。
void CollaborationClient::sendSelection(int start, int end)
{
    if (!isConnected()) return;

    QJsonObject msg;
    msg["type"] = "selection";
    msg["start"] = start;
    msg["end"] = end;
    sendMessage(msg);
}

// 函数说明：实现 CollaborationClient::sendChatMessage 的核心逻辑，供当前模块调用。
void CollaborationClient::sendChatMessage(const QString &content)
{
    if (!isConnected()) return;

    QJsonObject msg;
    msg["type"] = "chat";
    msg["content"] = content;
    sendMessage(msg);
}

// 函数说明：实现 CollaborationClient::requestSync 的核心逻辑，供当前模块调用。
void CollaborationClient::requestSync()
{
    if (!isConnected()) return;

    QJsonObject msg;
    msg["type"] = "sync";
    msg["fromOpSeq"] = m_lastServerOpSeq;
    sendMessage(msg);
}

// 函数说明：设置 CollaborationClient 的运行参数，并触发必要的界面或数据刷新。
void CollaborationClient::setUserPermission(const QString &userId, Permission permission)
{
    if (!isConnected()) return;
    if (m_currentUser.permission != Permission::Owner) return;

    QJsonObject msg;
    msg["type"] = "permission";
    msg["targetUserId"] = userId;
    msg["permission"] = static_cast<int>(permission);
    sendMessage(msg);
}

// 函数说明：实现 CollaborationClient::kickUser 的核心逻辑，供当前模块调用。
void CollaborationClient::kickUser(const QString &userId)
{
    if (!isConnected()) return;
    if (m_currentUser.permission != Permission::Owner) return;

    QJsonObject msg;
    msg["type"] = "kick";
    msg["targetUserId"] = userId;
    sendMessage(msg);
}

// 函数说明：响应 CollaborationClient 收到的信号或异步回调，并更新界面状态。
void CollaborationClient::onConnected()
{
    m_reconnectAttempts = 0;

    // Send join request
    QJsonObject msg;
    msg["type"] = "join";
    msg["sessionId"] = m_sessionId;
    msg["userName"] = m_userName;
    msg["userEmail"] = m_userEmail;
    if (!m_userId.isEmpty()) {
        msg["resumeUserId"] = m_userId;
    }
    msg["lastOpSeq"] = m_lastServerOpSeq;
    sendMessage(msg);
}

// 函数说明：响应 CollaborationClient 收到的信号或异步回调，并更新界面状态。
void CollaborationClient::onDisconnected()
{
    if (m_manualDisconnect) {
        setState(ConnectionState::Disconnected);
        emit disconnected();
        return;
    }

    if (m_state == ConnectionState::Connected || m_state == ConnectionState::Connecting
            || m_state == ConnectionState::Reconnecting) {
        attemptReconnect();
    } else {
        setState(ConnectionState::Disconnected);
        emit disconnected();
    }
}

// 函数说明：响应 CollaborationClient 收到的信号或异步回调，并更新界面状态。
void CollaborationClient::onTextMessage(const QString &message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        emit error(tr("Invalid message received from server"));
        return;
    }

    processMessage(doc.object());
}

// 函数说明：响应 CollaborationClient 收到的信号或异步回调，并更新界面状态。
void CollaborationClient::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    emit error(m_socket->errorString());

    if (!m_manualDisconnect &&
            (m_state == ConnectionState::Connecting || m_state == ConnectionState::Connected)) {
        attemptReconnect();
    }
}

// 函数说明：响应 CollaborationClient 收到的信号或异步回调，并更新界面状态。
void CollaborationClient::onSslErrors(const QList<QSslError> &errors)
{
    if (errors.isEmpty()) {
        return;
    }

    if (m_ignoreSslErrors) {
        m_socket->ignoreSslErrors(errors);
        emit error(tr("SSL errors detected and ignored due to CUTEMARKED_COLLAB_INSECURE_SSL"));
        return;
    }

    QStringList details;
    for (const QSslError &sslError : errors) {
        details.append(sslError.errorString());
    }
    emit error(tr("SSL handshake failed: %1").arg(details.join("; ")));
}

// 函数说明：响应 CollaborationClient 收到的信号或异步回调，并更新界面状态。
void CollaborationClient::onReconnectTimer()
{
    m_reconnectTimer->stop();

    if (m_reconnectAttempts >= MaxReconnectAttempts) {
        setState(ConnectionState::Disconnected);
        emit error(tr("Failed to reconnect after %1 attempts").arg(MaxReconnectAttempts));
        emit disconnected();
        return;
    }

    m_reconnectAttempts++;
    setState(ConnectionState::Reconnecting);
    m_socket->open(m_serverUrl);
}

// 函数说明：处理 CollaborationClient 的核心业务数据，并输出处理结果。
void CollaborationClient::processMessage(const QJsonObject &msg)
{
    QString type = msg["type"].toString();

    if (type == "joined") {
        handleJoined(msg);
    } else if (type == "user_joined") {
        handleUserJoined(msg);
    } else if (type == "user_left") {
        handleUserLeft(msg);
    } else if (type == "operation") {
        handleOperation(msg);
    } else if (type == "cursor") {
        handleCursor(msg);
    } else if (type == "selection") {
        handleSelection(msg);
    } else if (type == "chat") {
        handleChat(msg);
    } else if (type == "sync_response") {
        handleSyncResponse(msg);
    } else if (type == "permission_changed") {
        handlePermissionChanged(msg);
    } else if (type == "kicked") {
        handleKicked(msg);
    } else if (type == "ping") {
        handlePing(msg);
    } else if (type == "session_closed") {
        handleSessionClosed(msg);
    } else if (type == "error") {
        emit error(msg["message"].toString());
    }
}

// 函数说明：处理 CollaborationClient 接收到的事件、请求或用户操作。
void CollaborationClient::handleJoined(const QJsonObject &msg)
{
    m_sessionId = msg["sessionId"].toString();
    const QString joinedUserId = msg["userId"].toString();
    const bool userChanged = (!m_userId.isEmpty() && m_userId != joinedUserId);
    m_userId = joinedUserId;
    m_currentUser = User::fromJson(msg["user"].toObject());
    m_currentUser.socket = nullptr;
    m_manualDisconnect = false;

    if (msg.contains("serverOpSeq")) {
        m_lastServerOpSeq = qMax(m_lastServerOpSeq, msg["serverOpSeq"].toInteger());
    }

    // Load users
    m_users.clear();
    QJsonArray usersArray = msg["users"].toArray();
    for (const auto &val : usersArray) {
        User user = User::fromJson(val.toObject());
        user.socket = nullptr;
        m_users[user.id] = user;
    }

    const bool fullState = msg.value(QStringLiteral("fullState")).toBool(true);
    if (userChanged) {
        m_pendingOperations.clear();
    }

    if (m_document && fullState && msg.contains("documentState")) {
        m_document->setState(msg["documentState"].toArray());
    }

    if (msg.contains("operations")) {
        applyOperationBatch(msg["operations"].toArray());
    }

    if (m_document && fullState && !m_pendingOperations.isEmpty()) {
        // Re-apply local operations generated while disconnected.
        for (const Operation &op : m_pendingOperations) {
            m_document->applyOperation(op);
        }
    }

    // Load chat history
    m_chatHistory.clear();
    QJsonArray chatArray = msg["chatHistory"].toArray();
    for (const auto &val : chatArray) {
        m_chatHistory.append(ChatMessage::fromJson(val.toObject()));
    }

    setState(ConnectionState::Connected);
    flushPendingOperations();
    emit connected();
    emit joinedSession(m_sessionId, m_userId);
    emit userListChanged();
    emit documentSynced();
}

// 函数说明：处理 CollaborationClient 接收到的事件、请求或用户操作。
void CollaborationClient::handleUserJoined(const QJsonObject &msg)
{
    User user = User::fromJson(msg["user"].toObject());
    user.socket = nullptr;
    m_users[user.id] = user;

    emit userJoined(user);
    emit userListChanged();
}

// 函数说明：处理 CollaborationClient 接收到的事件、请求或用户操作。
void CollaborationClient::handleUserLeft(const QJsonObject &msg)
{
    User user = User::fromJson(msg["user"].toObject());
    m_users.remove(user.id);

    emit userLeft(user);
    emit userListChanged();
}

// 函数说明：处理 CollaborationClient 接收到的事件、请求或用户操作。
void CollaborationClient::handleOperation(const QJsonObject &msg)
{
    Operation op = Operation::fromJson(msg["operation"].toObject());
    QString userId = msg["userId"].toString();
    if (msg.contains("opSeq")) {
        m_lastServerOpSeq = qMax(m_lastServerOpSeq, msg["opSeq"].toInteger());
    }

    if (m_document && userId != m_userId) {
        m_document->applyOperation(op);
    }

    emit remoteOperationReceived(op, userId);
}

// 函数说明：处理 CollaborationClient 接收到的事件、请求或用户操作。
void CollaborationClient::handleCursor(const QJsonObject &msg)
{
    QString userId = msg["userId"].toString();
    int position = msg["position"].toInt();

    if (m_users.contains(userId)) {
        m_users[userId].cursorPosition = position;
    }

    emit cursorMoved(userId, position);
}

// 函数说明：处理 CollaborationClient 接收到的事件、请求或用户操作。
void CollaborationClient::handleSelection(const QJsonObject &msg)
{
    QString userId = msg["userId"].toString();
    int start = msg["start"].toInt();
    int end = msg["end"].toInt();

    if (m_users.contains(userId)) {
        m_users[userId].selectionStart = start;
        m_users[userId].selectionEnd = end;
    }

    emit selectionChanged(userId, start, end);
}

// 函数说明：处理 CollaborationClient 接收到的事件、请求或用户操作。
void CollaborationClient::handleChat(const QJsonObject &msg)
{
    ChatMessage message = ChatMessage::fromJson(msg["message"].toObject());
    m_chatHistory.append(message);

    emit chatMessageReceived(message);
}

// 函数说明：处理 CollaborationClient 接收到的事件、请求或用户操作。
void CollaborationClient::handleSyncResponse(const QJsonObject &msg)
{
    if (msg.contains("serverOpSeq")) {
        m_lastServerOpSeq = qMax(m_lastServerOpSeq, msg["serverOpSeq"].toInteger());
    }

    const bool fullState = msg.value(QStringLiteral("fullState")).toBool(false);
    if (m_document && fullState && msg.contains("documentState")) {
        m_document->setState(msg["documentState"].toArray());
        for (const Operation &op : m_pendingOperations) {
            m_document->applyOperation(op);
        }
    }

    if (msg.contains("operations")) {
        applyOperationBatch(msg["operations"].toArray());
    }

    flushPendingOperations();
    emit documentSynced();
}

// 函数说明：处理 CollaborationClient 接收到的事件、请求或用户操作。
void CollaborationClient::handlePermissionChanged(const QJsonObject &msg)
{
    QString userId = msg["userId"].toString();
    Permission perm = static_cast<Permission>(msg["permission"].toInt());

    if (m_users.contains(userId)) {
        m_users[userId].permission = perm;
    }

    if (userId == m_userId) {
        m_currentUser.permission = perm;
    }

    emit permissionChanged(userId, perm);
    emit userListChanged();
}

// 函数说明：处理 CollaborationClient 接收到的事件、请求或用户操作。
void CollaborationClient::handleKicked(const QJsonObject &msg)
{
    QString reason = msg["reason"].toString();

    m_reconnectAttempts = MaxReconnectAttempts; // Don't try to reconnect
    m_manualDisconnect = true;
    m_socket->close();

    m_users.clear();
    m_chatHistory.clear();
    m_pendingOperations.clear();

    setState(ConnectionState::Disconnected);
    emit kicked(reason);
    emit disconnected();
}

// 函数说明：处理 CollaborationClient 接收到的事件、请求或用户操作。
void CollaborationClient::handlePing(const QJsonObject &msg)
{
    Q_UNUSED(msg);

    // Respond with pong
    QJsonObject pong;
    pong["type"] = "pong";
    sendMessage(pong);
}

// 函数说明：处理 CollaborationClient 接收到的事件、请求或用户操作。
void CollaborationClient::handleSessionClosed(const QJsonObject &msg)
{
    Q_UNUSED(msg);

    m_reconnectAttempts = MaxReconnectAttempts; // Don't try to reconnect
    m_manualDisconnect = true;
    m_socket->close();

    m_users.clear();
    m_chatHistory.clear();
    m_pendingOperations.clear();

    setState(ConnectionState::Disconnected);
    emit error(tr("Session has been closed by the host"));
    emit disconnected();
}

// 函数说明：实现 CollaborationClient::sendMessage 的核心逻辑，供当前模块调用。
void CollaborationClient::sendMessage(const QJsonObject &msg)
{
    if (!m_socket->isValid()) return;

    QByteArray data = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    m_socket->sendTextMessage(QString::fromUtf8(data));
}

// 函数说明：设置 CollaborationClient 的运行参数，并触发必要的界面或数据刷新。
void CollaborationClient::setState(ConnectionState state)
{
    if (m_state != state) {
        m_state = state;
        emit connectionStateChanged(state);
    }
}

// 函数说明：实现 CollaborationClient::attemptReconnect 的核心逻辑，供当前模块调用。
void CollaborationClient::attemptReconnect()
{
    if (m_reconnectAttempts >= MaxReconnectAttempts) {
        setState(ConnectionState::Disconnected);
        emit error(tr("Connection lost"));
        emit disconnected();
        return;
    }

    setState(ConnectionState::Reconnecting);
    m_reconnectTimer->start(ReconnectInterval);
}

// 函数说明：实现 CollaborationClient::normalizeServerUrl 的核心逻辑，供当前模块调用。
QUrl CollaborationClient::normalizeServerUrl(const QUrl &url) const
{
    QUrl normalized = url;
    const QString scheme = normalized.scheme().toLower();
    if (scheme == QStringLiteral("http")) {
        normalized.setScheme(QStringLiteral("ws"));
    } else if (scheme == QStringLiteral("https")) {
        normalized.setScheme(QStringLiteral("wss"));
    } else if (scheme.isEmpty()) {
        normalized.setScheme(QStringLiteral("ws"));
    }

    if (normalized.scheme() == QStringLiteral("wss")) {
        QSslConfiguration sslConfiguration = QSslConfiguration::defaultConfiguration();
        m_socket->setSslConfiguration(sslConfiguration);
    }

    return normalized;
}

// 函数说明：应用 CollaborationClient 当前配置，让编辑器或预览立即生效。
void CollaborationClient::applyOperationBatch(const QJsonArray &operations)
{
    for (const QJsonValue &value : operations) {
        const QJsonObject item = value.toObject();
        const QString userId = item.value(QStringLiteral("userId")).toString();
        const qint64 opSeq = item.value(QStringLiteral("opSeq")).toInteger();
        if (opSeq > 0) {
            m_lastServerOpSeq = qMax(m_lastServerOpSeq, opSeq);
        }

        if (!m_document) {
            continue;
        }

        if (userId == m_userId) {
            continue;
        }

        const Operation op = Operation::fromJson(item.value(QStringLiteral("operation")).toObject());
        m_document->applyOperation(op);
        emit remoteOperationReceived(op, userId);
    }
}

// 函数说明：实现 CollaborationClient::flushPendingOperations 的核心逻辑，供当前模块调用。
void CollaborationClient::flushPendingOperations()
{
    if (!isConnected() || m_pendingOperations.isEmpty()) {
        return;
    }

    const QList<Operation> pending = m_pendingOperations;
    m_pendingOperations.clear();
    for (const Operation &op : pending) {
        if (!isConnected()) {
            queuePendingOperation(op);
            continue;
        }

        QJsonObject msg;
        msg["type"] = "operation";
        msg["operation"] = op.toJson();
        sendMessage(msg);
    }
}

// 函数说明：实现 CollaborationClient::queuePendingOperation 的核心逻辑，供当前模块调用。
void CollaborationClient::queuePendingOperation(const Operation &op)
{
    m_pendingOperations.append(op);
}

} // namespace Collaboration

