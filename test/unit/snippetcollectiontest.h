// 文件说明：test\unit\snippetcollectiontest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef SNIPPETCOLLECTIONTEST_H
#define SNIPPETCOLLECTIONTEST_H

#include <QObject>
#include <snippets/snippetcollection.h>


class SnippetCollectionTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void notifiesListenersOfNewSnippets();
    void notifiesListenersOfChangedSnippets();
    void notifiesListenersOfRemovedSnippets();

    void holdsSnippetsInTriggerOrder();

    void returnsNewCollectionOfUserDefinedSnippets();
    void returnsConstantNameOfJsonArray();
    void returnsEmptySnippetForNonExistingTrigger();
};

Q_DECLARE_METATYPE(SnippetCollection::CollectionChangedType) // for QSignalSpy
Q_DECLARE_METATYPE(Snippet) // for QSignalSpy

#endif // SNIPPETCOLLECTIONTEST_H

