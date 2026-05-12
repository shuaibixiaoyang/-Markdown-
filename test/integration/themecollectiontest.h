// 文件说明：test\integration\themecollectiontest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef THEMEMANAGERTEST_H
#define THEMEMANAGERTEST_H

#include <QObject>
class QTemporaryFile;
class ThemeCollection;


class ThemeCollectionTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void loadsThemesFromFileIntoCollection();

private:
    QTemporaryFile *themeFile;
};

#endif // THEMEMANAGERTEST_H



