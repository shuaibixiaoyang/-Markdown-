// 文件说明：app-static\editor\focusmode.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef FOCUSMODE_H
#define FOCUSMODE_H

#include <QObject>
#include <QPlainTextEdit>
#include <QColor>
#include <QPropertyAnimation>
#include <QTimer>

/**
 * @brief 焦点模式管理器
 *
 * 功能：
 * - 无干扰全屏写作模式
 * - 高亮当前段落/句子/行
 * - 淡化非焦点区域
 * - 打字机滚动模式
 * - 可配置的焦点范围
 * - 平滑过渡动画
 */
class FocusMode : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal fadeOpacity READ fadeOpacity WRITE setFadeOpacity)

public:
    // 焦点范围
    enum class FocusScope {
        Line,           // 当前行
        Sentence,       // 当前句子
        Paragraph,      // 当前段落
        Section         // 当前章节（到下一个标题）
    };
    Q_ENUM(FocusScope)

    // 滚动模式
    enum class ScrollMode {
        Normal,         // 正常滚动
        Typewriter,     // 打字机模式（光标居中）
        Smart           // 智能滚动
    };
    Q_ENUM(ScrollMode)

    // 配置选项
    struct Config {
        bool enabled;
        FocusScope scope;
        ScrollMode scrollMode;
        QColor focusColor;          // 焦点区域文字颜色
        QColor fadeColor;           // 淡化区域文字颜色
        QColor backgroundColor;     // 背景色
        qreal fadeOpacity;          // 淡化透明度 (0.0 - 1.0)
        int transitionDuration;     // 过渡动画时长 (ms)
        bool highlightCurrentLine;  // 高亮当前行背景
        QColor lineHighlightColor;  // 行高亮颜色
        bool hideInterface;         // 隐藏界面元素
        bool fullScreen;            // 全屏模式
        int marginPercent;          // 边距百分比

        Config()
            : enabled(false)
            , scope(FocusScope::Paragraph)
            , scrollMode(ScrollMode::Typewriter)
            , focusColor(Qt::black)
            , fadeColor(QColor(128, 128, 128, 100))
            , backgroundColor(Qt::white)
            , fadeOpacity(0.3)
            , transitionDuration(200)
            , highlightCurrentLine(true)
            , lineHighlightColor(QColor(255, 255, 200, 50))
            , hideInterface(true)
            , fullScreen(true)
            , marginPercent(20)
        {}
    };

    explicit FocusMode(QPlainTextEdit *editor, QObject *parent = nullptr);
    ~FocusMode();

    // 配置
    void setConfig(const Config &config);
    Config config() const { return m_config; }

    // 启用/禁用
    void enable();
    void disable();
    bool isEnabled() const { return m_config.enabled; }
    void toggle();

    // 焦点范围
    void setFocusScope(FocusScope scope);
    FocusScope focusScope() const { return m_config.scope; }

    // 滚动模式
    void setScrollMode(ScrollMode mode);
    ScrollMode scrollMode() const { return m_config.scrollMode; }

    // 样式
    void setFocusColor(const QColor &color);
    void setFadeColor(const QColor &color);
    void setBackgroundColor(const QColor &color);
    void setFadeOpacity(qreal opacity);
    qreal fadeOpacity() const { return m_currentFadeOpacity; }

    // 预设主题
    void applyLightTheme();
    void applyDarkTheme();
    void applySepiaTheme();

    // 获取焦点区域
    QPair<int, int> getFocusRange() const;  // 返回起始和结束位置

signals:
    void enabled();
    void disabled();
    void focusRangeChanged(int start, int end);
    void configChanged();

public slots:
    void updateFocus();
    void scrollToCenter();

private slots:
    void onCursorPositionChanged();
    void onTextChanged();
    void onScrollValueChanged();

private:
    // 计算焦点范围
    QPair<int, int> calculateLineRange();
    QPair<int, int> calculateSentenceRange();
    QPair<int, int> calculateParagraphRange();
    QPair<int, int> calculateSectionRange();

    // 应用样式
    void applyFocusStyle();
    void removeFocusStyle();
    void updateExtraSelections();

    // 打字机滚动
    void ensureCursorCentered();

    // 保存/恢复编辑器状态
    void saveEditorState();
    void restoreEditorState();

    QPlainTextEdit *m_editor;
    Config m_config;
    qreal m_currentFadeOpacity;
    QPropertyAnimation *m_fadeAnimation;
    QTimer *m_updateTimer;

    // 当前焦点范围
    int m_focusStart;
    int m_focusEnd;

    // 保存的编辑器状态
    struct EditorState {
        QPalette palette;
        QString styleSheet;
        bool wordWrapMode;
        qreal documentMargin;
    };
    EditorState m_savedState;
    bool m_stateSaved;
};

#endif // FOCUSMODE_H

