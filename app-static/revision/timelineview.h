// 文件说明：app-static\revision\timelineview.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QSplitter>
#include <QTextEdit>
#include "compat/webenginecompat.h"
#include <QDateTime>
#include <QMap>
#include <QSet>

#include "revisiontracker.h"

class RevisionTracker;
class TimelineScene;
class RevisionNode;
class RevisionEdge;

/**
 * @brief 修订节点图形项
 *
 * 表示时间线上的单个修订版本
 */
class RevisionNode : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit RevisionNode(const RevisionTracker::Revision &revision, QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QString revisionId() const { return m_revision.id; }
    RevisionTracker::Revision revision() const { return m_revision; }

    void setSelected(bool selected);
    bool isNodeSelected() const { return m_isSelected; }

    void setCompareMode(bool enabled, bool isPrimary = false);
    bool isCompareMode() const { return m_compareMode; }
    bool isPrimaryCompare() const { return m_isPrimaryCompare; }

    void setHighlighted(bool highlighted);
    bool isHighlighted() const { return m_highlighted; }

    void setBranchIndex(int index) { m_branchIndex = index; }
    int branchIndex() const { return m_branchIndex; }

    static constexpr qreal NodeRadius = 20.0;
    static constexpr qreal NodeSpacing = 80.0;

signals:
    void clicked(const QString &revisionId);
    void doubleClicked(const QString &revisionId);
    void contextMenuRequested(const QString &revisionId, const QPointF &pos);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    RevisionTracker::Revision m_revision;
    bool m_isSelected;
    bool m_compareMode;
    bool m_isPrimaryCompare;
    bool m_highlighted;
    bool m_hovered;
    int m_branchIndex;

    QColor nodeColor() const;
    QString tooltipText() const;
};

/**
 * @brief 修订连接边
 *
 * 表示两个修订之间的父子关系
 */
class RevisionEdge : public QGraphicsItem
{
public:
    RevisionEdge(RevisionNode *source, RevisionNode *target, QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void setHighlighted(bool highlighted) { m_highlighted = highlighted; }
    bool isHighlighted() const { return m_highlighted; }

    void updatePosition();

private:
    RevisionNode *m_source;
    RevisionNode *m_target;
    bool m_highlighted;
};

/**
 * @brief 时间线场景
 *
 * 管理所有修订节点和边的图形场景
 */
class TimelineScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit TimelineScene(QObject *parent = nullptr);
    ~TimelineScene();

    void setRevisions(const QVector<RevisionTracker::Revision> &revisions);
    void clear();

    void selectNode(const QString &revisionId);
    void clearSelection();

    void setCompareMode(bool enabled);
    bool isCompareMode() const { return m_compareMode; }

    void setFirstCompareRevision(const QString &revisionId);
    void setSecondCompareRevision(const QString &revisionId);
    QString firstCompareRevision() const { return m_firstCompareId; }
    QString secondCompareRevision() const { return m_secondCompareId; }

    RevisionNode* nodeById(const QString &revisionId) const;
    QVector<RevisionNode*> allNodes() const { return m_nodes.values().toVector(); }

signals:
    void nodeClicked(const QString &revisionId);
    void nodeDoubleClicked(const QString &revisionId);
    void compareRevisionsRequested(const QString &oldId, const QString &newId);
    void restoreRevisionRequested(const QString &revisionId);
    void nodeContextMenu(const QString &revisionId, const QPoint &globalPos);

private slots:
    void onNodeClicked(const QString &revisionId);
    void onNodeDoubleClicked(const QString &revisionId);
    void onNodeContextMenu(const QString &revisionId, const QPointF &scenePos);

private:
    void layoutNodes();
    void createEdges();
    void detectBranches();
    int calculateBranchIndex(const RevisionTracker::Revision &revision);

    QMap<QString, RevisionNode*> m_nodes;
    QVector<RevisionEdge*> m_edges;
    QVector<RevisionTracker::Revision> m_revisions;

    bool m_compareMode;
    QString m_firstCompareId;
    QString m_secondCompareId;
    QString m_selectedId;

    QMap<QString, int> m_branchIndices;
    int m_maxBranchIndex;
};

/**
 * @brief 时间线视图控件
 *
 * 完整的时间线可视化界面，包含：
 * - 时间线图形视图
 * - 版本详情面板
 * - 版本对比视图
 * - 控制工具栏
 */
class TimelineView : public QWidget
{
    Q_OBJECT

public:
    explicit TimelineView(QWidget *parent = nullptr);
    ~TimelineView();

    void setRevisionTracker(RevisionTracker *tracker);
    RevisionTracker* revisionTracker() const { return m_tracker; }

    void setDocumentPath(const QString &path);
    QString documentPath() const { return m_documentPath; }

    void refresh();
    void setCurrentContent(const QString &content);

signals:
    void revisionRestoreRequested(const QString &revisionId);
    void revisionSelected(const QString &revisionId);
    void compareRequested(const QString &oldId, const QString &newId);

public slots:
    void enterCompareMode();
    void exitCompareMode();
    void compareWithCurrent();
    void zoomIn();
    void zoomOut();
    void fitInView();

private slots:
    void onNodeClicked(const QString &revisionId);
    void onNodeDoubleClicked(const QString &revisionId);
    void onCompareRevisionsRequested(const QString &oldId, const QString &newId);
    void onRestoreRevisionRequested(const QString &revisionId);
    void onNodeContextMenu(const QString &revisionId, const QPoint &globalPos);
    void onCompareButtonClicked();
    void onRestoreButtonClicked();
    void onExportButtonClicked();

private:
    void setupUi();
    void setupToolbar();
    void setupTimelineView();
    void setupDetailPanel();
    void setupCompareView();
    void updateDetailPanel(const QString &revisionId);
    void showCompareResult(const QString &oldId, const QString &newId);
    void updateRevisions();

    RevisionTracker *m_tracker;
    QString m_documentPath;
    QString m_currentContent;
    QString m_selectedRevisionId;

    // UI 组件
    QVBoxLayout *m_mainLayout;
    QSplitter *m_mainSplitter;
    QSplitter *m_rightSplitter;

    // 工具栏
    QWidget *m_toolbar;
    QPushButton *m_refreshButton;
    QPushButton *m_compareModeButton;
    QPushButton *m_compareCurrentButton;
    QPushButton *m_zoomInButton;
    QPushButton *m_zoomOutButton;
    QPushButton *m_fitButton;
    QLabel *m_statusLabel;

    // 时间线视图
    QGraphicsView *m_graphicsView;
    TimelineScene *m_scene;

    // 详情面板
    QWidget *m_detailPanel;
    QLabel *m_revisionIdLabel;
    QLabel *m_timestampLabel;
    QLabel *m_authorLabel;
    QLabel *m_descriptionLabel;
    QLabel *m_statsLabel;
    QLabel *m_tagsLabel;
    QPushButton *m_restoreButton;
    QPushButton *m_compareButton;
    QPushButton *m_exportButton;

    // 对比视图
    QWidget *m_comparePanel;
    QComboBox *m_oldRevisionCombo;
    QComboBox *m_newRevisionCombo;
    QPushButton *m_doCompareButton;
    QWebEngineView *m_diffView;
    QLabel *m_compareStatusLabel;

    bool m_compareMode;
};

/**
 * @brief 时间线迷你视图
 *
 * 用于在主窗口状态栏或侧边栏显示的简化时间线
 */
class TimelineMiniView : public QWidget
{
    Q_OBJECT

public:
    explicit TimelineMiniView(QWidget *parent = nullptr);

    void setRevisionTracker(RevisionTracker *tracker);
    void setDocumentPath(const QString &path);
    void refresh();

signals:
    void revisionClicked(const QString &revisionId);
    void showFullTimeline();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    RevisionTracker *m_tracker;
    QString m_documentPath;
    QVector<RevisionTracker::Revision> m_revisions;
    int m_hoveredIndex;

    QRectF nodeRect(int index) const;
};

#endif // TIMELINEVIEW_H

