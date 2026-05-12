// 文件说明：test\integration\themecollectiontest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "themecollectiontest.h"

#include <QtTest>

#include <themes/themecollection.h>

// 函数说明：实现 ThemeCollectionTest::initTestCase 的核心逻辑，供当前模块调用。
void ThemeCollectionTest::initTestCase()
{
    themeFile = new QTemporaryFile(this);
    if (!themeFile->open())
        QFAIL("Failed to create temporary theme file");

    QTextStream out(themeFile);
    out << "{ \"themes\": ["
        << "   {  \"name\": \"default\","
        << "      \"markdownHighlighting\": \"default\","
        << "      \"codeHighlighting\": \"default\","
        << "      \"previewStylesheet\": \"default\" },"
        << "   {  \"name\": \"dark\","
        << "      \"markdownHighlighting\": \"dark\","
        << "      \"codeHighlighting\": \"black\","
        << "      \"previewStylesheet\": \"dark\" } ] }";
}

// 函数说明：实现 ThemeCollectionTest::cleanupTestCase 的核心逻辑，供当前模块调用。
void ThemeCollectionTest::cleanupTestCase()
{
    themeFile->close();
}

// 函数说明：加载 ThemeCollectionTest 需要的数据、配置或外部资源。
void ThemeCollectionTest::loadsThemesFromFileIntoCollection()
{
    ThemeCollection themeCollection;

    themeCollection.load(themeFile->fileName());

    QStringList themeNames = themeCollection.themeNames();
    QCOMPARE(themeNames.count(), 2);
    QCOMPARE(themeNames.at(0), QLatin1String("default"));
    QCOMPARE(themeNames.at(1), QLatin1String("dark"));
}

