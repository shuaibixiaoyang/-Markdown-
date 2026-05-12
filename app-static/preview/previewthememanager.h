// 文件说明：app-static\preview\previewthememanager.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef PREVIEWTHEMEMANAGER_H
#define PREVIEWTHEMEMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QColor>
#include <QFont>

/**
 * @brief 预览主题管理器
 *
 * 功能：
 * - 内置主题（亮色、暗色、护眼等）
 * - 自定义主题编辑
 * - 主题导入导出
 * - 实时预览切换
 */
class PreviewThemeManager : public QObject
{
    Q_OBJECT

public:
    // 主题样式配置
    struct ThemeStyle {
        QString id;                 // 主题ID
        QString name;               // 主题名称
        QString description;        // 描述
        bool isBuiltin;             // 是否内置

        // 背景和文字颜色
        QColor backgroundColor;     // 背景颜色
        QColor textColor;           // 正文颜色
        QColor linkColor;           // 链接颜色
        QColor linkHoverColor;      // 链接悬停颜色

        // 标题颜色
        QColor h1Color;
        QColor h2Color;
        QColor h3Color;
        QColor h4Color;
        QColor h5Color;
        QColor h6Color;

        // 代码块
        QColor codeBackground;      // 代码块背景
        QColor codeTextColor;       // 代码文字颜色
        QColor codeBorderColor;     // 代码块边框颜色

        // 引用块
        QColor blockquoteBackground;
        QColor blockquoteBorderColor;
        QColor blockquoteTextColor;

        // 表格
        QColor tableHeaderBackground;
        QColor tableBorderColor;
        QColor tableStripeColor;

        // 字体
        QString fontFamily;         // 正文字体
        QString codeFontFamily;     // 代码字体
        int fontSize;               // 正文字号
        int codeFontSize;           // 代码字号
        qreal lineHeight;           // 行高

        // 间距
        int paragraphSpacing;       // 段落间距
        int headingSpacing;         // 标题间距
        int contentPadding;         // 内容内边距
        int maxWidth;               // 最大宽度 (0 表示无限制)

        // 自定义 CSS
        QString customCss;

        ThemeStyle() :
            isBuiltin(false),
            backgroundColor("#ffffff"),
            textColor("#333333"),
            linkColor("#0066cc"),
            linkHoverColor("#004499"),
            h1Color("#111111"),
            h2Color("#222222"),
            h3Color("#333333"),
            h4Color("#444444"),
            h5Color("#555555"),
            h6Color("#666666"),
            codeBackground("#f5f5f5"),
            codeTextColor("#333333"),
            codeBorderColor("#e0e0e0"),
            blockquoteBackground("#f9f9f9"),
            blockquoteBorderColor("#ddd"),
            blockquoteTextColor("#666666"),
            tableHeaderBackground("#f0f0f0"),
            tableBorderColor("#ddd"),
            tableStripeColor("#fafafa"),
            fontFamily("-apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif"),
            codeFontFamily("'SF Mono', Consolas, Monaco, monospace"),
            fontSize(16),
            codeFontSize(14),
            lineHeight(1.6),
            paragraphSpacing(16),
            headingSpacing(24),
            contentPadding(20),
            maxWidth(800) {}
    };

    explicit PreviewThemeManager(QObject *parent = nullptr);
    ~PreviewThemeManager();

    // 主题管理
    QStringList themeIds() const;
    QStringList themeNames() const;
    ThemeStyle getTheme(const QString &id) const;
    bool addTheme(const ThemeStyle &theme);
    bool updateTheme(const ThemeStyle &theme);
    bool removeTheme(const QString &id);
    bool themeExists(const QString &id) const;

    // 当前主题
    void setCurrentTheme(const QString &id);
    QString currentThemeId() const { return m_currentThemeId; }
    ThemeStyle currentTheme() const;

    // CSS 生成
    QString generateCss(const ThemeStyle &theme) const;
    QString generateCss(const QString &themeId) const;
    QString currentCss() const;

    // 导入导出
    bool exportTheme(const QString &id, const QString &filePath);
    bool importTheme(const QString &filePath);
    bool exportAllThemes(const QString &dirPath);

    // 持久化
    bool saveThemes();
    bool loadThemes();

    // 内置主题
    void loadBuiltinThemes();
    QStringList builtinThemeIds() const;

signals:
    void themeChanged(const QString &themeId);
    void themeAdded(const QString &themeId);
    void themeUpdated(const QString &themeId);
    void themeRemoved(const QString &themeId);

private:
    void createLightTheme();
    void createDarkTheme();
    void createSepiaTheme();
    void createEyeCareTheme();
    void createGithubTheme();
    void createMinimalTheme();

    QString colorToCss(const QColor &color) const;
    QString getThemesFilePath() const;

    QMap<QString, ThemeStyle> m_themes;
    QString m_currentThemeId;
};

#endif // PREVIEWTHEMEMANAGER_H

