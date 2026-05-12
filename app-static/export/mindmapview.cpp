// 文件说明：app-static\export\mindmapview.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "mindmapview.h"

#include <QPainter>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QSvgGenerator>
#include <QPdfWriter>
#include <QLineEdit>
#include <QRegularExpression>
#include <QtMath>
#include <QDebug>

// ==================== MindMapView ====================

MindMapView::MindMapView(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
    , m_rootData(nullptr)
    , m_rootItem(nullptr)
    , m_layoutType(LayoutType::Horizontal)
    , m_selectedNode(nullptr)
    , m_horizontalSpacing(60)
    , m_verticalSpacing(30)
    , m_levelSpacing(150)
    , m_editable(true)
    , m_editingNode(nullptr)
    , m_editProxy(nullptr)
    , m_editWidget(nullptr)
{
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::TextAntialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    // 创建上下文菜单
    m_contextMenu = new QMenu(this);
    m_contextMenu->addAction(tr("展开全部"), this, &MindMapView::expandAll);
    m_contextMenu->addAction(tr("折叠全部"), this, &MindMapView::collapseAll);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(tr("适应窗口"), this, &MindMapView::fitInView);
    m_contextMenu->addAction(tr("居中显示"), this, &MindMapView::centerOnRoot);
    m_contextMenu->addSeparator();

    QMenu *layoutMenu = m_contextMenu->addMenu(tr("布局"));
    layoutMenu->addAction(tr("水平布局"), this, [this]() { setLayoutType(LayoutType::Horizontal); });
    layoutMenu->addAction(tr("垂直布局"), this, [this]() { setLayoutType(LayoutType::Vertical); });
    layoutMenu->addAction(tr("树形布局"), this, [this]() { setLayoutType(LayoutType::Tree); });
    layoutMenu->addAction(tr("径向布局"), this, [this]() { setLayoutType(LayoutType::Radial); });

    m_contextMenu->addSeparator();
    m_contextMenu->addAction(tr("导出为图片..."), this, [this]() {
        QString path = QFileDialog::getSaveFileName(this, tr("导出图片"),
            QString(), tr("PNG 图片 (*.png);;JPEG 图片 (*.jpg);;所有文件 (*)"));
        if (!path.isEmpty()) {
            exportToImage(path);
        }
    });
    m_contextMenu->addAction(tr("导出为 SVG..."), this, [this]() {
        QString path = QFileDialog::getSaveFileName(this, tr("导出 SVG"),
            QString(), tr("SVG 文件 (*.svg)"));
        if (!path.isEmpty()) {
            exportToSvg(path);
        }
    });
    m_contextMenu->addAction(tr("导出为 PDF..."), this, [this]() {
        QString path = QFileDialog::getSaveFileName(this, tr("导出 PDF"),
            QString(), tr("PDF 文件 (*.pdf)"));
        if (!path.isEmpty()) {
            exportToPdf(path);
        }
    });

    m_contextMenu->addSeparator();

    // 编辑菜单项
    m_contextMenu->addAction(tr("添加子节点"), this, [this]() {
        if (m_selectedNode) {
            addChildNode(m_selectedNode);
        }
    });
    m_contextMenu->addAction(tr("添加同级节点"), this, [this]() {
        if (m_selectedNode && m_selectedNode->parentNode()) {
            addSiblingNode(m_selectedNode);
        }
    });
    m_contextMenu->addAction(tr("编辑节点"), this, [this]() {
        if (m_selectedNode) {
            startEditingNode(m_selectedNode);
        }
    });
    m_contextMenu->addAction(tr("删除节点"), this, [this]() {
        if (m_selectedNode && m_selectedNode->level() > 0) {
            deleteNode(m_selectedNode);
        }
    });

    setBackgroundBrush(m_theme.backgroundColor);
}

// 函数说明：销毁 MindMapView 对象，释放本模块持有的资源。
MindMapView::~MindMapView()
{
    clear();
}

// 函数说明：加载 MindMapView 需要的数据、配置或外部资源。
void MindMapView::loadFromMarkdown(const QString &markdown)
{
    clear();

    m_rootData = parseMarkdown(markdown);
    if (!m_rootData) return;

    buildGraphicsItems();
    relayout();
    centerOnRoot();
}

// 函数说明：清空 MindMapView 保存的临时状态或缓存数据。
void MindMapView::clear()
{
    m_scene->clear();
    m_rootItem = nullptr;
    m_selectedNode = nullptr;

    if (m_rootData) {
        delete m_rootData;
        m_rootData = nullptr;
    }
}

// 函数说明：设置 MindMapView 的运行参数，并触发必要的界面或数据刷新。
void MindMapView::setLayoutType(LayoutType type)
{
    if (m_layoutType != type) {
        m_layoutType = type;
        relayout();
    }
}

// 函数说明：设置 MindMapView 的运行参数，并触发必要的界面或数据刷新。
void MindMapView::setTheme(const Theme &theme)
{
    m_theme = theme;
    setBackgroundBrush(theme.backgroundColor);

    if (m_rootItem) {
        m_scene->update();
    }
}

// 函数说明：实现 MindMapView::builtinThemes 的核心逻辑，供当前模块调用。
QVector<MindMapView::Theme> MindMapView::builtinThemes()
{
    QVector<Theme> themes;

    // 默认主题
    themes.append(Theme());

    // 深色主题
    Theme dark;
    dark.name = "Dark";
    dark.backgroundColor = QColor("#2D2D2D");
    dark.nodeColor = QColor("#4A90D9");
    dark.nodeTextColor = Qt::white;
    dark.lineColor = QColor("#555555");
    dark.selectedColor = QColor("#FF6B6B");
    themes.append(dark);

    // 绿色主题
    Theme green;
    green.name = "Green";
    green.backgroundColor = QColor("#F0FFF0");
    green.nodeColor = QColor("#2E8B57");
    green.nodeTextColor = Qt::white;
    green.lineColor = QColor("#3CB371");
    green.selectedColor = QColor("#FF6347");
    themes.append(green);

    // 紫色主题
    Theme purple;
    purple.name = "Purple";
    purple.backgroundColor = QColor("#F8F0FF");
    purple.nodeColor = QColor("#8A2BE2");
    purple.nodeTextColor = Qt::white;
    purple.lineColor = QColor("#9370DB");
    purple.selectedColor = QColor("#FF69B4");
    themes.append(purple);

    return themes;
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
bool MindMapView::exportToImage(const QString &filePath, const QString &format)
{
    if (!m_rootItem) return false;

    QRectF sceneRect = m_scene->itemsBoundingRect().adjusted(-50, -50, 50, 50);
    QImage image(sceneRect.size().toSize(), QImage::Format_ARGB32);
    image.fill(m_theme.backgroundColor);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    m_scene->render(&painter, QRectF(), sceneRect);

    return image.save(filePath, format.toUtf8().constData());
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
bool MindMapView::exportToSvg(const QString &filePath)
{
    if (!m_rootItem) return false;

    QRectF sceneRect = m_scene->itemsBoundingRect().adjusted(-50, -50, 50, 50);

    QSvgGenerator generator;
    generator.setFileName(filePath);
    generator.setSize(sceneRect.size().toSize());
    generator.setViewBox(QRect(0, 0, sceneRect.width(), sceneRect.height()));
    generator.setTitle(tr("思维导图"));

    QPainter painter(&generator);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    m_scene->render(&painter, QRectF(), sceneRect);

    return true;
}

// 函数说明：实现 MindMapView::expandAll 的核心逻辑，供当前模块调用。
void MindMapView::expandAll()
{
    if (!m_rootItem) return;

    std::function<void(MindMapNode*)> expand = [&expand](MindMapNode *node) {
        node->setCollapsed(false);
        for (MindMapNode *child : node->children()) {
            expand(child);
        }
    };

    expand(m_rootItem);
    relayout();
}

// 函数说明：实现 MindMapView::collapseAll 的核心逻辑，供当前模块调用。
void MindMapView::collapseAll()
{
    if (!m_rootItem) return;

    std::function<void(MindMapNode*)> collapse = [&collapse](MindMapNode *node) {
        if (node->level() > 0) {
            node->setCollapsed(true);
        }
        for (MindMapNode *child : node->children()) {
            collapse(child);
        }
    };

    collapse(m_rootItem);
    relayout();
}

// 函数说明：实现 MindMapView::expandNode 的核心逻辑，供当前模块调用。
void MindMapView::expandNode(MindMapNode *node)
{
    if (node) {
        node->setCollapsed(false);
        relayout();
    }
}

// 函数说明：实现 MindMapView::collapseNode 的核心逻辑，供当前模块调用。
void MindMapView::collapseNode(MindMapNode *node)
{
    if (node) {
        node->setCollapsed(true);
        relayout();
    }
}

// 函数说明：实现 MindMapView::centerOnRoot 的核心逻辑，供当前模块调用。
void MindMapView::centerOnRoot()
{
    if (m_rootItem) {
        centerOn(m_rootItem);
    }
}

// 函数说明：实现 MindMapView::fitInView 的核心逻辑，供当前模块调用。
void MindMapView::fitInView()
{
    if (m_rootItem) {
        QGraphicsView::fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
    }
}

// 函数说明：实现 MindMapView::wheelEvent 的核心逻辑，供当前模块调用。
void MindMapView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        // 缩放
        qreal factor = event->angleDelta().y() > 0 ? 1.1 : 0.9;
        scale(factor, factor);
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

// 函数说明：实现 MindMapView::contextMenuEvent 的核心逻辑，供当前模块调用。
void MindMapView::contextMenuEvent(QContextMenuEvent *event)
{
    m_contextMenu->exec(event->globalPos());
}

// 函数说明：处理鼠标事件，更新选择、拖拽或交互状态。
void MindMapView::mousePressEvent(QMouseEvent *event)
{
    QGraphicsView::mousePressEvent(event);
}

// 函数说明：处理鼠标事件，更新选择、拖拽或交互状态。
void MindMapView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QGraphicsView::mouseDoubleClickEvent(event);
}

// 函数说明：响应 MindMapView 收到的信号或异步回调，并更新界面状态。
void MindMapView::onNodeSelected(MindMapNode *node)
{
    if (m_selectedNode && m_selectedNode != node) {
        m_selectedNode->setSelected(false);
    }

    m_selectedNode = node;
    if (node) {
        node->setSelected(true);
        emit selectionChanged(node->text());
    }
}

// 函数说明：解析输入内容，转换为 MindMapView 后续处理使用的数据结构。
MindMapView::NodeData* MindMapView::parseMarkdown(const QString &markdown)
{
    NodeData *root = new NodeData();
    root->text = tr("文档");
    root->level = 0;

    parseHeadings(markdown, root);

    return root;
}

// 函数说明：解析输入内容，转换为 MindMapView 后续处理使用的数据结构。
void MindMapView::parseHeadings(const QString &markdown, NodeData *root)
{
    QStringList lines = markdown.split('\n');
    QVector<NodeData*> levelStack;
    levelStack.resize(7);
    levelStack[0] = root;

    QRegularExpression headingRegex("^(#{1,6})\\s+(.+)$");

    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i];
        QRegularExpressionMatch match = headingRegex.match(line);

        if (match.hasMatch()) {
            int level = match.captured(1).length();
            QString text = match.captured(2).trimmed();

            NodeData *node = new NodeData();
            node->text = text;
            node->level = level;
            node->lineNumber = i;

            // 找到父节点
            NodeData *parent = nullptr;
            for (int j = level - 1; j >= 0; --j) {
                if (levelStack[j]) {
                    parent = levelStack[j];
                    break;
                }
            }

            if (!parent) parent = root;

            node->parent = parent;
            parent->children.append(node);
            levelStack[level] = node;

            // 清除更深层级
            for (int j = level + 1; j < levelStack.size(); ++j) {
                levelStack[j] = nullptr;
            }
        }
    }
}

// 函数说明：实现 MindMapView::buildGraphicsItems 的核心逻辑，供当前模块调用。
void MindMapView::buildGraphicsItems()
{
    if (!m_rootData) return;

    m_rootItem = createNodeItem(m_rootData);
    m_scene->addItem(m_rootItem);
}

// 函数说明：创建 MindMapView 需要的对象、记录或输出内容。
MindMapNode* MindMapView::createNodeItem(NodeData *data, MindMapNode *parentItem)
{
    MindMapNode *node = new MindMapNode(data, this, parentItem);

    if (parentItem) {
        parentItem->addChild(node);
    }

    for (NodeData *childData : data->children) {
        createNodeItem(childData, node);
    }

    return node;
}

// 函数说明：实现 MindMapView::relayout 的核心逻辑，供当前模块调用。
void MindMapView::relayout()
{
    if (!m_rootItem) return;

    switch (m_layoutType) {
        case LayoutType::Radial:
            layoutRadial(m_rootItem, 0, 0, 0, 360, 200);
            break;
        case LayoutType::Tree:
            layoutTree(m_rootItem, 0, 0, true);
            break;
        case LayoutType::Horizontal:
            layoutHorizontal(m_rootItem, 0, 0);
            break;
        case LayoutType::Vertical:
            layoutVertical(m_rootItem, 0, 0);
            break;
    }

    m_rootItem->updateConnections();
    m_scene->setSceneRect(m_scene->itemsBoundingRect().adjusted(-100, -100, 100, 100));
}

// 函数说明：实现 MindMapView::layoutRadial 的核心逻辑，供当前模块调用。
void MindMapView::layoutRadial(MindMapNode *root, qreal cx, qreal cy,
                               qreal startAngle, qreal sweepAngle, qreal radius)
{
    root->setPos(cx, cy);

    QVector<MindMapNode*> visibleChildren;
    for (MindMapNode *child : root->children()) {
        if (!root->isCollapsed()) {
            visibleChildren.append(child);
        }
    }

    if (visibleChildren.isEmpty()) return;

    qreal angleStep = sweepAngle / visibleChildren.size();
    qreal angle = startAngle + angleStep / 2;

    for (MindMapNode *child : visibleChildren) {
        qreal x = cx + radius * qCos(qDegreesToRadians(angle));
        qreal y = cy + radius * qSin(qDegreesToRadians(angle));

        child->setPos(x, y);
        child->setVisible(true);

        // 递归布局子节点
        qreal childSweep = angleStep * 0.8;
        layoutRadial(child, x, y, angle - childSweep / 2, childSweep, radius * 0.7);

        angle += angleStep;
    }
}

// 函数说明：实现 MindMapView::layoutHorizontal 的核心逻辑，供当前模块调用。
void MindMapView::layoutHorizontal(MindMapNode *root, qreal x, qreal y)
{
    root->setPos(x, y);

    QVector<MindMapNode*> visibleChildren;
    for (MindMapNode *child : root->children()) {
        if (!root->isCollapsed()) {
            visibleChildren.append(child);
            child->setVisible(true);
        } else {
            child->setVisible(false);
        }
    }

    if (visibleChildren.isEmpty()) return;

    QSizeF subtreeSize = calculateSubtreeSize(root, true);
    qreal childX = x + m_levelSpacing;
    qreal startY = y - subtreeSize.height() / 2;
    qreal currentY = startY;

    for (MindMapNode *child : visibleChildren) {
        QSizeF childSize = calculateSubtreeSize(child, true);
        qreal childY = currentY + childSize.height() / 2;

        layoutHorizontal(child, childX, childY);

        currentY += childSize.height() + m_verticalSpacing;
    }
}

// 函数说明：实现 MindMapView::layoutVertical 的核心逻辑，供当前模块调用。
void MindMapView::layoutVertical(MindMapNode *root, qreal x, qreal y)
{
    root->setPos(x, y);

    QVector<MindMapNode*> visibleChildren;
    for (MindMapNode *child : root->children()) {
        if (!root->isCollapsed()) {
            visibleChildren.append(child);
            child->setVisible(true);
        } else {
            child->setVisible(false);
        }
    }

    if (visibleChildren.isEmpty()) return;

    QSizeF subtreeSize = calculateSubtreeSize(root, false);
    qreal childY = y + m_levelSpacing;
    qreal startX = x - subtreeSize.width() / 2;
    qreal currentX = startX;

    for (MindMapNode *child : visibleChildren) {
        QSizeF childSize = calculateSubtreeSize(child, false);
        qreal childX = currentX + childSize.width() / 2;

        layoutVertical(child, childX, childY);

        currentX += childSize.width() + m_horizontalSpacing;
    }
}

// 函数说明：实现 MindMapView::layoutTree 的核心逻辑，供当前模块调用。
void MindMapView::layoutTree(MindMapNode *root, qreal x, qreal y, bool horizontal)
{
    if (horizontal) {
        layoutHorizontal(root, x, y);
    } else {
        layoutVertical(root, x, y);
    }
}

// 函数说明：实现 MindMapView::calculateSubtreeSize 的核心逻辑，供当前模块调用。
QSizeF MindMapView::calculateSubtreeSize(MindMapNode *node, bool horizontal)
{
    QRectF nodeRect = node->boundingRect();
    qreal width = nodeRect.width();
    qreal height = nodeRect.height();

    if (node->isCollapsed() || node->children().isEmpty()) {
        return QSizeF(width, height);
    }

    qreal childrenWidth = 0;
    qreal childrenHeight = 0;

    for (MindMapNode *child : node->children()) {
        QSizeF childSize = calculateSubtreeSize(child, horizontal);

        if (horizontal) {
            childrenHeight += childSize.height() + m_verticalSpacing;
            childrenWidth = qMax(childrenWidth, childSize.width());
        } else {
            childrenWidth += childSize.width() + m_horizontalSpacing;
            childrenHeight = qMax(childrenHeight, childSize.height());
        }
    }

    if (horizontal) {
        childrenHeight -= m_verticalSpacing;
        return QSizeF(width + m_levelSpacing + childrenWidth,
                      qMax(height, childrenHeight));
    } else {
        childrenWidth -= m_horizontalSpacing;
        return QSizeF(qMax(width, childrenWidth),
                      height + m_levelSpacing + childrenHeight);
    }
}

// ==================== PDF 导出 ====================

bool MindMapView::exportToPdf(const QString &filePath)
{
    if (!m_rootItem) return false;

    QRectF sceneRect = m_scene->itemsBoundingRect().adjusted(-50, -50, 50, 50);

    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize(sceneRect.size().toSize(), QPageSize::Point));
    writer.setResolution(150);
    writer.setTitle(tr("思维导图"));

    QPainter painter(&writer);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    m_scene->render(&painter, QRectF(), sceneRect);
    painter.end();

    return true;
}

// ==================== 双向编辑 ====================

void MindMapView::setEditable(bool editable)
{
    m_editable = editable;
}

// 函数说明：启动 MindMapView 的异步任务、会话或后台流程。
void MindMapView::startEditingNode(MindMapNode *node)
{
    if (!m_editable || !node) return;

    finishEditingNode();  // 完成之前的编辑

    m_editingNode = node;

    // 创建编辑控件
    m_editWidget = new QLineEdit();
    m_editWidget->setText(node->text());
    m_editWidget->selectAll();
    m_editWidget->setMinimumWidth(100);

    // 设置样式
    m_editWidget->setStyleSheet(
        "QLineEdit { background: white; border: 2px solid #4A90D9; border-radius: 4px; padding: 4px; }"
    );

    // 创建代理
    m_editProxy = m_scene->addWidget(m_editWidget);
    m_editProxy->setPos(node->scenePos() - QPointF(node->boundingRect().width()/2, 15));
    m_editProxy->setZValue(1000);

    m_editWidget->setFocus();

    // 连接信号
    connect(m_editWidget, &QLineEdit::editingFinished, this, &MindMapView::finishEditingNode);
}

// 函数说明：实现 MindMapView::finishEditingNode 的核心逻辑，供当前模块调用。
void MindMapView::finishEditingNode()
{
    if (!m_editingNode || !m_editWidget) return;

    QString newText = m_editWidget->text().trimmed();
    QString oldText = m_editingNode->text();

    if (!newText.isEmpty() && newText != oldText) {
        int lineNumber = m_editingNode->lineNumber();
        m_editingNode->setText(newText);

        emit nodeTextChanged(lineNumber, oldText, newText);
        emit markdownChanged(generateMarkdown());
    }

    // 清理
    if (m_editProxy) {
        m_scene->removeItem(m_editProxy);
        delete m_editProxy;
        m_editProxy = nullptr;
    }
    m_editWidget = nullptr;
    m_editingNode = nullptr;

    relayout();
}

// 函数说明：向 MindMapView 管理的数据集合中添加一项内容。
void MindMapView::addChildNode(MindMapNode *parentNode, const QString &text)
{
    if (!m_editable || !parentNode) return;

    QString nodeText = text.isEmpty() ? tr("新节点") : text;
    int level = parentNode->level() + 1;

    // 创建新数据
    NodeData *newData = new NodeData();
    newData->text = nodeText;
    newData->level = level;
    newData->lineNumber = -1;  // 新节点
    newData->parent = parentNode->data();
    parentNode->data()->children.append(newData);

    // 创建图形项
    MindMapNode *newNode = new MindMapNode(newData, this, parentNode);
    parentNode->addChild(newNode);

    // 展开父节点
    if (parentNode->isCollapsed()) {
        parentNode->setCollapsed(false);
    }

    relayout();

    // 开始编辑
    startEditingNode(newNode);

    emit nodeAdded(parentNode->lineNumber(), level, nodeText);
    emit markdownChanged(generateMarkdown());
}

// 函数说明：向 MindMapView 管理的数据集合中添加一项内容。
void MindMapView::addSiblingNode(MindMapNode *node, const QString &text)
{
    if (!m_editable || !node || !node->parentNode()) return;

    MindMapNode *parentNode = node->parentNode();
    QString nodeText = text.isEmpty() ? tr("新节点") : text;
    int level = node->level();

    // 创建新数据
    NodeData *newData = new NodeData();
    newData->text = nodeText;
    newData->level = level;
    newData->lineNumber = -1;
    newData->parent = parentNode->data();

    // 在当前节点后插入
    int index = parentNode->data()->children.indexOf(node->data());
    if (index >= 0) {
        parentNode->data()->children.insert(index + 1, newData);
    } else {
        parentNode->data()->children.append(newData);
    }

    // 创建图形项
    MindMapNode *newNode = new MindMapNode(newData, this, parentNode);

    // 在当前节点后插入图形项
    int gIndex = parentNode->childrenRef().indexOf(node);
    if (gIndex >= 0) {
        parentNode->insertChild(gIndex + 1, newNode);
    } else {
        parentNode->addChild(newNode);
    }

    relayout();
    startEditingNode(newNode);

    emit nodeAdded(parentNode->lineNumber(), level, nodeText);
    emit markdownChanged(generateMarkdown());
}

// 函数说明：删除 MindMapView 管理的指定数据或资源。
void MindMapView::deleteNode(MindMapNode *node)
{
    if (!m_editable || !node || node->level() == 0) return;  // 不能删除根节点

    MindMapNode *parent = node->parentNode();
    if (!parent) return;

    int lineNumber = node->lineNumber();

    // 从父节点移除
    parent->data()->children.removeAll(node->data());
    parent->removeChild(node);

    // 删除数据和图形项
    delete node->data();
    m_scene->removeItem(node);
    delete node;

    if (m_selectedNode == node) {
        m_selectedNode = nullptr;
    }

    relayout();

    emit nodeDeleted(lineNumber);
    emit markdownChanged(generateMarkdown());
}

// 函数说明：根据当前数据生成 MindMapView 需要的输出结果。
QString MindMapView::generateMarkdown() const
{
    if (!m_rootData) return QString();

    QString markdown;

    std::function<void(const NodeData*, int)> generateNode = [&](const NodeData *node, int depth) {
        if (node->level > 0) {
            // 添加标题
            QString prefix = QString("#").repeated(node->level);
            markdown += QString("%1 %2\n\n").arg(prefix, node->text);
        }

        for (const NodeData *child : node->children) {
            generateNode(child, depth + 1);
        }
    };

    generateNode(m_rootData, 0);

    return markdown;
}

// ==================== MindMapNode ====================

MindMapNode::MindMapNode(MindMapView::NodeData *data, MindMapView *view, MindMapNode *parent)
    : QGraphicsItem(parent)
    , m_data(data)
    , m_view(view)
    , m_parentNode(parent)
    , m_selected(false)
    , m_hovered(false)
{
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable);

    // 计算节点大小
    QFontMetrics fm(m_view->theme().nodeFont);
    m_width = fm.horizontalAdvance(m_data->text) + 30;
    m_height = fm.height() + 16;
    m_width = qMax(m_width, 60.0);

    m_boundingRect = QRectF(-m_width / 2, -m_height / 2, m_width, m_height);
}

// 函数说明：销毁 MindMapNode 对象，释放本模块持有的资源。
MindMapNode::~MindMapNode()
{
}

// 函数说明：向 MindMapNode 管理的数据集合中添加一项内容。
void MindMapNode::addChild(MindMapNode *child)
{
    m_children.append(child);
}

// 函数说明：实现 MindMapNode::insertChild 的核心逻辑，供当前模块调用。
void MindMapNode::insertChild(int index, MindMapNode *child)
{
    if (index >= 0 && index <= m_children.size()) {
        m_children.insert(index, child);
    } else {
        m_children.append(child);
    }
}

// 函数说明：从 MindMapNode 管理的数据集合中移除指定内容。
void MindMapNode::removeChild(MindMapNode *child)
{
    m_children.removeAll(child);
}

// 函数说明：设置 MindMapNode 的运行参数，并触发必要的界面或数据刷新。
void MindMapNode::setText(const QString &text)
{
    m_data->text = text;

    // 重新计算节点大小
    QFontMetrics fm(m_view->theme().nodeFont);
    m_width = fm.horizontalAdvance(text) + 30;
    m_height = fm.height() + 16;
    m_width = qMax(m_width, 60.0);
    m_boundingRect = QRectF(-m_width / 2, -m_height / 2, m_width, m_height);

    update();
}

// 函数说明：设置 MindMapNode 的运行参数，并触发必要的界面或数据刷新。
void MindMapNode::setCollapsed(bool collapsed)
{
    m_data->collapsed = collapsed;

    for (MindMapNode *child : m_children) {
        child->setVisible(!collapsed);
    }
}

// 函数说明：切换 MindMapNode 对应功能的启用状态。
void MindMapNode::toggleCollapsed()
{
    setCollapsed(!m_data->collapsed);
}

// 函数说明：设置 MindMapNode 的运行参数，并触发必要的界面或数据刷新。
void MindMapNode::setSelected(bool selected)
{
    m_selected = selected;
    update();
}

// 函数说明：刷新 MindMapNode 的内部状态，并同步到相关界面。
void MindMapNode::updateConnections()
{
    for (MindMapNode *child : m_children) {
        if (child->isVisible()) {
            child->updateConnections();
        }
    }
    update();
}

// 函数说明：实现 MindMapNode::boundingRect 的核心逻辑，供当前模块调用。
QRectF MindMapNode::boundingRect() const
{
    return m_boundingRect.adjusted(-5, -5, 5, 5);
}

// 函数说明：绘制 MindMapNode 的可视区域或辅助标记。
void MindMapNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    const MindMapView::Theme &theme = m_view->theme();

    // 绘制连接线
    if (!m_data->collapsed) {
        for (MindMapNode *child : m_children) {
            if (child->isVisible()) {
                drawConnection(painter, child);
            }
        }
    }

    // 绘制节点背景
    QColor bgColor = theme.nodeColor;
    if (m_selected) {
        bgColor = theme.selectedColor;
    } else if (m_hovered) {
        bgColor = bgColor.lighter(120);
    }

    // 根据层级调整颜色
    if (m_data->level > 0) {
        int lightness = 100 + m_data->level * 10;
        bgColor = bgColor.lighter(lightness);
    }

    painter->setPen(Qt::NoPen);
    painter->setBrush(bgColor);
    painter->drawRoundedRect(m_boundingRect, theme.nodeRadius, theme.nodeRadius);

    // 绘制边框
    painter->setPen(QPen(bgColor.darker(120), 1));
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(m_boundingRect, theme.nodeRadius, theme.nodeRadius);

    // 绘制文本
    painter->setPen(theme.nodeTextColor);
    painter->setFont(theme.nodeFont);
    painter->drawText(m_boundingRect, Qt::AlignCenter, m_data->text);

    // 折叠指示器
    if (!m_children.isEmpty()) {
        QRectF indicatorRect(m_boundingRect.right() - 15, m_boundingRect.center().y() - 5, 10, 10);

        painter->setPen(QPen(theme.nodeTextColor, 1.5));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(indicatorRect);

        // + 或 -
        painter->drawLine(indicatorRect.center().x() - 3, indicatorRect.center().y(),
                          indicatorRect.center().x() + 3, indicatorRect.center().y());

        if (m_data->collapsed) {
            painter->drawLine(indicatorRect.center().x(), indicatorRect.center().y() - 3,
                              indicatorRect.center().x(), indicatorRect.center().y() + 3);
        }
    }
}

// 函数说明：实现 MindMapNode::drawConnection 的核心逻辑，供当前模块调用。
void MindMapNode::drawConnection(QPainter *painter, MindMapNode *child)
{
    const MindMapView::Theme &theme = m_view->theme();

    QPointF start = mapToScene(m_boundingRect.center());
    QPointF end = child->mapToScene(child->boundingRect().center());

    // 贝塞尔曲线
    QPainterPath path;
    path.moveTo(mapFromScene(start));

    QPointF mid1 = mapFromScene(QPointF((start.x() + end.x()) / 2, start.y()));
    QPointF mid2 = mapFromScene(QPointF((start.x() + end.x()) / 2, end.y()));
    QPointF endLocal = mapFromScene(end);

    path.cubicTo(mid1, mid2, endLocal);

    painter->setPen(QPen(theme.lineColor, theme.lineWidth, Qt::SolidLine, Qt::RoundCap));
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path);
}

// 函数说明：处理鼠标事件，更新选择、拖拽或交互状态。
void MindMapNode::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mousePressEvent(event);

    // 检查是否点击了折叠按钮
    QRectF indicatorRect(m_boundingRect.right() - 15, m_boundingRect.center().y() - 5, 10, 10);
    if (indicatorRect.contains(event->pos()) && !m_children.isEmpty()) {
        toggleCollapsed();
        m_view->relayout();
        return;
    }

    emit m_view->nodeClicked(m_data->lineNumber);
}

// 函数说明：处理鼠标事件，更新选择、拖拽或交互状态。
void MindMapNode::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mouseDoubleClickEvent(event);
    emit m_view->nodeDoubleClicked(m_data->lineNumber);
}

// 函数说明：实现 MindMapNode::hoverEnterEvent 的核心逻辑，供当前模块调用。
void MindMapNode::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
    m_hovered = true;
    update();
}

// 函数说明：实现 MindMapNode::hoverLeaveEvent 的核心逻辑，供当前模块调用。
void MindMapNode::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
    m_hovered = false;
    update();
}

