// 文件说明：test\unit\jsonthemetranslatortest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef JSONTHEMETRANSLATORTEST_H
#define JSONTHEMETRANSLATORTEST_H

#include <QObject>
class JsonThemeTranslator;


class JsonThemeTranslatorTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void doesNotProcessInvalidJsonDocument();
    void translatesEmptyJsonDocumentToEmptyThemes();
    void translatesJsonDocumentToThemes();
    void translatesThemesToJsonDocument();

private:
    JsonThemeTranslator *translator;
};

#endif // JSONTHEMETRANSLATORTEST_H




