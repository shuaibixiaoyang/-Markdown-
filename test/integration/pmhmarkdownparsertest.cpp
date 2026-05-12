// 文件说明：test\integration\pmhmarkdownparsertest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "pmhmarkdownparsertest.h"

#include <QtTest>

#include <pmhmarkdownparser.h>
#include "loremipsumtestdata.h"

// 函数说明：实现 PmhMarkdownParserTest::initTestCase 的核心逻辑，供当前模块调用。
void PmhMarkdownParserTest::initTestCase()
{
    parser = new PmhMarkdownParser();
}

// 函数说明：实现 PmhMarkdownParserTest::cleanupTestCase 的核心逻辑，供当前模块调用。
void PmhMarkdownParserTest::cleanupTestCase()
{
    delete parser;
}

// 函数说明：实现 PmhMarkdownParserTest::returnsEmptyMapForEmptyMarkdownDocument 的核心逻辑，供当前模块调用。
void PmhMarkdownParserTest::returnsEmptyMapForEmptyMarkdownDocument()
{
    QMap<MarkdownElement::Type, QList<MarkdownElement> > elements = parser->parseMarkdown("");
    QVERIFY(elements.isEmpty());
}

// 函数说明：实现 PmhMarkdownParserTest::returnsNoEntryForNonExistingMarkdownElements 的核心逻辑，供当前模块调用。
void PmhMarkdownParserTest::returnsNoEntryForNonExistingMarkdownElements()
{
    QMap<MarkdownElement::Type, QList<MarkdownElement> > elements = parser->parseMarkdown("# Header 1\n");

    QVERIFY(!elements.isEmpty());
    QVERIFY(elements[MarkdownElement::LINK].isEmpty());
}

// 函数说明：实现 PmhMarkdownParserTest::returnsEntryForMarkdownElement 的核心逻辑，供当前模块调用。
void PmhMarkdownParserTest::returnsEntryForMarkdownElement()
{
    QMap<MarkdownElement::Type, QList<MarkdownElement> > elements = parser->parseMarkdown("# Header 1\n");

    QVERIFY(!elements.isEmpty());
    QVERIFY(!elements[MarkdownElement::H1].isEmpty());
}

// 函数说明：实现 PmhMarkdownParserTest::returnsListOfEntriesForSingleMarkdownElementType 的核心逻辑，供当前模块调用。
void PmhMarkdownParserTest::returnsListOfEntriesForSingleMarkdownElementType()
{
    QMap<MarkdownElement::Type, QList<MarkdownElement> > elements = parser->parseMarkdown("# Header 1a\n"
                                                                                          "# Header 1b\n");

    QVERIFY(!elements.isEmpty());
    QCOMPARE(elements[MarkdownElement::H1].count(), 2);
}

// 函数说明：实现 PmhMarkdownParserTest::entryKnowsItsMarkdownElementType 的核心逻辑，供当前模块调用。
void PmhMarkdownParserTest::entryKnowsItsMarkdownElementType() 
{
    QMap<MarkdownElement::Type, QList<MarkdownElement> > elements = parser->parseMarkdown("# Header 1\n");

    MarkdownElement element = elements[MarkdownElement::H1].first();

    QCOMPARE(element.type, MarkdownElement::H1);
}

// 函数说明：实现 PmhMarkdownParserTest::entryHasStartAndEndPosition 的核心逻辑，供当前模块调用。
void PmhMarkdownParserTest::entryHasStartAndEndPosition()
{
    QMap<MarkdownElement::Type, QList<MarkdownElement> > elements = parser->parseMarkdown("# Header 1\n");

    MarkdownElement element = elements[MarkdownElement::H1].first();

    QCOMPARE(element.start, (unsigned long)0);
    // parser adds two additional \n and returns position before last \n 
    // so we get 9 char + 2 \n = 11
    QCOMPARE(element.end, (unsigned long)11); 
}

// 函数说明：实现 PmhMarkdownParserTest::benchmark_data 的核心逻辑，供当前模块调用。
void PmhMarkdownParserTest::benchmark_data()
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

// 函数说明：实现 PmhMarkdownParserTest::benchmark 的核心逻辑，供当前模块调用。
void PmhMarkdownParserTest::benchmark()
{
    QFETCH(QString, text);
    QBENCHMARK {
        QMap<MarkdownElement::Type, QList<MarkdownElement> > elements = parser->parseMarkdown(text);
    }
}

