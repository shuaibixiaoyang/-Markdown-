// 文件说明：app-static\converter\hoedownmarkdownconverter.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "hoedownmarkdownconverter.h"

extern "C" {
#if __has_include(<hoedown/html.h>)
#include <hoedown/html.h>
#include <hoedown/markdown.h>
#elif __has_include(<src/html.h>)
#include <src/html.h>
#include <src/markdown.h>
#else
#include <html.h>
#include <markdown.h>
#endif
}

#include "markdowndocument.h"
#include "template/htmltemplate.h"

class HoedownMarkdownDocument : public MarkdownDocument
{
public:
    explicit HoedownMarkdownDocument(hoedown_buffer *document, unsigned long options) :
        hoedownDocument(document),
        converterOptions(options)
    {}
    ~HoedownMarkdownDocument() { hoedown_buffer_free(hoedownDocument); }

    hoedown_buffer *document() const { return hoedownDocument; }
    unsigned long options() const { return converterOptions; }

private:
    hoedown_buffer *hoedownDocument;
    unsigned long converterOptions;
};


// 函数说明：构造 HoedownMarkdownConverter 对象，初始化本模块需要的状态、界面和资源。
HoedownMarkdownConverter::HoedownMarkdownConverter()
{
}

MarkdownDocument *HoedownMarkdownConverter::createDocument(const QString &text, ConverterOptions options)
{
    hoedown_buffer *doc = 0;

    if (text.length() > 0) {
        QString markdownText(text);

        QByteArray utf8Data = markdownText.toUtf8();
        doc = hoedown_buffer_new(utf8Data.length());
        hoedown_buffer_puts(doc, utf8Data.data());
    }

    return new HoedownMarkdownDocument(doc, translateConverterOptions(options));
}

// 函数说明：渲染 HoedownMarkdownConverter 的显示内容或导出片段。
QString HoedownMarkdownConverter::renderAsHtml(MarkdownDocument *document)
{
    QString html;

    if (document) {
        HoedownMarkdownDocument *doc = dynamic_cast<HoedownMarkdownDocument*>(document);

        if (doc && doc->document()) {
            hoedown_buffer *in = doc->document();
            hoedown_buffer *out = hoedown_buffer_new(64);

            hoedown_renderer *renderer = hoedown_html_renderer_new(0, 16);
            hoedown_markdown *markdown = hoedown_markdown_new(doc->options(), 16, renderer);

            hoedown_markdown_render(out, in->data, in->size, markdown);

            hoedown_markdown_free(markdown);
            hoedown_html_renderer_free(renderer);

            html = QString::fromUtf8(hoedown_buffer_cstr(out));

            hoedown_buffer_free(out);
        }
    }

    return html;
}

// 函数说明：渲染 HoedownMarkdownConverter 的显示内容或导出片段。
QString HoedownMarkdownConverter::renderAsTableOfContents(MarkdownDocument *document)
{
    QString toc;

    if (document) {
        HoedownMarkdownDocument *doc = dynamic_cast<HoedownMarkdownDocument*>(document);

        if (doc->document()) {
            hoedown_buffer *in = doc->document();
            hoedown_buffer *out = hoedown_buffer_new(64);

            hoedown_renderer *renderer = hoedown_html_toc_renderer_new(16);
            hoedown_markdown *markdown = hoedown_markdown_new(doc->options(), 16, renderer);

            hoedown_markdown_render(out, in->data, in->size, markdown);

            hoedown_markdown_free(markdown);
            hoedown_html_renderer_free(renderer);

            toc = QString::fromUtf8(hoedown_buffer_cstr(out));

            hoedown_buffer_free(out);
        }
    }

    return toc;
}

Template *HoedownMarkdownConverter::templateRenderer() const
{
    static HtmlTemplate htmlTemplate;
    return &htmlTemplate;
}

// 函数说明：实现 HoedownMarkdownConverter::supportedOptions 的核心逻辑，供当前模块调用。
MarkdownConverter::ConverterOptions HoedownMarkdownConverter::supportedOptions() const
{
    return MarkdownConverter::AutolinkOption |
           MarkdownConverter::NoStrikethroughOption |
           MarkdownConverter::ExtraFootnoteOption |
           MarkdownConverter::NoSuperscriptOption;
}

// 函数说明：实现 HoedownMarkdownConverter::translateConverterOptions 的核心逻辑，供当前模块调用。
unsigned long HoedownMarkdownConverter::translateConverterOptions(ConverterOptions options) const
{
    unsigned long converterOptions = HOEDOWN_EXT_FENCED_CODE | HOEDOWN_EXT_TABLES;

    // autolink
    if (options.testFlag(MarkdownConverter::AutolinkOption)) {
        converterOptions |= HOEDOWN_EXT_AUTOLINK;
    }

    // strikethrough
    if (!options.testFlag(MarkdownConverter::NoStrikethroughOption)) {
        converterOptions |= HOEDOWN_EXT_STRIKETHROUGH;
    }

//    // alphabetic lists
//    if (!options->isAlphabeticListsEnabled()) {
//        converterOptions |= MKD_NOALPHALIST;
//    }

//    // definition lists
//    if (!options->isDefinitionListsEnabled()) {
//        converterOptions |= MKD_NODLIST;
//    }

//    // SmartyPants
//    if (!options->isSmartyPantsEnabled()) {
//        converterOptions |= MKD_NOPANTS;
//    }

    // Footnotes
    if (options.testFlag(MarkdownConverter::ExtraFootnoteOption)) {
        converterOptions |= HOEDOWN_EXT_FOOTNOTES;
    }

    // Superscript
    if (!options.testFlag(MarkdownConverter::NoSuperscriptOption)) {
        converterOptions |= HOEDOWN_EXT_SUPERSCRIPT;
    }

    return converterOptions;
}

