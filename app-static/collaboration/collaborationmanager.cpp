// 文件说明：app-static\collaboration\collaborationmanager.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "collaborationmanager.h"
#include <QUrl>
#include <QUrlQuery>
#include <QHostInfo>

namespace Collaboration {

// 函数说明：构造 CollaborationManager 对象，初始化本模块需要的状态、界面和资源。
CollaborationManager::CollaborationManager(QObject *parent)
    : QObject(parent)
    , m_editor(nullptr)
    , m_document(new CrdtDocument(this))
    , m_server(new CollaborationServer(this))
    , m_client(new CollaborationClient(this))
    , m_mode(Mode::Offline)
    , m_ignoreLocalChanges(false)
    , m_ignoreRemoteChanges(false)
{
    // Connect client signals
    connect(m_client, &CollaborationClient::connected,
            this, &CollaborationManager::onClientConnected);
    connect(m_client, &CollaborationClient::disconnected,
            this, &CollaborationManager::onClientDisconnected);
    connect(m_client, &CollaborationClient::connectionStateChanged,
            this, &CollaborationManager::connectionStateChanged);
    connect(m_client, &CollaborationClient::remoteOperationReceived,
            this, &CollaborationManager::onRemoteOperation);
    connect(m_client, &CollaborationClient::cursorMoved,
            this, &CollaborationManager::onRemoteCursor);
    connect(m_client, &CollaborationClient::selectionChanged,
            this, &CollaborationManager::onRemoteSelection);
    connect(m_client, &CollaborationClient::chatMessageReceived,
            this, &CollaborationManager::onChatReceived);
    connect(m_client, &CollaborationClient::userJoined,
            this, &CollaborationManager::userJoined);
    connect(m_client, &CollaborationClient::userLeft,
            this, &CollaborationManager::userLeft);
    connect(m_client, &CollaborationClient::userListChanged,
            this, &CollaborationManager::userListChanged);
    connect(m_client, &CollaborationClient::permissionChanged,
            this, &CollaborationManager::permissionChanged);
    connect(m_client, &CollaborationClient::error,
            this, &CollaborationManager::error);

    // Connect server signals
    connect(m_server, &CollaborationServer::userJoined,
            this, &CollaborationManager::onServerUserJoined);
    connect(m_server, &CollaborationServer::userLeft,
            this, &CollaborationManager::onServerUserLeft);
    connect(m_server, &CollaborationServer::operationReceived,
            this, &CollaborationManager::onServerOperation);
    connect(m_server, &CollaborationServer::error,
            this, &CollaborationManager::error);

    // Connect document signals for synchronization
    connect(m_document, &CrdtDocument::operationGenerated, this, [this](const Operation &op) {
        if (m_mode == Mode::Joined) {
            m_client->sendOperation(op);
        }
        // Server broadcasts operations automatically
    });

    connect(m_document, &CrdtDocument::remoteOperationApplied, this,
            [this](int position, int charsRemoved, int charsAdded) {
        if (!m_ignoreRemoteChanges) {
            applyRemoteChanges(position, charsRemoved, charsAdded);
        }
    });

    m_client->setDocument(m_document);
}

// 函数说明：销毁 CollaborationManager 对象，释放本模块持有的资源。
CollaborationManager::~CollaborationManager()
{
    if (m_mode != Mode::Offline) {
        if (m_mode == Mode::Hosting) {
            stopHosting();
        } else {
            leaveSession();
        }
    }
}

// 函数说明：设置 CollaborationManager 的运行参数，并触发必要的界面或数据刷新。
void CollaborationManager::setEditor(QPlainTextEdit *editor)
{
    if (m_editor) {
        disconnectEditorSignals();
    }

    m_editor = editor;

    if (m_editor) {
        setupEditorConnections();
    }
}

// 函数说明：启动 CollaborationManager 的异步任务、会话或后台流程。
bool CollaborationManager::startHosting(quint16 port)
{
    if (m_mode != Mode::Offline) {
        emit error(tr("Already in a collaboration session"));
        return false;
    }

    const QByteArray certFile = qgetenv("CUTEMARKED_COLLAB_TLS_CERT");
    const QByteArray keyFile = qgetenv("CUTEMARKED_COLLAB_TLS_KEY");
    const QByteArray keyPassphrase = qgetenv("CUTEMARKED_COLLAB_TLS_KEY_PASSPHRASE");

    if (!certFile.isEmpty() || !keyFile.isEmpty()) {
        if (certFile.isEmpty() || keyFile.isEmpty()) {
            emit error(tr("TLS requires both CUTEMARKED_COLLAB_TLS_CERT and CUTEMARKED_COLLAB_TLS_KEY"));
            return false;
        }
        if (!m_server->enableTls(QString::fromUtf8(certFile),
                                 QString::fromUtf8(keyFile),
                                 keyPassphrase)) {
            return false;
        }
    } else {
        m_server->disableTls();
    }

    if (!m_server->start(port)) {
        return false;
    }

    // Create a session
    m_currentSessionId = m_server->createSession(
        QStringLiteral("document"),
        m_userName.isEmpty() ? tr("Host") : m_userName
    );

    // Initialize the CRDT document with current editor content
    initializeDocument();

    // Set document on server
    m_server->setDocument(m_currentSessionId, m_document);

    setMode(Mode::Hosting);

    const QString scheme = m_server->isSecureMode() ? QStringLiteral("wss") : QStringLiteral("ws");
    QString url = QStringLiteral("%1://%2:%3?session=%4")
        .arg(scheme)
        .arg(QHostInfo::localHostName())
        .arg(m_server->port())
        .arg(m_currentSessionId);

    emit sessionStarted(m_currentSessionId, url);
    return true;
}

// 函数说明：停止 CollaborationManager 正在运行的任务或会话。
void CollaborationManager::stopHosting()
{
    if (m_mode != Mode::Hosting) return;

    m_server->closeSession(m_currentSessionId);
    m_server->stop();
    m_currentSessionId.clear();
    m_remoteCursors.clear();

    setMode(Mode::Offline);
    emit sessionEnded();
}

// 函数说明：实现 CollaborationManager::sessionId 的核心逻辑，供当前模块调用。
QString CollaborationManager::sessionId() const
{
    if (m_mode == Mode::Hosting) {
        return m_currentSessionId;
    } else if (m_mode == Mode::Joined) {
        return m_client->sessionId();
    }
    return QString();
}

// 函数说明：实现 CollaborationManager::sessionUrl 的核心逻辑，供当前模块调用。
QString CollaborationManager::sessionUrl() const
{
    if (m_mode == Mode::Hosting && m_server->isRunning()) {
        const QString scheme = m_server->isSecureMode() ? QStringLiteral("wss") : QStringLiteral("ws");
        return QStringLiteral("%1://%2:%3?session=%4")
            .arg(scheme)
            .arg(QHostInfo::localHostName())
            .arg(m_server->port())
            .arg(m_currentSessionId);
    }
    return QString();
}

// 函数说明：实现 CollaborationManager::joinSession 的核心逻辑，供当前模块调用。
bool CollaborationManager::joinSession(const QString &url, const QString &userName, const QString &email)
{
    if (m_mode != Mode::Offline) {
        emit error(tr("Already in a collaboration session"));
        return false;
    }

    QUrl serverUrl(url);
    QUrlQuery query(serverUrl);
    QString sessionId = query.queryItemValue(QStringLiteral("session"));

    if (sessionId.isEmpty()) {
        // Try to extract session ID from path
        sessionId = serverUrl.path().mid(1); // Remove leading /
    }

    if (sessionId.isEmpty()) {
        emit error(tr("Invalid session URL"));
        return false;
    }

    // Remove query parameters for the WebSocket URL
    QUrl wsUrl;
    QString scheme = serverUrl.scheme().toLower();
    if (scheme == QStringLiteral("http")) {
        scheme = QStringLiteral("ws");
    } else if (scheme == QStringLiteral("https")) {
        scheme = QStringLiteral("wss");
    } else if (scheme.isEmpty()) {
        scheme = QStringLiteral("ws");
    }
    wsUrl.setScheme(scheme);
    wsUrl.setHost(serverUrl.host());
    wsUrl.setPort(serverUrl.port());

    m_userName = userName;
    m_userEmail = email;

    m_client->connectToSession(wsUrl, sessionId, userName, email);
    setMode(Mode::Joined);

    return true;
}

// 函数说明：实现 CollaborationManager::leaveSession 的核心逻辑，供当前模块调用。
void CollaborationManager::leaveSession()
{
    if (m_mode != Mode::Joined) return;

    m_client->disconnect();
    m_remoteCursors.clear();

    setMode(Mode::Offline);
    emit sessionEnded();
}

// 函数说明：实现 CollaborationManager::userId 的核心逻辑，供当前模块调用。
QString CollaborationManager::userId() const
{
    if (m_mode == Mode::Joined) {
        return m_client->userId();
    } else if (m_mode == Mode::Hosting) {
        return m_document->siteId();
    }
    return QString();
}

// 函数说明：实现 CollaborationManager::users 的核心逻辑，供当前模块调用。
QList<User> CollaborationManager::users() const
{
    if (m_mode == Mode::Hosting) {
        return m_server->users(m_currentSessionId);
    } else if (m_mode == Mode::Joined) {
        return m_client->users();
    }
    return QList<User>();
}

// 函数说明：设置 CollaborationManager 的运行参数，并触发必要的界面或数据刷新。
void CollaborationManager::setUserPermission(const QString &userId, Permission permission)
{
    if (m_mode == Mode::Hosting) {
        m_server->setUserPermission(m_currentSessionId, userId, permission);
    } else if (m_mode == Mode::Joined) {
        m_client->setUserPermission(userId, permission);
    }
}

// 函数说明：实现 CollaborationManager::kickUser 的核心逻辑，供当前模块调用。
void CollaborationManager::kickUser(const QString &userId)
{
    if (m_mode == Mode::Hosting) {
        m_server->kickUser(m_currentSessionId, userId);
    } else if (m_mode == Mode::Joined) {
        m_client->kickUser(userId);
    }
}

// 函数说明：实现 CollaborationManager::sendChatMessage 的核心逻辑，供当前模块调用。
void CollaborationManager::sendChatMessage(const QString &content)
{
    if (m_mode == Mode::Joined) {
        m_client->sendChatMessage(content);
    }
    // TODO: Handle hosting mode chat
}

// 函数说明：实现 CollaborationManager::chatHistory 的核心逻辑，供当前模块调用。
QList<ChatMessage> CollaborationManager::chatHistory() const
{
    if (m_mode == Mode::Hosting) {
        return m_server->chatHistory(m_currentSessionId);
    } else if (m_mode == Mode::Joined) {
        return m_client->chatHistory();
    }
    return QList<ChatMessage>();
}

// 函数说明：处理云同步操作，把本地文档状态同步到配置的远端。
void CollaborationManager::syncFromEditor()
{
    if (!m_editor) return;

    initializeDocument();
}

// 函数说明：处理云同步操作，把本地文档状态同步到配置的远端。
void CollaborationManager::syncToEditor()
{
    if (!m_editor || !m_document) return;

    m_ignoreLocalChanges = true;

    QTextCursor cursor = m_editor->textCursor();
    int cursorPos = cursor.position();

    m_editor->setPlainText(m_document->text());

    // Restore cursor position
    cursor.setPosition(qMin(cursorPos, m_editor->document()->characterCount() - 1));
    m_editor->setTextCursor(cursor);

    m_ignoreLocalChanges = false;
}

// 函数说明：设置 CollaborationManager 的运行参数，并触发必要的界面或数据刷新。
void CollaborationManager::setMode(Mode mode)
{
    if (m_mode != mode) {
        m_mode = mode;
        emit modeChanged(mode);
    }
}

// 函数说明：初始化 CollaborationManager 的 setupEditorConnections 相关界面、动作或服务连接。
void CollaborationManager::setupEditorConnections()
{
    if (!m_editor) return;

    connect(m_editor->document(), &QTextDocument::contentsChange,
            this, &CollaborationManager::onContentsChange);
    connect(m_editor, &QPlainTextEdit::cursorPositionChanged,
            this, &CollaborationManager::onCursorPositionChanged);
    connect(m_editor, &QPlainTextEdit::selectionChanged,
            this, &CollaborationManager::onSelectionChanged);
}

// 函数说明：实现 CollaborationManager::disconnectEditorSignals 的核心逻辑，供当前模块调用。
void CollaborationManager::disconnectEditorSignals()
{
    if (!m_editor) return;

    disconnect(m_editor->document(), &QTextDocument::contentsChange,
               this, &CollaborationManager::onContentsChange);
    disconnect(m_editor, &QPlainTextEdit::cursorPositionChanged,
               this, &CollaborationManager::onCursorPositionChanged);
    disconnect(m_editor, &QPlainTextEdit::selectionChanged,
               this, &CollaborationManager::onSelectionChanged);
}

// 函数说明：执行 CollaborationManager 的启动初始化流程，把延后加载的功能接入主界面。
void CollaborationManager::initializeDocument()
{
    if (!m_editor) return;

    // Clear and reinitialize CRDT document
    m_document->setState(QJsonArray());

    QString text = m_editor->toPlainText();
    m_ignoreRemoteChanges = true;

    for (int i = 0; i < text.length(); ++i) {
        m_document->localInsert(i, text[i]);
    }

    m_ignoreRemoteChanges = false;
}

// 函数说明：应用 CollaborationManager 当前配置，让编辑器或预览立即生效。
void CollaborationManager::applyRemoteChanges(int position, int charsRemoved, int charsAdded)
{
    if (!m_editor) return;

    m_ignoreLocalChanges = true;

    QTextCursor cursor = m_editor->textCursor();
    int savedPosition = cursor.position();

    cursor.setPosition(position);

    if (charsRemoved > 0) {
        cursor.setPosition(position + charsRemoved, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    }

    if (charsAdded > 0) {
        QString text = m_document->text();
        QString insertedText = text.mid(position, charsAdded);
        cursor.insertText(insertedText);
    }

    // Adjust saved cursor position
    if (savedPosition >= position) {
        savedPosition = savedPosition - charsRemoved + charsAdded;
    }
    savedPosition = qBound(0, savedPosition, m_editor->document()->characterCount() - 1);

    cursor.setPosition(savedPosition);
    m_editor->setTextCursor(cursor);

    m_ignoreLocalChanges = false;
}

// 函数说明：响应 CollaborationManager 收到的信号或异步回调，并更新界面状态。
void CollaborationManager::onContentsChange(int position, int charsRemoved, int charsAdded)
{
    if (m_ignoreLocalChanges || m_mode == Mode::Offline) return;

    // Get the text that was inserted
    QString text = m_editor->toPlainText();

    // Process deletions first
    for (int i = 0; i < charsRemoved; ++i) {
        m_document->localDelete(position);
    }

    // Process insertions
    for (int i = 0; i < charsAdded; ++i) {
        if (position + i < text.length()) {
            m_document->localInsert(position + i, text[position + i]);
        }
    }
}

// 函数说明：响应 CollaborationManager 收到的信号或异步回调，并更新界面状态。
void CollaborationManager::onCursorPositionChanged()
{
    if (m_mode == Mode::Offline || !m_editor) return;

    int position = m_editor->textCursor().position();

    if (m_mode == Mode::Joined) {
        m_client->sendCursorPosition(position);
    }
    // Server broadcasts to all clients
}

// 函数说明：响应 CollaborationManager 收到的信号或异步回调，并更新界面状态。
void CollaborationManager::onSelectionChanged()
{
    if (m_mode == Mode::Offline || !m_editor) return;

    QTextCursor cursor = m_editor->textCursor();
    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();

    if (m_mode == Mode::Joined) {
        m_client->sendSelection(start, end);
    }
}

// 函数说明：响应 CollaborationManager 收到的信号或异步回调，并更新界面状态。
void CollaborationManager::onClientConnected()
{
    emit sessionStarted(m_client->sessionId(), QString());
}

// 函数说明：响应 CollaborationManager 收到的信号或异步回调，并更新界面状态。
void CollaborationManager::onClientDisconnected()
{
    m_remoteCursors.clear();
    setMode(Mode::Offline);
    emit sessionEnded();
}

// 函数说明：响应 CollaborationManager 收到的信号或异步回调，并更新界面状态。
void CollaborationManager::onRemoteOperation(const Operation &op, const QString &userId)
{
    Q_UNUSED(op);
    Q_UNUSED(userId);
    // Document already applied the operation via client
    emit remoteCursorsUpdated();
}

// 函数说明：响应 CollaborationManager 收到的信号或异步回调，并更新界面状态。
void CollaborationManager::onRemoteCursor(const QString &userId, int position)
{
    if (userId == this->userId()) return;

    if (m_remoteCursors.contains(userId)) {
        m_remoteCursors[userId].position = position;
    } else {
        User user;
        if (m_mode == Mode::Joined) {
            user = m_client->user(userId);
        }

        RemoteCursor cursor;
        cursor.userId = userId;
        cursor.userName = user.name;
        cursor.position = position;
        cursor.selectionStart = 0;
        cursor.selectionEnd = 0;
        cursor.color = user.color;
        m_remoteCursors[userId] = cursor;
    }

    emit cursorMoved(userId, position, m_remoteCursors[userId].color);
    emit remoteCursorsUpdated();
}

// 函数说明：响应 CollaborationManager 收到的信号或异步回调，并更新界面状态。
void CollaborationManager::onRemoteSelection(const QString &userId, int start, int end)
{
    if (userId == this->userId()) return;

    if (m_remoteCursors.contains(userId)) {
        m_remoteCursors[userId].selectionStart = start;
        m_remoteCursors[userId].selectionEnd = end;
    }

    QColor color = m_remoteCursors.contains(userId) ? m_remoteCursors[userId].color : QColor(Qt::blue);
    emit selectionChanged(userId, start, end, color);
    emit remoteCursorsUpdated();
}

// 函数说明：响应 CollaborationManager 收到的信号或异步回调，并更新界面状态。
void CollaborationManager::onChatReceived(const ChatMessage &message)
{
    emit chatMessageReceived(message);
}

// 函数说明：响应 CollaborationManager 收到的信号或异步回调，并更新界面状态。
void CollaborationManager::onServerUserJoined(const QString &sessionId, const User &user)
{
    if (sessionId != m_currentSessionId) return;

    RemoteCursor cursor;
    cursor.userId = user.id;
    cursor.userName = user.name;
    cursor.position = 0;
    cursor.selectionStart = 0;
    cursor.selectionEnd = 0;
    cursor.color = user.color;
    m_remoteCursors[user.id] = cursor;

    emit userJoined(user);
    emit userListChanged();
}

// 函数说明：响应 CollaborationManager 收到的信号或异步回调，并更新界面状态。
void CollaborationManager::onServerUserLeft(const QString &sessionId, const User &user)
{
    if (sessionId != m_currentSessionId) return;

    m_remoteCursors.remove(user.id);

    emit userLeft(user);
    emit userListChanged();
    emit remoteCursorsUpdated();
}

// 函数说明：响应 CollaborationManager 收到的信号或异步回调，并更新界面状态。
void CollaborationManager::onServerOperation(const QString &sessionId, const Operation &op)
{
    if (sessionId != m_currentSessionId) return;

    // Operation already applied by server
    Q_UNUSED(op);
}

} // namespace Collaboration

