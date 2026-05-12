// 文件说明：test\unit\slidelinemappingtest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef SLIDELINEMAPPINGTEST_H
#define SLIDELINEMAPPINGTEST_H

#include <QObject>

class SlideLineMappingTest : public QObject
{
    Q_OBJECT

private slots:
    void holdsSingleEntryForEmptyDocuments();
    void horizontalSlideSeparatorMustBeSurroundedByBlankLines();
    void verticalSlideSeparatorMustBeSurroundedByBlankLines();
    void holdsEntryForeachSlide();
    void returnsSlideForEachLine();
};

#endif // SLIDELINEMAPPINGTEST_H


