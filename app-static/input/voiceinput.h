// 文件说明：app-static\input\voiceinput.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef VOICEINPUT_H
#define VOICEINPUT_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtMultimedia/QAudioSource>
#include <QtMultimedia/QAudioFormat>
#include <QBuffer>
#include <QTimer>
#include <QNetworkAccessManager>

/**
 * @brief 语音输入模块
 *
 * 功能：
 * - 实时语音录制
 * - 语音转文字（支持多种引擎）
 * - 语音命令识别
 * - 多语言支持
 *
 * 支持的识别引擎：
 * - 系统原生（macOS Dictation, Windows SAPI）
 * - 在线 API（需配置）
 */
class VoiceInput : public QObject
{
    Q_OBJECT

public:
    // 识别引擎
    enum class Engine {
        System,         // 系统原生
        CloudAPI,       // 云端 API
        Offline         // 离线模型
    };
    Q_ENUM(Engine)

    // 识别状态
    enum class State {
        Idle,           // 空闲
        Recording,      // 录音中
        Processing,     // 处理中
        Paused          // 暂停
    };
    Q_ENUM(State)

    // 语音命令
    enum class VoiceCommand {
        None,
        NewLine,        // 换行
        NewParagraph,   // 新段落
        Period,         // 句号
        Comma,          // 逗号
        QuestionMark,   // 问号
        ExclamationMark,// 感叹号
        Colon,          // 冒号
        Semicolon,      // 分号
        OpenQuote,      // 开引号
        CloseQuote,     // 闭引号
        Delete,         // 删除
        Undo,           // 撤销
        Bold,           // 加粗
        Italic,         // 斜体
        Heading1,       // 一级标题
        Heading2,       // 二级标题
        Heading3,       // 三级标题
        BulletList,     // 无序列表
        NumberedList,   // 有序列表
        Code,           // 代码
        Link,           // 链接
        Stop            // 停止录音
    };
    Q_ENUM(VoiceCommand)

    // 配置
    struct Config {
        Engine engine;
        QString language;           // 识别语言 (zh-CN, en-US, etc.)
        QString apiKey;             // API 密钥（如需）
        QString apiUrl;             // API URL（如需）
        bool enableCommands;        // 启用语音命令
        bool enablePunctuation;     // 自动标点
        bool enableContinuous;      // 连续识别
        int sampleRate;             // 采样率
        int silenceTimeout;         // 静音超时（毫秒）
        float silenceThreshold;     // 静音阈值
        bool enableAdaptiveSilenceThreshold; // 是否启用自适应静音阈值
        float adaptiveSensitivity;  // 自适应灵敏度系数（噪声底线乘数）
        float minSilenceThreshold;  // 自适应阈值下限
        float maxSilenceThreshold;  // 自适应阈值上限
        QString offlineModelType;   // 离线模型类型：whisper.cpp / vosk
        QString offlineModelPath;   // 离线模型路径（文件或目录）
        QString offlineDecoderExecutable; // 外部离线解码器可执行文件（可选）
        QStringList offlineDecoderArguments; // 外部解码器参数（支持占位符）
        int offlineDecodeTimeoutMs; // 离线解码超时时间（毫秒）

        Config()
            : engine(Engine::System)
            , language("zh-CN")
            , enableCommands(true)
            , enablePunctuation(true)
            , enableContinuous(true)
            , sampleRate(16000)
            , silenceTimeout(2000)
            , silenceThreshold(0.02f)
            , enableAdaptiveSilenceThreshold(true)
            , adaptiveSensitivity(2.4f)
            , minSilenceThreshold(0.008f)
            , maxSilenceThreshold(0.15f)
            , offlineModelType(QStringLiteral("whisper.cpp"))
            , offlineDecodeTimeoutMs(20000)
        {}
    };

    // 识别结果
    struct RecognitionResult {
        QString text;               // 识别文本
        float confidence;           // 置信度 (0-1)
        bool isFinal;               // 是否最终结果
        VoiceCommand command;       // 识别到的命令
        int startTime;              // 开始时间（毫秒）
        int endTime;                // 结束时间（毫秒）

        RecognitionResult()
            : confidence(0.0f)
            , isFinal(false)
            , command(VoiceCommand::None)
            , startTime(0)
            , endTime(0)
        {}
    };

    explicit VoiceInput(QObject *parent = nullptr);
    ~VoiceInput();

    // 配置
    void setConfig(const Config &config);
    Config config() const { return m_config; }
    void saveSettings() const;
    void loadSettings();

    // 检查可用性
    static bool isAvailable();
    static QStringList availableLanguages();
    static QStringList availableEngines();
    static bool isSystemEngineAvailable();
    static QStringList supportedOfflineModelTypes();
    static QString offlineModelFormatSpecification();

    // 录音控制
    bool startRecording();
    void stopRecording();
    void pauseRecording();
    void resumeRecording();
    bool isRecording() const { return m_state == State::Recording; }
    State state() const { return m_state; }

    // 获取音频数据
    QByteArray audioData() const { return m_audioBuffer; }
    int recordingDuration() const;  // 毫秒

    // 音量监测
    float currentVolume() const { return m_currentVolume; }
    float effectiveSilenceThreshold() const { return m_effectiveSilenceThreshold; }
    bool isSilent() const { return m_currentVolume < m_effectiveSilenceThreshold; }

    // 语音命令
    static VoiceCommand parseCommand(const QString &text);
    static QString commandToMarkdown(VoiceCommand command);
    bool validateOfflineModel(QString *reason = nullptr) const;

    // 错误信息
    QString lastError() const { return m_lastError; }

signals:
    void recordingStarted();
    void recordingStopped();
    void recordingPaused();
    void recordingResumed();
    void stateChanged(VoiceInput::State state);

    void volumeChanged(float volume);
    void silenceDetected();

    void partialResult(const RecognitionResult &result);
    void finalResult(const RecognitionResult &result);
    void commandRecognized(VoiceInput::VoiceCommand command);
    void engineFallbackApplied(VoiceInput::Engine requested,
                               VoiceInput::Engine effective,
                               const QString &reason);

    void errorOccurred(const QString &error);

private slots:
    void onAudioDataReady();
    void onSilenceTimeout();
    void onRecognitionResponse();

private:
    void initAudioInput();
    void processAudioData(const QByteArray &data);
    void recognizeWithCloudAPI(const QByteArray &audioData);
    void recognizeWithSystem(const QByteArray &audioData);
    void recognizeWithOffline(const QByteArray &audioData);
    float calculateVolume(const QByteArray &data);
    void updateAdaptiveThreshold(float volume);
    Engine resolveEffectiveEngine(QString *reason = nullptr) const;
    bool writePcm16MonoWav(const QByteArray &pcm,
                           const QString &filePath,
                           int sampleRate,
                           QString *error = nullptr) const;
    QString runOfflineDecoder(const QString &wavFilePath, QString *error = nullptr) const;
    QString defaultOfflineDecoderForType(const QString &modelType) const;

    // 命令解析
    void processText(const QString &text);
    bool checkForCommand(const QString &text, VoiceCommand &command);

    Config m_config;
    State m_state;

    QAudioSource *m_audioSource;
    QIODevice *m_audioDevice;
    QByteArray m_audioBuffer;
    QBuffer *m_buffer;

    QNetworkAccessManager *m_networkManager;
    QTimer *m_silenceTimer;

    float m_currentVolume;
    float m_noiseFloor;
    float m_effectiveSilenceThreshold;
    int m_noiseSampleCount;
    qint64 m_recordingStartTime;
    QString m_lastError;
    Engine m_effectiveEngine;

    // 命令映射
    static QMap<QString, VoiceCommand> s_commandMap;
    static void initCommandMap();
};

#endif // VOICEINPUT_H

