// 文件说明：test\integration\blogpublishertest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
/*
 * Integration tests for blog publisher configuration.
 */
#include "blogpublishertest.h"

#include <QtTest>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include <publishing/blogpublisher.h>

// 函数说明：保存 BlogPublisherTest 当前状态，保证用户修改可以持久化。
void BlogPublisherTest::savesConfigsWithoutPlaintextSecrets()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString configPath = tempDir.filePath(QStringLiteral("blog-configs.json"));

    BlogPublisher::BlogConfig config;
    config.platform = BlogPublisher::Platform::CSDN;
    config.name = QStringLiteral("test");

    QVERIFY(BlogPublisher::saveConfigs({config}, configPath));

    QFile file(configPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QVERIFY(doc.isArray());
    QCOMPARE(doc.array().size(), 1);
}

// 函数说明：加载 BlogPublisherTest 需要的数据、配置或外部资源。
void BlogPublisherTest::loadsEncryptedConfigsRoundtrip()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString configPath = tempDir.filePath(QStringLiteral("blog-configs.json"));

    BlogPublisher::BlogConfig original;
    original.platform = BlogPublisher::Platform::Zhihu;
    original.name = QStringLiteral("my-zhihu");

    QVERIFY(BlogPublisher::saveConfigs({original}, configPath));

    const QList<BlogPublisher::BlogConfig> loaded = BlogPublisher::loadConfigs(configPath);
    QCOMPARE(loaded.size(), 1);

    const BlogPublisher::BlogConfig actual = loaded.first();
    QCOMPARE(static_cast<int>(actual.platform), static_cast<int>(original.platform));
    QCOMPARE(actual.name, original.name);
}

// 函数说明：加载 BlogPublisherTest 需要的数据、配置或外部资源。
void BlogPublisherTest::loadsLegacyBase64Configs()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString configPath = tempDir.filePath(QStringLiteral("legacy-blog-configs.json"));

    QJsonObject obj;
    obj[QStringLiteral("platform")] = static_cast<int>(BlogPublisher::Platform::Nowcoder);
    obj[QStringLiteral("name")] = QStringLiteral("legacy");

    QJsonArray array;
    array.append(obj);

    QFile file(configPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(QJsonDocument(array).toJson(QJsonDocument::Compact));
    file.close();

    const QList<BlogPublisher::BlogConfig> loaded = BlogPublisher::loadConfigs(configPath);
    QCOMPARE(loaded.size(), 1);

    const BlogPublisher::BlogConfig actual = loaded.first();
    QCOMPARE(static_cast<int>(actual.platform), static_cast<int>(BlogPublisher::Platform::Nowcoder));
    QCOMPARE(actual.name, QStringLiteral("legacy"));
}

