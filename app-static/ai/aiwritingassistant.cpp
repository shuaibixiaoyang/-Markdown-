// 文件说明：app-static\ai\aiwritingassistant.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "aiwritingassistant.h"
#include <QTextCursor>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QSysInfo>
#include <QDebug>

namespace {

const QString kEncryptedPrefix = QStringLiteral("enc:v1:");

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

    // 兼容旧配置：历史版本直接明文存储。
    return storedText;
}

} // namespace

// 函数说明：构造 AIWritingAssistant 对象，初始化本模块需要的状态、界面和资源。
AIWritingAssistant::AIWritingAssistant(QObject *parent)
    : QObject(parent)
    , m_editor(nullptr)
    , m_client(nullptr)
    , m_currentTask(TaskType::Completion)
    , m_isStreaming(false)
    , m_completionInsertPosition(-1)
{
    // 创建客户端
    m_client = new BaiLianClient(this);

    if (m_client) {
        connect(m_client, &BaiLianClient::responseReceived,
                this, &AIWritingAssistant::onResponseReceived);
        connect(m_client, &BaiLianClient::streamChunkReceived,
                this, &AIWritingAssistant::onStreamChunkReceived);
        connect(m_client, &BaiLianClient::streamFinished,
                this, &AIWritingAssistant::onStreamFinished);
        connect(m_client, &BaiLianClient::errorOccurred,
                this, &AIWritingAssistant::onError);
        connect(m_client, &BaiLianClient::requestStarted, this, [this]() {
            emit requestStarted(m_currentTask);
        });
        connect(m_client, &BaiLianClient::requestFinished, this, [this]() {
            emit requestFinished(m_currentTask);
        });

        // 加载设置
        try {
            loadSettings();
        } catch (...) {
            // 如果加载设置失败，使用默认值
            qWarning() << "Failed to load AI settings, using defaults";
        }
    }
}

// 函数说明：销毁 AIWritingAssistant 对象，释放本模块持有的资源。
AIWritingAssistant::~AIWritingAssistant()
{
    saveSettings();
}

// 函数说明：设置 AIWritingAssistant 的运行参数，并触发必要的界面或数据刷新。
void AIWritingAssistant::setEditor(QPlainTextEdit *editor)
{
    m_editor = editor;
}

// 函数说明：设置 AIWritingAssistant 的运行参数，并触发必要的界面或数据刷新。
void AIWritingAssistant::setApiKey(const QString &apiKey)
{
    m_client->setApiKey(apiKey);
}

// 函数说明：设置 AIWritingAssistant 的运行参数，并触发必要的界面或数据刷新。
void AIWritingAssistant::setModel(const QString &model)
{
    m_client->setModel(model);
}

// 函数说明：实现 AIWritingAssistant::apiKey 的核心逻辑，供当前模块调用。
QString AIWritingAssistant::apiKey() const
{
    return m_client->apiKey();
}

// 函数说明：实现 AIWritingAssistant::model 的核心逻辑，供当前模块调用。
QString AIWritingAssistant::model() const
{
    return m_client->model();
}

// 函数说明：保存 AIWritingAssistant 当前状态，保证用户修改可以持久化。
void AIWritingAssistant::saveSettings()
{
    QSettings settings;
    settings.beginGroup("AIWritingAssistant");
    settings.setValue("apiKey", encryptSecret(m_client->apiKey()));
    settings.setValue("model", m_client->model());
    settings.setValue("rateLimitEnabled", m_client->rateLimitEnabled());
    settings.setValue("maxRequestsPerMinute", m_client->maxRequestsPerMinute());
    settings.setValue("minRequestIntervalMs", m_client->minRequestIntervalMs());
    settings.endGroup();
}

// 函数说明：加载 AIWritingAssistant 需要的数据、配置或外部资源。
void AIWritingAssistant::loadSettings()
{
    QSettings settings;
    settings.beginGroup("AIWritingAssistant");
    m_client->setApiKey(decryptSecret(settings.value("apiKey", "").toString()));
    m_client->setModel(settings.value("model", "qwen-turbo").toString());
    m_client->setRateLimit(
        settings.value("rateLimitEnabled", true).toBool(),
        settings.value("maxRequestsPerMinute", 60).toInt(),
        settings.value("minRequestIntervalMs", 300).toInt());
    settings.endGroup();
}

// 函数说明：实现 AIWritingAssistant::requestCompletion 的核心逻辑，供当前模块调用。
void AIWritingAssistant::requestCompletion(bool useSelection, bool stream)
{
    if (!m_editor) {
        emit errorOccurred(tr("编辑器未设置"));
        return;
    }

    if (!m_client) {
        emit errorOccurred(tr("AI 客户端未初始化"));
        return;
    }

    if (!isApiKeyConfigured()) {
        emit errorOccurred(tr("请先在 AI 设置中配置 API Key"));
        return;
    }

    QString context = getTextForContext(useSelection);
    if (context.isEmpty()) {
        emit errorOccurred(tr("没有可用于补全的文本内容"));
        return;
    }

    m_currentTask = TaskType::Completion;
    m_isStreaming = stream;

    // 记录插入位置（光标位置或选区末尾）
    QTextCursor cursor = m_editor->textCursor();
    m_completionInsertPosition = cursor.hasSelection() ?
        cursor.selectionEnd() : cursor.position();

    QJsonArray messages;

    // 系统提示词
    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] = BaiLianClient::completionSystemPrompt();
    messages.append(systemMessage);

    // 用户消息
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = QString("请续写以下内容：\n\n%1").arg(context);
    messages.append(userMessage);

    m_client->sendChatRequest(messages, 0.8, 1024, stream);
}

// 函数说明：实现 AIWritingAssistant::requestGrammarCheck 的核心逻辑，供当前模块调用。
void AIWritingAssistant::requestGrammarCheck(bool useSelection)
{
    if (!m_editor) {
        emit errorOccurred(tr("编辑器未设置"));
        return;
    }

    if (!isApiKeyConfigured()) {
        emit errorOccurred(tr("请先在 AI 设置中配置 API Key"));
        return;
    }

    QString text = getTextForContext(useSelection);
    if (text.isEmpty()) {
        emit errorOccurred(tr("没有可检查的文本内容"));
        return;
    }

    m_currentTask = TaskType::GrammarCheck;
    m_isStreaming = false;

    QJsonArray messages;

    // 系统提示词
    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] = BaiLianClient::grammarCheckSystemPrompt();
    messages.append(systemMessage);

    // 用户消息
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = QString("请检查以下文本：\n\n%1").arg(text);
    messages.append(userMessage);

    m_client->sendChatRequest(messages, 0.3, 2048, false);
}

// 函数说明：实现 AIWritingAssistant::requestSummary 的核心逻辑，供当前模块调用。
void AIWritingAssistant::requestSummary()
{
    if (!m_editor) {
        emit errorOccurred(tr("编辑器未设置"));
        return;
    }

    if (!isApiKeyConfigured()) {
        emit errorOccurred(tr("请先在 AI 设置中配置 API Key"));
        return;
    }

    QString text = m_editor->toPlainText();
    if (text.isEmpty()) {
        emit errorOccurred(tr("文档内容为空"));
        return;
    }

    m_currentTask = TaskType::Summary;
    m_isStreaming = false;

    QJsonArray messages;

    // 系统提示词
    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] = BaiLianClient::summarySystemPrompt();
    messages.append(systemMessage);

    // 用户消息
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = QString("请为以下文档生成摘要：\n\n%1").arg(text);
    messages.append(userMessage);

    m_client->sendChatRequest(messages, 0.5, 1024, false);
}

// 函数说明：实现 AIWritingAssistant::requestTitleSuggestions 的核心逻辑，供当前模块调用。
void AIWritingAssistant::requestTitleSuggestions()
{
    if (!m_editor) {
        emit errorOccurred(tr("编辑器未设置"));
        return;
    }

    if (!isApiKeyConfigured()) {
        emit errorOccurred(tr("请先在 AI 设置中配置 API Key"));
        return;
    }

    QString text = m_editor->toPlainText();
    if (text.isEmpty()) {
        emit errorOccurred(tr("文档内容为空"));
        return;
    }

    // 取前 2000 字符用于生成标题
    QString context = text.left(2000);

    m_currentTask = TaskType::TitleSuggestion;
    m_isStreaming = false;

    QJsonArray messages;

    // 系统提示词
    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] = BaiLianClient::titleSystemPrompt();
    messages.append(systemMessage);

    // 用户消息
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = QString("请根据以下内容生成标题建议：\n\n%1").arg(context);
    messages.append(userMessage);

    m_client->sendChatRequest(messages, 0.7, 512, false);
}

// 函数说明：实现 AIWritingAssistant::cancelRequest 的核心逻辑，供当前模块调用。
void AIWritingAssistant::cancelRequest()
{
    m_client->cancelRequest();
}

// 函数说明：判断 AIWritingAssistant 当前是否满足指定状态。
bool AIWritingAssistant::isRequesting() const
{
    return m_client->isRequesting();
}

// 函数说明：判断 AIWritingAssistant 当前是否满足指定状态。
bool AIWritingAssistant::isApiKeyConfigured() const
{
    return !m_client->apiKey().trimmed().isEmpty();
}

// 函数说明：实现 AIWritingAssistant::insertCompletion 的核心逻辑，供当前模块调用。
void AIWritingAssistant::insertCompletion(const QString &text)
{
    if (!m_editor || text.isEmpty()) return;

    QTextCursor cursor = m_editor->textCursor();

    // 如果有记录的插入位置，移动到那里
    if (m_completionInsertPosition >= 0) {
        cursor.setPosition(m_completionInsertPosition);
    }

    cursor.insertText(text);
    m_editor->setTextCursor(cursor);

    // 重置插入位置
    m_completionInsertPosition = -1;
}

// 函数说明：应用 AIWritingAssistant 当前配置，让编辑器或预览立即生效。
void AIWritingAssistant::applySelectedTitle(const QString &title)
{
    if (!m_editor || title.isEmpty()) return;

    QString text = m_editor->toPlainText();

    // 检查是否已有一级标题
    QRegularExpression h1Regex("^#\\s+.+$", QRegularExpression::MultilineOption);
    QRegularExpressionMatch match = h1Regex.match(text);

    QTextCursor cursor = m_editor->textCursor();
    cursor.movePosition(QTextCursor::Start);

    if (match.hasMatch()) {
        // 替换现有标题
        cursor.setPosition(match.capturedStart());
        cursor.setPosition(match.capturedEnd(), QTextCursor::KeepAnchor);
        cursor.insertText(QString("# %1").arg(title));
    } else {
        // 在文档开头插入标题
        cursor.insertText(QString("# %1\n\n").arg(title));
    }

    m_editor->setTextCursor(cursor);
}

// 函数说明：响应 AIWritingAssistant 收到的信号或异步回调，并更新界面状态。
void AIWritingAssistant::onResponseReceived(const QString &content)
{
    switch (m_currentTask) {
    case TaskType::Completion:
        emit completionFinished(content);
        break;
    case TaskType::GrammarCheck:
        emit grammarCheckFinished(content);
        break;
    case TaskType::Summary:
        emit summaryFinished(content);
        break;
    case TaskType::TitleSuggestion:
        parseTitleSuggestions(content);
        break;
    }
}

// 函数说明：响应 AIWritingAssistant 收到的信号或异步回调，并更新界面状态。
void AIWritingAssistant::onStreamChunkReceived(const QString &chunk)
{
    if (m_currentTask == TaskType::Completion) {
        emit completionChunkReceived(chunk);
    }
}

// 函数说明：响应 AIWritingAssistant 收到的信号或异步回调，并更新界面状态。
void AIWritingAssistant::onStreamFinished(const QString &fullContent)
{
    if (m_currentTask == TaskType::Completion) {
        emit completionFinished(fullContent);
    }
}

// 函数说明：响应 AIWritingAssistant 收到的信号或异步回调，并更新界面状态。
void AIWritingAssistant::onError(const QString &error)
{
    emit errorOccurred(error);
}

// 函数说明：读取 AIWritingAssistant 当前保存的状态或计算结果。
QString AIWritingAssistant::getTextForContext(bool useSelection) const
{
    if (!m_editor) return QString();

    if (useSelection && m_editor->textCursor().hasSelection()) {
        return m_editor->textCursor().selectedText();
    }

    // 获取光标前的文本作为上下文
    QTextCursor cursor = m_editor->textCursor();
    int position = cursor.position();

    QString fullText = m_editor->toPlainText();

    // 取光标前最多 2000 字符
    int start = qMax(0, position - 2000);
    return fullText.mid(start, position - start);
}

// 函数说明：解析输入内容，转换为 AIWritingAssistant 后续处理使用的数据结构。
void AIWritingAssistant::parseTitleSuggestions(const QString &response)
{
    QStringList titles;

    // 解析编号列表格式
    QRegularExpression lineRegex("^\\d+[.、]\\s*(.+)$", QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator it = lineRegex.globalMatch(response);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString title = match.captured(1).trimmed();
        if (!title.isEmpty()) {
            titles.append(title);
        }
    }

    // 如果解析失败，尝试按行分割
    if (titles.isEmpty()) {
        QStringList lines = response.split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            QString trimmed = line.trimmed();
            // 移除常见的列表前缀
            trimmed.remove(QRegularExpression("^[\\d]+[.、]\\s*"));
            trimmed.remove(QRegularExpression("^[-*•]\\s*"));
            if (!trimmed.isEmpty() && trimmed.length() < 100) {
                titles.append(trimmed);
            }
        }
    }

    emit titleSuggestionsFinished(titles);
}

