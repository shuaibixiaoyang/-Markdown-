// 文件说明：test\integration\jsonthemefiletest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef JSONTHEMEFILETEST_H
#define JSONTHEMEFILETEST_H

#include <QObject>

class JsonThemeFileTest : public QObject
{
	Q_OBJECT

private slots:
    void loadsEmptyThemeCollectionFromFile();
    void loadsThemesCollectionFromFile();

    void savesEmptyThemesCollectionToFile();
    void savesThemesCollectionToFile();

    void roundtripTest();
};

#endif // JSONTHEMEFILETEST_H

