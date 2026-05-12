// 文件说明：app-static\writing\outlinenavigator.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "outlinenavigator.h"

#include <QLabel>
#include <QRegularExpression>
#include <QTextBlock>
#include <QScrollBar>
#include <QHeaderView>
#include <QTimer>
#include <functional>

// 前向声明辅助函数
static QVector<OutlineNavigator::OutlineItem> buildHierarchy(const QVector<OutlineNavigator::OutlineItem> &flatItems);

// 函数说明：构造 OutlineNavigator 对象，初始化本模块需要的状态、界面和资源。
OutlineNavigator::OutlineNavigator(QWidget *parent)
    : QWidget(parent)
    , m_editor(nullptr)
    , m_maxLevel(6)
    , m_autoUpdate(true)
    , m_showLineNumbers(true)
    , m_highlightCurrent(true)
    , m_currentItem(nullptr)
    , m_updateTimer(new QTimer(this))
{
    setupUi();

    m_updateTimer->setSingleShot(true);
    m_updateTimer->setInterval(300);
    connect(m_updateTimer, &QTimer::timeout,
            this, &OutlineNavigator::updateOutline);
}

// 函数说明：销毁 OutlineNavigator 对象，释放本模块持有的资源。
OutlineNavigator::~OutlineNavigator()
{
}

// 函数说明：初始化 OutlineNavigator 的 setupUi 相关界面、动作或服务连接。
void OutlineNavigator::setupUi()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(5, 5, 5, 5);
    m_layout->setSpacing(5);

    // 标题
    QLabel *titleLabel = new QLabel(tr("📑 文档大纲"), this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    m_layout->addWidget(titleLabel);

    // 搜索框
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("搜索标题..."));
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &OutlineNavigator::filterOutline);
    m_layout->addWidget(m_searchEdit);

    // 树形控件
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderHidden(true);
    m_treeWidget->setIndentation(15);
    m_treeWidget->setAnimated(true);
    m_treeWidget->setExpandsOnDoubleClick(false);

    connect(m_treeWidget, &QTreeWidget::itemClicked,
            this, &OutlineNavigator::onItemClicked);
    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked,
            this, &OutlineNavigator::onItemDoubleClicked);

    m_layout->addWidget(m_treeWidget, 1);

    // 统计标签
    m_countLabel = new QLabel(tr("共 0 个标题"), this);
    m_countLabel->setStyleSheet("color: gray; font-size: 11px;");
    m_layout->addWidget(m_countLabel);

    setMinimumWidth(180);
}

// 函数说明：设置 OutlineNavigator 的运行参数，并触发必要的界面或数据刷新。
void OutlineNavigator::setEditor(QPlainTextEdit *editor)
{
    if (m_editor) {
        disconnect(m_editor, nullptr, this, nullptr);
    }

    m_editor = editor;

    if (m_editor) {
        connect(m_editor, &QPlainTextEdit::textChanged,
                this, &OutlineNavigator::onTextChanged);
        connect(m_editor, &QPlainTextEdit::cursorPositionChanged,
                this, &OutlineNavigator::onCursorPositionChanged);

        // 立即更新大纲
        updateOutline();
    }
}

// 函数说明：设置 OutlineNavigator 的运行参数，并触发必要的界面或数据刷新。
void OutlineNavigator::setMaxLevel(int level)
{
    m_maxLevel = qBound(1, level, 6);
    updateOutline();
}

// 函数说明：设置 OutlineNavigator 的运行参数，并触发必要的界面或数据刷新。
void OutlineNavigator::setAutoUpdate(bool enable)
{
    m_autoUpdate = enable;
}

// 函数说明：设置 OutlineNavigator 的运行参数，并触发必要的界面或数据刷新。
void OutlineNavigator::setShowLineNumbers(bool show)
{
    m_showLineNumbers = show;
    updateOutline();
}

// 函数说明：设置 OutlineNavigator 的运行参数，并触发必要的界面或数据刷新。
void OutlineNavigator::setHighlightCurrent(bool enable)
{
    m_highlightCurrent = enable;
}

// 函数说明：实现 OutlineNavigator::headingCount 的核心逻辑，供当前模块调用。
int OutlineNavigator::headingCount(int level) const
{
    int count = 0;
    std::function<void(const QVector<OutlineItem>&)> countItems;
    countItems = [&count, level, &countItems](const QVector<OutlineItem> &items) {
        for (const auto &item : items) {
            if (level == 0 || item.level == level) {
                count++;
            }
            countItems(item.children);
        }
    };
    countItems(m_outline);
    return count;
}

// 函数说明：响应 OutlineNavigator 收到的信号或异步回调，并更新界面状态。
void OutlineNavigator::onTextChanged()
{
    if (m_autoUpdate) {
        // 使用成员定时器防抖，避免每次按键都创建新定时器
        m_updateTimer->start();
    }
}

// 函数说明：响应 OutlineNavigator 收到的信号或异步回调，并更新界面状态。
void OutlineNavigator::onCursorPositionChanged()
{
    if (m_highlightCurrent) {
        highlightCurrentPosition();
    }
}

// 函数说明：刷新 OutlineNavigator 的内部状态，并同步到相关界面。
void OutlineNavigator::updateOutline()
{
    if (!m_editor) return;

    QString text = m_editor->toPlainText();
    m_outline = parseMarkdownHeadings(text);

    // 清空并重建树（先重置 m_currentItem 避免悬空指针）
    m_currentItem = nullptr;
    m_treeWidget->clear();
    buildTree(m_outline);

    // 默认展开所有
    m_treeWidget->expandAll();

    // 更新统计
    int total = headingCount();
    m_countLabel->setText(tr("共 %1 个标题").arg(total));

    // 应用当前过滤
    if (!m_searchEdit->text().isEmpty()) {
        filterOutline(m_searchEdit->text());
    }

    // 高亮当前位置
    if (m_highlightCurrent) {
        highlightCurrentPosition();
    }

    emit outlineUpdated();
}

// 函数说明：解析输入内容，转换为 OutlineNavigator 后续处理使用的数据结构。
QVector<OutlineNavigator::OutlineItem> OutlineNavigator::parseMarkdownHeadings(const QString &text)
{
    QVector<OutlineItem> items;
    QStringList lines = text.split('\n');

    // ATX 样式标题正则：# 到 ######
    QRegularExpression atxRegex("^(#{1,6})\\s+(.+)$");

    int position = 0;
    for (int lineNum = 0; lineNum < lines.size(); ++lineNum) {
        const QString &line = lines[lineNum];

        QRegularExpressionMatch match = atxRegex.match(line);
        if (match.hasMatch()) {
            int level = match.captured(1).length();

            if (level <= m_maxLevel) {
                OutlineItem item;
                item.level = level;
                item.text = match.captured(2).trimmed();
                item.lineNumber = lineNum + 1;  // 行号从 1 开始
                item.position = position;

                items.append(item);
            }
        }

        position += line.length() + 1;  // +1 for newline
    }

    // 构建层级结构
    return buildHierarchy(items);
}

// 辅助函数：获取指定路径节点的 level
static int getLevelAt(const QVector<OutlineNavigator::OutlineItem> &result,
                      const QVector<int> &path)
{
    const QVector<OutlineNavigator::OutlineItem> *container = &result;
    int level = 0;
    for (int idx : path) {
        level = (*container)[idx].level;
        container = &(*container)[idx].children;
    }
    return level;
}

// 辅助函数：构建层级结构（使用索引路径避免悬空指针）
static QVector<OutlineNavigator::OutlineItem> buildHierarchy(const QVector<OutlineNavigator::OutlineItem> &flatItems)
{
    QVector<OutlineNavigator::OutlineItem> result;
    // stack 存储索引路径而非裸指针，QVector 扩容时仍然有效
    QVector<QVector<int>> stack;

    for (const auto &item : flatItems) {
        OutlineNavigator::OutlineItem newItem = item;

        // 找到合适的父节点
        while (!stack.isEmpty() && getLevelAt(result, stack.last()) >= item.level) {
            stack.removeLast();
        }

        if (stack.isEmpty()) {
            result.append(newItem);
            QVector<int> path;
            path.append(result.size() - 1);
            stack.append(path);
        } else {
            QVector<OutlineNavigator::OutlineItem> *container = &result;
            for (int idx : stack.last()) {
                container = &(*container)[idx].children;
            }
            container->append(newItem);
            QVector<int> path = stack.last();
            path.append(container->size() - 1);
            stack.append(path);
        }
    }

    return result;
}

// 函数说明：实现 OutlineNavigator::buildTree 的核心逻辑，供当前模块调用。
void OutlineNavigator::buildTree(const QVector<OutlineItem> &items, QTreeWidgetItem *parent)
{
    for (const auto &item : items) {
        addItemToTree(item, parent);
    }
}

// 函数说明：向 OutlineNavigator 管理的数据集合中添加一项内容。
void OutlineNavigator::addItemToTree(const OutlineItem &item, QTreeWidgetItem *parent)
{
    QTreeWidgetItem *treeItem;

    if (parent) {
        treeItem = new QTreeWidgetItem(parent);
    } else {
        treeItem = new QTreeWidgetItem(m_treeWidget);
    }

    // 设置显示文本
    QString displayText;
    if (m_showLineNumbers) {
        displayText = QString("%1 %2 (L%3)")
            .arg(getLevelIcon(item.level))
            .arg(item.text)
            .arg(item.lineNumber);
    } else {
        displayText = QString("%1 %2")
            .arg(getLevelIcon(item.level))
            .arg(item.text);
    }

    treeItem->setText(0, displayText);
    treeItem->setData(0, Qt::UserRole, item.lineNumber);
    treeItem->setData(0, Qt::UserRole + 1, item.position);
    treeItem->setData(0, Qt::UserRole + 2, item.level);

    // 根据级别设置缩进样式
    QFont font = treeItem->font(0);
    if (item.level == 1) {
        font.setBold(true);
        font.setPointSize(font.pointSize() + 1);
    } else if (item.level == 2) {
        font.setBold(true);
    }
    treeItem->setFont(0, font);

    // 递归添加子项
    for (const auto &child : item.children) {
        addItemToTree(child, treeItem);
    }
}

// 函数说明：读取 OutlineNavigator 当前保存的状态或计算结果。
QString OutlineNavigator::getLevelIcon(int level)
{
    switch (level) {
        case 1: return "📌";
        case 2: return "📎";
        case 3: return "•";
        case 4: return "◦";
        case 5: return "▪";
        case 6: return "▫";
        default: return "•";
    }
}

// 函数说明：响应 OutlineNavigator 收到的信号或异步回调，并更新界面状态。
void OutlineNavigator::onItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)
    navigateToItem(item);
}

// 函数说明：响应 OutlineNavigator 收到的信号或异步回调，并更新界面状态。
void OutlineNavigator::onItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)
    // 双击切换展开/折叠
    item->setExpanded(!item->isExpanded());
}

// 函数说明：实现 OutlineNavigator::navigateToItem 的核心逻辑，供当前模块调用。
void OutlineNavigator::navigateToItem(QTreeWidgetItem *item)
{
    if (!item || !m_editor) return;

    int lineNumber = item->data(0, Qt::UserRole).toInt();
    int position = item->data(0, Qt::UserRole + 1).toInt();

    // 移动光标到目标位置
    QTextCursor cursor = m_editor->textCursor();
    cursor.setPosition(position);
    m_editor->setTextCursor(cursor);

    // 确保目标行可见
    QTextBlock block = m_editor->document()->findBlockByLineNumber(lineNumber - 1);
    if (block.isValid()) {
        cursor = QTextCursor(block);
        m_editor->setTextCursor(cursor);
        m_editor->centerCursor();
    }

    // 设置焦点到编辑器
    m_editor->setFocus();

    emit itemClicked(lineNumber, position);
}

// 函数说明：实现 OutlineNavigator::filterOutline 的核心逻辑，供当前模块调用。
void OutlineNavigator::filterOutline(const QString &filter)
{
    if (filter.isEmpty()) {
        // 显示所有项
        QTreeWidgetItemIterator it(m_treeWidget);
        while (*it) {
            (*it)->setHidden(false);
            ++it;
        }
        m_treeWidget->expandAll();
        return;
    }

    // 应用过滤
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        applyFilter(m_treeWidget->topLevelItem(i), filter);
    }
}

// 函数说明：应用 OutlineNavigator 当前配置，让编辑器或预览立即生效。
void OutlineNavigator::applyFilter(QTreeWidgetItem *item, const QString &filter)
{
    bool hasVisibleChild = false;

    // 先处理子项
    for (int i = 0; i < item->childCount(); ++i) {
        applyFilter(item->child(i), filter);
        if (!item->child(i)->isHidden()) {
            hasVisibleChild = true;
        }
    }

    // 检查当前项是否匹配
    bool matches = matchesFilter(item->text(0), filter);

    // 如果匹配或有可见子项，则显示
    item->setHidden(!matches && !hasVisibleChild);

    // 展开包含匹配项的父节点
    if (matches || hasVisibleChild) {
        item->setExpanded(true);
    }
}

// 函数说明：实现 OutlineNavigator::matchesFilter 的核心逻辑，供当前模块调用。
bool OutlineNavigator::matchesFilter(const QString &text, const QString &filter)
{
    return text.contains(filter, Qt::CaseInsensitive);
}

// 函数说明：实现 OutlineNavigator::expandAll 的核心逻辑，供当前模块调用。
void OutlineNavigator::expandAll()
{
    m_treeWidget->expandAll();
}

// 函数说明：实现 OutlineNavigator::collapseAll 的核心逻辑，供当前模块调用。
void OutlineNavigator::collapseAll()
{
    m_treeWidget->collapseAll();
}

// 函数说明：实现 OutlineNavigator::highlightCurrentPosition 的核心逻辑，供当前模块调用。
void OutlineNavigator::highlightCurrentPosition()
{
    if (!m_editor || m_outline.isEmpty()) return;

    int cursorPosition = m_editor->textCursor().position();

    // 找到当前位置对应的标题
    QTreeWidgetItem *targetItem = nullptr;
    int closestPosition = -1;

    QTreeWidgetItemIterator it(m_treeWidget);
    while (*it) {
        int itemPosition = (*it)->data(0, Qt::UserRole + 1).toInt();
        if (itemPosition <= cursorPosition && itemPosition > closestPosition) {
            closestPosition = itemPosition;
            targetItem = *it;
        }
        ++it;
    }

    // 更新高亮
    if (m_currentItem && m_currentItem != targetItem) {
        m_currentItem->setBackground(0, QBrush());
    }

    if (targetItem) {
        targetItem->setBackground(0, QBrush(QColor(200, 220, 255)));
        m_treeWidget->scrollToItem(targetItem);
        m_currentItem = targetItem;

        // 发送当前标题变化信号
        OutlineItem currentItem;
        currentItem.level = targetItem->data(0, Qt::UserRole + 2).toInt();
        currentItem.lineNumber = targetItem->data(0, Qt::UserRole).toInt();
        currentItem.position = closestPosition;
        emit currentHeadingChanged(currentItem);
    }
}

// 函数说明：实现 OutlineNavigator::findItemByPosition 的核心逻辑，供当前模块调用。
QTreeWidgetItem* OutlineNavigator::findItemByPosition(int position, QTreeWidgetItem *parent)
{
    int count = parent ? parent->childCount() : m_treeWidget->topLevelItemCount();

    for (int i = 0; i < count; ++i) {
        QTreeWidgetItem *item = parent ?
            parent->child(i) : m_treeWidget->topLevelItem(i);

        if (item->data(0, Qt::UserRole + 1).toInt() == position) {
            return item;
        }

        QTreeWidgetItem *found = findItemByPosition(position, item);
        if (found) return found;
    }

    return nullptr;
}

