// 文件说明：app\markdownhighlighter.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef MARKDOWNHIGHLIGHTER_H
#define MARKDOWNHIGHLIGHTER_H

#include <QSyntaxHighlighter>
// Markdown 语法解析相关的外部库头文件
#include "pmh_definitions.h"
#include "peg-markdown-highlight/definitions.h"
// 后台高亮计算线程
#include "highlightworkerthread.h"

// 拼写检查器前置声明
namespace hunspell {
class SpellChecker;
}

// Markdown 语法高亮类
// 继承自 Qt 语法高亮基类，负责给文本上色
class MarkdownHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    //构造函数 // 需要传入文档对象和拼写检查器
    MarkdownHighlighter(QTextDocument *document, hunspell::SpellChecker *spellChecker);
    ~MarkdownHighlighter();
    //重置高亮状态
    void reset();
    //设置高亮样式
    void setStyles(const QVector<PegMarkdownHighlight::HighlightingStyle> &styles);
    //开启/关闭拼写检查
    void setSpellingCheckEnabled(bool enabled);
     // 开启/关闭 YAML 头部支持
    void setYamlHeaderSupportEnabled(bool enabled);

protected:
    //重写高亮函数，对每一行文本进行上色
    void highlightBlock(const QString &textBlock) override;

private slots:
    //后台线程解析完成，返回语法元素，进行上色
    void resultReady(pmh_element **elements, unsigned long base_offset);

private:
    //对一段文本应用指定格式
    void applyFormat(unsigned long pos, unsigned long end, QTextCharFormat format, bool merge);
    //对当前行进行拼写错误检查
    void checkSpelling(const QString &textBlock);

    HighlightWorkerThread *workerThread;               // 后台高亮计算线程
    QVector<PegMarkdownHighlight::HighlightingStyle> highlightingStyles; // 高亮样式表
    QString previousText;                               // 上一次的文本（用于优化）
    QTextCharFormat spellFormat;                        // 拼写错误的文字格式（红色波浪线）
    hunspell::SpellChecker *spellChecker;               // 拼写检查器
    bool spellingCheckEnabled;                          // 是否开启拼写检查
    bool yamlHeaderSupportEnabled;                      // 是否支持 YAML 头部语法
};

#endif // MARKDOWNHIGHLIGHTER_H

