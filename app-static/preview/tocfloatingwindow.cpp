// 文件说明：app-static\preview\tocfloatingwindow.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "tocfloatingwindow.h"

#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextBlock>
#include <QScrollBar>
#include <QGraphicsDropShadowEffect>
#include <QApplication>
#include <QScreen>

// 函数说明：构造 TocFloatingWindow 对象，初始化本模块需要的状态、界面和资源。
TocFloatingWindow::TocFloatingWindow(QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint)
    , m_editor(nullptr)
    , m_webView(nullptr)
    , m_parentWindow(nullptr)
    , m_position(Position::TopRight)
    , m_offset(20, 80)
    , m_maxHeight(400)
    , m_autoHide(false)
    , m_autoFollow(true)
    , m_currentItem(nullptr)
    , m_autoHideTimer(new QTimer(this))
    , m_isDragging(false)
{
    setupUi();

    m_autoHideTimer->setSingleShot(true);
    m_autoHideTimer->setInterval(3000);
    connect(m_autoHideTimer, &QTimer::timeout,
            this, &TocFloatingWindow::onAutoHideTimeout);

    setAttribute(Qt::WA_TranslucentBackground);
    setWindowOpacity(0.95);
}

// 函数说明：销毁 TocFloatingWindow 对象，释放本模块持有的资源。
TocFloatingWindow::~TocFloatingWindow()
{
}

// 函数说明：初始化 TocFloatingWindow 的 setupUi 相关界面、动作或服务连接。
void TocFloatingWindow::setupUi()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(10, 10, 10, 10);
    m_layout->setSpacing(5);

    // 设置背景样式
    setStyleSheet(R"(
        TocFloatingWindow {
            background-color: rgba(255, 255, 255, 0.95);
            border: 1px solid #ddd;
            border-radius: 8px;
        }
    )");

    // 添加阴影效果
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 60));
    shadow->setOffset(0, 3);
    setGraphicsEffect(shadow);

    // 标题栏
    QHBoxLayout *titleLayout = new QHBoxLayout();

    QLabel *titleLabel = new QLabel(tr("📑 目录"), this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 13px; color: #333;");
    titleLayout->addWidget(titleLabel);

    titleLayout->addStretch();

    QPushButton *closeBtn = new QPushButton("×", this);
    closeBtn->setFixedSize(20, 20);
    closeBtn->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            border: none;
            font-size: 16px;
            color: #999;
        }
        QPushButton:hover {
            color: #333;
            background: rgba(0, 0, 0, 0.1);
            border-radius: 10px;
        }
    )");
    connect(closeBtn, &QPushButton::clicked, this, &TocFloatingWindow::hide);
    titleLayout->addWidget(closeBtn);

    m_layout->addLayout(titleLayout);

    // 搜索框
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("搜索..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setStyleSheet(R"(
        QLineEdit {
            border: 1px solid #ddd;
            border-radius: 4px;
            padding: 5px 8px;
            background: white;
        }
        QLineEdit:focus {
            border-color: #0078d4;
        }
    )");
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &TocFloatingWindow::filterItems);
    m_layout->addWidget(m_searchEdit);

    // 树形控件
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderHidden(true);
    m_treeWidget->setIndentation(15);
    m_treeWidget->setAnimated(true);
    m_treeWidget->setRootIsDecorated(true);
    m_treeWidget->setStyleSheet(R"(
        QTreeWidget {
            border: none;
            background: transparent;
            outline: none;
        }
        QTreeWidget::item {
            padding: 4px 0;
            border-radius: 4px;
        }
        QTreeWidget::item:hover {
            background: rgba(0, 120, 212, 0.1);
        }
        QTreeWidget::item:selected {
            background: rgba(0, 120, 212, 0.2);
            color: #0078d4;
        }
    )");

    connect(m_treeWidget, &QTreeWidget::itemClicked,
            this, &TocFloatingWindow::onItemClicked);

    m_layout->addWidget(m_treeWidget, 1);

    setFixedWidth(250);
    setMaximumHeight(m_maxHeight);
}

// 函数说明：设置 TocFloatingWindow 的运行参数，并触发必要的界面或数据刷新。
void TocFloatingWindow::setEditor(QPlainTextEdit *editor)
{
    if (m_editor) {
        disconnect(m_editor, nullptr, this, nullptr);
    }

    m_editor = editor;

    if (m_editor) {
        connect(m_editor->verticalScrollBar(), &QScrollBar::valueChanged,
                this, &TocFloatingWindow::onEditorScrolled);
    }
}

// 函数说明：设置 TocFloatingWindow 的运行参数，并触发必要的界面或数据刷新。
void TocFloatingWindow::setWebView(QWebEngineView *webView)
{
    m_webView = webView;
}

// 函数说明：设置 TocFloatingWindow 的运行参数，并触发必要的界面或数据刷新。
void TocFloatingWindow::setParentWindow(QWidget *window)
{
    m_parentWindow = window;
}

// 函数说明：设置 TocFloatingWindow 的运行参数，并触发必要的界面或数据刷新。
void TocFloatingWindow::setTocHtml(const QString &tocHtml)
{
    m_tocItems = parseHtmlToc(tocHtml);
    m_currentItem = nullptr;
    m_treeWidget->clear();
    buildTree(m_tocItems);
    m_treeWidget->expandAll();
}

// 函数说明：设置 TocFloatingWindow 的运行参数，并触发必要的界面或数据刷新。
void TocFloatingWindow::setTocItems(const QVector<TocItem> &items)
{
    m_tocItems = items;
    m_currentItem = nullptr;
    m_treeWidget->clear();
    buildTree(m_tocItems);
    m_treeWidget->expandAll();
}

// 函数说明：刷新 TocFloatingWindow 的内部状态，并同步到相关界面。
void TocFloatingWindow::updateFromMarkdown(const QString &markdown)
{
    m_tocItems = parseMarkdownHeadings(markdown);
    m_currentItem = nullptr;
    m_treeWidget->clear();
    buildTree(m_tocItems);
    m_treeWidget->expandAll();
}

// 函数说明：解析输入内容，转换为 TocFloatingWindow 后续处理使用的数据结构。
QVector<TocFloatingWindow::TocItem> TocFloatingWindow::parseHtmlToc(const QString &html)
{
    QVector<TocItem> items;

    // 解析 HTML TOC (通常是 <ul><li><a href="#anchor">Title</a></li></ul> 格式)
    QRegularExpression linkRegex("<a[^>]*href=\"#([^\"]*)\"[^>]*>([^<]*)</a>");
    QRegularExpressionMatchIterator it = linkRegex.globalMatch(html);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        TocItem item;
        item.anchor = match.captured(1);
        item.text = match.captured(2).trimmed();
        item.level = 1;  // HTML TOC 通常是扁平的
        items.append(item);
    }

    return items;
}

// 函数说明：解析输入内容，转换为 TocFloatingWindow 后续处理使用的数据结构。
QVector<TocFloatingWindow::TocItem> TocFloatingWindow::parseMarkdownHeadings(const QString &markdown)
{
    QVector<TocItem> flatItems;
    QStringList lines = markdown.split('\n');

    QRegularExpression headingRegex("^(#{1,6})\\s+(.+)$");

    int lineNumber = 0;
    for (const QString &line : lines) {
        lineNumber++;
        QRegularExpressionMatch match = headingRegex.match(line);

        if (match.hasMatch()) {
            TocItem item;
            item.level = match.captured(1).length();
            item.text = match.captured(2).trimmed();
            item.lineNumber = lineNumber;

            // 生成锚点
            item.anchor = item.text.toLower()
                .replace(QRegularExpression("[^a-z0-9\\u4e00-\\u9fff]+"), "-")
                .replace(QRegularExpression("^-+|-+$"), "");

            flatItems.append(item);
        }
    }

    // 构建层级结构（使用索引路径避免 QVector 扩容导致悬空指针）
    QVector<TocItem> result;
    QVector<QVector<int>> stack;

    for (const auto &item : flatItems) {
        TocItem newItem = item;

        while (!stack.isEmpty()) {
            // 通过索引路径获取 level
            const QVector<int> &path = stack.last();
            const QVector<TocItem> *container = &result;
            int parentLevel = 0;
            for (int idx : path) {
                parentLevel = (*container)[idx].level;
                container = &(*container)[idx].children;
            }
            if (parentLevel >= item.level) {
                stack.removeLast();
            } else {
                break;
            }
        }

        if (stack.isEmpty()) {
            result.append(newItem);
            QVector<int> path;
            path.append(result.size() - 1);
            stack.append(path);
        } else {
            QVector<TocItem> *container = &result;
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

// 函数说明：实现 TocFloatingWindow::buildTree 的核心逻辑，供当前模块调用。
void TocFloatingWindow::buildTree(const QVector<TocItem> &items, QTreeWidgetItem *parent)
{
    for (const auto &item : items) {
        addItemToTree(item, parent);
    }
}

// 函数说明：向 TocFloatingWindow 管理的数据集合中添加一项内容。
void TocFloatingWindow::addItemToTree(const TocItem &item, QTreeWidgetItem *parent)
{
    QTreeWidgetItem *treeItem;

    if (parent) {
        treeItem = new QTreeWidgetItem(parent);
    } else {
        treeItem = new QTreeWidgetItem(m_treeWidget);
    }

    QString displayText = QString("%1 %2")
        .arg(getLevelIcon(item.level))
        .arg(item.text);

    treeItem->setText(0, displayText);
    treeItem->setData(0, Qt::UserRole, item.anchor);
    treeItem->setData(0, Qt::UserRole + 1, item.lineNumber);
    treeItem->setData(0, Qt::UserRole + 2, item.level);

    // 根据级别设置字体
    QFont font = treeItem->font(0);
    if (item.level == 1) {
        font.setBold(true);
    }
    treeItem->setFont(0, font);

    // 递归添加子项
    for (const auto &child : item.children) {
        addItemToTree(child, treeItem);
    }
}

// 函数说明：读取 TocFloatingWindow 当前保存的状态或计算结果。
QString TocFloatingWindow::getLevelIcon(int level)
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

// 函数说明：设置 TocFloatingWindow 的运行参数，并触发必要的界面或数据刷新。
void TocFloatingWindow::setPosition(Position pos)
{
    m_position = pos;
    updatePosition();
}

// 函数说明：设置 TocFloatingWindow 的运行参数，并触发必要的界面或数据刷新。
void TocFloatingWindow::setOffset(int x, int y)
{
    m_offset = QPoint(x, y);
    updatePosition();
}

// 函数说明：设置 TocFloatingWindow 的运行参数，并触发必要的界面或数据刷新。
void TocFloatingWindow::setOpacity(qreal opacity)
{
    setWindowOpacity(opacity);
}

// 函数说明：设置 TocFloatingWindow 的运行参数，并触发必要的界面或数据刷新。
void TocFloatingWindow::setMaxHeight(int height)
{
    m_maxHeight = height;
    setMaximumHeight(height);
}

// 函数说明：设置 TocFloatingWindow 的运行参数，并触发必要的界面或数据刷新。
void TocFloatingWindow::setAutoHide(bool enable)
{
    m_autoHide = enable;
}

// 函数说明：设置 TocFloatingWindow 的运行参数，并触发必要的界面或数据刷新。
void TocFloatingWindow::setAutoFollow(bool enable)
{
    m_autoFollow = enable;
}

// 函数说明：实现 TocFloatingWindow::highlightItem 的核心逻辑，供当前模块调用。
void TocFloatingWindow::highlightItem(const QString &anchor)
{
    // 清除之前的高亮
    if (m_currentItem) {
        m_currentItem->setBackground(0, QBrush());
    }

    // 查找并高亮新项
    QTreeWidgetItemIterator it(m_treeWidget);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toString() == anchor) {
            (*it)->setBackground(0, QBrush(QColor(0, 120, 212, 50)));
            m_treeWidget->scrollToItem(*it);
            m_currentItem = *it;
            return;
        }
        ++it;
    }
}

// 函数说明：实现 TocFloatingWindow::highlightByLineNumber 的核心逻辑，供当前模块调用。
void TocFloatingWindow::highlightByLineNumber(int lineNumber)
{
    if (m_currentItem) {
        m_currentItem->setBackground(0, QBrush());
    }

    // 找到最近的标题
    QTreeWidgetItem *closestItem = nullptr;
    int closestLine = -1;

    QTreeWidgetItemIterator it(m_treeWidget);
    while (*it) {
        int itemLine = (*it)->data(0, Qt::UserRole + 1).toInt();
        if (itemLine <= lineNumber && itemLine > closestLine) {
            closestLine = itemLine;
            closestItem = *it;
        }
        ++it;
    }

    if (closestItem) {
        closestItem->setBackground(0, QBrush(QColor(0, 120, 212, 50)));
        m_treeWidget->scrollToItem(closestItem);
        m_currentItem = closestItem;
    }
}

// 函数说明：显示 TocFloatingWindow 管理的面板、对话框或提示信息。
void TocFloatingWindow::show()
{
    updatePosition();
    QWidget::show();
    emit visibilityChanged(true);

    if (m_autoHide) {
        m_autoHideTimer->start();
    }
}

// 函数说明：隐藏 TocFloatingWindow 管理的界面组件。
void TocFloatingWindow::hide()
{
    QWidget::hide();
    emit visibilityChanged(false);
}

// 函数说明：切换 TocFloatingWindow 对应功能的启用状态。
void TocFloatingWindow::toggle()
{
    if (isVisible()) {
        hide();
    } else {
        show();
    }
}

// 函数说明：刷新 TocFloatingWindow 的内部状态，并同步到相关界面。
void TocFloatingWindow::updatePosition()
{
    QPoint pos = calculatePosition();
    move(pos);
}

// 函数说明：实现 TocFloatingWindow::calculatePosition 的核心逻辑，供当前模块调用。
QPoint TocFloatingWindow::calculatePosition()
{
    QWidget *ref = m_parentWindow ? m_parentWindow : parentWidget();
    if (!ref) {
        return QPoint(100, 100);
    }

    QRect refRect = ref->geometry();
    QPoint result;

    switch (m_position) {
        case Position::TopLeft:
            result = refRect.topLeft() + m_offset;
            break;

        case Position::TopRight:
            result = QPoint(refRect.right() - width() - m_offset.x(),
                           refRect.top() + m_offset.y());
            break;

        case Position::BottomLeft:
            result = QPoint(refRect.left() + m_offset.x(),
                           refRect.bottom() - height() - m_offset.y());
            break;

        case Position::BottomRight:
            result = QPoint(refRect.right() - width() - m_offset.x(),
                           refRect.bottom() - height() - m_offset.y());
            break;

        case Position::FollowCursor:
            result = QCursor::pos() + QPoint(20, 20);
            break;
    }

    // 确保窗口在屏幕内
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenRect = screen->availableGeometry();
        result.setX(qBound(screenRect.left(), result.x(),
                          screenRect.right() - width()));
        result.setY(qBound(screenRect.top(), result.y(),
                          screenRect.bottom() - height()));
    }

    return result;
}

// 函数说明：实现 TocFloatingWindow::filterItems 的核心逻辑，供当前模块调用。
void TocFloatingWindow::filterItems(const QString &filter)
{
    if (filter.isEmpty()) {
        QTreeWidgetItemIterator it(m_treeWidget);
        while (*it) {
            (*it)->setHidden(false);
            ++it;
        }
        m_treeWidget->expandAll();
        return;
    }

    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        applyFilter(m_treeWidget->topLevelItem(i), filter);
    }
}

// 函数说明：应用 TocFloatingWindow 当前配置，让编辑器或预览立即生效。
void TocFloatingWindow::applyFilter(QTreeWidgetItem *item, const QString &filter)
{
    bool hasVisibleChild = false;

    for (int i = 0; i < item->childCount(); ++i) {
        applyFilter(item->child(i), filter);
        if (!item->child(i)->isHidden()) {
            hasVisibleChild = true;
        }
    }

    bool matches = matchesFilter(item->text(0), filter);
    item->setHidden(!matches && !hasVisibleChild);

    if (matches || hasVisibleChild) {
        item->setExpanded(true);
    }
}

// 函数说明：实现 TocFloatingWindow::matchesFilter 的核心逻辑，供当前模块调用。
bool TocFloatingWindow::matchesFilter(const QString &text, const QString &filter)
{
    return text.contains(filter, Qt::CaseInsensitive);
}

// 函数说明：实现 TocFloatingWindow::expandAll 的核心逻辑，供当前模块调用。
void TocFloatingWindow::expandAll()
{
    m_treeWidget->expandAll();
}

// 函数说明：实现 TocFloatingWindow::collapseAll 的核心逻辑，供当前模块调用。
void TocFloatingWindow::collapseAll()
{
    m_treeWidget->collapseAll();
}

// 函数说明：响应 TocFloatingWindow 收到的信号或异步回调，并更新界面状态。
void TocFloatingWindow::onItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)

    QString anchor = item->data(0, Qt::UserRole).toString();
    int lineNumber = item->data(0, Qt::UserRole + 1).toInt();

    // 跳转到编辑器对应位置
    if (m_editor && lineNumber > 0) {
        QTextBlock block = m_editor->document()->findBlockByLineNumber(lineNumber - 1);
        if (block.isValid()) {
            QTextCursor cursor(block);
            m_editor->setTextCursor(cursor);
            m_editor->centerCursor();
            m_editor->setFocus();
        }
    }

    // 跳转到预览对应位置
    if (m_webView && !anchor.isEmpty()) {
        QString js = QString("document.getElementById('%1').scrollIntoView({behavior: 'smooth'});")
            .arg(anchor);
        m_webView->page()->runJavaScript(js);
    }

    emit itemClicked(anchor, lineNumber);
}

// 函数说明：响应 TocFloatingWindow 收到的信号或异步回调，并更新界面状态。
void TocFloatingWindow::onEditorScrolled()
{
    if (!m_editor || !m_autoFollow) return;

    int currentLine = m_editor->textCursor().blockNumber() + 1;
    highlightByLineNumber(currentLine);
}

// 函数说明：响应 TocFloatingWindow 收到的信号或异步回调，并更新界面状态。
void TocFloatingWindow::onAutoHideTimeout()
{
    if (m_autoHide && !underMouse()) {
        hide();
    }
}

// 函数说明：显示 TocFloatingWindow 管理的面板、对话框或提示信息。
void TocFloatingWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    updatePosition();
}

// 函数说明：隐藏 TocFloatingWindow 管理的界面组件。
void TocFloatingWindow::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    m_autoHideTimer->stop();
}

// 函数说明：实现 TocFloatingWindow::enterEvent 的核心逻辑，供当前模块调用。
void TocFloatingWindow::enterEvent(QEnterEvent *event)
{
    QWidget::enterEvent(event);
    m_autoHideTimer->stop();
    setWindowOpacity(1.0);
}

// 函数说明：实现 TocFloatingWindow::leaveEvent 的核心逻辑，供当前模块调用。
void TocFloatingWindow::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    setWindowOpacity(0.95);

    if (m_autoHide) {
        m_autoHideTimer->start();
    }
}

// 函数说明：处理鼠标事件，更新选择、拖拽或交互状态。
void TocFloatingWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_dragStartPos = event->globalPosition().toPoint();
        m_windowStartPos = pos();
    }
    QWidget::mousePressEvent(event);
}

// 函数说明：处理鼠标事件，更新选择、拖拽或交互状态。
void TocFloatingWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging) {
        QPoint delta = event->globalPosition().toPoint() - m_dragStartPos;
        move(m_windowStartPos + delta);
    }
    QWidget::mouseMoveEvent(event);
}

// 函数说明：处理鼠标事件，更新选择、拖拽或交互状态。
void TocFloatingWindow::mouseReleaseEvent(QMouseEvent *event)
{
    m_isDragging = false;
    QWidget::mouseReleaseEvent(event);
}

