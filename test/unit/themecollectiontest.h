// 文件说明：test\unit\themecollectiontest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef THEMECOLLECTIONTEST_H
#define THEMECOLLECTIONTEST_H

#include <QObject>

class ThemeCollectionTest : public QObject
{
    Q_OBJECT

private slots:
    void returnsConstantNameOfJsonArray();
    void returnsNumberOfThemesInCollection();
    void returnsThemeAtIndexPosition();
    void returnsIfCollectionContainsTheme();
    void returnsThemeByName();
    void returnsNameOfAllThemes();
};

#endif // THEMECOLLECTIONTEST_H



