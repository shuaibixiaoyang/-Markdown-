// 文件说明：app-static\collaboration\remotecursoroverlay.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "remotecursoroverlay.h"
#include <QPainter>
#include <QScrollBar>
#include <QTextBlock>
#include <QDateTime>
#include <QFontMetrics>

namespace Collaboration {

// 函数说明：构造 RemoteCursorOverlay 对象，初始化本模块需要的状态、界面和资源。
RemoteCursorOverlay::RemoteCursorOverlay(QPlainTextEdit *editor, QWidget *parent)
    : QWidget(parent ? parent : editor)
    , m_editor(editor)
    , m_blinkTimer(new QTimer(this))
    , m_showCursors(true)
    , m_showSelections(true)
    , m_showLabels(true)
    , m_cursorVisible(true)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);

    // Position overlay over the editor viewport
    if (m_editor) {
        setParent(m_editor->viewport());
        setGeometry(m_editor->viewport()->rect());
        m_editor->viewport()->installEventFilter(this);

        // Update when editor scrolls or resizes
        connect(m_editor->verticalScrollBar(), &QScrollBar::valueChanged,
                this, &RemoteCursorOverlay::updateOverlay);
        connect(m_editor->horizontalScrollBar(), &QScrollBar::valueChanged,
                this, &RemoteCursorOverlay::updateOverlay);
        connect(m_editor->document(), &QTextDocument::contentsChanged,
                this, &RemoteCursorOverlay::updateOverlay);
    }

    // Cursor blink animation
    connect(m_blinkTimer, &QTimer::timeout, this, [this]() {
        m_cursorVisible = !m_cursorVisible;
        update();
    });
    m_blinkTimer->start(530);

    show();
}

// 函数说明：刷新 RemoteCursorOverlay 的内部状态，并同步到相关界面。
void RemoteCursorOverlay::updateCursor(const QString &userId, const QString &userName,
                                        int position, const QColor &color)
{
    RemoteCursorInfo &cursor = m_cursors[userId];
    cursor.userId = userId;
    cursor.userName = userName;
    cursor.position = position;
    cursor.color = color;
    cursor.visible = true;
    cursor.lastUpdate = QDateTime::currentMSecsSinceEpoch();

    update();
}

// 函数说明：刷新 RemoteCursorOverlay 的内部状态，并同步到相关界面。
void RemoteCursorOverlay::updateSelection(const QString &userId, int start, int end)
{
    if (!m_cursors.contains(userId)) return;

    m_cursors[userId].selectionStart = start;
    m_cursors[userId].selectionEnd = end;
    m_cursors[userId].lastUpdate = QDateTime::currentMSecsSinceEpoch();

    update();
}

// 函数说明：从 RemoteCursorOverlay 管理的数据集合中移除指定内容。
void RemoteCursorOverlay::removeCursor(const QString &userId)
{
    m_cursors.remove(userId);
    update();
}

// 函数说明：清空 RemoteCursorOverlay 保存的临时状态或缓存数据。
void RemoteCursorOverlay::clearAllCursors()
{
    m_cursors.clear();
    update();
}

// 函数说明：设置 RemoteCursorOverlay 的运行参数，并触发必要的界面或数据刷新。
void RemoteCursorOverlay::setShowCursors(bool show)
{
    m_showCursors = show;
    update();
}

// 函数说明：设置 RemoteCursorOverlay 的运行参数，并触发必要的界面或数据刷新。
void RemoteCursorOverlay::setShowSelections(bool show)
{
    m_showSelections = show;
    update();
}

// 函数说明：设置 RemoteCursorOverlay 的运行参数，并触发必要的界面或数据刷新。
void RemoteCursorOverlay::setShowLabels(bool show)
{
    m_showLabels = show;
    update();
}

// 函数说明：刷新 RemoteCursorOverlay 的内部状态，并同步到相关界面。
void RemoteCursorOverlay::updateOverlay()
{
    if (m_editor) {
        setGeometry(m_editor->viewport()->rect());
    }
    update();
}

// 函数说明：绘制 RemoteCursorOverlay 的可视区域或辅助标记。
void RemoteCursorOverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    if (!m_editor || m_cursors.isEmpty()) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    qint64 now = QDateTime::currentMSecsSinceEpoch();

    for (auto it = m_cursors.begin(); it != m_cursors.end(); ) {
        // Remove stale cursors
        if (now - it->lastUpdate > CursorTimeout) {
            it = m_cursors.erase(it);
            continue;
        }

        // Draw selection first (behind cursor)
        if (m_showSelections && it->selectionStart != it->selectionEnd) {
            drawSelection(painter, *it);
        }

        // Draw cursor
        if (m_showCursors && m_cursorVisible) {
            drawCursor(painter, *it);
        }

        ++it;
    }
}

// 函数说明：实现 RemoteCursorOverlay::eventFilter 的核心逻辑，供当前模块调用。
bool RemoteCursorOverlay::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_editor->viewport()) {
        if (event->type() == QEvent::Resize) {
            setGeometry(m_editor->viewport()->rect());
        }
    }
    return QWidget::eventFilter(watched, event);
}

// 函数说明：实现 RemoteCursorOverlay::drawCursor 的核心逻辑，供当前模块调用。
void RemoteCursorOverlay::drawCursor(QPainter &painter, const RemoteCursorInfo &cursor)
{
    QRect cursorRect = getCursorRect(cursor.position);
    if (cursorRect.isNull()) return;

    // Draw cursor line
    painter.fillRect(cursorRect, cursor.color);

    // Draw label if enabled
    if (m_showLabels) {
        drawLabel(painter, cursor, cursorRect);
    }
}

// 函数说明：实现 RemoteCursorOverlay::drawSelection 的核心逻辑，供当前模块调用。
void RemoteCursorOverlay::drawSelection(QPainter &painter, const RemoteCursorInfo &cursor)
{
    QVector<QRect> rects = getSelectionRects(cursor.selectionStart, cursor.selectionEnd);

    // Semi-transparent selection color
    QColor selColor = cursor.color;
    selColor.setAlpha(50);

    for (const QRect &rect : rects) {
        painter.fillRect(rect, selColor);
    }
}

// 函数说明：实现 RemoteCursorOverlay::drawLabel 的核心逻辑，供当前模块调用。
void RemoteCursorOverlay::drawLabel(QPainter &painter, const RemoteCursorInfo &cursor, const QRect &cursorRect)
{
    if (cursor.userName.isEmpty()) return;

    QFont labelFont = painter.font();
    labelFont.setPointSize(9);
    labelFont.setBold(true);
    painter.setFont(labelFont);

    QFontMetrics fm(labelFont);
    int textWidth = fm.horizontalAdvance(cursor.userName);
    int labelWidth = textWidth + LabelPadding * 2;

    QRect labelRect(cursorRect.left(), cursorRect.top() - LabelHeight - 2,
                    labelWidth, LabelHeight);

    // Keep label within viewport
    if (labelRect.right() > width()) {
        labelRect.moveRight(width() - 2);
    }
    if (labelRect.top() < 0) {
        labelRect.moveTop(cursorRect.bottom() + 2);
    }

    // Draw label background
    painter.setPen(Qt::NoPen);
    painter.setBrush(cursor.color);
    painter.drawRoundedRect(labelRect, 3, 3);

    // Draw label text
    painter.setPen(Qt::white);
    painter.drawText(labelRect, Qt::AlignCenter, cursor.userName);
}

// 函数说明：读取 RemoteCursorOverlay 当前保存的状态或计算结果。
QRect RemoteCursorOverlay::getCursorRect(int position) const
{
    if (!m_editor) return QRect();

    QTextCursor cursor(m_editor->document());
    cursor.setPosition(position);

    QRect cursorRect = m_editor->cursorRect(cursor);

    // Adjust for viewport
    cursorRect.setWidth(CursorWidth);

    return cursorRect;
}

// 函数说明：读取 RemoteCursorOverlay 当前保存的状态或计算结果。
QVector<QRect> RemoteCursorOverlay::getSelectionRects(int start, int end) const
{
    QVector<QRect> rects;
    if (!m_editor || start == end) return rects;

    if (start > end) std::swap(start, end);

    QTextDocument *doc = m_editor->document();
    QTextCursor startCursor(doc);
    QTextCursor endCursor(doc);
    startCursor.setPosition(start);
    endCursor.setPosition(end);

    int startBlock = startCursor.blockNumber();
    int endBlock = endCursor.blockNumber();

    for (int i = startBlock; i <= endBlock; ++i) {
        QTextBlock block = doc->findBlockByNumber(i);
        if (!block.isValid()) continue;

        QTextCursor blockStart(doc);
        blockStart.setPosition(block.position());
        QTextCursor blockEnd(doc);
        blockEnd.setPosition(block.position() + block.length() - 1);

        int selStart = (i == startBlock) ? start : block.position();
        int selEnd = (i == endBlock) ? end : block.position() + block.length() - 1;

        QTextCursor selStartCursor(doc);
        selStartCursor.setPosition(selStart);
        QTextCursor selEndCursor(doc);
        selEndCursor.setPosition(selEnd);

        QRect startRect = m_editor->cursorRect(selStartCursor);
        QRect endRect = m_editor->cursorRect(selEndCursor);

        QRect lineRect(startRect.left(), startRect.top(),
                       endRect.right() - startRect.left(), startRect.height());

        if (lineRect.width() > 0) {
            rects.append(lineRect);
        }
    }

    return rects;
}

} // namespace Collaboration

