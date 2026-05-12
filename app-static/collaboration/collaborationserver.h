// 文件说明：app-static\collaboration\collaborationserver.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef COLLABORATIONSERVER_H
#define COLLABORATIONSERVER_H

#include <QObject>
#include "compat/websocketcompat.h"
#include <QMap>
#include <QColor>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QSslConfiguration>

#include "crdtdocument.h"

namespace Collaboration {

// User permission levels
enum class Permission {
    None = 0,
    ReadOnly = 1,
    Comment = 2,
    Edit = 3,
    Owner = 4
};

// Represents a connected user
struct User {
    QString id;
    QString name;
    QString email;
    QColor color;
    Permission permission;
    int cursorPosition;
    int selectionStart;
    int selectionEnd;
    QWebSocket *socket;
    bool isHost;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["name"] = name;
        obj["email"] = email;
        obj["color"] = color.name();
        obj["permission"] = static_cast<int>(permission);
        obj["cursorPosition"] = cursorPosition;
        obj["selectionStart"] = selectionStart;
        obj["selectionEnd"] = selectionEnd;
        obj["isHost"] = isHost;
        return obj;
    }

    static User fromJson(const QJsonObject &obj) {
        User user;
        user.id = obj["id"].toString();
        user.name = obj["name"].toString();
        user.email = obj["email"].toString();
        user.color = QColor(obj["color"].toString());
        user.permission = static_cast<Permission>(obj["permission"].toInt());
        user.cursorPosition = obj["cursorPosition"].toInt();
        user.selectionStart = obj["selectionStart"].toInt();
        user.selectionEnd = obj["selectionEnd"].toInt();
        user.isHost = obj["isHost"].toBool();
        user.socket = nullptr;
        return user;
    }
};

// Chat message
struct ChatMessage {
    QString id;
    QString senderId;
    QString senderName;
    QString content;
    qint64 timestamp;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["senderId"] = senderId;
        obj["senderName"] = senderName;
        obj["content"] = content;
        obj["timestamp"] = timestamp;
        return obj;
    }

    static ChatMessage fromJson(const QJsonObject &obj) {
        ChatMessage msg;
        msg.id = obj["id"].toString();
        msg.senderId = obj["senderId"].toString();
        msg.senderName = obj["senderName"].toString();
        msg.content = obj["content"].toString();
        msg.timestamp = obj["timestamp"].toInteger();
        return msg;
    }
};

/**
 * @brief WebSocket server for real-time collaboration
 *
 * Manages document sessions, user connections, and synchronizes
 * CRDT operations across all connected clients.
 */
class CollaborationServer : public QObject
{
    Q_OBJECT

public:
    explicit CollaborationServer(QObject *parent = nullptr);
    ~CollaborationServer();

    // Server control
    bool start(quint16 port = 0);
    void stop();
    bool isRunning() const;
    quint16 port() const;
    QString serverUrl() const;
    bool isSecureMode() const { return m_secureMode; }
    bool enableTls(const QString &certificateFile,
                   const QString &privateKeyFile,
                   const QByteArray &privateKeyPassphrase = QByteArray());
    void disableTls();

    // Session management
    QString createSession(const QString &documentId, const QString &hostName);
    void closeSession(const QString &sessionId);
    bool hasSession(const QString &sessionId) const;

    // Document sync
    void setDocument(const QString &sessionId, CrdtDocument *doc);
    CrdtDocument* document(const QString &sessionId) const;

    // User management
    QList<User> users(const QString &sessionId) const;
    void setUserPermission(const QString &sessionId, const QString &userId, Permission perm);
    void kickUser(const QString &sessionId, const QString &userId);

    // Chat
    QList<ChatMessage> chatHistory(const QString &sessionId) const;

signals:
    void serverStarted(quint16 port);
    void serverStopped();
    void sessionCreated(const QString &sessionId);
    void sessionClosed(const QString &sessionId);
    void userJoined(const QString &sessionId, const User &user);
    void userLeft(const QString &sessionId, const User &user);
    void cursorMoved(const QString &sessionId, const QString &userId, int position);
    void selectionChanged(const QString &sessionId, const QString &userId, int start, int end);
    void chatMessageReceived(const QString &sessionId, const ChatMessage &message);
    void operationReceived(const QString &sessionId, const Operation &op);
    void error(const QString &message);

private slots:
    void onNewConnection();
    void onTextMessage(const QString &message);
    void onBinaryMessage(const QByteArray &message);
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);
    void heartbeat();

private:
    struct Session;

    void processMessage(QWebSocket *socket, const QJsonObject &msg);
    void handleJoin(QWebSocket *socket, const QJsonObject &msg);
    void handleLeave(QWebSocket *socket, const QJsonObject &msg);
    void handleOperation(QWebSocket *socket, const QJsonObject &msg);
    void handleCursor(QWebSocket *socket, const QJsonObject &msg);
    void handleSelection(QWebSocket *socket, const QJsonObject &msg);
    void handleChat(QWebSocket *socket, const QJsonObject &msg);
    void handleSync(QWebSocket *socket, const QJsonObject &msg);
    void handlePermission(QWebSocket *socket, const QJsonObject &msg);
    void handleKick(QWebSocket *socket, const QJsonObject &msg);

    void broadcast(const QString &sessionId, const QJsonObject &msg, QWebSocket *exclude = nullptr);
    void sendTo(QWebSocket *socket, const QJsonObject &msg);
    void sendError(QWebSocket *socket, const QString &error);

    QString generateSessionId() const;
    QColor generateUserColor() const;
    QString findSessionForSocket(QWebSocket *socket) const;
    User* findUserBySocket(const QString &sessionId, QWebSocket *socket);
    qint64 currentServerOpSeq(const Session &session) const;
    QJsonArray operationDelta(const Session &session, qint64 fromOpSeq, bool *hasGap) const;
    void appendOperationLog(Session &session, qint64 seq, const QString &userId, const QJsonObject &operation);

    struct Session {
        QString id;
        QString documentId;
        QString hostId;
        CrdtDocument *document;
        QMap<QString, User> users;
        QMap<QString, User> knownUsers;
        QList<ChatMessage> chatHistory;
        QList<QJsonObject> operationLog; // [{seq, userId, operation}]
        qint64 nextOperationSeq = 1;
        qint64 createdAt;
    };

    QWebSocketServer *m_server;
    QMap<QString, Session> m_sessions;
    QMap<QWebSocket*, QString> m_socketToSession;
    QMap<QWebSocket*, QString> m_socketToUser;
    QTimer *m_heartbeatTimer;
    quint16 m_port;
    bool m_secureMode;
    QSslConfiguration m_sslConfiguration;

    static const QList<QColor> s_userColors;
    static const int MaxOperationLogEntries = 2000;
};

} // namespace Collaboration

#endif // COLLABORATIONSERVER_H

