// 文件说明：app-static\ai\bailianclient.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "bailianclient.h"
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QDateTime>
#include <QDebug>

// 函数说明：构造 BaiLianClient 对象，初始化本模块需要的状态、界面和资源。
BaiLianClient::BaiLianClient(QObject *parent)
    : QObject(parent)
    , m_networkManager(nullptr)
    , m_currentReply(nullptr)
    , m_model("qwen-turbo")
    , m_baseUrl("https://dashscope.aliyuncs.com/compatible-mode/v1")
    , m_isStreaming(false)
    , m_enableRateLimit(true)
    , m_maxRequestsPerMinute(60)
    , m_minRequestIntervalMs(300)
    , m_lastRequestTimestampMs(0)
{
    qDebug() << "BaiLianClient: Initializing...";
    try {
        m_networkManager = new QNetworkAccessManager(this);
        qDebug() << "BaiLianClient: QNetworkAccessManager created successfully";
    } catch (const std::exception &e) {
        qCritical() << "BaiLianClient: Failed to create QNetworkAccessManager:" << e.what();
    } catch (...) {
        qCritical() << "BaiLianClient: Failed to create QNetworkAccessManager: unknown error";
    }
}

// 函数说明：销毁 BaiLianClient 对象，释放本模块持有的资源。
BaiLianClient::~BaiLianClient()
{
    cancelRequest();
}

// 函数说明：设置 BaiLianClient 的运行参数，并触发必要的界面或数据刷新。
void BaiLianClient::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
}

// 函数说明：设置 BaiLianClient 的运行参数，并触发必要的界面或数据刷新。
void BaiLianClient::setModel(const QString &model)
{
    m_model = model;
}

// 函数说明：设置 BaiLianClient 的运行参数，并触发必要的界面或数据刷新。
void BaiLianClient::setBaseUrl(const QString &baseUrl)
{
    m_baseUrl = baseUrl;
}

// 函数说明：设置 BaiLianClient 的运行参数，并触发必要的界面或数据刷新。
void BaiLianClient::setRateLimit(bool enabled, int maxRequestsPerMinute, int minRequestIntervalMs)
{
    m_enableRateLimit = enabled;
    m_maxRequestsPerMinute = qMax(1, maxRequestsPerMinute);
    m_minRequestIntervalMs = qMax(0, minRequestIntervalMs);
    m_requestTimestampsMs.clear();
    m_lastRequestTimestampMs = 0;
}

// 函数说明：实现 BaiLianClient::sendChatRequest 的核心逻辑，供当前模块调用。
void BaiLianClient::sendChatRequest(const QJsonArray &messages,
                                     double temperature,
                                     int maxTokens,
                                     bool stream)
{
    qDebug() << "BaiLianClient::sendChatRequest called";

    if (!m_networkManager) {
        qCritical() << "BaiLianClient: NetworkManager is null!";
        emit errorOccurred(tr("网络管理器未初始化"));
        return;
    }

    if (m_apiKey.isEmpty()) {
        qDebug() << "BaiLianClient: API Key is empty";
        emit errorOccurred(tr("API Key 未设置"));
        return;
    }

    QString rateLimitReason;
    if (!tryAcquireRequestQuota(&rateLimitReason)) {
        qDebug() << "BaiLianClient: Rate limit exceeded:" << rateLimitReason;
        emit errorOccurred(rateLimitReason);
        return;
    }

    cancelRequest();

    m_isStreaming = stream;
    m_streamBuffer.clear();
    m_fullStreamContent.clear();

    // 构建请求 URL
    QUrl url(m_baseUrl + "/chat/completions");
    qDebug() << "BaiLianClient: Request URL:" << url.toString();

    // 构建请求体
    QJsonObject requestBody;
    requestBody["model"] = m_model;
    requestBody["messages"] = messages;
    requestBody["temperature"] = temperature;
    requestBody["max_tokens"] = maxTokens;
    requestBody["stream"] = stream;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());

    // 发送请求
    QByteArray data = QJsonDocument(requestBody).toJson();
    qDebug() << "BaiLianClient: Sending request, stream mode:" << stream;

    m_currentReply = m_networkManager->post(request, data);

    if (!m_currentReply) {
        qCritical() << "BaiLianClient: Failed to create network reply!";
        emit errorOccurred(tr("无法创建网络请求"));
        return;
    }

    connect(m_currentReply, &QNetworkReply::finished,
            this, &BaiLianClient::onReplyFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred,
            this, &BaiLianClient::onErrorOccurred);

    if (stream) {
        connect(m_currentReply, &QNetworkReply::readyRead,
                this, &BaiLianClient::onReadyRead);
    }

    qDebug() << "BaiLianClient: Request sent successfully";
    emit requestStarted();
}

// 函数说明：实现 BaiLianClient::cancelRequest 的核心逻辑，供当前模块调用。
void BaiLianClient::cancelRequest()
{
    if (m_currentReply) {
        disconnect(m_currentReply, nullptr, this, nullptr);
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
}

// 函数说明：响应 BaiLianClient 收到的信号或异步回调，并更新界面状态。
void BaiLianClient::onReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    if (reply != m_currentReply) {
        // Ignore stale reply from a previous cancelled/replaced request.
        reply->deleteLater();
        return;
    }

    if (reply->error() == QNetworkReply::NoError) {
        if (m_isStreaming) {
            // 流式响应已经通过 onReadyRead 处理
            emit streamFinished(m_fullStreamContent);
        } else {
            // 非流式响应
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);

            if (doc.isObject()) {
                QString content = extractContentFromResponse(doc.object());
                emit responseReceived(content);
            } else {
                emit errorOccurred(tr("无效的响应格式"));
            }
        }
    }

    reply->deleteLater();
    m_currentReply = nullptr;
    emit requestFinished();
}

// 函数说明：响应 BaiLianClient 收到的信号或异步回调，并更新界面状态。
void BaiLianClient::onReadyRead()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply || reply != m_currentReply) {
        return;
    }

    QByteArray data = reply->readAll();
    parseStreamChunk(data);
}

// 函数说明：响应 BaiLianClient 收到的信号或异步回调，并更新界面状态。
void BaiLianClient::onErrorOccurred(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error);

    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply || reply != m_currentReply) {
        return;
    }

    QString errorMessage = reply->errorString();

    // 尝试从响应体获取更详细的错误信息
    QByteArray responseData = reply->readAll();
    if (!responseData.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("error")) {
                QJsonObject errorObj = obj["error"].toObject();
                if (errorObj.contains("message")) {
                    errorMessage = errorObj["message"].toString();
                }
            }
        }
    }

    emit errorOccurred(errorMessage);
}

// 函数说明：解析输入内容，转换为 BaiLianClient 后续处理使用的数据结构。
void BaiLianClient::parseStreamChunk(const QByteArray &data)
{
    m_streamBuffer += QString::fromUtf8(data);

    // SSE 格式解析
    QStringList lines = m_streamBuffer.split("\n");

    // 保留最后一个不完整的行
    m_streamBuffer = lines.takeLast();

    for (const QString &line : lines) {
        if (line.startsWith("data: ")) {
            QString jsonStr = line.mid(6).trimmed();

            if (jsonStr == "[DONE]") {
                continue;
            }

            QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                QJsonArray choices = obj["choices"].toArray();

                if (!choices.isEmpty()) {
                    QJsonObject choice = choices[0].toObject();
                    QJsonObject delta = choice["delta"].toObject();

                    if (delta.contains("content")) {
                        QString content = delta["content"].toString();
                        m_fullStreamContent += content;
                        emit streamChunkReceived(content);
                    }
                }
            }
        }
    }
}

// 函数说明：实现 BaiLianClient::extractContentFromResponse 的核心逻辑，供当前模块调用。
QString BaiLianClient::extractContentFromResponse(const QJsonObject &response)
{
    QJsonArray choices = response["choices"].toArray();
    if (choices.isEmpty()) {
        return QString();
    }

    QJsonObject choice = choices[0].toObject();
    QJsonObject message = choice["message"].toObject();

    return message["content"].toString();
}

// 函数说明：实现 BaiLianClient::tryAcquireRequestQuota 的核心逻辑，供当前模块调用。
bool BaiLianClient::tryAcquireRequestQuota(QString *reason)
{
    if (!m_enableRateLimit) {
        return true;
    }

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 windowStart = nowMs - 60000;

    while (!m_requestTimestampsMs.isEmpty() &&
           m_requestTimestampsMs.front() < windowStart) {
        m_requestTimestampsMs.pop_front();
    }

    if (m_lastRequestTimestampMs > 0 &&
        nowMs - m_lastRequestTimestampMs < m_minRequestIntervalMs) {
        if (reason) {
            const qint64 waitMs = m_minRequestIntervalMs - (nowMs - m_lastRequestTimestampMs);
            *reason = tr("请求过于频繁，请在 %1 ms 后重试").arg(waitMs);
        }
        return false;
    }

    if (m_requestTimestampsMs.size() >= m_maxRequestsPerMinute) {
        if (reason) {
            const qint64 waitMs = qMax<qint64>(0, m_requestTimestampsMs.front() + 60000 - nowMs);
            *reason = tr("已达到速率限制（每分钟最多 %1 次），请在 %2 ms 后重试")
                          .arg(m_maxRequestsPerMinute)
                          .arg(waitMs);
        }
        return false;
    }

    m_requestTimestampsMs.append(nowMs);
    m_lastRequestTimestampMs = nowMs;
    return true;
}

// 函数说明：实现 BaiLianClient::completionSystemPrompt 的核心逻辑，供当前模块调用。
QString BaiLianClient::completionSystemPrompt()
{
    return QStringLiteral(
        "你是一个专业的写作助手。你的任务是根据用户提供的文本内容，智能续写后续段落。"
        "请注意以下几点：\n"
        "1. 保持与原文相同的写作风格和语气\n"
        "2. 内容要自然流畅，逻辑连贯\n"
        "3. 续写内容不要太长，1-3段即可\n"
        "4. 如果原文是 Markdown 格式，请保持相同的格式\n"
        "5. 直接输出续写内容，不要添加任何解释或前缀"
    );
}

// 函数说明：实现 BaiLianClient::grammarCheckSystemPrompt 的核心逻辑，供当前模块调用。
QString BaiLianClient::grammarCheckSystemPrompt()
{
    return QStringLiteral(
        "你是一个专业的语法和文风审校专家。请检查用户提供的文本，并提供以下方面的建议：\n"
        "1. 语法错误（包括标点符号）\n"
        "2. 拼写错误\n"
        "3. 用词不当或可以改进的地方\n"
        "4. 句子结构问题\n"
        "5. 文风一致性\n\n"
        "请以清晰的格式列出问题和建议的修改。如果没有问题，请说明文本已经很完善。"
        "对于每个问题，请指出具体位置和建议的修改。"
    );
}

// 函数说明：实现 BaiLianClient::summarySystemPrompt 的核心逻辑，供当前模块调用。
QString BaiLianClient::summarySystemPrompt()
{
    return QStringLiteral(
        "你是一个专业的文档摘要专家。请为用户提供的文档生成一个简洁、准确的摘要。\n"
        "摘要应该：\n"
        "1. 包含文档的核心观点和主要内容\n"
        "2. 简洁明了，通常在 100-300 字之间\n"
        "3. 保持客观中立\n"
        "4. 如果文档有多个主题，可以分点列出\n"
        "5. 直接输出摘要内容，不要添加前缀"
    );
}

// 函数说明：实现 BaiLianClient::titleSystemPrompt 的核心逻辑，供当前模块调用。
QString BaiLianClient::titleSystemPrompt()
{
    return QStringLiteral(
        "你是一个专业的标题创作专家。请根据用户提供的文档内容，生成 3-5 个合适的标题建议。\n"
        "标题应该：\n"
        "1. 简洁有力，能够准确概括文档主题\n"
        "2. 具有吸引力\n"
        "3. 长度适中（通常在 10-30 个字符）\n"
        "4. 风格与文档内容匹配\n\n"
        "请以编号列表的形式输出标题建议，例如：\n"
        "1. 第一个标题建议\n"
        "2. 第二个标题建议\n"
        "..."
    );
}

