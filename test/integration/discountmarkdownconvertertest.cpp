// 文件说明：test\integration\discountmarkdownconvertertest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "discountmarkdownconvertertest.h"

#include <QtTest>
#include <QRegularExpression>

#include <converter/discountmarkdownconverter.h>
#include "loremipsumtestdata.h"

// 函数说明：实现 DiscountMarkdownConverterTest::initTestCase 的核心逻辑，供当前模块调用。
void DiscountMarkdownConverterTest::initTestCase()
{
    converter = new DiscountMarkdownConverter();
}

// 函数说明：实现 DiscountMarkdownConverterTest::convertsEmptyStringToEmptyHtml 的核心逻辑，供当前模块调用。
void DiscountMarkdownConverterTest::convertsEmptyStringToEmptyHtml()
{
    QString html = transformMarkdownToHtml(QString());

    QVERIFY(html.isNull());
}

// 函数说明：实现 DiscountMarkdownConverterTest::convertsMarkdownParagraphToHtml 的核心逻辑，供当前模块调用。
void DiscountMarkdownConverterTest::convertsMarkdownParagraphToHtml()
{
    QString markdown = "This is an example";

    QString html = transformMarkdownToHtml(markdown);

    QVERIFY(!html.isEmpty());
    QCOMPARE(html, QStringLiteral("<p>This is an example</p>"));
}

// 函数说明：实现 DiscountMarkdownConverterTest::convertsMarkdownHeaderToHtml 的核心逻辑，供当前模块调用。
void DiscountMarkdownConverterTest::convertsMarkdownHeaderToHtml()
{
    QString html = transformMarkdownToHtml(QStringLiteral("# This is an example"));
    verifyConvertedHeader(html, 1);

    html = transformMarkdownToHtml(QStringLiteral("## This is an example"));
    verifyConvertedHeader(html, 2);
}

// 函数说明：实现 DiscountMarkdownConverterTest::preservesGermanUmlautsInHtml 的核心逻辑，供当前模块调用。
void DiscountMarkdownConverterTest::preservesGermanUmlautsInHtml()
{
#if defined(Q_CC_MSVC)
    QSKIP("This causes an assert with MSVC");
#endif

    QString markdown = QStringLiteral("äöü");

    QString html = transformMarkdownToHtml(markdown);

    QVERIFY(!html.isEmpty());
    QCOMPARE(html, QStringLiteral("<p>äöü</p>"));
}

// 函数说明：实现 DiscountMarkdownConverterTest::supportsSuperscriptIfEnabled 的核心逻辑，供当前模块调用。
void DiscountMarkdownConverterTest::supportsSuperscriptIfEnabled()
{
    QString html = transformMarkdownToHtml(QStringLiteral("a^2"));
    QCOMPARE(html, QStringLiteral("<p>a<sup>2</sup></p>"));
}

// 函数说明：实现 DiscountMarkdownConverterTest::ignoresSuperscriptIfDisabled 的核心逻辑，供当前模块调用。
void DiscountMarkdownConverterTest::ignoresSuperscriptIfDisabled()
{
    MarkdownDocument *doc = converter->createDocument(QStringLiteral("a^2"), DiscountMarkdownConverter::NoSuperscriptOption);
    QCOMPARE(converter->renderAsHtml(doc), QStringLiteral("<p>a^2</p>"));
}

// 函数说明：实现 DiscountMarkdownConverterTest::benchmark_data 的核心逻辑，供当前模块调用。
void DiscountMarkdownConverterTest::benchmark_data()
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

// 函数说明：实现 DiscountMarkdownConverterTest::benchmark 的核心逻辑，供当前模块调用。
void DiscountMarkdownConverterTest::benchmark()
{
    QFETCH(QString, text);
    QBENCHMARK {
        MarkdownDocument *doc = converter->createDocument(text, MarkdownConverter::ConverterOptions());
        QString html = converter->renderAsHtml(doc);
    }
}

// 函数说明：实现 DiscountMarkdownConverterTest::benchmarkTableOfContents_data 的核心逻辑，供当前模块调用。
void DiscountMarkdownConverterTest::benchmarkTableOfContents_data()
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

// 函数说明：实现 DiscountMarkdownConverterTest::benchmarkTableOfContents 的核心逻辑，供当前模块调用。
void DiscountMarkdownConverterTest::benchmarkTableOfContents()
{
    QFETCH(QString, text);
    QBENCHMARK {
        MarkdownDocument *doc = converter->createDocument(text, MarkdownConverter::ConverterOptions());
        QString toc = converter->renderAsTableOfContents(doc);
    }
}

// 函数说明：实现 DiscountMarkdownConverterTest::cleanupTestCase 的核心逻辑，供当前模块调用。
void DiscountMarkdownConverterTest::cleanupTestCase()
{
    delete converter;
}

// 函数说明：判断 DiscountMarkdownConverterTest 当前是否满足指定状态。
bool DiscountMarkdownConverterTest::isIdAnchorDisabled(const QString &html)
{
    // On some linux systems (e.g. Fedora 21) the compile option --with-id-anchor is
    // disabled for the discount library
    return html.startsWith("<a name=");
}

// 函数说明：实现 DiscountMarkdownConverterTest::verifyConvertedHeader 的核心逻辑，供当前模块调用。
void DiscountMarkdownConverterTest::verifyConvertedHeader(const QString &html, int headerLevel)
{
    // Discount versions differ in header anchor separator style (dot vs hyphen).
    static const QString kAnchor = QStringLiteral("This[.-]is[.-]an[.-]example");

    QRegularExpression expected;
    if (isIdAnchorDisabled(html)) {
        expected = QRegularExpression(
            QStringLiteral("^<a name=\\\"%1\\\"></a>\\n<h%2>This is an example</h%2>$")
                .arg(kAnchor)
                .arg(headerLevel));
    } else {
        expected = QRegularExpression(
            QStringLiteral("^<h%1 id=\\\"%2\\\">This is an example</h%1>$")
                .arg(headerLevel)
                .arg(kAnchor));
    }

    QVERIFY2(expected.match(html).hasMatch(), qPrintable(html));
}
    
// 函数说明：实现 DiscountMarkdownConverterTest::transformMarkdownToHtml 的核心逻辑，供当前模块调用。
QString DiscountMarkdownConverterTest::transformMarkdownToHtml(const QString &text)
{
    MarkdownDocument *doc = converter->createDocument(text, MarkdownConverter::ConverterOptions());
    return converter->renderAsHtml(doc);
}

