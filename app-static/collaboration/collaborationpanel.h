// 文件说明：app-static\collaboration\collaborationpanel.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef COLLABORATIONPANEL_H
#define COLLABORATIONPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QComboBox>
#include <QGroupBox>
#include <QClipboard>
#include <QMenu>

#include "collaborationmanager.h"
#include "chatwidget.h"

namespace Collaboration {

/**
 * @brief Main collaboration panel widget
 *
 * Provides UI for:
 * - Starting/joining collaboration sessions
 * - User list with permission management
 * - Real-time chat
 * - Session status display
 */
class CollaborationPanel : public QWidget
{
    Q_OBJECT

public:
    explicit CollaborationPanel(QWidget *parent = nullptr);
    ~CollaborationPanel() = default;

    void setCollaborationManager(CollaborationManager *manager);
    CollaborationManager* collaborationManager() const { return m_manager; }

signals:
    void sessionStarted(const QString &sessionId);
    void sessionEnded();
    void visibilityRequested(bool visible);

private slots:
    void onStartHostingClicked();
    void onJoinSessionClicked();
    void onDisconnectClicked();
    void onCopyLinkClicked();

    void onModeChanged(CollaborationManager::Mode mode);
    void onSessionStarted(const QString &sessionId, const QString &url);
    void onSessionEnded();
    void onUserListChanged();
    void onChatMessageReceived(const ChatMessage &message);
    void onConnectionStateChanged(CollaborationClient::ConnectionState state);
    void onError(const QString &message);

    void onUserItemContextMenu(const QPoint &pos);
    void onUserPermissionAction();
    void onKickUserAction();

private:
    void setupUi();
    void setupStartPage();
    void setupSessionPage();
    void updateUserList();
    void showPage(int index);
    QString permissionToString(Permission perm) const;

    CollaborationManager *m_manager;

    // Main layout
    QVBoxLayout *m_mainLayout;
    QStackedWidget *m_stackedWidget;

    // Start page (index 0)
    QWidget *m_startPage;
    QLineEdit *m_userNameInput;
    QLineEdit *m_sessionUrlInput;
    QPushButton *m_startHostingButton;
    QPushButton *m_joinSessionButton;

    // Session page (index 1)
    QWidget *m_sessionPage;
    QLabel *m_statusLabel;
    QLabel *m_sessionIdLabel;
    QPushButton *m_copyLinkButton;
    QPushButton *m_disconnectButton;

    // User list
    QGroupBox *m_userListGroup;
    QListWidget *m_userList;

    // Chat widget
    ChatWidget *m_chatWidget;

    // Context menu for users
    QMenu *m_userContextMenu;
    QAction *m_setReadOnlyAction;
    QAction *m_setCommentAction;
    QAction *m_setEditAction;
    QAction *m_kickUserAction;
};

/**
 * @brief Custom list item for user display
 */
class UserListItem : public QListWidgetItem
{
public:
    UserListItem(const User &user, bool isCurrentUser, QListWidget *parent = nullptr);

    QString oderId() const { return m_userId; }
    Permission permission() const { return m_permission; }

private:
    QString m_userId;
    Permission m_permission;
};

} // namespace Collaboration

#endif // COLLABORATIONPANEL_H

