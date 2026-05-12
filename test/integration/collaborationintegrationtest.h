// 文件说明：test\integration\collaborationintegrationtest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
/*
 * Integration tests for collaboration transport and recovery protocol.
 */
#ifndef COLLABORATIONINTEGRATIONTEST_H
#define COLLABORATIONINTEGRATIONTEST_H

#include <QObject>

class CollaborationIntegrationTest : public QObject
{
    Q_OBJECT

private slots:
    void disconnectReconnectReceivesIncrementalDelta();
    void syncFallsBackToFullStateWhenOperationLogHasGap();
    void wssHandshakeAndJoinWorksWithTlsEnabled();
};

#endif // COLLABORATIONINTEGRATIONTEST_H


