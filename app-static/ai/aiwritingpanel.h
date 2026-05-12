// 文件说明：app-static\ai\aiwritingpanel.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef AIWRITINGPANEL_H
#define AIWRITINGPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QGroupBox>
#include <QListWidget>
#include <QProgressBar>
#include <QStackedWidget>
#include <QTabWidget>
#include <QCheckBox>

#include "aiwritingassistant.h"

/**
 * @brief AI 写作助手面板
 *
 * 提供 AI 写作辅助功能的 UI 界面
 */
class AIWritingPanel : public QWidget
{
    Q_OBJECT

public:
    explicit AIWritingPanel(QWidget *parent = nullptr);
    ~AIWritingPanel() = default;

    void setAssistant(AIWritingAssistant *assistant);
    AIWritingAssistant* assistant() const { return m_assistant; }

signals:
    void insertTextRequested(const QString &text);
    void applyTitleRequested(const QString &title);

private slots:
    // 按钮点击
    void onCompletionClicked();
    void onGrammarCheckClicked();
    void onSummaryClicked();
    void onTitleClicked();
    void onCancelClicked();
    void onInsertClicked();
    void onCopyClicked();
    void onSettingsClicked();

    // AI 响应
    void onCompletionChunk(const QString &chunk);
    void onCompletionFinished(const QString &content);
    void onGrammarCheckFinished(const QString &suggestions);
    void onSummaryFinished(const QString &summary);
    void onTitleSuggestionsFinished(const QStringList &titles);
    void onRequestStarted(AIWritingAssistant::TaskType task);
    void onRequestFinished(AIWritingAssistant::TaskType task);
    void onError(const QString &error);

    // 标题选择
    void onTitleItemDoubleClicked(QListWidgetItem *item);

private:
    void setupUi();
    void setupToolsPage();
    void setupResultPage();
    void setupSettingsPage();
    void showResultPage();
    void showToolsPage();
    void setLoading(bool loading);
    void clearResult();

    AIWritingAssistant *m_assistant;

    // 主布局
    QVBoxLayout *m_mainLayout;
    QStackedWidget *m_stackedWidget;

    // 工具页面
    QWidget *m_toolsPage;
    QPushButton *m_completionButton;
    QPushButton *m_grammarCheckButton;
    QPushButton *m_summaryButton;
    QPushButton *m_titleButton;
    QCheckBox *m_useSelectionCheck;

    // 结果页面
    QWidget *m_resultPage;
    QLabel *m_taskLabel;
    QTextEdit *m_resultText;
    QListWidget *m_titleList;
    QStackedWidget *m_resultStack;
    QPushButton *m_insertButton;
    QPushButton *m_copyButton;
    QPushButton *m_backButton;
    QPushButton *m_cancelButton;
    QProgressBar *m_progressBar;

    // 设置页面
    QWidget *m_settingsPage;
    QLineEdit *m_apiKeyEdit;
    QComboBox *m_modelCombo;
    QPushButton *m_saveSettingsButton;
    QPushButton *m_backFromSettingsButton;

    // 当前状态
    AIWritingAssistant::TaskType m_currentTask;
    bool m_isLoading;
};

#endif // AIWRITINGPANEL_H

