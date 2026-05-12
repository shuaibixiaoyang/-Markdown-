// 文件说明：app-static\datavisualization\datavisualizationpanel.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef DATAVISUALIZATIONPANEL_H
#define DATAVISUALIZATIONPANEL_H

#include <QWidget>
#include <QMap>
#include <QVector>

#include "datablockparser.h"
#include "databaseconnector.h"
#include "datachartwidget.h"
#include "variablemanager.h"

class QVBoxLayout;
class QHBoxLayout;
class QScrollArea;
class QPushButton;
class QLabel;
class QSplitter;
class QToolBar;
class QAction;
class QStackedWidget;
class QTimer;
class QNetworkAccessManager;

/**
 * @brief 数据可视化面板 - 文档即看板
 * 
 * 核心功能:
 * 1. 动态语法块解析和渲染
 * 2. 实时更新 (Live Refresh)
 * 3. 参数化控制 (全局变量)
 * 4. 悬停与下钻交互
 * 
 * 使用示例:
 * ```sql {render: "bar-chart", connection: "sales_db", title: "季度销售"}
 * SELECT category, SUM(amount) AS total 
 * FROM orders 
 * WHERE date > '$start_date'
 * GROUP BY category;
 * ```
 */
class DataVisualizationPanel : public QWidget
{
    Q_OBJECT

public:
    explicit DataVisualizationPanel(QWidget *parent = nullptr);
    ~DataVisualizationPanel();

    // 设置 Markdown 内容并解析数据块
    void setMarkdownContent(const QString &markdown);

    // 刷新所有数据块
    void refreshAll();

    // 刷新指定数据块
    void refreshBlock(const QString &blockId);

    // 获取数据块解析器
    DataBlockParser* parser() const { return m_parser; }

    // 获取变量管理器
    VariableManager* variableManager() const { return m_variableManager; }

    // 数据库连接管理
    void addDatabaseConnection(const DatabaseConnector::ConnectionConfig &config);
    bool hasDatabaseConnection(const QString &name) const;

    // 面板状态
    bool isVariablePanelVisible() const;
    void setVariablePanelVisible(bool visible);

public slots:
    // 工具栏动作
    void onRefreshAllClicked();
    void onToggleVariablesClicked();
    void onConfigureConnectionsClicked();
    void onExportAllClicked();

signals:
    // 数据块被点击，可用于跳转到编辑器对应位置
    void blockClicked(int startLine, int endLine);

    // 请求在编辑器中定位
    void requestLocateLine(int line);

    // 数据刷新完成
    void dataRefreshed();

    // 状态消息
    void statusMessage(const QString &message);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    // 处理查询完成
    void onQueryCompleted(const QString &blockId, const DatabaseConnector::QueryResult &result);

    // 处理图表刷新请求
    void onChartRefreshRequested(const QString &blockId);

    // 处理变量修改
    void onVariablesModified();

    // 处理数据点点击
    void onDataPointClicked(const QString &category, const QVariant &value, int rowIndex);

private:
    void setupUI();
    void createToolBar();
    void clearCharts();
    void executeDataBlocks();
    void executeDataBlock(const DataBlockParser::DataBlock &block);
    DataChartWidget* createChartWidget(const DataBlockParser::DataBlock &block);

private:
    // UI 组件
    QVBoxLayout *m_mainLayout;
    QToolBar *m_toolBar;
    QSplitter *m_splitter;
    QScrollArea *m_chartScrollArea;
    QWidget *m_chartContainer;
    QVBoxLayout *m_chartLayout;
    VariableEditorWidget *m_variableEditor;
    QLabel *m_statusLabel;
    QStackedWidget *m_stackedWidget;
    QWidget *m_emptyStateWidget;

    // 工具栏动作
    QAction *m_refreshAction;
    QAction *m_variablesAction;
    QAction *m_connectionsAction;
    QAction *m_exportAction;

    // 数据
    DataBlockParser *m_parser;
    VariableManager *m_variableManager;
    QString m_currentMarkdown;
    QVector<DataBlockParser::DataBlock> m_dataBlocks;
    QMap<QString, DataChartWidget*> m_chartWidgets;

    // 状态
    int m_pendingQueries;
    bool m_isRefreshing;

    // 防抖定时器
    QTimer *m_debounceTimer;
    static const int DEBOUNCE_DELAY_MS = 500;

    // 查询缓存
    QMap<QString, DatabaseConnector::QueryResult> m_queryCache;
    QMap<QString, qint64> m_cacheTimestamps;
    static const int CACHE_EXPIRE_MS = 30000; // 30秒缓存过期
    QNetworkAccessManager *m_networkManager;
};


/**
 * @brief 数据库连接配置对话框
 */
class QDialog;
class QLineEdit;
class QComboBox;
class QSpinBox;
class QListWidget;
class QDialogButtonBox;

class DatabaseConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DatabaseConnectionDialog(QWidget *parent = nullptr);

    void loadConnections();
    void saveConnections();

private slots:
    void onAddConnection();
    void onEditConnection();
    void onRemoveConnection();
    void onTestConnection();
    void onConnectionSelected();

private:
    void setupUI();
    void clearForm();
    void fillForm(const DatabaseConnector::ConnectionConfig &config);
    DatabaseConnector::ConnectionConfig getFormData() const;

    QListWidget *m_connectionList;
    QLineEdit *m_nameEdit;
    QComboBox *m_typeCombo;
    QLineEdit *m_hostEdit;
    QSpinBox *m_portSpin;
    QLineEdit *m_databaseEdit;
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QSpinBox *m_queryTimeoutSpin;
    QPushButton *m_addButton;
    QPushButton *m_editButton;
    QPushButton *m_removeButton;
    QPushButton *m_testButton;
    QDialogButtonBox *m_buttonBox;
};

#endif // DATAVISUALIZATIONPANEL_H

