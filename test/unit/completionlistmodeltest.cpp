// 文件说明：test\unit\completionlistmodeltest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "completionlistmodeltest.h"

#include <QtTest>

#include <snippets/snippet.h>
#include <completionlistmodel.h>


// 函数说明：实现 CompletionListModelTest::acceptsNewSnippet 的核心逻辑，供当前模块调用。
void CompletionListModelTest::acceptsNewSnippet()
{
    model = new CompletionListModel(this);

    QSignalSpy spy(model, &QAbstractItemModel::rowsInserted);

    Snippet snippet;
    snippet.trigger = "link";
    snippet.description = "Hyperlink";
    snippet.snippet = "[]()";

    model->snippetCollectionChanged(SnippetCollection::ItemAdded, snippet);

    QCOMPARE(spy.count(), 1);

    QCOMPARE(model->rowCount(), 1);
    assertItemMatchesSnippet(0, snippet);

    delete model;
}

// 函数说明：刷新 CompletionListModelTest 的内部状态，并同步到相关界面。
void CompletionListModelTest::updatesCorrectRowForSnippet()
{
    model = new CompletionListModel(this);

    Snippet snippet1; snippet1.trigger = "link"; snippet1.description = "Hyperlink";
    Snippet snippet2; snippet2.trigger = "gq"; snippet2.description = "German Quotes";

    model->snippetCollectionChanged(SnippetCollection::ItemAdded, snippet1);
    model->snippetCollectionChanged(SnippetCollection::ItemAdded, snippet2);

    snippet2.description = "Changed Description";

    model->snippetCollectionChanged(SnippetCollection::ItemChanged, snippet2);

    assertItemMatchesSnippet(0, snippet2);

    delete model;
}

// 函数说明：从 CompletionListModelTest 管理的数据集合中移除指定内容。
void CompletionListModelTest::removesCorrectRowForSnippet()
{
    model = new CompletionListModel(this);

    QSignalSpy spy(model, &QAbstractItemModel::rowsRemoved);

    Snippet snippet1; snippet1.trigger = "link"; snippet1.description = "Hyperlink";
    Snippet snippet2; snippet2.trigger = "gq"; snippet2.description = "German Quotes";

    model->snippetCollectionChanged(SnippetCollection::ItemAdded, snippet1);
    model->snippetCollectionChanged(SnippetCollection::ItemAdded, snippet2);

    model->snippetCollectionChanged(SnippetCollection::ItemDeleted, snippet2);

    QCOMPARE(spy.count(), 1);

    QCOMPARE(model->rowCount(), 1);
    assertItemMatchesSnippet(0, snippet1);

    delete model;
}

// 函数说明：实现 CompletionListModelTest::holdsSnippetsInTriggerOrder 的核心逻辑，供当前模块调用。
void CompletionListModelTest::holdsSnippetsInTriggerOrder()
{
    model = new CompletionListModel(this);

    Snippet snippet1; snippet1.trigger = "a";
    Snippet snippet2; snippet2.trigger = "b";
    Snippet snippet3; snippet3.trigger = "c";

    model->snippetCollectionChanged(SnippetCollection::ItemAdded, snippet2);    // b
    model->snippetCollectionChanged(SnippetCollection::ItemAdded, snippet3);    // c
    model->snippetCollectionChanged(SnippetCollection::ItemAdded, snippet1);    // a

    QCOMPARE(itemValue(0, Qt::EditRole).toString(), snippet1.trigger);    // a
    QCOMPARE(itemValue(1, Qt::EditRole).toString(), snippet2.trigger);    // b
    QCOMPARE(itemValue(2, Qt::EditRole).toString(), snippet3.trigger);    // c

    delete model;
}

// 函数说明：实现 CompletionListModelTest::assertItemMatchesSnippet 的核心逻辑，供当前模块调用。
void CompletionListModelTest::assertItemMatchesSnippet(int row, const Snippet &snippet)
{
    QCOMPARE(itemValue(row, Qt::EditRole).toString(), snippet.trigger);
    QVERIFY(itemValue(row, Qt::DisplayRole).toString().contains(snippet.trigger));
    QVERIFY(itemValue(row, Qt::DisplayRole).toString().contains(snippet.description));
    QCOMPARE(itemValue(row, Qt::ToolTipRole).toString(), snippet.snippet.toHtmlEscaped());
}

// 函数说明：实现 CompletionListModelTest::itemValue 的核心逻辑，供当前模块调用。
QVariant CompletionListModelTest::itemValue(int row, int role)
{
    return model->data(model->index(row), role);
}

