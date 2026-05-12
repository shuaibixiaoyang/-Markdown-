// 文件说明：test\unit\jsontranslatorfactorytest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "jsontranslatorfactorytest.h"

#include <QTest>

#include <jsontranslator.h>
#include <jsontranslatorfactory.h>
#include <snippets/jsonsnippettranslatorfactory.h>
#include <snippets/snippet.h>


// 函数说明：实现 JsonTranslatorFactoryTest::returnsNullIfNoJsonTranslatorExists 的核心逻辑，供当前模块调用。
void JsonTranslatorFactoryTest::returnsNullIfNoJsonTranslatorExists()
{
    JsonTranslator<int> *translator = JsonTranslatorFactory<int>::create();

    QVERIFY(translator == 0);
}

// 函数说明：实现 JsonTranslatorFactoryTest::returnsValidJsonTranslatorForSnippets 的核心逻辑，供当前模块调用。
void JsonTranslatorFactoryTest::returnsValidJsonTranslatorForSnippets()
{
    JsonTranslator<Snippet> *translator = JsonTranslatorFactory<Snippet>::create();

    QVERIFY(translator != 0);
}

