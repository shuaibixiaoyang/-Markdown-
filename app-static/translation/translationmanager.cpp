// 文件说明：app-static\translation\translationmanager.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "translationmanager.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QDateTime>
#include <QRandomGenerator>
#include <QEventLoop>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QCoreApplication>
#include <QSysInfo>
#include <QDebug>

QVector<TranslationManager::Language> TranslationManager::s_languages;

namespace {

const QString kEncryptedPrefix = QStringLiteral("enc:v1:");
const QString kOfflineDictionaryFormat = QStringLiteral("cutemarked-offline-dict");
const int kOfflineDictionaryVersion = 1;

QByteArray deriveSecretKey()
{
    QByteArray seed = QSysInfo::machineUniqueId();
    if (seed.isEmpty()) {
        seed = QSysInfo::machineHostName().toUtf8();
    }
    seed += QCoreApplication::organizationName().toUtf8();
    seed += QCoreApplication::applicationName().toUtf8();
    return QCryptographicHash::hash(seed, QCryptographicHash::Sha256);
}

QByteArray streamCipherXor(const QByteArray &input, const QByteArray &key, const QByteArray &nonce)
{
    QByteArray output(input.size(), '\0');
    QByteArray keystream;
    keystream.reserve(input.size());

    quint32 counter = 0;
    while (keystream.size() < input.size()) {
        const QByteArray material = key + nonce + QByteArray::number(counter++);
        keystream += QCryptographicHash::hash(material, QCryptographicHash::Sha256);
    }

    for (int i = 0; i < input.size(); ++i) {
        output[i] = input[i] ^ keystream[i];
    }
    return output;
}

QString encryptSecret(const QString &plainText)
{
    if (plainText.isEmpty()) {
        return QString();
    }

    QByteArray nonce(16, '\0');
    for (int i = 0; i < nonce.size(); ++i) {
        nonce[i] = static_cast<char>(QRandomGenerator::global()->generate() & 0xFF);
    }

    const QByteArray cipher = streamCipherXor(plainText.toUtf8(), deriveSecretKey(), nonce);
    const QByteArray payload = nonce + cipher;
    const QByteArray encoded = payload.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    return kEncryptedPrefix + QString::fromUtf8(encoded);
}

QString decryptSecret(const QString &storedText)
{
    if (storedText.isEmpty()) {
        return QString();
    }

    if (storedText.startsWith(kEncryptedPrefix)) {
        const QByteArray encoded = storedText.mid(kEncryptedPrefix.size()).toUtf8();
        const QByteArray payload = QByteArray::fromBase64(
            encoded, QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
        if (payload.size() <= 16) {
            return QString();
        }

        const QByteArray nonce = payload.left(16);
        const QByteArray cipher = payload.mid(16);
        return QString::fromUtf8(streamCipherXor(cipher, deriveSecretKey(), nonce));
    }

    // 向后兼容：允许读取明文或旧版本 Base64 文本。
    const QByteArray legacy = QByteArray::fromBase64(
        storedText.toUtf8(), QByteArray::AbortOnBase64DecodingErrors);
    if (!legacy.isEmpty()) {
        return QString::fromUtf8(legacy);
    }
    return storedText;
}

} // namespace

// 函数说明：构造 TranslationManager 对象，初始化本模块需要的状态、界面和资源。
TranslationManager::TranslationManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_batchIndex(0)
{
    initLanguages();
    loadSettings();
    m_cache.setMaxCost(m_config.cacheSize);

    if (!m_config.offlineDictionaryPath.isEmpty()) {
        loadOfflineDictionary(m_config.offlineDictionaryPath);
    }
}

// 函数说明：销毁 TranslationManager 对象，释放本模块持有的资源。
TranslationManager::~TranslationManager()
{
    saveSettings();
}

// 函数说明：设置 TranslationManager 的运行参数，并触发必要的界面或数据刷新。
void TranslationManager::setConfig(const Config &config)
{
    m_config = config;
    m_cache.setMaxCost(config.cacheSize);

    if (m_config.maxRequestsPerMinute < 1) {
        m_config.maxRequestsPerMinute = 1;
    }
    if (m_config.minRequestIntervalMs < 0) {
        m_config.minRequestIntervalMs = 0;
    }

    m_requestTimestampsMs.clear();
    m_lastRequestTimestampMs = 0;

    if (m_config.offlineDictionaryPath.isEmpty()) {
        m_offlineDict.clear();
    }

    saveSettings();
}

// 函数说明：保存 TranslationManager 当前状态，保证用户修改可以持久化。
void TranslationManager::saveSettings() const
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("TranslationManager"));
    settings.setValue(QStringLiteral("engine"), static_cast<int>(m_config.engine));
    settings.setValue(QStringLiteral("appId"), encryptSecret(m_config.appId));
    settings.setValue(QStringLiteral("apiKey"), encryptSecret(m_config.apiKey));
    settings.setValue(QStringLiteral("apiSecret"), encryptSecret(m_config.apiSecret));
    settings.setValue(QStringLiteral("defaultSourceLang"), m_config.defaultSourceLang);
    settings.setValue(QStringLiteral("defaultTargetLang"), m_config.defaultTargetLang);
    settings.setValue(QStringLiteral("enableCache"), m_config.enableCache);
    settings.setValue(QStringLiteral("cacheSize"), m_config.cacheSize);
    settings.setValue(QStringLiteral("maxTextLength"), m_config.maxTextLength);
    settings.setValue(QStringLiteral("enableRateLimit"), m_config.enableRateLimit);
    settings.setValue(QStringLiteral("maxRequestsPerMinute"), m_config.maxRequestsPerMinute);
    settings.setValue(QStringLiteral("minRequestIntervalMs"), m_config.minRequestIntervalMs);
    settings.setValue(QStringLiteral("offlineDictionaryPath"), m_config.offlineDictionaryPath);
    settings.endGroup();
}

// 函数说明：加载 TranslationManager 需要的数据、配置或外部资源。
void TranslationManager::loadSettings()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("TranslationManager"));

    m_config.engine = static_cast<Engine>(
        settings.value(QStringLiteral("engine"), static_cast<int>(m_config.engine)).toInt());
    m_config.appId = decryptSecret(settings.value(QStringLiteral("appId")).toString());
    m_config.apiKey = decryptSecret(settings.value(QStringLiteral("apiKey")).toString());
    m_config.apiSecret = decryptSecret(settings.value(QStringLiteral("apiSecret")).toString());
    m_config.defaultSourceLang = settings.value(
        QStringLiteral("defaultSourceLang"), m_config.defaultSourceLang).toString();
    m_config.defaultTargetLang = settings.value(
        QStringLiteral("defaultTargetLang"), m_config.defaultTargetLang).toString();
    m_config.enableCache = settings.value(QStringLiteral("enableCache"), m_config.enableCache).toBool();
    m_config.cacheSize = settings.value(QStringLiteral("cacheSize"), m_config.cacheSize).toInt();
    m_config.maxTextLength = settings.value(
        QStringLiteral("maxTextLength"), m_config.maxTextLength).toInt();
    m_config.enableRateLimit = settings.value(
        QStringLiteral("enableRateLimit"), m_config.enableRateLimit).toBool();
    m_config.maxRequestsPerMinute = settings.value(
        QStringLiteral("maxRequestsPerMinute"), m_config.maxRequestsPerMinute).toInt();
    m_config.minRequestIntervalMs = settings.value(
        QStringLiteral("minRequestIntervalMs"), m_config.minRequestIntervalMs).toInt();
    m_config.offlineDictionaryPath = settings.value(
        QStringLiteral("offlineDictionaryPath"), m_config.offlineDictionaryPath).toString();
    settings.endGroup();

    if (m_config.maxRequestsPerMinute < 1) {
        m_config.maxRequestsPerMinute = 1;
    }
    if (m_config.minRequestIntervalMs < 0) {
        m_config.minRequestIntervalMs = 0;
    }
}

// 函数说明：实现 TranslationManager::supportedLanguages 的核心逻辑，供当前模块调用。
QVector<TranslationManager::Language> TranslationManager::supportedLanguages()
{
    if (s_languages.isEmpty()) {
        initLanguages();
    }
    return s_languages;
}

// 函数说明：实现 TranslationManager::supportedLanguages 的核心逻辑，供当前模块调用。
QVector<TranslationManager::Language> TranslationManager::supportedLanguages(Engine engine)
{
    Q_UNUSED(engine)
    // 所有引擎支持的基本语言集
    return supportedLanguages();
}

// 函数说明：实现 TranslationManager::languageCode 的核心逻辑，供当前模块调用。
QString TranslationManager::languageCode(const QString &name)
{
    for (const auto &lang : s_languages) {
        if (lang.name == name || lang.nativeName == name) {
            return lang.code;
        }
    }
    return QString();
}

// 函数说明：实现 TranslationManager::languageName 的核心逻辑，供当前模块调用。
QString TranslationManager::languageName(const QString &code)
{
    for (const auto &lang : s_languages) {
        if (lang.code == code) {
            return lang.name;
        }
    }
    return code;
}

// 函数说明：判断 TranslationManager 当前是否满足指定状态。
bool TranslationManager::isLanguageSupported(const QString &code, Engine engine)
{
    Q_UNUSED(engine)
    for (const auto &lang : s_languages) {
        if (lang.code == code) {
            return true;
        }
    }
    return false;
}

// 函数说明：实现 TranslationManager::translate 的核心逻辑，供当前模块调用。
void TranslationManager::translate(const QString &text,
                                   const QString &targetLang,
                                   const QString &sourceLang)
{
    if (text.isEmpty()) {
        m_lastError = tr("翻译文本不能为空");
        emit errorOccurred(m_lastError);
        return;
    }

    if (text.length() > m_config.maxTextLength) {
        m_lastError = tr("文本长度超过限制 (%1 字符)").arg(m_config.maxTextLength);
        emit errorOccurred(m_lastError);
        return;
    }

    QString target = targetLang.isEmpty() ? m_config.defaultTargetLang : targetLang;
    QString source = sourceLang.isEmpty() ? m_config.defaultSourceLang : sourceLang;

    // 检查缓存
    if (m_config.enableCache) {
        QString key = cacheKey(text, target);
        if (m_cache.contains(key)) {
            emit translationCompleted(*m_cache[key]);
            return;
        }
    }

    // 关键配置检查
    switch (m_config.engine) {
    case Engine::Baidu:
    case Engine::Tencent:
        if (m_config.appId.trimmed().isEmpty() || m_config.apiSecret.trimmed().isEmpty()) {
            m_lastError = tr("当前引擎需要 App ID 和密钥");
            emit errorOccurred(m_lastError);
            return;
        }
        break;
    case Engine::Youdao:
        if (m_config.appId.trimmed().isEmpty() ||
            m_config.apiKey.trimmed().isEmpty() ||
            m_config.apiSecret.trimmed().isEmpty()) {
            m_lastError = tr("有道翻译需要填写「应用ID」「APIkey」和「应用秘钥」三个字段");
            emit errorOccurred(m_lastError);
            return;
        }
        break;
    case Engine::DeepL:
    case Engine::Google:
        if (m_config.apiKey.trimmed().isEmpty()) {
            m_lastError = tr("当前引擎需要 API Key");
            emit errorOccurred(m_lastError);
            return;
        }
        break;
    case Engine::Offline:
        break;
    }

    if (m_config.engine != Engine::Offline) {
        QString quotaReason;
        if (!tryAcquireRequestQuota(&quotaReason)) {
            m_lastError = quotaReason;
            emit errorOccurred(m_lastError);
            return;
        }
    }

    // 根据引擎选择翻译方法
    switch (m_config.engine) {
        case Engine::Tencent:
            translateWithTencent(text, source, target);
            break;
        case Engine::Baidu:
            translateWithBaidu(text, source, target);
            break;
        case Engine::Youdao:
            translateWithYoudao(text, source, target);
            break;
        case Engine::DeepL:
            translateWithDeepL(text, source, target);
            break;
        case Engine::Google:
            translateWithGoogle(text, source, target);
            break;
        case Engine::Offline:
        {
            TranslationResult result = translateOffline(text, source, target);
            emit translationCompleted(result);
            break;
        }
    }
}

// 函数说明：实现 TranslationManager::translateBatch 的核心逻辑，供当前模块调用。
void TranslationManager::translateBatch(const QStringList &texts,
                                        const QString &targetLang,
                                        const QString &sourceLang)
{
    if (texts.isEmpty()) return;

    m_batchTexts = texts;
    m_batchResults.clear();
    m_batchTargetLang = targetLang.isEmpty() ? m_config.defaultTargetLang : targetLang;
    m_batchSourceLang = sourceLang.isEmpty() ? m_config.defaultSourceLang : sourceLang;
    m_batchIndex = 0;

    // 开始翻译第一个
    translate(m_batchTexts[0], m_batchTargetLang, m_batchSourceLang);
}

// 函数说明：实现 TranslationManager::translateSync 的核心逻辑，供当前模块调用。
TranslationManager::TranslationResult TranslationManager::translateSync(
    const QString &text, const QString &targetLang, const QString &sourceLang)
{
    TranslationResult result;
    QEventLoop loop;

    connect(this, &TranslationManager::translationCompleted, &loop,
            [&result, &loop](const TranslationResult &r) {
                result = r;
                loop.quit();
            });

    connect(this, &TranslationManager::errorOccurred, &loop,
            [&loop](const QString &) {
                loop.quit();
            });

    // 设置超时
    QTimer::singleShot(30000, &loop, &QEventLoop::quit);

    translate(text, targetLang, sourceLang);
    loop.exec();

    return result;
}

// 函数说明：实现 TranslationManager::detectLanguage 的核心逻辑，供当前模块调用。
void TranslationManager::detectLanguage(const QString &text)
{
    // 使用百度语言检测 API
    if (text.isEmpty()) return;

    if (m_config.appId.trimmed().isEmpty() || m_config.apiSecret.trimmed().isEmpty()) {
        m_lastError = tr("语言检测需要 App ID 和密钥");
        emit errorOccurred(m_lastError);
        return;
    }

    QString quotaReason;
    if (!tryAcquireRequestQuota(&quotaReason)) {
        m_lastError = quotaReason;
        emit errorOccurred(m_lastError);
        return;
    }

    QString url = "https://fanyi-api.baidu.com/api/trans/vip/language";

    QUrlQuery query;
    query.addQueryItem("q", text.left(100));  // 只取前100字符
    query.addQueryItem("appid", m_config.appId);

    QString salt = QString::number(QRandomGenerator::global()->generate());
    QString sign = generateBaiduSign(text.left(100), salt);
    query.addQueryItem("salt", salt);
    query.addQueryItem("sign", sign);

    QNetworkRequest request{QUrl{url + "?" + query.toString(QUrl::FullyEncoded)}};
    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished,
            this, &TranslationManager::onDetectResponse);
}

// 函数说明：实现 TranslationManager::detectLanguageSync 的核心逻辑，供当前模块调用。
QString TranslationManager::detectLanguageSync(const QString &text)
{
    QString detectedLang;
    QEventLoop loop;

    connect(this, &TranslationManager::languageDetected, &loop,
            [&detectedLang, &loop](const QString &, const QString &lang) {
                detectedLang = lang;
                loop.quit();
            });

    QTimer::singleShot(10000, &loop, &QEventLoop::quit);

    detectLanguage(text);
    loop.exec();

    return detectedLang;
}

// 函数说明：清空 TranslationManager 保存的临时状态或缓存数据。
void TranslationManager::clearCache()
{
    m_cache.clear();
}

// 函数说明：实现 TranslationManager::cacheSize 的核心逻辑，供当前模块调用。
int TranslationManager::cacheSize() const
{
    return m_cache.size();
}

// 函数说明：判断 TranslationManager 当前是否满足指定状态。
bool TranslationManager::isCached(const QString &text, const QString &targetLang) const
{
    return m_cache.contains(cacheKey(text, targetLang));
}

// 函数说明：加载 TranslationManager 需要的数据、配置或外部资源。
bool TranslationManager::loadOfflineDictionary(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = tr("无法加载词典文件: %1").arg(filePath);
        return false;
    }

    const QByteArray rawData = file.readAll();
    file.close();

    if (rawData.trimmed().isEmpty()) {
        m_lastError = tr("离线词典文件为空: %1").arg(filePath);
        return false;
    }

    QMap<QString, QMap<QString, QString>> parsedDict;

    auto insertEntry = [&parsedDict](const QString &sourceLang,
                                     const QString &targetLang,
                                     const QString &term,
                                     const QString &translation) {
        const QString src = sourceLang.trimmed().toLower();
        const QString tgt = targetLang.trimmed().toLower();
        const QString key = src + "_" + tgt;
        parsedDict[key][term.trimmed().toLower()] = translation.trimmed();
    };

    auto parseAsJsonFormat = [&]() -> bool {
        QJsonParseError jsonError;
        const QJsonDocument doc = QJsonDocument::fromJson(rawData, &jsonError);
        if (jsonError.error != QJsonParseError::NoError || !doc.isObject()) {
            return false;
        }

        const QJsonObject root = doc.object();
        const QString format = root.value(QStringLiteral("format")).toString().trimmed();
        const int version = root.value(QStringLiteral("version")).toInt(-1);
        if (format != kOfflineDictionaryFormat || version != kOfflineDictionaryVersion) {
            m_lastError = tr("离线词典 JSON 格式不匹配，期望 format=%1 version=%2")
                              .arg(kOfflineDictionaryFormat)
                              .arg(kOfflineDictionaryVersion);
            return false;
        }

        const QJsonValue entriesValue = root.value(QStringLiteral("entries"));
        if (!entriesValue.isArray()) {
            m_lastError = tr("离线词典 JSON 缺少 entries 数组");
            return false;
        }

        const QJsonArray entries = entriesValue.toArray();
        int loadedCount = 0;

        for (int i = 0; i < entries.size(); ++i) {
            if (!entries.at(i).isObject()) {
                m_lastError = tr("离线词典 JSON 第 %1 项不是对象").arg(i + 1);
                return false;
            }

            const QJsonObject obj = entries.at(i).toObject();
            const QString sourceLang = obj.value(QStringLiteral("source")).toString();
            const QString targetLang = obj.value(QStringLiteral("target")).toString();
            const QString term = obj.value(QStringLiteral("term")).toString();
            const QString translation = obj.value(QStringLiteral("translation")).toString();

            if (sourceLang.trimmed().isEmpty() ||
                targetLang.trimmed().isEmpty() ||
                term.trimmed().isEmpty() ||
                translation.trimmed().isEmpty()) {
                m_lastError = tr("离线词典 JSON 第 %1 项缺少必填字段(source/target/term/translation)")
                                  .arg(i + 1);
                return false;
            }

            insertEntry(sourceLang, targetLang, term, translation);
            ++loadedCount;
        }

        if (loadedCount == 0) {
            m_lastError = tr("离线词典 JSON entries 为空");
            return false;
        }

        return true;
    };

    auto parseAsTsvFormat = [&]() -> bool {
        QTextStream in(rawData);
        in.setEncoding(QStringConverter::Utf8);

        int lineNo = 0;
        int loadedCount = 0;

        while (!in.atEnd()) {
            const QString rawLine = in.readLine();
            ++lineNo;

            const QString trimmed = rawLine.trimmed();
            if (trimmed.isEmpty() || trimmed.startsWith('#')) {
                continue;
            }

            const QStringList parts = rawLine.split('\t');
            if (parts.size() < 4) {
                m_lastError = tr("离线词典 TSV 格式错误（第 %1 行）：至少需要 4 列(source\\ttarget\\tterm\\ttranslation)")
                                  .arg(lineNo);
                return false;
            }

            const QString sourceLang = parts.at(0);
            const QString targetLang = parts.at(1);
            const QString term = parts.at(2);
            const QString translation = parts.mid(3).join(QStringLiteral("\t"));

            if (sourceLang.trimmed().isEmpty() ||
                targetLang.trimmed().isEmpty() ||
                term.trimmed().isEmpty() ||
                translation.trimmed().isEmpty()) {
                m_lastError = tr("离线词典 TSV 第 %1 行存在空字段").arg(lineNo);
                return false;
            }

            insertEntry(sourceLang, targetLang, term, translation);
            ++loadedCount;
        }

        if (loadedCount == 0) {
            m_lastError = tr("离线词典 TSV 没有可用词条");
            return false;
        }
        return true;
    };

    bool ok = parseAsJsonFormat();
    if (!ok) {
        const QString jsonError = m_lastError;
        ok = parseAsTsvFormat();
        if (!ok && !jsonError.isEmpty()) {
            m_lastError = tr("%1；并且 TSV 回退解析失败: %2")
                              .arg(jsonError, m_lastError);
        }
    }

    if (!ok) {
        return false;
    }

    m_offlineDict = parsedDict;
    m_config.offlineDictionaryPath = filePath;
    saveSettings();
    return true;
}

// 函数说明：实现 TranslationManager::lookupWord 的核心逻辑，供当前模块调用。
QString TranslationManager::lookupWord(const QString &word, const QString &targetLang)
{
    QString key = "auto_" + targetLang;

    // 尝试多种语言组合
    QStringList tryKeys;
    tryKeys << "en_" + targetLang
            << "zh_" + targetLang
            << "auto_" + targetLang;

    QString lowerWord = word.toLower();

    for (const QString &k : tryKeys) {
        if (m_offlineDict.contains(k) && m_offlineDict[k].contains(lowerWord)) {
            return m_offlineDict[k][lowerWord];
        }
    }

    return QString();
}

// 函数说明：实现 TranslationManager::offlineDictionaryFormatSpecification 的核心逻辑，供当前模块调用。
QString TranslationManager::offlineDictionaryFormatSpecification()
{
    return QStringLiteral(
        "JSON v1:\n"
        "{\n"
        "  \"format\": \"cutemarked-offline-dict\",\n"
        "  \"version\": 1,\n"
        "  \"entries\": [\n"
        "    {\"source\": \"en\", \"target\": \"zh\", \"term\": \"hello\", \"translation\": \"你好\"}\n"
        "  ]\n"
        "}\n\n"
        "TSV 兼容格式（每行四列）:\n"
        "source_lang<TAB>target_lang<TAB>term<TAB>translation\n"
        "例如: en\\tzh\\thello\\t你好");
}

// 函数说明：响应 TranslationManager 收到的信号或异步回调，并更新界面状态。
void TranslationManager::onTranslationResponse()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    reply->deleteLater();

    TranslationResult result;

    if (reply->error() != QNetworkReply::NoError) {
        m_lastError = tr("网络请求失败: %1").arg(reply->errorString());
        emit errorOccurred(m_lastError);
        return;
    }

    QByteArray data = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        m_lastError = tr("API 响应不是有效的 JSON: %1\n原始响应: %2")
                          .arg(parseError.errorString(),
                               QString::fromUtf8(data.left(500)));
        emit errorOccurred(m_lastError);
        return;
    }

    QJsonObject obj = doc.object();

    // 解析响应（根据引擎不同格式不同）
    QString engineName = reply->property("engine").toString();

    if (engineName == "baidu") {
        // 百度翻译响应 — error_code 可能是字符串或数字
        if (obj.contains("error_code")) {
            QString errCode = obj["error_code"].isString()
                ? obj["error_code"].toString()
                : QString::number(obj["error_code"].toInt());
            if (errCode != "0" && errCode != "52000") {
                m_lastError = tr("百度翻译错误 (错误码 %1): %2")
                                  .arg(errCode, obj["error_msg"].toString());
                emit errorOccurred(m_lastError);
                return;
            }
        }

        result.sourceLanguage = obj["from"].toString();
        result.targetLanguage = obj["to"].toString();

        QJsonArray results = obj["trans_result"].toArray();
        if (!results.isEmpty()) {
            QJsonObject first = results.first().toObject();
            result.sourceText = first["src"].toString();
            result.translatedText = first["dst"].toString();

            // 合并多段翻译
            for (int i = 1; i < results.size(); ++i) {
                result.sourceText += "\n" + results[i].toObject()["src"].toString();
                result.translatedText += "\n" + results[i].toObject()["dst"].toString();
            }
        }

        result.engine = "百度翻译";
        result.confidence = 1.0f;

    } else if (engineName == "youdao") {
        // 有道翻译响应 — errorCode 可能是字符串 "0" 或数字 0
        QString errCodeStr = obj["errorCode"].isString()
            ? obj["errorCode"].toString()
            : QString::number(obj["errorCode"].toInt());
        if (errCodeStr != "0") {
            m_lastError = tr("有道翻译错误 (错误码 %1)").arg(errCodeStr);
            emit errorOccurred(m_lastError);
            return;
        }

        result.sourceText = obj["query"].toString();
        QJsonArray translations = obj["translation"].toArray();
        if (!translations.isEmpty()) {
            result.translatedText = translations.first().toString();
        }

        // 基本词典
        if (obj.contains("basic")) {
            QJsonObject basic = obj["basic"].toObject();
            result.pronunciation = basic["phonetic"].toString();

            QJsonArray explains = basic["explains"].toArray();
            for (const auto &e : explains) {
                result.alternatives << e.toString();
            }
        }

        result.engine = "有道翻译";
        result.confidence = 1.0f;

    } else if (engineName == "deepl") {
        // DeepL 响应
        if (obj.contains("message")) {
            m_lastError = tr("DeepL 错误: %1").arg(obj["message"].toString());
            emit errorOccurred(m_lastError);
            return;
        }

        QJsonArray translations = obj["translations"].toArray();
        if (!translations.isEmpty()) {
            QJsonObject first = translations.first().toObject();
            result.translatedText = first["text"].toString();
            result.sourceLanguage = first["detected_source_language"].toString().toLower();
        }

        result.engine = "DeepL";
        result.confidence = 1.0f;

    } else if (engineName == "tencent") {
        // 腾讯翻译响应
        QJsonObject response = obj["Response"].toObject();
        if (response.contains("Error")) {
            m_lastError = tr("腾讯翻译错误: %1").arg(
                response["Error"].toObject()["Message"].toString());
            emit errorOccurred(m_lastError);
            return;
        }

        result.translatedText = response["TargetText"].toString();
        result.sourceLanguage = response["Source"].toString();
        result.engine = "腾讯翻译";
        result.confidence = 1.0f;

    } else if (engineName == "google") {
        // Google 翻译响应
        QJsonObject responseData = obj["data"].toObject();
        QJsonArray translations = responseData["translations"].toArray();
        if (!translations.isEmpty()) {
            result.translatedText = translations.first().toObject()["translatedText"].toString();
        }

        result.engine = "Google 翻译";
        result.confidence = 1.0f;

    } else {
        m_lastError = tr("未知的翻译引擎: %1").arg(engineName);
        emit errorOccurred(m_lastError);
        return;
    }

    // 如果经过解析后译文仍为空，报告错误并附带原始响应以便诊断
    if (result.translatedText.isEmpty()) {
        QString rawResponse = QString::fromUtf8(data.left(800));
        m_lastError = tr("翻译返回了空结果。\nAPI 原始响应:\n%1").arg(rawResponse);
        emit errorOccurred(m_lastError);
        return;
    }

    // 缓存结果
    if (m_config.enableCache) {
        QString key = cacheKey(result.sourceText, result.targetLanguage);
        m_cache.insert(key, new TranslationResult(result));
    }

    // 检查是否是批量翻译
    if (!m_batchTexts.isEmpty() && m_batchIndex < m_batchTexts.size()) {
        m_batchResults.append(result);
        emit progressChanged(m_batchIndex + 1, m_batchTexts.size());

        m_batchIndex++;
        if (m_batchIndex < m_batchTexts.size()) {
            // 继续翻译下一个
            QTimer::singleShot(100, this, [this]() {
                translate(m_batchTexts[m_batchIndex], m_batchTargetLang, m_batchSourceLang);
            });
        } else {
            // 批量翻译完成
            emit batchTranslationCompleted(m_batchResults);
            m_batchTexts.clear();
            m_batchResults.clear();
        }
    } else {
        emit translationCompleted(result);
    }
}

// 函数说明：响应 TranslationManager 收到的信号或异步回调，并更新界面状态。
void TranslationManager::onBatchResponse()
{
    // 由 onTranslationResponse 处理
}

// 函数说明：响应 TranslationManager 收到的信号或异步回调，并更新界面状态。
void TranslationManager::onDetectResponse()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        m_lastError = tr("语言检测失败: %1").arg(reply->errorString());
        emit errorOccurred(m_lastError);
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    if (obj.contains("error_code")) {
        m_lastError = tr("语言检测错误: %1").arg(obj["error_msg"].toString());
        emit errorOccurred(m_lastError);
        return;
    }

    QJsonObject dataObj = obj["data"].toObject();
    QString lang = dataObj["src"].toString();
    QString text = reply->property("text").toString();

    emit languageDetected(text, lang);
}

// 函数说明：实现 TranslationManager::translateWithBaidu 的核心逻辑，供当前模块调用。
void TranslationManager::translateWithBaidu(const QString &text,
                                            const QString &source,
                                            const QString &target)
{
    QString url = "https://fanyi-api.baidu.com/api/trans/vip/translate";

    QString salt = QString::number(QRandomGenerator::global()->generate());
    QString sign = generateBaiduSign(text, salt);

    QUrlQuery query;
    query.addQueryItem("q", text);
    query.addQueryItem("from", toEngineCode(source, Engine::Baidu));
    query.addQueryItem("to", toEngineCode(target, Engine::Baidu));
    query.addQueryItem("appid", m_config.appId);
    query.addQueryItem("salt", salt);
    query.addQueryItem("sign", sign);

    QNetworkRequest request{QUrl{url}};
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/x-www-form-urlencoded");

    QNetworkReply *reply = m_networkManager->post(request,
                                                   query.toString(QUrl::FullyEncoded).toUtf8());
    reply->setProperty("engine", "baidu");
    reply->setProperty("sourceText", text);
    reply->setProperty("targetLang", target);

    connect(reply, &QNetworkReply::finished,
            this, &TranslationManager::onTranslationResponse);
}

// 函数说明：实现 TranslationManager::translateWithYoudao 的核心逻辑，供当前模块调用。
void TranslationManager::translateWithYoudao(const QString &text,
                                             const QString &source,
                                             const QString &target)
{
    QString url = "https://openapi.youdao.com/api";

    QString curtime = QString::number(QDateTime::currentSecsSinceEpoch());
    QString salt = QString::number(QRandomGenerator::global()->generate());

    // 有道 v3 签名: SHA256(应用ID + input + salt + curtime + 应用秘钥)
    // 注意: 有道签名中用的是"应用ID"(m_config.appId)，不是"APIkey"
    QString input = text;
    if (text.length() > 20) {
        input = text.left(10) + QString::number(text.length()) + text.right(10);
    }
    QString signStr = m_config.appId + input + salt + curtime + m_config.apiSecret;
    QByteArray hash = QCryptographicHash::hash(signStr.toUtf8(), QCryptographicHash::Sha256);
    QString sign = QString::fromLatin1(hash.toHex());

    QUrlQuery query;
    query.addQueryItem("q", text);
    query.addQueryItem("from", toEngineCode(source, Engine::Youdao));
    query.addQueryItem("to", toEngineCode(target, Engine::Youdao));
    // 有道 API 的 appKey 参数填"应用ID"
    query.addQueryItem("appKey", m_config.appId);
    query.addQueryItem("salt", salt);
    query.addQueryItem("sign", sign);
    query.addQueryItem("signType", "v3");
    query.addQueryItem("curtime", curtime);

    QNetworkRequest request{QUrl{url}};
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/x-www-form-urlencoded");

    QNetworkReply *reply = m_networkManager->post(request,
                                                   query.toString(QUrl::FullyEncoded).toUtf8());
    reply->setProperty("engine", "youdao");
    reply->setProperty("sourceText", text);
    reply->setProperty("targetLang", target);

    connect(reply, &QNetworkReply::finished,
            this, &TranslationManager::onTranslationResponse);
}

// 函数说明：实现 TranslationManager::translateWithDeepL 的核心逻辑，供当前模块调用。
void TranslationManager::translateWithDeepL(const QString &text,
                                            const QString &source,
                                            const QString &target)
{
    QString url = "https://api-free.deepl.com/v2/translate";

    QUrlQuery query;
    query.addQueryItem("text", text);
    query.addQueryItem("target_lang", toEngineCode(target, Engine::DeepL).toUpper());
    if (source != "auto") {
        query.addQueryItem("source_lang", toEngineCode(source, Engine::DeepL).toUpper());
    }

    QNetworkRequest request{QUrl{url}};
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/x-www-form-urlencoded");
    request.setRawHeader("Authorization", QString("DeepL-Auth-Key %1").arg(m_config.apiKey).toUtf8());

    QNetworkReply *reply = m_networkManager->post(request,
                                                   query.toString(QUrl::FullyEncoded).toUtf8());
    reply->setProperty("engine", "deepl");
    reply->setProperty("sourceText", text);
    reply->setProperty("targetLang", target);

    connect(reply, &QNetworkReply::finished,
            this, &TranslationManager::onTranslationResponse);
}

// 函数说明：实现 TranslationManager::translateWithGoogle 的核心逻辑，供当前模块调用。
void TranslationManager::translateWithGoogle(const QString &text,
                                             const QString &source,
                                             const QString &target)
{
    // Google Cloud Translation API
    QString url = QString("https://translation.googleapis.com/language/translate/v2?key=%1")
                      .arg(m_config.apiKey);

    QJsonObject requestBody;
    requestBody["q"] = text;
    requestBody["source"] = toEngineCode(source, Engine::Google);
    requestBody["target"] = toEngineCode(target, Engine::Google);
    requestBody["format"] = "text";

    QNetworkRequest request{QUrl{url}};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_networkManager->post(request,
                                                   QJsonDocument(requestBody).toJson());
    reply->setProperty("engine", "google");
    reply->setProperty("sourceText", text);
    reply->setProperty("targetLang", target);

    connect(reply, &QNetworkReply::finished,
            this, &TranslationManager::onTranslationResponse);
}

// 函数说明：实现 TranslationManager::translateWithTencent 的核心逻辑，供当前模块调用。
void TranslationManager::translateWithTencent(const QString &text,
                                              const QString &source,
                                              const QString &target)
{
    QString url = "https://tmt.tencentcloudapi.com";

    QJsonObject requestBody;
    requestBody["SourceText"] = text;
    requestBody["Source"] = toEngineCode(source, Engine::Tencent);
    requestBody["Target"] = toEngineCode(target, Engine::Tencent);
    requestBody["ProjectId"] = 0;

    // 需要实现腾讯云 API 签名
    QNetworkRequest request{QUrl{url}};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    // 添加腾讯云认证头...

    QNetworkReply *reply = m_networkManager->post(request,
                                                   QJsonDocument(requestBody).toJson());
    reply->setProperty("engine", "tencent");
    reply->setProperty("sourceText", text);
    reply->setProperty("targetLang", target);

    connect(reply, &QNetworkReply::finished,
            this, &TranslationManager::onTranslationResponse);
}

// 函数说明：实现 TranslationManager::translateOffline 的核心逻辑，供当前模块调用。
TranslationManager::TranslationResult TranslationManager::translateOffline(
    const QString &text, const QString &source, const QString &target)
{
    Q_UNUSED(source)

    TranslationResult result;
    result.sourceText = text;
    result.targetLanguage = target;
    result.engine = "离线词典";

    // 简单的单词查找
    QString translation = lookupWord(text.trimmed(), target);
    if (!translation.isEmpty()) {
        result.translatedText = translation;
        result.confidence = 1.0f;
    } else {
        result.translatedText = text;  // 未找到则返回原文
        result.confidence = 0.0f;
    }

    return result;
}

// 函数说明：根据当前数据生成 TranslationManager 需要的输出结果。
QString TranslationManager::generateBaiduSign(const QString &text, const QString &salt)
{
    QString str = m_config.appId + text + salt + m_config.apiSecret;
    QByteArray hash = QCryptographicHash::hash(str.toUtf8(), QCryptographicHash::Md5);
    return QString::fromLatin1(hash.toHex());
}

// 函数说明：根据当前数据生成 TranslationManager 需要的输出结果。
QString TranslationManager::generateYoudaoSign(const QString &text,
                                               const QString &salt,
                                               const QString &curtime)
{
    // 有道签名: SHA256(appKey + input + salt + curtime + appSecret)
    QString input = text;
    if (text.length() > 20) {
        input = text.left(10) + QString::number(text.length()) + text.right(10);
    }

    QString str = m_config.appId + input + salt + curtime + m_config.apiSecret;
    QByteArray hash = QCryptographicHash::hash(str.toUtf8(), QCryptographicHash::Sha256);
    return QString::fromLatin1(hash.toHex());
}

// 函数说明：根据当前数据生成 TranslationManager 需要的输出结果。
QString TranslationManager::generateTencentSign(const QMap<QString, QString> &params)
{
    Q_UNUSED(params)
    // 腾讯云签名算法（TC3-HMAC-SHA256）
    // 实际实现需要完整的签名流程
    return QString();
}

// 函数说明：实现 TranslationManager::toEngineCode 的核心逻辑，供当前模块调用。
QString TranslationManager::toEngineCode(const QString &code, Engine engine)
{
    QString lower = code.toLower();

    // 特殊处理 "auto"
    if (lower == "auto") {
        switch (engine) {
            case Engine::Baidu: return "auto";
            case Engine::Youdao: return "auto";
            case Engine::DeepL: return "";  // DeepL 不传则自动检测
            case Engine::Google: return "";
            case Engine::Tencent: return "auto";
            default: return "auto";
        }
    }

    // 语言代码映射
    QMap<QString, QString> baiduMap = {
        {"zh", "zh"}, {"zh-cn", "zh"}, {"zh-tw", "cht"},
        {"en", "en"}, {"en-us", "en"}, {"en-gb", "en"},
        {"ja", "jp"}, {"ko", "kor"},
        {"fr", "fra"}, {"de", "de"}, {"es", "spa"},
        {"ru", "ru"}, {"pt", "pt"}, {"it", "it"}
    };

    QMap<QString, QString> youdaoMap = {
        {"zh", "zh-CHS"}, {"zh-cn", "zh-CHS"}, {"zh-tw", "zh-CHT"},
        {"en", "en"}, {"ja", "ja"}, {"ko", "ko"},
        {"fr", "fr"}, {"de", "de"}, {"es", "es"},
        {"ru", "ru"}, {"pt", "pt"}, {"it", "it"}
    };

    QMap<QString, QString> deeplMap = {
        {"zh", "ZH"}, {"zh-cn", "ZH"}, {"zh-tw", "ZH"},
        {"en", "EN"}, {"en-us", "EN-US"}, {"en-gb", "EN-GB"},
        {"ja", "JA"}, {"ko", "KO"},
        {"fr", "FR"}, {"de", "DE"}, {"es", "ES"},
        {"ru", "RU"}, {"pt", "PT"}, {"it", "IT"}
    };

    switch (engine) {
        case Engine::Baidu:
            return baiduMap.value(lower, lower);
        case Engine::Youdao:
            return youdaoMap.value(lower, lower);
        case Engine::DeepL:
            return deeplMap.value(lower, lower.toUpper());
        case Engine::Google:
        case Engine::Tencent:
        default:
            return lower;
    }
}

// 函数说明：实现 TranslationManager::fromEngineCode 的核心逻辑，供当前模块调用。
QString TranslationManager::fromEngineCode(const QString &code, Engine engine)
{
    Q_UNUSED(engine)
    // 反向映射
    QString lower = code.toLower();

    if (lower == "zh-chs" || lower == "zh") return "zh-CN";
    if (lower == "zh-cht" || lower == "cht") return "zh-TW";
    if (lower == "jp") return "ja";
    if (lower == "kor") return "ko";
    if (lower == "fra") return "fr";
    if (lower == "spa") return "es";

    return lower;
}

// 函数说明：实现 TranslationManager::cacheKey 的核心逻辑，供当前模块调用。
QString TranslationManager::cacheKey(const QString &text, const QString &targetLang) const
{
    return QString("%1_%2_%3")
        .arg(static_cast<int>(m_config.engine))
        .arg(targetLang)
        .arg(QString::fromLatin1(
            QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Md5).toHex()));
}

// 函数说明：实现 TranslationManager::tryAcquireRequestQuota 的核心逻辑，供当前模块调用。
bool TranslationManager::tryAcquireRequestQuota(QString *reason)
{
    if (!m_config.enableRateLimit) {
        return true;
    }

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const int minIntervalMs = qMax(0, m_config.minRequestIntervalMs);
    const int maxPerMinute = qMax(1, m_config.maxRequestsPerMinute);
    const qint64 windowStart = nowMs - 60000;

    while (!m_requestTimestampsMs.isEmpty() &&
           m_requestTimestampsMs.front() < windowStart) {
        m_requestTimestampsMs.pop_front();
    }

    if (m_lastRequestTimestampMs > 0 &&
        nowMs - m_lastRequestTimestampMs < minIntervalMs) {
        if (reason) {
            const qint64 waitMs = minIntervalMs - (nowMs - m_lastRequestTimestampMs);
            *reason = tr("请求过于频繁，请在 %1 ms 后重试").arg(waitMs);
        }
        return false;
    }

    if (m_requestTimestampsMs.size() >= maxPerMinute) {
        if (reason) {
            const qint64 waitMs = qMax<qint64>(0, m_requestTimestampsMs.front() + 60000 - nowMs);
            *reason = tr("已达到速率限制（每分钟最多 %1 次），请在 %2 ms 后重试")
                          .arg(maxPerMinute)
                          .arg(waitMs);
        }
        return false;
    }

    m_requestTimestampsMs.append(nowMs);
    m_lastRequestTimestampMs = nowMs;
    return true;
}

// 函数说明：实现 TranslationManager::initLanguages 的核心逻辑，供当前模块调用。
void TranslationManager::initLanguages()
{
    if (!s_languages.isEmpty()) return;

    s_languages = {
        {"zh", "中文（简体）", "简体中文"},
        {"zh-CN", "中文（简体）", "简体中文"},
        {"zh-TW", "中文（繁体）", "繁體中文"},
        {"en", "英语", "English"},
        {"en-US", "英语（美国）", "English (US)"},
        {"en-GB", "英语（英国）", "English (UK)"},
        {"ja", "日语", "日本語"},
        {"ko", "韩语", "한국어"},
        {"fr", "法语", "Français"},
        {"de", "德语", "Deutsch"},
        {"es", "西班牙语", "Español"},
        {"it", "意大利语", "Italiano"},
        {"pt", "葡萄牙语", "Português"},
        {"ru", "俄语", "Русский"},
        {"ar", "阿拉伯语", "العربية"},
        {"th", "泰语", "ไทย"},
        {"vi", "越南语", "Tiếng Việt"},
        {"id", "印尼语", "Bahasa Indonesia"},
        {"ms", "马来语", "Bahasa Melayu"},
        {"nl", "荷兰语", "Nederlands"},
        {"pl", "波兰语", "Polski"},
        {"tr", "土耳其语", "Türkçe"},
        {"el", "希腊语", "Ελληνικά"},
        {"cs", "捷克语", "Čeština"},
        {"sv", "瑞典语", "Svenska"},
        {"da", "丹麦语", "Dansk"},
        {"fi", "芬兰语", "Suomi"},
        {"hu", "匈牙利语", "Magyar"},
        {"uk", "乌克兰语", "Українська"},
        {"he", "希伯来语", "עברית"},
        {"hi", "印地语", "हिन्दी"}
    };
}

