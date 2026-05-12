// 文件说明：app-static\rendering\renderingstylemanager.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "renderingstylemanager.h"

// 函数说明：实现 RenderingStyleManager::instance 的核心逻辑，供当前模块调用。
RenderingStyleManager& RenderingStyleManager::instance()
{
    static RenderingStyleManager instance;
    return instance;
}

// 函数说明：构造 RenderingStyleManager 对象，初始化本模块需要的状态、界面和资源。
RenderingStyleManager::RenderingStyleManager()
    : currentTheme_("light")
{
    colorScheme_ = createLightTheme();
    
    fontSettings_.diagramFont = QFont("Arial", 10);
    fontSettings_.diagramLabelFont = QFont("Arial", 9);
    fontSettings_.mathFont = QFont("Times New Roman", 12);
}

RenderingStyleManager::~RenderingStyleManager() = default;

// 函数说明：实现 RenderingStyleManager::currentTheme 的核心逻辑，供当前模块调用。
QString RenderingStyleManager::currentTheme() const
{
    return currentTheme_;
}

// 函数说明：加载 RenderingStyleManager 需要的数据、配置或外部资源。
void RenderingStyleManager::loadTheme(const QString &themeName)
{
    currentTheme_ = themeName;
    
    if (themeName == "dark") {
        colorScheme_ = createDarkTheme();
    } else if (themeName == "high-contrast") {
        colorScheme_ = createHighContrastTheme();
    } else {
        colorScheme_ = createLightTheme();
    }
    
    emit themeChanged(themeName);
    emit colorsChanged();
}

// 函数说明：保存 RenderingStyleManager 当前状态，保证用户修改可以持久化。
void RenderingStyleManager::saveSettings()
{
    QSettings settings;
    settings.beginGroup("RenderingStyle");
    settings.setValue("theme", currentTheme_);
    settings.endGroup();
}

// 函数说明：实现 RenderingStyleManager::colorScheme 的核心逻辑，供当前模块调用。
RenderingStyleManager::ColorScheme RenderingStyleManager::colorScheme() const
{
    return colorScheme_;
}

// 函数说明：设置 RenderingStyleManager 的运行参数，并触发必要的界面或数据刷新。
void RenderingStyleManager::setColor(const QString &colorName, const QColor &color)
{
    if (colorName == "backgroundColor") {
        colorScheme_.backgroundColor = color;
    } else if (colorName == "nodeBackground") {
        colorScheme_.nodeBackground = color;
    } else if (colorName == "nodeBorder") {
        colorScheme_.nodeBorder = color;
    } else if (colorName == "nodeText") {
        colorScheme_.nodeText = color;
    } else if (colorName == "edgeColor") {
        colorScheme_.edgeColor = color;
    }
    
    emit colorsChanged();
}

// 函数说明：实现 RenderingStyleManager::fontSettings 的核心逻辑，供当前模块调用。
RenderingStyleManager::FontSettings RenderingStyleManager::fontSettings() const
{
    return fontSettings_;
}

// 函数说明：设置 RenderingStyleManager 的运行参数，并触发必要的界面或数据刷新。
void RenderingStyleManager::setFont(const QString &fontName, const QFont &font)
{
    if (fontName == "diagramFont") {
        fontSettings_.diagramFont = font;
    } else if (fontName == "diagramLabelFont") {
        fontSettings_.diagramLabelFont = font;
    } else if (fontName == "mathFont") {
        fontSettings_.mathFont = font;
    }
    
    emit fontsChanged();
}

// 函数说明：创建 RenderingStyleManager 需要的对象、记录或输出内容。
RenderingStyleManager::ColorScheme RenderingStyleManager::createLightTheme()
{
    ColorScheme scheme;
    scheme.backgroundColor = QColor("#ffffff");
    scheme.nodeBackground = QColor("#f5f5f5");
    scheme.nodeBorder = QColor("#333333");
    scheme.nodeText = QColor("#000000");
    scheme.edgeColor = QColor("#666666");
    scheme.highlightColor = QColor("#4a90d9");
    scheme.errorColor = QColor("#e74c3c");
    scheme.successColor = QColor("#27ae60");
    scheme.warningColor = QColor("#f39c12");
    return scheme;
}

// 函数说明：创建 RenderingStyleManager 需要的对象、记录或输出内容。
RenderingStyleManager::ColorScheme RenderingStyleManager::createDarkTheme()
{
    ColorScheme scheme;
    scheme.backgroundColor = QColor("#1e1e1e");
    scheme.nodeBackground = QColor("#2d2d2d");
    scheme.nodeBorder = QColor("#cccccc");
    scheme.nodeText = QColor("#ffffff");
    scheme.edgeColor = QColor("#aaaaaa");
    scheme.highlightColor = QColor("#5dade2");
    scheme.errorColor = QColor("#e74c3c");
    scheme.successColor = QColor("#2ecc71");
    scheme.warningColor = QColor("#f1c40f");
    return scheme;
}

// 函数说明：创建 RenderingStyleManager 需要的对象、记录或输出内容。
RenderingStyleManager::ColorScheme RenderingStyleManager::createHighContrastTheme()
{
    ColorScheme scheme;
    scheme.backgroundColor = QColor("#000000");
    scheme.nodeBackground = QColor("#000000");
    scheme.nodeBorder = QColor("#ffffff");
    scheme.nodeText = QColor("#ffffff");
    scheme.edgeColor = QColor("#ffffff");
    scheme.highlightColor = QColor("#00ffff");
    scheme.errorColor = QColor("#ff0000");
    scheme.successColor = QColor("#00ff00");
    scheme.warningColor = QColor("#ffff00");
    return scheme;
}

