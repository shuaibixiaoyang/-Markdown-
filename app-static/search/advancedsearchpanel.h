// 文件说明：app-static\search\advancedsearchpanel.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef ADVANCEDSEARCHPANEL_H
#define ADVANCEDSEARCHPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include <QTreeWidget>
#include <QProgressBar>
#include <QGroupBox>
#include <QSpinBox>
#include <QCompleter>
#include <QStringListModel>

#include "searchindexmanager.h"

/**
 * @brief 高级搜索面板
 *
 * 提供全文搜索、模糊匹配、正则表达式搜索的 UI 界面
 */
class AdvancedSearchPanel : public QWidget
{
    Q_OBJECT

public:
    explicit AdvancedSearchPanel(QWidget *parent = nullptr);
    ~AdvancedSearchPanel() = default;

    void setSearchManager(SearchIndexManager *manager);
    SearchIndexManager* searchManager() const { return m_searchManager; }

signals:
    void resultSelected(const QString &filePath, int lineNumber, int column);
    void openFileRequested(const QString &filePath);

public slots:
    void focusSearchInput();
    void clearSearch();
    void showHistoryTab();

private slots:
    void onSearchClicked();
    void onSearchTextChanged(const QString &text);
    void onSearchReturnPressed();
    void onResultItemClicked(QTreeWidgetItem *item, int column);
    void onResultItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onHistoryItemClicked(QListWidgetItem *item);
    void onClearHistoryClicked();
    void onRebuildIndexClicked();
    void onAddPathClicked();
    void onRemovePathClicked();

    void onIndexingStarted();
    void onIndexingProgress(int current, int total, const QString &file);
    void onIndexingFinished(int count);
    void onSearchCompleted(const QList<SearchResult> &results);
    void onSearchError(const QString &error);

private:
    void setupUi();
    void setupSearchBox();
    void setupOptions();
    void setupResultsView();
    void setupHistoryView();
    void setupIndexSettings();
    void updateHistoryCompleter();
    void displayResults(const QList<SearchResult> &results);
    SearchOptions buildSearchOptions();

    SearchIndexManager *m_searchManager;

    // 搜索框
    QLineEdit *m_searchInput;
    QPushButton *m_searchButton;
    QCompleter *m_historyCompleter;
    QStringListModel *m_historyModel;

    // 搜索选项
    QCheckBox *m_caseSensitiveCheck;
    QCheckBox *m_wholeWordCheck;
    QCheckBox *m_regexCheck;
    QCheckBox *m_fuzzyMatchCheck;
    QSpinBox *m_fuzzyToleranceSpin;
    QLabel *m_fuzzyToleranceLabel;

    // 结果视图
    QTreeWidget *m_resultsTree;
    QLabel *m_resultCountLabel;
    QProgressBar *m_progressBar;

    // 历史视图
    QListWidget *m_historyList;
    QPushButton *m_clearHistoryButton;

    // 索引设置
    QListWidget *m_pathList;
    QPushButton *m_addPathButton;
    QPushButton *m_removePathButton;
    QPushButton *m_rebuildIndexButton;
    QLabel *m_indexStatusLabel;

    // 布局
    QVBoxLayout *m_mainLayout;
    QTabWidget *m_tabWidget;
};

#endif // ADVANCEDSEARCHPANEL_H

