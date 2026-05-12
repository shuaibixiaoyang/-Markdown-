// 文件说明：app-static\datavisualization\datavisualizationpanel.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "datavisualizationpanel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QLabel>
#include <QSplitter>
#include <QToolBar>
#include <QAction>
#include <QStackedWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <QTimer>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include <datalocation.h>

namespace {

QString connectionConfigPath()
{
    return QDir(DataLocation::writableLocation())
        .filePath(QStringLiteral("datavis-connections.json"));
}

QString queryCacheKey(const DataBlockParser::DataBlock &block,
                     const QString &sql,
                     const QVariantList &params)
{
    const QString paramJson = QString::fromUtf8(
        QJsonDocument(QJsonArray::fromVariantList(params)).toJson(QJsonDocument::Compact));
    return QStringLiteral("%1:%2:%3:%4").arg(block.connection, sql, paramJson, block.id);
}

int blockQueryTimeoutMs(const DataBlockParser::DataBlock &block)
{
    bool ok = false;
    const int timeout = block.options.value(QStringLiteral("timeout")).toString().toInt(&ok);
    if (ok && timeout >= DatabaseConnector::NoTimeoutMs) {
        return timeout;
    }
    return DatabaseConnector::instance().defaultQueryTimeoutMs();
}

DatabaseConnector::QueryResult parseJsonDocumentResult(const QJsonDocument &doc)
{
    DatabaseConnector::QueryResult result;
    result.success = true;
    result.affectedRows = 0;
    result.executionTimeMs = 0;

    if (doc.isArray()) {
        const QJsonArray array = doc.array();
        if (array.isEmpty()) {
            return result;
        }

        if (array.at(0).isObject()) {
            const QJsonObject firstObj = array.at(0).toObject();
            result.columns = firstObj.keys();

            for (const QJsonValue &value : array) {
                if (!value.isObject()) {
                    continue;
                }
                const QJsonObject object = value.toObject();
                QVector<QVariant> row;
                row.reserve(result.columns.size());
                for (const QString &column : result.columns) {
                    row.append(object.value(column).toVariant());
                }
                result.rows.append(row);
            }
            return result;
        }

        result.columns = QStringList() << QObject::tr("value");
        for (const QJsonValue &value : array) {
            QVector<QVariant> row;
            row.append(value.toVariant());
            result.rows.append(row);
        }
        return result;
    }

    if (doc.isObject()) {
        const QJsonObject object = doc.object();
        result.columns = object.keys();
        QVector<QVariant> row;
        row.reserve(result.columns.size());
        for (const QString &column : result.columns) {
            row.append(object.value(column).toVariant());
        }
        result.rows.append(row);
        return result;
    }

    result.success = false;
    result.errorMessage = QObject::tr("JSON 根节点必须是对象或数组");
    return result;
}

} // namespace

// ==================== DataVisualizationPanel ====================

DataVisualizationPanel::DataVisualizationPanel(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_toolBar(nullptr)
    , m_splitter(nullptr)
    , m_chartScrollArea(nullptr)
    , m_chartContainer(nullptr)
    , m_chartLayout(nullptr)
    , m_variableEditor(nullptr)
    , m_statusLabel(nullptr)
    , m_stackedWidget(nullptr)
    , m_emptyStateWidget(nullptr)
    , m_parser(new DataBlockParser(this))
    , m_variableManager(new VariableManager(this))
    , m_pendingQueries(0)
    , m_isRefreshing(false)
    , m_debounceTimer(new QTimer(this))
    , m_networkManager(new QNetworkAccessManager(this))
{
    setupUI();
    
    // 设置防抖定时器
    m_debounceTimer->setSingleShot(true);
    connect(m_debounceTimer, &QTimer::timeout, this, [this]() {
        executeDataBlocks();
    });
    
    // 连接数据库查询完成信号
    connect(&DatabaseConnector::instance(), &DatabaseConnector::queryCompleted,
            this, &DataVisualizationPanel::onQueryCompleted);
}

// 函数说明：销毁 DataVisualizationPanel 对象，释放本模块持有的资源。
DataVisualizationPanel::~DataVisualizationPanel()
{
    clearCharts();
}

// 函数说明：初始化 DataVisualizationPanel 的 setupUI 相关界面、动作或服务连接。
void DataVisualizationPanel::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // 工具栏
    createToolBar();
    m_mainLayout->addWidget(m_toolBar);

    // 使用 StackedWidget 切换空状态和内容状态
    m_stackedWidget = new QStackedWidget(this);

    // 空状态页面
    m_emptyStateWidget = new QWidget(this);
    QVBoxLayout *emptyLayout = new QVBoxLayout(m_emptyStateWidget);
    emptyLayout->setAlignment(Qt::AlignCenter);
    
    QLabel *emptyIcon = new QLabel("📊", this);
    emptyIcon->setStyleSheet("font-size: 48px;");
    emptyIcon->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyIcon);
    
    QLabel *emptyText = new QLabel(tr("没有可视化数据块"), this);
    emptyText->setStyleSheet("font-size: 16px; color: #666;");
    emptyText->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyText);
    
    QLabel *emptyHint = new QLabel(
        tr("在 Markdown 中添加带有 render 选项的代码块：\n"
           "```sql {render: \"bar-chart\", connection: \"mydb\"}\n"
           "SELECT category, COUNT(*) FROM table GROUP BY category;\n"
           "```"),
        this
    );
    emptyHint->setStyleSheet("font-size: 12px; color: #999; font-family: monospace;");
    emptyHint->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyHint);
    
    m_stackedWidget->addWidget(m_emptyStateWidget);

    // 内容页面
    m_splitter = new QSplitter(Qt::Horizontal, this);

    // 左侧: 图表滚动区域
    m_chartScrollArea = new QScrollArea(this);
    m_chartScrollArea->setWidgetResizable(true);
    m_chartScrollArea->setFrameShape(QFrame::NoFrame);
    m_chartScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_chartContainer = new QWidget(this);
    m_chartLayout = new QVBoxLayout(m_chartContainer);
    m_chartLayout->setContentsMargins(8, 8, 8, 8);
    m_chartLayout->setSpacing(16);
    m_chartLayout->addStretch();

    m_chartScrollArea->setWidget(m_chartContainer);
    m_splitter->addWidget(m_chartScrollArea);

    // 右侧: 变量编辑器
    m_variableEditor = new VariableEditorWidget(this);
    m_variableEditor->setVariableManager(m_variableManager);
    m_variableEditor->setMinimumWidth(200);
    m_variableEditor->setMaximumWidth(350);
    m_variableEditor->hide(); // 初始隐藏
    
    connect(m_variableEditor, &VariableEditorWidget::variablesModified,
            this, &DataVisualizationPanel::onVariablesModified);
    
    m_splitter->addWidget(m_variableEditor);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 0);

    m_stackedWidget->addWidget(m_splitter);
    m_mainLayout->addWidget(m_stackedWidget, 1);

    // 状态栏
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("padding: 4px 8px; color: #666; font-size: 11px;");
    m_mainLayout->addWidget(m_statusLabel);

    // 初始显示空状态
    m_stackedWidget->setCurrentWidget(m_emptyStateWidget);
}

// 函数说明：创建 DataVisualizationPanel 需要的对象、记录或输出内容。
void DataVisualizationPanel::createToolBar()
{
    m_toolBar = new QToolBar(this);
    m_toolBar->setMovable(false);
    m_toolBar->setIconSize(QSize(16, 16));
    m_toolBar->setStyleSheet("QToolBar { border-bottom: 1px solid #ddd; padding: 4px; }");

    // 刷新按钮
    m_refreshAction = new QAction(tr("刷新全部"), this);
    m_refreshAction->setToolTip(tr("刷新所有数据图表 (F5)"));
    m_refreshAction->setShortcut(QKeySequence(Qt::Key_F5));
    connect(m_refreshAction, &QAction::triggered, this, &DataVisualizationPanel::onRefreshAllClicked);
    m_toolBar->addAction(m_refreshAction);

    m_toolBar->addSeparator();

    // 变量面板切换
    m_variablesAction = new QAction(tr("变量"), this);
    m_variablesAction->setToolTip(tr("显示/隐藏全局变量面板"));
    m_variablesAction->setCheckable(true);
    connect(m_variablesAction, &QAction::triggered, this, &DataVisualizationPanel::onToggleVariablesClicked);
    m_toolBar->addAction(m_variablesAction);

    // 数据库连接配置
    m_connectionsAction = new QAction(tr("连接"), this);
    m_connectionsAction->setToolTip(tr("配置数据库连接"));
    connect(m_connectionsAction, &QAction::triggered, this, &DataVisualizationPanel::onConfigureConnectionsClicked);
    m_toolBar->addAction(m_connectionsAction);

    m_toolBar->addSeparator();

    // 导出
    m_exportAction = new QAction(tr("导出"), this);
    m_exportAction->setToolTip(tr("导出所有数据"));
    connect(m_exportAction, &QAction::triggered, this, &DataVisualizationPanel::onExportAllClicked);
    m_toolBar->addAction(m_exportAction);

    // 弹性空间
    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toolBar->addWidget(spacer);

    // 帮助标签
    QLabel *helpLabel = new QLabel(tr("数据可视化面板"), this);
    helpLabel->setStyleSheet("color: #888; font-size: 11px;");
    m_toolBar->addWidget(helpLabel);
}

// 函数说明：设置 DataVisualizationPanel 的运行参数，并触发必要的界面或数据刷新。
void DataVisualizationPanel::setMarkdownContent(const QString &markdown)
{
    m_currentMarkdown = markdown;
    
    // 解析全局变量
    m_variableManager->parseFromMarkdown(markdown);
    m_variableEditor->refresh();
    
    // 解析数据块
    QVector<DataBlockParser::DataBlock> newBlocks = m_parser->parseDocument(markdown);
    
    // 检查数据块是否有变化
    bool blocksChanged = (newBlocks.size() != m_dataBlocks.size());
    if (!blocksChanged) {
        for (int i = 0; i < newBlocks.size(); ++i) {
            if (newBlocks[i].code != m_dataBlocks[i].code ||
                newBlocks[i].renderType != m_dataBlocks[i].renderType ||
                newBlocks[i].connection != m_dataBlocks[i].connection) {
                blocksChanged = true;
                break;
            }
        }
    }
    
    m_dataBlocks = newBlocks;
    
    if (m_dataBlocks.isEmpty()) {
        clearCharts();
        m_stackedWidget->setCurrentWidget(m_emptyStateWidget);
        m_statusLabel->setText(tr("没有检测到数据块"));
        m_debounceTimer->stop();
        return;
    }
    
    // 如果数据块结构变化，重建图表控件
    if (blocksChanged || m_chartWidgets.isEmpty()) {
        clearCharts();
        
        m_stackedWidget->setCurrentWidget(m_splitter);
        
        // 为每个数据块创建图表控件
        for (const DataBlockParser::DataBlock &block : m_dataBlocks) {
            DataChartWidget *chartWidget = createChartWidget(block);
            
            // 插入到布局中（在 stretch 之前）
            m_chartLayout->insertWidget(m_chartLayout->count() - 1, chartWidget);
            m_chartWidgets[block.id] = chartWidget;
        }
    }
    
    m_statusLabel->setText(tr("检测到 %1 个数据块").arg(m_dataBlocks.size()));
    
    // 使用防抖执行数据查询
    m_debounceTimer->start(DEBOUNCE_DELAY_MS);
}

// 函数说明：实现 DataVisualizationPanel::refreshAll 的核心逻辑，供当前模块调用。
void DataVisualizationPanel::refreshAll()
{
    if (m_isRefreshing) return;
    
    m_isRefreshing = true;
    m_statusLabel->setText(tr("正在刷新..."));
    
    // 重新解析变量
    m_variableManager->parseFromMarkdown(m_currentMarkdown);
    m_variableEditor->refresh();
    
    // 重新执行所有查询
    executeDataBlocks();
}

// 函数说明：实现 DataVisualizationPanel::refreshBlock 的核心逻辑，供当前模块调用。
void DataVisualizationPanel::refreshBlock(const QString &blockId)
{
    for (const DataBlockParser::DataBlock &block : m_dataBlocks) {
        if (block.id == blockId) {
            if (m_chartWidgets.contains(blockId)) {
                m_chartWidgets[blockId]->showLoading();
            }
            m_isRefreshing = true;
            m_pendingQueries = 1;
            m_statusLabel->setText(tr("正在刷新图表..."));
            executeDataBlock(block);
            break;
        }
    }
}

// 函数说明：向 DataVisualizationPanel 管理的数据集合中添加一项内容。
void DataVisualizationPanel::addDatabaseConnection(const DatabaseConnector::ConnectionConfig &config)
{
    DatabaseConnector::instance().addConnection(config);
}

// 函数说明：检查 DataVisualizationPanel 是否具备对应的数据或能力。
bool DataVisualizationPanel::hasDatabaseConnection(const QString &name) const
{
    return DatabaseConnector::instance().hasConnection(name);
}

// 函数说明：判断 DataVisualizationPanel 当前是否满足指定状态。
bool DataVisualizationPanel::isVariablePanelVisible() const
{
    return m_variableEditor->isVisible();
}

// 函数说明：设置 DataVisualizationPanel 的运行参数，并触发必要的界面或数据刷新。
void DataVisualizationPanel::setVariablePanelVisible(bool visible)
{
    m_variableEditor->setVisible(visible);
    m_variablesAction->setChecked(visible);
}

// 函数说明：响应 DataVisualizationPanel 收到的信号或异步回调，并更新界面状态。
void DataVisualizationPanel::onRefreshAllClicked()
{
    refreshAll();
}

// 函数说明：响应 DataVisualizationPanel 收到的信号或异步回调，并更新界面状态。
void DataVisualizationPanel::onToggleVariablesClicked()
{
    setVariablePanelVisible(!m_variableEditor->isVisible());
}

// 函数说明：响应 DataVisualizationPanel 收到的信号或异步回调，并更新界面状态。
void DataVisualizationPanel::onConfigureConnectionsClicked()
{
    DatabaseConnectionDialog dialog(this);
    dialog.loadConnections();
    
    if (dialog.exec() == QDialog::Accepted) {
        dialog.saveConnections();
        emit statusMessage(tr("数据库连接已更新"));
    }
}

// 函数说明：响应 DataVisualizationPanel 收到的信号或异步回调，并更新界面状态。
void DataVisualizationPanel::onExportAllClicked()
{
    QString dirPath = QFileDialog::getExistingDirectory(
        this,
        tr("选择导出目录"),
        QString()
    );

    if (dirPath.isEmpty()) return;

    // 批量导出所有图表数据
    int exportedCount = 0;
    QStringList errors;

    for (auto it = m_chartWidgets.begin(); it != m_chartWidgets.end(); ++it) {
        DataChartWidget *chartWidget = it.value();
        const DataBlockParser::DataBlock &block = chartWidget->dataBlock();

        // 生成文件名
        QString fileName = block.title.isEmpty() ?
            QString("chart_%1").arg(block.id) : block.title;
        // 清理文件名中的非法字符
        fileName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");

        // 获取查询结果
        QString cacheKey;
        for (auto cacheIt = m_queryCache.begin(); cacheIt != m_queryCache.end(); ++cacheIt) {
            if (cacheIt.key().contains(block.id)) {
                const DatabaseConnector::QueryResult &result = cacheIt.value();

                if (!result.success || result.rows.isEmpty()) {
                    continue;
                }

                // 导出为 CSV
                QString csvPath = QString("%1/%2.csv").arg(dirPath).arg(fileName);
                QFile csvFile(csvPath);

                if (csvFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream stream(&csvFile);

                    // 写入表头
                    stream << result.columns.join(",") << "\n";

                    // 写入数据行
                    for (const QVector<QVariant> &row : result.rows) {
                        QStringList rowData;
                        for (const QVariant &cell : row) {
                            QString cellStr = cell.toString();
                            // 处理包含逗号或引号的单元格
                            if (cellStr.contains(',') || cellStr.contains('"') || cellStr.contains('\n')) {
                                cellStr.replace("\"", "\"\"");
                                cellStr = QString("\"%1\"").arg(cellStr);
                            }
                            rowData << cellStr;
                        }
                        stream << rowData.join(",") << "\n";
                    }

                    csvFile.close();
                    exportedCount++;
                } else {
                    errors << tr("无法写入文件: %1").arg(csvPath);
                }
                break;
            }
        }
    }

    // 显示结果
    if (errors.isEmpty()) {
        emit statusMessage(tr("成功导出 %1 个图表数据到 %2").arg(exportedCount).arg(dirPath));
    } else {
        emit statusMessage(tr("导出完成，%1 个成功，%2 个失败").arg(exportedCount).arg(errors.size()));
    }
}

// 函数说明：响应尺寸变化，重新计算 DataVisualizationPanel 的布局。
void DataVisualizationPanel::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

// 函数说明：响应 DataVisualizationPanel 收到的信号或异步回调，并更新界面状态。
void DataVisualizationPanel::onQueryCompleted(const QString &blockId, 
                                              const DatabaseConnector::QueryResult &result)
{
    if (m_pendingQueries > 0) {
        m_pendingQueries--;
    }
    
    // 保存到缓存
    if (result.success) {
        // 找到对应的数据块来生成缓存键
        for (const DataBlockParser::DataBlock &block : m_dataBlocks) {
            if (block.id == blockId) {
                QString cacheKey = block.id;
                if (block.sourceType == DataBlockParser::SourceType::SQL) {
                    QVariantList params;
                    QString sql = m_variableManager->substituteForSQL(block.code, &params);
                    cacheKey = queryCacheKey(block, sql, params);
                }
                m_queryCache[cacheKey] = result;
                m_cacheTimestamps[cacheKey] = QDateTime::currentMSecsSinceEpoch();
                break;
            }
        }
    }
    
    if (m_chartWidgets.contains(blockId)) {
        m_chartWidgets[blockId]->setData(result);
    }
    
    if (m_pendingQueries <= 0) {
        m_isRefreshing = false;
        m_statusLabel->setText(tr("数据已更新 (%1 个图表)")
            .arg(m_chartWidgets.size()));
        emit dataRefreshed();
    }
}

// 函数说明：响应 DataVisualizationPanel 收到的信号或异步回调，并更新界面状态。
void DataVisualizationPanel::onChartRefreshRequested(const QString &blockId)
{
    refreshBlock(blockId);
}

// 函数说明：响应 DataVisualizationPanel 收到的信号或异步回调，并更新界面状态。
void DataVisualizationPanel::onVariablesModified()
{
    // 变量修改后刷新所有图表
    refreshAll();
}

// 函数说明：响应 DataVisualizationPanel 收到的信号或异步回调，并更新界面状态。
void DataVisualizationPanel::onDataPointClicked(const QString &category, 
                                                 const QVariant &value, 
                                                 int rowIndex)
{
    DataChartWidget *chart = qobject_cast<DataChartWidget*>(sender());
    if (chart) {
        DataBlockParser::DataBlock block = chart->dataBlock();
        emit blockClicked(block.startLine, block.endLine);
    }
}

// 函数说明：清空 DataVisualizationPanel 保存的临时状态或缓存数据。
void DataVisualizationPanel::clearCharts()
{
    for (DataChartWidget *widget : m_chartWidgets) {
        m_chartLayout->removeWidget(widget);
        widget->deleteLater();
    }
    m_chartWidgets.clear();
}

// 函数说明：实现 DataVisualizationPanel::executeDataBlocks 的核心逻辑，供当前模块调用。
void DataVisualizationPanel::executeDataBlocks()
{
    m_pendingQueries = m_dataBlocks.size();
    if (m_pendingQueries <= 0) {
        m_isRefreshing = false;
        m_statusLabel->setText(tr("没有可执行的数据块"));
        emit dataRefreshed();
        return;
    }

    for (const DataBlockParser::DataBlock &block : m_dataBlocks) {
        if (m_chartWidgets.contains(block.id)) {
            m_chartWidgets[block.id]->showLoading();
        }
        executeDataBlock(block);
    }
}

// 函数说明：实现 DataVisualizationPanel::executeDataBlock 的核心逻辑，供当前模块调用。
void DataVisualizationPanel::executeDataBlock(const DataBlockParser::DataBlock &block)
{
    if (block.sourceType == DataBlockParser::SourceType::SQL) {
        QVariantList params;
        QString sql = m_variableManager->substituteForSQL(block.code, &params);

        // 生成缓存键
        const QString cacheKey = queryCacheKey(block, sql, params);

        // 检查缓存
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (m_queryCache.contains(cacheKey) && m_cacheTimestamps.contains(cacheKey)) {
            const qint64 cacheAge = now - m_cacheTimestamps[cacheKey];
            if (cacheAge < CACHE_EXPIRE_MS) {
                onQueryCompleted(block.id, m_queryCache.value(cacheKey));
                return;
            }
        }

        DatabaseConnector::instance().executeQueryAsync(
            block.connection,
            sql,
            params,
            block.id,
            blockQueryTimeoutMs(block));
        return;
    }

    if (block.sourceType == DataBlockParser::SourceType::JSON) {
        DatabaseConnector::QueryResult result;
        QString jsonStr = m_variableManager->substituteVariables(block.code);
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            result.success = false;
            result.affectedRows = 0;
            result.executionTimeMs = 0;
            result.errorMessage = tr("JSON 解析错误: %1").arg(parseError.errorString());
        } else {
            result = parseJsonDocumentResult(doc);
        }
        onQueryCompleted(block.id, result);
        return;
    }

    if (block.sourceType == DataBlockParser::SourceType::CSV) {
        DatabaseConnector::QueryResult result;
        result.success = true;
        result.affectedRows = 0;
        result.executionTimeMs = 0;

        QString csvStr = m_variableManager->substituteVariables(block.code);
        QStringList lines = csvStr.split('\n', Qt::SkipEmptyParts);
        
        if (!lines.isEmpty()) {
            // 第一行是列头
            result.columns = lines[0].split(',');
            for (QString &col : result.columns) {
                col = col.trimmed().remove('"');
            }
            
            // 剩余行是数据
            for (int i = 1; i < lines.size(); ++i) {
                QStringList values = lines[i].split(',');
                QVector<QVariant> row;
                for (const QString &val : values) {
                    row.append(val.trimmed().remove('"'));
                }
                result.rows.append(row);
            }
        }
        onQueryCompleted(block.id, result);
        return;
    }

    if (block.sourceType == DataBlockParser::SourceType::API) {
        QString source = m_variableManager->substituteVariables(
            block.options.value(QStringLiteral("source")).toString()).trimmed();
        if (source.startsWith(QStringLiteral("api://"))) {
            source = source.mid(6);
        }

        const QUrl url(source);
        if (!url.isValid() || url.scheme().isEmpty()) {
            DatabaseConnector::QueryResult result;
            result.success = false;
            result.affectedRows = 0;
            result.executionTimeMs = 0;
            result.errorMessage = tr("API 数据源无效: %1").arg(source);
            onQueryCompleted(block.id, result);
            return;
        }

        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
        request.setTransferTimeout(blockQueryTimeoutMs(block));

        const QString blockId = block.id;
        const qint64 startMs = QDateTime::currentMSecsSinceEpoch();
        QNetworkReply *reply = m_networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, blockId, startMs]() {
            DatabaseConnector::QueryResult result;
            result.success = false;
            result.affectedRows = 0;
            result.executionTimeMs = QDateTime::currentMSecsSinceEpoch() - startMs;

            if (reply->error() != QNetworkReply::NoError) {
                result.errorMessage = tr("API 请求失败: %1").arg(reply->errorString());
            } else {
                QJsonParseError parseError;
                const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
                if (parseError.error != QJsonParseError::NoError) {
                    result.errorMessage = tr("API 返回 JSON 解析错误: %1")
                        .arg(parseError.errorString());
                } else {
                    result = parseJsonDocumentResult(doc);
                    result.executionTimeMs = QDateTime::currentMSecsSinceEpoch() - startMs;
                }
            }

            reply->deleteLater();
            onQueryCompleted(blockId, result);
        });
        return;
    }

    DatabaseConnector::QueryResult unsupportedResult;
    unsupportedResult.success = false;
    unsupportedResult.affectedRows = 0;
    unsupportedResult.executionTimeMs = 0;
    unsupportedResult.errorMessage = tr("不支持的数据源类型");
    onQueryCompleted(block.id, unsupportedResult);
}

// 函数说明：创建 DataVisualizationPanel 需要的对象、记录或输出内容。
DataChartWidget* DataVisualizationPanel::createChartWidget(const DataBlockParser::DataBlock &block)
{
    DataChartWidget *widget = new DataChartWidget(this);
    widget->setDataBlock(block);
    widget->setMinimumHeight(350);
    widget->setMaximumHeight(500);
    
    // 设置标题
    if (!block.title.isEmpty()) {
        widget->setTitle(block.title);
    } else {
        widget->setTitle(DataBlockParser::renderTypeToString(block.renderType));
    }
    
    // 连接信号
    connect(widget, &DataChartWidget::refreshRequested,
            this, &DataVisualizationPanel::onChartRefreshRequested);
    connect(widget, &DataChartWidget::dataPointClicked,
            this, &DataVisualizationPanel::onDataPointClicked);
    
    widget->showLoading();
    
    return widget;
}


// ==================== DatabaseConnectionDialog ====================

DatabaseConnectionDialog::DatabaseConnectionDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
}

// 函数说明：初始化 DatabaseConnectionDialog 的 setupUI 相关界面、动作或服务连接。
void DatabaseConnectionDialog::setupUI()
{
    setWindowTitle(tr("数据库连接管理"));
    setMinimumSize(600, 400);
    resize(700, 450);

    QHBoxLayout *mainLayout = new QHBoxLayout();

    // 左侧: 连接列表
    QVBoxLayout *leftLayout = new QVBoxLayout();
    
    QLabel *listLabel = new QLabel(tr("连接列表"), this);
    listLabel->setStyleSheet("font-weight: bold;");
    leftLayout->addWidget(listLabel);
    
    m_connectionList = new QListWidget(this);
    m_connectionList->setMinimumWidth(180);
    connect(m_connectionList, &QListWidget::currentRowChanged,
            this, &DatabaseConnectionDialog::onConnectionSelected);
    leftLayout->addWidget(m_connectionList);
    
    QHBoxLayout *listButtonLayout = new QHBoxLayout();
    m_addButton = new QPushButton(tr("添加"), this);
    connect(m_addButton, &QPushButton::clicked, this, &DatabaseConnectionDialog::onAddConnection);
    listButtonLayout->addWidget(m_addButton);
    
    m_removeButton = new QPushButton(tr("删除"), this);
    connect(m_removeButton, &QPushButton::clicked, this, &DatabaseConnectionDialog::onRemoveConnection);
    listButtonLayout->addWidget(m_removeButton);
    leftLayout->addLayout(listButtonLayout);
    
    mainLayout->addLayout(leftLayout);

    // 右侧: 配置表单
    QGroupBox *formGroup = new QGroupBox(tr("连接配置"), this);
    QFormLayout *formLayout = new QFormLayout(formGroup);

    m_nameEdit = new QLineEdit(this);
    formLayout->addRow(tr("名称:"), m_nameEdit);

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItems({"SQLite", "MySQL", "PostgreSQL", "ODBC"});
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        bool needsHost = (index != 0); // SQLite 不需要主机
        m_hostEdit->setEnabled(needsHost);
        m_portSpin->setEnabled(needsHost);
        m_usernameEdit->setEnabled(needsHost);
        m_passwordEdit->setEnabled(needsHost);
    });
    formLayout->addRow(tr("类型:"), m_typeCombo);

    m_hostEdit = new QLineEdit(this);
    m_hostEdit->setPlaceholderText("localhost");
    formLayout->addRow(tr("主机:"), m_hostEdit);

    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(3306);
    formLayout->addRow(tr("端口:"), m_portSpin);

    m_databaseEdit = new QLineEdit(this);
    m_databaseEdit->setPlaceholderText(tr("数据库名或文件路径"));
    formLayout->addRow(tr("数据库:"), m_databaseEdit);

    m_usernameEdit = new QLineEdit(this);
    formLayout->addRow(tr("用户名:"), m_usernameEdit);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    formLayout->addRow(tr("密码:"), m_passwordEdit);

    m_queryTimeoutSpin = new QSpinBox(this);
    m_queryTimeoutSpin->setRange(0, 600000);
    m_queryTimeoutSpin->setSingleStep(1000);
    m_queryTimeoutSpin->setSuffix(QStringLiteral(" ms"));
    formLayout->addRow(tr("查询超时(0=不限):"), m_queryTimeoutSpin);

    // 测试和保存按钮
    QHBoxLayout *actionLayout = new QHBoxLayout();
    m_testButton = new QPushButton(tr("测试连接"), this);
    connect(m_testButton, &QPushButton::clicked, this, &DatabaseConnectionDialog::onTestConnection);
    actionLayout->addWidget(m_testButton);
    
    m_editButton = new QPushButton(tr("保存修改"), this);
    connect(m_editButton, &QPushButton::clicked, this, &DatabaseConnectionDialog::onEditConnection);
    actionLayout->addWidget(m_editButton);
    formLayout->addRow(actionLayout);

    mainLayout->addWidget(formGroup, 1);

    // 对话框按钮
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    QVBoxLayout *wrapperLayout = new QVBoxLayout();
    wrapperLayout->addLayout(mainLayout);
    wrapperLayout->addWidget(m_buttonBox);
    setLayout(wrapperLayout);

    // 初始状态
    m_typeCombo->setCurrentIndex(0);
}

// 函数说明：加载 DatabaseConnectionDialog 需要的数据、配置或外部资源。
void DatabaseConnectionDialog::loadConnections()
{
    const QString configPath = connectionConfigPath();
    const QFileInfo configFileInfo(configPath);
    if (!DatabaseConnector::instance().loadConnections(configPath)
        && configFileInfo.exists()) {
        QMessageBox::warning(this, tr("加载失败"),
            tr("无法加载连接配置文件:\n%1").arg(configPath));
    }

    m_queryTimeoutSpin->setValue(DatabaseConnector::instance().defaultQueryTimeoutMs());
    m_connectionList->clear();
    
    QStringList names = DatabaseConnector::instance().connectionNames();
    for (const QString &name : names) {
        m_connectionList->addItem(name);
    }
    
    if (m_connectionList->count() > 0) {
        m_connectionList->setCurrentRow(0);
    } else {
        clearForm();
    }
}

// 函数说明：保存 DatabaseConnectionDialog 当前状态，保证用户修改可以持久化。
void DatabaseConnectionDialog::saveConnections()
{
    DatabaseConnector::instance().setDefaultQueryTimeoutMs(m_queryTimeoutSpin->value());

    const QString configPath = connectionConfigPath();
    if (!DatabaseConnector::instance().saveConnections(configPath)) {
        QMessageBox::warning(this, tr("保存失败"),
            tr("无法保存连接配置文件:\n%1").arg(configPath));
    }
}

// 函数说明：响应 DatabaseConnectionDialog 收到的信号或异步回调，并更新界面状态。
void DatabaseConnectionDialog::onAddConnection()
{
    QString baseName = "new_connection";
    QString name = baseName;
    int suffix = 1;
    
    while (DatabaseConnector::instance().hasConnection(name)) {
        name = QString("%1_%2").arg(baseName).arg(suffix++);
    }
    
    DatabaseConnector::ConnectionConfig config;
    config.name = name;
    config.type = DatabaseConnector::DatabaseType::SQLite;
    config.port = 0;
    
    DatabaseConnector::instance().addConnection(config);
    
    m_connectionList->addItem(name);
    m_connectionList->setCurrentRow(m_connectionList->count() - 1);
}

// 函数说明：响应 DatabaseConnectionDialog 收到的信号或异步回调，并更新界面状态。
void DatabaseConnectionDialog::onEditConnection()
{
    if (m_connectionList->currentRow() < 0) return;
    
    QString oldName = m_connectionList->currentItem()->text();
    DatabaseConnector::ConnectionConfig config = getFormData();
    
    // 如果名称改变，需要删除旧的
    if (oldName != config.name) {
        DatabaseConnector::instance().removeConnection(oldName);
        m_connectionList->currentItem()->setText(config.name);
    }
    
    DatabaseConnector::instance().addConnection(config);
    QMessageBox::information(this, tr("保存成功"), tr("连接配置已保存"));
}

// 函数说明：响应 DatabaseConnectionDialog 收到的信号或异步回调，并更新界面状态。
void DatabaseConnectionDialog::onRemoveConnection()
{
    if (m_connectionList->currentRow() < 0) return;
    
    QString name = m_connectionList->currentItem()->text();
    
    if (QMessageBox::question(this, tr("确认删除"),
            tr("确定要删除连接 '%1' 吗？").arg(name)) == QMessageBox::Yes) {
        DatabaseConnector::instance().removeConnection(name);
        delete m_connectionList->takeItem(m_connectionList->currentRow());
    }
}

// 函数说明：响应 DatabaseConnectionDialog 收到的信号或异步回调，并更新界面状态。
void DatabaseConnectionDialog::onTestConnection()
{
    DatabaseConnector::ConnectionConfig config = getFormData();
    
    // 临时添加连接进行测试
    QString testName = "__test_connection__";
    config.name = testName;
    DatabaseConnector::instance().addConnection(config);
    
    QApplication::setOverrideCursor(Qt::WaitCursor);
    bool success = DatabaseConnector::instance().openConnection(testName);
    QApplication::restoreOverrideCursor();
    
    if (success) {
        QMessageBox::information(this, tr("连接成功"), tr("数据库连接测试成功！"));
        DatabaseConnector::instance().closeConnection(testName);
    } else {
        QString error = DatabaseConnector::instance().lastError(testName);
        QMessageBox::warning(this, tr("连接失败"), 
            tr("数据库连接测试失败：\n%1").arg(error));
    }
    
    DatabaseConnector::instance().removeConnection(testName);
}

// 函数说明：响应 DatabaseConnectionDialog 收到的信号或异步回调，并更新界面状态。
void DatabaseConnectionDialog::onConnectionSelected()
{
    if (m_connectionList->currentRow() < 0) {
        clearForm();
        return;
    }
    
    QString name = m_connectionList->currentItem()->text();
    DatabaseConnector::ConnectionConfig config = 
        DatabaseConnector::instance().getConnectionConfig(name);
    fillForm(config);
}

// 函数说明：清空 DatabaseConnectionDialog 保存的临时状态或缓存数据。
void DatabaseConnectionDialog::clearForm()
{
    m_nameEdit->clear();
    m_typeCombo->setCurrentIndex(0);
    m_hostEdit->clear();
    m_portSpin->setValue(3306);
    m_databaseEdit->clear();
    m_usernameEdit->clear();
    m_passwordEdit->clear();
}

// 函数说明：实现 DatabaseConnectionDialog::fillForm 的核心逻辑，供当前模块调用。
void DatabaseConnectionDialog::fillForm(const DatabaseConnector::ConnectionConfig &config)
{
    m_nameEdit->setText(config.name);
    m_typeCombo->setCurrentIndex(static_cast<int>(config.type));
    m_hostEdit->setText(config.host);
    m_portSpin->setValue(config.port > 0 ? config.port : 3306);
    m_databaseEdit->setText(config.database);
    m_usernameEdit->setText(config.username);
    m_passwordEdit->setText(config.password);
}

// 函数说明：读取 DatabaseConnectionDialog 当前保存的状态或计算结果。
DatabaseConnector::ConnectionConfig DatabaseConnectionDialog::getFormData() const
{
    DatabaseConnector::ConnectionConfig config;
    config.name = m_nameEdit->text();
    config.type = static_cast<DatabaseConnector::DatabaseType>(m_typeCombo->currentIndex());
    config.host = m_hostEdit->text();
    config.port = m_portSpin->value();
    config.database = m_databaseEdit->text();
    config.username = m_usernameEdit->text();
    config.password = m_passwordEdit->text();
    return config;
}

