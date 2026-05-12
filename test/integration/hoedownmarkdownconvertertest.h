// 文件说明：test\integration\hoedownmarkdownconvertertest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef HOEDOWNMARKDOWNCONVERTERTEST_H
#define HOEDOWNMARKDOWNCONVERTERTEST_H

#include <QObject>

class HoedownMarkdownConverter;


class HoedownMarkdownConverterTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void convertsEmptyStringToEmptyHtml();
    void convertsMarkdownParagraphToHtml();
    void preservesGermanUmlautsInHtml();

    void supportsSuperscriptIfEnabled();
    void ignoresSuperscriptIfDisabled();

    void benchmark_data();
    void benchmark();
    void benchmarkTableOfContents_data();
    void benchmarkTableOfContents();

    void cleanupTestCase();

private:
    HoedownMarkdownConverter *converter;
};

#endif // HOEDOWNMARKDOWNCONVERTERTEST_H

