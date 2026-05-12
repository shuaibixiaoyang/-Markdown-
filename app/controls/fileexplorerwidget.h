// 文件说明：app\controls\fileexplorerwidget.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef FILEEXPLORERWIDGET_H // 中文注释：开始头文件保护，避免重复包含。
#define FILEEXPLORERWIDGET_H // 中文注释：定义头文件保护宏。

#include <QWidget> // 中文注释：引入当前文件需要的依赖头文件。

//这是一个文件浏览器控件，提供文件系统浏览功能。
//基于Qt的Model/View架构，支持目录树形展示、文件排序和过滤。
//用户双击文件时发射`fileSelected()`信号，传递文件路径供外部使用。
//采用延迟初始化机制，在控件首次显示时完成初始化。
namespace Ui { // 中文注释：声明命名空间范围。
class FileExplorerWidget;  // 前向声明UI类，由Qt Designer自动生成 // 中文注释：声明类类型或前置声明类。
} // 中文注释：结束当前代码块。

// Qt核心类的前向声明
class QFileSystemModel;        // 文件系统数据模型 // 中文注释：声明类类型或前置声明类。
class QSortFilterProxyModel;   // 排序过滤代理模型 // 中文注释：声明类类型或前置声明类。

/**
 * 文件浏览器控件类
 *
 * 提供文件系统浏览功能，基于Qt的Model/View架构实现。
 * 支持目录树形展示、文件排序和过滤功能。
 */
class FileExplorerWidget : public QWidget // 中文注释：声明类类型或前置声明类。
{ // 中文注释：进入当前代码块。
    Q_OBJECT  // Qt元对象系统宏，支持信号槽机制 // 中文注释：启用 Qt 元对象系统能力。

public: // 中文注释：声明类成员的访问权限区域。
    /**
     * 构造函数
     * parent 父窗口指针，默认为nullptr
     */
    explicit FileExplorerWidget(QWidget *parent = nullptr); // 中文注释：声明变量、对象或函数。

    /**
     * 析构函数，释放资源
     */
    ~FileExplorerWidget(); // 中文注释：执行当前语句。

signals: // 中文注释：声明 Qt 信号区域。
    /**
     * 文件选中信号
     * filePath 被选中文件的完整路径
     *
     * 当用户双击文件或通过其他方式选中文件时发射此信号
     */
    void fileSelected(const QString &filePath); // 中文注释：声明变量、对象或函数。

protected: // 中文注释：声明类成员的访问权限区域。
    /**
     * 控件显示事件处理函数
     * event 显示事件对象
     *
     * 重写此函数以在控件首次显示时执行延迟初始化
     */
    void showEvent(QShowEvent *event); // 中文注释：声明变量、对象或函数。

private slots: // 中文注释：声明 Qt 槽函数区域。
    /**
     * 文件打开槽函数
     * index 被点击项的模型索引
     *
     * 处理用户双击文件或目录的操作：
     * - 如果是目录：展开/折叠目录
     * - 如果是文件：发射fileSelected()信号
     */
    void fileOpen(const QModelIndex &index); // 中文注释：声明变量、对象或函数。

private: // 中文注释：声明类成员的访问权限区域。
    bool initialized;                    // 初始化标志，用于延迟初始化 // 中文注释：声明变量、对象或函数。
    Ui::FileExplorerWidget *ui;          // UI界面指针，包含所有界面组件 // 中文注释：保留当前代码结构。
    QFileSystemModel *model;             // 文件系统数据模型，管理文件和目录数据 // 中文注释：声明变量、对象或函数。
    QSortFilterProxyModel *sortModel;    // 排序过滤代理模型，提供排序和过滤功能 // 中文注释：声明变量、对象或函数。
}; // 中文注释：结束类型或作用域声明。

#endif // FILEEXPLORERWIDGET_H // 中文注释：结束条件编译或头文件保护。

