// 文件说明：app-static\snippets\jsonsnippettranslator.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef JSONSNIPPETTRANSLATOR_H
#define JSONSNIPPETTRANSLATOR_H

#include <jsontranslator.h>
#include <QJsonObject>
struct Snippet;


class JsonSnippetTranslator : public JsonTranslator<Snippet>
{
private:
    Snippet fromJsonObject(const QJsonObject &object) override;
    QJsonObject toJsonObject(const Snippet &snippet) override;
};

#endif // JSONSNIPPETTRANSLATOR_H

