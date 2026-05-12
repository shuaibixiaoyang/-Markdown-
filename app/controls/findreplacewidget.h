// 文件说明：app\controls\findreplacewidget.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef FINDREPLACEWIDGET_H // 中文注释：开始头文件保护，避免重复包含。
#define FINDREPLACEWIDGET_H // 中文注释：定义头文件保护宏。

#include <QWidget> // 中文注释：引入当前文件需要的依赖头文件。
#include <QTextDocument> // 中文注释：引入当前文件需要的依赖头文件。

namespace Ui { // 中文注释：声明命名空间范围。
class FindReplaceWidget;  // 前向声明UI类 // 中文注释：声明类类型或前置声明类。
} // 中文注释：结束当前代码块。

class QPlainTextEdit;  // 前向声明纯文本编辑控件 // 中文注释：声明类类型或前置声明类。

/**
 * 查找替换控件类
 *
 * 提供文本查找和替换功能，支持大小写敏感、全字匹配、正则表达式等选项。
 * 可附着在QPlainTextEdit控件上使用。
 */
class FindReplaceWidget : public QWidget // 中文注释：声明类类型或前置声明类。
{ // 中文注释：进入当前代码块。
    Q_OBJECT // 中文注释：启用 Qt 元对象系统能力。

public: // 中文注释：声明类成员的访问权限区域。
    /**
     * 构造函数
     * parent 父窗口指针，默认为nullptr
     */
    explicit FindReplaceWidget(QWidget *parent = nullptr); // 中文注释：声明变量、对象或函数。

    /**
     * 析构函数
     */
    ~FindReplaceWidget(); // 中文注释：执行当前语句。

    /**
     * 设置要操作的文本编辑控件
     * editor 目标文本编辑控件指针
     */
    void setTextEdit(QPlainTextEdit *editor); // 中文注释：声明变量、对象或函数。

signals: // 中文注释：声明 Qt 信号区域。
    /**
     * 对话框关闭信号
     * 当查找替换窗口关闭时发射
     */
    void dialogClosed(); // 中文注释：声明变量、对象或函数。

public slots: // 中文注释：声明 Qt 槽函数区域。
    void findPreviousClicked();   // 查找上一个 // 中文注释：声明变量、对象或函数。
    void findNextClicked();       // 查找下一个 // 中文注释：声明变量、对象或函数。
    void replaceClicked();        // 替换当前匹配项 // 中文注释：声明变量、对象或函数。
    void replaceAllClicked();     // 替换所有匹配项 // 中文注释：声明变量、对象或函数。

protected: // 中文注释：声明类成员的访问权限区域。
    /**
     * 显示事件处理
     * 控件显示时自动获取焦点
     */
    void showEvent(QShowEvent *event) override; // 中文注释：声明变量、对象或函数。

    /**
     * 键盘事件处理
     * 支持Esc键关闭窗口等快捷键操作
     */
    void keyPressEvent(QKeyEvent *event) override; // 中文注释：声明变量、对象或函数。

private slots: // 中文注释：声明 Qt 槽函数区域。
    void caseSensitiveToggled(bool enabled);        // 大小写敏感选项切换 // 中文注释：声明变量、对象或函数。
    void wholeWordsOnlyToggled(bool enabled);       // 全字匹配选项切换 // 中文注释：声明变量、对象或函数。
    void useRegularExpressionsToggled(bool enabled); // 正则表达式选项切换 // 中文注释：声明变量、对象或函数。
    void showOptionsMenu();                          // 显示选项菜单 // 中文注释：声明变量、对象或函数。

private: // 中文注释：声明类成员的访问权限区域。
    /**
     * 设置查找选项菜单
     * 初始化下拉菜单中的各项选项
     */
    void setupFindOptionsMenu(); // 中文注释：声明变量、对象或函数。

    /**
     * 执行文本查找
     * searchString 要查找的字符串
     * findOptions 查找选项标志
     * 返回 是否找到匹配项
     */
    bool find(const QString &searchString, QTextDocument::FindFlags findOptions = QTextDocument::FindFlags()) const; // 中文注释：声明变量、对象或函数。

    /**
     * 使用正则表达式执行查找
     * pattern 正则表达式模式
     * findOptions 查找选项标志
     * 返回 是否找到匹配项
     */
    bool findUsingRegExp(const QString &pattern, QTextDocument::FindFlags findOptions = QTextDocument::FindFlags()) const; // 中文注释：声明变量、对象或函数。

    Ui::FindReplaceWidget *ui;      // UI界面指针 // 中文注释：保留当前代码结构。
    QPlainTextEdit *textEditor;     // 关联的文本编辑控件 // 中文注释：声明变量、对象或函数。

    bool findCaseSensitively;       // 是否区分大小写 // 中文注释：声明变量、对象或函数。
    bool findWholeWordsOnly;        // 是否全字匹配 // 中文注释：声明变量、对象或函数。
    bool findUseRegExp;             // 是否使用正则表达式 // 中文注释：声明变量、对象或函数。
}; // 中文注释：结束类型或作用域声明。

#endif // FINDREPLACEWIDGET_H // 中文注释：结束条件编译或头文件保护。

