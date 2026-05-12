// 文件说明：test\integration\discountmarkdownconvertertest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef DISCOUNTMARKDOWNCONVERTERTEST_H
#define DISCOUNTMARKDOWNCONVERTERTEST_H

#include <QObject>

class DiscountMarkdownConverter;


class DiscountMarkdownConverterTest : public QObject
{
    Q_OBJECT
    
private slots:
    void initTestCase();

    void convertsEmptyStringToEmptyHtml();
    void convertsMarkdownParagraphToHtml();
    void convertsMarkdownHeaderToHtml();
    void preservesGermanUmlautsInHtml();

    void supportsSuperscriptIfEnabled();
    void ignoresSuperscriptIfDisabled();

    void benchmark_data();
    void benchmark();
    void benchmarkTableOfContents_data();
    void benchmarkTableOfContents();

    void cleanupTestCase();

private:
    void verifyConvertedHeader(const QString &html, int headerLevel);
    QString transformMarkdownToHtml(const QString &text);
    bool isIdAnchorDisabled(const QString &html);
    DiscountMarkdownConverter *converter;
};

#endif // DISCOUNTMARKDOWNCONVERTERTEST_H

