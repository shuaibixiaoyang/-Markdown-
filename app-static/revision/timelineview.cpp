// 文件说明：app-static\revision\timelineview.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "timelineview.h"

#include <QPainter>
#include <QPainterPath>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QToolTip>
#include <QScrollBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QFrame>
#include <QApplication>
#include <QtMath>
#include <QWheelEvent>

// ============================================================================
// RevisionNode 实现
// ============================================================================

RevisionNode::RevisionNode(const RevisionTracker::Revision &revision, QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_revision(revision)
    , m_isSelected(false)
    , m_compareMode(false)
    , m_isPrimaryCompare(false)
    , m_highlighted(false)
    , m_hovered(false)
    , m_branchIndex(0)
{
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setToolTip(tooltipText());
}

// 函数说明：实现 RevisionNode::boundingRect 的核心逻辑，供当前模块调用。
QRectF RevisionNode::boundingRect() const
{
    return QRectF(-NodeRadius - 2, -NodeRadius - 2,
                  (NodeRadius + 2) * 2, (NodeRadius + 2) * 2);
}

// 函数说明：绘制 RevisionNode 的可视区域或辅助标记。
void RevisionNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->setRenderHint(QPainter::Antialiasing);

    // 绘制外圈（选中或对比模式）
    if (m_isSelected || m_compareMode) {
        QPen outerPen;
        if (m_compareMode) {
            outerPen.setColor(m_isPrimaryCompare ? QColor(220, 50, 50) : QColor(50, 150, 50));
            outerPen.setWidth(4);
        } else {
            outerPen.setColor(QColor(0, 120, 215));
            outerPen.setWidth(3);
        }
        painter->setPen(outerPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(QPointF(0, 0), NodeRadius + 3, NodeRadius + 3);
    }

    // 绘制节点主体
    QColor color = nodeColor();
    if (m_hovered) {
        color = color.lighter(120);
    }

    painter->setPen(QPen(color.darker(130), 2));
    painter->setBrush(color);
    painter->drawEllipse(QPointF(0, 0), NodeRadius, NodeRadius);

    // 绘制图标（根据类型）
    painter->setPen(Qt::white);
    QFont font = painter->font();
    font.setPixelSize(12);
    font.setBold(true);
    painter->setFont(font);

    QString icon;
    if (m_revision.isAutoSave) {
        icon = "A";  // Auto
    } else if (!m_revision.tags.isEmpty()) {
        icon = "T";  // Tagged
    } else {
        icon = "M";  // Manual
    }
    painter->drawText(boundingRect(), Qt::AlignCenter, icon);

    // 绘制高亮效果
    if (m_highlighted) {
        painter->setPen(QPen(QColor(255, 200, 0), 3));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(QPointF(0, 0), NodeRadius + 6, NodeRadius + 6);
    }
}

// 函数说明：设置 RevisionNode 的运行参数，并触发必要的界面或数据刷新。
void RevisionNode::setSelected(bool selected)
{
    m_isSelected = selected;
    update();
}

// 函数说明：设置 RevisionNode 的运行参数，并触发必要的界面或数据刷新。
void RevisionNode::setCompareMode(bool enabled, bool isPrimary)
{
    m_compareMode = enabled;
    m_isPrimaryCompare = isPrimary;
    update();
}

// 函数说明：设置 RevisionNode 的运行参数，并触发必要的界面或数据刷新。
void RevisionNode::setHighlighted(bool highlighted)
{
    m_highlighted = highlighted;
    update();
}

// 函数说明：实现 RevisionNode::nodeColor 的核心逻辑，供当前模块调用。
QColor RevisionNode::nodeColor() const
{
    if (m_revision.isAutoSave) {
        return QColor(150, 150, 150);  // 灰色 - 自动保存
    } else if (!m_revision.tags.isEmpty()) {
        return QColor(255, 165, 0);    // 橙色 - 有标签
    } else {
        return QColor(70, 130, 180);   // 钢蓝色 - 手动保存
    }
}

// 函数说明：实现 RevisionNode::tooltipText 的核心逻辑，供当前模块调用。
QString RevisionNode::tooltipText() const
{
    QString tooltip;
    tooltip += QString("<b>%1</b><br>").arg(m_revision.description.isEmpty()
                                             ? tr("修订版本") : m_revision.description);
    tooltip += QString("<br><b>时间:</b> %1").arg(m_revision.timestamp.toString("yyyy-MM-dd hh:mm:ss"));
    tooltip += QString("<br><b>作者:</b> %1").arg(m_revision.author.isEmpty()
                                                    ? tr("未知") : m_revision.author);
    tooltip += QString("<br><b>字数:</b> %1").arg(m_revision.wordCount);
    tooltip += QString("<br><b>行数:</b> %1").arg(m_revision.lineCount);

    if (!m_revision.tags.isEmpty()) {
        tooltip += QString("<br><b>标签:</b> %1").arg(m_revision.tags.join(", "));
    }

    if (m_revision.isAutoSave) {
        tooltip += QString("<br><i>(自动保存)</i>");
    }

    return tooltip;
}

// 函数说明：处理鼠标事件，更新选择、拖拽或交互状态。
void RevisionNode::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_revision.id);
    }
    QGraphicsObject::mousePressEvent(event);
}

// 函数说明：处理鼠标事件，更新选择、拖拽或交互状态。
void RevisionNode::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit doubleClicked(m_revision.id);
    }
    QGraphicsObject::mouseDoubleClickEvent(event);
}

// 函数说明：实现 RevisionNode::contextMenuEvent 的核心逻辑，供当前模块调用。
void RevisionNode::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    emit contextMenuRequested(m_revision.id, event->scenePos());
    event->accept();
}

// 函数说明：实现 RevisionNode::hoverEnterEvent 的核心逻辑，供当前模块调用。
void RevisionNode::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    m_hovered = true;
    update();
    QGraphicsObject::hoverEnterEvent(event);
}

// 函数说明：实现 RevisionNode::hoverLeaveEvent 的核心逻辑，供当前模块调用。
void RevisionNode::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    m_hovered = false;
    update();
    QGraphicsObject::hoverLeaveEvent(event);
}

// ============================================================================
// RevisionEdge 实现
// ============================================================================

RevisionEdge::RevisionEdge(RevisionNode *source, RevisionNode *target, QGraphicsItem *parent)
    : QGraphicsItem(parent)
    , m_source(source)
    , m_target(target)
    , m_highlighted(false)
{
    setZValue(-1);  // 在节点下方
}

// 函数说明：实现 RevisionEdge::boundingRect 的核心逻辑，供当前模块调用。
QRectF RevisionEdge::boundingRect() const
{
    if (!m_source || !m_target) return QRectF();

    QPointF p1 = m_source->scenePos();
    QPointF p2 = m_target->scenePos();

    return QRectF(qMin(p1.x(), p2.x()) - 10, qMin(p1.y(), p2.y()) - 10,
                  qAbs(p2.x() - p1.x()) + 20, qAbs(p2.y() - p1.y()) + 20);
}

// 函数说明：绘制 RevisionEdge 的可视区域或辅助标记。
void RevisionEdge::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    if (!m_source || !m_target) return;

    painter->setRenderHint(QPainter::Antialiasing);

    QPointF p1 = m_source->scenePos();
    QPointF p2 = m_target->scenePos();

    // 计算从节点边缘开始的点
    QLineF line(p1, p2);
    qreal angle = qAtan2(p2.y() - p1.y(), p2.x() - p1.x());

    QPointF start(p1.x() + RevisionNode::NodeRadius * qCos(angle),
                  p1.y() + RevisionNode::NodeRadius * qSin(angle));
    QPointF end(p2.x() - RevisionNode::NodeRadius * qCos(angle),
                p2.y() - RevisionNode::NodeRadius * qSin(angle));

    // 绘制连接线
    QPen pen;
    if (m_highlighted) {
        pen.setColor(QColor(0, 120, 215));
        pen.setWidth(3);
    } else {
        pen.setColor(QColor(180, 180, 180));
        pen.setWidth(2);
    }
    painter->setPen(pen);

    // 如果是分支，使用曲线
    if (m_source->branchIndex() != m_target->branchIndex()) {
        QPainterPath path;
        path.moveTo(start);
        QPointF ctrl1(start.x() + (end.x() - start.x()) * 0.5, start.y());
        QPointF ctrl2(start.x() + (end.x() - start.x()) * 0.5, end.y());
        path.cubicTo(ctrl1, ctrl2, end);
        painter->drawPath(path);
    } else {
        painter->drawLine(start, end);
    }

    // 绘制箭头
    qreal arrowSize = 8;
    QPointF arrowP1 = end - QPointF(qCos(angle - M_PI/6) * arrowSize,
                                     qSin(angle - M_PI/6) * arrowSize);
    QPointF arrowP2 = end - QPointF(qCos(angle + M_PI/6) * arrowSize,
                                     qSin(angle + M_PI/6) * arrowSize);

    painter->setBrush(pen.color());
    QPolygonF arrow;
    arrow << end << arrowP1 << arrowP2;
    painter->drawPolygon(arrow);
}

// 函数说明：刷新 RevisionEdge 的内部状态，并同步到相关界面。
void RevisionEdge::updatePosition()
{
    prepareGeometryChange();
}

// ============================================================================
// TimelineScene 实现
// ============================================================================

TimelineScene::TimelineScene(QObject *parent)
    : QGraphicsScene(parent)
    , m_compareMode(false)
    , m_maxBranchIndex(0)
{
}

// 函数说明：销毁 TimelineScene 对象，释放本模块持有的资源。
TimelineScene::~TimelineScene()
{
    clear();
}

// 函数说明：设置 TimelineScene 的运行参数，并触发必要的界面或数据刷新。
void TimelineScene::setRevisions(const QVector<RevisionTracker::Revision> &revisions)
{
    clear();
    m_revisions = revisions;

    if (m_revisions.isEmpty()) return;

    // 检测分支
    detectBranches();

    // 创建节点
    for (const RevisionTracker::Revision &rev : m_revisions) {
        RevisionNode *node = new RevisionNode(rev);
        node->setBranchIndex(m_branchIndices.value(rev.id, 0));

        connect(node, &RevisionNode::clicked, this, &TimelineScene::onNodeClicked);
        connect(node, &RevisionNode::doubleClicked, this, &TimelineScene::onNodeDoubleClicked);
        connect(node, &RevisionNode::contextMenuRequested, this, &TimelineScene::onNodeContextMenu);

        addItem(node);
        m_nodes[rev.id] = node;
    }

    // 布局节点
    layoutNodes();

    // 创建边
    createEdges();

    // 设置场景大小
    QRectF bounds = itemsBoundingRect();
    setSceneRect(bounds.adjusted(-50, -50, 50, 50));
}

// 函数说明：清空 TimelineScene 保存的临时状态或缓存数据。
void TimelineScene::clear()
{
    for (RevisionEdge *edge : m_edges) {
        removeItem(edge);
        delete edge;
    }
    m_edges.clear();

    for (RevisionNode *node : m_nodes) {
        removeItem(node);
        delete node;
    }
    m_nodes.clear();

    m_revisions.clear();
    m_branchIndices.clear();
    m_maxBranchIndex = 0;
    m_firstCompareId.clear();
    m_secondCompareId.clear();
    m_selectedId.clear();
}

// 函数说明：实现 TimelineScene::selectNode 的核心逻辑，供当前模块调用。
void TimelineScene::selectNode(const QString &revisionId)
{
    // 清除之前的选择
    if (!m_selectedId.isEmpty() && m_nodes.contains(m_selectedId)) {
        m_nodes[m_selectedId]->setSelected(false);
    }

    m_selectedId = revisionId;

    // 选中新节点
    if (m_nodes.contains(revisionId)) {
        m_nodes[revisionId]->setSelected(true);
    }

    emit nodeClicked(revisionId);
}

// 函数说明：清空 TimelineScene 保存的临时状态或缓存数据。
void TimelineScene::clearSelection()
{
    if (!m_selectedId.isEmpty() && m_nodes.contains(m_selectedId)) {
        m_nodes[m_selectedId]->setSelected(false);
    }
    m_selectedId.clear();
}

// 函数说明：设置 TimelineScene 的运行参数，并触发必要的界面或数据刷新。
void TimelineScene::setCompareMode(bool enabled)
{
    m_compareMode = enabled;

    if (!enabled) {
        // 清除对比模式
        for (RevisionNode *node : m_nodes) {
            node->setCompareMode(false);
        }
        m_firstCompareId.clear();
        m_secondCompareId.clear();
    }
}

// 函数说明：设置 TimelineScene 的运行参数，并触发必要的界面或数据刷新。
void TimelineScene::setFirstCompareRevision(const QString &revisionId)
{
    // 清除之前的第一个对比
    if (!m_firstCompareId.isEmpty() && m_nodes.contains(m_firstCompareId)) {
        m_nodes[m_firstCompareId]->setCompareMode(false);
    }

    m_firstCompareId = revisionId;

    if (m_nodes.contains(revisionId)) {
        m_nodes[revisionId]->setCompareMode(true, true);
    }

    // 检查是否可以开始对比
    if (!m_firstCompareId.isEmpty() && !m_secondCompareId.isEmpty()) {
        emit compareRevisionsRequested(m_firstCompareId, m_secondCompareId);
    }
}

// 函数说明：设置 TimelineScene 的运行参数，并触发必要的界面或数据刷新。
void TimelineScene::setSecondCompareRevision(const QString &revisionId)
{
    // 清除之前的第二个对比
    if (!m_secondCompareId.isEmpty() && m_nodes.contains(m_secondCompareId)) {
        m_nodes[m_secondCompareId]->setCompareMode(false);
    }

    m_secondCompareId = revisionId;

    if (m_nodes.contains(revisionId)) {
        m_nodes[revisionId]->setCompareMode(true, false);
    }

    // 检查是否可以开始对比
    if (!m_firstCompareId.isEmpty() && !m_secondCompareId.isEmpty()) {
        emit compareRevisionsRequested(m_firstCompareId, m_secondCompareId);
    }
}

// 函数说明：实现 TimelineScene::nodeById 的核心逻辑，供当前模块调用。
RevisionNode* TimelineScene::nodeById(const QString &revisionId) const
{
    return m_nodes.value(revisionId, nullptr);
}

// 函数说明：响应 TimelineScene 收到的信号或异步回调，并更新界面状态。
void TimelineScene::onNodeClicked(const QString &revisionId)
{
    if (m_compareMode) {
        // 对比模式下选择节点
        if (m_firstCompareId.isEmpty()) {
            setFirstCompareRevision(revisionId);
        } else if (m_secondCompareId.isEmpty() && revisionId != m_firstCompareId) {
            setSecondCompareRevision(revisionId);
        } else {
            // 重新选择
            setFirstCompareRevision(revisionId);
            setSecondCompareRevision(QString());
        }
    } else {
        selectNode(revisionId);
    }
}

// 函数说明：响应 TimelineScene 收到的信号或异步回调，并更新界面状态。
void TimelineScene::onNodeDoubleClicked(const QString &revisionId)
{
    emit nodeDoubleClicked(revisionId);
}

// 函数说明：响应 TimelineScene 收到的信号或异步回调，并更新界面状态。
void TimelineScene::onNodeContextMenu(const QString &revisionId, const QPointF &scenePos)
{
    QPoint globalPos;
    for (QGraphicsView *view : views()) {
        globalPos = view->mapToGlobal(view->mapFromScene(scenePos));
        break;
    }
    emit nodeContextMenu(revisionId, globalPos);
}

// 函数说明：实现 TimelineScene::layoutNodes 的核心逻辑，供当前模块调用。
void TimelineScene::layoutNodes()
{
    if (m_revisions.isEmpty()) return;

    // 按时间排序（从旧到新）
    QVector<RevisionTracker::Revision> sorted = m_revisions;
    std::sort(sorted.begin(), sorted.end(),
              [](const RevisionTracker::Revision &a, const RevisionTracker::Revision &b) {
                  return a.timestamp < b.timestamp;
              });

    // 布局：X 轴表示时间，Y 轴表示分支
    qreal x = 0;
    qreal branchSpacing = 60;

    for (const RevisionTracker::Revision &rev : sorted) {
        if (m_nodes.contains(rev.id)) {
            RevisionNode *node = m_nodes[rev.id];
            int branchIndex = m_branchIndices.value(rev.id, 0);
            qreal y = branchIndex * branchSpacing;

            node->setPos(x, y);
            x += RevisionNode::NodeSpacing;
        }
    }
}

// 函数说明：创建 TimelineScene 需要的对象、记录或输出内容。
void TimelineScene::createEdges()
{
    for (const RevisionTracker::Revision &rev : m_revisions) {
        if (!rev.parentId.isEmpty() && m_nodes.contains(rev.parentId) && m_nodes.contains(rev.id)) {
            RevisionNode *parent = m_nodes[rev.parentId];
            RevisionNode *child = m_nodes[rev.id];

            RevisionEdge *edge = new RevisionEdge(parent, child);
            addItem(edge);
            m_edges.append(edge);
        }
    }
}

// 函数说明：实现 TimelineScene::detectBranches 的核心逻辑，供当前模块调用。
void TimelineScene::detectBranches()
{
    m_branchIndices.clear();
    m_maxBranchIndex = 0;

    // 构建子节点映射
    QMap<QString, QStringList> children;  // parentId -> childIds
    for (const RevisionTracker::Revision &rev : m_revisions) {
        if (!rev.parentId.isEmpty()) {
            children[rev.parentId].append(rev.id);
        }
    }

    // 找到根节点（没有父节点的）
    QStringList roots;
    for (const RevisionTracker::Revision &rev : m_revisions) {
        if (rev.parentId.isEmpty()) {
            roots.append(rev.id);
        }
    }

    // 分配分支索引
    for (const QString &rootId : roots) {
        m_branchIndices[rootId] = 0;
    }

    // BFS 遍历分配分支
    QVector<RevisionTracker::Revision> sorted = m_revisions;
    std::sort(sorted.begin(), sorted.end(),
              [](const RevisionTracker::Revision &a, const RevisionTracker::Revision &b) {
                  return a.timestamp < b.timestamp;
              });

    for (const RevisionTracker::Revision &rev : sorted) {
        if (m_branchIndices.contains(rev.id)) continue;

        if (!rev.parentId.isEmpty() && m_branchIndices.contains(rev.parentId)) {
            QStringList siblings = children[rev.parentId];
            if (siblings.size() == 1) {
                // 唯一的子节点，继承父节点的分支
                m_branchIndices[rev.id] = m_branchIndices[rev.parentId];
            } else {
                // 多个子节点，创建新分支
                int parentBranch = m_branchIndices[rev.parentId];
                int myIndex = siblings.indexOf(rev.id);
                if (myIndex == 0) {
                    m_branchIndices[rev.id] = parentBranch;
                } else {
                    m_maxBranchIndex++;
                    m_branchIndices[rev.id] = m_maxBranchIndex;
                }
            }
        } else {
            m_branchIndices[rev.id] = 0;
        }
    }
}

// ============================================================================
// TimelineView 实现
// ============================================================================

TimelineView::TimelineView(QWidget *parent)
    : QWidget(parent)
    , m_tracker(nullptr)
    , m_compareMode(false)
{
    setupUi();
}

// 函数说明：销毁 TimelineView 对象，释放本模块持有的资源。
TimelineView::~TimelineView()
{
}

// 函数说明：初始化 TimelineView 的 setupUi 相关界面、动作或服务连接。
void TimelineView::setupUi()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    setupToolbar();

    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainLayout->addWidget(m_mainSplitter);

    setupTimelineView();

    m_rightSplitter = new QSplitter(Qt::Vertical, this);
    m_mainSplitter->addWidget(m_rightSplitter);

    setupDetailPanel();
    setupCompareView();

    // 设置分割比例
    m_mainSplitter->setSizes({600, 400});
    m_rightSplitter->setSizes({200, 300});
}

// 函数说明：初始化 TimelineView 的 setupToolbar 相关界面、动作或服务连接。
void TimelineView::setupToolbar()
{
    m_toolbar = new QWidget(this);
    QHBoxLayout *toolbarLayout = new QHBoxLayout(m_toolbar);
    toolbarLayout->setContentsMargins(5, 5, 5, 5);
    toolbarLayout->setSpacing(5);

    m_refreshButton = new QPushButton(tr("刷新"), m_toolbar);
    m_refreshButton->setToolTip(tr("刷新时间线"));
    connect(m_refreshButton, &QPushButton::clicked, this, &TimelineView::refresh);
    toolbarLayout->addWidget(m_refreshButton);

    toolbarLayout->addSpacing(10);

    m_compareModeButton = new QPushButton(tr("对比模式"), m_toolbar);
    m_compareModeButton->setCheckable(true);
    m_compareModeButton->setToolTip(tr("启用对比模式，选择两个版本进行对比"));
    connect(m_compareModeButton, &QPushButton::toggled, this, [this](bool checked) {
        if (checked) enterCompareMode();
        else exitCompareMode();
    });
    toolbarLayout->addWidget(m_compareModeButton);

    m_compareCurrentButton = new QPushButton(tr("与当前对比"), m_toolbar);
    m_compareCurrentButton->setToolTip(tr("将选中版本与当前文档对比"));
    connect(m_compareCurrentButton, &QPushButton::clicked, this, &TimelineView::compareWithCurrent);
    toolbarLayout->addWidget(m_compareCurrentButton);

    toolbarLayout->addStretch();

    m_zoomInButton = new QPushButton("+", m_toolbar);
    m_zoomInButton->setFixedWidth(30);
    m_zoomInButton->setToolTip(tr("放大"));
    connect(m_zoomInButton, &QPushButton::clicked, this, &TimelineView::zoomIn);
    toolbarLayout->addWidget(m_zoomInButton);

    m_zoomOutButton = new QPushButton("-", m_toolbar);
    m_zoomOutButton->setFixedWidth(30);
    m_zoomOutButton->setToolTip(tr("缩小"));
    connect(m_zoomOutButton, &QPushButton::clicked, this, &TimelineView::zoomOut);
    toolbarLayout->addWidget(m_zoomOutButton);

    m_fitButton = new QPushButton(tr("适应"), m_toolbar);
    m_fitButton->setToolTip(tr("适应窗口大小"));
    connect(m_fitButton, &QPushButton::clicked, this, &TimelineView::fitInView);
    toolbarLayout->addWidget(m_fitButton);

    toolbarLayout->addSpacing(10);

    m_statusLabel = new QLabel(m_toolbar);
    toolbarLayout->addWidget(m_statusLabel);

    m_mainLayout->addWidget(m_toolbar);
}

// 函数说明：初始化 TimelineView 的 setupTimelineView 相关界面、动作或服务连接。
void TimelineView::setupTimelineView()
{
    m_scene = new TimelineScene(this);
    m_graphicsView = new QGraphicsView(m_scene, this);
    m_graphicsView->setRenderHint(QPainter::Antialiasing);
    m_graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
    m_graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    m_graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    m_graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_graphicsView->setBackgroundBrush(QBrush(QColor(250, 250, 250)));

    connect(m_scene, &TimelineScene::nodeClicked, this, &TimelineView::onNodeClicked);
    connect(m_scene, &TimelineScene::nodeDoubleClicked, this, &TimelineView::onNodeDoubleClicked);
    connect(m_scene, &TimelineScene::compareRevisionsRequested, this, &TimelineView::onCompareRevisionsRequested);
    connect(m_scene, &TimelineScene::nodeContextMenu, this, &TimelineView::onNodeContextMenu);

    m_mainSplitter->addWidget(m_graphicsView);
}

// 函数说明：初始化 TimelineView 的 setupDetailPanel 相关界面、动作或服务连接。
void TimelineView::setupDetailPanel()
{
    m_detailPanel = new QWidget(this);
    QVBoxLayout *detailLayout = new QVBoxLayout(m_detailPanel);
    detailLayout->setContentsMargins(10, 10, 10, 10);

    QLabel *titleLabel = new QLabel(tr("<b>修订详情</b>"), m_detailPanel);
    detailLayout->addWidget(titleLabel);

    QFrame *line = new QFrame(m_detailPanel);
    line->setFrameShape(QFrame::HLine);
    detailLayout->addWidget(line);

    m_revisionIdLabel = new QLabel(m_detailPanel);
    m_revisionIdLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    detailLayout->addWidget(m_revisionIdLabel);

    m_timestampLabel = new QLabel(m_detailPanel);
    detailLayout->addWidget(m_timestampLabel);

    m_authorLabel = new QLabel(m_detailPanel);
    detailLayout->addWidget(m_authorLabel);

    m_descriptionLabel = new QLabel(m_detailPanel);
    m_descriptionLabel->setWordWrap(true);
    detailLayout->addWidget(m_descriptionLabel);

    m_statsLabel = new QLabel(m_detailPanel);
    detailLayout->addWidget(m_statsLabel);

    m_tagsLabel = new QLabel(m_detailPanel);
    m_tagsLabel->setWordWrap(true);
    detailLayout->addWidget(m_tagsLabel);

    detailLayout->addStretch();

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_restoreButton = new QPushButton(tr("恢复此版本"), m_detailPanel);
    m_restoreButton->setEnabled(false);
    connect(m_restoreButton, &QPushButton::clicked, this, &TimelineView::onRestoreButtonClicked);
    buttonLayout->addWidget(m_restoreButton);

    m_compareButton = new QPushButton(tr("选择对比"), m_detailPanel);
    m_compareButton->setEnabled(false);
    connect(m_compareButton, &QPushButton::clicked, this, &TimelineView::onCompareButtonClicked);
    buttonLayout->addWidget(m_compareButton);

    m_exportButton = new QPushButton(tr("导出"), m_detailPanel);
    m_exportButton->setEnabled(false);
    connect(m_exportButton, &QPushButton::clicked, this, &TimelineView::onExportButtonClicked);
    buttonLayout->addWidget(m_exportButton);

    detailLayout->addLayout(buttonLayout);

    m_rightSplitter->addWidget(m_detailPanel);
}

// 函数说明：初始化 TimelineView 的 setupCompareView 相关界面、动作或服务连接。
void TimelineView::setupCompareView()
{
    m_comparePanel = new QWidget(this);
    QVBoxLayout *compareLayout = new QVBoxLayout(m_comparePanel);
    compareLayout->setContentsMargins(10, 10, 10, 10);

    QLabel *compareTitleLabel = new QLabel(tr("<b>版本对比</b>"), m_comparePanel);
    compareLayout->addWidget(compareTitleLabel);

    QFrame *line = new QFrame(m_comparePanel);
    line->setFrameShape(QFrame::HLine);
    compareLayout->addWidget(line);

    QHBoxLayout *selectorLayout = new QHBoxLayout();

    selectorLayout->addWidget(new QLabel(tr("旧版本:"), m_comparePanel));
    m_oldRevisionCombo = new QComboBox(m_comparePanel);
    selectorLayout->addWidget(m_oldRevisionCombo);

    selectorLayout->addWidget(new QLabel(tr("新版本:"), m_comparePanel));
    m_newRevisionCombo = new QComboBox(m_comparePanel);
    selectorLayout->addWidget(m_newRevisionCombo);

    m_doCompareButton = new QPushButton(tr("对比"), m_comparePanel);
    connect(m_doCompareButton, &QPushButton::clicked, this, [this]() {
        QString oldId = m_oldRevisionCombo->currentData().toString();
        QString newId = m_newRevisionCombo->currentData().toString();
        if (!oldId.isEmpty() && !newId.isEmpty()) {
            showCompareResult(oldId, newId);
        }
    });
    selectorLayout->addWidget(m_doCompareButton);

    compareLayout->addLayout(selectorLayout);

    m_compareStatusLabel = new QLabel(m_comparePanel);
    compareLayout->addWidget(m_compareStatusLabel);

    m_diffView = new QWebEngineView(m_comparePanel);
    m_diffView->setMinimumHeight(200);
    compareLayout->addWidget(m_diffView, 1);

    m_rightSplitter->addWidget(m_comparePanel);
}

// 函数说明：设置 TimelineView 的运行参数，并触发必要的界面或数据刷新。
void TimelineView::setRevisionTracker(RevisionTracker *tracker)
{
    m_tracker = tracker;
    updateRevisions();
}

// 函数说明：设置 TimelineView 的运行参数，并触发必要的界面或数据刷新。
void TimelineView::setDocumentPath(const QString &path)
{
    m_documentPath = path;
    updateRevisions();
}

// 函数说明：实现 TimelineView::refresh 的核心逻辑，供当前模块调用。
void TimelineView::refresh()
{
    updateRevisions();
}

// 函数说明：设置 TimelineView 的运行参数，并触发必要的界面或数据刷新。
void TimelineView::setCurrentContent(const QString &content)
{
    m_currentContent = content;
}

// 函数说明：刷新 TimelineView 的内部状态，并同步到相关界面。
void TimelineView::updateRevisions()
{
    if (!m_tracker || m_documentPath.isEmpty()) {
        m_scene->clear();
        m_statusLabel->setText(tr("无修订记录"));
        return;
    }

    // 加载修订
    m_tracker->loadRevisions(m_documentPath);
    QVector<RevisionTracker::Revision> revisions = m_tracker->getRevisions(m_documentPath);

    m_scene->setRevisions(revisions);

    // 更新下拉框
    m_oldRevisionCombo->clear();
    m_newRevisionCombo->clear();

    for (const RevisionTracker::Revision &rev : revisions) {
        QString label = QString("%1 - %2")
                        .arg(rev.timestamp.toString("MM-dd hh:mm"))
                        .arg(rev.description.isEmpty() ? tr("无描述") : rev.description.left(20));
        m_oldRevisionCombo->addItem(label, rev.id);
        m_newRevisionCombo->addItem(label, rev.id);
    }

    // 添加"当前文档"选项
    m_newRevisionCombo->addItem(tr("当前文档"), "current");

    m_statusLabel->setText(tr("共 %1 个修订版本").arg(revisions.size()));

    fitInView();
}

// 函数说明：刷新 TimelineView 的内部状态，并同步到相关界面。
void TimelineView::updateDetailPanel(const QString &revisionId)
{
    if (!m_tracker || revisionId.isEmpty()) {
        m_revisionIdLabel->setText(tr("ID: -"));
        m_timestampLabel->setText(tr("时间: -"));
        m_authorLabel->setText(tr("作者: -"));
        m_descriptionLabel->setText(tr("描述: -"));
        m_statsLabel->setText(tr("统计: -"));
        m_tagsLabel->setText(tr("标签: -"));
        m_restoreButton->setEnabled(false);
        m_compareButton->setEnabled(false);
        m_exportButton->setEnabled(false);
        return;
    }

    RevisionTracker::Revision rev = m_tracker->getRevision(revisionId);

    m_revisionIdLabel->setText(tr("ID: %1").arg(rev.id));
    m_timestampLabel->setText(tr("时间: %1").arg(rev.timestamp.toString("yyyy-MM-dd hh:mm:ss")));
    m_authorLabel->setText(tr("作者: %1").arg(rev.author.isEmpty() ? tr("未知") : rev.author));
    m_descriptionLabel->setText(tr("描述: %1").arg(rev.description.isEmpty() ? tr("无") : rev.description));
    m_statsLabel->setText(tr("统计: %1 字, %2 行").arg(rev.wordCount).arg(rev.lineCount));
    m_tagsLabel->setText(tr("标签: %1").arg(rev.tags.isEmpty() ? tr("无") : rev.tags.join(", ")));

    m_restoreButton->setEnabled(true);
    m_compareButton->setEnabled(true);
    m_exportButton->setEnabled(true);
}

// 函数说明：显示 TimelineView 管理的面板、对话框或提示信息。
void TimelineView::showCompareResult(const QString &oldId, const QString &newId)
{
    if (!m_tracker) return;

    QString diffHtml;

    if (newId == "current") {
        // 与当前文档对比
        QVector<RevisionTracker::DiffBlock> diffs = m_tracker->compareWithCurrent(oldId, m_currentContent);
        diffHtml = m_tracker->generateDiffHtml(diffs);
        m_compareStatusLabel->setText(tr("对比: 修订 %1 与 当前文档").arg(oldId.left(8)));
    } else {
        QVector<RevisionTracker::DiffBlock> diffs = m_tracker->compareRevisions(oldId, newId);
        diffHtml = m_tracker->generateDiffHtml(diffs);
        m_compareStatusLabel->setText(tr("对比: 修订 %1 与 %2").arg(oldId.left(8), newId.left(8)));
    }

    m_diffView->setHtml(diffHtml);

    emit compareRequested(oldId, newId);
}

// 函数说明：实现 TimelineView::enterCompareMode 的核心逻辑，供当前模块调用。
void TimelineView::enterCompareMode()
{
    m_compareMode = true;
    m_scene->setCompareMode(true);
    m_statusLabel->setText(tr("对比模式: 请选择第一个版本"));
}

// 函数说明：实现 TimelineView::exitCompareMode 的核心逻辑，供当前模块调用。
void TimelineView::exitCompareMode()
{
    m_compareMode = false;
    m_scene->setCompareMode(false);
    m_compareModeButton->setChecked(false);
    m_statusLabel->setText(tr("共 %1 个修订版本").arg(m_scene->allNodes().size()));
}

// 函数说明：实现 TimelineView::compareWithCurrent 的核心逻辑，供当前模块调用。
void TimelineView::compareWithCurrent()
{
    if (m_selectedRevisionId.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请先选择一个修订版本"));
        return;
    }

    showCompareResult(m_selectedRevisionId, "current");
}

// 函数说明：实现 TimelineView::zoomIn 的核心逻辑，供当前模块调用。
void TimelineView::zoomIn()
{
    m_graphicsView->scale(1.2, 1.2);
}

// 函数说明：实现 TimelineView::zoomOut 的核心逻辑，供当前模块调用。
void TimelineView::zoomOut()
{
    m_graphicsView->scale(1/1.2, 1/1.2);
}

// 函数说明：实现 TimelineView::fitInView 的核心逻辑，供当前模块调用。
void TimelineView::fitInView()
{
    if (m_scene->items().isEmpty()) return;
    m_graphicsView->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

// 函数说明：响应 TimelineView 收到的信号或异步回调，并更新界面状态。
void TimelineView::onNodeClicked(const QString &revisionId)
{
    m_selectedRevisionId = revisionId;
    updateDetailPanel(revisionId);
    emit revisionSelected(revisionId);

    if (m_compareMode) {
        if (m_scene->firstCompareRevision().isEmpty()) {
            m_statusLabel->setText(tr("对比模式: 已选择第一个版本，请选择第二个"));
        } else if (m_scene->secondCompareRevision().isEmpty()) {
            m_statusLabel->setText(tr("对比模式: 请点击对比查看结果"));
        }
    }
}

// 函数说明：响应 TimelineView 收到的信号或异步回调，并更新界面状态。
void TimelineView::onNodeDoubleClicked(const QString &revisionId)
{
    // 双击恢复版本
    onRestoreRevisionRequested(revisionId);
}

// 函数说明：响应 TimelineView 收到的信号或异步回调，并更新界面状态。
void TimelineView::onCompareRevisionsRequested(const QString &oldId, const QString &newId)
{
    showCompareResult(oldId, newId);
}

// 函数说明：响应 TimelineView 收到的信号或异步回调，并更新界面状态。
void TimelineView::onRestoreRevisionRequested(const QString &revisionId)
{
    if (!m_tracker) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("确认恢复"),
        tr("确定要恢复到修订版本 %1 吗？\n\n当前未保存的更改将会丢失。").arg(revisionId.left(8)),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        emit revisionRestoreRequested(revisionId);
    }
}

// 函数说明：响应 TimelineView 收到的信号或异步回调，并更新界面状态。
void TimelineView::onNodeContextMenu(const QString &revisionId, const QPoint &globalPos)
{
    QMenu menu(this);

    QAction *viewAction = menu.addAction(tr("查看详情"));
    connect(viewAction, &QAction::triggered, this, [this, revisionId]() {
        m_scene->selectNode(revisionId);
        onNodeClicked(revisionId);
    });

    QAction *restoreAction = menu.addAction(tr("恢复此版本"));
    connect(restoreAction, &QAction::triggered, this, [this, revisionId]() {
        onRestoreRevisionRequested(revisionId);
    });

    menu.addSeparator();

    QAction *compareOldAction = menu.addAction(tr("设为对比旧版本"));
    connect(compareOldAction, &QAction::triggered, this, [this, revisionId]() {
        int index = m_oldRevisionCombo->findData(revisionId);
        if (index >= 0) m_oldRevisionCombo->setCurrentIndex(index);
    });

    QAction *compareNewAction = menu.addAction(tr("设为对比新版本"));
    connect(compareNewAction, &QAction::triggered, this, [this, revisionId]() {
        int index = m_newRevisionCombo->findData(revisionId);
        if (index >= 0) m_newRevisionCombo->setCurrentIndex(index);
    });

    QAction *compareCurrentAction = menu.addAction(tr("与当前文档对比"));
    connect(compareCurrentAction, &QAction::triggered, this, [this, revisionId]() {
        showCompareResult(revisionId, "current");
    });

    menu.addSeparator();

    QAction *tagAction = menu.addAction(tr("添加标签..."));
    connect(tagAction, &QAction::triggered, this, [this, revisionId]() {
        if (!m_tracker) return;
        bool ok;
        QString tag = QInputDialog::getText(this, tr("添加标签"), tr("标签名称:"),
                                            QLineEdit::Normal, QString(), &ok);
        if (ok && !tag.isEmpty()) {
            m_tracker->addRevisionTag(revisionId, tag);
            updateDetailPanel(revisionId);
        }
    });

    menu.exec(globalPos);
}

// 函数说明：响应 TimelineView 收到的信号或异步回调，并更新界面状态。
void TimelineView::onCompareButtonClicked()
{
    if (m_selectedRevisionId.isEmpty()) return;

    // 将选中的版本设为旧版本
    int index = m_oldRevisionCombo->findData(m_selectedRevisionId);
    if (index >= 0) {
        m_oldRevisionCombo->setCurrentIndex(index);
    }

    // 新版本设为当前文档
    index = m_newRevisionCombo->findData("current");
    if (index >= 0) {
        m_newRevisionCombo->setCurrentIndex(index);
    }

    showCompareResult(m_selectedRevisionId, "current");
}

// 函数说明：响应 TimelineView 收到的信号或异步回调，并更新界面状态。
void TimelineView::onRestoreButtonClicked()
{
    if (!m_selectedRevisionId.isEmpty()) {
        onRestoreRevisionRequested(m_selectedRevisionId);
    }
}

// 函数说明：响应 TimelineView 收到的信号或异步回调，并更新界面状态。
void TimelineView::onExportButtonClicked()
{
    if (!m_tracker || m_selectedRevisionId.isEmpty()) return;

    QString content = m_tracker->getRevisionContent(m_selectedRevisionId);
    if (content.isEmpty()) return;

    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("导出修订版本"),
        QString(),
        tr("Markdown 文件 (*.md);;文本文件 (*.txt);;所有文件 (*)")
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(content.toUtf8());
        file.close();
        QMessageBox::information(this, tr("导出成功"),
                                 tr("修订版本已导出到:\n%1").arg(filePath));
    } else {
        QMessageBox::warning(this, tr("导出失败"),
                             tr("无法写入文件:\n%1").arg(filePath));
    }
}

// ============================================================================
// TimelineMiniView 实现
// ============================================================================

TimelineMiniView::TimelineMiniView(QWidget *parent)
    : QWidget(parent)
    , m_tracker(nullptr)
    , m_hoveredIndex(-1)
{
    setMinimumHeight(30);
    setMaximumHeight(50);
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
}

// 函数说明：设置 TimelineMiniView 的运行参数，并触发必要的界面或数据刷新。
void TimelineMiniView::setRevisionTracker(RevisionTracker *tracker)
{
    m_tracker = tracker;
    refresh();
}

// 函数说明：设置 TimelineMiniView 的运行参数，并触发必要的界面或数据刷新。
void TimelineMiniView::setDocumentPath(const QString &path)
{
    m_documentPath = path;
    refresh();
}

// 函数说明：实现 TimelineMiniView::refresh 的核心逻辑，供当前模块调用。
void TimelineMiniView::refresh()
{
    if (!m_tracker || m_documentPath.isEmpty()) {
        m_revisions.clear();
    } else {
        m_tracker->loadRevisions(m_documentPath);
        m_revisions = m_tracker->getRevisions(m_documentPath);
    }
    update();
}

// 函数说明：绘制 TimelineMiniView 的可视区域或辅助标记。
void TimelineMiniView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 背景
    painter.fillRect(rect(), QColor(245, 245, 245));

    if (m_revisions.isEmpty()) {
        painter.setPen(QColor(150, 150, 150));
        painter.drawText(rect(), Qt::AlignCenter, tr("无修订记录"));
        return;
    }

    // 绘制时间线
    int margin = 5;
    int nodeSize = 10;
    int y = height() / 2;

    qreal spacing = (width() - 2 * margin) / qreal(qMax(1, m_revisions.size()));

    // 绘制连接线
    painter.setPen(QPen(QColor(200, 200, 200), 2));
    painter.drawLine(margin + nodeSize/2, y,
                     width() - margin - nodeSize/2, y);

    // 绘制节点
    for (int i = 0; i < m_revisions.size(); ++i) {
        qreal x = margin + spacing * i + spacing / 2;

        QColor color;
        if (m_revisions[i].isAutoSave) {
            color = QColor(150, 150, 150);
        } else if (!m_revisions[i].tags.isEmpty()) {
            color = QColor(255, 165, 0);
        } else {
            color = QColor(70, 130, 180);
        }

        if (i == m_hoveredIndex) {
            color = color.lighter(120);
            nodeSize = 12;
        } else {
            nodeSize = 10;
        }

        painter.setPen(QPen(color.darker(130), 1));
        painter.setBrush(color);
        painter.drawEllipse(QPointF(x, y), nodeSize/2, nodeSize/2);
    }
}

// 函数说明：处理鼠标事件，更新选择、拖拽或交互状态。
void TimelineMiniView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        for (int i = 0; i < m_revisions.size(); ++i) {
            if (nodeRect(i).contains(event->pos())) {
                emit revisionClicked(m_revisions[i].id);
                return;
            }
        }
        emit showFullTimeline();
    }
    QWidget::mousePressEvent(event);
}

// 函数说明：处理鼠标事件，更新选择、拖拽或交互状态。
void TimelineMiniView::mouseMoveEvent(QMouseEvent *event)
{
    int oldHovered = m_hoveredIndex;
    m_hoveredIndex = -1;

    for (int i = 0; i < m_revisions.size(); ++i) {
        if (nodeRect(i).contains(event->pos())) {
            m_hoveredIndex = i;
            setToolTip(QString("%1\n%2")
                       .arg(m_revisions[i].timestamp.toString("yyyy-MM-dd hh:mm"))
                       .arg(m_revisions[i].description.isEmpty()
                            ? tr("无描述") : m_revisions[i].description));
            break;
        }
    }

    if (m_hoveredIndex != oldHovered) {
        update();
    }

    QWidget::mouseMoveEvent(event);
}

// 函数说明：实现 TimelineMiniView::nodeRect 的核心逻辑，供当前模块调用。
QRectF TimelineMiniView::nodeRect(int index) const
{
    if (index < 0 || index >= m_revisions.size()) return QRectF();

    int margin = 5;
    qreal spacing = (width() - 2 * margin) / qreal(qMax(1, m_revisions.size()));
    qreal x = margin + spacing * index + spacing / 2;
    qreal y = height() / 2;
    qreal size = 15;  // 点击区域比节点大一点

    return QRectF(x - size/2, y - size/2, size, size);
}

