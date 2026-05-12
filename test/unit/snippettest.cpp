// 文件说明：test\unit\snippettest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "snippettest.h"

#include <QTest>

#include <snippets/snippet.h>

// 函数说明：判断 SnippetTest 当前是否满足指定状态。
void SnippetTest::isLessThanComparable()
{
    Snippet snippet1;
    snippet1.trigger = "abc";

    Snippet snippet2;
    snippet2.trigger = "xyz";

    QCOMPARE(snippet1 < snippet2, true);
    QCOMPARE(snippet2 < snippet1, false);
    QCOMPARE(snippet1 < snippet1, false);
}

// 函数说明：判断 SnippetTest 当前是否满足指定状态。
void SnippetTest::isEqualComparable()
{
    Snippet snippet1;
    snippet1.trigger = "abc";
    snippet1.description = "description 1";

    Snippet snippet2;
    snippet2.trigger = "abc";
    snippet2.description = "description 2";

    Snippet snippet3;
    snippet3.trigger = "xyz";
    snippet3.description = "description 1";

    QCOMPARE(snippet1 == snippet1, true);
    QCOMPARE(snippet1 == snippet2, true);
    QCOMPARE(snippet1 == snippet3, false);
}

// 函数说明：判断 SnippetTest 当前是否满足指定状态。
void SnippetTest::isInitializedAfterCreation()
{
    Snippet snippet;
    QVERIFY(snippet.trigger.isNull());
    QVERIFY(snippet.description.isNull());
    QVERIFY(snippet.snippet.isNull());
    QCOMPARE(snippet.cursorPosition, 0);
    QCOMPARE(snippet.builtIn, false);
}

