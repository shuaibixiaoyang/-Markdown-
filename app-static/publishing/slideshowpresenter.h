// 文件说明：app-static\publishing\slideshowpresenter.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef SLIDESHOWPRESENTER_H
#define SLIDESHOWPRESENTER_H

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QKeyEvent>

class QLabel;
class QStackedWidget;
class QPropertyAnimation;
class QTimer;

/**
 * @brief 幻灯片演示器 - Markdown 转演示文稿
 *
 * 将 Markdown 文档转换为全屏幻灯片演示
 *
 * 分页规则:
 * - "---" 水平线分隔幻灯片
 * - 或按一级/二级标题自动分页
 *
 * 支持特性:
 * - 全屏演示
 * - 键盘导航
 * - 过渡动画
 * - 演讲者备注
 * - 进度指示
 * - 演讲计时器
 */
class SlideshowPresenter : public QWidget
{
    Q_OBJECT

public:
    // 幻灯片信息
    struct Slide {
        QString title;
        QString content;        // HTML 内容
        QString notes;          // 演讲者备注
        QString background;     // 背景颜色或图片
        int level;              // 标题级别
    };

    // 演示选项
    struct PresentationOptions {
        bool showProgress;          // 显示进度条
        bool showSlideNumber;       // 显示页码
        bool showTimer;             // 显示计时器
        bool enableAutoPlay;        // 自动播放
        int autoPlayInterval;       // 自动播放间隔（秒）
        QString theme;              // 主题 (light/dark/custom)
        QString transitionType;     // 过渡类型 (fade/slide/zoom/none)
        int transitionDuration;     // 过渡时长（毫秒）

        PresentationOptions()
            : showProgress(true)
            , showSlideNumber(true)
            , showTimer(false)
            , enableAutoPlay(false)
            , autoPlayInterval(5)
            , theme("dark")
            , transitionType("fade")
            , transitionDuration(300)
        {}
    };

    explicit SlideshowPresenter(QWidget *parent = nullptr);
    ~SlideshowPresenter();

    // 从 Markdown 加载
    bool loadFromMarkdown(const QString &markdown);

    // 从幻灯片列表加载
    void setSlides(const QVector<Slide> &slides);
    QVector<Slide> slides() const { return m_slides; }

    // 设置选项
    void setOptions(const PresentationOptions &options);
    PresentationOptions options() const { return m_options; }

    // 演示控制
    void startPresentation();
    void stopPresentation();
    void goToSlide(int index);
    void nextSlide();
    void previousSlide();
    void firstSlide();
    void lastSlide();

    // 状态
    int currentSlideIndex() const { return m_currentIndex; }
    int slideCount() const { return m_slides.size(); }
    bool isPresenting() const { return m_isPresenting; }

    // 自动播放
    void startAutoPlay();
    void stopAutoPlay();
    void toggleAutoPlay();

signals:
    void slideChanged(int index);
    void presentationStarted();
    void presentationEnded();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onAutoPlayTimeout();
    void updateTimer();

private:
    void setupUI();
    void parseMarkdownToSlides(const QString &markdown);
    QString renderSlideHtml(const Slide &slide);
    void applyTheme();
    void applyTransition();
    void updateProgressBar();
    void updateSlideNumber();
    QString markdownToSlideHtml(const QString &markdown);

    // UI 组件
    QStackedWidget *m_slideStack;
    QLabel *m_progressBar;
    QLabel *m_slideNumberLabel;
    QLabel *m_timerLabel;

    // 数据
    QVector<Slide> m_slides;
    PresentationOptions m_options;

    // 状态
    int m_currentIndex;
    bool m_isPresenting;
    bool m_isAutoPlaying;

    // 定时器
    QTimer *m_autoPlayTimer;
    QTimer *m_presentationTimer;
    int m_elapsedSeconds;

    // 动画
    QPropertyAnimation *m_transitionAnimation;
};


/**
 * @brief 演讲者视图 - 显示备注和预览
 */
class PresenterView : public QWidget
{
    Q_OBJECT

public:
    explicit PresenterView(SlideshowPresenter *presenter, QWidget *parent = nullptr);
    ~PresenterView();

    void updateView();

private:
    void setupUI();

    SlideshowPresenter *m_presenter;
    QLabel *m_currentSlidePreview;
    QLabel *m_nextSlidePreview;
    QLabel *m_notesLabel;
    QLabel *m_timerLabel;
    QLabel *m_progressLabel;
};

#endif // SLIDESHOWPRESENTER_H

