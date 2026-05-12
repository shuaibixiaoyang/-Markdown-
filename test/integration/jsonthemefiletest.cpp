// 文件说明：test\integration\jsonthemefiletest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "jsonthemefiletest.h"


#include <QtTest>
#include <QTemporaryFile>
#include <QTextStream>

#include <jsonfile.h>
#include <themes/jsonthemetranslatorfactory.h>
#include <themes/themecollection.h>


// 函数说明：加载 JsonThemeFileTest 需要的数据、配置或外部资源。
void JsonThemeFileTest::loadsEmptyThemeCollectionFromFile()
{
    QTemporaryFile themeFile(this);
    if (!themeFile.open())
        QFAIL("Failed to create temporary theme file");

    QTextStream out(&themeFile); out << "{ \"themes\": [] }";

    themeFile.close();

    ThemeCollection collection;
    bool success = JsonFile<Theme>::load(themeFile.fileName(), &collection);

    QVERIFY(success);
    QCOMPARE(collection.count(), 0);
}

// 函数说明：加载 JsonThemeFileTest 需要的数据、配置或外部资源。
void JsonThemeFileTest::loadsThemesCollectionFromFile()
{
    QTemporaryFile themeFile(this);
    if (!themeFile.open())
        QFAIL("Failed to create temporary theme file");

    QTextStream out(&themeFile);
    out << "{ \"themes\": ["
        << "   {  \"name\": \"default\","
        << "      \"markdownHighlighting\": \"default\","
        << "      \"codeHighlighting\": \"default\","
        << "      \"previewStylesheet\": \"default\","
        << "      \"builtIn\": true },"
        << "   {  \"name\": \"dark\","
        << "      \"markdownHighlighting\": \"dark\","
        << "      \"codeHighlighting\": \"black\","
        << "      \"previewStylesheet\": \"dark\" } ] }";

    themeFile.close();

    ThemeCollection collection;
    bool success = JsonFile<Theme>::load(themeFile.fileName(), &collection);

    QVERIFY(success);
    QCOMPARE(collection.count(), 2);
    QCOMPARE(collection.at(0).name(), QLatin1String("default"));
    QCOMPARE(collection.at(1).name(), QLatin1String("dark"));
}

// 函数说明：保存 JsonThemeFileTest 当前状态，保证用户修改可以持久化。
void JsonThemeFileTest::savesEmptyThemesCollectionToFile()
{
    QTemporaryFile themeFile(this);
    if (!themeFile.open())
        QFAIL("Failed to create temporary theme file");

    ThemeCollection collection;
    bool success = JsonFile<Theme>::save(themeFile.fileName(), &collection);

    QVERIFY(success);

    QTextStream in(&themeFile);
    QString fileContent = in.readAll().trimmed();

    QVERIFY(fileContent.startsWith("{"));
    QVERIFY(fileContent.contains("\"themes\": ["));
    QVERIFY(fileContent.endsWith("}"));
}

// 函数说明：保存 JsonThemeFileTest 当前状态，保证用户修改可以持久化。
void JsonThemeFileTest::savesThemesCollectionToFile()
{
    QTemporaryFile themeFile(this);
    if (!themeFile.open())
        QFAIL("Failed to create temporary theme file");

    Theme theme1("default", "default", "default", "default");

    Theme theme2("dark", "dark", "black", "dark");

    ThemeCollection collection;
    collection.insert(theme1);
    collection.insert(theme2);

    bool success = JsonFile<Theme>::save(themeFile.fileName(), &collection);

    QVERIFY(success);

    QTextStream in(&themeFile);
    QString fileContent = in.readAll().trimmed();

    QVERIFY(fileContent.startsWith("{"));
    QVERIFY(fileContent.contains("\"themes\": ["));
    QVERIFY(fileContent.contains("\"name\": \"default\""));
    QVERIFY(fileContent.contains("\"name\": \"dark\""));
    QVERIFY(fileContent.endsWith("}"));
}

// 函数说明：实现 JsonThemeFileTest::roundtripTest 的核心逻辑，供当前模块调用。
void JsonThemeFileTest::roundtripTest()
{
    QTemporaryFile themeFile(this);
    if (!themeFile.open())
        QFAIL("Failed to create temporary theme file");

    Theme theme1("default", "default", "default", "default", true);

    Theme theme2("dark", "dark", "black", "dark");

    ThemeCollection collection1;
    collection1.insert(theme1);
    collection1.insert(theme2);

    bool saveSuccess = JsonFile<Theme>::save(themeFile.fileName(), &collection1);
    QVERIFY(saveSuccess);

    ThemeCollection collection2;
    bool loadSuccess = JsonFile<Theme>::load(themeFile.fileName(), &collection2);
    QVERIFY(loadSuccess);

    QCOMPARE(collection2.count(), 2);

    QCOMPARE(collection2.at(0).name(), theme1.name());
    QCOMPARE(collection2.at(0).markdownHighlighting(), theme1.markdownHighlighting());
    QCOMPARE(collection2.at(0).codeHighlighting(), theme1.codeHighlighting());
    QCOMPARE(collection2.at(0).previewStylesheet(), theme1.previewStylesheet());
    QCOMPARE(collection2.at(0).isBuiltIn(), theme1.isBuiltIn());

    QCOMPARE(collection2.at(1).name(), theme2.name());
    QCOMPARE(collection2.at(1).markdownHighlighting(), theme2.markdownHighlighting());
    QCOMPARE(collection2.at(1).codeHighlighting(), theme2.codeHighlighting());
    QCOMPARE(collection2.at(1).previewStylesheet(), theme2.previewStylesheet());
    QCOMPARE(collection2.at(0).isBuiltIn(), theme1.isBuiltIn());
}

