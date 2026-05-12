// 文件说明：test\integration\hoedownmarkdownconvertertest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "hoedownmarkdownconvertertest.h"

#include <QtTest>

#include <converter/hoedownmarkdownconverter.h>
#include "loremipsumtestdata.h"

// 函数说明：实现 HoedownMarkdownConverterTest::initTestCase 的核心逻辑，供当前模块调用。
void HoedownMarkdownConverterTest::initTestCase()
{
    converter = new HoedownMarkdownConverter();
}

// 函数说明：实现 HoedownMarkdownConverterTest::convertsEmptyStringToEmptyHtml 的核心逻辑，供当前模块调用。
void HoedownMarkdownConverterTest::convertsEmptyStringToEmptyHtml()
{
    MarkdownDocument *doc = converter->createDocument(QString(), MarkdownConverter::ConverterOptions());
    QString html = converter->renderAsHtml(doc);

    QVERIFY(html.isNull());
}

// 函数说明：实现 HoedownMarkdownConverterTest::convertsMarkdownParagraphToHtml 的核心逻辑，供当前模块调用。
void HoedownMarkdownConverterTest::convertsMarkdownParagraphToHtml()
{
    QString markdown = "This is an example";

    MarkdownDocument *doc = converter->createDocument(markdown, MarkdownConverter::ConverterOptions());
    QString html = converter->renderAsHtml(doc);

    QVERIFY(!html.isEmpty());
    QVERIFY2(html.contains("<p>This is an example</p>"), qPrintable(html));
}

// 函数说明：实现 HoedownMarkdownConverterTest::preservesGermanUmlautsInHtml 的核心逻辑，供当前模块调用。
void HoedownMarkdownConverterTest::preservesGermanUmlautsInHtml()
{
    QString markdown = QStringLiteral("äöüß");

    MarkdownDocument *doc = converter->createDocument(markdown, MarkdownConverter::ConverterOptions());
    QString html = converter->renderAsHtml(doc);

    QVERIFY(!html.isEmpty());
    QVERIFY2(html.contains(QStringLiteral("<p>äöüß</p>")), qPrintable(html));
}

// 函数说明：实现 HoedownMarkdownConverterTest::supportsSuperscriptIfEnabled 的核心逻辑，供当前模块调用。
void HoedownMarkdownConverterTest::supportsSuperscriptIfEnabled()
{
    MarkdownDocument *doc = converter->createDocument(QStringLiteral("a^2"), MarkdownConverter::ConverterOptions());
    QCOMPARE(converter->renderAsHtml(doc), QStringLiteral("<p>a<sup>2</sup></p>\n"));
}

// 函数说明：实现 HoedownMarkdownConverterTest::ignoresSuperscriptIfDisabled 的核心逻辑，供当前模块调用。
void HoedownMarkdownConverterTest::ignoresSuperscriptIfDisabled()
{
    MarkdownDocument *doc = converter->createDocument(QStringLiteral("a^2"), HoedownMarkdownConverter::NoSuperscriptOption);
    QCOMPARE(converter->renderAsHtml(doc), QStringLiteral("<p>a^2</p>\n"));
}

// 函数说明：实现 HoedownMarkdownConverterTest::benchmark_data 的核心逻辑，供当前模块调用。
void HoedownMarkdownConverterTest::benchmark_data()
{
    QTest::addColumn<QString>("text");
    QTest::newRow("500 words text") << fiveHundredWordsLoremIpsumText;
    QString twoThousandWordsLoremIpsumText = (fiveHundredWordsLoremIpsumText +
                                              fiveHundredWordsLoremIpsumText +
                                              fiveHundredWordsLoremIpsumText +
                                              fiveHundredWordsLoremIpsumText);
    QTest::newRow("2000 words text") << twoThousandWordsLoremIpsumText;
    QString tenThousandWordsLoremIpsumText = (twoThousandWordsLoremIpsumText +
                                              twoThousandWordsLoremIpsumText +
                                              twoThousandWordsLoremIpsumText +
                                              twoThousandWordsLoremIpsumText +
                                              twoThousandWordsLoremIpsumText);
    QTest::newRow("10000 words text") << tenThousandWordsLoremIpsumText;
}

// 函数说明：实现 HoedownMarkdownConverterTest::benchmark 的核心逻辑，供当前模块调用。
void HoedownMarkdownConverterTest::benchmark()
{
    QFETCH(QString, text);
    QBENCHMARK {
        MarkdownDocument *doc = converter->createDocument(text, MarkdownConverter::ConverterOptions());
        QString html = converter->renderAsHtml(doc);
    }
}

// 函数说明：实现 HoedownMarkdownConverterTest::benchmarkTableOfContents_data 的核心逻辑，供当前模块调用。
void HoedownMarkdownConverterTest::benchmarkTableOfContents_data()
{
    QTest::addColumn<QString>("text");
    QTest::newRow("500 words text") << fiveHundredWordsLoremIpsumText;
    QString twoThousandWordsLoremIpsumText = (fiveHundredWordsLoremIpsumText +
                                              fiveHundredWordsLoremIpsumText +
                                              fiveHundredWordsLoremIpsumText +
                                              fiveHundredWordsLoremIpsumText);
    QTest::newRow("2000 words text") << twoThousandWordsLoremIpsumText;
    QString tenThousandWordsLoremIpsumText = (twoThousandWordsLoremIpsumText +
                                              twoThousandWordsLoremIpsumText +
                                              twoThousandWordsLoremIpsumText +
                                              twoThousandWordsLoremIpsumText +
                                              twoThousandWordsLoremIpsumText);
    QTest::newRow("10000 words text") << tenThousandWordsLoremIpsumText;
}

// 函数说明：实现 HoedownMarkdownConverterTest::benchmarkTableOfContents 的核心逻辑，供当前模块调用。
void HoedownMarkdownConverterTest::benchmarkTableOfContents()
{
    QFETCH(QString, text);
    QBENCHMARK {
        MarkdownDocument *doc = converter->createDocument(text, MarkdownConverter::ConverterOptions());
        QString toc = converter->renderAsTableOfContents(doc);
    }
}

// 函数说明：实现 HoedownMarkdownConverterTest::cleanupTestCase 的核心逻辑，供当前模块调用。
void HoedownMarkdownConverterTest::cleanupTestCase()
{
    delete converter;
}

