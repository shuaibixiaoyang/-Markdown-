// 文件说明：app-static\themes\jsonthemetranslator.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "jsonthemetranslator.h"

namespace {

static const QLatin1String NAME("name");
static const QLatin1String MARKDOWN_HIGHLIGHTING("markdownHighlighting");
static const QLatin1String CODE_HIGHLIGHTING("codeHighlighting");
static const QLatin1String PREVIEW_STYLESHEET("previewStylesheet");
static const QLatin1String BUILT_IN("builtIn");

}

// 函数说明：实现 JsonThemeTranslator::fromJsonObject 的核心逻辑，供当前模块调用。
Theme JsonThemeTranslator::fromJsonObject(const QJsonObject &object)
{
    QString name = object.value(NAME).toString();
    QString markdownHighlighting = object.value(MARKDOWN_HIGHLIGHTING).toString();
    QString codeHighlighting = object.value(CODE_HIGHLIGHTING).toString();
    QString previewStylesheet = object.value(PREVIEW_STYLESHEET).toString();
    bool builtIn = object.value(BUILT_IN).toBool();

    return { name, markdownHighlighting, codeHighlighting, previewStylesheet, builtIn };
}

// 函数说明：实现 JsonThemeTranslator::toJsonObject 的核心逻辑，供当前模块调用。
QJsonObject JsonThemeTranslator::toJsonObject(const Theme &theme)
{
    QJsonObject object;
    object.insert(NAME, theme.name());
    object.insert(MARKDOWN_HIGHLIGHTING, theme.markdownHighlighting());
    object.insert(CODE_HIGHLIGHTING, theme.codeHighlighting());
    object.insert(PREVIEW_STYLESHEET, theme.previewStylesheet());
    object.insert(BUILT_IN, theme.isBuiltIn());

    return object;
}


