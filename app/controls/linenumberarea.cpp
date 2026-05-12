// 文件说明：app\controls\linenumberarea.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "linenumberarea.h" // 中文注释：引入当前文件需要的依赖头文件。

#include "markdowneditor.h" // 中文注释：引入当前文件需要的依赖头文件。

// 构造函数
// 将行号区域挂载到对应的 MarkdownEditor 上
LineNumberArea::LineNumberArea(MarkdownEditor *editor) : // 中文注释：保留当前代码结构。
    QWidget(editor) // 中文注释：声明变量、对象或函数。
{ // 中文注释：进入当前代码块。
    this->editor = editor; // 中文注释：更新变量或对象状态。
} // 中文注释：结束当前代码块。
// 返回行号区域的建议大小
// 宽度由编辑器计算，高度不需要指定
QSize LineNumberArea::sizeHint() const // 中文注释：声明变量、对象或函数。
{ // 中文注释：进入当前代码块。
    return QSize(editor->lineNumberAreaWidth(), 0); // 中文注释：返回函数处理结果。
} // 中文注释：结束当前代码块。


// 绘制事件
// 直接交给 MarkdownEditor 处理具体的行号绘制逻辑
void LineNumberArea::paintEvent(QPaintEvent *event) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    editor->lineNumberAreaPaintEvent(event); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。

