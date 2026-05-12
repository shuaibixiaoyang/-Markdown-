// 文件说明：app-static\ai\aiwritingassistant.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef AIWRITINGASSISTANT_H
#define AIWRITINGASSISTANT_H

#include <QObject>
#include <QPlainTextEdit>
#include <QSettings>

#include "bailianclient.h"

/**
 * @brief AI 写作助手管理器
 *
 * 提供 AI 智能写作辅助功能：
 * - AI 补全：智能续写段落
 * - 语法纠错：AI 驱动的语法和风格建议
 * - 自动摘要：长文档一键生成摘要
 * - 智能标题：基于内容自动生成标题建议
 */
class AIWritingAssistant : public QObject
{
    Q_OBJECT

public:
    enum class TaskType {
        Completion,     // AI 补全
        GrammarCheck,   // 语法纠错
        Summary,        // 自动摘要
        TitleSuggestion // 智能标题
    };
    Q_ENUM(TaskType)

    explicit AIWritingAssistant(QObject *parent = nullptr);
    ~AIWritingAssistant();

    // 编辑器绑定
    void setEditor(QPlainTextEdit *editor);
    QPlainTextEdit* editor() const { return m_editor; }

    // API 配置
    void setApiKey(const QString &apiKey);
    void setModel(const QString &model);
    QString apiKey() const;
    QString model() const;

    // 保存/加载配置
    void saveSettings();
    void loadSettings();

    // AI 功能
    void requestCompletion(bool useSelection = false, bool stream = true);
    void requestGrammarCheck(bool useSelection = false);
    void requestSummary();
    void requestTitleSuggestions();

    // 取消请求
    void cancelRequest();

    // 状态
    bool isRequesting() const;
    bool isApiKeyConfigured() const;
    TaskType currentTask() const { return m_currentTask; }

    // 客户端访问
    BaiLianClient* client() const { return m_client; }

signals:
    // 补全相关信号
    void completionChunkReceived(const QString &chunk);
    void completionFinished(const QString &fullContent);

    // 语法检查信号
    void grammarCheckFinished(const QString &suggestions);

    // 摘要信号
    void summaryFinished(const QString &summary);

    // 标题建议信号
    void titleSuggestionsFinished(const QStringList &titles);

    // 通用信号
    void requestStarted(TaskType task);
    void requestFinished(TaskType task);
    void errorOccurred(const QString &error);

public slots:
    void insertCompletion(const QString &text);
    void applySelectedTitle(const QString &title);

private slots:
    void onResponseReceived(const QString &content);
    void onStreamChunkReceived(const QString &chunk);
    void onStreamFinished(const QString &fullContent);
    void onError(const QString &error);

private:
    QString getTextForContext(bool useSelection) const;
    void parseGrammarCheckResponse(const QString &response);
    void parseTitleSuggestions(const QString &response);

    QPlainTextEdit *m_editor;
    BaiLianClient *m_client;
    TaskType m_currentTask;
    bool m_isStreaming;

    // 用于补全时记录插入位置
    int m_completionInsertPosition;
};

#endif // AIWRITINGASSISTANT_H

