// 文件说明：app-static\collaboration\collaborationclient.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef COLLABORATIONCLIENT_H
#define COLLABORATIONCLIENT_H

#include <QObject>
#include "compat/websocketcompat.h"
#include <QTimer>
#include <QUrl>
#include <QSslError>

#include "crdtdocument.h"
#include "collaborationserver.h"

namespace Collaboration {

/**
 * @brief WebSocket client for connecting to collaboration sessions
 *
 * Handles connection management, message routing, and
 * synchronization with the collaboration server.
 */
class CollaborationClient : public QObject
{
    Q_OBJECT

public:
    enum class ConnectionState {
        Disconnected,
        Connecting,
        Connected,
        Reconnecting
    };
    Q_ENUM(ConnectionState)

    explicit CollaborationClient(QObject *parent = nullptr);
    ~CollaborationClient();

    // Connection management
    void connectToSession(const QUrl &serverUrl, const QString &sessionId,
                          const QString &userName, const QString &userEmail = QString());
    void disconnect();
    bool isConnected() const;
    ConnectionState state() const { return m_state; }

    // Session info
    QString sessionId() const { return m_sessionId; }
    QString userId() const { return m_userId; }
    User currentUser() const { return m_currentUser; }
    QList<User> users() const { return m_users.values(); }
    User user(const QString &userId) const { return m_users.value(userId); }

    // Document
    CrdtDocument* document() const { return m_document; }
    void setDocument(CrdtDocument *doc);

    // Operations
    void sendOperation(const Operation &op);
    void sendCursorPosition(int position);
    void sendSelection(int start, int end);
    void sendChatMessage(const QString &content);
    void requestSync();

    // Permissions (host only)
    void setUserPermission(const QString &userId, Permission permission);
    void kickUser(const QString &userId);

    // Chat history
    QList<ChatMessage> chatHistory() const { return m_chatHistory; }

signals:
    void connected();
    void disconnected();
    void connectionStateChanged(ConnectionState state);
    void joinedSession(const QString &sessionId, const QString &userId);
    void leftSession();

    void userJoined(const User &user);
    void userLeft(const User &user);
    void userListChanged();

    void cursorMoved(const QString &userId, int position);
    void selectionChanged(const QString &userId, int start, int end);

    void remoteOperationReceived(const Operation &op, const QString &userId);
    void documentSynced();

    void chatMessageReceived(const ChatMessage &message);
    void permissionChanged(const QString &userId, Permission permission);

    void kicked(const QString &reason);
    void error(const QString &message);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessage(const QString &message);
    void onError(QAbstractSocket::SocketError error);
    void onSslErrors(const QList<QSslError> &errors);
    void onReconnectTimer();

private:
    void processMessage(const QJsonObject &msg);
    void handleJoined(const QJsonObject &msg);
    void handleUserJoined(const QJsonObject &msg);
    void handleUserLeft(const QJsonObject &msg);
    void handleOperation(const QJsonObject &msg);
    void handleCursor(const QJsonObject &msg);
    void handleSelection(const QJsonObject &msg);
    void handleChat(const QJsonObject &msg);
    void handleSyncResponse(const QJsonObject &msg);
    void handlePermissionChanged(const QJsonObject &msg);
    void handleKicked(const QJsonObject &msg);
    void handlePing(const QJsonObject &msg);
    void handleSessionClosed(const QJsonObject &msg);

    void sendMessage(const QJsonObject &msg);
    void setState(ConnectionState state);
    void attemptReconnect();
    QUrl normalizeServerUrl(const QUrl &url) const;
    void applyOperationBatch(const QJsonArray &operations);
    void flushPendingOperations();
    void queuePendingOperation(const Operation &op);

    QWebSocket *m_socket;
    QTimer *m_reconnectTimer;
    ConnectionState m_state;

    QUrl m_serverUrl;
    QString m_sessionId;
    QString m_userId;
    QString m_userName;
    QString m_userEmail;

    User m_currentUser;
    QMap<QString, User> m_users;
    CrdtDocument *m_document;
    QList<ChatMessage> m_chatHistory;
    QList<Operation> m_pendingOperations;
    qint64 m_lastServerOpSeq;
    bool m_ignoreSslErrors;
    bool m_manualDisconnect;

    int m_reconnectAttempts;
    static const int MaxReconnectAttempts = 5;
    static const int ReconnectInterval = 3000;
};

} // namespace Collaboration

#endif // COLLABORATIONCLIENT_H

