// 文件说明：app\htmlhighlighter.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef HTMLHIGHLIGHTER_H
#define HTMLHIGHLIGHTER_H

// 包含Qt语法高亮基类头文件
#include <QSyntaxHighlighter>

// 包含正则表达式头文件，用于匹配HTML语法
#include <QRegularExpression>

// HTML语法高亮类，继承自QSyntaxHighlighter
// 用于在文本编辑器中对HTML代码进行彩色高亮显示
class HtmlHighlighter : public QSyntaxHighlighter
{
public:
    // 构造函数，指定要高亮的文本文档
    explicit HtmlHighlighter(QTextDocument *document);

    // 获取高亮功能是否启用
    bool isEnabled() const;

    // 设置高亮功能启用/禁用状态
    void setEnabled(bool enabled);

protected:
    // 重写高亮核心方法，对单行文本进行语法高亮处理
    void highlightBlock(const QString &text) override;

private:
    // 高亮规则结构体
    // 存储正则表达式匹配模式和对应的文本样式
    struct HighlightingRule
    {
        QRegularExpression pattern;   // 匹配HTML元素的正则表达式
        QTextCharFormat *format;      // 匹配后应用的文本样式
    };

    // 存储所有HTML高亮规则的列表
    QList<HighlightingRule> highlightingRules;

    // HTML关键字样式（标签名等）
    QTextCharFormat keywordFormat;

    // HTML图片标签样式
    QTextCharFormat imageFormat;

    // HTML链接标签样式
    QTextCharFormat linkFormat;

    // 高亮功能启用状态
    bool enabled;
};

#endif // HTMLHIGHLIGHTER_H

