// 文件说明：test\integration\pmhmarkdownparsertest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef PMHMARKDOWNPARSERTEST_H
#define PMHMARKDOWNPARSERTEST_H

#include <QObject>

class PmhMarkdownParser;

class PmhMarkdownParserTest : public QObject
{
    Q_OBJECT
    
private slots:
    void initTestCase();
    void cleanupTestCase();

    void returnsEmptyMapForEmptyMarkdownDocument();
    void returnsNoEntryForNonExistingMarkdownElements();
    void returnsEntryForMarkdownElement();
    void returnsListOfEntriesForSingleMarkdownElementType();
    void entryKnowsItsMarkdownElementType();
    void entryHasStartAndEndPosition();

    void benchmark_data();
    void benchmark();

private:
    PmhMarkdownParser *parser;
};

#endif // PMHMARKDOWNPARSERTEST_H


