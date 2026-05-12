// 文件说明：test\unit\voiceinputtest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef VOICEINPUTTEST_H
#define VOICEINPUTTEST_H

#include <QObject>

class VoiceInputTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void availableEnginesContainCloudAndOffline();
    void offlineModelSpecIsDefined();
    void validateOfflineModelRejectsInvalidConfig();
    void validateOfflineModelAcceptsWhisperAndVoskPaths();

private:
    QString m_oldOrganizationName;
    QString m_oldApplicationName;
};

#endif // VOICEINPUTTEST_H

