// 文件说明：app-static\revealviewsynchronizer.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "revealviewsynchronizer.h"

#include <QPlainTextEdit>
#include <QTextBlock>
#include "compat/webenginecompat.h"
#ifndef NO_WEBENGINE
#include <QWebChannel>
#endif

#include "slidelinemapping.h"


// 函数说明：构造 RevealViewSynchronizer 对象，初始化本模块需要的状态、界面和资源。
RevealViewSynchronizer::RevealViewSynchronizer(QWebEngineView *webView, QPlainTextEdit *editor) :
    ViewSynchronizer(webView, editor),
    currentSlide(qMakePair(0, 0)),
    slideLineMapping(new SlideLineMapping())
{
    connect(webView, &QWebEngineView::loadFinished,
            this, &RevealViewSynchronizer::registerEvents);

#ifndef NO_WEBENGINE
    // 设置 WebChannel 用于 JavaScript 与 C++ 交互
    QWebChannel *channel = new QWebChannel(this);
    channel->registerObject(QStringLiteral("synchronizer"), this);
    webView->page()->setWebChannel(channel);
#endif

    connect(editor, &QPlainTextEdit::cursorPositionChanged,
            this, &RevealViewSynchronizer::cursorPositionChanged);
    connect(editor, &QPlainTextEdit::textChanged,
            this, &RevealViewSynchronizer::textChanged);

    textChanged();
}

// 函数说明：销毁 RevealViewSynchronizer 对象，释放本模块持有的资源。
RevealViewSynchronizer::~RevealViewSynchronizer()
{
    delete slideLineMapping;
}

// 函数说明：实现 RevealViewSynchronizer::horizontalSlide 的核心逻辑，供当前模块调用。
int RevealViewSynchronizer::horizontalSlide() const
{
    return currentSlide.first;
}

// 函数说明：实现 RevealViewSynchronizer::verticalSlide 的核心逻辑，供当前模块调用。
int RevealViewSynchronizer::verticalSlide() const
{
    return currentSlide.second;
}

// 函数说明：实现 RevealViewSynchronizer::slideChanged 的核心逻辑，供当前模块调用。
void RevealViewSynchronizer::slideChanged(int horizontal, int vertical)
{
    if (currentSlide.first == horizontal && currentSlide.second == vertical)
        return;

    currentSlide = qMakePair(horizontal, vertical);

    int lineNumber = slideLineMapping->lineForSlide(currentSlide);
    if (lineNumber > 0) {
        gotoLine(lineNumber);
    }
}

// 函数说明：实现 RevealViewSynchronizer::registerEvents 的核心逻辑，供当前模块调用。
void RevealViewSynchronizer::registerEvents()
{
    // 注入 QWebChannel JavaScript API
    QString initScript = 
        "new QWebChannel(qt.webChannelTransport, function(channel) {"
        "  window.synchronizer = channel.objects.synchronizer;"
        "  var mainWinUpdate = false;"
        "  function feedbackPosition(event) {"
        "    if (mainWinUpdate) return;"
        "    synchronizer.slideChanged(event.indexh, event.indexv);"
        "  }"
        "  if (typeof Reveal !== 'undefined') {"
        "    Reveal.addEventListener('ready', function() {"
        "      Reveal.addEventListener('slidechanged', feedbackPosition);"
        "    });"
        "  }"
        "  synchronizer.gotoSlideRequested.connect(function(horizontal, vertical) {"
        "    mainWinUpdate = true;"
        "    if (typeof Reveal !== 'undefined') {"
        "      Reveal.slide(horizontal, vertical);"
        "    }"
        "    mainWinUpdate = false;"
        "  });"
        "  // Restore slide position"
        "  window.location.hash = '/' + synchronizer.horizontalSlide + '/' + synchronizer.verticalSlide;"
        "});";
    m_webView->page()->runJavaScript(initScript);
}

// 函数说明：实现 RevealViewSynchronizer::restoreSlidePosition 的核心逻辑，供当前模块调用。
void RevealViewSynchronizer::restoreSlidePosition()
{
    QString restorePosition =
        "if (typeof synchronizer !== 'undefined') {"
        "  window.location.hash = '/' + synchronizer.horizontalSlide + '/' + synchronizer.verticalSlide;"
        "}";
    m_webView->page()->runJavaScript(restorePosition);
}

// 函数说明：实现 RevealViewSynchronizer::cursorPositionChanged 的核心逻辑，供当前模块调用。
void RevealViewSynchronizer::cursorPositionChanged()
{
    int lineNumber = m_editor->textCursor().blockNumber() + 1;

    QPair<int, int> slide = slideLineMapping->slideForLine(lineNumber);
    if (slide.first >= 0 && slide.second >= 0) {
        gotoSlide(slide);
    }
}

// 函数说明：实现 RevealViewSynchronizer::textChanged 的核心逻辑，供当前模块调用。
void RevealViewSynchronizer::textChanged()
{
    QString code = m_editor->toPlainText();
    slideLineMapping->build(code);
}

// 函数说明：实现 RevealViewSynchronizer::gotoLine 的核心逻辑，供当前模块调用。
void RevealViewSynchronizer::gotoLine(int lineNumber)
{
    QTextCursor cursor(m_editor->document()->findBlockByNumber(lineNumber-1));
    m_editor->setTextCursor(cursor);
}

// 函数说明：实现 RevealViewSynchronizer::gotoSlide 的核心逻辑，供当前模块调用。
void RevealViewSynchronizer::gotoSlide(QPair<int, int> slide)
{
    if (currentSlide.first == slide.first && currentSlide.second == slide.second)
        return;

    currentSlide = slide;
    emit gotoSlideRequested(slide.first, slide.second);
}

