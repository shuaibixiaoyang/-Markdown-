// 文件说明：test\integration\revealmarkdownconvertertest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef REVEALMARKDOWNCONVERTERTEST_H
#define REVEALMARKDOWNCONVERTERTEST_H

#include <QObject>

class RevealMarkdownConverter;


class RevealMarkdownConverterTest : public QObject
{
    Q_OBJECT
    
private slots:
    void initTestCase();

    void convertsEmptyStringToEmptyHtml();
    void returnsAnyMarkdownTextUnchanged();
    void preservesGermanUmlautsInHtml();

    void cleanupTestCase();

private:
    RevealMarkdownConverter *converter;
};

#endif // REVEALMARKDOWNCONVERTERTEST_H

