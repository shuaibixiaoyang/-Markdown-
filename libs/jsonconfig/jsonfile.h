// 文件说明：libs\jsonconfig\jsonfile.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef JSONFILE_H
#define JSONFILE_H

#include <QFile>
#include <QJsonDocument>
#include <QTextStream>
#include "jsoncollection.h"
#include "jsontranslator.h"
#include "jsontranslatorfactory.h"

class QString;


template <class T>
class JsonFile
{
public:
    static bool load(const QString &fileName, JsonCollection<T> *collection);
    static bool save(const QString &fileName, JsonCollection<T> *collection);

private:
    JsonFile();
};

template <class T>
bool JsonFile<T>::load(const QString &fileName, JsonCollection<T> *collection)
{
    QFile jsonFile(fileName);
    if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll());

    QScopedPointer<JsonTranslator<T> > translator(JsonTranslatorFactory<T>::create());
    return translator->processDocument(doc,collection);
}

template <class T>
bool JsonFile<T>::save(const QString &fileName, JsonCollection<T> *collection)
{
    QFile jsonFile(fileName);
    if (!jsonFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QScopedPointer<JsonTranslator<T> > translator(JsonTranslatorFactory<T>::create());
    QJsonDocument doc = translator->createDocument(collection);

    QTextStream out(&jsonFile);
    out << doc.toJson();

    return true;
}

#endif // JSONFILE_H


