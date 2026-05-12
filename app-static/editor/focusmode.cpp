// 文件说明：app-static\editor\focusmode.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "focusmode.h"

#include <QTextBlock>
#include <QTextCursor>
#include <QScrollBar>
#include <QApplication>
#include <QScreen>
#include <QDebug>

// 函数说明：构造 FocusMode 对象，初始化本模块需要的状态、界面和资源。
FocusMode::FocusMode(QPlainTextEdit *editor, QObject *parent)
    : QObject(parent)
    , m_editor(editor)
    , m_currentFadeOpacity(0.3)
    , m_fadeAnimation(nullptr)
    , m_updateTimer(new QTimer(this))
    , m_focusStart(0)
    , m_focusEnd(0)
    , m_stateSaved(false)
{
    m_fadeAnimation = new QPropertyAnimation(this, "fadeOpacity", this);
    m_fadeAnimation->setDuration(m_config.transitionDuration);
    m_fadeAnimation->setEasingCurve(QEasingCurve::OutQuad);

    m_updateTimer->setSingleShot(true);
    m_updateTimer->setInterval(50);
    connect(m_updateTimer, &QTimer::timeout, this, &FocusMode::updateFocus);

    connect(m_editor, &QPlainTextEdit::cursorPositionChanged,
            this, &FocusMode::onCursorPositionChanged);
    connect(m_editor, &QPlainTextEdit::textChanged,
            this, &FocusMode::onTextChanged);
}

// 函数说明：销毁 FocusMode 对象，释放本模块持有的资源。
FocusMode::~FocusMode()
{
    if (m_config.enabled) {
        disable();
    }
}

// 函数说明：设置 FocusMode 的运行参数，并触发必要的界面或数据刷新。
void FocusMode::setConfig(const Config &config)
{
    bool wasEnabled = m_config.enabled;
    m_config = config;

    if (wasEnabled && !config.enabled) {
        disable();
    } else if (!wasEnabled && config.enabled) {
        enable();
    } else if (config.enabled) {
        applyFocusStyle();
        updateFocus();
    }

    emit configChanged();
}

// 函数说明：实现 FocusMode::enable 的核心逻辑，供当前模块调用。
void FocusMode::enable()
{
    if (m_config.enabled) return;

    saveEditorState();
    m_config.enabled = true;
    applyFocusStyle();
    updateFocus();

    emit enabled();
}

// 函数说明：实现 FocusMode::disable 的核心逻辑，供当前模块调用。
void FocusMode::disable()
{
    if (!m_config.enabled) return;

    m_config.enabled = false;
    removeFocusStyle();
    restoreEditorState();

    emit disabled();
}

// 函数说明：切换 FocusMode 对应功能的启用状态。
void FocusMode::toggle()
{
    if (m_config.enabled) {
        disable();
    } else {
        enable();
    }
}

// 函数说明：设置 FocusMode 的运行参数，并触发必要的界面或数据刷新。
void FocusMode::setFocusScope(FocusScope scope)
{
    if (m_config.scope != scope) {
        m_config.scope = scope;
        if (m_config.enabled) {
            updateFocus();
        }
    }
}

// 函数说明：设置 FocusMode 的运行参数，并触发必要的界面或数据刷新。
void FocusMode::setScrollMode(ScrollMode mode)
{
    m_config.scrollMode = mode;
}

// 函数说明：设置 FocusMode 的运行参数，并触发必要的界面或数据刷新。
void FocusMode::setFocusColor(const QColor &color)
{
    m_config.focusColor = color;
    if (m_config.enabled) {
        applyFocusStyle();
    }
}

// 函数说明：设置 FocusMode 的运行参数，并触发必要的界面或数据刷新。
void FocusMode::setFadeColor(const QColor &color)
{
    m_config.fadeColor = color;
    if (m_config.enabled) {
        updateExtraSelections();
    }
}

// 函数说明：设置 FocusMode 的运行参数，并触发必要的界面或数据刷新。
void FocusMode::setBackgroundColor(const QColor &color)
{
    m_config.backgroundColor = color;
    if (m_config.enabled) {
        applyFocusStyle();
    }
}

// 函数说明：设置 FocusMode 的运行参数，并触发必要的界面或数据刷新。
void FocusMode::setFadeOpacity(qreal opacity)
{
    m_currentFadeOpacity = opacity;
    if (m_config.enabled) {
        updateExtraSelections();
    }
}

// 函数说明：应用 FocusMode 当前配置，让编辑器或预览立即生效。
void FocusMode::applyLightTheme()
{
    m_config.focusColor = QColor(30, 30, 30);
    m_config.fadeColor = QColor(180, 180, 180);
    m_config.backgroundColor = QColor(255, 255, 255);
    m_config.lineHighlightColor = QColor(245, 245, 220, 100);

    if (m_config.enabled) {
        applyFocusStyle();
        updateFocus();
    }
}

// 函数说明：应用 FocusMode 当前配置，让编辑器或预览立即生效。
void FocusMode::applyDarkTheme()
{
    m_config.focusColor = QColor(220, 220, 220);
    m_config.fadeColor = QColor(100, 100, 100);
    m_config.backgroundColor = QColor(40, 44, 52);
    m_config.lineHighlightColor = QColor(60, 64, 72, 100);

    if (m_config.enabled) {
        applyFocusStyle();
        updateFocus();
    }
}

// 函数说明：应用 FocusMode 当前配置，让编辑器或预览立即生效。
void FocusMode::applySepiaTheme()
{
    m_config.focusColor = QColor(70, 50, 30);
    m_config.fadeColor = QColor(160, 140, 120);
    m_config.backgroundColor = QColor(249, 241, 228);
    m_config.lineHighlightColor = QColor(240, 230, 210, 100);

    if (m_config.enabled) {
        applyFocusStyle();
        updateFocus();
    }
}

// 函数说明：读取 FocusMode 当前保存的状态或计算结果。
QPair<int, int> FocusMode::getFocusRange() const
{
    return qMakePair(m_focusStart, m_focusEnd);
}

// 函数说明：刷新 FocusMode 的内部状态，并同步到相关界面。
void FocusMode::updateFocus()
{
    if (!m_config.enabled || !m_editor) return;

    QPair<int, int> range;

    switch (m_config.scope) {
        case FocusScope::Line:
            range = calculateLineRange();
            break;
        case FocusScope::Sentence:
            range = calculateSentenceRange();
            break;
        case FocusScope::Paragraph:
            range = calculateParagraphRange();
            break;
        case FocusScope::Section:
            range = calculateSectionRange();
            break;
    }

    if (range.first != m_focusStart || range.second != m_focusEnd) {
        m_focusStart = range.first;
        m_focusEnd = range.second;
        updateExtraSelections();
        emit focusRangeChanged(m_focusStart, m_focusEnd);
    }

    if (m_config.scrollMode == ScrollMode::Typewriter) {
        ensureCursorCentered();
    }
}

// 函数说明：实现 FocusMode::scrollToCenter 的核心逻辑，供当前模块调用。
void FocusMode::scrollToCenter()
{
    ensureCursorCentered();
}

// 函数说明：响应 FocusMode 收到的信号或异步回调，并更新界面状态。
void FocusMode::onCursorPositionChanged()
{
    if (m_config.enabled) {
        m_updateTimer->start();
    }
}

// 函数说明：响应 FocusMode 收到的信号或异步回调，并更新界面状态。
void FocusMode::onTextChanged()
{
    if (m_config.enabled) {
        m_updateTimer->start();
    }
}

// 函数说明：响应 FocusMode 收到的信号或异步回调，并更新界面状态。
void FocusMode::onScrollValueChanged()
{
    // 可用于实现智能滚动
}

// 函数说明：实现 FocusMode::calculateLineRange 的核心逻辑，供当前模块调用。
QPair<int, int> FocusMode::calculateLineRange()
{
    QTextCursor cursor = m_editor->textCursor();
    QTextBlock block = cursor.block();

    return qMakePair(block.position(), block.position() + block.length() - 1);
}

// 函数说明：实现 FocusMode::calculateSentenceRange 的核心逻辑，供当前模块调用。
QPair<int, int> FocusMode::calculateSentenceRange()
{
    QTextCursor cursor = m_editor->textCursor();
    QString text = m_editor->toPlainText();
    int pos = cursor.position();

    // 向前查找句子开始
    int start = pos;
    while (start > 0) {
        QChar ch = text.at(start - 1);
        if (ch == '.' || ch == '!' || ch == '?' ||
            ch == QChar(0x3002) ||  // 。
            ch == QChar(0xFF01) ||  // ！
            ch == QChar(0xFF1F)) {  // ？
            break;
        }
        start--;
    }

    // 跳过空白
    while (start < text.length() && text.at(start).isSpace()) {
        start++;
    }

    // 向后查找句子结束
    int end = pos;
    while (end < text.length()) {
        QChar ch = text.at(end);
        if (ch == '.' || ch == '!' || ch == '?' ||
            ch == QChar(0x3002) || ch == QChar(0xFF01) || ch == QChar(0xFF1F)) {
            end++;
            break;
        }
        end++;
    }

    return qMakePair(start, end);
}

// 函数说明：实现 FocusMode::calculateParagraphRange 的核心逻辑，供当前模块调用。
QPair<int, int> FocusMode::calculateParagraphRange()
{
    QTextCursor cursor = m_editor->textCursor();
    QTextBlock block = cursor.block();

    // 查找段落开始（向上找到空行或文档开始）
    QTextBlock startBlock = block;
    while (startBlock.isValid() && startBlock.previous().isValid()) {
        QTextBlock prevBlock = startBlock.previous();
        if (prevBlock.text().trimmed().isEmpty()) {
            break;
        }
        startBlock = prevBlock;
    }

    // 查找段落结束（向下找到空行或文档结束）
    QTextBlock endBlock = block;
    while (endBlock.isValid() && endBlock.next().isValid()) {
        if (endBlock.text().trimmed().isEmpty()) {
            break;
        }
        endBlock = endBlock.next();
    }

    // 如果最后一个块是空行，使用前一个块
    if (endBlock.text().trimmed().isEmpty() && endBlock.previous().isValid()) {
        endBlock = endBlock.previous();
    }

    int start = startBlock.position();
    int end = endBlock.position() + endBlock.length() - 1;

    return qMakePair(start, end);
}

// 函数说明：实现 FocusMode::calculateSectionRange 的核心逻辑，供当前模块调用。
QPair<int, int> FocusMode::calculateSectionRange()
{
    QTextCursor cursor = m_editor->textCursor();
    QTextBlock block = cursor.block();
    QString text = m_editor->toPlainText();

    // 查找章节开始（向上找到标题行或文档开始）
    QTextBlock startBlock = block;
    while (startBlock.isValid() && startBlock.previous().isValid()) {
        QTextBlock prevBlock = startBlock.previous();
        QString lineText = prevBlock.text();
        if (lineText.startsWith('#')) {
            startBlock = prevBlock;
            break;
        }
        startBlock = prevBlock;
    }

    // 查找章节结束（向下找到下一个标题行或文档结束）
    QTextBlock endBlock = block;
    while (endBlock.next().isValid()) {
        endBlock = endBlock.next();
        QString lineText = endBlock.text();
        if (lineText.startsWith('#')) {
            endBlock = endBlock.previous();
            break;
        }
    }

    int start = startBlock.position();
    int end = endBlock.position() + endBlock.length() - 1;

    return qMakePair(start, end);
}

// 函数说明：应用 FocusMode 当前配置，让编辑器或预览立即生效。
void FocusMode::applyFocusStyle()
{
    if (!m_editor) return;

    // 设置背景色
    QPalette palette = m_editor->palette();
    palette.setColor(QPalette::Base, m_config.backgroundColor);
    palette.setColor(QPalette::Text, m_config.focusColor);
    m_editor->setPalette(palette);

    // 设置边距
    if (m_config.marginPercent > 0) {
        int width = m_editor->viewport()->width();
        int margin = width * m_config.marginPercent / 100;
        m_editor->document()->setDocumentMargin(margin);
    }

    // 应用样式表
    QString styleSheet = QString(
        "QPlainTextEdit {"
        "    background-color: %1;"
        "    color: %2;"
        "    selection-background-color: %3;"
        "    border: none;"
        "}"
    ).arg(m_config.backgroundColor.name())
     .arg(m_config.focusColor.name())
     .arg(m_config.focusColor.lighter(170).name());

    m_editor->setStyleSheet(styleSheet);
}

// 函数说明：从 FocusMode 管理的数据集合中移除指定内容。
void FocusMode::removeFocusStyle()
{
    if (!m_editor) return;

    // 清除额外选择
    m_editor->setExtraSelections(QList<QTextEdit::ExtraSelection>());

    // 清除样式表
    m_editor->setStyleSheet(QString());

    // 清除边距
    m_editor->document()->setDocumentMargin(0);
}

// 函数说明：刷新 FocusMode 的内部状态，并同步到相关界面。
void FocusMode::updateExtraSelections()
{
    if (!m_editor || !m_config.enabled) return;

    QList<QTextEdit::ExtraSelection> selections;
    QString text = m_editor->toPlainText();

    // 创建淡化区域的选择
    // 焦点区域之前
    if (m_focusStart > 0) {
        QTextEdit::ExtraSelection selection;
        QTextCharFormat format;
        QColor fadeColor = m_config.fadeColor;
        fadeColor.setAlphaF(m_currentFadeOpacity);
        format.setForeground(fadeColor);
        selection.format = format;

        QTextCursor cursor(m_editor->document());
        cursor.setPosition(0);
        cursor.setPosition(m_focusStart, QTextCursor::KeepAnchor);
        selection.cursor = cursor;
        selections.append(selection);
    }

    // 焦点区域之后
    if (m_focusEnd < text.length()) {
        QTextEdit::ExtraSelection selection;
        QTextCharFormat format;
        QColor fadeColor = m_config.fadeColor;
        fadeColor.setAlphaF(m_currentFadeOpacity);
        format.setForeground(fadeColor);
        selection.format = format;

        QTextCursor cursor(m_editor->document());
        cursor.setPosition(m_focusEnd);
        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        selection.cursor = cursor;
        selections.append(selection);
    }

    // 当前行高亮
    if (m_config.highlightCurrentLine) {
        QTextEdit::ExtraSelection lineSelection;
        lineSelection.format.setBackground(m_config.lineHighlightColor);
        lineSelection.format.setProperty(QTextFormat::FullWidthSelection, true);
        lineSelection.cursor = m_editor->textCursor();
        lineSelection.cursor.clearSelection();
        selections.append(lineSelection);
    }

    m_editor->setExtraSelections(selections);
}

// 函数说明：实现 FocusMode::ensureCursorCentered 的核心逻辑，供当前模块调用。
void FocusMode::ensureCursorCentered()
{
    if (!m_editor || m_config.scrollMode != ScrollMode::Typewriter) return;

    QTextCursor cursor = m_editor->textCursor();
    QRect cursorRect = m_editor->cursorRect(cursor);
    QRect viewportRect = m_editor->viewport()->rect();

    int targetY = viewportRect.height() / 2;
    int currentY = cursorRect.top();
    int diff = currentY - targetY;

    if (qAbs(diff) > 10) {  // 只在差异较大时滚动
        QScrollBar *scrollBar = m_editor->verticalScrollBar();
        scrollBar->setValue(scrollBar->value() + diff);
    }
}

// 函数说明：保存 FocusMode 当前状态，保证用户修改可以持久化。
void FocusMode::saveEditorState()
{
    if (!m_editor || m_stateSaved) return;

    m_savedState.palette = m_editor->palette();
    m_savedState.styleSheet = m_editor->styleSheet();
    m_savedState.wordWrapMode = m_editor->wordWrapMode() != QTextOption::NoWrap;
    m_savedState.documentMargin = m_editor->document()->documentMargin();
    m_stateSaved = true;
}

// 函数说明：实现 FocusMode::restoreEditorState 的核心逻辑，供当前模块调用。
void FocusMode::restoreEditorState()
{
    if (!m_editor || !m_stateSaved) return;

    m_editor->setPalette(m_savedState.palette);
    m_editor->setStyleSheet(m_savedState.styleSheet);
    m_editor->document()->setDocumentMargin(m_savedState.documentMargin);
    m_stateSaved = false;
}

