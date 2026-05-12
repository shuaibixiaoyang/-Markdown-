// 文件说明：app-static\template\presentationtemplate.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "presentationtemplate.h"

#include <QFile>

// 函数说明：构造 PresentationTemplate 对象，初始化本模块需要的状态、界面和资源。
PresentationTemplate::PresentationTemplate()
{
    QFile f(":/template_presentation.html");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        presentationTemplate = f.readAll();
    }
}

// 函数说明：渲染 PresentationTemplate 的显示内容或导出片段。
QString PresentationTemplate::render(const QString &body, RenderOptions options) const
{
    if (presentationTemplate.isEmpty()) {
        return body;
    }

    return QString(presentationTemplate)
            .replace(QLatin1String("<!--__HTML_HEADER__-->"), QString())
            .replace(QLatin1String("<!--__HTML_CONTENT__-->"), body)
            .replace(QLatin1String("<!--__REVEAL_PLUGINS__-->"), buildRevealPlugins(options));
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
QString PresentationTemplate::exportAsHtml(const QString &, const QString &body, RenderOptions options) const
{
    return render(body, options);
}

// 函数说明：实现 PresentationTemplate::buildRevealPlugins 的核心逻辑，供当前模块调用。
QString PresentationTemplate::buildRevealPlugins(RenderOptions options) const
{
    QString plugins;

    // add MathJax.js script as reveal plugin
    if (options.testFlag(Template::MathSupport)) {
        plugins += "{ src: 'https://cdn.jsdelivr.net/reveal.js/2.6.2/plugin/math/math.js', async: true },\n";
    }

    // add Highlight.js script as reveal plugin
    if (options.testFlag(Template::CodeHighlighting)) {
        plugins += "{ src: 'https://cdn.jsdelivr.net/reveal.js/2.6.2/plugin/highlight/highlight.js', async: true, callback: function() { hljs.initHighlightingOnLoad(); } },\n";
    }

    return plugins;
}

