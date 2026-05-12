// 文件说明：app-static\search\advancedsearchpanel.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "advancedsearchpanel.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QTabWidget>
#include <QFileInfo>
#include <QApplication>

// 函数说明：构造 AdvancedSearchPanel 对象，初始化本模块需要的状态、界面和资源。
AdvancedSearchPanel::AdvancedSearchPanel(QWidget *parent)
    : QWidget(parent)
    , m_searchManager(nullptr)
    , m_mainLayout(new QVBoxLayout(this))
    , m_tabWidget(new QTabWidget(this))
    , m_historyModel(new QStringListModel(this))
{
    setupUi();
}

// 函数说明：设置 AdvancedSearchPanel 的运行参数，并触发必要的界面或数据刷新。
void AdvancedSearchPanel::setSearchManager(SearchIndexManager *manager)
{
    if (m_searchManager) {
        disconnect(m_searchManager, nullptr, this, nullptr);
    }

    m_searchManager = manager;

    if (m_searchManager) {
        connect(m_searchManager, &SearchIndexManager::indexingStarted,
                this, &AdvancedSearchPanel::onIndexingStarted);
        connect(m_searchManager, &SearchIndexManager::indexingProgress,
                this, &AdvancedSearchPanel::onIndexingProgress);
        connect(m_searchManager, &SearchIndexManager::indexingFinished,
                this, &AdvancedSearchPanel::onIndexingFinished);
        connect(m_searchManager, &SearchIndexManager::searchCompleted,
                this, &AdvancedSearchPanel::onSearchCompleted);
        connect(m_searchManager, &SearchIndexManager::error,
                this, &AdvancedSearchPanel::onSearchError);

        // 更新历史
        m_historyModel->setStringList(m_searchManager->searchHistory());
        updateHistoryCompleter();

        // 更新路径列表
        m_pathList->clear();
        m_pathList->addItems(m_searchManager->searchPaths());

        // 更新索引状态
        m_indexStatusLabel->setText(tr("已索引 %1 个文档").arg(m_searchManager->indexedDocumentCount()));
    }
}

// 函数说明：初始化 AdvancedSearchPanel 的 setupUi 相关界面、动作或服务连接。
void AdvancedSearchPanel::setupUi()
{
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(8);

    // 标题
    QLabel *titleLabel = new QLabel(tr("高级搜索"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    m_mainLayout->addWidget(titleLabel);

    setupSearchBox();
    setupOptions();

    // 创建标签页
    QWidget *resultsPage = new QWidget(this);
    QVBoxLayout *resultsLayout = new QVBoxLayout(resultsPage);
    resultsLayout->setContentsMargins(0, 0, 0, 0);
    setupResultsView();
    resultsLayout->addWidget(m_resultCountLabel);
    resultsLayout->addWidget(m_progressBar);
    resultsLayout->addWidget(m_resultsTree, 1);
    m_tabWidget->addTab(resultsPage, tr("搜索结果"));

    QWidget *historyPage = new QWidget(this);
    QVBoxLayout *historyLayout = new QVBoxLayout(historyPage);
    historyLayout->setContentsMargins(0, 0, 0, 0);
    setupHistoryView();
    historyLayout->addWidget(m_historyList, 1);
    QHBoxLayout *historyBtnLayout = new QHBoxLayout();
    historyBtnLayout->addStretch();
    historyBtnLayout->addWidget(m_clearHistoryButton);
    historyLayout->addLayout(historyBtnLayout);
    m_tabWidget->addTab(historyPage, tr("搜索历史"));

    QWidget *settingsPage = new QWidget(this);
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsPage);
    settingsLayout->setContentsMargins(0, 0, 0, 0);
    setupIndexSettings();
    settingsLayout->addWidget(new QLabel(tr("搜索路径:"), settingsPage));
    settingsLayout->addWidget(m_pathList, 1);
    QHBoxLayout *pathBtnLayout = new QHBoxLayout();
    pathBtnLayout->addWidget(m_addPathButton);
    pathBtnLayout->addWidget(m_removePathButton);
    pathBtnLayout->addStretch();
    settingsLayout->addLayout(pathBtnLayout);
    settingsLayout->addWidget(m_indexStatusLabel);
    settingsLayout->addWidget(m_rebuildIndexButton);
    m_tabWidget->addTab(settingsPage, tr("索引设置"));

    m_mainLayout->addWidget(m_tabWidget, 1);
}

// 函数说明：初始化 AdvancedSearchPanel 的 setupSearchBox 相关界面、动作或服务连接。
void AdvancedSearchPanel::setupSearchBox()
{
    QHBoxLayout *searchLayout = new QHBoxLayout();

    m_searchInput = new QLineEdit(this);
    m_searchInput->setPlaceholderText(tr("输入搜索关键词..."));
    m_searchInput->setClearButtonEnabled(true);
    m_searchInput->setStyleSheet(
        "QLineEdit { padding: 8px; border: 1px solid #ced4da; border-radius: 4px; }"
        "QLineEdit:focus { border-color: #80bdff; }"
    );

    // 设置自动补全
    m_historyCompleter = new QCompleter(m_historyModel, this);
    m_historyCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_historyCompleter->setCompletionMode(QCompleter::PopupCompletion);
    m_searchInput->setCompleter(m_historyCompleter);

    connect(m_searchInput, &QLineEdit::textChanged,
            this, &AdvancedSearchPanel::onSearchTextChanged);
    connect(m_searchInput, &QLineEdit::returnPressed,
            this, &AdvancedSearchPanel::onSearchReturnPressed);

    searchLayout->addWidget(m_searchInput, 1);

    m_searchButton = new QPushButton(tr("搜索"), this);
    m_searchButton->setStyleSheet(
        "QPushButton { padding: 8px 16px; background-color: #007bff; color: white; border: none; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #0056b3; }"
        "QPushButton:pressed { background-color: #004085; }"
        "QPushButton:disabled { background-color: #6c757d; }"
    );
    connect(m_searchButton, &QPushButton::clicked,
            this, &AdvancedSearchPanel::onSearchClicked);
    searchLayout->addWidget(m_searchButton);

    m_mainLayout->addLayout(searchLayout);
}

// 函数说明：初始化 AdvancedSearchPanel 的 setupOptions 相关界面、动作或服务连接。
void AdvancedSearchPanel::setupOptions()
{
    QGroupBox *optionsGroup = new QGroupBox(tr("搜索选项"), this);
    QVBoxLayout *optionsLayout = new QVBoxLayout(optionsGroup);

    QHBoxLayout *row1 = new QHBoxLayout();
    m_caseSensitiveCheck = new QCheckBox(tr("区分大小写"), optionsGroup);
    m_wholeWordCheck = new QCheckBox(tr("全词匹配"), optionsGroup);
    row1->addWidget(m_caseSensitiveCheck);
    row1->addWidget(m_wholeWordCheck);
    row1->addStretch();
    optionsLayout->addLayout(row1);

    QHBoxLayout *row2 = new QHBoxLayout();
    m_regexCheck = new QCheckBox(tr("正则表达式"), optionsGroup);
    connect(m_regexCheck, &QCheckBox::toggled, this, [this](bool checked) {
        // 正则和模糊匹配互斥
        if (checked) {
            m_fuzzyMatchCheck->setChecked(false);
        }
        m_wholeWordCheck->setEnabled(!checked);
    });

    m_fuzzyMatchCheck = new QCheckBox(tr("模糊匹配"), optionsGroup);
    connect(m_fuzzyMatchCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked) {
            m_regexCheck->setChecked(false);
        }
        m_fuzzyToleranceSpin->setEnabled(checked);
        m_fuzzyToleranceLabel->setEnabled(checked);
    });

    row2->addWidget(m_regexCheck);
    row2->addWidget(m_fuzzyMatchCheck);
    row2->addStretch();
    optionsLayout->addLayout(row2);

    QHBoxLayout *row3 = new QHBoxLayout();
    m_fuzzyToleranceLabel = new QLabel(tr("容错字符数:"), optionsGroup);
    m_fuzzyToleranceLabel->setEnabled(false);
    m_fuzzyToleranceSpin = new QSpinBox(optionsGroup);
    m_fuzzyToleranceSpin->setRange(1, 5);
    m_fuzzyToleranceSpin->setValue(2);
    m_fuzzyToleranceSpin->setEnabled(false);
    m_fuzzyToleranceSpin->setToolTip(tr("允许的最大编辑距离（越大容错越多，但速度越慢）"));
    row3->addWidget(m_fuzzyToleranceLabel);
    row3->addWidget(m_fuzzyToleranceSpin);
    row3->addStretch();
    optionsLayout->addLayout(row3);

    m_mainLayout->addWidget(optionsGroup);
}

// 函数说明：初始化 AdvancedSearchPanel 的 setupResultsView 相关界面、动作或服务连接。
void AdvancedSearchPanel::setupResultsView()
{
    m_resultCountLabel = new QLabel(tr("输入关键词开始搜索"), this);
    m_resultCountLabel->setStyleSheet("color: #6c757d;");

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->hide();

    m_resultsTree = new QTreeWidget(this);
    m_resultsTree->setHeaderLabels({tr("文件"), tr("行"), tr("匹配内容")});
    m_resultsTree->setRootIsDecorated(true);
    m_resultsTree->setAlternatingRowColors(true);
    m_resultsTree->header()->setStretchLastSection(true);
    m_resultsTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_resultsTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_resultsTree->setStyleSheet(
        "QTreeWidget { border: 1px solid #dee2e6; border-radius: 4px; }"
        "QTreeWidget::item { padding: 4px; }"
        "QTreeWidget::item:hover { background-color: #e9ecef; }"
        "QTreeWidget::item:selected { background-color: #007bff; color: white; }"
    );

    connect(m_resultsTree, &QTreeWidget::itemClicked,
            this, &AdvancedSearchPanel::onResultItemClicked);
    connect(m_resultsTree, &QTreeWidget::itemDoubleClicked,
            this, &AdvancedSearchPanel::onResultItemDoubleClicked);
}

// 函数说明：初始化 AdvancedSearchPanel 的 setupHistoryView 相关界面、动作或服务连接。
void AdvancedSearchPanel::setupHistoryView()
{
    m_historyList = new QListWidget(this);
    m_historyList->setStyleSheet(
        "QListWidget { border: 1px solid #dee2e6; border-radius: 4px; }"
        "QListWidget::item { padding: 8px; }"
        "QListWidget::item:hover { background-color: #e9ecef; }"
    );
    connect(m_historyList, &QListWidget::itemClicked,
            this, &AdvancedSearchPanel::onHistoryItemClicked);

    m_clearHistoryButton = new QPushButton(tr("清除历史"), this);
    m_clearHistoryButton->setStyleSheet(
        "QPushButton { padding: 6px 12px; background-color: #dc3545; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #c82333; }"
    );
    connect(m_clearHistoryButton, &QPushButton::clicked,
            this, &AdvancedSearchPanel::onClearHistoryClicked);
}

// 函数说明：初始化 AdvancedSearchPanel 的 setupIndexSettings 相关界面、动作或服务连接。
void AdvancedSearchPanel::setupIndexSettings()
{
    m_pathList = new QListWidget(this);
    m_pathList->setStyleSheet(
        "QListWidget { border: 1px solid #dee2e6; border-radius: 4px; }"
        "QListWidget::item { padding: 6px; }"
    );

    m_addPathButton = new QPushButton(tr("添加路径"), this);
    m_addPathButton->setStyleSheet(
        "QPushButton { padding: 6px 12px; background-color: #28a745; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #218838; }"
    );
    connect(m_addPathButton, &QPushButton::clicked,
            this, &AdvancedSearchPanel::onAddPathClicked);

    m_removePathButton = new QPushButton(tr("移除路径"), this);
    m_removePathButton->setStyleSheet(
        "QPushButton { padding: 6px 12px; background-color: #dc3545; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #c82333; }"
    );
    connect(m_removePathButton, &QPushButton::clicked,
            this, &AdvancedSearchPanel::onRemovePathClicked);

    m_indexStatusLabel = new QLabel(tr("未建立索引"), this);
    m_indexStatusLabel->setStyleSheet("color: #6c757d;");

    m_rebuildIndexButton = new QPushButton(tr("重建索引"), this);
    m_rebuildIndexButton->setStyleSheet(
        "QPushButton { padding: 8px 16px; background-color: #17a2b8; color: white; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #138496; }"
    );
    connect(m_rebuildIndexButton, &QPushButton::clicked,
            this, &AdvancedSearchPanel::onRebuildIndexClicked);
}

// 函数说明：刷新 AdvancedSearchPanel 的内部状态，并同步到相关界面。
void AdvancedSearchPanel::updateHistoryCompleter()
{
    if (m_searchManager) {
        m_historyModel->setStringList(m_searchManager->searchHistory());

        // 更新历史列表
        m_historyList->clear();
        m_historyList->addItems(m_searchManager->searchHistory());
    }
}

// 函数说明：实现 AdvancedSearchPanel::focusSearchInput 的核心逻辑，供当前模块调用。
void AdvancedSearchPanel::focusSearchInput()
{
    m_searchInput->setFocus();
    m_searchInput->selectAll();
}

// 函数说明：显示 AdvancedSearchPanel 管理的面板、对话框或提示信息。
void AdvancedSearchPanel::showHistoryTab()
{
    // 切换到历史标签页 (索引为1)
    if (m_tabWidget) {
        m_tabWidget->setCurrentIndex(1);
    }
}

// 函数说明：清空 AdvancedSearchPanel 保存的临时状态或缓存数据。
void AdvancedSearchPanel::clearSearch()
{
    m_searchInput->clear();
    m_resultsTree->clear();
    m_resultCountLabel->setText(tr("输入关键词开始搜索"));
}

// 函数说明：响应 AdvancedSearchPanel 收到的信号或异步回调，并更新界面状态。
void AdvancedSearchPanel::onSearchClicked()
{
    QString query = m_searchInput->text().trimmed();
    if (query.isEmpty()) {
        return;
    }

    if (!m_searchManager) {
        QMessageBox::warning(this, tr("搜索"), tr("搜索管理器未初始化"));
        return;
    }

    // 添加到历史
    m_searchManager->addToHistory(query);
    updateHistoryCompleter();

    // 执行搜索
    m_resultsTree->clear();
    m_resultCountLabel->setText(tr("正在搜索..."));
    m_searchButton->setEnabled(false);
    QApplication::processEvents();

    SearchOptions options = buildSearchOptions();
    m_searchManager->search(query, options);
}

// 函数说明：响应 AdvancedSearchPanel 收到的信号或异步回调，并更新界面状态。
void AdvancedSearchPanel::onSearchTextChanged(const QString &text)
{
    Q_UNUSED(text);
    // 可以实现即时搜索
}

// 函数说明：响应 AdvancedSearchPanel 收到的信号或异步回调，并更新界面状态。
void AdvancedSearchPanel::onSearchReturnPressed()
{
    onSearchClicked();
}

// 函数说明：响应 AdvancedSearchPanel 收到的信号或异步回调，并更新界面状态。
void AdvancedSearchPanel::onResultItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    if (!item) return;

    // 如果是文件项（顶层项），展开/折叠
    if (!item->parent()) {
        item->setExpanded(!item->isExpanded());
        return;
    }

    // 如果是结果项，发送信号
    QString filePath = item->data(0, Qt::UserRole).toString();
    int lineNumber = item->data(0, Qt::UserRole + 1).toInt();
    int column_num = item->data(0, Qt::UserRole + 2).toInt();

    emit resultSelected(filePath, lineNumber, column_num);
}

// 函数说明：响应 AdvancedSearchPanel 收到的信号或异步回调，并更新界面状态。
void AdvancedSearchPanel::onResultItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    if (!item || !item->parent()) return;

    QString filePath = item->data(0, Qt::UserRole).toString();
    emit openFileRequested(filePath);
}

// 函数说明：响应 AdvancedSearchPanel 收到的信号或异步回调，并更新界面状态。
void AdvancedSearchPanel::onHistoryItemClicked(QListWidgetItem *item)
{
    if (!item) return;

    m_searchInput->setText(item->text());
    m_tabWidget->setCurrentIndex(0); // 切换到结果页
    onSearchClicked();
}

// 函数说明：响应 AdvancedSearchPanel 收到的信号或异步回调，并更新界面状态。
void AdvancedSearchPanel::onClearHistoryClicked()
{
    if (m_searchManager) {
        m_searchManager->clearHistory();
        updateHistoryCompleter();
    }
}

// 函数说明：响应 AdvancedSearchPanel 收到的信号或异步回调，并更新界面状态。
void AdvancedSearchPanel::onRebuildIndexClicked()
{
    if (!m_searchManager) return;

    if (m_searchManager->searchPaths().isEmpty()) {
        QMessageBox::warning(this, tr("索引"), tr("请先添加搜索路径"));
        return;
    }

    m_searchManager->rebuildIndex();
}

// 函数说明：响应 AdvancedSearchPanel 收到的信号或异步回调，并更新界面状态。
void AdvancedSearchPanel::onAddPathClicked()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("选择搜索目录"));
    if (path.isEmpty()) return;

    if (m_searchManager) {
        m_searchManager->addSearchPath(path);
        m_pathList->clear();
        m_pathList->addItems(m_searchManager->searchPaths());
    }
}

// 函数说明：响应 AdvancedSearchPanel 收到的信号或异步回调，并更新界面状态。
void AdvancedSearchPanel::onRemovePathClicked()
{
    QListWidgetItem *item = m_pathList->currentItem();
    if (!item) return;

    if (m_searchManager) {
        m_searchManager->removeSearchPath(item->text());
        m_pathList->clear();
        m_pathList->addItems(m_searchManager->searchPaths());
    }
}

// 函数说明：响应 AdvancedSearchPanel 收到的信号或异步回调，并更新界面状态。
void AdvancedSearchPanel::onIndexingStarted()
{
    m_progressBar->show();
    m_progressBar->setValue(0);
    m_indexStatusLabel->setText(tr("正在建立索引..."));
    m_rebuildIndexButton->setEnabled(false);
}

// 函数说明：响应 AdvancedSearchPanel 收到的信号或异步回调，并更新界面状态。
void AdvancedSearchPanel::onIndexingProgress(int current, int total, const QString &file)
{
    Q_UNUSED(file);
    if (total > 0) {
        m_progressBar->setValue(current * 100 / total);
    }
    m_indexStatusLabel->setText(tr("正在索引: %1 / %2").arg(current).arg(total));
}

// 函数说明：响应 AdvancedSearchPanel 收到的信号或异步回调，并更新界面状态。
void AdvancedSearchPanel::onIndexingFinished(int count)
{
    m_progressBar->hide();
    m_indexStatusLabel->setText(tr("已索引 %1 个文档").arg(count));
    m_rebuildIndexButton->setEnabled(true);
}

// 函数说明：响应 AdvancedSearchPanel 收到的信号或异步回调，并更新界面状态。
void AdvancedSearchPanel::onSearchCompleted(const QList<SearchResult> &results)
{
    m_searchButton->setEnabled(true);
    displayResults(results);
}

// 函数说明：响应 AdvancedSearchPanel 收到的信号或异步回调，并更新界面状态。
void AdvancedSearchPanel::onSearchError(const QString &error)
{
    m_searchButton->setEnabled(true);
    QMessageBox::warning(this, tr("搜索错误"), error);
}

// 函数说明：实现 AdvancedSearchPanel::displayResults 的核心逻辑，供当前模块调用。
void AdvancedSearchPanel::displayResults(const QList<SearchResult> &results)
{
    m_resultsTree->clear();

    if (results.isEmpty()) {
        m_resultCountLabel->setText(tr("没有找到匹配的结果"));
        return;
    }

    m_resultCountLabel->setText(tr("找到 %1 个匹配").arg(results.size()));

    // 按文件分组
    QMap<QString, QList<SearchResult>> groupedResults;
    for (const SearchResult &result : results) {
        groupedResults[result.filePath].append(result);
    }

    for (auto it = groupedResults.constBegin(); it != groupedResults.constEnd(); ++it) {
        const QString &filePath = it.key();
        const QList<SearchResult> &fileResults = it.value();

        // 创建文件项
        QFileInfo fi(filePath);
        QTreeWidgetItem *fileItem = new QTreeWidgetItem(m_resultsTree);
        fileItem->setText(0, fi.fileName());
        fileItem->setText(1, QString::number(fileResults.size()));
        fileItem->setText(2, fi.path());
        fileItem->setToolTip(0, filePath);
        fileItem->setExpanded(true);

        // 添加匹配项
        for (const SearchResult &result : fileResults) {
            QTreeWidgetItem *resultItem = new QTreeWidgetItem(fileItem);
            resultItem->setText(0, result.title);
            resultItem->setText(1, QString::number(result.lineNumber));

            QString snippet = result.snippet;
            if (result.isFuzzyMatch) {
                snippet = "[模糊] " + snippet;
            }
            resultItem->setText(2, snippet);

            resultItem->setData(0, Qt::UserRole, result.filePath);
            resultItem->setData(0, Qt::UserRole + 1, result.lineNumber);
            resultItem->setData(0, Qt::UserRole + 2, result.columnNumber);
            resultItem->setData(0, Qt::UserRole + 3, result.score);

            // 模糊匹配用不同颜色
            if (result.isFuzzyMatch) {
                resultItem->setForeground(2, QColor("#fd7e14"));
            }
        }
    }
}

// 函数说明：实现 AdvancedSearchPanel::buildSearchOptions 的核心逻辑，供当前模块调用。
SearchOptions AdvancedSearchPanel::buildSearchOptions()
{
    SearchOptions options;
    options.caseSensitive = m_caseSensitiveCheck->isChecked();
    options.wholeWord = m_wholeWordCheck->isChecked();
    options.useRegex = m_regexCheck->isChecked();
    options.fuzzyMatch = m_fuzzyMatchCheck->isChecked();
    options.fuzzyTolerance = m_fuzzyToleranceSpin->value();
    options.maxResults = 100;
    return options;
}

