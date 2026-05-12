// 文件说明：app-static\snippets\jsonsnippettranslator.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "jsonsnippettranslator.h"

#include "snippet.h"

namespace {

static const QLatin1String TRIGGER("trigger");
static const QLatin1String DESCRIPTION("description");
static const QLatin1String SNIPPET("snippet");
static const QLatin1String CURSOR("cursor");
static const QLatin1String BUILTIN("builtIn");

}


// 函数说明：实现 JsonSnippetTranslator::fromJsonObject 的核心逻辑，供当前模块调用。
Snippet JsonSnippetTranslator::fromJsonObject(const QJsonObject &object)
{
    Snippet snippet;

    snippet.trigger = object.value(TRIGGER).toString();
    snippet.description = object.value(DESCRIPTION).toString();
    snippet.snippet = object.value(SNIPPET).toString();
    snippet.cursorPosition = object.value(CURSOR).toDouble();
    snippet.builtIn = object.value(BUILTIN).toBool();

    return snippet;
}

// 函数说明：实现 JsonSnippetTranslator::toJsonObject 的核心逻辑，供当前模块调用。
QJsonObject JsonSnippetTranslator::toJsonObject(const Snippet &snippet)
{
    QJsonObject object;

    object.insert(TRIGGER, snippet.trigger);
    object.insert(DESCRIPTION, snippet.description);
    object.insert(SNIPPET, snippet.snippet);
    object.insert(CURSOR, snippet.cursorPosition);
    object.insert(BUILTIN, snippet.builtIn);

    return object;
}

