// 文件说明：test\unit\slidelinemappingtest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "slidelinemappingtest.h"

#include <QTest>

#include <slidelinemapping.h>

// 函数说明：实现 SlideLineMappingTest::holdsSingleEntryForEmptyDocuments 的核心逻辑，供当前模块调用。
void SlideLineMappingTest::holdsSingleEntryForEmptyDocuments()
{
    SlideLineMapping mapping;
    QString markdownDocument = "";
    QPair<int, int> expectedSlide(0, 0);
    int expectedLineNumber = 1;

    mapping.build(markdownDocument);

    QCOMPARE(mapping.lineToSlide().count(), 1);
    QCOMPARE(mapping.slideToLine().count(), 1);
    QCOMPARE(mapping.slideForLine(expectedLineNumber), expectedSlide);
    QCOMPARE(mapping.lineForSlide(expectedSlide), expectedLineNumber);
}

// 函数说明：实现 SlideLineMappingTest::horizontalSlideSeparatorMustBeSurroundedByBlankLines 的核心逻辑，供当前模块调用。
void SlideLineMappingTest::horizontalSlideSeparatorMustBeSurroundedByBlankLines()
{
    SlideLineMapping mapping;
    QString markdownDocument = "Text\n---\nText";
    QPair<int, int> expectedSlide(0, 0);
    int expectedLineNumber = 1;

    mapping.build(markdownDocument);

    QCOMPARE(mapping.lineToSlide().count(), 1);
    QCOMPARE(mapping.slideToLine().count(), 1);
    QCOMPARE(mapping.slideForLine(expectedLineNumber), expectedSlide);
    QCOMPARE(mapping.lineForSlide(expectedSlide), expectedLineNumber);
}

// 函数说明：实现 SlideLineMappingTest::verticalSlideSeparatorMustBeSurroundedByBlankLines 的核心逻辑，供当前模块调用。
void SlideLineMappingTest::verticalSlideSeparatorMustBeSurroundedByBlankLines()
{
    SlideLineMapping mapping;
    QString markdownDocument = "Text\n--\nText";
    QPair<int, int> expectedSlide(0, 0);
    int expectedLineNumber = 1;

    mapping.build(markdownDocument);

    QCOMPARE(mapping.lineToSlide().count(), 1);
    QCOMPARE(mapping.slideToLine().count(), 1);
    QCOMPARE(mapping.slideForLine(expectedLineNumber), expectedSlide);
    QCOMPARE(mapping.lineForSlide(expectedSlide), expectedLineNumber);
}

// 函数说明：实现 SlideLineMappingTest::holdsEntryForeachSlide 的核心逻辑，供当前模块调用。
void SlideLineMappingTest::holdsEntryForeachSlide()
{
    SlideLineMapping mapping;
    QString markdownDocument = "Slide 1\n\n---\n\nSlide 2\n\n---\n\nSlide3";

    mapping.build(markdownDocument);

    QCOMPARE(mapping.lineToSlide().count(), 3);
    QCOMPARE(mapping.slideToLine().count(), 3);

    QCOMPARE(mapping.slideForLine(3), qMakePair(0, 0));
    QCOMPARE(mapping.slideForLine(7), qMakePair(1, 0));
    QCOMPARE(mapping.slideForLine(9), qMakePair(2, 0));
    QCOMPARE(mapping.lineForSlide(qMakePair(0, 0)), 1);
    QCOMPARE(mapping.lineForSlide(qMakePair(1, 0)), 4);
    QCOMPARE(mapping.lineForSlide(qMakePair(2, 0)), 8);
}

// 函数说明：实现 SlideLineMappingTest::returnsSlideForEachLine 的核心逻辑，供当前模块调用。
void SlideLineMappingTest::returnsSlideForEachLine()
{
    SlideLineMapping mapping;
    QString markdownDocument = "Slide 1\n\n---\n\nSlide 2\n---\n";

    mapping.build(markdownDocument);

    QCOMPARE(mapping.lineToSlide().count(), 2);
    QCOMPARE(mapping.slideToLine().count(), 2);

    QCOMPARE(mapping.slideForLine(1), qMakePair(0, 0));
    QCOMPARE(mapping.slideForLine(2), qMakePair(0, 0));
    QCOMPARE(mapping.slideForLine(3), qMakePair(0, 0));
    QCOMPARE(mapping.slideForLine(4), qMakePair(1, 0));
    QCOMPARE(mapping.slideForLine(5), qMakePair(1, 0));
    QCOMPARE(mapping.slideForLine(6), qMakePair(1, 0));
    QCOMPARE(mapping.slideForLine(7), qMakePair(1, 0));
    QCOMPARE(mapping.slideForLine(8), qMakePair(-1, -1));
}

