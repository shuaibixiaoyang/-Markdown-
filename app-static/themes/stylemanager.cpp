// 文件说明：app-static\themes\stylemanager.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "stylemanager.h"

#include <QMap>


static const QMap<QString, QString> BUILTIN_MARKDOWN_HIGHLIGHTINGS = {
    { "Default", "default" },
    { "Solarized Light", "solarized-light" },
    { "Solarized Dark", "solarized-dark" },
    { "Clearness Dark", "clearness-dark" },
    { "Byword Dark", "byword-dark" }
};

static const QMap<QString, QString> BUILTIN_CODE_HIGHLIGHTINGS = {
    { "Default", "default" },
    { "Github", "github" },
    { "Solarized Light", "solarized_light" },
    { "Solarized Dark", "solarized_dark" }
};

static const QMap<QString, QString> BUILTIN_PREVIEW_STYLESHEETS = {
    { "Default", "qrc:/css/markdown.css" },
    { "Github", "qrc:/css/github.css" },
    { "Solarized Light", "qrc:/css/solarized-light.css" },
    { "Solarized Dark", "qrc:/css/solarized-dark.css" },
    { "Clearness", "qrc:/css/clearness.css" },
    { "Clearness Dark", "qrc:/css/clearness-dark.css" },
    { "Byword Dark", "qrc:/css/byword-dark.css" }
};

QMap<QString, QString> StyleManager::customPreviewStylesheets;


// 函数说明：实现 StyleManager::insertCustomPreviewStylesheet 的核心逻辑，供当前模块调用。
void StyleManager::insertCustomPreviewStylesheet(const QString &styleName, const QString &stylePath)
{
    customPreviewStylesheets.insert(styleName, stylePath);
}

// 函数说明：实现 StyleManager::markdownHighlightingPath 的核心逻辑，供当前模块调用。
QString StyleManager::markdownHighlightingPath(const Theme &theme)
{
    return BUILTIN_MARKDOWN_HIGHLIGHTINGS[theme.markdownHighlighting()];
}

// 函数说明：实现 StyleManager::codeHighlightingPath 的核心逻辑，供当前模块调用。
QString StyleManager::codeHighlightingPath(const Theme &theme)
{
    return BUILTIN_CODE_HIGHLIGHTINGS[theme.codeHighlighting()];
}

// 函数说明：更新预览相关状态，保持 Markdown、HTML 和目录视图一致。
QString StyleManager::previewStylesheetPath(const Theme &theme)
{
    if (customPreviewStylesheets.contains(theme.previewStylesheet())) {
        return customPreviewStylesheets[theme.previewStylesheet()];
    }

    return BUILTIN_PREVIEW_STYLESHEETS[theme.previewStylesheet()];
}

