// 文件说明：app-static\ai\aiwritingpanel.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "aiwritingpanel.h"
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QCheckBox>
#include <QScrollBar>

// 函数说明：构造 AIWritingPanel 对象，初始化本模块需要的状态、界面和资源。
AIWritingPanel::AIWritingPanel(QWidget *parent)
    : QWidget(parent)
    , m_assistant(nullptr)
    , m_mainLayout(new QVBoxLayout(this))
    , m_stackedWidget(new QStackedWidget(this))
    , m_currentTask(AIWritingAssistant::TaskType::Completion)
    , m_isLoading(false)
{
    setupUi();
}

// 函数说明：设置 AIWritingPanel 的运行参数，并触发必要的界面或数据刷新。
void AIWritingPanel::setAssistant(AIWritingAssistant *assistant)
{
    if (m_assistant) {
        disconnect(m_assistant, nullptr, this, nullptr);
    }

    m_assistant = assistant;

    if (m_assistant) {
        connect(m_assistant, &AIWritingAssistant::completionChunkReceived,
                this, &AIWritingPanel::onCompletionChunk);
        connect(m_assistant, &AIWritingAssistant::completionFinished,
                this, &AIWritingPanel::onCompletionFinished);
        connect(m_assistant, &AIWritingAssistant::grammarCheckFinished,
                this, &AIWritingPanel::onGrammarCheckFinished);
        connect(m_assistant, &AIWritingAssistant::summaryFinished,
                this, &AIWritingPanel::onSummaryFinished);
        connect(m_assistant, &AIWritingAssistant::titleSuggestionsFinished,
                this, &AIWritingPanel::onTitleSuggestionsFinished);
        connect(m_assistant, &AIWritingAssistant::requestStarted,
                this, &AIWritingPanel::onRequestStarted);
        connect(m_assistant, &AIWritingAssistant::requestFinished,
                this, &AIWritingPanel::onRequestFinished);
        connect(m_assistant, &AIWritingAssistant::errorOccurred,
                this, &AIWritingPanel::onError);

        // 更新设置界面 - 添加空指针检查
        if (m_apiKeyEdit && m_modelCombo) {
            m_apiKeyEdit->setText(m_assistant->apiKey());
            int modelIndex = m_modelCombo->findText(m_assistant->model());
            if (modelIndex >= 0) {
                m_modelCombo->setCurrentIndex(modelIndex);
            }
        }
    }
}

// 函数说明：初始化 AIWritingPanel 的 setupUi 相关界面、动作或服务连接。
void AIWritingPanel::setupUi()
{
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(8);

    // 标题
    QLabel *titleLabel = new QLabel(tr("AI 智能写作助手"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(titleLabel);

    // 设置按钮
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->addStretch();
    QPushButton *settingsBtn = new QPushButton(tr("设置"), this);
    settingsBtn->setFlat(true);
    connect(settingsBtn, &QPushButton::clicked, this, &AIWritingPanel::onSettingsClicked);
    headerLayout->addWidget(settingsBtn);
    m_mainLayout->addLayout(headerLayout);

    // 设置页面
    setupToolsPage();
    setupResultPage();
    setupSettingsPage();

    m_stackedWidget->addWidget(m_toolsPage);
    m_stackedWidget->addWidget(m_resultPage);
    m_stackedWidget->addWidget(m_settingsPage);
    m_stackedWidget->setCurrentIndex(0);

    m_mainLayout->addWidget(m_stackedWidget, 1);
}

// 函数说明：初始化 AIWritingPanel 的 setupToolsPage 相关界面、动作或服务连接。
void AIWritingPanel::setupToolsPage()
{
    m_toolsPage = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(m_toolsPage);
    layout->setSpacing(12);

    // AI 补全
    QGroupBox *completionGroup = new QGroupBox(tr("AI 补全"), m_toolsPage);
    QVBoxLayout *completionLayout = new QVBoxLayout(completionGroup);
    QLabel *completionDesc = new QLabel(tr("智能续写当前段落，保持一致的写作风格"), completionGroup);
    completionDesc->setWordWrap(true);
    completionDesc->setStyleSheet("color: #6c757d;");
    completionLayout->addWidget(completionDesc);
    m_completionButton = new QPushButton(tr("开始补全"), completionGroup);
    m_completionButton->setStyleSheet(
        "QPushButton { padding: 10px; background-color: #007bff; color: white; border: none; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #0056b3; }"
        "QPushButton:pressed { background-color: #004085; }"
    );
    connect(m_completionButton, &QPushButton::clicked, this, &AIWritingPanel::onCompletionClicked);
    completionLayout->addWidget(m_completionButton);
    layout->addWidget(completionGroup);

    // 语法纠错
    QGroupBox *grammarGroup = new QGroupBox(tr("语法纠错"), m_toolsPage);
    QVBoxLayout *grammarLayout = new QVBoxLayout(grammarGroup);
    QLabel *grammarDesc = new QLabel(tr("AI 驱动的语法和风格检查，提供改进建议"), grammarGroup);
    grammarDesc->setWordWrap(true);
    grammarDesc->setStyleSheet("color: #6c757d;");
    grammarLayout->addWidget(grammarDesc);
    m_grammarCheckButton = new QPushButton(tr("检查语法"), grammarGroup);
    m_grammarCheckButton->setStyleSheet(
        "QPushButton { padding: 10px; background-color: #28a745; color: white; border: none; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #218838; }"
        "QPushButton:pressed { background-color: #1e7e34; }"
    );
    connect(m_grammarCheckButton, &QPushButton::clicked, this, &AIWritingPanel::onGrammarCheckClicked);
    grammarLayout->addWidget(m_grammarCheckButton);
    layout->addWidget(grammarGroup);

    // 自动摘要
    QGroupBox *summaryGroup = new QGroupBox(tr("自动摘要"), m_toolsPage);
    QVBoxLayout *summaryLayout = new QVBoxLayout(summaryGroup);
    QLabel *summaryDesc = new QLabel(tr("一键生成文档摘要，快速了解主要内容"), summaryGroup);
    summaryDesc->setWordWrap(true);
    summaryDesc->setStyleSheet("color: #6c757d;");
    summaryLayout->addWidget(summaryDesc);
    m_summaryButton = new QPushButton(tr("生成摘要"), summaryGroup);
    m_summaryButton->setStyleSheet(
        "QPushButton { padding: 10px; background-color: #17a2b8; color: white; border: none; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #138496; }"
        "QPushButton:pressed { background-color: #117a8b; }"
    );
    connect(m_summaryButton, &QPushButton::clicked, this, &AIWritingPanel::onSummaryClicked);
    summaryLayout->addWidget(m_summaryButton);
    layout->addWidget(summaryGroup);

    // 智能标题
    QGroupBox *titleGroup = new QGroupBox(tr("智能标题"), m_toolsPage);
    QVBoxLayout *titleLayout = new QVBoxLayout(titleGroup);
    QLabel *titleDesc = new QLabel(tr("基于文档内容自动生成标题建议"), titleGroup);
    titleDesc->setWordWrap(true);
    titleDesc->setStyleSheet("color: #6c757d;");
    titleLayout->addWidget(titleDesc);
    m_titleButton = new QPushButton(tr("生成标题"), titleGroup);
    m_titleButton->setStyleSheet(
        "QPushButton { padding: 10px; background-color: #ffc107; color: black; border: none; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #e0a800; }"
        "QPushButton:pressed { background-color: #d39e00; }"
    );
    connect(m_titleButton, &QPushButton::clicked, this, &AIWritingPanel::onTitleClicked);
    titleLayout->addWidget(m_titleButton);
    layout->addWidget(titleGroup);

    // 使用选中文本选项
    m_useSelectionCheck = new QCheckBox(tr("仅处理选中的文本"), m_toolsPage);
    m_useSelectionCheck->setToolTip(tr("勾选后，AI 补全和语法纠错将只处理选中的文本"));
    layout->addWidget(m_useSelectionCheck);

    layout->addStretch();
}

// 函数说明：初始化 AIWritingPanel 的 setupResultPage 相关界面、动作或服务连接。
void AIWritingPanel::setupResultPage()
{
    m_resultPage = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(m_resultPage);
    layout->setSpacing(8);

    // 任务标签
    m_taskLabel = new QLabel(m_resultPage);
    QFont taskFont = m_taskLabel->font();
    taskFont.setBold(true);
    m_taskLabel->setFont(taskFont);
    layout->addWidget(m_taskLabel);

    // 进度条
    m_progressBar = new QProgressBar(m_resultPage);
    m_progressBar->setRange(0, 0); // 不确定模式
    m_progressBar->hide();
    layout->addWidget(m_progressBar);

    // 结果区域（堆叠：文本/列表）
    m_resultStack = new QStackedWidget(m_resultPage);

    // 文本结果
    m_resultText = new QTextEdit(m_resultPage);
    m_resultText->setReadOnly(true);
    m_resultText->setStyleSheet(
        "QTextEdit { background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 4px; padding: 8px; }"
    );
    m_resultStack->addWidget(m_resultText);

    // 标题列表
    m_titleList = new QListWidget(m_resultPage);
    m_titleList->setStyleSheet(
        "QListWidget { background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 4px; }"
        "QListWidget::item { padding: 12px; border-bottom: 1px solid #e9ecef; }"
        "QListWidget::item:hover { background-color: #e9ecef; }"
        "QListWidget::item:selected { background-color: #007bff; color: white; }"
    );
    connect(m_titleList, &QListWidget::itemDoubleClicked,
            this, &AIWritingPanel::onTitleItemDoubleClicked);
    m_resultStack->addWidget(m_titleList);

    layout->addWidget(m_resultStack, 1);

    // 按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_backButton = new QPushButton(tr("返回"), m_resultPage);
    connect(m_backButton, &QPushButton::clicked, this, &AIWritingPanel::showToolsPage);
    buttonLayout->addWidget(m_backButton);

    m_cancelButton = new QPushButton(tr("取消"), m_resultPage);
    m_cancelButton->setStyleSheet(
        "QPushButton { padding: 8px 16px; background-color: #dc3545; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #c82333; }"
    );
    connect(m_cancelButton, &QPushButton::clicked, this, &AIWritingPanel::onCancelClicked);
    buttonLayout->addWidget(m_cancelButton);

    buttonLayout->addStretch();

    m_copyButton = new QPushButton(tr("复制"), m_resultPage);
    m_copyButton->setStyleSheet(
        "QPushButton { padding: 8px 16px; background-color: #6c757d; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #5a6268; }"
    );
    connect(m_copyButton, &QPushButton::clicked, this, &AIWritingPanel::onCopyClicked);
    buttonLayout->addWidget(m_copyButton);

    m_insertButton = new QPushButton(tr("插入"), m_resultPage);
    m_insertButton->setStyleSheet(
        "QPushButton { padding: 8px 16px; background-color: #28a745; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #218838; }"
    );
    connect(m_insertButton, &QPushButton::clicked, this, &AIWritingPanel::onInsertClicked);
    buttonLayout->addWidget(m_insertButton);

    layout->addLayout(buttonLayout);
}

// 函数说明：初始化 AIWritingPanel 的 setupSettingsPage 相关界面、动作或服务连接。
void AIWritingPanel::setupSettingsPage()
{
    m_settingsPage = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(m_settingsPage);
    layout->setSpacing(16);

    QLabel *settingsTitle = new QLabel(tr("AI 设置"), m_settingsPage);
    QFont titleFont = settingsTitle->font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    settingsTitle->setFont(titleFont);
    layout->addWidget(settingsTitle);

    // API Key
    QGroupBox *apiGroup = new QGroupBox(tr("API 配置"), m_settingsPage);
    QVBoxLayout *apiLayout = new QVBoxLayout(apiGroup);

    QLabel *apiKeyLabel = new QLabel(tr("阿里百炼 API Key:"), apiGroup);
    apiLayout->addWidget(apiKeyLabel);

    m_apiKeyEdit = new QLineEdit(apiGroup);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText(tr("sk-xxxxxxxxxxxxxxxx"));
    apiLayout->addWidget(m_apiKeyEdit);

    QLabel *modelLabel = new QLabel(tr("模型:"), apiGroup);
    apiLayout->addWidget(modelLabel);

    m_modelCombo = new QComboBox(apiGroup);
    m_modelCombo->addItems({
        "qwen-turbo",
        "qwen-plus",
        "qwen-max",
        "qwen-max-longcontext"
    });
    apiLayout->addWidget(m_modelCombo);

    layout->addWidget(apiGroup);

    // 提示信息
    QLabel *infoLabel = new QLabel(
        tr("提示：API Key 可从阿里云百炼平台获取。\n"
           "不同模型有不同的能力和价格，请根据需要选择。"),
        m_settingsPage
    );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #6c757d;");
    layout->addWidget(infoLabel);

    layout->addStretch();

    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_backFromSettingsButton = new QPushButton(tr("返回"), m_settingsPage);
    connect(m_backFromSettingsButton, &QPushButton::clicked, this, &AIWritingPanel::showToolsPage);
    buttonLayout->addWidget(m_backFromSettingsButton);

    buttonLayout->addStretch();

    m_saveSettingsButton = new QPushButton(tr("保存设置"), m_settingsPage);
    m_saveSettingsButton->setStyleSheet(
        "QPushButton { padding: 8px 16px; background-color: #28a745; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #218838; }"
    );
    connect(m_saveSettingsButton, &QPushButton::clicked, this, [this]() {
        if (m_assistant) {
            m_assistant->setApiKey(m_apiKeyEdit->text());
            m_assistant->setModel(m_modelCombo->currentText());
            m_assistant->saveSettings();
            QMessageBox::information(this, tr("设置已保存"), tr("AI 设置已保存"));
            showToolsPage();
        }
    });
    buttonLayout->addWidget(m_saveSettingsButton);

    layout->addLayout(buttonLayout);
}

// 函数说明：显示 AIWritingPanel 管理的面板、对话框或提示信息。
void AIWritingPanel::showResultPage()
{
    m_stackedWidget->setCurrentWidget(m_resultPage);
}

// 函数说明：显示 AIWritingPanel 管理的面板、对话框或提示信息。
void AIWritingPanel::showToolsPage()
{
    m_stackedWidget->setCurrentWidget(m_toolsPage);
}

// 函数说明：设置 AIWritingPanel 的运行参数，并触发必要的界面或数据刷新。
void AIWritingPanel::setLoading(bool loading)
{
    m_isLoading = loading;
    m_progressBar->setVisible(loading);
    m_cancelButton->setVisible(loading);
    m_backButton->setEnabled(!loading);
    m_insertButton->setEnabled(!loading);
    m_copyButton->setEnabled(!loading);
}

// 函数说明：清空 AIWritingPanel 保存的临时状态或缓存数据。
void AIWritingPanel::clearResult()
{
    m_resultText->clear();
    m_titleList->clear();
}

// 函数说明：响应 AIWritingPanel 收到的信号或异步回调，并更新界面状态。
void AIWritingPanel::onCompletionClicked()
{
    if (!m_assistant) {
        QMessageBox::warning(this, tr("错误"), tr("AI 助手未初始化"));
        return;
    }

    // 检查API Key是否配置
    if (!m_assistant->isApiKeyConfigured()) {
        QMessageBox::information(this, tr("需要配置 API Key"),
            tr("使用 AI 功能前，请先配置阿里百炼 API Key。\n\n"
               "1. 点击右上角的「设置」按钮\n"
               "2. 输入您的 API Key\n"
               "3. 选择合适的模型\n"
               "4. 点击「保存设置」\n\n"
               "API Key 可从阿里云百炼平台获取：\n"
               "https://bailian.console.aliyun.com/"));

        // 自动切换到设置页面
        m_stackedWidget->setCurrentWidget(m_settingsPage);
        return;
    }

    clearResult();
    m_currentTask = AIWritingAssistant::TaskType::Completion;
    m_taskLabel->setText(tr("AI 补全"));
    m_resultStack->setCurrentWidget(m_resultText);
    showResultPage();

    m_assistant->requestCompletion(m_useSelectionCheck->isChecked(), true);
}

// 函数说明：响应 AIWritingPanel 收到的信号或异步回调，并更新界面状态。
void AIWritingPanel::onGrammarCheckClicked()
{
    if (!m_assistant) {
        QMessageBox::warning(this, tr("错误"), tr("AI 助手未初始化"));
        return;
    }

    if (!m_assistant->isApiKeyConfigured()) {
        QMessageBox::information(this, tr("需要配置 API Key"),
            tr("使用 AI 功能前，请先在设置中配置阿里百炼 API Key。"));
        m_stackedWidget->setCurrentWidget(m_settingsPage);
        return;
    }

    clearResult();
    m_currentTask = AIWritingAssistant::TaskType::GrammarCheck;
    m_taskLabel->setText(tr("语法纠错"));
    m_resultStack->setCurrentWidget(m_resultText);
    showResultPage();

    m_assistant->requestGrammarCheck(m_useSelectionCheck->isChecked());
}

// 函数说明：响应 AIWritingPanel 收到的信号或异步回调，并更新界面状态。
void AIWritingPanel::onSummaryClicked()
{
    if (!m_assistant) {
        QMessageBox::warning(this, tr("错误"), tr("AI 助手未初始化"));
        return;
    }

    if (!m_assistant->isApiKeyConfigured()) {
        QMessageBox::information(this, tr("需要配置 API Key"),
            tr("使用 AI 功能前，请先在设置中配置阿里百炼 API Key。"));
        m_stackedWidget->setCurrentWidget(m_settingsPage);
        return;
    }

    clearResult();
    m_currentTask = AIWritingAssistant::TaskType::Summary;
    m_taskLabel->setText(tr("自动摘要"));
    m_resultStack->setCurrentWidget(m_resultText);
    showResultPage();

    m_assistant->requestSummary();
}

// 函数说明：响应 AIWritingPanel 收到的信号或异步回调，并更新界面状态。
void AIWritingPanel::onTitleClicked()
{
    if (!m_assistant) {
        QMessageBox::warning(this, tr("错误"), tr("AI 助手未初始化"));
        return;
    }

    if (!m_assistant->isApiKeyConfigured()) {
        QMessageBox::information(this, tr("需要配置 API Key"),
            tr("使用 AI 功能前，请先在设置中配置阿里百炼 API Key。"));
        m_stackedWidget->setCurrentWidget(m_settingsPage);
        return;
    }

    clearResult();
    m_currentTask = AIWritingAssistant::TaskType::TitleSuggestion;
    m_taskLabel->setText(tr("智能标题"));
    m_resultStack->setCurrentWidget(m_titleList);
    showResultPage();

    m_assistant->requestTitleSuggestions();
}

// 函数说明：响应 AIWritingPanel 收到的信号或异步回调，并更新界面状态。
void AIWritingPanel::onCancelClicked()
{
    if (m_assistant) {
        m_assistant->cancelRequest();
    }
    setLoading(false);
}

// 函数说明：响应 AIWritingPanel 收到的信号或异步回调，并更新界面状态。
void AIWritingPanel::onInsertClicked()
{
    if (m_currentTask == AIWritingAssistant::TaskType::TitleSuggestion) {
        QListWidgetItem *item = m_titleList->currentItem();
        if (item) {
            emit applyTitleRequested(item->text());
        }
    } else {
        QString text = m_resultText->toPlainText();
        if (!text.isEmpty()) {
            emit insertTextRequested(text);
        }
    }
}

// 函数说明：响应 AIWritingPanel 收到的信号或异步回调，并更新界面状态。
void AIWritingPanel::onCopyClicked()
{
    QString text;
    if (m_currentTask == AIWritingAssistant::TaskType::TitleSuggestion) {
        QListWidgetItem *item = m_titleList->currentItem();
        if (item) {
            text = item->text();
        }
    } else {
        text = m_resultText->toPlainText();
    }

    if (!text.isEmpty()) {
        QApplication::clipboard()->setText(text);
        QMessageBox::information(this, tr("已复制"), tr("内容已复制到剪贴板"));
    }
}

// 函数说明：响应 AIWritingPanel 收到的信号或异步回调，并更新界面状态。
void AIWritingPanel::onSettingsClicked()
{
    m_stackedWidget->setCurrentWidget(m_settingsPage);
}

// 函数说明：响应 AIWritingPanel 收到的信号或异步回调，并更新界面状态。
void AIWritingPanel::onCompletionChunk(const QString &chunk)
{
    m_resultText->moveCursor(QTextCursor::End);
    m_resultText->insertPlainText(chunk);
    m_resultText->verticalScrollBar()->setValue(
        m_resultText->verticalScrollBar()->maximum()
    );
}

// 函数说明：响应 AIWritingPanel 收到的信号或异步回调，并更新界面状态。
void AIWritingPanel::onCompletionFinished(const QString &content)
{
    // 流式模式下内容已经显示，非流式模式需要设置
    if (m_resultText->toPlainText().isEmpty()) {
        m_resultText->setPlainText(content);
    }
}

// 函数说明：响应 AIWritingPanel 收到的信号或异步回调，并更新界面状态。
void AIWritingPanel::onGrammarCheckFinished(const QString &suggestions)
{
    m_resultText->setPlainText(suggestions);
}

// 函数说明：响应 AIWritingPanel 收到的信号或异步回调，并更新界面状态。
void AIWritingPanel::onSummaryFinished(const QString &summary)
{
    m_resultText->setPlainText(summary);
}

// 函数说明：响应 AIWritingPanel 收到的信号或异步回调，并更新界面状态。
void AIWritingPanel::onTitleSuggestionsFinished(const QStringList &titles)
{
    m_titleList->clear();
    for (const QString &title : titles) {
        QListWidgetItem *item = new QListWidgetItem(title, m_titleList);
        item->setToolTip(tr("双击应用此标题"));
    }

    if (!titles.isEmpty()) {
        m_titleList->setCurrentRow(0);
    }
}

// 函数说明：响应 AIWritingPanel 收到的信号或异步回调，并更新界面状态。
void AIWritingPanel::onRequestStarted(AIWritingAssistant::TaskType task)
{
    Q_UNUSED(task);
    setLoading(true);
}

// 函数说明：响应 AIWritingPanel 收到的信号或异步回调，并更新界面状态。
void AIWritingPanel::onRequestFinished(AIWritingAssistant::TaskType task)
{
    Q_UNUSED(task);
    setLoading(false);
}

// 函数说明：响应 AIWritingPanel 收到的信号或异步回调，并更新界面状态。
void AIWritingPanel::onError(const QString &error)
{
    setLoading(false);
    QMessageBox::warning(this, tr("AI 错误"), error);
}

// 函数说明：响应 AIWritingPanel 收到的信号或异步回调，并更新界面状态。
void AIWritingPanel::onTitleItemDoubleClicked(QListWidgetItem *item)
{
    if (item) {
        emit applyTitleRequested(item->text());
    }
}

