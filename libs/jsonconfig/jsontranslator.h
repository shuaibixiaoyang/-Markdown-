// 文件说明：libs\jsonconfig\jsontranslator.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef JSONTRANSLATOR_H
#define JSONTRANSLATOR_H

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "jsoncollection.h"

template <class T>
class JsonTranslator
{
public:
    virtual ~JsonTranslator() {}

    bool processDocument(const QJsonDocument &jsonDocument, JsonCollection<T> *collection);
    QJsonDocument createDocument(JsonCollection<T> *collection);

private:
    virtual T fromJsonObject(const QJsonObject &object) = 0;
    virtual QJsonObject toJsonObject(const T &item) = 0;

    virtual bool isValid(const QJsonDocument &jsonDocument, const QString &arrayName) const;
};

template <class T>
bool JsonTranslator<T>::processDocument(const QJsonDocument &jsonDocument, JsonCollection<T> *collection)
{
    if (!isValid(jsonDocument, collection->name()))
        return false;

    QJsonArray array = jsonDocument.object().value(collection->name()).toArray();
    for (const QJsonValue &entry : array) {
        T newItem = fromJsonObject(entry.toObject());
        collection->insert(newItem);
    }

    return true;
}

template <class T>
QJsonDocument JsonTranslator<T>::createDocument(JsonCollection<T> *collection)
{
    QJsonArray array;
    for (int i = 0; i < collection->count(); ++i) {
        T item = collection->at(i);

        QJsonObject entry = toJsonObject(item);
        array.append(entry);
    }

    QJsonObject object;
    object.insert(collection->name(), array);

    QJsonDocument doc(object);
    return doc;
}

template <class T>
bool JsonTranslator<T>::isValid(const QJsonDocument &jsonDocument, const QString &arrayName) const
{
    return !jsonDocument.isEmpty() &&
           jsonDocument.isObject() &&
           jsonDocument.object().contains(arrayName) &&
           jsonDocument.object().value(arrayName).isArray();
}

#endif // JSONTRANSLATOR_H


