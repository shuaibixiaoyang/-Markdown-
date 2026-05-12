// 文件说明：test\unit\main.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include <QTest>
#include <QCoreApplication>

#include "dictionarytest.h"
#include "jsonsnippettranslatortest.h"
#include "jsonthemetranslatortest.h"
#include "jsontranslatorfactorytest.h"
#include "slidelinemappingtest.h"
#include "snippetcollectiontest.h"
#include "completionlistmodeltest.h"
#include "datavisualizationsecuritytest.h"
#include "snippettest.h"
#include "stylemanagertest.h"
#include "themecollectiontest.h"
#include "themetest.h"
#include "yamlheadercheckertest.h"
#include "ocrenhancementtest.h"
#include "gitmanagertest.h"
#include "translationmanagertest.h"
#include "voiceinputtest.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    int ret = 0;
    auto execTest = [argc, argv](QObject *testObject) -> int {
        int localArgc = argc;
        QVector<QByteArray> argStorage;
        argStorage.reserve(argc);
        QVector<char *> argPointers;
        argPointers.reserve(argc);

        for (int i = 0; i < argc; ++i) {
            argStorage.append(QByteArray(argv[i]));
        }
        for (int i = 0; i < argStorage.size(); ++i) {
            argPointers.append(argStorage[i].data());
        }

        return QTest::qExec(testObject, localArgc, argPointers.data());
    };

    SnippetTest test;
    ret += execTest(&test);

    JsonSnippetTranslatorTest test2;
    ret += execTest(&test2);

    SnippetCollectionTest test3;
    ret += execTest(&test3);

    CompletionListModelTest test4;
    ret += execTest(&test4);

    DictionaryTest test5;
    ret += execTest(&test5);

    SlideLineMappingTest test6;
    ret += execTest(&test6);

    JsonTranslatorFactoryTest test7;
    ret += execTest(&test7);

    YamlHeaderCheckerTest test8;
    ret += execTest(&test8);

    ThemeTest test9;
    ret += execTest(&test9);

    ThemeCollectionTest test10;
    ret += execTest(&test10);

    StyleManagerTest test11;
    ret += execTest(&test11);

    JsonThemeTranslatorTest test12;
    ret += execTest(&test12);

    DataVisualizationSecurityTest test13;
    ret += execTest(&test13);

    OcrEnhancementTest test14;
    ret += execTest(&test14);

    GitManagerTest test15;
    ret += execTest(&test15);

    TranslationManagerTest test16;
    ret += execTest(&test16);

    VoiceInputTest test17;
    ret += execTest(&test17);

    return ret;
}

