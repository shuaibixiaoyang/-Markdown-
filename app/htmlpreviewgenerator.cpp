// 文件说明：app\htmlpreviewgenerator.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "htmlpreviewgenerator.h"

#include <QFile>

#include <converter/markdownconverter.h>
#include <converter/markdowndocument.h>
#include <converter/discountmarkdownconverter.h>
#include <converter/revealmarkdownconverter.h>

#ifdef ENABLE_HOEDOWN
#include <converter/hoedownmarkdownconverter.h>
#endif

#include <template/template.h>

#include "options.h"
#include "yamlheaderchecker.h"

// 构造函数：初始化配置、创建转换器、连接信号
HtmlPreviewGenerator::HtmlPreviewGenerator(Options *opt, QObject *parent) :
    QThread(parent),
    options(opt),
    document(0),
    converter(0)
{
    connect(options, &Options::markdownConverterChanged, this, &HtmlPreviewGenerator::markdownConverterChanged);
    markdownConverterChanged();
}

// 检查转换器是否支持某个功能
bool HtmlPreviewGenerator::isSupported(MarkdownConverter::ConverterOption option) const
{
    return converter->supportedOptions().testFlag(option);
}

// Markdown 文本变化，加入任务队列
void HtmlPreviewGenerator::markdownTextChanged(const QString &text)
{
    // 去掉 YAML 头部
    YamlHeaderChecker checker(text);
    QString actualText = checker.hasHeader() && options->isYamlHeaderSupportEnabled() ?
                             checker.body()
                                                                                      : text;

    // 加锁，把文本加入任务队列
    QMutexLocker locker(&tasksMutex);
    tasks.enqueue(actualText);
    bufferNotEmpty.wakeOne(); // 唤醒线程处理
}

// 导出完整 HTML（带样式和高亮脚本）
QString HtmlPreviewGenerator::exportHtml(const QString &styleSheet, const QString &highlightingScript)
{
    if (!document) return QString();

    QString header;
    if (!styleSheet.isEmpty()) {
        header += QString("\n<style>%1</style>").arg(styleSheet);
    }

    if (!highlightingScript.isEmpty()) {
        QString highlightStyle;
        QFile f(QString(":/scripts/highlight.js/styles/%1.css").arg(converter->templateRenderer()->codeHighlightingStyle()));
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            highlightStyle = f.readAll();
        }

        header += QString("\n<style>%1</style>").arg(highlightStyle);
        header += QString("\n<script>%1</script>").arg(highlightingScript);
        header += "\n<script>hljs.initHighlightingOnLoad();</script>";
    }

    return converter->templateRenderer()->exportAsHtml(header, converter->renderAsHtml(document), renderOptions());
}

// 开启/关闭数学公式支持
void HtmlPreviewGenerator::setMathSupportEnabled(bool enabled)
{
    options->setMathSupportEnabled(enabled);
    generateHtmlFromMarkdown(); // 重新生成 HTML
}

// 开启/关闭图表支持
void HtmlPreviewGenerator::setDiagramSupportEnabled(bool enabled)
{
    options->setDiagramSupportEnabled(enabled);
    generateHtmlFromMarkdown();
}

// 开启/关闭代码高亮
void HtmlPreviewGenerator::setCodeHighlightingEnabled(bool enabled)
{
    options->setCodeHighlightingEnabled(enabled);
    generateHtmlFromMarkdown();
}

// 设置代码高亮样式
void HtmlPreviewGenerator::setCodeHighlightingStyle(const QString &style)
{
    converter->templateRenderer()->setCodeHighlightingStyle(style);
    generateHtmlFromMarkdown();
}

// 切换 Markdown 转换器（discount / hoedown / reveal）
void HtmlPreviewGenerator::markdownConverterChanged()
{
    QString style;

    if (converter) {
        style = converter->templateRenderer()->codeHighlightingStyle();
        delete converter;
    }

    switch (options->markdownConverter()) {
#ifdef ENABLE_HOEDOWN
    case Options::HoedownMarkdownConverter:
        converter = new HoedownMarkdownConverter();
        converter->templateRenderer()->setCodeHighlightingStyle(style);
        break;
#endif

    case Options::RevealMarkdownConverter:
        converter = new RevealMarkdownConverter();
        converter->templateRenderer()->setCodeHighlightingStyle(style);
        break;

    case Options::DiscountMarkdownConverter:
    default:
        converter = new DiscountMarkdownConverter();
        converter->templateRenderer()->setCodeHighlightingStyle(style);
        break;
    }
}

// 线程主循环：处理 HTML 生成任务
void HtmlPreviewGenerator::run()
{
    forever {
        QString text;

        {
            // 加锁等待任务
            QMutexLocker locker(&tasksMutex);
            while (tasks.count() == 0) {
                bufferNotEmpty.wait(&tasksMutex);
            }

            // 只取最后一个任务，前面的丢弃
            while (!tasks.isEmpty())
                text = tasks.dequeue();
        }

        // 空任务表示退出
        if (text.isNull()) {
            return;
        }

        // 延迟处理，避免输入过快频繁生成
        this->msleep(calculateDelay(text));

        // 检查队列是否为空
        bool empty;
        {
            QMutexLocker locker(&tasksMutex);
            empty = tasks.isEmpty();
        }

        if (empty) {
            delete document;

            // 创建文档并生成 HTML 和目录
            document = converter->createDocument(text, converterOptions());
            generateHtmlFromMarkdown();
            generateTableOfContents();
        }
    }
}

// 生成 HTML 并发送结果
void HtmlPreviewGenerator::generateHtmlFromMarkdown()
{
    if (!document) return;

    QString html = converter->templateRenderer()->render(converter->renderAsHtml(document), renderOptions());
    emit htmlResultReady(html);
}

// 生成目录 TOC
void HtmlPreviewGenerator::generateTableOfContents()
{
    if (!document) return;

    QString toc = converter->renderAsTableOfContents(document);
    QString styledToc = QString("<html><head>\n<style>ul { list-style:none; padding:0; margin-left:1em; } a { text-decoration:none; }</style>\n</head><body>%1</body></html>").arg(toc);
    emit tocResultReady(styledToc);
}

// 获取 Markdown 转换选项
MarkdownConverter::ConverterOptions HtmlPreviewGenerator::converterOptions() const
{
    MarkdownConverter::ConverterOptions parserOptionFlags(MarkdownConverter::TableOfContentsOption | MarkdownConverter::NoStyleOption);

    if (options->isAutolinkEnabled())
        parserOptionFlags |= MarkdownConverter::AutolinkOption;
    if (!options->isStrikethroughEnabled())
        parserOptionFlags |= MarkdownConverter::NoStrikethroughOption;
    if (!options->isAlphabeticListsEnabled())
        parserOptionFlags |= MarkdownConverter::NoAlphaListOption;
    if (!options->isDefinitionListsEnabled())
        parserOptionFlags |= MarkdownConverter::NoDefinitionListOption;
    if (!options->isSmartyPantsEnabled())
        parserOptionFlags |= MarkdownConverter::NoSmartypantsOption;
    if (options->isFootnotesEnabled())
        parserOptionFlags |= MarkdownConverter::ExtraFootnoteOption;
    if (!options->isSuperscriptEnabled())
        parserOptionFlags |= MarkdownConverter::NoSuperscriptOption;

    return parserOptionFlags;
}

// 获取 HTML 渲染选项
Template::RenderOptions HtmlPreviewGenerator::renderOptions() const
{
    Template::RenderOptions renderOptionFlags;

    if (options->isMathSupportEnabled())
        renderOptionFlags |= Template::MathSupport;
    if (options->isMathInlineSupportEnabled())
        renderOptionFlags |= Template::MathInlineSupport;
    if (options->isDiagramSupportEnabled())
        renderOptionFlags |= Template::DiagramSupport;
    if (options->isCodeHighlightingEnabled())
        renderOptionFlags |= Template::CodeHighlighting;

    return renderOptionFlags;
}

// 根据文本长度计算延迟时间
int HtmlPreviewGenerator::calculateDelay(const QString &text)
{
    const int MIN = 50;
    const int MAX = 2000;
    return qMin(qMax(text.size() / 100, MIN), MAX);
}

