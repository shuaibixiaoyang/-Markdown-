// 文件说明：app-static\collaboration\chatwidget.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

#include "collaborationserver.h"

namespace Collaboration {

/**
 * @brief Chat widget for real-time collaboration chat
 *
 * Provides a simple chat interface with message history,
 * input field, and send button.
 */
class ChatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWidget(QWidget *parent = nullptr);
    ~ChatWidget() = default;

    void addMessage(const ChatMessage &message);
    void setMessages(const QList<ChatMessage> &messages);
    void clear();

    void setCurrentUserId(const QString &userId) { m_currentUserId = userId; }
    void setEnabled(bool enabled);

signals:
    void messageSent(const QString &content);

private slots:
    void onSendClicked();
    void onInputReturnPressed();

private:
    void setupUi();
    void scrollToBottom();
    QString formatTime(qint64 timestamp) const;

    QVBoxLayout *m_mainLayout;
    QListWidget *m_messageList;
    QLineEdit *m_inputField;
    QPushButton *m_sendButton;
    QLabel *m_headerLabel;

    QString m_currentUserId;
};

/**
 * @brief Custom list widget item for chat messages
 */
class ChatMessageItem : public QListWidgetItem
{
public:
    ChatMessageItem(const ChatMessage &message, bool isOwnMessage, QListWidget *parent = nullptr);

    QString senderId() const { return m_message.senderId; }
    qint64 timestamp() const { return m_message.timestamp; }

private:
    ChatMessage m_message;
};

} // namespace Collaboration

#endif // CHATWIDGET_H

