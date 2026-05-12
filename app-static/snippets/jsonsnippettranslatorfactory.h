// 文件说明：app-static\snippets\jsonsnippettranslatorfactory.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef JSONSNIPPETTRANSLATORFACTORY_H
#define JSONSNIPPETTRANSLATORFACTORY_H

#include <jsontranslatorfactory.h>
#include <jsontranslator.h>

#include "snippets/snippet.h"
#include "snippets/jsonsnippettranslator.h"


template <> class JsonTranslatorFactory<Snippet>
{
public:
    static JsonTranslator<Snippet> *create()
    {
        return new JsonSnippetTranslator();
    }
};

#endif // JSONSNIPPETTRANSLATORFACTORY_H


