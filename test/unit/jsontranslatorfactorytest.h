// 文件说明：test\unit\jsontranslatorfactorytest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef JSONTRANSLATORFACTORYTEST_H
#define JSONTRANSLATORFACTORYTEST_H

#include <QObject>

class JsonTranslatorFactoryTest : public QObject
{
    Q_OBJECT

private slots:
    void returnsNullIfNoJsonTranslatorExists();
    void returnsValidJsonTranslatorForSnippets();
};

#endif // JSONTRANSLATORFACTORYTEST_H


