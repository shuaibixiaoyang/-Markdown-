// 文件说明：app-static\themes\jsonthemetranslatorfactory.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef JSONTHEMETRANSLATORFACTORY_H
#define JSONTHEMETRANSLATORFACTORY_H

#include <jsontranslatorfactory.h>
#include <jsontranslator.h>

#include "themes/theme.h"
#include "themes/jsonthemetranslator.h"


template <> class JsonTranslatorFactory<Theme>
{
public:
    static JsonTranslator<Theme> *create()
    {
        return new JsonThemeTranslator();
    }
};

#endif // JSONTHEMETRANSLATORFACTORY_H


