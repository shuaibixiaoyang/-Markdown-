// 文件说明：test\unit\gitmanagertest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
/*
 * Git manager reliability tests for Qt6 implementation.
 */
#ifndef GITMANAGERTEST_H
#define GITMANAGERTEST_H

#include <QObject>

class GitManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void checkoutFailureProvidesDetailedError();
    void statusParsingSupportsSpacesAndUnicode();
    void lfsTrackAndUntrackRoundTrip();
};

#endif // GITMANAGERTEST_H

