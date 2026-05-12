// 文件说明：test\integration\cloudsyncintegrationtest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
/*
 * Integration tests for CloudSync WebDAV security and resume transfer.
 */
#ifndef CLOUDSYNCINTEGRATIONTEST_H
#define CLOUDSYNCINTEGRATIONTEST_H

#include <QObject>

class CloudSyncIntegrationTest : public QObject
{
    Q_OBJECT

private slots:
    void saveConfigEncryptsWebDavCredentials();
    void startupConfigPathRestoresServiceList();
    void editedServicePersistsSslAndResumeAcrossRestart();
    void renameRollbackKeepsOriginalWhenTargetNameConflicts();
    void webdavRejectsHttpWhenSslVerificationEnabled();
    void webdavDownloadSupportsRangeResume();
    void webdavDownloadFallsBackToFullAfterUnsatisfiedRange();
    void webdavUploadSupportsContentRangeResume();
};

#endif // CLOUDSYNCINTEGRATIONTEST_H

