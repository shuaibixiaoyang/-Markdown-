// 文件说明：libs\jsonconfig\jsontranslatorfactory.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef JSONTRANSLATORFACTORY_H
#define JSONTRANSLATORFACTORY_H

#include "jsontranslator.h"


template <class T>
class JsonTranslatorFactory
{
public:
    static JsonTranslator<T> *create() { return 0; }
};

#endif // JSONTRANSLATORFACTORY_H


