// 文件说明：app-static\collaboration\chatwidget.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "chatwidget.h"
#include <QDateTime>
#include <QScrollBar>
#include <QHBoxLayout>
#include <QFont>

namespace Collaboration {

// 函数说明：构造 ChatWidget 对象，初始化本模块需要的状态、界面和资源。
ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(new QVBoxLayout(this))
    , m_messageList(new QListWidget(this))
    , m_inputField(new QLineEdit(this))
    , m_sendButton(new QPushButton(tr("Send"), this))
    , m_headerLabel(new QLabel(tr("Chat"), this))
{
    setupUi();
}

// 函数说明：初始化 ChatWidget 的 setupUi 相关界面、动作或服务连接。
void ChatWidget::setupUi()
{
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(8);

    // Header
    QFont headerFont = m_headerLabel->font();
    headerFont.setBold(true);
    m_headerLabel->setFont(headerFont);
    m_mainLayout->addWidget(m_headerLabel);

    // Message list
    m_messageList->setWordWrap(true);
    m_messageList->setSelectionMode(QAbstractItemView::NoSelection);
    m_messageList->setStyleSheet(
        "QListWidget {"
        "  background-color: #f8f9fa;"
        "  border: 1px solid #dee2e6;"
        "  border-radius: 4px;"
        "}"
        "QListWidget::item {"
        "  padding: 8px;"
        "  border-bottom: 1px solid #e9ecef;"
        "}"
        "QListWidget::item:last {"
        "  border-bottom: none;"
        "}"
    );
    m_mainLayout->addWidget(m_messageList, 1);

    // Input area
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->setSpacing(8);

    m_inputField->setPlaceholderText(tr("Type a message..."));
    m_inputField->setStyleSheet(
        "QLineEdit {"
        "  padding: 8px;"
        "  border: 1px solid #ced4da;"
        "  border-radius: 4px;"
        "}"
        "QLineEdit:focus {"
        "  border-color: #80bdff;"
        "}"
    );
    inputLayout->addWidget(m_inputField, 1);

    m_sendButton->setStyleSheet(
        "QPushButton {"
        "  padding: 8px 16px;"
        "  background-color: #007bff;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #0056b3;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #004085;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #6c757d;"
        "}"
    );
    inputLayout->addWidget(m_sendButton);

    m_mainLayout->addLayout(inputLayout);

    // Connections
    connect(m_sendButton, &QPushButton::clicked,
            this, &ChatWidget::onSendClicked);
    connect(m_inputField, &QLineEdit::returnPressed,
            this, &ChatWidget::onInputReturnPressed);
}

// 函数说明：向 ChatWidget 管理的数据集合中添加一项内容。
void ChatWidget::addMessage(const ChatMessage &message)
{
    bool isOwnMessage = (message.senderId == m_currentUserId);
    ChatMessageItem *item = new ChatMessageItem(message, isOwnMessage, m_messageList);

    m_messageList->addItem(item);
    scrollToBottom();
}

// 函数说明：设置 ChatWidget 的运行参数，并触发必要的界面或数据刷新。
void ChatWidget::setMessages(const QList<ChatMessage> &messages)
{
    m_messageList->clear();

    for (const ChatMessage &message : messages) {
        bool isOwnMessage = (message.senderId == m_currentUserId);
        ChatMessageItem *item = new ChatMessageItem(message, isOwnMessage, m_messageList);
        m_messageList->addItem(item);
    }

    scrollToBottom();
}

// 函数说明：清空 ChatWidget 保存的临时状态或缓存数据。
void ChatWidget::clear()
{
    m_messageList->clear();
    m_inputField->clear();
}

// 函数说明：设置 ChatWidget 的运行参数，并触发必要的界面或数据刷新。
void ChatWidget::setEnabled(bool enabled)
{
    m_inputField->setEnabled(enabled);
    m_sendButton->setEnabled(enabled);

    if (!enabled) {
        m_inputField->setPlaceholderText(tr("Join a session to chat..."));
    } else {
        m_inputField->setPlaceholderText(tr("Type a message..."));
    }
}

// 函数说明：响应 ChatWidget 收到的信号或异步回调，并更新界面状态。
void ChatWidget::onSendClicked()
{
    QString content = m_inputField->text().trimmed();
    if (content.isEmpty()) return;

    emit messageSent(content);
    m_inputField->clear();
    m_inputField->setFocus();
}

// 函数说明：响应 ChatWidget 收到的信号或异步回调，并更新界面状态。
void ChatWidget::onInputReturnPressed()
{
    onSendClicked();
}

// 函数说明：实现 ChatWidget::scrollToBottom 的核心逻辑，供当前模块调用。
void ChatWidget::scrollToBottom()
{
    QScrollBar *scrollBar = m_messageList->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

// 函数说明：实现 ChatWidget::formatTime 的核心逻辑，供当前模块调用。
QString ChatWidget::formatTime(qint64 timestamp) const
{
    QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(timestamp);
    QDateTime now = QDateTime::currentDateTime();

    if (dateTime.date() == now.date()) {
        return dateTime.toString("HH:mm");
    } else {
        return dateTime.toString("MM/dd HH:mm");
    }
}

// ChatMessageItem implementation

ChatMessageItem::ChatMessageItem(const ChatMessage &message, bool isOwnMessage, QListWidget *parent)
    : QListWidgetItem(parent)
    , m_message(message)
{
    QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(message.timestamp);
    QString timeStr = dateTime.toString("HH:mm");

    QString displayText;
    if (isOwnMessage) {
        displayText = QString("[%1] %2: %3")
            .arg(timeStr)
            .arg(QObject::tr("You"))
            .arg(message.content);
        setForeground(QColor("#0d6efd"));
    } else {
        displayText = QString("[%1] %2: %3")
            .arg(timeStr)
            .arg(message.senderName)
            .arg(message.content);
        setForeground(QColor("#212529"));
    }

    setText(displayText);

    // Set tooltip with full timestamp
    setToolTip(dateTime.toString("yyyy-MM-dd HH:mm:ss"));
}

} // namespace Collaboration

