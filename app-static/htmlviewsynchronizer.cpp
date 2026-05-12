// 文件说明：app-static\htmlviewsynchronizer.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "htmlviewsynchronizer.h"

#include <QPlainTextEdit>
#include <QScrollBar>
#include "compat/webenginecompat.h"
#include <QJsonDocument>
#include <QJsonObject>


// 函数说明：构造 HtmlViewSynchronizer 对象，初始化本模块需要的状态、界面和资源。
HtmlViewSynchronizer::HtmlViewSynchronizer(QWebEngineView *webView, QPlainTextEdit *editor) :
    ViewSynchronizer(webView, editor),
    scrollBarPos(0)
{
    // synchronize scrollbars
    connect(editor->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &HtmlViewSynchronizer::scrollValueChanged);

    // In Qt WebEngine, content size changes are handled differently
    // We connect to loadFinished to handle content size changes
    connect(webView, &QWebEngineView::loadFinished,
            this, &HtmlViewSynchronizer::htmlContentSizeChanged);
}

// 函数说明：销毁 HtmlViewSynchronizer 对象，释放本模块持有的资源。
HtmlViewSynchronizer::~HtmlViewSynchronizer()
{
}

// 函数说明：实现 HtmlViewSynchronizer::webViewScrolled 的核心逻辑，供当前模块调用。
void HtmlViewSynchronizer::webViewScrolled()
{
    // Qt WebEngine doesn't expose scroll position directly
    // Use JavaScript to get scroll position
    m_webView->page()->runJavaScript(
        "JSON.stringify({scrollTop: document.documentElement.scrollTop || document.body.scrollTop, "
        "scrollHeight: document.documentElement.scrollHeight || document.body.scrollHeight, "
        "clientHeight: document.documentElement.clientHeight || document.body.clientHeight})",
        [this](const QVariant &result) {
            QJsonDocument doc = QJsonDocument::fromJson(result.toString().toUtf8());
            if (!doc.isNull()) {
                QJsonObject obj = doc.object();
                double scrollTop = obj["scrollTop"].toDouble();
                double scrollHeight = obj["scrollHeight"].toDouble();
                double clientHeight = obj["clientHeight"].toDouble();
                double maxScroll = scrollHeight - clientHeight;
                
                if (maxScroll > 0) {
                    double ratio = scrollTop / maxScroll;
                    int editorMax = m_editor->verticalScrollBar()->maximum();
                    m_editor->verticalScrollBar()->setValue(qRound(ratio * editorMax));
                }
            }
            rememberScrollBarPos();
        });
}

// 函数说明：实现 HtmlViewSynchronizer::scrollValueChanged 的核心逻辑，供当前模块调用。
void HtmlViewSynchronizer::scrollValueChanged(int value)
{
    int textMax = m_editor->verticalScrollBar()->maximum();
    if (textMax <= 0) return;
    
    double ratio = (double)value / textMax;
    
    // Use JavaScript to scroll the web view
    QString script = QString(
        "(function() {"
        "  var scrollHeight = document.documentElement.scrollHeight || document.body.scrollHeight;"
        "  var clientHeight = document.documentElement.clientHeight || document.body.clientHeight;"
        "  var maxScroll = scrollHeight - clientHeight;"
        "  var targetScroll = maxScroll * %1;"
        "  window.scrollTo(0, targetScroll);"
        "})();"
    ).arg(ratio);
    
    m_webView->page()->runJavaScript(script);
}

// 函数说明：实现 HtmlViewSynchronizer::htmlContentSizeChanged 的核心逻辑，供当前模块调用。
void HtmlViewSynchronizer::htmlContentSizeChanged()
{
    if (scrollBarPos > 0) {
        // restore previous scrollbar position
        scrollValueChanged(scrollBarPos);
    }
}

// 函数说明：实现 HtmlViewSynchronizer::rememberScrollBarPos 的核心逻辑，供当前模块调用。
void HtmlViewSynchronizer::rememberScrollBarPos()
{
    scrollBarPos = m_editor->verticalScrollBar()->value();
}

