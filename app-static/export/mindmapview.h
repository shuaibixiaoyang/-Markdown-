// 文件说明：app-static\export\mindmapview.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef MINDMAPVIEW_H
#define MINDMAPVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsProxyWidget>
#include <QString>
#include <QVector>
#include <QColor>
#include <QFont>
#include <QMenu>

/**
 * @brief 思维导图视图
 *
 * 将 Markdown 文档结构可视化为思维导图
 *
 * 功能：
 * - 从 Markdown 标题生成节点
 * - 自动布局（径向、树形、水平）
 * - 节点展开/折叠
 * - 缩放和平移
 * - 导出为图片/SVG
 * - 点击节点跳转到对应行
 */

// 前向声明
class MindMapNode;

class MindMapView : public QGraphicsView
{
    Q_OBJECT

public:
    // 布局类型
    enum class LayoutType {
        Radial,         // 径向布局
        Tree,           // 树形布局
        Horizontal,     // 水平布局
        Vertical        // 垂直布局
    };
    Q_ENUM(LayoutType)

    // 主题
    struct Theme {
        QString name;
        QColor backgroundColor;
        QColor nodeColor;
        QColor nodeTextColor;
        QColor lineColor;
        QColor selectedColor;
        QFont nodeFont;
        int nodeRadius;
        int lineWidth;

        Theme()
            : name("Default")
            , backgroundColor(Qt::white)
            , nodeColor(QColor("#4A90D9"))
            , nodeTextColor(Qt::white)
            , lineColor(QColor("#888888"))
            , selectedColor(QColor("#FF6B6B"))
            , nodeRadius(8)
            , lineWidth(2)
        {
            nodeFont.setPointSize(11);
        }
    };

    // 节点数据
    struct NodeData {
        QString text;
        int level;              // 层级 (0=根, 1=H1, 2=H2, ...)
        int lineNumber;         // 对应的行号
        QVector<NodeData*> children;
        NodeData *parent;
        bool collapsed;

        NodeData()
            : level(0)
            , lineNumber(-1)
            , parent(nullptr)
            , collapsed(false)
        {}

        ~NodeData() {
            qDeleteAll(children);
        }
    };

    explicit MindMapView(QWidget *parent = nullptr);
    ~MindMapView();

    // 加载 Markdown
    void loadFromMarkdown(const QString &markdown);
    void clear();

    // 布局
    void setLayoutType(LayoutType type);
    LayoutType layoutType() const { return m_layoutType; }
    void relayout();

    // 主题
    void setTheme(const Theme &theme);
    Theme theme() const { return m_theme; }
    static QVector<Theme> builtinThemes();

    // 导出
    bool exportToImage(const QString &filePath, const QString &format = "PNG");
    bool exportToSvg(const QString &filePath);
    bool exportToPdf(const QString &filePath);

    // 双向编辑
    void setEditable(bool editable);
    bool isEditable() const { return m_editable; }
    void startEditingNode(MindMapNode *node);
    void finishEditingNode();
    void addChildNode(MindMapNode *parentNode, const QString &text = QString());
    void addSiblingNode(MindMapNode *node, const QString &text = QString());
    void deleteNode(MindMapNode *node);
    QString generateMarkdown() const;

    // 节点操作
    void expandAll();
    void collapseAll();
    void expandNode(MindMapNode *node);
    void collapseNode(MindMapNode *node);
    void centerOnRoot();
    void fitInView();

signals:
    void nodeClicked(int lineNumber);
    void nodeDoubleClicked(int lineNumber);
    void selectionChanged(const QString &nodeText);
    void markdownChanged(const QString &newMarkdown);  // 双向编辑：导图更改后发出
    void nodeTextChanged(int lineNumber, const QString &oldText, const QString &newText);
    void nodeAdded(int parentLineNumber, int level, const QString &text);
    void nodeDeleted(int lineNumber);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void onNodeSelected(MindMapNode *node);

private:
    // 解析 Markdown
    NodeData* parseMarkdown(const QString &markdown);
    void parseHeadings(const QString &markdown, NodeData *root);

    // 创建图形项
    void buildGraphicsItems();
    MindMapNode* createNodeItem(NodeData *data, MindMapNode *parentItem = nullptr);
    void createConnections();

    // 布局算法
    void layoutRadial(MindMapNode *root, qreal cx, qreal cy, qreal startAngle, qreal sweepAngle, qreal radius);
    void layoutTree(MindMapNode *root, qreal x, qreal y, bool horizontal);
    void layoutVertical(MindMapNode *root, qreal x, qreal y);
    void layoutHorizontal(MindMapNode *root, qreal x, qreal y);

    // 计算子树大小
    QSizeF calculateSubtreeSize(MindMapNode *node, bool horizontal);

    QGraphicsScene *m_scene;
    NodeData *m_rootData;
    MindMapNode *m_rootItem;
    LayoutType m_layoutType;
    Theme m_theme;
    QMenu *m_contextMenu;
    MindMapNode *m_selectedNode;

    qreal m_horizontalSpacing;
    qreal m_verticalSpacing;
    qreal m_levelSpacing;

    // 双向编辑
    bool m_editable;
    MindMapNode *m_editingNode;
    QGraphicsProxyWidget *m_editProxy;
    class QLineEdit *m_editWidget;
    QString m_originalMarkdown;
};

/**
 * @brief 思维导图节点图形项
 */
class MindMapNode : public QGraphicsItem
{
public:
    MindMapNode(MindMapView::NodeData *data, MindMapView *view, MindMapNode *parent = nullptr);
    ~MindMapNode();

    // 数据访问
    MindMapView::NodeData* data() const { return m_data; }
    QString text() const { return m_data->text; }
    void setText(const QString &text);
    int level() const { return m_data->level; }
    void setLevel(int level) { m_data->level = level; }
    int lineNumber() const { return m_data->lineNumber; }
    void setLineNumber(int lineNumber) { m_data->lineNumber = lineNumber; }

    // 子节点
    void addChild(MindMapNode *child);
    void insertChild(int index, MindMapNode *child);
    void removeChild(MindMapNode *child);
    QVector<MindMapNode*> children() const { return m_children; }
    QVector<MindMapNode*>& childrenRef() { return m_children; }
    MindMapNode* parentNode() const { return m_parentNode; }

    // 展开/折叠
    bool isCollapsed() const { return m_data->collapsed; }
    void setCollapsed(bool collapsed);
    void toggleCollapsed();

    // 选中状态
    bool isSelected() const { return m_selected; }
    void setSelected(bool selected);

    // 连接线
    void updateConnections();

    // QGraphicsItem 接口
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    QRectF calculateBoundingRect() const;
    void drawConnection(QPainter *painter, MindMapNode *child);

    MindMapView::NodeData *m_data;
    MindMapView *m_view;
    MindMapNode *m_parentNode;
    QVector<MindMapNode*> m_children;
    bool m_selected;
    bool m_hovered;
    QRectF m_boundingRect;
    qreal m_width;
    qreal m_height;
};

#endif // MINDMAPVIEW_H

