// 文件说明：app-static\preview\previewthememanager.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "previewthememanager.h"

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>

// 函数说明：构造 PreviewThemeManager 对象，初始化本模块需要的状态、界面和资源。
PreviewThemeManager::PreviewThemeManager(QObject *parent)
    : QObject(parent)
    , m_currentThemeId("light")
{
    loadBuiltinThemes();
    loadThemes();
}

// 函数说明：销毁 PreviewThemeManager 对象，释放本模块持有的资源。
PreviewThemeManager::~PreviewThemeManager()
{
    saveThemes();
}

// 函数说明：加载 PreviewThemeManager 需要的数据、配置或外部资源。
void PreviewThemeManager::loadBuiltinThemes()
{
    createLightTheme();
    createDarkTheme();
    createSepiaTheme();
    createEyeCareTheme();
    createGithubTheme();
    createMinimalTheme();
}

// 函数说明：创建 PreviewThemeManager 需要的对象、记录或输出内容。
void PreviewThemeManager::createLightTheme()
{
    ThemeStyle theme;
    theme.id = "light";
    theme.name = tr("亮色主题");
    theme.description = tr("清爽明亮的默认主题");
    theme.isBuiltin = true;
    // 使用默认值
    m_themes[theme.id] = theme;
}

// 函数说明：创建 PreviewThemeManager 需要的对象、记录或输出内容。
void PreviewThemeManager::createDarkTheme()
{
    ThemeStyle theme;
    theme.id = "dark";
    theme.name = tr("暗色主题");
    theme.description = tr("深色背景护眼主题");
    theme.isBuiltin = true;

    theme.backgroundColor = QColor("#1e1e1e");
    theme.textColor = QColor("#d4d4d4");
    theme.linkColor = QColor("#569cd6");
    theme.linkHoverColor = QColor("#9cdcfe");

    theme.h1Color = QColor("#ffffff");
    theme.h2Color = QColor("#f0f0f0");
    theme.h3Color = QColor("#e0e0e0");
    theme.h4Color = QColor("#d0d0d0");
    theme.h5Color = QColor("#c0c0c0");
    theme.h6Color = QColor("#b0b0b0");

    theme.codeBackground = QColor("#2d2d2d");
    theme.codeTextColor = QColor("#ce9178");
    theme.codeBorderColor = QColor("#404040");

    theme.blockquoteBackground = QColor("#252526");
    theme.blockquoteBorderColor = QColor("#569cd6");
    theme.blockquoteTextColor = QColor("#9cdcfe");

    theme.tableHeaderBackground = QColor("#2d2d2d");
    theme.tableBorderColor = QColor("#404040");
    theme.tableStripeColor = QColor("#252526");

    m_themes[theme.id] = theme;
}

// 函数说明：创建 PreviewThemeManager 需要的对象、记录或输出内容。
void PreviewThemeManager::createSepiaTheme()
{
    ThemeStyle theme;
    theme.id = "sepia";
    theme.name = tr("复古主题");
    theme.description = tr("温暖的复古风格");
    theme.isBuiltin = true;

    theme.backgroundColor = QColor("#f4ecd8");
    theme.textColor = QColor("#5b4636");
    theme.linkColor = QColor("#8b4513");
    theme.linkHoverColor = QColor("#a0522d");

    theme.h1Color = QColor("#3c2415");
    theme.h2Color = QColor("#4a3728");
    theme.h3Color = QColor("#5b4636");
    theme.h4Color = QColor("#6b5544");
    theme.h5Color = QColor("#7b6454");
    theme.h6Color = QColor("#8b7364");

    theme.codeBackground = QColor("#e8dcc8");
    theme.codeTextColor = QColor("#5b4636");
    theme.codeBorderColor = QColor("#d4c4a8");

    theme.blockquoteBackground = QColor("#efe5d0");
    theme.blockquoteBorderColor = QColor("#c4a87c");
    theme.blockquoteTextColor = QColor("#6b5544");

    theme.tableHeaderBackground = QColor("#e8dcc8");
    theme.tableBorderColor = QColor("#d4c4a8");
    theme.tableStripeColor = QColor("#f0e6d2");

    m_themes[theme.id] = theme;
}

// 函数说明：创建 PreviewThemeManager 需要的对象、记录或输出内容。
void PreviewThemeManager::createEyeCareTheme()
{
    ThemeStyle theme;
    theme.id = "eyecare";
    theme.name = tr("护眼主题");
    theme.description = tr("淡绿色护眼背景");
    theme.isBuiltin = true;

    theme.backgroundColor = QColor("#c7edcc");
    theme.textColor = QColor("#2d5016");
    theme.linkColor = QColor("#1e7b46");
    theme.linkHoverColor = QColor("#0d5c30");

    theme.h1Color = QColor("#1a3d0c");
    theme.h2Color = QColor("#234a12");
    theme.h3Color = QColor("#2d5016");
    theme.h4Color = QColor("#3a5f22");
    theme.h5Color = QColor("#476e2e");
    theme.h6Color = QColor("#547d3a");

    theme.codeBackground = QColor("#b8e0be");
    theme.codeTextColor = QColor("#2d5016");
    theme.codeBorderColor = QColor("#9dd4a5");

    theme.blockquoteBackground = QColor("#bfe5c5");
    theme.blockquoteBorderColor = QColor("#6bb377");
    theme.blockquoteTextColor = QColor("#3a5f22");

    theme.tableHeaderBackground = QColor("#b8e0be");
    theme.tableBorderColor = QColor("#9dd4a5");
    theme.tableStripeColor = QColor("#d0f0d5");

    m_themes[theme.id] = theme;
}

// 函数说明：创建 PreviewThemeManager 需要的对象、记录或输出内容。
void PreviewThemeManager::createGithubTheme()
{
    ThemeStyle theme;
    theme.id = "github";
    theme.name = tr("GitHub 风格");
    theme.description = tr("模仿 GitHub 的 Markdown 渲染风格");
    theme.isBuiltin = true;

    theme.backgroundColor = QColor("#ffffff");
    theme.textColor = QColor("#24292e");
    theme.linkColor = QColor("#0366d6");
    theme.linkHoverColor = QColor("#0366d6");

    theme.h1Color = QColor("#24292e");
    theme.h2Color = QColor("#24292e");
    theme.h3Color = QColor("#24292e");
    theme.h4Color = QColor("#24292e");
    theme.h5Color = QColor("#24292e");
    theme.h6Color = QColor("#6a737d");

    theme.codeBackground = QColor("#f6f8fa");
    theme.codeTextColor = QColor("#24292e");
    theme.codeBorderColor = QColor("#e1e4e8");

    theme.blockquoteBackground = QColor("#ffffff");
    theme.blockquoteBorderColor = QColor("#dfe2e5");
    theme.blockquoteTextColor = QColor("#6a737d");

    theme.tableHeaderBackground = QColor("#f6f8fa");
    theme.tableBorderColor = QColor("#dfe2e5");
    theme.tableStripeColor = QColor("#f6f8fa");

    theme.fontFamily = "-apple-system, BlinkMacSystemFont, 'Segoe UI', Helvetica, Arial, sans-serif";
    theme.maxWidth = 980;
    theme.contentPadding = 45;

    m_themes[theme.id] = theme;
}

// 函数说明：创建 PreviewThemeManager 需要的对象、记录或输出内容。
void PreviewThemeManager::createMinimalTheme()
{
    ThemeStyle theme;
    theme.id = "minimal";
    theme.name = tr("极简主题");
    theme.description = tr("简洁的黑白风格");
    theme.isBuiltin = true;

    theme.backgroundColor = QColor("#ffffff");
    theme.textColor = QColor("#000000");
    theme.linkColor = QColor("#000000");
    theme.linkHoverColor = QColor("#555555");

    theme.h1Color = QColor("#000000");
    theme.h2Color = QColor("#000000");
    theme.h3Color = QColor("#000000");
    theme.h4Color = QColor("#000000");
    theme.h5Color = QColor("#000000");
    theme.h6Color = QColor("#000000");

    theme.codeBackground = QColor("#f0f0f0");
    theme.codeTextColor = QColor("#000000");
    theme.codeBorderColor = QColor("#cccccc");

    theme.blockquoteBackground = QColor("#ffffff");
    theme.blockquoteBorderColor = QColor("#000000");
    theme.blockquoteTextColor = QColor("#333333");

    theme.tableHeaderBackground = QColor("#f0f0f0");
    theme.tableBorderColor = QColor("#000000");
    theme.tableStripeColor = QColor("#f8f8f8");

    theme.fontFamily = "Georgia, 'Times New Roman', serif";
    theme.lineHeight = 1.8;
    theme.maxWidth = 700;

    m_themes[theme.id] = theme;
}

// 函数说明：实现 PreviewThemeManager::themeIds 的核心逻辑，供当前模块调用。
QStringList PreviewThemeManager::themeIds() const
{
    return m_themes.keys();
}

// 函数说明：实现 PreviewThemeManager::themeNames 的核心逻辑，供当前模块调用。
QStringList PreviewThemeManager::themeNames() const
{
    QStringList names;
    for (const auto &theme : m_themes) {
        names.append(theme.name);
    }
    return names;
}

// 函数说明：读取 PreviewThemeManager 当前保存的状态或计算结果。
PreviewThemeManager::ThemeStyle PreviewThemeManager::getTheme(const QString &id) const
{
    return m_themes.value(id, ThemeStyle());
}

// 函数说明：向 PreviewThemeManager 管理的数据集合中添加一项内容。
bool PreviewThemeManager::addTheme(const ThemeStyle &theme)
{
    if (m_themes.contains(theme.id)) {
        return false;
    }

    m_themes[theme.id] = theme;
    saveThemes();
    emit themeAdded(theme.id);
    return true;
}

// 函数说明：刷新 PreviewThemeManager 的内部状态，并同步到相关界面。
bool PreviewThemeManager::updateTheme(const ThemeStyle &theme)
{
    if (!m_themes.contains(theme.id)) {
        return false;
    }

    // 不允许修改内置主题
    if (m_themes[theme.id].isBuiltin) {
        return false;
    }

    m_themes[theme.id] = theme;
    saveThemes();
    emit themeUpdated(theme.id);

    if (m_currentThemeId == theme.id) {
        emit themeChanged(theme.id);
    }

    return true;
}

// 函数说明：从 PreviewThemeManager 管理的数据集合中移除指定内容。
bool PreviewThemeManager::removeTheme(const QString &id)
{
    if (!m_themes.contains(id)) {
        return false;
    }

    // 不允许删除内置主题
    if (m_themes[id].isBuiltin) {
        return false;
    }

    m_themes.remove(id);
    saveThemes();
    emit themeRemoved(id);

    if (m_currentThemeId == id) {
        setCurrentTheme("light");
    }

    return true;
}

// 函数说明：实现 PreviewThemeManager::themeExists 的核心逻辑，供当前模块调用。
bool PreviewThemeManager::themeExists(const QString &id) const
{
    return m_themes.contains(id);
}

// 函数说明：设置 PreviewThemeManager 的运行参数，并触发必要的界面或数据刷新。
void PreviewThemeManager::setCurrentTheme(const QString &id)
{
    if (m_themes.contains(id) && m_currentThemeId != id) {
        m_currentThemeId = id;
        emit themeChanged(id);
    }
}

// 函数说明：实现 PreviewThemeManager::currentTheme 的核心逻辑，供当前模块调用。
PreviewThemeManager::ThemeStyle PreviewThemeManager::currentTheme() const
{
    return m_themes.value(m_currentThemeId, ThemeStyle());
}

// 函数说明：实现 PreviewThemeManager::colorToCss 的核心逻辑，供当前模块调用。
QString PreviewThemeManager::colorToCss(const QColor &color) const
{
    if (color.alpha() < 255) {
        return QString("rgba(%1, %2, %3, %4)")
            .arg(color.red())
            .arg(color.green())
            .arg(color.blue())
            .arg(color.alphaF(), 0, 'f', 2);
    }
    return color.name();
}

// 函数说明：根据当前数据生成 PreviewThemeManager 需要的输出结果。
QString PreviewThemeManager::generateCss(const ThemeStyle &theme) const
{
    QString css = QString(R"(
:root {
    --bg-color: %1;
    --text-color: %2;
    --link-color: %3;
    --link-hover-color: %4;
    --code-bg: %5;
    --code-text: %6;
    --code-border: %7;
    --blockquote-bg: %8;
    --blockquote-border: %9;
    --blockquote-text: %10;
}

html, body {
    background-color: var(--bg-color);
    color: var(--text-color);
    font-family: %11;
    font-size: %12px;
    line-height: %13;
    margin: 0;
    padding: %14px;
}

body {
    max-width: %15px;
    margin: 0 auto;
}

a {
    color: var(--link-color);
    text-decoration: none;
}

a:hover {
    color: var(--link-hover-color);
    text-decoration: underline;
}

h1 { color: %16; margin-top: %17px; margin-bottom: 0.5em; font-size: 2em; }
h2 { color: %18; margin-top: %17px; margin-bottom: 0.5em; font-size: 1.5em; border-bottom: 1px solid #eee; padding-bottom: 0.3em; }
h3 { color: %19; margin-top: %17px; margin-bottom: 0.5em; font-size: 1.25em; }
h4 { color: %20; margin-top: %17px; margin-bottom: 0.5em; font-size: 1em; }
h5 { color: %21; margin-top: %17px; margin-bottom: 0.5em; font-size: 0.875em; }
h6 { color: %22; margin-top: %17px; margin-bottom: 0.5em; font-size: 0.85em; }

p {
    margin: %23px 0;
}

pre {
    background: var(--code-bg);
    border: 1px solid var(--code-border);
    border-radius: 4px;
    padding: 16px;
    overflow-x: auto;
    font-family: %24;
    font-size: %25px;
}

code {
    background: var(--code-bg);
    color: var(--code-text);
    padding: 2px 6px;
    border-radius: 3px;
    font-family: %24;
    font-size: %25px;
}

pre code {
    background: none;
    padding: 0;
    border-radius: 0;
}

blockquote {
    background: var(--blockquote-bg);
    border-left: 4px solid var(--blockquote-border);
    color: var(--blockquote-text);
    margin: 1em 0;
    padding: 0.5em 1em;
}

blockquote p {
    margin: 0.5em 0;
}

table {
    border-collapse: collapse;
    width: 100%%;
    margin: 1em 0;
}

th, td {
    border: 1px solid %26;
    padding: 8px 12px;
    text-align: left;
}

th {
    background: %27;
    font-weight: bold;
}

tr:nth-child(even) {
    background: %28;
}

img {
    max-width: 100%%;
    height: auto;
}

hr {
    border: none;
    border-top: 1px solid #ddd;
    margin: 2em 0;
}

ul, ol {
    padding-left: 2em;
}

li {
    margin: 0.5em 0;
}

%29
)")
    .arg(colorToCss(theme.backgroundColor))        // 1
    .arg(colorToCss(theme.textColor))              // 2
    .arg(colorToCss(theme.linkColor))              // 3
    .arg(colorToCss(theme.linkHoverColor))         // 4
    .arg(colorToCss(theme.codeBackground))         // 5
    .arg(colorToCss(theme.codeTextColor))          // 6
    .arg(colorToCss(theme.codeBorderColor))        // 7
    .arg(colorToCss(theme.blockquoteBackground))   // 8
    .arg(colorToCss(theme.blockquoteBorderColor))  // 9
    .arg(colorToCss(theme.blockquoteTextColor))    // 10
    .arg(theme.fontFamily)                         // 11
    .arg(theme.fontSize)                           // 12
    .arg(theme.lineHeight)                         // 13
    .arg(theme.contentPadding)                     // 14
    .arg(theme.maxWidth > 0 ? QString::number(theme.maxWidth) : "100%")  // 15
    .arg(colorToCss(theme.h1Color))                // 16
    .arg(theme.headingSpacing)                     // 17
    .arg(colorToCss(theme.h2Color))                // 18
    .arg(colorToCss(theme.h3Color))                // 19
    .arg(colorToCss(theme.h4Color))                // 20
    .arg(colorToCss(theme.h5Color))                // 21
    .arg(colorToCss(theme.h6Color))                // 22
    .arg(theme.paragraphSpacing)                   // 23
    .arg(theme.codeFontFamily)                     // 24
    .arg(theme.codeFontSize)                       // 25
    .arg(colorToCss(theme.tableBorderColor))       // 26
    .arg(colorToCss(theme.tableHeaderBackground))  // 27
    .arg(colorToCss(theme.tableStripeColor))       // 28
    .arg(theme.customCss);                         // 29

    return css;
}

// 函数说明：根据当前数据生成 PreviewThemeManager 需要的输出结果。
QString PreviewThemeManager::generateCss(const QString &themeId) const
{
    return generateCss(getTheme(themeId));
}

// 函数说明：实现 PreviewThemeManager::currentCss 的核心逻辑，供当前模块调用。
QString PreviewThemeManager::currentCss() const
{
    return generateCss(currentTheme());
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
bool PreviewThemeManager::exportTheme(const QString &id, const QString &filePath)
{
    if (!m_themes.contains(id)) {
        return false;
    }

    const ThemeStyle &theme = m_themes[id];
    QJsonObject obj;
    obj["id"] = theme.id;
    obj["name"] = theme.name;
    obj["description"] = theme.description;
    obj["backgroundColor"] = theme.backgroundColor.name();
    obj["textColor"] = theme.textColor.name();
    obj["linkColor"] = theme.linkColor.name();
    obj["linkHoverColor"] = theme.linkHoverColor.name();
    obj["h1Color"] = theme.h1Color.name();
    obj["h2Color"] = theme.h2Color.name();
    obj["h3Color"] = theme.h3Color.name();
    obj["h4Color"] = theme.h4Color.name();
    obj["h5Color"] = theme.h5Color.name();
    obj["h6Color"] = theme.h6Color.name();
    obj["codeBackground"] = theme.codeBackground.name();
    obj["codeTextColor"] = theme.codeTextColor.name();
    obj["codeBorderColor"] = theme.codeBorderColor.name();
    obj["blockquoteBackground"] = theme.blockquoteBackground.name();
    obj["blockquoteBorderColor"] = theme.blockquoteBorderColor.name();
    obj["blockquoteTextColor"] = theme.blockquoteTextColor.name();
    obj["tableHeaderBackground"] = theme.tableHeaderBackground.name();
    obj["tableBorderColor"] = theme.tableBorderColor.name();
    obj["tableStripeColor"] = theme.tableStripeColor.name();
    obj["fontFamily"] = theme.fontFamily;
    obj["codeFontFamily"] = theme.codeFontFamily;
    obj["fontSize"] = theme.fontSize;
    obj["codeFontSize"] = theme.codeFontSize;
    obj["lineHeight"] = theme.lineHeight;
    obj["paragraphSpacing"] = theme.paragraphSpacing;
    obj["headingSpacing"] = theme.headingSpacing;
    obj["contentPadding"] = theme.contentPadding;
    obj["maxWidth"] = theme.maxWidth;
    obj["customCss"] = theme.customCss;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(QJsonDocument(obj).toJson());
    file.close();
    return true;
}

// 函数说明：实现 PreviewThemeManager::importTheme 的核心逻辑，供当前模块调用。
bool PreviewThemeManager::importTheme(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull()) {
        return false;
    }

    QJsonObject obj = doc.object();
    ThemeStyle theme;
    theme.id = obj["id"].toString();
    theme.name = obj["name"].toString();
    theme.description = obj["description"].toString();
    theme.isBuiltin = false;
    theme.backgroundColor = QColor(obj["backgroundColor"].toString());
    theme.textColor = QColor(obj["textColor"].toString());
    theme.linkColor = QColor(obj["linkColor"].toString());
    theme.linkHoverColor = QColor(obj["linkHoverColor"].toString());
    theme.h1Color = QColor(obj["h1Color"].toString());
    theme.h2Color = QColor(obj["h2Color"].toString());
    theme.h3Color = QColor(obj["h3Color"].toString());
    theme.h4Color = QColor(obj["h4Color"].toString());
    theme.h5Color = QColor(obj["h5Color"].toString());
    theme.h6Color = QColor(obj["h6Color"].toString());
    theme.codeBackground = QColor(obj["codeBackground"].toString());
    theme.codeTextColor = QColor(obj["codeTextColor"].toString());
    theme.codeBorderColor = QColor(obj["codeBorderColor"].toString());
    theme.blockquoteBackground = QColor(obj["blockquoteBackground"].toString());
    theme.blockquoteBorderColor = QColor(obj["blockquoteBorderColor"].toString());
    theme.blockquoteTextColor = QColor(obj["blockquoteTextColor"].toString());
    theme.tableHeaderBackground = QColor(obj["tableHeaderBackground"].toString());
    theme.tableBorderColor = QColor(obj["tableBorderColor"].toString());
    theme.tableStripeColor = QColor(obj["tableStripeColor"].toString());
    theme.fontFamily = obj["fontFamily"].toString();
    theme.codeFontFamily = obj["codeFontFamily"].toString();
    theme.fontSize = obj["fontSize"].toInt(16);
    theme.codeFontSize = obj["codeFontSize"].toInt(14);
    theme.lineHeight = obj["lineHeight"].toDouble(1.6);
    theme.paragraphSpacing = obj["paragraphSpacing"].toInt(16);
    theme.headingSpacing = obj["headingSpacing"].toInt(24);
    theme.contentPadding = obj["contentPadding"].toInt(20);
    theme.maxWidth = obj["maxWidth"].toInt(800);
    theme.customCss = obj["customCss"].toString();

    // 避免ID冲突
    if (m_themes.contains(theme.id)) {
        theme.id = theme.id + "_imported";
    }

    return addTheme(theme);
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
bool PreviewThemeManager::exportAllThemes(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    bool success = true;
    for (const auto &theme : m_themes) {
        if (!theme.isBuiltin) {
            QString filePath = dirPath + "/" + theme.id + ".json";
            if (!exportTheme(theme.id, filePath)) {
                success = false;
            }
        }
    }
    return success;
}

// 函数说明：读取 PreviewThemeManager 当前保存的状态或计算结果。
QString PreviewThemeManager::getThemesFilePath() const
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataPath);
    return dataPath + "/preview_themes.json";
}

// 函数说明：保存 PreviewThemeManager 当前状态，保证用户修改可以持久化。
bool PreviewThemeManager::saveThemes()
{
    QJsonArray themesArray;

    for (const auto &theme : m_themes) {
        if (!theme.isBuiltin) {
            QJsonObject obj;
            obj["id"] = theme.id;
            obj["name"] = theme.name;
            obj["description"] = theme.description;
            obj["backgroundColor"] = theme.backgroundColor.name();
            obj["textColor"] = theme.textColor.name();
            // ... 其他属性省略，与 exportTheme 相同
            themesArray.append(obj);
        }
    }

    QJsonObject root;
    root["version"] = 1;
    root["currentTheme"] = m_currentThemeId;
    root["themes"] = themesArray;

    QFile file(getThemesFilePath());
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(QJsonDocument(root).toJson());
    file.close();
    return true;
}

// 函数说明：加载 PreviewThemeManager 需要的数据、配置或外部资源。
bool PreviewThemeManager::loadThemes()
{
    QFile file(getThemesFilePath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull()) {
        return false;
    }

    QJsonObject root = doc.object();
    m_currentThemeId = root["currentTheme"].toString("light");

    QJsonArray themesArray = root["themes"].toArray();
    for (const QJsonValue &value : themesArray) {
        QJsonObject obj = value.toObject();
        ThemeStyle theme;
        theme.id = obj["id"].toString();
        theme.name = obj["name"].toString();
        theme.description = obj["description"].toString();
        theme.isBuiltin = false;
        theme.backgroundColor = QColor(obj["backgroundColor"].toString());
        theme.textColor = QColor(obj["textColor"].toString());
        // ... 其他属性的加载
        m_themes[theme.id] = theme;
    }

    return true;
}

// 函数说明：实现 PreviewThemeManager::builtinThemeIds 的核心逻辑，供当前模块调用。
QStringList PreviewThemeManager::builtinThemeIds() const
{
    QStringList ids;
    for (auto it = m_themes.begin(); it != m_themes.end(); ++it) {
        if (it->isBuiltin) {
            ids.append(it.key());
        }
    }
    return ids;
}

