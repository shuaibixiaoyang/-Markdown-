// 文件说明：app-static\collaboration\collaborationpanel.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "collaborationpanel.h"
#include <QApplication>
#include <QMessageBox>
#include <QInputDialog>

namespace Collaboration {

// 函数说明：构造 CollaborationPanel 对象，初始化本模块需要的状态、界面和资源。
CollaborationPanel::CollaborationPanel(QWidget *parent)
    : QWidget(parent)
    , m_manager(nullptr)
    , m_mainLayout(new QVBoxLayout(this))
    , m_stackedWidget(new QStackedWidget(this))
    , m_userContextMenu(new QMenu(this))
{
    setupUi();
}

// 函数说明：设置 CollaborationPanel 的运行参数，并触发必要的界面或数据刷新。
void CollaborationPanel::setCollaborationManager(CollaborationManager *manager)
{
    if (m_manager) {
        disconnect(m_manager, nullptr, this, nullptr);
    }

    m_manager = manager;

    if (m_manager) {
        connect(m_manager, &CollaborationManager::modeChanged,
                this, &CollaborationPanel::onModeChanged);
        connect(m_manager, &CollaborationManager::sessionStarted,
                this, &CollaborationPanel::onSessionStarted);
        connect(m_manager, &CollaborationManager::sessionEnded,
                this, &CollaborationPanel::onSessionEnded);
        connect(m_manager, &CollaborationManager::userListChanged,
                this, &CollaborationPanel::onUserListChanged);
        connect(m_manager, &CollaborationManager::chatMessageReceived,
                this, &CollaborationPanel::onChatMessageReceived);
        connect(m_manager, &CollaborationManager::connectionStateChanged,
                this, &CollaborationPanel::onConnectionStateChanged);
        connect(m_manager, &CollaborationManager::error,
                this, &CollaborationPanel::onError);

        connect(m_chatWidget, &ChatWidget::messageSent, m_manager, [this](const QString &content) {
            if (m_manager) {
                m_manager->sendChatMessage(content);
            }
        });

        // Update UI based on current state
        onModeChanged(m_manager->mode());
    }
}

// 函数说明：初始化 CollaborationPanel 的 setupUi 相关界面、动作或服务连接。
void CollaborationPanel::setupUi()
{
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    setupStartPage();
    setupSessionPage();

    m_stackedWidget->addWidget(m_startPage);
    m_stackedWidget->addWidget(m_sessionPage);
    m_stackedWidget->setCurrentIndex(0);

    m_mainLayout->addWidget(m_stackedWidget);

    // Setup user context menu
    m_setReadOnlyAction = m_userContextMenu->addAction(tr("Set Read-Only"));
    m_setCommentAction = m_userContextMenu->addAction(tr("Set Comment"));
    m_setEditAction = m_userContextMenu->addAction(tr("Set Edit"));
    m_userContextMenu->addSeparator();
    m_kickUserAction = m_userContextMenu->addAction(tr("Kick User"));

    connect(m_setReadOnlyAction, &QAction::triggered, this, &CollaborationPanel::onUserPermissionAction);
    connect(m_setCommentAction, &QAction::triggered, this, &CollaborationPanel::onUserPermissionAction);
    connect(m_setEditAction, &QAction::triggered, this, &CollaborationPanel::onUserPermissionAction);
    connect(m_kickUserAction, &QAction::triggered, this, &CollaborationPanel::onKickUserAction);
}

// 函数说明：初始化 CollaborationPanel 的 setupStartPage 相关界面、动作或服务连接。
void CollaborationPanel::setupStartPage()
{
    m_startPage = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(m_startPage);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(16);

    // Title
    QLabel *titleLabel = new QLabel(tr("Real-time Collaboration"), m_startPage);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    // User name
    QGroupBox *userGroup = new QGroupBox(tr("Your Name"), m_startPage);
    QVBoxLayout *userLayout = new QVBoxLayout(userGroup);
    m_userNameInput = new QLineEdit(userGroup);
    m_userNameInput->setPlaceholderText(tr("Enter your display name"));
    m_userNameInput->setText(tr("User"));
    userLayout->addWidget(m_userNameInput);
    layout->addWidget(userGroup);

    // Host session
    QGroupBox *hostGroup = new QGroupBox(tr("Host Session"), m_startPage);
    QVBoxLayout *hostLayout = new QVBoxLayout(hostGroup);
    QLabel *hostDesc = new QLabel(tr("Start hosting a new collaboration session. Others can join using the generated link."), hostGroup);
    hostDesc->setWordWrap(true);
    hostDesc->setStyleSheet("color: #6c757d;");
    hostLayout->addWidget(hostDesc);
    m_startHostingButton = new QPushButton(tr("Start Hosting"), hostGroup);
    m_startHostingButton->setStyleSheet(
        "QPushButton {"
        "  padding: 10px 20px;"
        "  background-color: #28a745;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #218838; }"
        "QPushButton:pressed { background-color: #1e7e34; }"
    );
    hostLayout->addWidget(m_startHostingButton);
    layout->addWidget(hostGroup);

    // Join session
    QGroupBox *joinGroup = new QGroupBox(tr("Join Session"), m_startPage);
    QVBoxLayout *joinLayout = new QVBoxLayout(joinGroup);
    QLabel *joinDesc = new QLabel(tr("Enter the session URL to join an existing collaboration session."), joinGroup);
    joinDesc->setWordWrap(true);
    joinDesc->setStyleSheet("color: #6c757d;");
    joinLayout->addWidget(joinDesc);
    m_sessionUrlInput = new QLineEdit(joinGroup);
    m_sessionUrlInput->setPlaceholderText(tr("ws://hostname:port?session=XXXXXX  or  wss://hostname:port?session=XXXXXX"));
    joinLayout->addWidget(m_sessionUrlInput);
    m_joinSessionButton = new QPushButton(tr("Join Session"), joinGroup);
    m_joinSessionButton->setStyleSheet(
        "QPushButton {"
        "  padding: 10px 20px;"
        "  background-color: #007bff;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #0056b3; }"
        "QPushButton:pressed { background-color: #004085; }"
    );
    joinLayout->addWidget(m_joinSessionButton);
    layout->addWidget(joinGroup);

    layout->addStretch();

    // Connections
    connect(m_startHostingButton, &QPushButton::clicked,
            this, &CollaborationPanel::onStartHostingClicked);
    connect(m_joinSessionButton, &QPushButton::clicked,
            this, &CollaborationPanel::onJoinSessionClicked);
}

// 函数说明：初始化 CollaborationPanel 的 setupSessionPage 相关界面、动作或服务连接。
void CollaborationPanel::setupSessionPage()
{
    m_sessionPage = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(m_sessionPage);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    // Status bar
    QHBoxLayout *statusLayout = new QHBoxLayout();
    m_statusLabel = new QLabel(tr("Disconnected"), m_sessionPage);
    m_statusLabel->setStyleSheet(
        "QLabel {"
        "  padding: 4px 8px;"
        "  background-color: #dc3545;"
        "  color: white;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
    );
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();
    layout->addLayout(statusLayout);

    // Session info
    QGroupBox *sessionGroup = new QGroupBox(tr("Session"), m_sessionPage);
    QVBoxLayout *sessionLayout = new QVBoxLayout(sessionGroup);

    QHBoxLayout *idLayout = new QHBoxLayout();
    QLabel *idTitleLabel = new QLabel(tr("Session ID:"), sessionGroup);
    m_sessionIdLabel = new QLabel(tr("N/A"), sessionGroup);
    m_sessionIdLabel->setStyleSheet("font-weight: bold; font-family: monospace;");
    idLayout->addWidget(idTitleLabel);
    idLayout->addWidget(m_sessionIdLabel);
    idLayout->addStretch();
    sessionLayout->addLayout(idLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_copyLinkButton = new QPushButton(tr("Copy Link"), sessionGroup);
    m_copyLinkButton->setStyleSheet(
        "QPushButton {"
        "  padding: 6px 12px;"
        "  background-color: #6c757d;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover { background-color: #5a6268; }"
    );
    buttonLayout->addWidget(m_copyLinkButton);

    m_disconnectButton = new QPushButton(tr("Disconnect"), sessionGroup);
    m_disconnectButton->setStyleSheet(
        "QPushButton {"
        "  padding: 6px 12px;"
        "  background-color: #dc3545;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover { background-color: #c82333; }"
    );
    buttonLayout->addWidget(m_disconnectButton);
    buttonLayout->addStretch();
    sessionLayout->addLayout(buttonLayout);

    layout->addWidget(sessionGroup);

    // User list
    m_userListGroup = new QGroupBox(tr("Collaborators"), m_sessionPage);
    QVBoxLayout *userLayout = new QVBoxLayout(m_userListGroup);
    m_userList = new QListWidget(m_userListGroup);
    m_userList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_userList->setStyleSheet(
        "QListWidget {"
        "  border: 1px solid #dee2e6;"
        "  border-radius: 4px;"
        "}"
        "QListWidget::item {"
        "  padding: 8px;"
        "  border-bottom: 1px solid #e9ecef;"
        "}"
    );
    userLayout->addWidget(m_userList);
    layout->addWidget(m_userListGroup);

    // Chat widget
    m_chatWidget = new ChatWidget(m_sessionPage);
    layout->addWidget(m_chatWidget, 1);

    // Connections
    connect(m_copyLinkButton, &QPushButton::clicked,
            this, &CollaborationPanel::onCopyLinkClicked);
    connect(m_disconnectButton, &QPushButton::clicked,
            this, &CollaborationPanel::onDisconnectClicked);
    connect(m_userList, &QListWidget::customContextMenuRequested,
            this, &CollaborationPanel::onUserItemContextMenu);
}

// 函数说明：响应 CollaborationPanel 收到的信号或异步回调，并更新界面状态。
void CollaborationPanel::onStartHostingClicked()
{
    if (!m_manager) return;

    QString userName = m_userNameInput->text().trimmed();
    if (userName.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter your name."));
        return;
    }

    m_manager->setUserName(userName);

    if (!m_manager->startHosting()) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to start hosting session."));
    }
}

// 函数说明：响应 CollaborationPanel 收到的信号或异步回调，并更新界面状态。
void CollaborationPanel::onJoinSessionClicked()
{
    if (!m_manager) return;

    QString userName = m_userNameInput->text().trimmed();
    if (userName.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter your name."));
        return;
    }

    QString url = m_sessionUrlInput->text().trimmed();
    if (url.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter the session URL."));
        return;
    }

    if (!m_manager->joinSession(url, userName)) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to join session."));
    }
}

// 函数说明：响应 CollaborationPanel 收到的信号或异步回调，并更新界面状态。
void CollaborationPanel::onDisconnectClicked()
{
    if (!m_manager) return;

    if (m_manager->isHost()) {
        int result = QMessageBox::question(this, tr("Stop Hosting"),
            tr("Are you sure you want to stop hosting? All users will be disconnected."),
            QMessageBox::Yes | QMessageBox::No);

        if (result == QMessageBox::Yes) {
            m_manager->stopHosting();
        }
    } else {
        m_manager->leaveSession();
    }
}

// 函数说明：响应 CollaborationPanel 收到的信号或异步回调，并更新界面状态。
void CollaborationPanel::onCopyLinkClicked()
{
    if (!m_manager) return;

    QString url = m_manager->sessionUrl();
    if (!url.isEmpty()) {
        QApplication::clipboard()->setText(url);
        QMessageBox::information(this, tr("Link Copied"),
            tr("Session link has been copied to clipboard."));
    }
}

// 函数说明：响应 CollaborationPanel 收到的信号或异步回调，并更新界面状态。
void CollaborationPanel::onModeChanged(CollaborationManager::Mode mode)
{
    switch (mode) {
    case CollaborationManager::Mode::Offline:
        showPage(0);
        m_chatWidget->setEnabled(false);
        break;
    case CollaborationManager::Mode::Hosting:
    case CollaborationManager::Mode::Joined:
        showPage(1);
        m_chatWidget->setEnabled(true);
        break;
    }
}

// 函数说明：响应 CollaborationPanel 收到的信号或异步回调，并更新界面状态。
void CollaborationPanel::onSessionStarted(const QString &sessionId, const QString &url)
{
    Q_UNUSED(url);
    m_sessionIdLabel->setText(sessionId);
    m_statusLabel->setText(tr("Connected"));
    m_statusLabel->setStyleSheet(
        "QLabel {"
        "  padding: 4px 8px;"
        "  background-color: #28a745;"
        "  color: white;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
    );

    m_copyLinkButton->setVisible(m_manager && m_manager->isHost());

    if (m_manager) {
        m_chatWidget->setCurrentUserId(m_manager->userId());
        m_chatWidget->setMessages(m_manager->chatHistory());
    }

    updateUserList();
    emit sessionStarted(sessionId);
}

// 函数说明：响应 CollaborationPanel 收到的信号或异步回调，并更新界面状态。
void CollaborationPanel::onSessionEnded()
{
    m_sessionIdLabel->setText(tr("N/A"));
    m_statusLabel->setText(tr("Disconnected"));
    m_statusLabel->setStyleSheet(
        "QLabel {"
        "  padding: 4px 8px;"
        "  background-color: #dc3545;"
        "  color: white;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
    );

    m_userList->clear();
    m_chatWidget->clear();

    emit sessionEnded();
}

// 函数说明：响应 CollaborationPanel 收到的信号或异步回调，并更新界面状态。
void CollaborationPanel::onUserListChanged()
{
    updateUserList();
}

// 函数说明：响应 CollaborationPanel 收到的信号或异步回调，并更新界面状态。
void CollaborationPanel::onChatMessageReceived(const ChatMessage &message)
{
    m_chatWidget->addMessage(message);
}

// 函数说明：响应 CollaborationPanel 收到的信号或异步回调，并更新界面状态。
void CollaborationPanel::onConnectionStateChanged(CollaborationClient::ConnectionState state)
{
    switch (state) {
    case CollaborationClient::ConnectionState::Disconnected:
        m_statusLabel->setText(tr("Disconnected"));
        m_statusLabel->setStyleSheet(
            "QLabel { padding: 4px 8px; background-color: #dc3545; color: white; border-radius: 4px; font-weight: bold; }"
        );
        break;
    case CollaborationClient::ConnectionState::Connecting:
        m_statusLabel->setText(tr("Connecting..."));
        m_statusLabel->setStyleSheet(
            "QLabel { padding: 4px 8px; background-color: #ffc107; color: black; border-radius: 4px; font-weight: bold; }"
        );
        break;
    case CollaborationClient::ConnectionState::Connected:
        m_statusLabel->setText(tr("Connected"));
        m_statusLabel->setStyleSheet(
            "QLabel { padding: 4px 8px; background-color: #28a745; color: white; border-radius: 4px; font-weight: bold; }"
        );
        break;
    case CollaborationClient::ConnectionState::Reconnecting:
        m_statusLabel->setText(tr("Reconnecting..."));
        m_statusLabel->setStyleSheet(
            "QLabel { padding: 4px 8px; background-color: #fd7e14; color: white; border-radius: 4px; font-weight: bold; }"
        );
        break;
    }
}

// 函数说明：响应 CollaborationPanel 收到的信号或异步回调，并更新界面状态。
void CollaborationPanel::onError(const QString &message)
{
    QMessageBox::warning(this, tr("Collaboration Error"), message);
}

// 函数说明：响应 CollaborationPanel 收到的信号或异步回调，并更新界面状态。
void CollaborationPanel::onUserItemContextMenu(const QPoint &pos)
{
    if (!m_manager || !m_manager->isHost()) return;

    QListWidgetItem *item = m_userList->itemAt(pos);
    if (!item) return;

    UserListItem *userItem = dynamic_cast<UserListItem*>(item);
    if (!userItem) return;

    // Don't show menu for self
    if (userItem->oderId() == m_manager->userId()) return;

    m_userContextMenu->popup(m_userList->mapToGlobal(pos));
}

// 函数说明：响应 CollaborationPanel 收到的信号或异步回调，并更新界面状态。
void CollaborationPanel::onUserPermissionAction()
{
    if (!m_manager) return;

    QAction *action = qobject_cast<QAction*>(sender());
    if (!action) return;

    QListWidgetItem *item = m_userList->currentItem();
    if (!item) return;

    UserListItem *userItem = dynamic_cast<UserListItem*>(item);
    if (!userItem) return;

    Permission newPerm = Permission::Edit;
    if (action == m_setReadOnlyAction) {
        newPerm = Permission::ReadOnly;
    } else if (action == m_setCommentAction) {
        newPerm = Permission::Comment;
    } else if (action == m_setEditAction) {
        newPerm = Permission::Edit;
    }

    m_manager->setUserPermission(userItem->oderId(), newPerm);
}

// 函数说明：响应 CollaborationPanel 收到的信号或异步回调，并更新界面状态。
void CollaborationPanel::onKickUserAction()
{
    if (!m_manager) return;

    QListWidgetItem *item = m_userList->currentItem();
    if (!item) return;

    UserListItem *userItem = dynamic_cast<UserListItem*>(item);
    if (!userItem) return;

    int result = QMessageBox::question(this, tr("Kick User"),
        tr("Are you sure you want to remove this user from the session?"),
        QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
        m_manager->kickUser(userItem->oderId());
    }
}

// 函数说明：刷新 CollaborationPanel 的内部状态，并同步到相关界面。
void CollaborationPanel::updateUserList()
{
    m_userList->clear();

    if (!m_manager) return;

    QList<User> users = m_manager->users();
    QString currentUserId = m_manager->userId();

    for (const User &user : users) {
        bool isCurrentUser = (user.id == currentUserId);
        UserListItem *item = new UserListItem(user, isCurrentUser, m_userList);
        m_userList->addItem(item);
    }
}

// 函数说明：显示 CollaborationPanel 管理的面板、对话框或提示信息。
void CollaborationPanel::showPage(int index)
{
    m_stackedWidget->setCurrentIndex(index);
}

// 函数说明：实现 CollaborationPanel::permissionToString 的核心逻辑，供当前模块调用。
QString CollaborationPanel::permissionToString(Permission perm) const
{
    switch (perm) {
    case Permission::None: return tr("None");
    case Permission::ReadOnly: return tr("Read-Only");
    case Permission::Comment: return tr("Comment");
    case Permission::Edit: return tr("Edit");
    case Permission::Owner: return tr("Owner");
    default: return tr("Unknown");
    }
}

// UserListItem implementation

UserListItem::UserListItem(const User &user, bool isCurrentUser, QListWidget *parent)
    : QListWidgetItem(parent)
    , m_userId(user.id)
    , m_permission(user.permission)
{
    QString displayText = user.name;

    if (user.isHost) {
        displayText += QString(" (%1)").arg(QObject::tr("Host"));
    }

    if (isCurrentUser) {
        displayText += QString(" (%1)").arg(QObject::tr("You"));
    }

    QString permText;
    switch (user.permission) {
    case Permission::Owner: permText = QObject::tr("Owner"); break;
    case Permission::Edit: permText = QObject::tr("Edit"); break;
    case Permission::Comment: permText = QObject::tr("Comment"); break;
    case Permission::ReadOnly: permText = QObject::tr("Read-Only"); break;
    default: permText = QObject::tr("None"); break;
    }
    displayText += QString(" - %1").arg(permText);

    setText(displayText);

    // Set color indicator
    QPixmap colorIcon(16, 16);
    colorIcon.fill(user.color);
    setIcon(QIcon(colorIcon));

    // Highlight current user
    if (isCurrentUser) {
        setBackground(QColor("#e3f2fd"));
    }
}

} // namespace Collaboration

