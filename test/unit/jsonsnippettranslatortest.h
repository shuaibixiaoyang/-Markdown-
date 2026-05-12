// 文件说明：test\unit\jsonsnippettranslatortest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef JSONSNIPPETTRANSLATORTEST_H
#define JSONSNIPPETTRANSLATORTEST_H

#include <QObject>
class JsonSnippetTranslator;


class JsonSnippetTranslatorTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void translatesJsonDocumentToSnippets();
    void translatesEmptyJsonDocumentToEmptySnippets();
    void defectIfJsonDocumentIsInvalid();

    void translatesSnippetCollectionToJsonDocument();

    void cleanupTestCase();

private:
    JsonSnippetTranslator *translator;
};

#endif // JSONSNIPPETTRANSLATORTEST_H

