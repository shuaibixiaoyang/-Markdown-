// 文件说明：test\unit\voiceinputtest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "voiceinputtest.h"

#include <QtTest>

#include <QCoreApplication>
#include <QFile>
#include <QRandomGenerator>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>

#include <input/voiceinput.h>

namespace {

QString makeUniqueTestId()
{
    return QString::number(QRandomGenerator::global()->generate(), 16);
}

void clearVoiceInputSettings()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("VoiceInput"));
    settings.remove(QString());
    settings.endGroup();
    settings.sync();
}

} // namespace

// 函数说明：实现 VoiceInputTest::init 的核心逻辑，供当前模块调用。
void VoiceInputTest::init()
{
    QStandardPaths::setTestModeEnabled(true);

    m_oldOrganizationName = QCoreApplication::organizationName();
    m_oldApplicationName = QCoreApplication::applicationName();

    QCoreApplication::setOrganizationName(QStringLiteral("CuteMarkEdVoiceTestOrg_%1").arg(makeUniqueTestId()));
    QCoreApplication::setApplicationName(QStringLiteral("CuteMarkEdVoiceTestApp_%1").arg(makeUniqueTestId()));

    clearVoiceInputSettings();
}

// 函数说明：实现 VoiceInputTest::cleanup 的核心逻辑，供当前模块调用。
void VoiceInputTest::cleanup()
{
    clearVoiceInputSettings();
    QCoreApplication::setOrganizationName(m_oldOrganizationName);
    QCoreApplication::setApplicationName(m_oldApplicationName);
}

// 函数说明：实现 VoiceInputTest::availableEnginesContainCloudAndOffline 的核心逻辑，供当前模块调用。
void VoiceInputTest::availableEnginesContainCloudAndOffline()
{
    const QStringList engines = VoiceInput::availableEngines();
    QVERIFY(engines.contains(QStringLiteral("CloudAPI")));
    QVERIFY(engines.contains(QStringLiteral("Offline")));
}

// 函数说明：实现 VoiceInputTest::offlineModelSpecIsDefined 的核心逻辑，供当前模块调用。
void VoiceInputTest::offlineModelSpecIsDefined()
{
    const QString spec = VoiceInput::offlineModelFormatSpecification();
    QVERIFY(spec.contains(QStringLiteral("whisper.cpp")));
    QVERIFY(spec.contains(QStringLiteral("vosk")));
    QVERIFY(spec.contains(QStringLiteral("{model}")));
    QVERIFY(spec.contains(QStringLiteral("{audio}")));
}

// 函数说明：实现 VoiceInputTest::validateOfflineModelRejectsInvalidConfig 的核心逻辑，供当前模块调用。
void VoiceInputTest::validateOfflineModelRejectsInvalidConfig()
{
    VoiceInput input;
    VoiceInput::Config cfg = input.config();
    cfg.engine = VoiceInput::Engine::Offline;
    cfg.offlineModelType = QStringLiteral("whisper.cpp");
    cfg.offlineModelPath.clear();
    input.setConfig(cfg);

    QString reason;
    QVERIFY(!input.validateOfflineModel(&reason));
    QVERIFY(!reason.isEmpty());
}

// 函数说明：实现 VoiceInputTest::validateOfflineModelAcceptsWhisperAndVoskPaths 的核心逻辑，供当前模块调用。
void VoiceInputTest::validateOfflineModelAcceptsWhisperAndVoskPaths()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString whisperModelPath = dir.filePath(QStringLiteral("ggml-base.bin"));
    QFile whisperModel(whisperModelPath);
    QVERIFY(whisperModel.open(QIODevice::WriteOnly));
    whisperModel.write("dummy-model");
    whisperModel.close();

    VoiceInput input;
    VoiceInput::Config cfg = input.config();
    cfg.engine = VoiceInput::Engine::Offline;
    cfg.offlineModelType = QStringLiteral("whisper.cpp");
    cfg.offlineModelPath = whisperModelPath;
    cfg.offlineDecoderExecutable.clear();
    input.setConfig(cfg);

    QString reason;
    QVERIFY(input.validateOfflineModel(&reason));

    cfg.offlineModelType = QStringLiteral("vosk");
    cfg.offlineModelPath = dir.path();
    input.setConfig(cfg);
    QVERIFY(input.validateOfflineModel(&reason));
}

