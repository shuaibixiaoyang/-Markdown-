// 文件说明：app-static\preview\presentationmode.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "presentationmode.h"

#include <QVBoxLayout>
#include <QTimer>
#include <QRegularExpression>
#include "compat/webenginecompat.h"
#include <QApplication>
#include <QScreen>

// 函数说明：构造 PresentationMode 对象，初始化本模块需要的状态、界面和资源。
PresentationMode::PresentationMode(QWidget *parent)
    : QWidget(parent)
    , m_webView(new QWebEngineView(this))
    , m_currentSlide(0)
    , m_isRunning(false)
    , m_isPaused(false)
    , m_timer(new QTimer(this))
    , m_elapsedSeconds(0)
    , m_transitionTimer(new QTimer(this))
    , m_inTransition(false)
{
    setupUi();

    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &PresentationMode::onTimerTick);

    m_transitionTimer->setSingleShot(true);
    connect(m_transitionTimer, &QTimer::timeout, this, &PresentationMode::onTransitionFinished);

    // 监听网页 title 变化，接收 JS 发来的导航命令
    connect(m_webView, &QWebEngineView::titleChanged, this, [this](const QString &title) {
        if (!m_isRunning) return;
        if (title == "CMD:NEXT") nextSlide();
        else if (title == "CMD:PREV") previousSlide();
        else if (title == "CMD:STOP") stop();
        else if (title == "CMD:FIRST") firstSlide();
        else if (title == "CMD:LAST") lastSlide();
    });
}

// 函数说明：销毁 PresentationMode 对象，释放本模块持有的资源。
PresentationMode::~PresentationMode()
{
}

// 函数说明：初始化 PresentationMode 的 setupUi 相关界面、动作或服务连接。
void PresentationMode::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_webView);

    m_webView->settings()->setAttribute(QWebEngineSettings::ShowScrollBars, false);
    m_webView->setContextMenuPolicy(Qt::NoContextMenu);

    // 安装事件过滤器到 WebView 及其子控件，捕获键盘和鼠标事件
    m_webView->installEventFilter(this);
    // WebEngineView 的实际输入由内部子控件处理，也需要过滤
    m_webView->setFocusPolicy(Qt::NoFocus);

    setStyleSheet("background-color: black;");
    setCursor(Qt::BlankCursor);
}

// 函数说明：设置 PresentationMode 的运行参数，并触发必要的界面或数据刷新。
void PresentationMode::setMarkdownContent(const QString &markdown)
{
    m_slides = parseMarkdownToSlides(markdown);
    m_currentSlide = 0;

    if (m_isRunning) {
        renderCurrentSlide();
    }
}

// 函数说明：设置 PresentationMode 的运行参数，并触发必要的界面或数据刷新。
void PresentationMode::setHtmlContent(const QString &html)
{
    // 简化处理：将整个 HTML 作为一张幻灯片
    Slide slide;
    slide.title = tr("演示文稿");
    slide.content = html;
    m_slides.clear();
    m_slides.append(slide);
    m_currentSlide = 0;

    if (m_isRunning) {
        renderCurrentSlide();
    }
}

// 函数说明：设置 PresentationMode 的运行参数，并触发必要的界面或数据刷新。
void PresentationMode::setSlides(const QVector<Slide> &slides)
{
    m_slides = slides;
    m_currentSlide = 0;

    if (m_isRunning) {
        renderCurrentSlide();
    }
}

// 函数说明：设置 PresentationMode 的运行参数，并触发必要的界面或数据刷新。
void PresentationMode::setConfig(const Config &config)
{
    m_config = config;

    if (m_isRunning) {
        renderCurrentSlide();
    }
}

// 函数说明：解析输入内容，转换为 PresentationMode 后续处理使用的数据结构。
QVector<PresentationMode::Slide> PresentationMode::parseMarkdownToSlides(const QString &markdown)
{
    QVector<Slide> slides;
    QStringList lines = markdown.split('\n');

    Slide currentSlide;
    QString currentContent;
    bool inSlide = false;
    //正则匹配标题
    QRegularExpression headingRegex("^(#{1,2})\\s+(.+)$");

    for (const QString &line : lines) {
        QRegularExpressionMatch match = headingRegex.match(line);

        if (match.hasMatch()) {
            int level = match.captured(1).length();
            QString title = match.captured(2);

            // 保存当前幻灯片
            if (inSlide) {
                currentSlide.content = currentContent.trimmed();
                slides.append(currentSlide);
            }

            // 开始新幻灯片
            currentSlide = Slide();
            currentSlide.title = title;
            currentSlide.level = level;
            currentContent.clear();
            inSlide = true;
        } else if (inSlide) {
            // 检查演讲者备注 (以 Note: 或 备注: 开头)
            if (line.trimmed().startsWith("Note:") || line.trimmed().startsWith("备注:")) {
                currentSlide.notes = line.mid(line.indexOf(':') + 1).trimmed();
            } else {
                currentContent += line + "\n";
            }
        }
    }

    // 保存最后一张幻灯片
    if (inSlide) {
        currentSlide.content = currentContent.trimmed();
        slides.append(currentSlide);
    }

    // 如果没有幻灯片，创建一张包含全部内容的幻灯片
    if (slides.isEmpty()) {
        Slide slide;
        slide.title = tr("演示文稿");
        slide.content = markdown;
        slides.append(slide);
    }

    return slides;
}

// 函数说明：启动 PresentationMode 的异步任务、会话或后台流程。
void PresentationMode::start()
{
    if (m_slides.isEmpty()) {
        return;
    }

    m_isRunning = true;
    m_isPaused = false;
    m_currentSlide = 0;
    m_elapsedSeconds = 0;

    // 使用无边框窗口覆盖整个屏幕（兼容 macOS 和 Windows）
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    // 获取主屏幕尺寸，最大化覆盖
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        setGeometry(screen->geometry());
    }

    show();
    raise();
    activateWindow();
    setFocus();

    renderCurrentSlide();

    // 为 WebView 所有子控件安装事件过滤器（WebEngine 渲染窗口是动态创建的）
    for (QObject *child : m_webView->children()) {
        child->installEventFilter(this);
    }
    // 用定时器延迟再装一次，确保捕获到 WebEngine 延迟创建的子控件
    QTimer::singleShot(500, this, [this]() {
        for (QObject *child : m_webView->children()) {
            child->installEventFilter(this);
        }
        // focusWidget 也需要安装
        if (m_webView->focusWidget()) {
            m_webView->focusWidget()->installEventFilter(this);
        }
    });

    if (m_config.enableTimer) {
        m_timer->start();
    }

    emit presentationStarted();
    emit slideChanged(m_currentSlide, m_slides.size());
}

// 函数说明：停止 PresentationMode 正在运行的任务或会话。
void PresentationMode::stop()
{
    m_isRunning = false;
    m_isPaused = false;
    m_timer->stop();

    setWindowFlags(Qt::Widget);
    showNormal();
    hide();
    emit presentationEnded();
}

// 函数说明：实现 PresentationMode::pause 的核心逻辑，供当前模块调用。
void PresentationMode::pause()
{
    m_isPaused = true;
    m_timer->stop();
}

// 函数说明：实现 PresentationMode::resume 的核心逻辑，供当前模块调用。
void PresentationMode::resume()
{
    m_isPaused = false;
    if (m_config.enableTimer) {
        m_timer->start();
    }
}

// 函数说明：实现 PresentationMode::nextSlide 的核心逻辑，供当前模块调用。
void PresentationMode::nextSlide()
{
    if (m_inTransition) return;

    if (m_currentSlide < m_slides.size() - 1) {
        m_currentSlide++;
        applyTransition(true);
    } else if (m_config.loopSlides) {
        m_currentSlide = 0;
        applyTransition(true);
    }
}

// 函数说明：实现 PresentationMode::previousSlide 的核心逻辑，供当前模块调用。
void PresentationMode::previousSlide()
{
    if (m_inTransition) return;

    if (m_currentSlide > 0) {
        m_currentSlide--;
        applyTransition(false);
    } else if (m_config.loopSlides) {
        m_currentSlide = m_slides.size() - 1;
        applyTransition(false);
    }
}

// 函数说明：实现 PresentationMode::gotoSlide 的核心逻辑，供当前模块调用。
void PresentationMode::gotoSlide(int index)
{
    if (m_inTransition) return;

    if (index >= 0 && index < m_slides.size()) {
        bool forward = index > m_currentSlide;
        m_currentSlide = index;
        applyTransition(forward);
    }
}

// 函数说明：实现 PresentationMode::firstSlide 的核心逻辑，供当前模块调用。
void PresentationMode::firstSlide()
{
    gotoSlide(0);
}

// 函数说明：实现 PresentationMode::lastSlide 的核心逻辑，供当前模块调用。
void PresentationMode::lastSlide()
{
    gotoSlide(m_slides.size() - 1);
}

// 函数说明：启动 PresentationMode 的异步任务、会话或后台流程。
void PresentationMode::startTimer()
{
    m_elapsedSeconds = 0;
    m_timer->start();
}

// 函数说明：停止 PresentationMode 正在运行的任务或会话。
void PresentationMode::stopTimer()
{
    m_timer->stop();
}

// 函数说明：实现 PresentationMode::resetTimer 的核心逻辑，供当前模块调用。
void PresentationMode::resetTimer()
{
    m_elapsedSeconds = 0;
    emit timerTick(0);
}

// 函数说明：实现 PresentationMode::elapsedSeconds 的核心逻辑，供当前模块调用。
int PresentationMode::elapsedSeconds() const
{
    return m_elapsedSeconds;
}

// 函数说明：响应 PresentationMode 收到的信号或异步回调，并更新界面状态。
void PresentationMode::onTimerTick()
{
    m_elapsedSeconds++;
    emit timerTick(m_elapsedSeconds);
}

// 函数说明：响应 PresentationMode 收到的信号或异步回调，并更新界面状态。
void PresentationMode::onTransitionFinished()
{
    m_inTransition = false;
}

// 函数说明：处理键盘事件，把快捷键或输入转成编辑器动作。
void PresentationMode::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Right:
        case Qt::Key_Down:
        case Qt::Key_Space:
        case Qt::Key_PageDown:
        case Qt::Key_N:
            nextSlide();
            break;

        case Qt::Key_Left:
        case Qt::Key_Up:
        case Qt::Key_PageUp:
        case Qt::Key_P:
        case Qt::Key_Backspace:
            previousSlide();
            break;

        case Qt::Key_Home:
            firstSlide();
            break;

        case Qt::Key_End:
            lastSlide();
            break;

        case Qt::Key_Escape:
        case Qt::Key_Q:
            stop();
            break;

        case Qt::Key_F:
            if (isFullScreen()) {
                showNormal();
            } else {
                showFullScreen();
            }
            break;

        case Qt::Key_B:
            // 黑屏
            if (m_isPaused) {
                resume();
                renderCurrentSlide();
            } else {
                pause();
                m_webView->setHtml("<html><body style='background:black;'></body></html>");
            }
            break;

        default:
            // 数字键直接跳转
            if (event->key() >= Qt::Key_1 && event->key() <= Qt::Key_9) {
                int slideNum = event->key() - Qt::Key_1;
                gotoSlide(slideNum);
            }
            break;
    }

    QWidget::keyPressEvent(event);
}

// 函数说明：处理鼠标事件，更新选择、拖拽或交互状态。
void PresentationMode::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        nextSlide();
    } else if (event->button() == Qt::RightButton) {
        previousSlide();
    }

    QWidget::mousePressEvent(event);
}

// 函数说明：实现 PresentationMode::wheelEvent 的核心逻辑，供当前模块调用。
void PresentationMode::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() < 0) {
        nextSlide();
    } else {
        previousSlide();
    }

    QWidget::wheelEvent(event);
}

// 函数说明：响应尺寸变化，重新计算 PresentationMode 的布局。
void PresentationMode::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (m_isRunning) {
        renderCurrentSlide();
    }
}

// 函数说明：应用 PresentationMode 当前配置，让编辑器或预览立即生效。
void PresentationMode::applyTransition(bool forward)
{
    if (m_config.transition == TransitionType::None) {
        renderCurrentSlide();
        emit slideChanged(m_currentSlide, m_slides.size());
        return;
    }

    m_inTransition = true;
    m_transitionTimer->start(m_config.transitionDuration);

    renderCurrentSlide();
    emit slideChanged(m_currentSlide, m_slides.size());
}

// 函数说明：渲染 PresentationMode 的显示内容或导出片段。
void PresentationMode::renderCurrentSlide()
{
    if (m_currentSlide < 0 || m_currentSlide >= m_slides.size()) {
        return;
    }

    const Slide &slide = m_slides[m_currentSlide];
    QString html = generateSlideHtml(slide);
    m_webView->setHtml(html);
}

// 函数说明：根据当前数据生成 PresentationMode 需要的输出结果。
QString PresentationMode::generateSlideHtml(const Slide &slide)
{
    QString css = generatePresentationCss();

    QString progressBar;
    if (m_config.showProgress) {
        int progress = m_slides.size() > 1 ?
            (m_currentSlide * 100) / (m_slides.size() - 1) : 100;
        progressBar = QString(
            "<div class='progress-bar'>"
            "<div class='progress' style='width: %1%;'></div>"
            "</div>").arg(progress);
    }

    QString slideNumber;
    if (m_config.showSlideNumber) {
        slideNumber = QString(
            "<div class='slide-number'>%1 / %2</div>")
            .arg(m_currentSlide + 1)
            .arg(m_slides.size());
    }

    return QString(R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <style>%1</style>
</head>
<body>
    <div class="slide">
        <h1 class="title">%2</h1>
        <div class="content">%3</div>
    </div>
    %4
    %5
    <script>
    // 点击页面 → 通知 Qt 切换下一张
    document.addEventListener('click', function(e) {
        if (e.button === 0) {
            document.title = 'CMD:NEXT';
        } else if (e.button === 2) {
            document.title = 'CMD:PREV';
        }
    });
    document.addEventListener('contextmenu', function(e) {
        e.preventDefault();
        document.title = 'CMD:PREV';
    });
    document.addEventListener('keydown', function(e) {
        if (e.key === 'Escape' || e.key === 'q') {
            document.title = 'CMD:STOP';
        } else if (e.key === 'ArrowRight' || e.key === 'ArrowDown' || e.key === ' ' || e.key === 'PageDown' || e.key === 'n') {
            document.title = 'CMD:NEXT';
        } else if (e.key === 'ArrowLeft' || e.key === 'ArrowUp' || e.key === 'Backspace' || e.key === 'PageUp' || e.key === 'p') {
            document.title = 'CMD:PREV';
        } else if (e.key === 'Home') {
            document.title = 'CMD:FIRST';
        } else if (e.key === 'End') {
            document.title = 'CMD:LAST';
        }
    });
    </script>
</body>
</html>
)")
    .arg(css)
    .arg(slide.title)
    .arg(slide.content)
    .arg(progressBar)
    .arg(slideNumber);
}

// 函数说明：根据当前数据生成 PresentationMode 需要的输出结果。
QString PresentationMode::generatePresentationCss()
{
    return QString(R"(
* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

html, body {
    width: 100%%;
    height: 100%%;
    background: %1;
    color: %2;
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
    font-size: %3px;
    overflow: hidden;
}

.slide {
    width: 100%%;
    height: 100%%;
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
    padding: 60px;
    text-align: center;
}

.title {
    font-size: 2.5em;
    font-weight: bold;
    margin-bottom: 40px;
    color: %4;
}

.content {
    font-size: 1.2em;
    line-height: 1.8;
    max-width: 80%%;
}

.content h2 {
    font-size: 1.8em;
    margin: 30px 0 20px;
    color: %4;
}

.content h3 {
    font-size: 1.4em;
    margin: 25px 0 15px;
}

.content p {
    margin: 20px 0;
}

.content ul, .content ol {
    text-align: left;
    margin: 20px auto;
    padding-left: 40px;
    max-width: 70%%;
}

.content li {
    margin: 15px 0;
}

.content code {
    background: rgba(255, 255, 255, 0.1);
    padding: 4px 10px;
    border-radius: 4px;
    font-family: 'SF Mono', Consolas, monospace;
}

.content pre {
    background: rgba(0, 0, 0, 0.3);
    padding: 20px;
    border-radius: 8px;
    margin: 20px auto;
    max-width: 80%%;
    overflow-x: auto;
    text-align: left;
}

.content blockquote {
    border-left: 4px solid %4;
    padding-left: 20px;
    margin: 20px auto;
    max-width: 70%%;
    font-style: italic;
    opacity: 0.9;
    text-align: left;
}

.content img {
    max-width: 80%%;
    max-height: 60vh;
    margin: 20px auto;
    border-radius: 8px;
}

.progress-bar {
    position: fixed;
    bottom: 0;
    left: 0;
    right: 0;
    height: 4px;
    background: rgba(255, 255, 255, 0.2);
}

.progress {
    height: 100%%;
    background: %4;
    transition: width 0.3s ease;
}

.slide-number {
    position: fixed;
    bottom: 20px;
    right: 30px;
    font-size: 16px;
    opacity: 0.6;
}

/* 过渡动画 */
.slide {
    animation: fadeIn %5ms ease-out;
}

@keyframes fadeIn {
    from { opacity: 0; transform: translateY(20px); }
    to { opacity: 1; transform: translateY(0); }
}
)")
    .arg(m_config.backgroundColor)
    .arg(m_config.textColor)
    .arg(m_config.fontSize)
    .arg(m_config.accentColor)
    .arg(m_config.transitionDuration);
}

// 函数说明：读取 PresentationMode 当前保存的状态或计算结果。
QString PresentationMode::getTransitionCss(TransitionType type, bool entering)
{
    switch (type) {
        case TransitionType::Fade:
            return entering ?
                "animation: fadeIn 0.3s ease-out;" :
                "animation: fadeOut 0.3s ease-out;";

        case TransitionType::SlideLeft:
            return entering ?
                "animation: slideInLeft 0.3s ease-out;" :
                "animation: slideOutLeft 0.3s ease-out;";

        case TransitionType::SlideRight:
            return entering ?
                "animation: slideInRight 0.3s ease-out;" :
                "animation: slideOutRight 0.3s ease-out;";

        case TransitionType::Zoom:
            return entering ?
                "animation: zoomIn 0.3s ease-out;" :
                "animation: zoomOut 0.3s ease-out;";

        default:
            return "";
    }
}

//事件过滤器，拦截并处理演示模式下的所有按键鼠标，滚轮事件
bool PresentationMode::eventFilter(QObject *obj, QEvent *event)
{
    //如果演示模式未运行，直接交给父类处理
    if (!m_isRunning) return QWidget::eventFilter(obj, event);
    //拦截键盘按下的事件
    if (event->type() == QEvent::KeyPress) {
        //转换成键盘事件
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        //交给自己的键盘处理函数
        keyPressEvent(keyEvent);
        return true;  // 事件已处理
    }
    //拦截鼠标按下事件
    if (event->type() == QEvent::MouseButtonPress) {
        //转换成鼠标事件
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        //左键 = 下一页
        if (mouseEvent->button() == Qt::LeftButton) {
            nextSlide();
            return true;
        }
        //右键=上一页
        else if (mouseEvent->button() == Qt::RightButton) {
            previousSlide();
            return true;
        }
    }
    //拦截鼠标滚轮事件
    if (event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
        //滚轮向下=下一页
        if (wheelEvent->angleDelta().y() < 0) {
            nextSlide();
        } else {
            previousSlide();
        }
        return true;
    }

    // 4. 拦截 子控件添加 事件
    // 给新创建的子控件也安装事件过滤器
    // 保证所有子控件的操作都被演示模式接管
    if (event->type() == QEvent::ChildAdded) {
        QChildEvent *childEvent = static_cast<QChildEvent*>(event);
        if (childEvent->child()) {
            childEvent->child()->installEventFilter(this);
        }
    }
    //其他事件按默认处理
    return QWidget::eventFilter(obj, event);
}

