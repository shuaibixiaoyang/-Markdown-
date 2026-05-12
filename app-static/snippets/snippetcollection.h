// 文件说明：app-static\snippets\snippetcollection.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef SNIPPETCOLLECTION_H
#define SNIPPETCOLLECTION_H

#include <QObject>
#include <QMap>
#include <jsoncollection.h>
#include "snippet.h"


class SnippetCollection : public QObject, public JsonCollection<Snippet>
{
    Q_OBJECT
    Q_ENUMS(CollectionChangedType)

public:
    enum CollectionChangedType
    {
        ItemAdded,
        ItemChanged,
        ItemDeleted
    };

    explicit SnippetCollection(QObject *parent = nullptr);

    int count() const;

    int insert(const Snippet& snippet);
    void update(const Snippet& snippet);
    void remove(const Snippet& snippet);

    const QString name() const;
    bool contains(const QString &trigger) const;
    const Snippet snippet(const QString &trigger) const;
    const Snippet &at(int offset) const;

    QSharedPointer<SnippetCollection> userDefinedSnippets() const;

signals:
    void collectionChanged(SnippetCollection::CollectionChangedType changedType, const Snippet &snippet);

private:
    QMap<QString, Snippet> snippets;
};

#endif // SNIPPETCOLLECTION_H

