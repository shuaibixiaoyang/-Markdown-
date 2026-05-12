// 文件说明：app\markdownmanipulator.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef MARKDOWNMANIPULATOR_H
#define MARKDOWNMANIPULATOR_H

// 包含Qt基础类型定义
#include <Qt>
// 包含Qt列表容器
#include <QList>
// 包含Qt字符串类
#include <QString>
// 包含Qt字符串列表类
#include <QStringList>

// 前置声明Qt字符类
class QChar;
// 前置声明Qt纯文本编辑器类
class QPlainTextEdit;

// Markdown文本操作工具类
// 提供对纯文本编辑器中Markdown内容的格式化、插入、修改功能
class MarkdownManipulator
{
public:
    // 构造函数，绑定需要操作的纯文本编辑器
    explicit MarkdownManipulator(QPlainTextEdit *editor);

    // 用指定标签包裹选中的文本
    void wrapSelectedText(const QString &tag);

    // 用起始标签和结束标签包裹当前段落
    void wrapCurrentParagraph(const QString &startTag, const QString &endTag);

    // 在当前行末尾追加指定文本
    void appendToLine(const QString &text);

    // 在当前行开头添加指定标记
    void prependToLine(const QChar &mark);

    // 提升当前行的标题等级
    void increaseHeadingLevel();

    // 降低当前行的标题等级
    void decreaseHeadingLevel();

    // 将当前行格式化为引用样式
    void formatTextAsQuote();

    // 插入表格，指定行数、列数、对齐方式和单元格内容
    void insertTable(int rows, int columns, const QList<Qt::Alignment> &alignments, const QList<QStringList> &cells);

    // 插入图片链接，指定替代文本、图片路径和可选标题
    void insertImageLink(const QString &alternateText, const QString &imageSource, const QString &optionalTitle = QString());

private:
    // 私有工具方法：使用标记格式化当前文本块
    void formatBlock(const QChar &mark);

    // 绑定的纯文本编辑器对象指针
    QPlainTextEdit *editor;
};

#endif // MARKDOWNMANIPULATOR_H

