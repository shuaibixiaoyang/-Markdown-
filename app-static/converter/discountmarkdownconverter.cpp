// 文件说明：app-static\converter\discountmarkdownconverter.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "discountmarkdownconverter.h"

extern "C" {
#ifdef Q_OS_WIN
#include <Windows.h>
#endif
#include <mkdio.h>
}

#include "markdowndocument.h"
#include "template/htmltemplate.h"

class DiscountMarkdownDocument : public MarkdownDocument
{
public:
    explicit DiscountMarkdownDocument(MMIOT *document) : discountDocument(document) {}
    ~DiscountMarkdownDocument() { mkd_cleanup(discountDocument); }

    MMIOT *document() const { return discountDocument; }

private:
    MMIOT *discountDocument;
};


// 函数说明：构造 DiscountMarkdownConverter 对象，初始化本模块需要的状态、界面和资源。
DiscountMarkdownConverter::DiscountMarkdownConverter()
{
}

MarkdownDocument *DiscountMarkdownConverter::createDocument(const QString &text, ConverterOptions options)
{
    MMIOT *doc = 0;

    if (text.length() > 0) {
        QString markdownText(text);

        // text has to always end with a line break,
        // otherwise characters are missing in HTML
        if (!markdownText.endsWith('\n')) {
            markdownText.append('\n');
        }

        unsigned long flags = 0;
        translateConverterOptions(options, &flags);

        QByteArray utf8Data = markdownText.toUtf8();
        doc = mkd_string(utf8Data.constData(), utf8Data.length(), (mkd_flag_t)flags);

        mkd_compile(doc, (mkd_flag_t)flags);
    }

    return new DiscountMarkdownDocument(doc);
}

// 函数说明：渲染 DiscountMarkdownConverter 的显示内容或导出片段。
QString DiscountMarkdownConverter::renderAsHtml(MarkdownDocument *document)
{
    QString html;

    if (document) {
        DiscountMarkdownDocument *doc = dynamic_cast<DiscountMarkdownDocument*>(document);

        if (doc && doc->document()) {
            char *out;
            mkd_document(doc->document(), &out);

            html = QString::fromUtf8(out);
        }
    }

    return html;
}

// 函数说明：渲染 DiscountMarkdownConverter 的显示内容或导出片段。
QString DiscountMarkdownConverter::renderAsTableOfContents(MarkdownDocument *document)
{
    QString toc;

    if (document) {
        DiscountMarkdownDocument *doc = dynamic_cast<DiscountMarkdownDocument*>(document);

        if (doc && doc->document()) {
            // generate table of contents
            char *out;
            mkd_toc(doc->document(), &out);

            toc = QString::fromUtf8(out);
        }
    }

    return toc;
}

Template *DiscountMarkdownConverter::templateRenderer() const
{
    static HtmlTemplate htmlTemplate;
    return &htmlTemplate;
}

// 函数说明：实现 DiscountMarkdownConverter::supportedOptions 的核心逻辑，供当前模块调用。
MarkdownConverter::ConverterOptions DiscountMarkdownConverter::supportedOptions() const
{
    return MarkdownConverter::AutolinkOption |
           MarkdownConverter::NoStrikethroughOption |
           MarkdownConverter::NoAlphaListOption |
           MarkdownConverter::NoDefinitionListOption |
           MarkdownConverter::NoSmartypantsOption |
           MarkdownConverter::ExtraFootnoteOption |
           MarkdownConverter::NoSuperscriptOption;
}

// 函数说明：实现 DiscountMarkdownConverter::translateConverterOptions 的核心逻辑，供当前模块调用。
void DiscountMarkdownConverter::translateConverterOptions(ConverterOptions options, unsigned long *flags) const
{
    *flags |= MKD_TOC;
    *flags |= MKD_NOSTYLE;

    if (options.testFlag(MarkdownConverter::AutolinkOption)) {
        *flags |= MKD_AUTOLINK;
    }
    if (options.testFlag(MarkdownConverter::NoStrikethroughOption)) {
        *flags |= MKD_NOSTRIKETHROUGH;
    }
    if (options.testFlag(MarkdownConverter::NoAlphaListOption)) {
        *flags |= MKD_NOALPHALIST;
    }
    if (options.testFlag(MarkdownConverter::NoSmartypantsOption)) {
        *flags |= MKD_NOPANTS;
    }
    if (options.testFlag(MarkdownConverter::ExtraFootnoteOption)) {
        *flags |= MKD_EXTRA_FOOTNOTE;
    }
    if (options.testFlag(MarkdownConverter::NoSuperscriptOption)) {
        *flags |= MKD_NOSUPERSCRIPT;
    }
}

