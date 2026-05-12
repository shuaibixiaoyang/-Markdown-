// 文件说明：test\unit\jsonthemetranslatortest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "jsonthemetranslatortest.h"

#include <QtTest>

#include <themes/jsonthemetranslator.h>
#include <themes/theme.h>
#include <themes/themecollection.h>

static const QLatin1String A_THEME_NAME("mytheme");
static const QLatin1String A_MARKDOWN_HIGHLIGHTING("default");
static const QLatin1String A_CODE_HIGHLIGHTING("monokai");
static const QLatin1String A_PREVIEW_STYLESHEET("github");


QJsonDocument NewJsonDocumentWithObject(const QJsonObject &jsonObject)
{
    QJsonArray themeArray;
    themeArray.append(jsonObject);

    QJsonObject object;
    object.insert("themes", themeArray);

    return QJsonDocument(object);
}

QJsonObject NewJsonThemeObject()
{
    QJsonObject jsonObject;
    jsonObject.insert("name", A_THEME_NAME);
    jsonObject.insert("markdownHighlighting", A_MARKDOWN_HIGHLIGHTING);
    jsonObject.insert("codeHighlighting", A_CODE_HIGHLIGHTING);
    jsonObject.insert("previewStylesheet", A_PREVIEW_STYLESHEET);
    jsonObject.insert("builtIn", true);

    return jsonObject;
}

// 函数说明：实现 JsonThemeTranslatorTest::initTestCase 的核心逻辑，供当前模块调用。
void JsonThemeTranslatorTest::initTestCase()
{
    translator = new JsonThemeTranslator();
}

// 函数说明：实现 JsonThemeTranslatorTest::cleanupTestCase 的核心逻辑，供当前模块调用。
void JsonThemeTranslatorTest::cleanupTestCase()
{
    delete translator;
}

// 函数说明：实现 JsonThemeTranslatorTest::doesNotProcessInvalidJsonDocument 的核心逻辑，供当前模块调用。
void JsonThemeTranslatorTest::doesNotProcessInvalidJsonDocument()
{
    QJsonDocument doc;

    ThemeCollection collection;
    bool success = translator->processDocument(doc, &collection);

    QVERIFY(!success);
    QCOMPARE(collection.count(), 0);
}

// 函数说明：实现 JsonThemeTranslatorTest::translatesEmptyJsonDocumentToEmptyThemes 的核心逻辑，供当前模块调用。
void JsonThemeTranslatorTest::translatesEmptyJsonDocumentToEmptyThemes()
{
    QJsonObject themesObject;
    themesObject.insert("themes", QJsonArray());

    QJsonDocument doc(themesObject);

    ThemeCollection collection;
    bool success = translator->processDocument(doc, &collection);

    QVERIFY(success);
    QCOMPARE(collection.count(), 0);
}

// 函数说明：实现 JsonThemeTranslatorTest::translatesJsonDocumentToThemes 的核心逻辑，供当前模块调用。
void JsonThemeTranslatorTest::translatesJsonDocumentToThemes()
{
    QJsonDocument doc = NewJsonDocumentWithObject(NewJsonThemeObject());

    ThemeCollection collection;
    bool success = translator->processDocument(doc, &collection);

    QVERIFY(success);
    QCOMPARE(collection.count(), 1);
    QCOMPARE(collection.at(0).name(), A_THEME_NAME);
    QCOMPARE(collection.at(0).markdownHighlighting(), A_MARKDOWN_HIGHLIGHTING);
    QCOMPARE(collection.at(0).codeHighlighting(), A_CODE_HIGHLIGHTING);
    QCOMPARE(collection.at(0).previewStylesheet(), A_PREVIEW_STYLESHEET);
    QCOMPARE(collection.at(0).isBuiltIn(), true);
}

// 函数说明：实现 JsonThemeTranslatorTest::translatesThemesToJsonDocument 的核心逻辑，供当前模块调用。
void JsonThemeTranslatorTest::translatesThemesToJsonDocument()
{
    Theme theme(A_THEME_NAME, A_MARKDOWN_HIGHLIGHTING, A_CODE_HIGHLIGHTING, A_PREVIEW_STYLESHEET, true);
    ThemeCollection collection;
    collection.insert(theme);

    QJsonDocument doc = translator->createDocument(&collection);

    QJsonObject actual = doc.object().value("themes").toArray().first().toObject();
    QCOMPARE(actual["name"].toString(), A_THEME_NAME);
    QCOMPARE(actual["markdownHighlighting"].toString(), A_MARKDOWN_HIGHLIGHTING);
    QCOMPARE(actual["codeHighlighting"].toString(), A_CODE_HIGHLIGHTING);
    QCOMPARE(actual["previewStylesheet"].toString(), A_PREVIEW_STYLESHEET);
    QCOMPARE(actual["builtIn"].toBool(), true);
}


