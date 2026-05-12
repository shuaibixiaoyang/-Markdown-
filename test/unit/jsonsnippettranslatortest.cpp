// 文件说明：test\unit\jsonsnippettranslatortest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "jsonsnippettranslatortest.h"

#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <snippets/jsonsnippettranslator.h>
#include <snippets/snippet.h>
#include <snippets/snippetcollection.h>


QJsonDocument NewSnippetDocumentWithObject(const QJsonObject &jsonObject)
{
    QJsonArray snippetArray;
    snippetArray.append(jsonObject);

    QJsonObject object;
    object.insert("snippets", snippetArray);

    QJsonDocument doc(object);
    return doc;
}

// 函数说明：实现 JsonSnippetTranslatorTest::initTestCase 的核心逻辑，供当前模块调用。
void JsonSnippetTranslatorTest::initTestCase()
{
    translator = new JsonSnippetTranslator();
}

// 函数说明：实现 JsonSnippetTranslatorTest::translatesJsonDocumentToSnippets 的核心逻辑，供当前模块调用。
void JsonSnippetTranslatorTest::translatesJsonDocumentToSnippets()
{
    Snippet expected;
    expected.trigger = "trigger";
    expected.description = "description";
    expected.snippet = "snippet";
    expected.cursorPosition = 1;
    expected.builtIn = true;

    QJsonObject jsonObject;
    jsonObject.insert("trigger", expected.trigger);
    jsonObject.insert("description", expected.description);
    jsonObject.insert("snippet", expected.snippet);
    jsonObject.insert("cursor", expected.cursorPosition);
    jsonObject.insert("builtIn", expected.builtIn);

    QJsonDocument doc = NewSnippetDocumentWithObject(jsonObject);

    SnippetCollection collection;
    bool success = translator->processDocument(doc, &collection);

    QVERIFY(success);
    QCOMPARE(collection.count(), 1);
    QCOMPARE(collection.at(0).trigger, expected.trigger);
    QCOMPARE(collection.at(0).description, expected.description);
    QCOMPARE(collection.at(0).snippet, expected.snippet);
    QCOMPARE(collection.at(0).cursorPosition, expected.cursorPosition);
    QCOMPARE(collection.at(0).builtIn, expected.builtIn);
}

// 函数说明：实现 JsonSnippetTranslatorTest::translatesEmptyJsonDocumentToEmptySnippets 的核心逻辑，供当前模块调用。
void JsonSnippetTranslatorTest::translatesEmptyJsonDocumentToEmptySnippets()
{
    Snippet expected;
    expected.trigger = QString();
    expected.description = QString();
    expected.snippet = QString();
    expected.cursorPosition = 0;
    expected.builtIn = false;

    QJsonObject emptyJsonObject;
    QJsonDocument doc = NewSnippetDocumentWithObject(emptyJsonObject);

    SnippetCollection collection;
    bool success = translator->processDocument(doc, &collection);

    QVERIFY(success);
    QCOMPARE(collection.count(), 1);
    QCOMPARE(collection.at(0).trigger, expected.trigger);
    QCOMPARE(collection.at(0).description, expected.description);
    QCOMPARE(collection.at(0).snippet, expected.snippet);
    QCOMPARE(collection.at(0).cursorPosition, expected.cursorPosition);
    QCOMPARE(collection.at(0).builtIn, expected.builtIn);
}

// 函数说明：实现 JsonSnippetTranslatorTest::defectIfJsonDocumentIsInvalid 的核心逻辑，供当前模块调用。
void JsonSnippetTranslatorTest::defectIfJsonDocumentIsInvalid()
{
    QJsonDocument doc;

    SnippetCollection collection;
    bool success = translator->processDocument(doc, &collection);

    QVERIFY(!success);
    QCOMPARE(collection.count(), 0);
}

// 函数说明：实现 JsonSnippetTranslatorTest::translatesSnippetCollectionToJsonDocument 的核心逻辑，供当前模块调用。
void JsonSnippetTranslatorTest::translatesSnippetCollectionToJsonDocument()
{
    Snippet snippet;
    snippet.trigger = "trigger";
    snippet.description = "description";
    snippet.snippet = "snippet";
    snippet.cursorPosition = 1;
    snippet.builtIn = true;

    SnippetCollection collection;
    collection.insert(snippet);

    QJsonObject expected;
    expected.insert("trigger", snippet.trigger);
    expected.insert("description", snippet.description);
    expected.insert("snippet", snippet.snippet);
    expected.insert("cursor", snippet.cursorPosition);
    expected.insert("builtIn", snippet.builtIn);

    QJsonDocument actual = translator->createDocument(&collection);

    QVERIFY(actual.isObject());
    QVERIFY(actual.object().contains("snippets"));
    QVERIFY(actual.object().value("snippets").isArray());

    QJsonObject actualObject = actual.object().value("snippets").toArray().first().toObject();
    QCOMPARE(actualObject["trigger"], expected["trigger"]);
    QCOMPARE(actualObject["description"], expected["description"]);
    QCOMPARE(actualObject["snippet"], expected["snippet"]);
    QCOMPARE(actualObject["cursorPosition"], expected["cursorPosition"]);
    QCOMPARE(actualObject["builtIn"], expected["builtIn"]);
}

// 函数说明：实现 JsonSnippetTranslatorTest::cleanupTestCase 的核心逻辑，供当前模块调用。
void JsonSnippetTranslatorTest::cleanupTestCase()
{
    delete translator;
}


