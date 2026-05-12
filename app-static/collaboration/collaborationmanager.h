// 文件说明：app-static\collaboration\collaborationmanager.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef COLLABORATIONMANAGER_H
#define COLLABORATIONMANAGER_H

#include <QObject>
#include <QPlainTextEdit>
#include <QTextCursor>

#include "crdtdocument.h"
#include "collaborationserver.h"
#include "collaborationclient.h"

namespace Collaboration {

/**
 * @brief Manages the integration between the editor and collaboration system
 *
 * This class bridges the gap between the QPlainTextEdit and the CRDT document,
 * handling local edits, remote operations, and cursor synchronization.
 */
class CollaborationManager : public QObject
{
    Q_OBJECT

public:
    enum class Mode {
        Offline,        // Not in collaboration mode
        Hosting,        // Hosting a session
        Joined          // Joined a remote session
    };
    Q_ENUM(Mode)

    explicit CollaborationManager(QObject *parent = nullptr);
    ~CollaborationManager();

    // Editor binding
    void setEditor(QPlainTextEdit *editor);
    QPlainTextEdit* editor() const { return m_editor; }

    // Mode and state
    Mode mode() const { return m_mode; }
    bool isActive() const { return m_mode != Mode::Offline; }
    bool isHost() const { return m_mode == Mode::Hosting; }

    // Session management - Host
    bool startHosting(quint16 port = 0);
    void stopHosting();
    QString sessionId() const;
    QString sessionUrl() const;

    // Session management - Client
    bool joinSession(const QString &url, const QString &userName, const QString &email = QString());
    void leaveSession();

    // User info
    QString userId() const;
    QString userName() const { return m_userName; }
    void setUserName(const QString &name) { m_userName = name; }
    void setUserEmail(const QString &email) { m_userEmail = email; }

    // User management (host only)
    QList<User> users() const;
    void setUserPermission(const QString &userId, Permission permission);
    void kickUser(const QString &userId);

    // Chat
    void sendChatMessage(const QString &content);
    QList<ChatMessage> chatHistory() const;

    // Components access
    CollaborationServer* server() const { return m_server; }
    CollaborationClient* client() const { return m_client; }
    CrdtDocument* document() const { return m_document; }

signals:
    void modeChanged(Mode mode);
    void sessionStarted(const QString &sessionId, const QString &url);
    void sessionEnded();
    void connectionStateChanged(CollaborationClient::ConnectionState state);

    void userJoined(const User &user);
    void userLeft(const User &user);
    void userListChanged();

    void cursorMoved(const QString &userId, int position, const QColor &color);
    void selectionChanged(const QString &userId, int start, int end, const QColor &color);
    void remoteCursorsUpdated();

    void chatMessageReceived(const ChatMessage &message);
    void permissionChanged(const QString &userId, Permission permission);

    void error(const QString &message);

public slots:
    void syncFromEditor();
    void syncToEditor();

private slots:
    // Editor events
    void onContentsChange(int position, int charsRemoved, int charsAdded);
    void onCursorPositionChanged();
    void onSelectionChanged();

    // Client events
    void onClientConnected();
    void onClientDisconnected();
    void onRemoteOperation(const Operation &op, const QString &userId);
    void onRemoteCursor(const QString &userId, int position);
    void onRemoteSelection(const QString &userId, int start, int end);
    void onChatReceived(const ChatMessage &message);

    // Server events
    void onServerUserJoined(const QString &sessionId, const User &user);
    void onServerUserLeft(const QString &sessionId, const User &user);
    void onServerOperation(const QString &sessionId, const Operation &op);

private:
    void setMode(Mode mode);
    void setupEditorConnections();
    void disconnectEditorSignals();
    void initializeDocument();
    void applyRemoteChanges(int position, int charsRemoved, int charsAdded);

    QPlainTextEdit *m_editor;
    CrdtDocument *m_document;
    CollaborationServer *m_server;
    CollaborationClient *m_client;

    Mode m_mode;
    QString m_userName;
    QString m_userEmail;
    QString m_currentSessionId;

    bool m_ignoreLocalChanges;
    bool m_ignoreRemoteChanges;

    // Track remote cursors for rendering
    struct RemoteCursor {
        QString userId;
        QString userName;
        int position;
        int selectionStart;
        int selectionEnd;
        QColor color;
    };
    QMap<QString, RemoteCursor> m_remoteCursors;
};

} // namespace Collaboration

#endif // COLLABORATIONMANAGER_H

