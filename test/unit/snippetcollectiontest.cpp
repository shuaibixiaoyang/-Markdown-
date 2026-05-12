// 文件说明：test\unit\snippetcollectiontest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "snippetcollectiontest.h"

#include <QtTest>

#include <snippets/snippetcollection.h>


// 函数说明：实现 SnippetCollectionTest::initTestCase 的核心逻辑，供当前模块调用。
void SnippetCollectionTest::initTestCase()
{
    qRegisterMetaType<SnippetCollection::CollectionChangedType>();  // for QSignalSpy
    qRegisterMetaType<Snippet>();  // for QSignalSpy
}

// 函数说明：实现 SnippetCollectionTest::notifiesListenersOfNewSnippets 的核心逻辑，供当前模块调用。
void SnippetCollectionTest::notifiesListenersOfNewSnippets()
{
    const Snippet snippet;
    SnippetCollection collection;

    QSignalSpy spy(&collection, &SnippetCollection::collectionChanged);

    collection.insert(snippet);

    QCOMPARE(spy.count(), 1);

    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).value<SnippetCollection::CollectionChangedType>(), SnippetCollection::ItemAdded);
}

// 函数说明：实现 SnippetCollectionTest::notifiesListenersOfChangedSnippets 的核心逻辑，供当前模块调用。
void SnippetCollectionTest::notifiesListenersOfChangedSnippets()
{
    const Snippet snippet;
    SnippetCollection collection;
    QSignalSpy spy(&collection, &SnippetCollection::collectionChanged);

    collection.insert(snippet);
    collection.update(snippet);

    QCOMPARE(spy.count(), 2);

    QList<QVariant> arguments = spy.takeAt(1);
    QCOMPARE(arguments.at(0).value<SnippetCollection::CollectionChangedType>(), SnippetCollection::ItemChanged);
}

// 函数说明：实现 SnippetCollectionTest::notifiesListenersOfRemovedSnippets 的核心逻辑，供当前模块调用。
void SnippetCollectionTest::notifiesListenersOfRemovedSnippets()
{
    const Snippet snippet;
    SnippetCollection collection;
    QSignalSpy spy(&collection, &SnippetCollection::collectionChanged);

    collection.insert(snippet);
    collection.remove(snippet);

    QCOMPARE(spy.count(), 2);

    QList<QVariant> arguments = spy.takeAt(1);
    QCOMPARE(arguments.at(0).value<SnippetCollection::CollectionChangedType>(), SnippetCollection::ItemDeleted);
}

// 函数说明：实现 SnippetCollectionTest::holdsSnippetsInTriggerOrder 的核心逻辑，供当前模块调用。
void SnippetCollectionTest::holdsSnippetsInTriggerOrder()
{
    Snippet snippet1; snippet1.trigger = "a";
    Snippet snippet2; snippet2.trigger = "b";
    Snippet snippet3; snippet3.trigger = "c";

    SnippetCollection collection;

    collection.insert(snippet2);    // b
    collection.insert(snippet3);    // c
    collection.insert(snippet1);    // a

    QCOMPARE(collection.at(0).trigger, snippet1.trigger);    // a
    QCOMPARE(collection.at(1).trigger, snippet2.trigger);    // b
    QCOMPARE(collection.at(2).trigger, snippet3.trigger);    // c
}

// 函数说明：实现 SnippetCollectionTest::returnsNewCollectionOfUserDefinedSnippets 的核心逻辑，供当前模块调用。
void SnippetCollectionTest::returnsNewCollectionOfUserDefinedSnippets()
{
    Snippet snippet1; snippet1.trigger = "a"; snippet1.builtIn = true;
    Snippet snippet2; snippet2.trigger = "b"; snippet2.builtIn = false;
    Snippet snippet3; snippet3.trigger = "c"; snippet3.builtIn = false;

    SnippetCollection collection;
    collection.insert(snippet2);
    collection.insert(snippet1);
    collection.insert(snippet3);

    QSharedPointer<SnippetCollection> userDefinedSnippets = collection.userDefinedSnippets();

    QVERIFY(!userDefinedSnippets.isNull());
    QVERIFY(userDefinedSnippets.data() != &collection);
    QCOMPARE(userDefinedSnippets->count(), 2);
    QVERIFY(!userDefinedSnippets->contains(snippet1.trigger));
    QVERIFY(userDefinedSnippets->contains(snippet2.trigger));
    QVERIFY(userDefinedSnippets->contains(snippet3.trigger));
}

// 函数说明：实现 SnippetCollectionTest::returnsConstantNameOfJsonArray 的核心逻辑，供当前模块调用。
void SnippetCollectionTest::returnsConstantNameOfJsonArray()
{
    SnippetCollection collection;
    QCOMPARE(collection.name(), QStringLiteral("snippets"));
}

// 函数说明：实现 SnippetCollectionTest::returnsEmptySnippetForNonExistingTrigger 的核心逻辑，供当前模块调用。
void SnippetCollectionTest::returnsEmptySnippetForNonExistingTrigger()
{
    Snippet snippet; snippet.trigger = "ArbitraryName";
    SnippetCollection collection;
    collection.insert(snippet);

    Snippet foundSnippet = collection.snippet("NonExistingSnippet");

    QCOMPARE(foundSnippet.trigger, QString());
    QCOMPARE(foundSnippet.description, QString());
    QCOMPARE(foundSnippet.snippet, QString());
}

