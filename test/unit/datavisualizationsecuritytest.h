// 文件说明：test\unit\datavisualizationsecuritytest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef DATAVISUALIZATIONSECURITYTEST_H
#define DATAVISUALIZATIONSECURITYTEST_H

#include <QObject>

class DataVisualizationSecurityTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void sqlSubstitutionUsesBindParams();
    void listVariableExpansionUsesBindParams();
    void databaseConnectorRejectsMultiStatementSql();
    void databaseConnectorPersistsTimeoutAndConnections();
};

#endif // DATAVISUALIZATIONSECURITYTEST_H

