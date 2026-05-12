// 文件说明：app\controls\linenumberarea.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef LINENUMBERAREA_H // 中文注释：开始头文件保护，避免重复包含。
#define LINENUMBERAREA_H // 中文注释：定义头文件保护宏。

#include <QWidget> // 中文注释：引入当前文件需要的依赖头文件。

class MarkdownEditor; // 中文注释：声明类类型或前置声明类。

//行号显示区域部件
// 依附于 MarkdownEditor，专门负责绘制左侧行号
class LineNumberArea : public QWidget // 中文注释：声明类类型或前置声明类。
{ // 中文注释：进入当前代码块。
    Q_OBJECT // 中文注释：启用 Qt 元对象系统能力。
public: // 中文注释：声明类成员的访问权限区域。
    // 构造函数，需要传入所属的编辑器
    explicit LineNumberArea(MarkdownEditor *editor); // 中文注释：声明变量、对象或函数。
     // 推荐尺寸，由编辑器计算行号宽度后返回
    QSize sizeHint() const; // 中文注释：声明变量、对象或函数。

protected: // 中文注释：声明类成员的访问权限区域。
    // 绘制事件，编辑器会调用这里来画行号
    void paintEvent(QPaintEvent *event); // 中文注释：声明变量、对象或函数。

private: // 中文注释：声明类成员的访问权限区域。
    // 指向所属的 Markdown 编辑器
    MarkdownEditor *editor; // 中文注释：执行当前语句。
}; // 中文注释：结束类型或作用域声明。

#endif // LINENUMBERAREA_H // 中文注释：结束条件编译或头文件保护。

