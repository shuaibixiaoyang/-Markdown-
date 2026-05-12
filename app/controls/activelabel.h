// 文件说明：app\controls\activelabel.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef ACTIVELABEL_H // 中文注释：开始头文件保护，避免重复包含。
#define ACTIVELABEL_H // 中文注释：定义头文件保护宏。

// 包含QLabel基类的头文件
#include <QLabel> // 中文注释：引入当前文件需要的依赖头文件。

// 前置声明QAction类，用于声明指针
class QAction; // 中文注释：声明类类型或前置声明类。

// 自定义标签类，继承自QLabel，支持绑定QAction和双击信号
class ActiveLabel : public QLabel // 中文注释：声明类类型或前置声明类。
{ // 中文注释：进入当前代码块。
    Q_OBJECT // 中文注释：启用 Qt 元对象系统能力。

public: // 中文注释：声明类成员的访问权限区域。
    // 构造函数，父部件指针默认空
    explicit ActiveLabel(QWidget *parent = nullptr); // 中文注释：声明变量、对象或函数。

    // 带初始文本的构造函数
    explicit ActiveLabel(const QString &text, QWidget *parent = nullptr); // 中文注释：声明变量、对象或函数。

    // 设置要绑定的QAction对象，绑定后会同步状态
    void setAction(QAction *action); // 中文注释：声明变量、对象或函数。

public slots: // 中文注释：声明 Qt 槽函数区域。
    // 从绑定的QAction更新标签的显示状态
    void updateFromAction(); // 中文注释：声明变量、对象或函数。

signals: // 中文注释：声明 Qt 信号区域。
    // 鼠标双击时发出此信号
    void doubleClicked(); // 中文注释：声明变量、对象或函数。

protected: // 中文注释：声明类成员的访问权限区域。
    // 重写鼠标双击事件处理函数
    void mouseDoubleClickEvent(QMouseEvent *e) override; // 中文注释：声明变量、对象或函数。

private: // 中文注释：声明类成员的访问权限区域。
    // 指向绑定的QAction对象的指针
    QAction *m_action; // 中文注释：声明变量、对象或函数。
}; // 中文注释：结束类型或作用域声明。

#endif // ACTIVELABEL_H // 中文注释：结束条件编译或头文件保护。

