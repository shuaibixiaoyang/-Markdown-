// 文件说明：test\unit\bailianclienttest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef BAILIANCLIENTTEST_H
#define BAILIANCLIENTTEST_H

#include <QObject>

class BaiLianClientTest : public QObject
{
    Q_OBJECT

private slots:
    void cancelPreviousRequestDoesNotBreakNextRequest();
};

#endif // BAILIANCLIENTTEST_H

