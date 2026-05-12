// 文件说明：test\integration\jsonsnippetfiletest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef JSONSNIPPETFILETEST_H
#define JSONSNIPPETFILETEST_H

#include <QObject>


class JsonSnippetFileTest : public QObject
{
    Q_OBJECT

private slots:
    void loadsEmptySnippetsCollectionFromFile();
    void loadsSnippetsCollectionFromFile();

    void savesEmptySnippetsCollectionToFile();
    void savesSnippetsCollectionToFile();

    void roundtripTest();
};

#endif // JSONSNIPPETFILETEST_H

