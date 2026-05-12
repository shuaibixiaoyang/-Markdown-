// 文件说明：test\unit\themetest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "themetest.h"

#include <QTest>

#include <themes/theme.h>

static const QLatin1String A_THEME_NAME("name");
static const QLatin1String A_MARKDOWN_HIGHLIGHTING("markdown");
static const QLatin1String A_CODE_HIGHLIGHTING("code");
static const QLatin1String A_PREVIEW_STYLESHEET("preview");

// 函数说明：判断 ThemeTest 当前是否满足指定状态。
void ThemeTest::isLessThanComparable()
{
    Theme theme1("abc", A_MARKDOWN_HIGHLIGHTING, A_CODE_HIGHLIGHTING, A_PREVIEW_STYLESHEET);

    Theme theme2("xyz", A_MARKDOWN_HIGHLIGHTING, A_CODE_HIGHLIGHTING, A_PREVIEW_STYLESHEET);

    QCOMPARE(theme1 < theme2, true);
    QCOMPARE(theme2 < theme1, false);
    QCOMPARE(theme1 < theme1, false);
}

// 函数说明：判断 ThemeTest 当前是否满足指定状态。
void ThemeTest::isEqualComparable()
{
    Theme theme1("abc", A_MARKDOWN_HIGHLIGHTING, A_CODE_HIGHLIGHTING, A_PREVIEW_STYLESHEET);

    Theme theme2("abc", A_MARKDOWN_HIGHLIGHTING, A_CODE_HIGHLIGHTING, A_PREVIEW_STYLESHEET);

    Theme theme3("xyz", A_MARKDOWN_HIGHLIGHTING, A_CODE_HIGHLIGHTING, A_PREVIEW_STYLESHEET);

    QCOMPARE(theme1 == theme1, true);
    QCOMPARE(theme1 == theme2, true);
    QCOMPARE(theme1 == theme3, false);
}

// 函数说明：实现 ThemeTest::throwsIfNameIsEmpty 的核心逻辑，供当前模块调用。
void ThemeTest::throwsIfNameIsEmpty()
{
    try {
        Theme theme("", A_MARKDOWN_HIGHLIGHTING, A_CODE_HIGHLIGHTING, A_PREVIEW_STYLESHEET);
        QFAIL("Expected exception of type runtime_error not thrown");
    } catch(const std::runtime_error &) {} 
}

// 函数说明：实现 ThemeTest::throwsIfMarkdownHighlightingIsEmpty 的核心逻辑，供当前模块调用。
void ThemeTest::throwsIfMarkdownHighlightingIsEmpty()
{
    try {
        Theme theme(A_THEME_NAME, "", A_CODE_HIGHLIGHTING, A_PREVIEW_STYLESHEET);
        QFAIL("Expected exception of type runtime_error not thrown");
    } catch(const std::runtime_error &) {} 
}

// 函数说明：实现 ThemeTest::throwsIfCodeHighlightingIsEmpty 的核心逻辑，供当前模块调用。
void ThemeTest::throwsIfCodeHighlightingIsEmpty()
{
    try {
        Theme theme(A_THEME_NAME, A_MARKDOWN_HIGHLIGHTING, "", A_PREVIEW_STYLESHEET);
        QFAIL("Expected exception of type runtime_error not thrown");
    } catch(const std::runtime_error &) {} 
}

// 函数说明：实现 ThemeTest::throwsIfPreviewStylesheetIsEmpty 的核心逻辑，供当前模块调用。
void ThemeTest::throwsIfPreviewStylesheetIsEmpty()
{
    try {
        Theme theme(A_THEME_NAME, A_MARKDOWN_HIGHLIGHTING, A_CODE_HIGHLIGHTING, "");
        QFAIL("Expected exception of type runtime_error not thrown");
    } catch(const std::runtime_error &) {} 
}



