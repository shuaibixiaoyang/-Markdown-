// 文件说明：test\integration\jsonsnippetfiletest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "jsonsnippetfiletest.h"

#include <QtTest>
#include <QTemporaryFile>
#include <QTextStream>

#include <jsonfile.h>
#include <snippets/jsonsnippettranslatorfactory.h>
#include <snippets/snippetcollection.h>

// 函数说明：加载 JsonSnippetFileTest 需要的数据、配置或外部资源。
void JsonSnippetFileTest::loadsEmptySnippetsCollectionFromFile()
{
    QTemporaryFile snippetFile(this);
    if (!snippetFile.open())
        QFAIL("Failed to create temporary snippet file");

    QTextStream out(&snippetFile); out << "{ \"snippets\": [] }";

    snippetFile.close();

    SnippetCollection collection;
    bool success = JsonFile<Snippet>::load(snippetFile.fileName(), &collection);

    QVERIFY(success);
    QCOMPARE(collection.count(), 0);
}

// 函数说明：加载 JsonSnippetFileTest 需要的数据、配置或外部资源。
void JsonSnippetFileTest::loadsSnippetsCollectionFromFile()
{
    QTemporaryFile snippetFile(this);
    if (!snippetFile.open())
        QFAIL("Failed to create temporary snippet file");

    QTextStream out(&snippetFile);
    out << "{ \"snippets\": ["
        << "   {  \"trigger\": \"abc\","
        << "      \"description\": \"description xyz\","
        << "      \"snippet\": \"content abc\","
        << "      \"cursor\": 0,"
        << "      \"builtIn\": true },"
        << "   {  \"trigger\": \"xyz\","
        << "      \"description\": \"description xyz\","
        << "      \"snippet\": \"content xyz\","
        << "      \"cursor\": 1,"
        << "      \"builtIn\": false } ] }";

    snippetFile.close();

    SnippetCollection collection;
    bool success = JsonFile<Snippet>::load(snippetFile.fileName(), &collection);

    QVERIFY(success);
    QCOMPARE(collection.count(), 2);
    QVERIFY(collection.contains("abc"));
    QVERIFY(collection.contains("xyz"));
}

// 函数说明：保存 JsonSnippetFileTest 当前状态，保证用户修改可以持久化。
void JsonSnippetFileTest::savesEmptySnippetsCollectionToFile()
{
    QTemporaryFile snippetFile(this);
    if (!snippetFile.open())
        QFAIL("Failed to create temporary snippet file");

    SnippetCollection collection;
    bool success = JsonFile<Snippet>::save(snippetFile.fileName(), &collection);

    QVERIFY(success);

    QTextStream in(&snippetFile);
    QString fileContent = in.readAll().trimmed();

    QVERIFY(fileContent.startsWith("{"));
    QVERIFY(fileContent.contains("\"snippets\": ["));
    QVERIFY(fileContent.endsWith("}"));
}

// 函数说明：保存 JsonSnippetFileTest 当前状态，保证用户修改可以持久化。
void JsonSnippetFileTest::savesSnippetsCollectionToFile()
{
    QTemporaryFile snippetFile(this);
    if (!snippetFile.open())
        QFAIL("Failed to create temporary snippet file");

    Snippet snippet1;
    snippet1.trigger = "abc";

    Snippet snippet2;
    snippet2.trigger = "xyz";

    SnippetCollection collection;
    collection.insert(snippet1);
    collection.insert(snippet2);

    bool success = JsonFile<Snippet>::save(snippetFile.fileName(), &collection);

    QVERIFY(success);

    QTextStream in(&snippetFile);
    QString fileContent = in.readAll().trimmed();

    QVERIFY(fileContent.startsWith("{"));
    QVERIFY(fileContent.contains("\"snippets\": ["));
    QVERIFY(fileContent.contains("\"trigger\": \"abc\""));
    QVERIFY(fileContent.contains("\"trigger\": \"xyz\""));
    QVERIFY(fileContent.endsWith("}"));
}

// 函数说明：实现 JsonSnippetFileTest::roundtripTest 的核心逻辑，供当前模块调用。
void JsonSnippetFileTest::roundtripTest()
{
    QTemporaryFile snippetFile(this);
    if (!snippetFile.open())
        QFAIL("Failed to create temporary snippet file");

    Snippet snippet1;
    snippet1.trigger = "abc";
    snippet1.description = "description abc";
    snippet1.snippet = "content abc";
    snippet1.cursorPosition = 0;
    snippet1.builtIn = true;

    Snippet snippet2;
    snippet2.trigger = "xyz";
    snippet2.description = "description xyz";
    snippet2.snippet = "content xyz";
    snippet2.cursorPosition = 1;
    snippet2.builtIn = false;

    SnippetCollection collection1;
    collection1.insert(snippet1);
    collection1.insert(snippet2);

    bool saveSuccess = JsonFile<Snippet>::save(snippetFile.fileName(), &collection1);
    QVERIFY(saveSuccess);

    SnippetCollection collection2;
    bool loadSuccess = JsonFile<Snippet>::load(snippetFile.fileName(), &collection2);
    QVERIFY(loadSuccess);

    QCOMPARE(collection2.count(), 2);

    QCOMPARE(collection2.snippet("abc").description, snippet1.description);
    QCOMPARE(collection2.snippet("abc").snippet, snippet1.snippet);
    QCOMPARE(collection2.snippet("abc").cursorPosition, snippet1.cursorPosition);
    QCOMPARE(collection2.snippet("abc").builtIn, snippet1.builtIn);

    QCOMPARE(collection2.snippet("xyz").description, snippet2.description);
    QCOMPARE(collection2.snippet("xyz").snippet, snippet2.snippet);
    QCOMPARE(collection2.snippet("xyz").cursorPosition, snippet2.cursorPosition);
    QCOMPARE(collection2.snippet("xyz").builtIn, snippet2.builtIn);
}

