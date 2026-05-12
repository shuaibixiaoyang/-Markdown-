// 文件说明：test\integration\htmltemplatetest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "htmltemplatetest.h"

#include <QtTest>

#include <template/htmltemplate.h>
#include "loremipsumtestdata.h"

static const QString HTML_TEMPLATE = QStringLiteral("<html><head><!--__HTML_HEADER__--></head><body><!--__HTML_CONTENT__--></body></html>");
static const QString SCROLL_SCRIPT = QStringLiteral("<script type=\"text/javascript\">window.onscroll = function() { synchronizer.webViewScrolled(); }; </script>");
static const QString MERMAID_CSS   = QStringLiteral("<link rel=\"stylesheet\" href=\"qrc:/scripts/mermaid/mermaid.css\">");
static const QString MERMAID_JS    = QStringLiteral("<script src=\"qrc:/scripts/mermaid/mermaid.full.min.js\"></script>");
static const QString HIGHLIGHT_JS  = QStringLiteral("<link rel=\"stylesheet\" href=\"qrc:/scripts/highlight.js/styles/.css\">\n<script src=\"qrc:/scripts/highlight.js/highlight.pack.js\"></script>\n<script>hljs.initHighlightingOnLoad();</script>");

// 函数说明：渲染 HtmlTemplateTest 的显示内容或导出片段。
void HtmlTemplateTest::rendersContentInsideBodyTags()
{
    HtmlTemplate htmlTemplate(HTML_TEMPLATE);

    QString html = htmlTemplate.render("<p>TEST</p>", Template::RenderOptions());

    const QString expected = QStringLiteral("<html><head>%1\n</head><body><p>TEST</p></body></html>")
        .arg(SCROLL_SCRIPT);
    QCOMPARE(html, expected);
}

// 函数说明：渲染 HtmlTemplateTest 的显示内容或导出片段。
void HtmlTemplateTest::rendersMermaidGraphInsideCodeTags()
{
    HtmlTemplate htmlTemplate(HTML_TEMPLATE);

    QString html = htmlTemplate.render("<pre><code class=\"mermaid\">TEST</code></pre>", HtmlTemplate::DiagramSupport);

    const QString expected = QStringLiteral("<html><head>%1\n%2\n%3\n</head><body><pre><code class=\"mermaid\">TEST</code></pre></body></html>")
        .arg(SCROLL_SCRIPT).arg(MERMAID_CSS).arg(MERMAID_JS);
    QCOMPARE(html, expected);
}

// 函数说明：实现 HtmlTemplateTest::replacesMermaidCodeTagsByDivTagsIfCodeHighlightingEnabled 的核心逻辑，供当前模块调用。
void HtmlTemplateTest::replacesMermaidCodeTagsByDivTagsIfCodeHighlightingEnabled()
{
    HtmlTemplate htmlTemplate(HTML_TEMPLATE);

    QString html = htmlTemplate.render("<pre><code class=\"mermaid\">TEST</code></pre>", HtmlTemplate::DiagramSupport | HtmlTemplate::CodeHighlighting);

    const QString expected = QStringLiteral("<html><head>%1\n%2\n%3\n%4\n</head><body><div class=\"mermaid\">\nTEST</div></body></html>")
        .arg(SCROLL_SCRIPT).arg(HIGHLIGHT_JS).arg(MERMAID_CSS).arg(MERMAID_JS);
    QCOMPARE(html, expected);
}

