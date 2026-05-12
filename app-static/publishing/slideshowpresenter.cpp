// 文件说明：app-static\publishing\slideshowpresenter.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "slideshowpresenter.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QTimer>
#include <QPropertyAnimation>
#include <QPainter>
#include <QScreen>
#include <QApplication>
#include <QRegularExpression>
#include <QTextDocument>
#include <QGraphicsOpacityEffect>

// 函数说明：构造 SlideshowPresenter 对象，初始化本模块需要的状态、界面和资源。
SlideshowPresenter::SlideshowPresenter(QWidget *parent)
    : QWidget(parent)
    , m_slideStack(nullptr)
    , m_progressBar(nullptr)
    , m_slideNumberLabel(nullptr)
    , m_timerLabel(nullptr)
    , m_currentIndex(0)
    , m_isPresenting(false)
    , m_isAutoPlaying(false)
    , m_autoPlayTimer(new QTimer(this))
    , m_presentationTimer(new QTimer(this))
    , m_elapsedSeconds(0)
    , m_transitionAnimation(nullptr)
{
    setupUI();

    connect(m_autoPlayTimer, &QTimer::timeout, this, &SlideshowPresenter::onAutoPlayTimeout);
    connect(m_presentationTimer, &QTimer::timeout, this, &SlideshowPresenter::updateTimer);
}

// 函数说明：销毁 SlideshowPresenter 对象，释放本模块持有的资源。
SlideshowPresenter::~SlideshowPresenter()
{
}

// 函数说明：初始化 SlideshowPresenter 的 setupUI 相关界面、动作或服务连接。
void SlideshowPresenter::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 幻灯片容器
    m_slideStack = new QStackedWidget(this);
    m_slideStack->setStyleSheet("background: #1a1a1a;");
    mainLayout->addWidget(m_slideStack, 1);

    // 底部控制栏
    QWidget *controlBar = new QWidget(this);
    controlBar->setFixedHeight(40);
    controlBar->setStyleSheet("background: rgba(0,0,0,0.8);");

    QHBoxLayout *controlLayout = new QHBoxLayout(controlBar);
    controlLayout->setContentsMargins(20, 5, 20, 5);

    // 进度条
    m_progressBar = new QLabel(this);
    m_progressBar->setFixedHeight(4);
    m_progressBar->setStyleSheet("background: #4a90d9; border-radius: 2px;");

    // 页码
    m_slideNumberLabel = new QLabel("1 / 1", this);
    m_slideNumberLabel->setStyleSheet("color: white; font-size: 14px;");
    controlLayout->addWidget(m_slideNumberLabel);

    controlLayout->addStretch();

    // 计时器
    m_timerLabel = new QLabel("00:00", this);
    m_timerLabel->setStyleSheet("color: white; font-size: 14px; font-family: monospace;");
    controlLayout->addWidget(m_timerLabel);

    mainLayout->addWidget(controlBar);

    // 进度条覆盖在顶部
    m_progressBar->setParent(this);
    m_progressBar->move(0, 0);

    setStyleSheet("background: #1a1a1a;");
}

// 函数说明：加载 SlideshowPresenter 需要的数据、配置或外部资源。
bool SlideshowPresenter::loadFromMarkdown(const QString &markdown)
{
    m_slides.clear();
    parseMarkdownToSlides(markdown);

    if (m_slides.isEmpty()) {
        return false;
    }

    // 为每个幻灯片创建显示组件
    while (m_slideStack->count() > 0) {
        QWidget *widget = m_slideStack->widget(0);
        m_slideStack->removeWidget(widget);
        delete widget;
    }

    for (const Slide &slide : m_slides) {
        QLabel *slideLabel = new QLabel(this);
        slideLabel->setAlignment(Qt::AlignCenter);
        slideLabel->setWordWrap(true);
        slideLabel->setTextFormat(Qt::RichText);
        slideLabel->setStyleSheet(
            "QLabel {"
            "  color: white;"
            "  font-size: 24px;"
            "  padding: 60px;"
            "  background: #1a1a1a;"
            "}"
        );
        slideLabel->setText(renderSlideHtml(slide));
        m_slideStack->addWidget(slideLabel);
    }

    m_currentIndex = 0;
    updateProgressBar();
    updateSlideNumber();

    return true;
}

// 函数说明：设置 SlideshowPresenter 的运行参数，并触发必要的界面或数据刷新。
void SlideshowPresenter::setSlides(const QVector<Slide> &slides)
{
    m_slides = slides;

    while (m_slideStack->count() > 0) {
        QWidget *widget = m_slideStack->widget(0);
        m_slideStack->removeWidget(widget);
        delete widget;
    }

    for (const Slide &slide : m_slides) {
        QLabel *slideLabel = new QLabel(this);
        slideLabel->setAlignment(Qt::AlignCenter);
        slideLabel->setWordWrap(true);
        slideLabel->setTextFormat(Qt::RichText);
        slideLabel->setText(renderSlideHtml(slide));
        m_slideStack->addWidget(slideLabel);
    }

    m_currentIndex = 0;
    updateProgressBar();
    updateSlideNumber();
}

// 函数说明：设置 SlideshowPresenter 的运行参数，并触发必要的界面或数据刷新。
void SlideshowPresenter::setOptions(const PresentationOptions &options)
{
    m_options = options;
    applyTheme();

    m_progressBar->setVisible(m_options.showProgress);
    m_slideNumberLabel->setVisible(m_options.showSlideNumber);
    m_timerLabel->setVisible(m_options.showTimer);

    m_autoPlayTimer->setInterval(m_options.autoPlayInterval * 1000);
}

// 函数说明：启动 SlideshowPresenter 的异步任务、会话或后台流程。
void SlideshowPresenter::startPresentation()
{
    if (m_slides.isEmpty()) return;

    m_isPresenting = true;
    m_currentIndex = 0;
    m_elapsedSeconds = 0;

    // 使用无边框窗口覆盖整个屏幕（兼容 macOS 和 Windows）
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        setGeometry(screen->geometry());
    }

    show();
    raise();
    activateWindow();

    // 确保焦点在演示窗口上（接收键盘事件）
    setFocus();

    // 开始计时
    if (m_options.showTimer) {
        m_presentationTimer->start(1000);
    }

    // 自动播放
    if (m_options.enableAutoPlay) {
        startAutoPlay();
    }

    m_slideStack->setCurrentIndex(0);
    updateProgressBar();
    updateSlideNumber();
    emit presentationStarted();
}

// 函数说明：停止 SlideshowPresenter 正在运行的任务或会话。
void SlideshowPresenter::stopPresentation()
{
    m_isPresenting = false;

    m_presentationTimer->stop();
    stopAutoPlay();

    // 恢复窗口标志并关闭
    setWindowFlags(Qt::Widget);
    showNormal();
    hide();

    emit presentationEnded();
}

// 函数说明：实现 SlideshowPresenter::goToSlide 的核心逻辑，供当前模块调用。
void SlideshowPresenter::goToSlide(int index)
{
    if (index < 0 || index >= m_slides.size()) return;

    int oldIndex = m_currentIndex;
    m_currentIndex = index;

    applyTransition();
    m_slideStack->setCurrentIndex(m_currentIndex);

    updateProgressBar();
    updateSlideNumber();

    if (oldIndex != m_currentIndex) {
        emit slideChanged(m_currentIndex);
    }
}

// 函数说明：实现 SlideshowPresenter::nextSlide 的核心逻辑，供当前模块调用。
void SlideshowPresenter::nextSlide()
{
    if (m_currentIndex < m_slides.size() - 1) {
        goToSlide(m_currentIndex + 1);
    }
}

// 函数说明：实现 SlideshowPresenter::previousSlide 的核心逻辑，供当前模块调用。
void SlideshowPresenter::previousSlide()
{
    if (m_currentIndex > 0) {
        goToSlide(m_currentIndex - 1);
    }
}

// 函数说明：实现 SlideshowPresenter::firstSlide 的核心逻辑，供当前模块调用。
void SlideshowPresenter::firstSlide()
{
    goToSlide(0);
}

// 函数说明：实现 SlideshowPresenter::lastSlide 的核心逻辑，供当前模块调用。
void SlideshowPresenter::lastSlide()
{
    goToSlide(m_slides.size() - 1);
}

// 函数说明：启动 SlideshowPresenter 的异步任务、会话或后台流程。
void SlideshowPresenter::startAutoPlay()
{
    m_isAutoPlaying = true;
    m_autoPlayTimer->start();
}

// 函数说明：停止 SlideshowPresenter 正在运行的任务或会话。
void SlideshowPresenter::stopAutoPlay()
{
    m_isAutoPlaying = false;
    m_autoPlayTimer->stop();
}

// 函数说明：切换 SlideshowPresenter 对应功能的启用状态。
void SlideshowPresenter::toggleAutoPlay()
{
    if (m_isAutoPlaying) {
        stopAutoPlay();
    } else {
        startAutoPlay();
    }
}

// 函数说明：处理键盘事件，把快捷键或输入转成编辑器动作。
void SlideshowPresenter::keyPressEvent(QKeyEvent *event)
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
        case Qt::Key_Backspace:
        case Qt::Key_PageUp:
        case Qt::Key_P:
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
            stopPresentation();
            break;

        case Qt::Key_F:
            if (isFullScreen()) {
                showNormal();
            } else {
                showFullScreen();
            }
            break;

        case Qt::Key_A:
            toggleAutoPlay();
            break;

        default:
            // 数字键直接跳转
            if (event->key() >= Qt::Key_1 && event->key() <= Qt::Key_9) {
                int slideNum = event->key() - Qt::Key_0;
                if (slideNum <= m_slides.size()) {
                    goToSlide(slideNum - 1);
                }
            }
            break;
    }

    QWidget::keyPressEvent(event);
}

// 函数说明：处理鼠标事件，更新选择、拖拽或交互状态。
void SlideshowPresenter::mousePressEvent(QMouseEvent *event)
{
    // 左键下一页，右键上一页
    if (event->button() == Qt::LeftButton) {
        nextSlide();
    } else if (event->button() == Qt::RightButton) {
        previousSlide();
    }

    QWidget::mousePressEvent(event);
}

// 函数说明：实现 SlideshowPresenter::wheelEvent 的核心逻辑，供当前模块调用。
void SlideshowPresenter::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() < 0) {
        nextSlide();
    } else {
        previousSlide();
    }

    QWidget::wheelEvent(event);
}

// 函数说明：响应尺寸变化，重新计算 SlideshowPresenter 的布局。
void SlideshowPresenter::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // 更新进度条宽度
    updateProgressBar();
}

// 函数说明：绘制 SlideshowPresenter 的可视区域或辅助标记。
void SlideshowPresenter::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
}

// 函数说明：响应 SlideshowPresenter 收到的信号或异步回调，并更新界面状态。
void SlideshowPresenter::onAutoPlayTimeout()
{
    if (m_currentIndex < m_slides.size() - 1) {
        nextSlide();
    } else {
        // 循环播放
        firstSlide();
    }
}

// 函数说明：刷新 SlideshowPresenter 的内部状态，并同步到相关界面。
void SlideshowPresenter::updateTimer()
{
    m_elapsedSeconds++;
    int minutes = m_elapsedSeconds / 60;
    int seconds = m_elapsedSeconds % 60;
    m_timerLabel->setText(QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0')));
}

// 函数说明：解析输入内容，转换为 SlideshowPresenter 后续处理使用的数据结构。
void SlideshowPresenter::parseMarkdownToSlides(const QString &markdown)
{
    // 分隔符: --- 或 *** 或 ___
    QRegularExpression separator("^(-{3,}|\\*{3,}|_{3,})$",
        QRegularExpression::MultilineOption);

    QStringList parts = markdown.split(separator);

    for (const QString &part : parts) {
        QString trimmed = part.trimmed();
        if (trimmed.isEmpty()) continue;

        Slide slide;

        // 提取标题
        QRegularExpression titleRegex("^(#{1,6})\\s+(.+)$",
            QRegularExpression::MultilineOption);
        QRegularExpressionMatch titleMatch = titleRegex.match(trimmed);

        if (titleMatch.hasMatch()) {
            slide.level = titleMatch.captured(1).length();
            slide.title = titleMatch.captured(2);
        }

        // 提取演讲者备注（以 Note: 或 备注: 开头的段落）
        QRegularExpression notesRegex("(?:^Note:|^备注:|^Notes:)(.+?)(?=^#|$)",
            QRegularExpression::MultilineOption | QRegularExpression::DotMatchesEverythingOption);
        QRegularExpressionMatch notesMatch = notesRegex.match(trimmed);

        if (notesMatch.hasMatch()) {
            slide.notes = notesMatch.captured(1).trimmed();
            trimmed.remove(notesMatch.captured(0));
        }

        // 简单的 Markdown 转 HTML
        slide.content = markdownToSlideHtml(trimmed);

        m_slides.append(slide);
    }

    // 如果没有找到分隔符，按标题分页
    if (m_slides.isEmpty() || m_slides.size() == 1) {
        m_slides.clear();

        QRegularExpression headingRegex("^(#{1,2})\\s+(.+)$",
            QRegularExpression::MultilineOption);
        QRegularExpressionMatchIterator it = headingRegex.globalMatch(markdown);

        int lastPos = 0;
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();

            if (lastPos > 0) {
                // 保存上一个幻灯片的内容
                QString content = markdown.mid(lastPos, match.capturedStart() - lastPos);
                if (!m_slides.isEmpty()) {
                    m_slides.last().content = markdownToSlideHtml(content);
                }
            }

            Slide slide;
            slide.level = match.captured(1).length();
            slide.title = match.captured(2);
            m_slides.append(slide);

            lastPos = match.capturedStart();
        }

        // 最后一个幻灯片
        if (!m_slides.isEmpty() && lastPos < markdown.length()) {
            m_slides.last().content = markdownToSlideHtml(markdown.mid(lastPos));
        }
    }
}

// 函数说明：渲染 SlideshowPresenter 的显示内容或导出片段。
QString SlideshowPresenter::renderSlideHtml(const Slide &slide)
{
    QString html;

    // 标题
    if (!slide.title.isEmpty()) {
        int fontSize = 48 - (slide.level - 1) * 8;
        html += QString("<h1 style='font-size: %1px; margin-bottom: 40px; color: #fff;'>%2</h1>")
            .arg(fontSize)
            .arg(slide.title);
    }

    // 内容
    html += QString("<div style='font-size: 24px; line-height: 1.6; color: #ddd;'>%1</div>")
        .arg(slide.content);

    return html;
}

// 函数说明：实现 SlideshowPresenter::markdownToSlideHtml 的核心逻辑，供当前模块调用。
QString SlideshowPresenter::markdownToSlideHtml(const QString &markdown)
{
    QString html = markdown;

    // 标题
    html.replace(QRegularExpression("^###### (.+)$", QRegularExpression::MultilineOption),
        "<h6 style='font-size: 18px; color: #fff;'>\\1</h6>");
    html.replace(QRegularExpression("^##### (.+)$", QRegularExpression::MultilineOption),
        "<h5 style='font-size: 20px; color: #fff;'>\\1</h5>");
    html.replace(QRegularExpression("^#### (.+)$", QRegularExpression::MultilineOption),
        "<h4 style='font-size: 24px; color: #fff;'>\\1</h4>");
    html.replace(QRegularExpression("^### (.+)$", QRegularExpression::MultilineOption),
        "<h3 style='font-size: 28px; color: #fff;'>\\1</h3>");
    html.replace(QRegularExpression("^## (.+)$", QRegularExpression::MultilineOption),
        "<h2 style='font-size: 36px; color: #fff;'>\\1</h2>");
    html.replace(QRegularExpression("^# (.+)$", QRegularExpression::MultilineOption),
        "<h1 style='font-size: 48px; color: #fff;'>\\1</h1>");

    // 无序列表
    html.replace(QRegularExpression("^[\\*\\-\\+] (.+)$", QRegularExpression::MultilineOption),
        "<li style='margin: 10px 0;'>\\1</li>");

    // 粗体和斜体
    html.replace(QRegularExpression("\\*\\*(.+?)\\*\\*"), "<strong>\\1</strong>");
    html.replace(QRegularExpression("\\*(.+?)\\*"), "<em>\\1</em>");

    // 代码
    html.replace(QRegularExpression("`([^`]+)`"),
        "<code style='background: #333; padding: 2px 6px; border-radius: 3px;'>\\1</code>");

    // 图片
    html.replace(QRegularExpression("!\\[([^\\]]*)\\]\\(([^\\)]+)\\)"),
        "<img src='\\2' alt='\\1' style='max-width: 80%; max-height: 60vh;'/>");

    // 换行
    html.replace("\n\n", "<br/><br/>");

    return html;
}

// 函数说明：应用 SlideshowPresenter 当前配置，让编辑器或预览立即生效。
void SlideshowPresenter::applyTheme()
{
    QString bgColor, textColor;

    if (m_options.theme == "light") {
        bgColor = "#ffffff";
        textColor = "#333333";
    } else if (m_options.theme == "dark") {
        bgColor = "#1a1a1a";
        textColor = "#ffffff";
    } else {
        bgColor = "#1a1a1a";
        textColor = "#ffffff";
    }

    setStyleSheet(QString("background: %1;").arg(bgColor));
    m_slideStack->setStyleSheet(QString("background: %1;").arg(bgColor));
}

// 函数说明：应用 SlideshowPresenter 当前配置，让编辑器或预览立即生效。
void SlideshowPresenter::applyTransition()
{
    if (m_options.transitionType == "none") return;

    QWidget *currentWidget = m_slideStack->currentWidget();
    if (!currentWidget) return;

    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(currentWidget);
    currentWidget->setGraphicsEffect(effect);

    QPropertyAnimation *animation = new QPropertyAnimation(effect, "opacity", this);
    animation->setDuration(m_options.transitionDuration);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

// 函数说明：刷新 SlideshowPresenter 的内部状态，并同步到相关界面。
void SlideshowPresenter::updateProgressBar()
{
    if (m_slides.isEmpty()) return;

    float progress = (float)(m_currentIndex + 1) / m_slides.size();
    int width = this->width() * progress;
    m_progressBar->setFixedWidth(width);
}

// 函数说明：刷新 SlideshowPresenter 的内部状态，并同步到相关界面。
void SlideshowPresenter::updateSlideNumber()
{
    m_slideNumberLabel->setText(QString("%1 / %2")
        .arg(m_currentIndex + 1)
        .arg(m_slides.size()));
}


// ==================== PresenterView ====================

PresenterView::PresenterView(SlideshowPresenter *presenter, QWidget *parent)
    : QWidget(parent)
    , m_presenter(presenter)
{
    setupUI();

    connect(m_presenter, &SlideshowPresenter::slideChanged,
            this, &PresenterView::updateView);
}

// 函数说明：销毁 PresenterView 对象，释放本模块持有的资源。
PresenterView::~PresenterView()
{
}

// 函数说明：初始化 PresenterView 的 setupUI 相关界面、动作或服务连接。
void PresenterView::setupUI()
{
    setWindowTitle(tr("演讲者视图"));
    setMinimumSize(800, 600);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);

    // 左侧：当前幻灯片预览
    QVBoxLayout *leftLayout = new QVBoxLayout();

    QLabel *currentLabel = new QLabel(tr("当前幻灯片"), this);
    currentLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    leftLayout->addWidget(currentLabel);

    m_currentSlidePreview = new QLabel(this);
    m_currentSlidePreview->setMinimumSize(400, 300);
    m_currentSlidePreview->setStyleSheet("background: #1a1a1a; border: 1px solid #333;");
    m_currentSlidePreview->setAlignment(Qt::AlignCenter);
    leftLayout->addWidget(m_currentSlidePreview);

    // 进度
    m_progressLabel = new QLabel(this);
    m_progressLabel->setStyleSheet("font-size: 16px;");
    leftLayout->addWidget(m_progressLabel);

    mainLayout->addLayout(leftLayout);

    // 右侧：下一张预览 + 备注
    QVBoxLayout *rightLayout = new QVBoxLayout();

    QLabel *nextLabel = new QLabel(tr("下一张"), this);
    nextLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    rightLayout->addWidget(nextLabel);

    m_nextSlidePreview = new QLabel(this);
    m_nextSlidePreview->setMinimumSize(300, 200);
    m_nextSlidePreview->setStyleSheet("background: #1a1a1a; border: 1px solid #333;");
    m_nextSlidePreview->setAlignment(Qt::AlignCenter);
    rightLayout->addWidget(m_nextSlidePreview);

    QLabel *notesLabel = new QLabel(tr("备注"), this);
    notesLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    rightLayout->addWidget(notesLabel);

    m_notesLabel = new QLabel(this);
    m_notesLabel->setWordWrap(true);
    m_notesLabel->setStyleSheet("background: #f5f5f5; padding: 10px; border: 1px solid #ddd;");
    m_notesLabel->setMinimumHeight(150);
    rightLayout->addWidget(m_notesLabel);

    // 计时器
    m_timerLabel = new QLabel("00:00", this);
    m_timerLabel->setStyleSheet("font-size: 48px; font-family: monospace; text-align: center;");
    m_timerLabel->setAlignment(Qt::AlignCenter);
    rightLayout->addWidget(m_timerLabel);

    mainLayout->addLayout(rightLayout);
}

// 函数说明：刷新 PresenterView 的内部状态，并同步到相关界面。
void PresenterView::updateView()
{
    int current = m_presenter->currentSlideIndex();
    int total = m_presenter->slideCount();

    m_progressLabel->setText(QString("%1 / %2").arg(current + 1).arg(total));

    const QVector<SlideshowPresenter::Slide> &slides = m_presenter->slides();

    // 当前幻灯片
    if (current < slides.size()) {
        m_currentSlidePreview->setText(QString("<h3>%1</h3>")
            .arg(slides[current].title));
        m_notesLabel->setText(slides[current].notes.isEmpty() ?
            tr("(无备注)") : slides[current].notes);
    }

    // 下一张
    if (current + 1 < slides.size()) {
        m_nextSlidePreview->setText(QString("<h4>%1</h4>")
            .arg(slides[current + 1].title));
    } else {
        m_nextSlidePreview->setText(tr("(演示结束)"));
    }
}

