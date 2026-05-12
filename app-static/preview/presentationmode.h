// 文件说明：app-static\preview\presentationmode.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef PRESENTATIONMODE_H
#define PRESENTATIONMODE_H

#include <QWidget>
#include "compat/webenginecompat.h"
#include <QStringList>
#include <QKeyEvent>

/**
 * @brief 演示模式
 *
 * 功能：
 * - 将 Markdown 内容按标题分割成幻灯片
 * - 全屏幻灯片展示
 * - 键盘/鼠标导航
 * - 多种过渡动画
 * - 演讲者备注支持
 * - 计时器功能
 */
class PresentationMode : public QWidget
{
    Q_OBJECT

public:
    // 过渡动画类型
    enum class TransitionType {
        None,
        Fade,
        SlideLeft,
        SlideRight,
        SlideUp,
        SlideDown,
        Zoom
    };
    Q_ENUM(TransitionType)

    // 幻灯片结构
    struct Slide {
        QString title;          // 标题
        QString content;        // HTML 内容
        QString notes;          // 演讲者备注
        int level;              // 标题级别

        Slide() : level(1) {}
    };

    // 配置
    struct Config {
        TransitionType transition;
        int transitionDuration;     // 毫秒
        QString backgroundColor;
        QString textColor;
        QString accentColor;
        int fontSize;
        bool showProgress;          // 显示进度条
        bool showSlideNumber;       // 显示幻灯片编号
        bool enableTimer;           // 启用计时器
        bool loopSlides;            // 循环播放

        Config() :
            transition(TransitionType::Fade),
            transitionDuration(300),
            backgroundColor("#1a1a2e"),
            textColor("#ffffff"),
            accentColor("#e94560"),
            fontSize(32),
            showProgress(true),
            showSlideNumber(true),
            enableTimer(false),
            loopSlides(false) {}
    };

    explicit PresentationMode(QWidget *parent = nullptr);
    ~PresentationMode();

    // 设置内容
    void setMarkdownContent(const QString &markdown);
    void setHtmlContent(const QString &html);
    void setSlides(const QVector<Slide> &slides);

    // 配置
    void setConfig(const Config &config);
    Config config() const { return m_config; }

    // 控制
    void start();
    void stop();
    void pause();
    void resume();
    bool isRunning() const { return m_isRunning; }

    // 导航
    void nextSlide();
    void previousSlide();
    void gotoSlide(int index);
    void firstSlide();
    void lastSlide();
    int currentSlideIndex() const { return m_currentSlide; }
    int slideCount() const { return m_slides.size(); }

    // 计时器
    void startTimer();
    void stopTimer();
    void resetTimer();
    int elapsedSeconds() const;

signals:
    void slideChanged(int index, int total);
    void presentationStarted();
    void presentationEnded();
    void timerTick(int seconds);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onTimerTick();
    void onTransitionFinished();

private:
    void setupUi();
    QVector<Slide> parseMarkdownToSlides(const QString &markdown);
    void renderCurrentSlide();
    QString generateSlideHtml(const Slide &slide);
    QString generatePresentationCss();
    void applyTransition(bool forward);
    QString getTransitionCss(TransitionType type, bool entering);

    QWebEngineView *m_webView;
    QVector<Slide> m_slides;
    Config m_config;

    int m_currentSlide;
    bool m_isRunning;
    bool m_isPaused;

    // 计时器
    QTimer *m_timer;
    int m_elapsedSeconds;

    // 过渡动画
    QTimer *m_transitionTimer;
    bool m_inTransition;
};

#endif // PRESENTATIONMODE_H

