// 文件说明：app-static\themes\jsonthemetranslator.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef JSONTHEMETRANSLATOR_H
#define JSONTHEMETRANSLATOR_H

#include <jsontranslator.h>
#include <QJsonObject>
#include "theme.h"


class JsonThemeTranslator : public JsonTranslator<Theme>
{
private:
    Theme fromJsonObject(const QJsonObject &object);
    QJsonObject toJsonObject(const Theme &theme);
};

#endif // JSONTHEMETRANSLATOR_H



