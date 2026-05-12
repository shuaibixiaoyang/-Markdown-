// 文件说明：test\unit\themecollectiontest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "themecollectiontest.h"

#include <QtTest>

#include <themes/themecollection.h>


// 函数说明：实现 ThemeCollectionTest::returnsConstantNameOfJsonArray 的核心逻辑，供当前模块调用。
void ThemeCollectionTest::returnsConstantNameOfJsonArray()
{
    ThemeCollection collection;
    QCOMPARE(collection.name(), QStringLiteral("themes"));
}

// 函数说明：实现 ThemeCollectionTest::returnsNumberOfThemesInCollection 的核心逻辑，供当前模块调用。
void ThemeCollectionTest::returnsNumberOfThemesInCollection()
{
    ThemeCollection collection;
    Theme theme("name", "markdown", "code", "preview");
    collection.insert(theme);

    QCOMPARE(collection.count(), 1);
}

// 函数说明：实现 ThemeCollectionTest::returnsThemeAtIndexPosition 的核心逻辑，供当前模块调用。
void ThemeCollectionTest::returnsThemeAtIndexPosition()
{
    ThemeCollection collection;
    Theme theme("name", "markdown", "code", "preview");
    collection.insert(theme);

    Theme actual = collection.at(0);

    QCOMPARE(actual, theme);
}

// 函数说明：实现 ThemeCollectionTest::returnsIfCollectionContainsTheme 的核心逻辑，供当前模块调用。
void ThemeCollectionTest::returnsIfCollectionContainsTheme()
{
    ThemeCollection collection;
    Theme theme("name", "markdown", "code", "preview");
    collection.insert(theme);
    
    QCOMPARE(collection.contains("name"), true);
    QCOMPARE(collection.contains("missing"), false);
}

// 函数说明：实现 ThemeCollectionTest::returnsThemeByName 的核心逻辑，供当前模块调用。
void ThemeCollectionTest::returnsThemeByName()
{
    ThemeCollection collection;
    Theme theme("name", "markdown", "code", "preview");
    collection.insert(theme);
    
    Theme actual = collection.theme("name");

    QCOMPARE(actual, theme);
}

// 函数说明：实现 ThemeCollectionTest::returnsNameOfAllThemes 的核心逻辑，供当前模块调用。
void ThemeCollectionTest::returnsNameOfAllThemes()
{
    Theme theme1("name 1", "markdown", "code", "preview");
    Theme theme2("name 2", "markdown", "code", "preview");
    ThemeCollection collection;
    collection.insert(theme1);
    collection.insert(theme2);
    
    QStringList themeNames = collection.themeNames();

    QCOMPARE(themeNames.count(), 2);
    QCOMPARE(themeNames.at(0), theme1.name());
    QCOMPARE(themeNames.at(1), theme2.name());
}


