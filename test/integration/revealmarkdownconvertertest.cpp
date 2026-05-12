// 文件说明：test\integration\revealmarkdownconvertertest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "revealmarkdownconvertertest.h"

#include <QtTest>

#include <converter/revealmarkdownconverter.h>

// 函数说明：实现 RevealMarkdownConverterTest::initTestCase 的核心逻辑，供当前模块调用。
void RevealMarkdownConverterTest::initTestCase()
{
    converter = new RevealMarkdownConverter();
}

// 函数说明：实现 RevealMarkdownConverterTest::convertsEmptyStringToEmptyHtml 的核心逻辑，供当前模块调用。
void RevealMarkdownConverterTest::convertsEmptyStringToEmptyHtml()
{
    MarkdownDocument *doc = converter->createDocument(QString(), MarkdownConverter::ConverterOptions());
    QString html = converter->renderAsHtml(doc);

    QVERIFY(html.isNull());
}

// 函数说明：实现 RevealMarkdownConverterTest::returnsAnyMarkdownTextUnchanged 的核心逻辑，供当前模块调用。
void RevealMarkdownConverterTest::returnsAnyMarkdownTextUnchanged()
{
    MarkdownDocument *doc = converter->createDocument(QStringLiteral("This is an example"), MarkdownConverter::ConverterOptions());
    QCOMPARE(converter->renderAsHtml(doc), QStringLiteral("This is an example"));

    doc = converter->createDocument(QStringLiteral("# This is an example"), MarkdownConverter::ConverterOptions());
    QCOMPARE(converter->renderAsHtml(doc), QStringLiteral("# This is an example"));

    doc = converter->createDocument(QStringLiteral("## This is an example"), MarkdownConverter::ConverterOptions());
    QCOMPARE(converter->renderAsHtml(doc), QStringLiteral("## This is an example"));
}

// 函数说明：实现 RevealMarkdownConverterTest::preservesGermanUmlautsInHtml 的核心逻辑，供当前模块调用。
void RevealMarkdownConverterTest::preservesGermanUmlautsInHtml()
{
    QString markdown = QStringLiteral("äöüß");

    MarkdownDocument *doc = converter->createDocument(markdown, MarkdownConverter::ConverterOptions());
    QString html = converter->renderAsHtml(doc);

    QVERIFY(!html.isEmpty());
    QCOMPARE(html, QStringLiteral("äöüß"));
}

// 函数说明：实现 RevealMarkdownConverterTest::cleanupTestCase 的核心逻辑，供当前模块调用。
void RevealMarkdownConverterTest::cleanupTestCase()
{
    delete converter;
}

