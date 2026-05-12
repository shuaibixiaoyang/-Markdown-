// 文件说明：test\integration\datavisualizationpanelintegrationtest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
/*
 * Integration tests for DataVisualizationPanel async flow and config compatibility.
 */
#ifndef DATAVISUALIZATIONPANELINTEGRATIONTEST_H
#define DATAVISUALIZATIONPANELINTEGRATIONTEST_H

#include <QObject>

class DataVisualizationPanelIntegrationTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void apiErrorAndTimeoutAlwaysReleasePending();
    void sqlCacheHitAndVariableChangeInvalidatesCache();
    void upgradesLegacyConnectionConfigViaDialogLoadSave();
};

#endif // DATAVISUALIZATIONPANELINTEGRATIONTEST_H

