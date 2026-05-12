// 文件说明：app-static\preview\tocfloatingwindow.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef TOCFLOATINGWINDOW_H
#define TOCFLOATINGWINDOW_H

#include <QWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QVector>
#include <QPlainTextEdit>
#include "compat/webenginecompat.h"

/**
 * @brief TOC 悬浮窗口
 *
 * 功能：
 * - 悬浮显示文档目录
 * - 跟随主窗口位置
 * - 点击跳转
 * - 搜索过滤
 * - 自动折叠/展开
 * - 半透明效果
 */
class TocFloatingWindow : public QWidget
{
    Q_OBJECT

public:
    // TOC 项结构
    struct TocItem {
        int level;              // 标题级别 (1-6)
        QString text;           // 标题文本
        QString anchor;         // 锚点 ID
        int lineNumber;         // 对应行号
        QVector<TocItem> children;

        TocItem() : level(0), lineNumber(0) {}
    };

    // 显示位置
    enum class Position {
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
        FollowCursor
    };
    Q_ENUM(Position)

    explicit TocFloatingWindow(QWidget *parent = nullptr);
    ~TocFloatingWindow();

    // 设置关联的控件
    void setEditor(QPlainTextEdit *editor);
    void setWebView(QWebEngineView *webView);
    void setParentWindow(QWidget *window);

    // 设置 TOC 内容
    void setTocHtml(const QString &tocHtml);
    void setTocItems(const QVector<TocItem> &items);
    void updateFromMarkdown(const QString &markdown);

    // 位置和显示
    void setPosition(Position pos);
    Position position() const { return m_position; }
    void setOffset(int x, int y);
    void setOpacity(qreal opacity);
    void setMaxHeight(int height);
    void setAutoHide(bool enable);
    void setAutoFollow(bool enable);

    // 当前高亮
    void highlightItem(const QString &anchor);
    void highlightByLineNumber(int lineNumber);

signals:
    void itemClicked(const QString &anchor, int lineNumber);
    void visibilityChanged(bool visible);

public slots:
    void show();
    void hide();
    void toggle();
    void updatePosition();
    void filterItems(const QString &filter);
    void expandAll();
    void collapseAll();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onItemClicked(QTreeWidgetItem *item, int column);
    void onEditorScrolled();
    void onAutoHideTimeout();

private:
    void setupUi();
    QVector<TocItem> parseHtmlToc(const QString &html);
    QVector<TocItem> parseMarkdownHeadings(const QString &markdown);
    void buildTree(const QVector<TocItem> &items, QTreeWidgetItem *parent = nullptr);
    void addItemToTree(const TocItem &item, QTreeWidgetItem *parent);
    void applyFilter(QTreeWidgetItem *item, const QString &filter);
    bool matchesFilter(const QString &text, const QString &filter);
    QPoint calculatePosition();
    QString getLevelIcon(int level);

    // 关联控件
    QPlainTextEdit *m_editor;
    QWebEngineView *m_webView;
    QWidget *m_parentWindow;

    // UI 组件
    QVBoxLayout *m_layout;
    QLineEdit *m_searchEdit;
    QTreeWidget *m_treeWidget;

    // TOC 数据
    QVector<TocItem> m_tocItems;

    // 配置
    Position m_position;
    QPoint m_offset;
    int m_maxHeight;
    bool m_autoHide;
    bool m_autoFollow;

    // 状态
    QTreeWidgetItem *m_currentItem;
    QTimer *m_autoHideTimer;
    bool m_isDragging;
    QPoint m_dragStartPos;
    QPoint m_windowStartPos;
};

#endif // TOCFLOATINGWINDOW_H

