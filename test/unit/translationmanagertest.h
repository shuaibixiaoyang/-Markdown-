// 文件说明：test\unit\translationmanagertest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef TRANSLATIONMANAGERTEST_H
#define TRANSLATIONMANAGERTEST_H

#include <QObject>

class TranslationManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void loadsOfflineDictionaryFromJsonV1();
    void loadsOfflineDictionaryFromTsvCompat();
    void rejectsInvalidOfflineDictionaryFormat();
    void persistsSecretsEncryptedInSettings();
    void enforcesRateLimitForOnlineRequests();

private:
    QString m_oldOrganizationName;
    QString m_oldApplicationName;
};

#endif // TRANSLATIONMANAGERTEST_H

