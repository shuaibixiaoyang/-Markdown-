// 文件说明：app-static\rendering\renderingstylemanager.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef RENDERINGSTYLEMANAGER_H
#define RENDERINGSTYLEMANAGER_H

#include <QObject>
#include <QColor>
#include <QFont>
#include <QString>
#include <QSettings>

class RenderingStyleManager : public QObject
{
    Q_OBJECT

public:
    struct ColorScheme {
        QColor backgroundColor;
        QColor nodeBackground;
        QColor nodeBorder;
        QColor nodeText;
        QColor edgeColor;
        QColor highlightColor;
        QColor errorColor;
        QColor successColor;
        QColor warningColor;
    };

    struct FontSettings {
        QFont diagramFont;
        QFont diagramLabelFont;
        QFont mathFont;
    };

    static RenderingStyleManager& instance();

    // Theme management
    QString currentTheme() const;
    void loadTheme(const QString &themeName);
    void saveSettings();

    // Color access
    ColorScheme colorScheme() const;
    void setColor(const QString &colorName, const QColor &color);

    // Font access
    FontSettings fontSettings() const;
    void setFont(const QString &fontName, const QFont &font);

    // Theme factories
    static ColorScheme createLightTheme();
    static ColorScheme createDarkTheme();
    static ColorScheme createHighContrastTheme();

signals:
    void themeChanged(const QString &themeName);
    void colorsChanged();
    void fontsChanged();

private:
    RenderingStyleManager();
    ~RenderingStyleManager();
    Q_DISABLE_COPY(RenderingStyleManager)

    QString currentTheme_;
    ColorScheme colorScheme_;
    FontSettings fontSettings_;
};

#endif // RENDERINGSTYLEMANAGER_H

