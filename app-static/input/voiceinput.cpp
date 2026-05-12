// 文件说明：app-static\input\voiceinput.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "voiceinput.h"

#include <QAudioDevice>
#include <QMediaDevices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QDataStream>
#include <QtEndian>
#include <cmath>

#ifdef Q_OS_MAC
#include <AudioToolbox/AudioToolbox.h>
#endif

QMap<QString, VoiceInput::VoiceCommand> VoiceInput::s_commandMap;

// 函数说明：构造 VoiceInput 对象，初始化本模块需要的状态、界面和资源。
VoiceInput::VoiceInput(QObject *parent)
    : QObject(parent)
    , m_state(State::Idle)
    , m_audioSource(nullptr)
    , m_audioDevice(nullptr)
    , m_buffer(new QBuffer(this))
    , m_networkManager(new QNetworkAccessManager(this))
    , m_silenceTimer(new QTimer(this))
    , m_currentVolume(0.0f)
    , m_noiseFloor(0.0f)
    , m_effectiveSilenceThreshold(0.02f)
    , m_noiseSampleCount(0)
    , m_recordingStartTime(0)
    , m_effectiveEngine(Engine::System)
{
    initCommandMap();
    loadSettings();
    m_effectiveSilenceThreshold = m_config.silenceThreshold;

    connect(m_silenceTimer, &QTimer::timeout,
            this, &VoiceInput::onSilenceTimeout);
}

// 函数说明：销毁 VoiceInput 对象，释放本模块持有的资源。
VoiceInput::~VoiceInput()
{
    stopRecording();
    saveSettings();
}

// 函数说明：设置 VoiceInput 的运行参数，并触发必要的界面或数据刷新。
void VoiceInput::setConfig(const Config &config)
{
    m_config = config;
    m_config.sampleRate = qBound(8000, m_config.sampleRate, 48000);
    m_config.silenceTimeout = qBound(0, m_config.silenceTimeout, 60000);
    m_config.silenceThreshold = qBound(0.001f, m_config.silenceThreshold, 0.5f);
    m_config.adaptiveSensitivity = qBound(1.0f, m_config.adaptiveSensitivity, 6.0f);
    m_config.minSilenceThreshold = qBound(0.001f, m_config.minSilenceThreshold, 0.3f);
    m_config.maxSilenceThreshold = qBound(0.005f, m_config.maxSilenceThreshold, 0.5f);
    if (m_config.maxSilenceThreshold < m_config.minSilenceThreshold) {
        qSwap(m_config.maxSilenceThreshold, m_config.minSilenceThreshold);
    }
    m_config.offlineDecodeTimeoutMs = qBound(1000, m_config.offlineDecodeTimeoutMs, 120000);

    m_noiseFloor = 0.0f;
    m_noiseSampleCount = 0;
    m_effectiveSilenceThreshold = m_config.silenceThreshold;
    saveSettings();
}

// 函数说明：保存 VoiceInput 当前状态，保证用户修改可以持久化。
void VoiceInput::saveSettings() const
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("VoiceInput"));
    settings.setValue(QStringLiteral("engine"), static_cast<int>(m_config.engine));
    settings.setValue(QStringLiteral("language"), m_config.language);
    settings.setValue(QStringLiteral("apiKey"), m_config.apiKey);
    settings.setValue(QStringLiteral("apiUrl"), m_config.apiUrl);
    settings.setValue(QStringLiteral("enableCommands"), m_config.enableCommands);
    settings.setValue(QStringLiteral("enablePunctuation"), m_config.enablePunctuation);
    settings.setValue(QStringLiteral("enableContinuous"), m_config.enableContinuous);
    settings.setValue(QStringLiteral("sampleRate"), m_config.sampleRate);
    settings.setValue(QStringLiteral("silenceTimeout"), m_config.silenceTimeout);
    settings.setValue(QStringLiteral("silenceThreshold"), m_config.silenceThreshold);
    settings.setValue(QStringLiteral("enableAdaptiveSilenceThreshold"),
                      m_config.enableAdaptiveSilenceThreshold);
    settings.setValue(QStringLiteral("adaptiveSensitivity"), m_config.adaptiveSensitivity);
    settings.setValue(QStringLiteral("minSilenceThreshold"), m_config.minSilenceThreshold);
    settings.setValue(QStringLiteral("maxSilenceThreshold"), m_config.maxSilenceThreshold);
    settings.setValue(QStringLiteral("offlineModelType"), m_config.offlineModelType);
    settings.setValue(QStringLiteral("offlineModelPath"), m_config.offlineModelPath);
    settings.setValue(QStringLiteral("offlineDecoderExecutable"),
                      m_config.offlineDecoderExecutable);
    settings.setValue(QStringLiteral("offlineDecoderArguments"),
                      m_config.offlineDecoderArguments);
    settings.setValue(QStringLiteral("offlineDecodeTimeoutMs"),
                      m_config.offlineDecodeTimeoutMs);
    settings.endGroup();
}

// 函数说明：加载 VoiceInput 需要的数据、配置或外部资源。
void VoiceInput::loadSettings()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("VoiceInput"));
    Config loaded;
    loaded.engine = static_cast<Engine>(
        settings.value(QStringLiteral("engine"), static_cast<int>(loaded.engine)).toInt());
    loaded.language = settings.value(QStringLiteral("language"), loaded.language).toString();
    loaded.apiKey = settings.value(QStringLiteral("apiKey"), loaded.apiKey).toString();
    loaded.apiUrl = settings.value(QStringLiteral("apiUrl"), loaded.apiUrl).toString();
    loaded.enableCommands = settings.value(
        QStringLiteral("enableCommands"), loaded.enableCommands).toBool();
    loaded.enablePunctuation = settings.value(
        QStringLiteral("enablePunctuation"), loaded.enablePunctuation).toBool();
    loaded.enableContinuous = settings.value(
        QStringLiteral("enableContinuous"), loaded.enableContinuous).toBool();
    loaded.sampleRate = settings.value(QStringLiteral("sampleRate"), loaded.sampleRate).toInt();
    loaded.silenceTimeout = settings.value(
        QStringLiteral("silenceTimeout"), loaded.silenceTimeout).toInt();
    loaded.silenceThreshold = settings.value(
        QStringLiteral("silenceThreshold"), loaded.silenceThreshold).toFloat();
    loaded.enableAdaptiveSilenceThreshold = settings.value(
        QStringLiteral("enableAdaptiveSilenceThreshold"),
        loaded.enableAdaptiveSilenceThreshold).toBool();
    loaded.adaptiveSensitivity = settings.value(
        QStringLiteral("adaptiveSensitivity"), loaded.adaptiveSensitivity).toFloat();
    loaded.minSilenceThreshold = settings.value(
        QStringLiteral("minSilenceThreshold"), loaded.minSilenceThreshold).toFloat();
    loaded.maxSilenceThreshold = settings.value(
        QStringLiteral("maxSilenceThreshold"), loaded.maxSilenceThreshold).toFloat();
    loaded.offlineModelType = settings.value(
        QStringLiteral("offlineModelType"), loaded.offlineModelType).toString();
    loaded.offlineModelPath = settings.value(
        QStringLiteral("offlineModelPath"), loaded.offlineModelPath).toString();
    loaded.offlineDecoderExecutable = settings.value(
        QStringLiteral("offlineDecoderExecutable"),
        loaded.offlineDecoderExecutable).toString();
    loaded.offlineDecoderArguments = settings.value(
        QStringLiteral("offlineDecoderArguments"), loaded.offlineDecoderArguments).toStringList();
    loaded.offlineDecodeTimeoutMs = settings.value(
        QStringLiteral("offlineDecodeTimeoutMs"), loaded.offlineDecodeTimeoutMs).toInt();
    settings.endGroup();

    setConfig(loaded);
}

// 函数说明：判断 VoiceInput 当前是否满足指定状态。
bool VoiceInput::isAvailable()
{
    QAudioDevice inputDevice = QMediaDevices::defaultAudioInput();
    return !inputDevice.isNull();
}

// 函数说明：实现 VoiceInput::availableLanguages 的核心逻辑，供当前模块调用。
QStringList VoiceInput::availableLanguages()
{
    return QStringList()
        << "zh-CN"      // 中文（简体）
        << "zh-TW"      // 中文（繁体）
        << "en-US"      // 英语（美国）
        << "en-GB"      // 英语（英国）
        << "ja-JP"      // 日语
        << "ko-KR"      // 韩语
        << "fr-FR"      // 法语
        << "de-DE"      // 德语
        << "es-ES"      // 西班牙语
        << "ru-RU"      // 俄语
        << "pt-BR"      // 葡萄牙语（巴西）
        << "it-IT";     // 意大利语
}

// 函数说明：实现 VoiceInput::availableEngines 的核心逻辑，供当前模块调用。
QStringList VoiceInput::availableEngines()
{
    QStringList engines;
    if (isSystemEngineAvailable()) {
        engines << QStringLiteral("System");
    }
    engines << QStringLiteral("CloudAPI");
    engines << QStringLiteral("Offline");
    return engines;
}

// 函数说明：判断 VoiceInput 当前是否满足指定状态。
bool VoiceInput::isSystemEngineAvailable()
{
#if defined(CUTEMARKED_ENABLE_SYSTEM_SPEECH)
#ifdef Q_OS_MACOS
    return true;
#elif defined(Q_OS_WIN)
    return true;
#else
    return false;
#endif
#else
    return false;
#endif
}

// 函数说明：实现 VoiceInput::supportedOfflineModelTypes 的核心逻辑，供当前模块调用。
QStringList VoiceInput::supportedOfflineModelTypes()
{
    return QStringList()
        << QStringLiteral("whisper.cpp")
        << QStringLiteral("vosk");
}

// 函数说明：实现 VoiceInput::offlineModelFormatSpecification 的核心逻辑，供当前模块调用。
QString VoiceInput::offlineModelFormatSpecification()
{
    return QStringLiteral(
        "离线模型定义：\n"
        "1) offlineModelType: whisper.cpp 或 vosk\n"
        "2) offlineModelPath:\n"
        "   - whisper.cpp: 模型文件路径（例如 ggml-base.bin）\n"
        "   - vosk: 模型目录路径\n"
        "3) offlineDecoderExecutable: 可选。外部解码器程序路径。\n"
        "4) offlineDecoderArguments: 可选参数列表，支持占位符：\n"
        "   {model} {audio} {lang}\n"
        "   例如 whisper.cpp: -m {model} -f {audio} -l {lang}");
}

// 函数说明：实现 VoiceInput::validateOfflineModel 的核心逻辑，供当前模块调用。
bool VoiceInput::validateOfflineModel(QString *reason) const
{
    const QString modelType = m_config.offlineModelType.trimmed().toLower();
    if (!supportedOfflineModelTypes().contains(modelType)) {
        if (reason) {
            *reason = tr("不支持的离线模型类型：%1").arg(m_config.offlineModelType);
        }
        return false;
    }

    if (m_config.offlineModelPath.trimmed().isEmpty()) {
        if (reason) {
            *reason = tr("未配置离线模型路径");
        }
        return false;
    }

    const QFileInfo modelInfo(m_config.offlineModelPath);
    if (!modelInfo.exists()) {
        if (reason) {
            *reason = tr("离线模型路径不存在：%1").arg(m_config.offlineModelPath);
        }
        return false;
    }

    if (modelType == QStringLiteral("whisper.cpp") && !modelInfo.isFile()) {
        if (reason) {
            *reason = tr("whisper.cpp 需要模型文件路径");
        }
        return false;
    }

    if (modelType == QStringLiteral("vosk") && !modelInfo.isDir()) {
        if (reason) {
            *reason = tr("vosk 需要模型目录路径");
        }
        return false;
    }

    if (!m_config.offlineDecoderExecutable.trimmed().isEmpty()) {
        const QFileInfo decoderInfo(m_config.offlineDecoderExecutable);
        if (!decoderInfo.exists() || !decoderInfo.isFile() || !decoderInfo.isExecutable()) {
            if (reason) {
                *reason = tr("离线解码器不可执行：%1").arg(m_config.offlineDecoderExecutable);
            }
            return false;
        }
    }

    return true;
}

// 函数说明：实现 VoiceInput::resolveEffectiveEngine 的核心逻辑，供当前模块调用。
VoiceInput::Engine VoiceInput::resolveEffectiveEngine(QString *reason) const
{
    const bool cloudReady = !m_config.apiUrl.trimmed().isEmpty() && !m_config.apiKey.trimmed().isEmpty();
    QString offlineReason;
    const bool offlineReady = validateOfflineModel(&offlineReason);
    const bool systemReady = isSystemEngineAvailable();

    auto assignReason = [reason](const QString &text) {
        if (reason) {
            *reason = text;
        }
    };

    switch (m_config.engine) {
    case Engine::System:
        if (systemReady) {
            assignReason(QString());
            return Engine::System;
        }
        if (offlineReady) {
            assignReason(QObject::tr("系统语音引擎在当前平台不可用，已自动切换到离线模型"));
            return Engine::Offline;
        }
        if (cloudReady) {
            assignReason(QObject::tr("系统语音引擎在当前平台不可用，已自动切换到云端 API"));
            return Engine::CloudAPI;
        }
        assignReason(QObject::tr("系统语音引擎不可用，且未配置可用的离线模型/云端 API"));
        return m_config.engine;
    case Engine::Offline:
        if (offlineReady) {
            assignReason(QString());
            return Engine::Offline;
        }
        if (cloudReady) {
            assignReason(QObject::tr("离线模型不可用（%1），已自动切换到云端 API").arg(offlineReason));
            return Engine::CloudAPI;
        }
        if (systemReady) {
            assignReason(QObject::tr("离线模型不可用（%1），已自动切换到系统语音引擎").arg(offlineReason));
            return Engine::System;
        }
        assignReason(QObject::tr("离线模型不可用：%1").arg(offlineReason));
        return m_config.engine;
    case Engine::CloudAPI:
        if (cloudReady) {
            assignReason(QString());
            return Engine::CloudAPI;
        }
        if (offlineReady) {
            assignReason(QObject::tr("云端 API 未配置，已自动切换到离线模型"));
            return Engine::Offline;
        }
        if (systemReady) {
            assignReason(QObject::tr("云端 API 未配置，已自动切换到系统语音引擎"));
            return Engine::System;
        }
        assignReason(QObject::tr("云端 API 未配置，且无可用的离线模型/系统语音引擎"));
        return m_config.engine;
    }

    assignReason(QObject::tr("未知语音引擎配置"));
    return m_config.engine;
}

// 函数说明：启动 VoiceInput 的异步任务、会话或后台流程。
bool VoiceInput::startRecording()
{
    if (m_state == State::Recording) {
        return true;
    }

    if (!isAvailable()) {
        m_lastError = tr("没有可用的音频输入设备");
        emit errorOccurred(m_lastError);
        return false;
    }

    QString engineReason;
    m_effectiveEngine = resolveEffectiveEngine(&engineReason);
    if (!engineReason.isEmpty() && m_effectiveEngine != m_config.engine) {
        emit engineFallbackApplied(m_config.engine, m_effectiveEngine, engineReason);
    }

    if (m_effectiveEngine == Engine::CloudAPI &&
        (m_config.apiUrl.trimmed().isEmpty() || m_config.apiKey.trimmed().isEmpty())) {
        m_lastError = tr("云端 API 未配置（需要 API URL 与 API Key）");
        emit errorOccurred(m_lastError);
        return false;
    }

    if (m_effectiveEngine == Engine::Offline) {
        QString offlineReason;
        if (!validateOfflineModel(&offlineReason)) {
            m_lastError = tr("离线模型不可用：%1").arg(offlineReason);
            emit errorOccurred(m_lastError);
            return false;
        }
    }

    if (m_effectiveEngine == Engine::System && !isSystemEngineAvailable()) {
        m_lastError = tr("系统语音引擎在当前平台不可用");
        emit errorOccurred(m_lastError);
        return false;
    }

    initAudioInput();

    if (!m_audioSource) {
        m_lastError = tr("无法初始化音频输入");
        emit errorOccurred(m_lastError);
        return false;
    }

    m_audioBuffer.clear();
    m_buffer->setBuffer(&m_audioBuffer);
    m_buffer->open(QIODevice::WriteOnly);

    m_audioDevice = m_audioSource->start();
    if (!m_audioDevice) {
        m_lastError = tr("无法启动音频录制");
        emit errorOccurred(m_lastError);
        return false;
    }

    connect(m_audioDevice, &QIODevice::readyRead,
            this, &VoiceInput::onAudioDataReady);

    m_state = State::Recording;
    m_recordingStartTime = QDateTime::currentMSecsSinceEpoch();
    m_noiseFloor = 0.0f;
    m_noiseSampleCount = 0;
    m_effectiveSilenceThreshold = m_config.silenceThreshold;

    if (m_config.silenceTimeout > 0) {
        m_silenceTimer->start(m_config.silenceTimeout);
    }

    emit stateChanged(m_state);
    emit recordingStarted();

    return true;
}

// 函数说明：停止 VoiceInput 正在运行的任务或会话。
void VoiceInput::stopRecording()
{
    if (m_state == State::Idle) {
        return;
    }

    m_silenceTimer->stop();

    if (m_audioSource) {
        m_audioSource->stop();
        delete m_audioSource;
        m_audioSource = nullptr;
    }

    m_audioDevice = nullptr;
    m_buffer->close();

    // 执行识别
    if (!m_audioBuffer.isEmpty()) {
        if (m_effectiveEngine == Engine::CloudAPI) {
            recognizeWithCloudAPI(m_audioBuffer);
        } else if (m_effectiveEngine == Engine::Offline) {
            recognizeWithOffline(m_audioBuffer);
        } else {
            recognizeWithSystem(m_audioBuffer);
        }
    }

    m_state = State::Idle;
    emit stateChanged(m_state);
    emit recordingStopped();
}

// 函数说明：实现 VoiceInput::pauseRecording 的核心逻辑，供当前模块调用。
void VoiceInput::pauseRecording()
{
    if (m_state != State::Recording) {
        return;
    }

    if (m_audioSource) {
        m_audioSource->suspend();
    }

    m_silenceTimer->stop();
    m_state = State::Paused;
    emit stateChanged(m_state);
    emit recordingPaused();
}

// 函数说明：实现 VoiceInput::resumeRecording 的核心逻辑，供当前模块调用。
void VoiceInput::resumeRecording()
{
    if (m_state != State::Paused) {
        return;
    }

    if (m_audioSource) {
        m_audioSource->resume();
    }

    if (m_config.silenceTimeout > 0) {
        m_silenceTimer->start(m_config.silenceTimeout);
    }

    m_state = State::Recording;
    emit stateChanged(m_state);
    emit recordingResumed();
}

// 函数说明：实现 VoiceInput::recordingDuration 的核心逻辑，供当前模块调用。
int VoiceInput::recordingDuration() const
{
    if (m_recordingStartTime == 0) {
        return 0;
    }
    return QDateTime::currentMSecsSinceEpoch() - m_recordingStartTime;
}

// 函数说明：解析输入内容，转换为 VoiceInput 后续处理使用的数据结构。
VoiceInput::VoiceCommand VoiceInput::parseCommand(const QString &text)
{
    if (s_commandMap.isEmpty()) {
        initCommandMap();
    }

    QString lower = text.toLower().trimmed();

    // 检查完全匹配
    if (s_commandMap.contains(lower)) {
        return s_commandMap[lower];
    }

    // 检查部分匹配
    for (auto it = s_commandMap.begin(); it != s_commandMap.end(); ++it) {
        if (lower.contains(it.key())) {
            return it.value();
        }
    }

    return VoiceCommand::None;
}

// 函数说明：实现 VoiceInput::commandToMarkdown 的核心逻辑，供当前模块调用。
QString VoiceInput::commandToMarkdown(VoiceCommand command)
{
    switch (command) {
        case VoiceCommand::NewLine:
            return "\n";
        case VoiceCommand::NewParagraph:
            return "\n\n";
        case VoiceCommand::Period:
            return "。";
        case VoiceCommand::Comma:
            return "，";
        case VoiceCommand::QuestionMark:
            return "？";
        case VoiceCommand::ExclamationMark:
            return "！";
        case VoiceCommand::Colon:
            return "：";
        case VoiceCommand::Semicolon:
            return ";";
        case VoiceCommand::OpenQuote:
            return QString::fromUtf8("\xe2\x80\x9c");  // "
        case VoiceCommand::CloseQuote:
            return QString::fromUtf8("\xe2\x80\x9d");  // "
        case VoiceCommand::Bold:
            return "**";
        case VoiceCommand::Italic:
            return "*";
        case VoiceCommand::Heading1:
            return "# ";
        case VoiceCommand::Heading2:
            return "## ";
        case VoiceCommand::Heading3:
            return "### ";
        case VoiceCommand::BulletList:
            return "- ";
        case VoiceCommand::NumberedList:
            return "1. ";
        case VoiceCommand::Code:
            return "`";
        case VoiceCommand::Link:
            return "[](";
        default:
            return QString();
    }
}

// 函数说明：响应 VoiceInput 收到的信号或异步回调，并更新界面状态。
void VoiceInput::onAudioDataReady()
{
    if (!m_audioDevice) return;

    QByteArray data = m_audioDevice->readAll();
    if (data.isEmpty()) return;

    m_audioBuffer.append(data);
    processAudioData(data);
}

// 函数说明：响应 VoiceInput 收到的信号或异步回调，并更新界面状态。
void VoiceInput::onSilenceTimeout()
{
    if (m_state != State::Recording) return;

    if (isSilent() && m_config.enableContinuous) {
        emit silenceDetected();

        // 处理当前缓冲区
        if (!m_audioBuffer.isEmpty()) {
            if (m_effectiveEngine == Engine::CloudAPI) {
                recognizeWithCloudAPI(m_audioBuffer);
            } else if (m_effectiveEngine == Engine::Offline) {
                recognizeWithOffline(m_audioBuffer);
            } else {
                recognizeWithSystem(m_audioBuffer);
            }
            m_audioBuffer.clear();
        }
    }
}

// 函数说明：响应 VoiceInput 收到的信号或异步回调，并更新界面状态。
void VoiceInput::onRecognitionResponse()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        m_lastError = tr("语音识别失败: %1").arg(reply->errorString());
        emit errorOccurred(m_lastError);
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    QJsonObject obj = doc.object();

    // 解析结果（根据具体 API 格式调整）
    RecognitionResult result;

    if (obj.contains("results")) {
        QJsonArray results = obj["results"].toArray();
        if (!results.isEmpty()) {
            QJsonObject firstResult = results.first().toObject();
            QJsonArray alternatives = firstResult["alternatives"].toArray();
            if (!alternatives.isEmpty()) {
                QJsonObject firstAlt = alternatives.first().toObject();
                result.text = firstAlt["transcript"].toString();
                result.confidence = firstAlt["confidence"].toDouble();
            }
            result.isFinal = firstResult["isFinal"].toBool(true);
        }
    } else if (obj.contains("text")) {
        // 简单格式
        result.text = obj["text"].toString();
        result.confidence = obj["confidence"].toDouble(1.0);
        result.isFinal = true;
    }

    if (!result.text.isEmpty()) {
        processText(result.text);

        if (result.isFinal) {
            emit finalResult(result);
        } else {
            emit partialResult(result);
        }
    }
}

// 函数说明：实现 VoiceInput::initAudioInput 的核心逻辑，供当前模块调用。
void VoiceInput::initAudioInput()
{
    QAudioFormat format;
    format.setSampleRate(m_config.sampleRate);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    QAudioDevice inputDevice = QMediaDevices::defaultAudioInput();

    if (!inputDevice.isFormatSupported(format)) {
        // 尝试使用设备支持的格式
        format = inputDevice.preferredFormat();
    }

    m_audioSource = new QAudioSource(inputDevice, format, this);
}

// 函数说明：处理 VoiceInput 的核心业务数据，并输出处理结果。
void VoiceInput::processAudioData(const QByteArray &data)
{
    float volume = calculateVolume(data);
    m_currentVolume = volume;
    emit volumeChanged(volume);
    updateAdaptiveThreshold(volume);

    // 如果检测到声音，重置静音计时器
    if (volume > m_effectiveSilenceThreshold) {
        if (m_silenceTimer->isActive()) {
            m_silenceTimer->start(m_config.silenceTimeout);
        }
    }
}

// 函数说明：刷新 VoiceInput 的内部状态，并同步到相关界面。
void VoiceInput::updateAdaptiveThreshold(float volume)
{
    if (!m_config.enableAdaptiveSilenceThreshold) {
        m_effectiveSilenceThreshold = m_config.silenceThreshold;
        return;
    }

    const float alpha = 0.08f;
    if (m_noiseSampleCount <= 0) {
        m_noiseFloor = volume;
        m_noiseSampleCount = 1;
    } else {
        const float likelyNoiseLimit = m_effectiveSilenceThreshold * 1.2f;
        if (volume <= likelyNoiseLimit) {
            m_noiseFloor = (1.0f - alpha) * m_noiseFloor + alpha * volume;
        } else {
            const float slowAlpha = alpha * 0.2f;
            m_noiseFloor = (1.0f - slowAlpha) * m_noiseFloor + slowAlpha * volume;
        }
        ++m_noiseSampleCount;
    }

    const float rawAdaptive = m_noiseFloor * qMax(1.0f, m_config.adaptiveSensitivity);
    m_effectiveSilenceThreshold = qBound(m_config.minSilenceThreshold,
                                         rawAdaptive,
                                         m_config.maxSilenceThreshold);
}

// 函数说明：实现 VoiceInput::recognizeWithCloudAPI 的核心逻辑，供当前模块调用。
void VoiceInput::recognizeWithCloudAPI(const QByteArray &audioData)
{
    if (m_config.apiUrl.isEmpty() || m_config.apiKey.isEmpty()) {
        m_lastError = tr("未配置云端 API");
        emit errorOccurred(m_lastError);
        return;
    }

    QUrl url(m_config.apiUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_config.apiKey).toUtf8());

    // 构建请求体（根据具体 API 格式调整）
    QJsonObject config;
    config["encoding"] = "LINEAR16";
    config["sampleRateHertz"] = m_config.sampleRate;
    config["languageCode"] = m_config.language;
    config["enableAutomaticPunctuation"] = m_config.enablePunctuation;

    QJsonObject audio;
    audio["content"] = QString::fromLatin1(audioData.toBase64());

    QJsonObject requestBody;
    requestBody["config"] = config;
    requestBody["audio"] = audio;

    QNetworkReply *reply = m_networkManager->post(request,
        QJsonDocument(requestBody).toJson());

    connect(reply, &QNetworkReply::finished,
            this, &VoiceInput::onRecognitionResponse);
}

// 函数说明：实现 VoiceInput::recognizeWithSystem 的核心逻辑，供当前模块调用。
void VoiceInput::recognizeWithSystem(const QByteArray &audioData)
{
    Q_UNUSED(audioData)

    if (!isSystemEngineAvailable()) {
        m_lastError = tr("系统语音引擎在当前平台不可用，请切换云端 API 或离线模型");
        emit errorOccurred(m_lastError);
        return;
    }

    m_lastError = tr("系统语音引擎桥接尚未启用。"
                     "请在构建时开启 CUTEMARKED_ENABLE_SYSTEM_SPEECH，"
                     "或切换云端 API / 离线模型。");
    emit errorOccurred(m_lastError);
}

// 函数说明：写入 VoiceInput 的配置或数据，用于下次启动恢复。
bool VoiceInput::writePcm16MonoWav(const QByteArray &pcm,
                                   const QString &filePath,
                                   int sampleRate,
                                   QString *error) const
{
    QFile outFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = tr("无法写入临时音频文件：%1").arg(outFile.errorString());
        }
        return false;
    }

    const quint16 channels = 1;
    const quint16 bitsPerSample = 16;
    const quint32 dataSize = static_cast<quint32>(pcm.size());
    const quint32 byteRate = static_cast<quint32>(sampleRate) * channels * bitsPerSample / 8;
    const quint16 blockAlign = channels * bitsPerSample / 8;
    const quint32 fmtChunkSize = 16;
    const quint32 riffSize = 4 + (8 + fmtChunkSize) + (8 + dataSize);

    QDataStream stream(&outFile);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream.writeRawData("RIFF", 4);
    stream << riffSize;
    stream.writeRawData("WAVE", 4);

    stream.writeRawData("fmt ", 4);
    stream << fmtChunkSize;
    stream << static_cast<quint16>(1); // PCM
    stream << channels;
    stream << static_cast<quint32>(sampleRate);
    stream << byteRate;
    stream << blockAlign;
    stream << bitsPerSample;

    stream.writeRawData("data", 4);
    stream << dataSize;
    stream.writeRawData(pcm.constData(), pcm.size());

    return true;
}

// 函数说明：实现 VoiceInput::defaultOfflineDecoderForType 的核心逻辑，供当前模块调用。
QString VoiceInput::defaultOfflineDecoderForType(const QString &modelType) const
{
    const QString type = modelType.trimmed().toLower();
    if (type == QStringLiteral("whisper.cpp")) {
        const QString whisperCli = QStandardPaths::findExecutable(QStringLiteral("whisper-cli"));
        if (!whisperCli.isEmpty()) {
            return whisperCli;
        }
        const QString legacy = QStandardPaths::findExecutable(QStringLiteral("main"));
        if (!legacy.isEmpty()) {
            return legacy;
        }
    } else if (type == QStringLiteral("vosk")) {
        const QString voskCli = QStandardPaths::findExecutable(QStringLiteral("vosk-transcriber"));
        if (!voskCli.isEmpty()) {
            return voskCli;
        }
    }
    return QString();
}

// 函数说明：实现 VoiceInput::runOfflineDecoder 的核心逻辑，供当前模块调用。
QString VoiceInput::runOfflineDecoder(const QString &wavFilePath, QString *error) const
{
    QString program = m_config.offlineDecoderExecutable.trimmed();
    if (program.isEmpty()) {
        program = defaultOfflineDecoderForType(m_config.offlineModelType);
    }

    if (program.isEmpty()) {
        if (error) {
            *error = tr("未找到离线解码器。请在设置中配置 offlineDecoderExecutable");
        }
        return QString();
    }

    QStringList args = m_config.offlineDecoderArguments;
    if (args.isEmpty()) {
        const QString type = m_config.offlineModelType.trimmed().toLower();
        if (type == QStringLiteral("whisper.cpp")) {
            args << QStringLiteral("-m") << QStringLiteral("{model}")
                 << QStringLiteral("-f") << QStringLiteral("{audio}")
                 << QStringLiteral("-l") << QStringLiteral("{lang}");
        } else if (type == QStringLiteral("vosk")) {
            args << QStringLiteral("--model") << QStringLiteral("{model}")
                 << QStringLiteral("--input") << QStringLiteral("{audio}")
                 << QStringLiteral("--lang") << QStringLiteral("{lang}");
        }
    }

    for (QString &arg : args) {
        arg.replace(QStringLiteral("{model}"), m_config.offlineModelPath);
        arg.replace(QStringLiteral("{audio}"), wavFilePath);
        arg.replace(QStringLiteral("{lang}"), m_config.language);
    }

    QProcess process;
    process.setProgram(program);
    process.setArguments(args);
    process.start();

    if (!process.waitForStarted(3000)) {
        if (error) {
            *error = tr("无法启动离线解码器：%1").arg(program);
        }
        return QString();
    }

    if (!process.waitForFinished(m_config.offlineDecodeTimeoutMs)) {
        process.kill();
        process.waitForFinished();
        if (error) {
            *error = tr("离线解码超时（%1 ms）").arg(m_config.offlineDecodeTimeoutMs);
        }
        return QString();
    }

    const QString stdOut = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    const QString stdErr = QString::fromUtf8(process.readAllStandardError()).trimmed();

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (error) {
            *error = tr("离线解码器执行失败（exit=%1）：%2")
                         .arg(process.exitCode())
                         .arg(stdErr.isEmpty() ? stdOut : stdErr);
        }
        return QString();
    }

    if (stdOut.isEmpty()) {
        if (error) {
            *error = tr("离线解码器未返回文本输出");
        }
        return QString();
    }

    // 兼容 JSON 输出与纯文本输出
    const QJsonDocument json = QJsonDocument::fromJson(stdOut.toUtf8());
    if (json.isObject()) {
        const QJsonObject obj = json.object();
        const QString text = obj.value(QStringLiteral("text")).toString().trimmed();
        if (!text.isEmpty()) {
            return text;
        }
        const QJsonArray resultArr = obj.value(QStringLiteral("result")).toArray();
        if (!resultArr.isEmpty()) {
            return resultArr.first().toObject().value(QStringLiteral("text")).toString().trimmed();
        }
    }

    return stdOut;
}

// 函数说明：实现 VoiceInput::recognizeWithOffline 的核心逻辑，供当前模块调用。
void VoiceInput::recognizeWithOffline(const QByteArray &audioData)
{
    QString reason;
    if (!validateOfflineModel(&reason)) {
        m_lastError = tr("离线模型配置无效：%1").arg(reason);
        emit errorOccurred(m_lastError);
        return;
    }

    if (audioData.isEmpty()) {
        m_lastError = tr("离线识别输入为空");
        emit errorOccurred(m_lastError);
        return;
    }

    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        m_lastError = tr("无法创建离线识别临时目录");
        emit errorOccurred(m_lastError);
        return;
    }

    const QString wavPath = QDir(tempDir.path()).filePath(QStringLiteral("voice_input.wav"));
    QString writeError;
    if (!writePcm16MonoWav(audioData, wavPath, m_config.sampleRate, &writeError)) {
        m_lastError = writeError;
        emit errorOccurred(m_lastError);
        return;
    }

    QString decodeError;
    const QString text = runOfflineDecoder(wavPath, &decodeError).trimmed();
    if (text.isEmpty()) {
        m_lastError = decodeError.isEmpty() ? tr("离线识别返回空结果") : decodeError;
        emit errorOccurred(m_lastError);
        return;
    }

    RecognitionResult result;
    result.text = text;
    result.confidence = 0.8f;
    result.isFinal = true;

    processText(result.text);
    emit finalResult(result);
}

// 函数说明：实现 VoiceInput::calculateVolume 的核心逻辑，供当前模块调用。
float VoiceInput::calculateVolume(const QByteArray &data)
{
    if (data.isEmpty()) return 0.0f;

    const qint16 *samples = reinterpret_cast<const qint16*>(data.constData());
    int sampleCount = data.size() / sizeof(qint16);

    if (sampleCount == 0) return 0.0f;

    // 计算 RMS
    double sum = 0.0;
    for (int i = 0; i < sampleCount; ++i) {
        double sample = samples[i] / 32768.0;  // 归一化
        sum += sample * sample;
    }

    double rms = std::sqrt(sum / sampleCount);
    return static_cast<float>(rms);
}

// 函数说明：处理 VoiceInput 的核心业务数据，并输出处理结果。
void VoiceInput::processText(const QString &text)
{
    if (!m_config.enableCommands) return;

    VoiceCommand command;
    if (checkForCommand(text, command)) {
        emit commandRecognized(command);
    }
}

// 函数说明：实现 VoiceInput::checkForCommand 的核心逻辑，供当前模块调用。
bool VoiceInput::checkForCommand(const QString &text, VoiceCommand &command)
{
    command = parseCommand(text);
    return command != VoiceCommand::None;
}

// 函数说明：实现 VoiceInput::initCommandMap 的核心逻辑，供当前模块调用。
void VoiceInput::initCommandMap()
{
    if (!s_commandMap.isEmpty()) return;

    // 中文命令
    s_commandMap["换行"] = VoiceCommand::NewLine;
    s_commandMap["新行"] = VoiceCommand::NewLine;
    s_commandMap["下一行"] = VoiceCommand::NewLine;

    s_commandMap["新段落"] = VoiceCommand::NewParagraph;
    s_commandMap["另起一段"] = VoiceCommand::NewParagraph;
    s_commandMap["新段"] = VoiceCommand::NewParagraph;

    s_commandMap["句号"] = VoiceCommand::Period;
    s_commandMap["逗号"] = VoiceCommand::Comma;
    s_commandMap["问号"] = VoiceCommand::QuestionMark;
    s_commandMap["感叹号"] = VoiceCommand::ExclamationMark;
    s_commandMap["冒号"] = VoiceCommand::Colon;
    s_commandMap["分号"] = VoiceCommand::Semicolon;

    s_commandMap["左引号"] = VoiceCommand::OpenQuote;
    s_commandMap["开引号"] = VoiceCommand::OpenQuote;
    s_commandMap["右引号"] = VoiceCommand::CloseQuote;
    s_commandMap["闭引号"] = VoiceCommand::CloseQuote;

    s_commandMap["删除"] = VoiceCommand::Delete;
    s_commandMap["撤销"] = VoiceCommand::Undo;

    s_commandMap["加粗"] = VoiceCommand::Bold;
    s_commandMap["粗体"] = VoiceCommand::Bold;
    s_commandMap["斜体"] = VoiceCommand::Italic;

    s_commandMap["一级标题"] = VoiceCommand::Heading1;
    s_commandMap["大标题"] = VoiceCommand::Heading1;
    s_commandMap["二级标题"] = VoiceCommand::Heading2;
    s_commandMap["三级标题"] = VoiceCommand::Heading3;

    s_commandMap["列表"] = VoiceCommand::BulletList;
    s_commandMap["无序列表"] = VoiceCommand::BulletList;
    s_commandMap["有序列表"] = VoiceCommand::NumberedList;
    s_commandMap["编号列表"] = VoiceCommand::NumberedList;

    s_commandMap["代码"] = VoiceCommand::Code;
    s_commandMap["链接"] = VoiceCommand::Link;

    s_commandMap["停止"] = VoiceCommand::Stop;
    s_commandMap["停止录音"] = VoiceCommand::Stop;
    s_commandMap["结束"] = VoiceCommand::Stop;

    // 英文命令
    s_commandMap["new line"] = VoiceCommand::NewLine;
    s_commandMap["newline"] = VoiceCommand::NewLine;

    s_commandMap["new paragraph"] = VoiceCommand::NewParagraph;
    s_commandMap["paragraph"] = VoiceCommand::NewParagraph;

    s_commandMap["period"] = VoiceCommand::Period;
    s_commandMap["full stop"] = VoiceCommand::Period;
    s_commandMap["comma"] = VoiceCommand::Comma;
    s_commandMap["question mark"] = VoiceCommand::QuestionMark;
    s_commandMap["exclamation mark"] = VoiceCommand::ExclamationMark;
    s_commandMap["colon"] = VoiceCommand::Colon;
    s_commandMap["semicolon"] = VoiceCommand::Semicolon;

    s_commandMap["open quote"] = VoiceCommand::OpenQuote;
    s_commandMap["close quote"] = VoiceCommand::CloseQuote;
    s_commandMap["quote"] = VoiceCommand::OpenQuote;
    s_commandMap["end quote"] = VoiceCommand::CloseQuote;

    s_commandMap["delete"] = VoiceCommand::Delete;
    s_commandMap["backspace"] = VoiceCommand::Delete;
    s_commandMap["undo"] = VoiceCommand::Undo;

    s_commandMap["bold"] = VoiceCommand::Bold;
    s_commandMap["italic"] = VoiceCommand::Italic;

    s_commandMap["heading one"] = VoiceCommand::Heading1;
    s_commandMap["heading 1"] = VoiceCommand::Heading1;
    s_commandMap["heading two"] = VoiceCommand::Heading2;
    s_commandMap["heading 2"] = VoiceCommand::Heading2;
    s_commandMap["heading three"] = VoiceCommand::Heading3;
    s_commandMap["heading 3"] = VoiceCommand::Heading3;

    s_commandMap["bullet list"] = VoiceCommand::BulletList;
    s_commandMap["numbered list"] = VoiceCommand::NumberedList;

    s_commandMap["code"] = VoiceCommand::Code;
    s_commandMap["link"] = VoiceCommand::Link;

    s_commandMap["stop"] = VoiceCommand::Stop;
    s_commandMap["stop recording"] = VoiceCommand::Stop;
}

