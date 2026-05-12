// 文件说明：test\unit\translationmanagertest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "translationmanagertest.h"

#include <QtTest>

#include <QFile>
#include <QRandomGenerator>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>

#include <translation/translationmanager.h>

namespace {

void clearTranslationManagerSettings()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("TranslationManager"));
    settings.remove(QString());
    settings.endGroup();
    settings.sync();
}

QString makeUniqueTestId()
{
    return QString::number(QRandomGenerator::global()->generate(), 16);
}

} // namespace

// 函数说明：实现 TranslationManagerTest::init 的核心逻辑，供当前模块调用。
void TranslationManagerTest::init()
{
    QStandardPaths::setTestModeEnabled(true);

    m_oldOrganizationName = QCoreApplication::organizationName();
    m_oldApplicationName = QCoreApplication::applicationName();

    QCoreApplication::setOrganizationName(QStringLiteral("CuteMarkEdTestOrg_%1").arg(makeUniqueTestId()));
    QCoreApplication::setApplicationName(QStringLiteral("CuteMarkEdTestApp_%1").arg(makeUniqueTestId()));

    clearTranslationManagerSettings();
}

// 函数说明：实现 TranslationManagerTest::cleanup 的核心逻辑，供当前模块调用。
void TranslationManagerTest::cleanup()
{
    clearTranslationManagerSettings();
    QCoreApplication::setOrganizationName(m_oldOrganizationName);
    QCoreApplication::setApplicationName(m_oldApplicationName);
}

// 函数说明：加载 TranslationManagerTest 需要的数据、配置或外部资源。
void TranslationManagerTest::loadsOfflineDictionaryFromJsonV1()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString dictPath = dir.filePath(QStringLiteral("offline_dict.json"));
    QFile file(dictPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(
        "{\n"
        "  \"format\": \"cutemarked-offline-dict\",\n"
        "  \"version\": 1,\n"
        "  \"entries\": [\n"
        "    {\"source\": \"en\", \"target\": \"zh\", \"term\": \"hello\", \"translation\": \"你好\"},\n"
        "    {\"source\": \"en\", \"target\": \"zh\", \"term\": \"world\", \"translation\": \"世界\"}\n"
        "  ]\n"
        "}\n");
    file.close();

    TranslationManager manager;
    QVERIFY(manager.loadOfflineDictionary(dictPath));
    QCOMPARE(manager.lookupWord(QStringLiteral("hello"), QStringLiteral("zh")),
             QStringLiteral("你好"));
    QCOMPARE(manager.lookupWord(QStringLiteral("world"), QStringLiteral("zh")),
             QStringLiteral("世界"));
}

// 函数说明：加载 TranslationManagerTest 需要的数据、配置或外部资源。
void TranslationManagerTest::loadsOfflineDictionaryFromTsvCompat()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString dictPath = dir.filePath(QStringLiteral("offline_dict.tsv"));
    QFile file(dictPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(
        "# source\\ttarget\\tterm\\ttranslation\n"
        "en\tzh\tcat\t猫\n"
        "en\tzh\tdog\t狗\n");
    file.close();

    TranslationManager manager;
    QVERIFY(manager.loadOfflineDictionary(dictPath));
    QCOMPARE(manager.lookupWord(QStringLiteral("cat"), QStringLiteral("zh")),
             QStringLiteral("猫"));
    QCOMPARE(manager.lookupWord(QStringLiteral("dog"), QStringLiteral("zh")),
             QStringLiteral("狗"));
}

// 函数说明：实现 TranslationManagerTest::rejectsInvalidOfflineDictionaryFormat 的核心逻辑，供当前模块调用。
void TranslationManagerTest::rejectsInvalidOfflineDictionaryFormat()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString dictPath = dir.filePath(QStringLiteral("invalid_dict.json"));
    QFile file(dictPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(
        "{\n"
        "  \"format\": \"unknown-format\",\n"
        "  \"version\": 99,\n"
        "  \"entries\": []\n"
        "}\n");
    file.close();

    TranslationManager manager;
    QVERIFY(!manager.loadOfflineDictionary(dictPath));
    QVERIFY(!manager.lastError().isEmpty());
}

// 函数说明：实现 TranslationManagerTest::persistsSecretsEncryptedInSettings 的核心逻辑，供当前模块调用。
void TranslationManagerTest::persistsSecretsEncryptedInSettings()
{
    TranslationManager::Config config;
    config.engine = TranslationManager::Engine::DeepL;
    config.appId = QStringLiteral("app-id-value");
    config.apiKey = QStringLiteral("secret-api-key-123");
    config.apiSecret = QStringLiteral("secret-api-secret-456");
    config.defaultSourceLang = QStringLiteral("auto");
    config.defaultTargetLang = QStringLiteral("ja");
    config.enableRateLimit = true;
    config.maxRequestsPerMinute = 77;
    config.minRequestIntervalMs = 432;

    {
        TranslationManager manager;
        manager.setConfig(config);
        manager.saveSettings();
    }

    QSettings settings;
    settings.beginGroup(QStringLiteral("TranslationManager"));
    const QString storedApiKey = settings.value(QStringLiteral("apiKey")).toString();
    const QString storedApiSecret = settings.value(QStringLiteral("apiSecret")).toString();
    settings.endGroup();

    QVERIFY(!storedApiKey.contains(config.apiKey));
    QVERIFY(!storedApiSecret.contains(config.apiSecret));
    QVERIFY(storedApiKey.startsWith(QStringLiteral("enc:v1:")));
    QVERIFY(storedApiSecret.startsWith(QStringLiteral("enc:v1:")));

    TranslationManager loaded;
    const TranslationManager::Config loadedConfig = loaded.config();
    QCOMPARE(loadedConfig.engine, config.engine);
    QCOMPARE(loadedConfig.appId, config.appId);
    QCOMPARE(loadedConfig.apiKey, config.apiKey);
    QCOMPARE(loadedConfig.apiSecret, config.apiSecret);
    QCOMPARE(loadedConfig.defaultTargetLang, config.defaultTargetLang);
    QCOMPARE(loadedConfig.maxRequestsPerMinute, config.maxRequestsPerMinute);
    QCOMPARE(loadedConfig.minRequestIntervalMs, config.minRequestIntervalMs);
}

// 函数说明：实现 TranslationManagerTest::enforcesRateLimitForOnlineRequests 的核心逻辑，供当前模块调用。
void TranslationManagerTest::enforcesRateLimitForOnlineRequests()
{
    TranslationManager manager;

    TranslationManager::Config config = manager.config();
    config.engine = TranslationManager::Engine::DeepL;
    config.apiKey = QStringLiteral("dummy-key");
    config.enableRateLimit = true;
    config.maxRequestsPerMinute = 1;
    config.minRequestIntervalMs = 60000;
    manager.setConfig(config);

    QSignalSpy errorSpy(&manager, &TranslationManager::errorOccurred);

    manager.translate(QStringLiteral("hello"), QStringLiteral("zh"), QStringLiteral("en"));
    manager.translate(QStringLiteral("world"), QStringLiteral("zh"), QStringLiteral("en"));

    QTRY_VERIFY(errorSpy.count() >= 1);

    bool hasRateLimitError = false;
    for (int i = 0; i < errorSpy.count(); ++i) {
        const QString message = errorSpy.at(i).at(0).toString();
        if (message.contains(QStringLiteral("速率限制")) ||
            message.contains(QStringLiteral("频繁"))) {
            hasRateLimitError = true;
            break;
        }
    }

    QVERIFY(hasRateLimitError);
}

