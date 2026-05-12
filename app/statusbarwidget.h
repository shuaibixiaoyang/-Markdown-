// 文件说明：app\statusbarwidget.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef STATUSBARWIDGET_H
#define STATUSBARWIDGET_H

// 包含Qt基础控件头文件
#include <QWidget>

// 前置声明Qt相关类
class QLabel;
class QActionGroup;

// 前置声明编辑器相关类
class MarkdownEditor;
class ActiveLabel;

// 状态栏部件类
// 用于显示编辑器状态信息：行列号、字数、样式、HTML预览等
class StatusBarWidget : public QWidget
{
    // Qt元对象宏，支持信号槽机制
    Q_OBJECT

public:
    // 构造函数，绑定对应的Markdown编辑器
    explicit StatusBarWidget(MarkdownEditor* editor);
    // 析构函数，释放状态栏内的所有子控件
    ~StatusBarWidget();

public slots:
    // 整体更新状态栏所有信息
    void update();
    // 设置是否显示行号和列号
    void showLineColumn(bool enabled);

    // 为HTML标签绑定动作
    void setHtmlAction(QAction *action);
    // 为样式切换设置动作组
    void setStyleActions(QActionGroup *actionGroup);

private slots:
    // 编辑器光标位置改变时更新状态栏
    void cursorPositionChanged();
    // 编辑器文本内容改变时更新状态栏
    void textChanged();

    // 显示样式右键菜单
    void styleContextMenu(const QPoint &pos);
    // 更新当前样式标签显示
    void updateStyleLabel();

private:
    MarkdownEditor* m_editor;       // 关联的Markdown编辑器对象

    QLabel *m_lineColLabel;         // 显示行号和列号的标签
    QLabel *m_wordCountLabel;       // 显示单词/字符统计的标签

    QLabel *m_styleLabel;           // 显示当前编辑器样式的标签

    QActionGroup *m_styleActions;   // 样式切换的动作组

    ActiveLabel *m_htmlLabel;       // HTML预览/导出功能的活动标签
};

#endif // STATUSBARWIDGET_H

