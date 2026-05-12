// 文件说明：app-static\ai\bailianclient.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef BAILIANCLIENT_H
#define BAILIANCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>

/**
 * @brief 阿里百炼 API 客户端
 *
 * 封装阿里百炼大模型 API 调用，支持流式和非流式响应
 */
class BaiLianClient : public QObject
{
    Q_OBJECT

public:
    explicit BaiLianClient(QObject *parent = nullptr);
    ~BaiLianClient();

    // API 配置
    void setApiKey(const QString &apiKey);
    void setModel(const QString &model);
    void setBaseUrl(const QString &baseUrl);
    void setRateLimit(bool enabled, int maxRequestsPerMinute, int minRequestIntervalMs);

    QString apiKey() const { return m_apiKey; }
    QString model() const { return m_model; }
    QString baseUrl() const { return m_baseUrl; }
    bool rateLimitEnabled() const { return m_enableRateLimit; }
    int maxRequestsPerMinute() const { return m_maxRequestsPerMinute; }
    int minRequestIntervalMs() const { return m_minRequestIntervalMs; }

    // 发送请求
    void sendChatRequest(const QJsonArray &messages,
                         double temperature = 0.7,
                         int maxTokens = 2048,
                         bool stream = false);

    // 取消请求
    void cancelRequest();

    // 状态
    bool isRequesting() const { return m_currentReply != nullptr; }

    // 预设的系统提示词
    static QString completionSystemPrompt();
    static QString grammarCheckSystemPrompt();
    static QString summarySystemPrompt();
    static QString titleSystemPrompt();

signals:
    void responseReceived(const QString &content);
    void streamChunkReceived(const QString &chunk);
    void streamFinished(const QString &fullContent);
    void errorOccurred(const QString &error);
    void requestStarted();
    void requestFinished();

private slots:
    void onReplyFinished();
    void onReadyRead();
    void onErrorOccurred(QNetworkReply::NetworkError error);

private:
    void parseStreamChunk(const QByteArray &data);
    QString extractContentFromResponse(const QJsonObject &response);
    bool tryAcquireRequestQuota(QString *reason = nullptr);

    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_currentReply;

    QString m_apiKey;
    QString m_model;
    QString m_baseUrl;
    QString m_streamBuffer;
    QString m_fullStreamContent;
    bool m_isStreaming;
    bool m_enableRateLimit;
    int m_maxRequestsPerMinute;
    int m_minRequestIntervalMs;
    QList<qint64> m_requestTimestampsMs;
    qint64 m_lastRequestTimestampMs;
};

#endif // BAILIANCLIENT_H

