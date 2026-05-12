// 文件说明：app-static\writing\outlinenavigator.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef OUTLINENAVIGATOR_H
#define OUTLINENAVIGATOR_H

#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QString>
#include <QVector>
#include <QLabel>
#include <QTimer>

/**
 * @brief 大纲导航器
 *
 * 功能：
 * - 解析 Markdown 标题生成大纲
 * - 树形结构显示层级关系
 * - 点击标题跳转到对应位置
 * - 支持搜索过滤
 * - 显示各级标题数量
 */
class OutlineNavigator : public QWidget
{
    Q_OBJECT

public:
    // 大纲项结构
    struct OutlineItem {
        int level;              // 标题级别 (1-6)
        QString text;           // 标题文本
        int lineNumber;         // 行号
        int position;           // 文本位置
        QVector<OutlineItem> children;  // 子项

        OutlineItem() : level(0), lineNumber(0), position(0) {}
    };

    explicit OutlineNavigator(QWidget *parent = nullptr);
    ~OutlineNavigator();

    // 设置编辑器
    void setEditor(QPlainTextEdit *editor);

    // 获取大纲
    QVector<OutlineItem> outline() const { return m_outline; }
    int headingCount(int level = 0) const;  // level=0 返回总数

    // 配置
    void setMaxLevel(int level);            // 最大显示级别
    void setAutoUpdate(bool enable);        // 自动更新
    void setShowLineNumbers(bool show);     // 显示行号
    void setHighlightCurrent(bool enable);  // 高亮当前位置

signals:
    void outlineUpdated();
    void itemClicked(int lineNumber, int position);
    void currentHeadingChanged(const OutlineItem &item);

public slots:
    void updateOutline();
    void navigateToItem(QTreeWidgetItem *item);
    void filterOutline(const QString &filter);
    void expandAll();
    void collapseAll();
    void highlightCurrentPosition();

private slots:
    void onTextChanged();
    void onCursorPositionChanged();
    void onItemClicked(QTreeWidgetItem *item, int column);
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    void setupUi();
    QVector<OutlineItem> parseMarkdownHeadings(const QString &text);
    void buildTree(const QVector<OutlineItem> &items, QTreeWidgetItem *parent = nullptr);
    void addItemToTree(const OutlineItem &item, QTreeWidgetItem *parent);
    QTreeWidgetItem *findItemByPosition(int position, QTreeWidgetItem *parent = nullptr);
    bool matchesFilter(const QString &text, const QString &filter);
    void applyFilter(QTreeWidgetItem *item, const QString &filter);
    QString getLevelIcon(int level);

    QPlainTextEdit *m_editor;
    QVector<OutlineItem> m_outline;

    // UI 组件
    QVBoxLayout *m_layout;
    QLineEdit *m_searchEdit;
    QTreeWidget *m_treeWidget;
    QLabel *m_countLabel;

    // 配置
    int m_maxLevel;
    bool m_autoUpdate;
    bool m_showLineNumbers;
    bool m_highlightCurrent;

    // 状态
    QTreeWidgetItem *m_currentItem;
    QTimer *m_updateTimer;        // 防抖定时器
};

#endif // OUTLINENAVIGATOR_H

