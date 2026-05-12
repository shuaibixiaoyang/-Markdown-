// 文件说明：test\unit\snippettest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef SNIPPETTEST_H
#define SNIPPETTEST_H

#include <QObject>

class SnippetTest : public QObject
{
    Q_OBJECT

private slots:
    void isLessThanComparable();
    void isEqualComparable();
    void isInitializedAfterCreation();
};

#endif // SNIPPETTEST_H

