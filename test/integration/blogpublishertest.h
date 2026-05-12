// 文件说明：test\integration\blogpublishertest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
/*
 * Integration tests for blog publisher configuration security/auth migration.
 */
#ifndef BLOGPUBLISHERTEST_H
#define BLOGPUBLISHERTEST_H

#include <QObject>

class BlogPublisherTest : public QObject
{
    Q_OBJECT

private slots:
    void savesConfigsWithoutPlaintextSecrets();
    void loadsEncryptedConfigsRoundtrip();
    void loadsLegacyBase64Configs();
};

#endif // BLOGPUBLISHERTEST_H

