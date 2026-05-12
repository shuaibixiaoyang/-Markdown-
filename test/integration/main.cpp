// 文件说明：test\integration\main.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include <QTest>

#include <QApplication>
#include <QByteArray>
#include <QStringList>

#include "discountmarkdownconvertertest.h"
#include "blogpublishertest.h"
#include "cloudsyncintegrationtest.h"
#include "collaborationintegrationtest.h"
#include "datavisualizationpanelintegrationtest.h"
#include "ocrintegrationtest.h"
#include "htmlpreviewcontrollertest.h"
#include "htmltemplatetest.h"
#include "epubexportertest.h"
#include "jsonsnippetfiletest.h"
#include "jsonthemefiletest.h"
#include "latexexportertest.h"
#include "mainwindowlfsintegrationtest.h"
#include "pmhmarkdownparsertest.h"
#include "revealmarkdownconvertertest.h"
#include "themecollectiontest.h"

#ifdef ENABLE_HOEDOWN
#include "hoedownmarkdownconvertertest.h"
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    int ret = 0;
    const QStringList selectedSuites = QString::fromLocal8Bit(qgetenv("CUTEMARKED_TEST_SUITES"))
                                           .split(',', Qt::SkipEmptyParts);

    auto shouldRunSuite = [&selectedSuites](const QString &suiteName) {
        if (selectedSuites.isEmpty()) {
            return true;
        }

        for (const QString &candidate : selectedSuites) {
            if (candidate.trimmed().compare(suiteName, Qt::CaseInsensitive) == 0) {
                return true;
            }
        }
        return false;
    };

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

    auto runSuite = [&](const QString &suiteName, QObject *testObject) {
        if (!shouldRunSuite(suiteName)) {
            return;
        }
        ret += execTest(testObject);
    };

    DiscountMarkdownConverterTest test;
    runSuite(QStringLiteral("DiscountMarkdownConverterTest"), &test);

#ifdef ENABLE_HOEDOWN
    HoedownMarkdownConverterTest test2;
    runSuite(QStringLiteral("HoedownMarkdownConverterTest"), &test2);
#endif

    RevealMarkdownConverterTest test3;
    runSuite(QStringLiteral("RevealMarkdownConverterTest"), &test3);

    JsonSnippetFileTest test4;
    runSuite(QStringLiteral("JsonSnippetFileTest"), &test4);

    PmhMarkdownParserTest test5;
    runSuite(QStringLiteral("PmhMarkdownParserTest"), &test5);

    HtmlTemplateTest test7;
    runSuite(QStringLiteral("HtmlTemplateTest"), &test7);

    JsonThemeFileTest test8;
    runSuite(QStringLiteral("JsonThemeFileTest"), &test8);

    ThemeCollectionTest test9;
    runSuite(QStringLiteral("ThemeCollectionTest"), &test9);

    LaTeXExporterTest test11;
    runSuite(QStringLiteral("LaTeXExporterTest"), &test11);

    EpubExporterTest test12;
    runSuite(QStringLiteral("EpubExporterTest"), &test12);

    BlogPublisherTest test13;
    runSuite(QStringLiteral("BlogPublisherTest"), &test13);

    CloudSyncIntegrationTest test16;
    runSuite(QStringLiteral("CloudSyncIntegrationTest"), &test16);

    CollaborationIntegrationTest test10;
    runSuite(QStringLiteral("CollaborationIntegrationTest"), &test10);

    DataVisualizationPanelIntegrationTest test14;
    runSuite(QStringLiteral("DataVisualizationPanelIntegrationTest"), &test14);

    OcrIntegrationTest test15;
    runSuite(QStringLiteral("OcrIntegrationTest"), &test15);

    MainWindowLfsIntegrationTest test17;
    runSuite(QStringLiteral("MainWindowLfsIntegrationTest"), &test17);

    // Run WebEngine-dependent UI test last to avoid side effects on headless runs.
    HtmlPreviewControllerTest test6;
    runSuite(QStringLiteral("HtmlPreviewControllerTest"), &test6);

    return ret;
}

