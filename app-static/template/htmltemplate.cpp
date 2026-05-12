// 文件说明：app-static\template\htmltemplate.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "htmltemplate.h"

#include <QFile>
#include <QRegularExpression>

// 函数说明：构造 HtmlTemplate 对象，初始化本模块需要的状态、界面和资源。
HtmlTemplate::HtmlTemplate()
{
    QFile f(":/template.html");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        htmlTemplate = f.readAll();
    }
}

// 函数说明：构造 HtmlTemplate 对象，初始化本模块需要的状态、界面和资源。
HtmlTemplate::HtmlTemplate(const QString &templateString) :
	htmlTemplate(templateString)
{
}

// 函数说明：渲染 HtmlTemplate 的显示内容或导出片段。
QString HtmlTemplate::render(const QString &body, RenderOptions options) const
{
    // add scrollbar synchronization
    options |= Template::ScrollbarSynchronization;

    QString htmlBody(body);

    // Mermaid and highlighting.js don't work nicely together
    // So we need to replace the <code> section by a <div> section
    if (options.testFlag(Template::CodeHighlighting) && options.testFlag(Template::DiagramSupport)) {
        convertDiagramCodeSectionToDiv(htmlBody);
    }

    return renderAsHtml(QString(), htmlBody, options);
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
QString HtmlTemplate::exportAsHtml(const QString &header, const QString &body, RenderOptions options) const
{
    // clear code highlighting option since it depends on the resource file
    options &= ~Template::CodeHighlighting;

    return renderAsHtml(header, body, options);
}

// 函数说明：渲染 HtmlTemplate 的显示内容或导出片段。
QString HtmlTemplate::renderAsHtml(const QString &header, const QString &body, Template::RenderOptions options) const
{
    if (htmlTemplate.isEmpty()) {
        return body;
    }

    QString htmlHeader = buildHtmlHeader(options);
    htmlHeader += header;

    return QString(htmlTemplate)
            .replace(QLatin1String("<!--__HTML_HEADER__-->"), htmlHeader)
            .replace(QLatin1String("<!--__HTML_CONTENT__-->"), body);
}

//加入一些脚本和样式
QString HtmlTemplate::buildHtmlHeader(RenderOptions options) const
{
    QString header;

    // add javascript for scrollbar synchronization
    if (options.testFlag(Template::ScrollbarSynchronization)) {
        header += "<script type=\"text/javascript\">window.onscroll = function() { synchronizer.webViewScrolled(); }; </script>\n";
    }

    // add MathJax.js script to HTML header
    if (options.testFlag(Template::MathSupport)) {

        // Add MathJax support for inline LaTeX Math
        if (options.testFlag(Template::MathInlineSupport)) {
            header += "<script type=\"text/x-mathjax-config\">MathJax.Hub.Config({tex2jax: {inlineMath: [['$','$'], ['\\\\(','\\\\)']]}});</script>";
        }

        header += "<script type=\"text/javascript\" src=\"http://cdn.mathjax.org/mathjax/latest/MathJax.js?config=TeX-AMS-MML_HTMLorMML\"></script>\n";
    }

    // add Highlight.js script to HTML header
    if (options.testFlag(Template::CodeHighlighting)) {
        header += QString("<link rel=\"stylesheet\" href=\"qrc:/scripts/highlight.js/styles/%1.css\">\n").arg(codeHighlightingStyle());
        header += "<script src=\"qrc:/scripts/highlight.js/highlight.pack.js\"></script>\n";
        header += "<script>hljs.initHighlightingOnLoad();</script>\n";
    }

    // add mermaid.js script to HTML header
    if (options.testFlag(Template::DiagramSupport)) {
        header += "<link rel=\"stylesheet\" href=\"qrc:/scripts/mermaid/mermaid.css\">\n";
        header += "<script src=\"qrc:/scripts/mermaid/mermaid.full.min.js\"></script>\n";
    }

    return header;
}

// 函数说明：实现 HtmlTemplate::convertDiagramCodeSectionToDiv 的核心逻辑，供当前模块调用。
void HtmlTemplate::convertDiagramCodeSectionToDiv(QString &body) const
{
    static const QRegularExpression rx(QStringLiteral("<pre><code class=\"mermaid\">(.*?)</code></pre>"),
                                       QRegularExpression::DotMatchesEverythingOption);
    body.replace(rx, QStringLiteral("<div class=\"mermaid\">\n\\1</div>"));
}

