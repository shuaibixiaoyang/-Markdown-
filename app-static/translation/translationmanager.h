// 文件说明：app-static\translation\translationmanager.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef TRANSLATIONMANAGER_H
#define TRANSLATIONMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QNetworkAccessManager>
#include <QCache>
#include <QList>

/**
 * @brief 翻译管理器
 *
 * 功能：
 * - 多语言翻译
 * - 支持多种翻译引擎
 * - 翻译缓存
 * - 批量翻译
 * - 语言检测
 *
 * 支持的翻译引擎：
 * - 腾讯翻译君 API
 * - 百度翻译 API
 * - 有道翻译 API
 * - DeepL API
 * - Google Translate API
 * - 离线词典（基础词汇）
 */
class TranslationManager : public QObject
{
    Q_OBJECT

public:
    // 翻译引擎
    enum class Engine {
        Tencent,        // 腾讯翻译君
        Baidu,          // 百度翻译
        Youdao,         // 有道翻译
        DeepL,          // DeepL
        Google,         // Google Translate
        Offline         // 离线词典
    };
    Q_ENUM(Engine)

    // 语言代码
    struct Language {
        QString code;       // 语言代码 (zh, en, ja, etc.)
        QString name;       // 语言名称 (中文、English、日本語)
        QString nativeName; // 本地名称

        Language() {}
        Language(const QString &c, const QString &n, const QString &nn = QString())
            : code(c), name(n), nativeName(nn.isEmpty() ? n : nn) {}
    };

    // 翻译结果
    struct TranslationResult {
        QString sourceText;         // 原文
        QString translatedText;     // 译文
        QString sourceLanguage;     // 源语言
        QString targetLanguage;     // 目标语言
        float confidence;           // 置信度
        QString pronunciation;      // 发音/拼音
        QStringList alternatives;   // 备选翻译
        QString engine;             // 使用的引擎

        TranslationResult() : confidence(0.0f) {}
    };

    // 配置
    struct Config {
        Engine engine;
        QString apiKey;
        QString apiSecret;          // 部分 API 需要
        QString appId;              // 部分 API 需要
        QString defaultSourceLang;
        QString defaultTargetLang;
        bool enableCache;
        int cacheSize;              // 缓存条目数
        int maxTextLength;          // 最大文本长度
        bool enableRateLimit;       // 启用速率限制
        int maxRequestsPerMinute;   // 每分钟最大请求次数
        int minRequestIntervalMs;   // 最小请求间隔（毫秒）
        QString offlineDictionaryPath; // 离线词典路径

        Config()
            : engine(Engine::Baidu)
            , defaultSourceLang("auto")
            , defaultTargetLang("zh")
            , enableCache(true)
            , cacheSize(1000)
            , maxTextLength(5000)
            , enableRateLimit(true)
            , maxRequestsPerMinute(60)
            , minRequestIntervalMs(200)
        {}
    };

    explicit TranslationManager(QObject *parent = nullptr);
    ~TranslationManager();

    // 配置
    void setConfig(const Config &config);
    Config config() const { return m_config; }
    void saveSettings() const;
    void loadSettings();

    // 语言支持
    static QVector<Language> supportedLanguages();
    static QVector<Language> supportedLanguages(Engine engine);
    static QString languageCode(const QString &name);
    static QString languageName(const QString &code);
    static bool isLanguageSupported(const QString &code, Engine engine = Engine::Baidu);

    // 翻译
    void translate(const QString &text,
                   const QString &targetLang = QString(),
                   const QString &sourceLang = QString());

    void translateBatch(const QStringList &texts,
                        const QString &targetLang = QString(),
                        const QString &sourceLang = QString());

    // 同步翻译（阻塞）
    TranslationResult translateSync(const QString &text,
                                    const QString &targetLang = QString(),
                                    const QString &sourceLang = QString());

    // 语言检测
    void detectLanguage(const QString &text);
    QString detectLanguageSync(const QString &text);

    // 缓存管理
    void clearCache();
    int cacheSize() const;
    bool isCached(const QString &text, const QString &targetLang) const;

    // 离线词典
    bool loadOfflineDictionary(const QString &filePath);
    QString lookupWord(const QString &word, const QString &targetLang);
    static QString offlineDictionaryFormatSpecification();

    // 错误信息
    QString lastError() const { return m_lastError; }

signals:
    void translationCompleted(const TranslationResult &result);
    void batchTranslationCompleted(const QVector<TranslationResult> &results);
    void languageDetected(const QString &text, const QString &language);
    void errorOccurred(const QString &error);
    void progressChanged(int current, int total);

private slots:
    void onTranslationResponse();
    void onBatchResponse();
    void onDetectResponse();

private:
    // 引擎特定的翻译实现
    void translateWithTencent(const QString &text, const QString &source, const QString &target);
    void translateWithBaidu(const QString &text, const QString &source, const QString &target);
    void translateWithYoudao(const QString &text, const QString &source, const QString &target);
    void translateWithDeepL(const QString &text, const QString &source, const QString &target);
    void translateWithGoogle(const QString &text, const QString &source, const QString &target);
    TranslationResult translateOffline(const QString &text, const QString &source, const QString &target);

    // 签名生成
    QString generateBaiduSign(const QString &text, const QString &salt);
    QString generateYoudaoSign(const QString &text, const QString &salt, const QString &curtime);
    QString generateTencentSign(const QMap<QString, QString> &params);

    // 语言代码转换
    QString toEngineCode(const QString &code, Engine engine);
    QString fromEngineCode(const QString &code, Engine engine);

    // 缓存键生成
    QString cacheKey(const QString &text, const QString &targetLang) const;
    bool tryAcquireRequestQuota(QString *reason = nullptr);

    Config m_config;
    QNetworkAccessManager *m_networkManager;
    QCache<QString, TranslationResult> m_cache;
    QMap<QString, QMap<QString, QString>> m_offlineDict;  // lang -> (word -> translation)
    QString m_lastError;
    QList<qint64> m_requestTimestampsMs;
    qint64 m_lastRequestTimestampMs = 0;

    // 批量翻译状态
    QStringList m_batchTexts;
    QVector<TranslationResult> m_batchResults;
    QString m_batchTargetLang;
    QString m_batchSourceLang;
    int m_batchIndex;

    // 语言映射
    static QVector<Language> s_languages;
    static void initLanguages();
};

#endif // TRANSLATIONMANAGER_H

