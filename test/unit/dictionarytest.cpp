// 文件说明：test\unit\dictionarytest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "dictionarytest.h"

#include <QTest>

#include <spellchecker/dictionary.h>

// 函数说明：实现 DictionaryTest::returnsLanguageNameForLanguageCode 的核心逻辑，供当前模块调用。
void DictionaryTest::returnsLanguageNameForLanguageCode()
{
    Dictionary german("de_DE", "");
    QCOMPARE(german.languageName(), QStringLiteral("Deutsch"));

    Dictionary americanEnglish("en_US", "");
    // Qt 6 uses CLDR long name for en_US.
    QCOMPARE(americanEnglish.languageName(), QStringLiteral("American English"));
}

// 函数说明：实现 DictionaryTest::returnsCountryNameForLanguage 的核心逻辑，供当前模块调用。
void DictionaryTest::returnsCountryNameForLanguage()
{
    Dictionary german("de_DE", "");
    QCOMPARE(german.countryName(), QStringLiteral("Deutschland"));

    Dictionary americanEnglish("en_US", "");
    QCOMPARE(americanEnglish.countryName(), QStringLiteral("United States"));
}

