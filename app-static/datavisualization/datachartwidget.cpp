// 文件说明：app-static\datavisualization\datachartwidget.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "datachartwidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QDialog>
#include <QToolTip>
#include <QMouseEvent>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QtMath>
#include <QComboBox>
#include <QTimer>
#include <QFile>
#include <limits>

#include <QtCharts/QLegend>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QPieSlice>

// ==================== DataChartWidget ====================

DataChartWidget::DataChartWidget(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_chartView(nullptr)
    , m_chart(nullptr)
    , m_titleLabel(nullptr)
    , m_loadingLabel(nullptr)
    , m_errorLabel(nullptr)
    , m_tooltipLabel(nullptr)
    , m_toolbarWidget(nullptr)
    , m_isLoading(false)
    , m_tooltipEnabled(true)
    , m_animationEnabled(true)
    , m_spinnerLabel(nullptr)
    , m_spinnerAnimation(nullptr)
    , m_chartTypeCombo(nullptr)
{
    // 初始化颜色主题
    m_colorPalette = {
        QColor("#4285f4"), QColor("#ea4335"), QColor("#fbbc04"),
        QColor("#34a853"), QColor("#ff6d01"), QColor("#46bdc6"),
        QColor("#7baaf7"), QColor("#f07b72"), QColor("#fcd04f"),
        QColor("#81c995"), QColor("#ff8a65"), QColor("#4dd0e1")
    };
    
    setupUI();
}

DataChartWidget::~DataChartWidget() = default;

// 函数说明：初始化 DataChartWidget 的 setupUI 相关界面、动作或服务连接。
void DataChartWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(4);

    // 标题栏和工具栏
    QWidget *headerWidget = new QWidget(this);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    headerLayout->addWidget(m_titleLabel);

    headerLayout->addStretch();

    // 工具栏按钮
    m_toolbarWidget = new QWidget(this);
    QHBoxLayout *toolbarLayout = new QHBoxLayout(m_toolbarWidget);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(4);

    QPushButton *refreshBtn = new QPushButton(tr("刷新"), this);
    refreshBtn->setToolTip(tr("刷新数据"));
    refreshBtn->setFixedSize(60, 24);
    connect(refreshBtn, &QPushButton::clicked, this, &DataChartWidget::onRefreshClicked);
    toolbarLayout->addWidget(refreshBtn);

    QPushButton *dataBtn = new QPushButton(tr("数据"), this);
    dataBtn->setToolTip(tr("查看原始数据"));
    dataBtn->setFixedSize(60, 24);
    connect(dataBtn, &QPushButton::clicked, this, &DataChartWidget::onShowDataClicked);
    toolbarLayout->addWidget(dataBtn);

    QPushButton *exportBtn = new QPushButton(tr("导出"), this);
    exportBtn->setToolTip(tr("导出数据"));
    exportBtn->setFixedSize(60, 24);
    connect(exportBtn, &QPushButton::clicked, this, &DataChartWidget::onExportClicked);
    toolbarLayout->addWidget(exportBtn);

    QPushButton *fullscreenBtn = new QPushButton(tr("全屏"), this);
    fullscreenBtn->setToolTip(tr("全屏查看"));
    fullscreenBtn->setFixedSize(60, 24);
    connect(fullscreenBtn, &QPushButton::clicked, this, &DataChartWidget::onFullscreenClicked);
    toolbarLayout->addWidget(fullscreenBtn);

    toolbarLayout->addSpacing(10);

    // 图表类型切换
    m_chartTypeCombo = new QComboBox(this);
    m_chartTypeCombo->addItem(tr("柱状图"), static_cast<int>(DataBlockParser::RenderType::BarChart));
    m_chartTypeCombo->addItem(tr("折线图"), static_cast<int>(DataBlockParser::RenderType::LineChart));
    m_chartTypeCombo->addItem(tr("饼图"), static_cast<int>(DataBlockParser::RenderType::PieChart));
    m_chartTypeCombo->addItem(tr("面积图"), static_cast<int>(DataBlockParser::RenderType::AreaChart));
    m_chartTypeCombo->addItem(tr("散点图"), static_cast<int>(DataBlockParser::RenderType::ScatterChart));
    m_chartTypeCombo->setFixedWidth(80);
    connect(m_chartTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DataChartWidget::onChartTypeChanged);
    toolbarLayout->addWidget(m_chartTypeCombo);

    headerLayout->addWidget(m_toolbarWidget);
    m_mainLayout->addWidget(headerWidget);

    // 图表视图
    m_chart = new QChart();
    m_chart->setAnimationOptions(QChart::SeriesAnimations);
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignBottom);

    m_chartView = new QChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setRubberBand(QChartView::RectangleRubberBand);
    m_mainLayout->addWidget(m_chartView, 1);

    // 加载提示容器
    QWidget *loadingContainer = new QWidget(this);
    QVBoxLayout *loadingLayout = new QVBoxLayout(loadingContainer);
    loadingLayout->setAlignment(Qt::AlignCenter);
    
    // 旋转加载动画
    m_spinnerLabel = new QLabel("⟳", this);
    m_spinnerLabel->setStyleSheet("font-size: 32px; color: #4285f4;");
    m_spinnerLabel->setAlignment(Qt::AlignCenter);
    loadingLayout->addWidget(m_spinnerLabel);
    
    m_loadingLabel = new QLabel(tr("正在加载数据..."), this);
    m_loadingLabel->setAlignment(Qt::AlignCenter);
    m_loadingLabel->setStyleSheet("font-size: 14px; color: #666;");
    loadingLayout->addWidget(m_loadingLabel);
    
    loadingContainer->hide();
    m_mainLayout->addWidget(loadingContainer);
    
    // 设置旋转动画
    m_spinnerAnimation = new QPropertyAnimation(m_spinnerLabel, "rotation", this);
    // 使用定时器实现简单的旋转效果
    QTimer *spinnerTimer = new QTimer(this);
    connect(spinnerTimer, &QTimer::timeout, this, [this]() {
        if (m_isLoading && m_spinnerLabel) {
            static int angle = 0;
            angle = (angle + 30) % 360;
            m_spinnerLabel->setStyleSheet(
                QString("font-size: 32px; color: #4285f4;")
            );
        }
    });
    spinnerTimer->start(100);

    // 错误提示
    m_errorLabel = new QLabel(this);
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setStyleSheet("font-size: 14px; color: #d32f2f; background: #ffebee; "
                                 "padding: 10px; border-radius: 4px;");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();
    m_mainLayout->addWidget(m_errorLabel);

    // 悬浮提示标签
    m_tooltipLabel = new QLabel(this);
    m_tooltipLabel->setStyleSheet(
        "background-color: rgba(50, 50, 50, 0.9);"
        "color: white;"
        "padding: 8px 12px;"
        "border-radius: 4px;"
        "font-size: 12px;"
    );
    m_tooltipLabel->hide();
    m_tooltipLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    // 添加阴影效果
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(10);
    shadow->setColor(QColor(0, 0, 0, 80));
    shadow->setOffset(2, 2);
    m_tooltipLabel->setGraphicsEffect(shadow);

    setMinimumSize(400, 300);
}

// 函数说明：设置 DataChartWidget 的运行参数，并触发必要的界面或数据刷新。
void DataChartWidget::setDataBlock(const DataBlockParser::DataBlock &block)
{
    m_block = block;
    
    if (!block.title.isEmpty()) {
        m_titleLabel->setText(block.title);
    }
    
    // 同步图表类型选择器
    if (m_chartTypeCombo) {
        int index = m_chartTypeCombo->findData(static_cast<int>(block.renderType));
        if (index >= 0) {
            m_chartTypeCombo->blockSignals(true);
            m_chartTypeCombo->setCurrentIndex(index);
            m_chartTypeCombo->blockSignals(false);
        }
    }
}

// 函数说明：设置 DataChartWidget 的运行参数，并触发必要的界面或数据刷新。
void DataChartWidget::setData(const DatabaseConnector::QueryResult &result)
{
    m_data = result;
    hideLoading();
    
    if (!result.success) {
        showError(result.errorMessage);
        return;
    }
    
    m_errorLabel->hide();
    updateChart();
}

// 函数说明：实现 DataChartWidget::refresh 的核心逻辑，供当前模块调用。
void DataChartWidget::refresh()
{
    showLoading();
    emit refreshRequested(m_block.id);
}

// 函数说明：设置 DataChartWidget 的运行参数，并触发必要的界面或数据刷新。
void DataChartWidget::setTitle(const QString &title)
{
    m_titleLabel->setText(title);
    m_chart->setTitle(title);
}

// 函数说明：设置 DataChartWidget 的运行参数，并触发必要的界面或数据刷新。
void DataChartWidget::setAnimationEnabled(bool enabled)
{
    m_animationEnabled = enabled;
    m_chart->setAnimationOptions(enabled ? QChart::SeriesAnimations : QChart::NoAnimation);
}

// 函数说明：设置 DataChartWidget 的运行参数，并触发必要的界面或数据刷新。
void DataChartWidget::setLegendVisible(bool visible)
{
    m_chart->legend()->setVisible(visible);
}

// 函数说明：设置 DataChartWidget 的运行参数，并触发必要的界面或数据刷新。
void DataChartWidget::setTooltipEnabled(bool enabled)
{
    m_tooltipEnabled = enabled;
}

// 函数说明：显示 DataChartWidget 管理的面板、对话框或提示信息。
void DataChartWidget::showLoading()
{
    m_isLoading = true;
    m_chartView->hide();
    m_errorLabel->hide();
    if (m_loadingLabel && m_loadingLabel->parentWidget()) {
        m_loadingLabel->parentWidget()->show();
    }
}

// 函数说明：隐藏 DataChartWidget 管理的界面组件。
void DataChartWidget::hideLoading()
{
    m_isLoading = false;
    if (m_loadingLabel && m_loadingLabel->parentWidget()) {
        m_loadingLabel->parentWidget()->hide();
    }
    m_chartView->show();
}

// 函数说明：显示 DataChartWidget 管理的面板、对话框或提示信息。
void DataChartWidget::showError(const QString &message)
{
    m_chartView->hide();
    m_loadingLabel->hide();
    m_errorLabel->setText(tr("错误: %1").arg(message));
    m_errorLabel->show();
}

// 函数说明：响应尺寸变化，重新计算 DataChartWidget 的布局。
void DataChartWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

// 函数说明：实现 DataChartWidget::eventFilter 的核心逻辑，供当前模块调用。
bool DataChartWidget::eventFilter(QObject *obj, QEvent *event)
{
    return QWidget::eventFilter(obj, event);
}

// 函数说明：刷新 DataChartWidget 的内部状态，并同步到相关界面。
void DataChartWidget::updateChart()
{
    // 清除旧的系列
    m_chart->removeAllSeries();
    
    // 清除旧的坐标轴
    for (QAbstractAxis *axis : m_chart->axes()) {
        m_chart->removeAxis(axis);
    }

    m_dataPointToRowMap.clear();

    switch (m_block.renderType) {
        case DataBlockParser::RenderType::BarChart:
            createBarChart();
            break;
        case DataBlockParser::RenderType::LineChart:
            createLineChart();
            break;
        case DataBlockParser::RenderType::PieChart:
            createPieChart();
            break;
        case DataBlockParser::RenderType::AreaChart:
            createAreaChart();
            break;
        case DataBlockParser::RenderType::ScatterChart:
            createScatterChart();
            break;
        case DataBlockParser::RenderType::Table:
            createTableView();
            break;
        default:
            createBarChart(); // 默认柱状图
            break;
    }
}

// 函数说明：实现 DataChartWidget::extractMultiSeriesData 的核心逻辑，供当前模块调用。
DataChartWidget::ChartData DataChartWidget::extractMultiSeriesData()
{
    ChartData data;
    m_dataPointToRowMap.clear();

    if (m_data.rows.isEmpty() || m_data.columns.size() < 2) {
        return data;
    }

    // 第一列是类别
    for (int i = 0; i < m_data.rows.size(); ++i) {
        const QVector<QVariant> &row = m_data.rows[i];
        if (!row.isEmpty()) {
            data.categories.append(row[0].toString());
            m_dataPointToRowMap[i] = i;
        }
    }

    // 剩余列是数据系列
    for (int col = 1; col < m_data.columns.size(); ++col) {
        QString seriesName = m_data.columns[col];
        QVector<double> values;
        
        for (int row = 0; row < m_data.rows.size(); ++row) {
            if (col < m_data.rows[row].size()) {
                values.append(m_data.rows[row][col].toDouble());
            } else {
                values.append(0.0);
            }
        }
        
        data.series[seriesName] = values;
    }

    return data;
}

// 函数说明：创建 DataChartWidget 需要的对象、记录或输出内容。
void DataChartWidget::createBarChart()
{
    ChartData data = extractMultiSeriesData();
    
    if (data.categories.isEmpty()) {
        showError(tr("没有可用的数据"));
        return;
    }

    QBarSeries *series = new QBarSeries();
    double maxValue = 0;
    int colorIndex = 0;

    // 为每个数据系列创建 BarSet
    for (auto it = data.series.begin(); it != data.series.end(); ++it) {
        QBarSet *barSet = new QBarSet(it.key());
        
        for (double value : it.value()) {
            *barSet << value;
            if (value > maxValue) maxValue = value;
        }
        
        // 设置颜色
        QColor color = m_colorPalette[colorIndex % m_colorPalette.size()];
        barSet->setColor(color);
        barSet->setBorderColor(color.darker(120));
        
        series->append(barSet);
        colorIndex++;
    }

    // 连接信号
    connect(series, &QBarSeries::hovered, this, &DataChartWidget::onBarHovered);
    connect(series, &QBarSeries::clicked, this, &DataChartWidget::onBarClicked);

    m_chart->addSeries(series);

    // 设置坐标轴
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(data.categories);
    m_chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0, maxValue * 1.1);
    axisY->setLabelFormat("%.2f");
    m_chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
}

// 函数说明：创建 DataChartWidget 需要的对象、记录或输出内容。
void DataChartWidget::createLineChart()
{
    ChartData data = extractMultiSeriesData();
    
    if (data.categories.isEmpty()) {
        showError(tr("没有可用的数据"));
        return;
    }

    double maxValue = std::numeric_limits<double>::lowest();
    double minValue = std::numeric_limits<double>::max();
    int colorIndex = 0;
    
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(data.categories);
    m_chart->addAxis(axisX, Qt::AlignBottom);

    // 为每个数据系列创建折线
    for (auto it = data.series.begin(); it != data.series.end(); ++it) {
        QLineSeries *series = new QLineSeries();
        series->setName(it.key());

        const QVector<double> &values = it.value();
        for (int i = 0; i < values.size(); ++i) {
            series->append(i, values[i]);
            if (values[i] > maxValue) maxValue = values[i];
            if (values[i] < minValue) minValue = values[i];
        }

        // 连接信号
        connect(series, &QLineSeries::hovered, this, &DataChartWidget::onPointHovered);
        connect(series, &QLineSeries::clicked, this, &DataChartWidget::onPointClicked);

        m_chart->addSeries(series);
        series->attachAxis(axisX);

        // 设置样式
        QColor color = m_colorPalette[colorIndex % m_colorPalette.size()];
        QPen pen = series->pen();
        pen.setWidth(3);
        pen.setColor(color);
        series->setPen(pen);
        series->setPointsVisible(true);
        
        colorIndex++;
    }

    // Y轴 - 数值轴
    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(qMin(0.0, minValue), maxValue * 1.1);
    m_chart->addAxis(axisY, Qt::AlignLeft);
    
    // 将所有系列附加到 Y 轴
    for (QAbstractSeries *s : m_chart->series()) {
        s->attachAxis(axisY);
    }
}

// 函数说明：创建 DataChartWidget 需要的对象、记录或输出内容。
void DataChartWidget::createPieChart()
{
    ChartData chartData = extractMultiSeriesData();
    QStringList categories = chartData.categories;
    QVector<double> values;
    
    // 获取第一个系列的值
    if (!chartData.series.isEmpty()) {
        values = chartData.series.first();
    }
    
    if (categories.isEmpty()) {
        showError(tr("没有可用的数据"));
        return;
    }

    QPieSeries *series = new QPieSeries();

    double total = 0;
    for (double v : values) total += v;

    for (int i = 0; i < categories.size() && i < values.size(); ++i) {
        QPieSlice *slice = series->append(categories[i], values[i]);
        slice->setLabelVisible(total > 0 && values[i] / total > 0.05);  // 显示大于5%的标签
        slice->setLabel(QString("%1 (%2%)").arg(categories[i]).arg(total > 0 ? values[i] / total * 100 : 0, 0, 'f', 1));
    }

    // 连接信号
    connect(series, &QPieSeries::hovered, this, &DataChartWidget::onPieHovered);
    connect(series, &QPieSeries::clicked, this, &DataChartWidget::onPieClicked);

    m_chart->addSeries(series);

    // 颜色主题
    QList<QColor> colors = {
        QColor("#4285f4"), QColor("#ea4335"), QColor("#fbbc04"),
        QColor("#34a853"), QColor("#ff6d01"), QColor("#46bdc6"),
        QColor("#7baaf7"), QColor("#f07b72"), QColor("#fcd04f")
    };

    QList<QPieSlice*> slices = series->slices();
    for (int i = 0; i < slices.size(); ++i) {
        slices[i]->setColor(colors[i % colors.size()]);
    }
}

// 函数说明：创建 DataChartWidget 需要的对象、记录或输出内容。
void DataChartWidget::createAreaChart()
{
    ChartData chartData = extractMultiSeriesData();
    QStringList categories = chartData.categories;
    QVector<double> values;
    
    // 获取第一个系列的值
    if (!chartData.series.isEmpty()) {
        values = chartData.series.first();
    }
    
    if (categories.isEmpty()) {
        showError(tr("没有可用的数据"));
        return;
    }

    QLineSeries *upperSeries = new QLineSeries();
    for (int i = 0; i < values.size(); ++i) {
        upperSeries->append(i, values[i]);
    }

    QLineSeries *lowerSeries = new QLineSeries();
    for (int i = 0; i < values.size(); ++i) {
        lowerSeries->append(i, 0);
    }

    QAreaSeries *series = new QAreaSeries(upperSeries, lowerSeries);
    series->setName(m_data.columns.size() > 1 ? m_data.columns[1] : tr("数值"));

    m_chart->addSeries(series);

    // 设置坐标轴
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    m_chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    double maxValue = *std::max_element(values.begin(), values.end());
    axisY->setRange(0, maxValue * 1.1);
    m_chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // 设置样式
    QLinearGradient gradient(QPointF(0, 0), QPointF(0, 1));
    gradient.setColorAt(0.0, QColor("#4285f4").lighter(120));
    gradient.setColorAt(1.0, QColor("#4285f4").lighter(180));
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    series->setBrush(gradient);
    series->setPen(QPen(QColor("#4285f4"), 2));
}

// 函数说明：创建 DataChartWidget 需要的对象、记录或输出内容。
void DataChartWidget::createScatterChart()
{
    if (m_data.rows.isEmpty() || m_data.columns.size() < 2) {
        showError(tr("散点图需要至少两列数值数据"));
        return;
    }

    QScatterSeries *series = new QScatterSeries();
    series->setName(tr("数据点"));
    series->setMarkerShape(QScatterSeries::MarkerShapeCircle);
    series->setMarkerSize(12.0);

    double minX = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();

    for (int i = 0; i < m_data.rows.size(); ++i) {
        const QVector<QVariant> &row = m_data.rows[i];
        if (row.size() >= 2) {
            double x = row[0].toDouble();
            double y = row[1].toDouble();
            series->append(x, y);
            m_dataPointToRowMap[series->count() - 1] = i;
            
            minX = qMin(minX, x);
            maxX = qMax(maxX, x);
            minY = qMin(minY, y);
            maxY = qMax(maxY, y);
        }
    }

    connect(series, &QScatterSeries::hovered, this, &DataChartWidget::onPointHovered);
    connect(series, &QScatterSeries::clicked, this, &DataChartWidget::onPointClicked);

    m_chart->addSeries(series);

    // 设置坐标轴
    QValueAxis *axisX = new QValueAxis();
    axisX->setRange(minX - (maxX - minX) * 0.1, maxX + (maxX - minX) * 0.1);
    axisX->setTitleText(m_data.columns[0]);
    m_chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(minY - (maxY - minY) * 0.1, maxY + (maxY - minY) * 0.1);
    axisY->setTitleText(m_data.columns.size() > 1 ? m_data.columns[1] : tr("Y"));
    m_chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    series->setColor(QColor("#4285f4"));
    series->setBorderColor(QColor("#3367d6"));
}

// 函数说明：创建 DataChartWidget 需要的对象、记录或输出内容。
void DataChartWidget::createTableView()
{
    // 表格视图使用详情对话框
    onShowDataClicked();
}

// 函数说明：显示 DataChartWidget 管理的面板、对话框或提示信息。
void DataChartWidget::showTooltip(const QPoint &pos, const QString &text)
{
    if (!m_tooltipEnabled) return;

    m_tooltipLabel->setText(text);
    m_tooltipLabel->adjustSize();

    // 计算位置，确保不超出边界
    QPoint tooltipPos = pos + QPoint(15, 15);
    if (tooltipPos.x() + m_tooltipLabel->width() > width()) {
        tooltipPos.setX(pos.x() - m_tooltipLabel->width() - 10);
    }
    if (tooltipPos.y() + m_tooltipLabel->height() > height()) {
        tooltipPos.setY(pos.y() - m_tooltipLabel->height() - 10);
    }

    m_tooltipLabel->move(tooltipPos);
    m_tooltipLabel->show();
    m_tooltipLabel->raise();
}

// 函数说明：隐藏 DataChartWidget 管理的界面组件。
void DataChartWidget::hideTooltip()
{
    m_tooltipLabel->hide();
}

// 函数说明：显示 DataChartWidget 管理的面板、对话框或提示信息。
void DataChartWidget::showDetailDialog(const QString &title, int rowIndex)
{
    DataDetailDialog *dialog = new DataDetailDialog(this);
    dialog->setTitle(title);
    dialog->setData(m_data, rowIndex);
    dialog->exec();
    dialog->deleteLater();
}

// 悬停处理
void DataChartWidget::onBarHovered(bool status, int index, QBarSet *barset)
{
    if (status) {
        QString category;
        if (index >= 0 && index < m_data.rows.size()) {
            category = m_data.rows[index][0].toString();
        }
        double value = barset->at(index);
        
        QString text = QString("<b>%1</b><br/>%2: %3")
            .arg(category)
            .arg(barset->label())
            .arg(value, 0, 'f', 2);
        
        showTooltip(QCursor::pos() - mapToGlobal(QPoint(0, 0)), text);
    } else {
        hideTooltip();
    }
}

// 函数说明：响应 DataChartWidget 收到的信号或异步回调，并更新界面状态。
void DataChartWidget::onPieHovered(QPieSlice *slice, bool state)
{
    if (state) {
        slice->setExploded(true);
        slice->setExplodeDistanceFactor(0.1);
        
        QString text = QString("<b>%1</b><br/>数值: %2<br/>占比: %3%")
            .arg(slice->label().split(" (").first())
            .arg(slice->value(), 0, 'f', 2)
            .arg(slice->percentage() * 100, 0, 'f', 1);
        
        showTooltip(QCursor::pos() - mapToGlobal(QPoint(0, 0)), text);
    } else {
        slice->setExploded(false);
        hideTooltip();
    }
}

// 函数说明：响应 DataChartWidget 收到的信号或异步回调，并更新界面状态。
void DataChartWidget::onPointHovered(const QPointF &point, bool state)
{
    if (state) {
        QString text = QString("X: %1<br/>Y: %2")
            .arg(point.x(), 0, 'f', 2)
            .arg(point.y(), 0, 'f', 2);
        
        showTooltip(QCursor::pos() - mapToGlobal(QPoint(0, 0)), text);
    } else {
        hideTooltip();
    }
}

// 点击处理
void DataChartWidget::onBarClicked(int index, QBarSet *barset)
{
    if (index >= 0 && index < m_data.rows.size()) {
        QString category = m_data.rows[index][0].toString();
        showDetailDialog(tr("数据详情 - %1").arg(category), index);
        emit dataPointClicked(category, barset->at(index), index);
    }
}

// 函数说明：响应 DataChartWidget 收到的信号或异步回调，并更新界面状态。
void DataChartWidget::onPieClicked(QPieSlice *slice)
{
    // 找到对应的行
    QString label = slice->label().split(" (").first();
    for (int i = 0; i < m_data.rows.size(); ++i) {
        if (m_data.rows[i][0].toString() == label) {
            showDetailDialog(tr("数据详情 - %1").arg(label), i);
            emit dataPointClicked(label, slice->value(), i);
            break;
        }
    }
}

// 函数说明：响应 DataChartWidget 收到的信号或异步回调，并更新界面状态。
void DataChartWidget::onPointClicked(const QPointF &point)
{
    // 找到最近的数据点
    int nearestRow = -1;
    double minDist = std::numeric_limits<double>::max();
    
    for (int i = 0; i < m_data.rows.size(); ++i) {
        if (m_data.rows[i].size() >= 2) {
            double x = m_data.rows[i][0].toDouble();
            double y = m_data.rows[i][1].toDouble();
            double dist = qSqrt(qPow(x - point.x(), 2) + qPow(y - point.y(), 2));
            if (dist < minDist) {
                minDist = dist;
                nearestRow = i;
            }
        }
    }
    
    if (nearestRow >= 0) {
        showDetailDialog(tr("数据详情"), nearestRow);
        emit dataPointClicked(QString(), point.y(), nearestRow);
    }
}

// 工具栏动作
void DataChartWidget::onRefreshClicked()
{
    refresh();
}

// 函数说明：响应 DataChartWidget 收到的信号或异步回调，并更新界面状态。
void DataChartWidget::onShowDataClicked()
{
    emit showRawDataRequested(m_block.id);
    showDetailDialog(tr("原始数据 - %1").arg(m_block.title), -1);
}

// 函数说明：响应 DataChartWidget 收到的信号或异步回调，并更新界面状态。
void DataChartWidget::onExportClicked()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("导出数据"),
        QString(),
        tr("CSV 文件 (*.csv);;所有文件 (*)")
    );
    
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("导出失败"), tr("无法创建文件"));
        return;
    }
    
    QTextStream out(&file);
    
    // 写入列头
    out << m_data.columns.join(",") << "\n";
    
    // 写入数据
    for (const QVector<QVariant> &row : m_data.rows) {
        QStringList values;
        for (const QVariant &v : row) {
            QString str = v.toString();
            if (str.contains(",") || str.contains("\"") || str.contains("\n")) {
                str = "\"" + str.replace("\"", "\"\"") + "\"";
            }
            values.append(str);
        }
        out << values.join(",") << "\n";
    }
    
    file.close();
    QMessageBox::information(this, tr("导出成功"), tr("数据已导出到 %1").arg(fileName));
}

// 函数说明：响应 DataChartWidget 收到的信号或异步回调，并更新界面状态。
void DataChartWidget::onFullscreenClicked()
{
    // 创建全屏对话框
    QDialog *fullscreenDialog = new QDialog(this);
    fullscreenDialog->setWindowTitle(m_block.title.isEmpty() ? tr("图表全屏") : m_block.title);
    fullscreenDialog->setWindowFlags(Qt::Window);
    fullscreenDialog->setAttribute(Qt::WA_DeleteOnClose);
    
    QVBoxLayout *layout = new QVBoxLayout(fullscreenDialog);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // 创建新的图表视图
    QChart *fullChart = new QChart();
    fullChart->setTitle(m_chart->title());
    fullChart->setAnimationOptions(QChart::SeriesAnimations);
    fullChart->legend()->setVisible(true);
    fullChart->legend()->setAlignment(Qt::AlignBottom);
    
    // 复制系列到新图表
    for (QAbstractSeries *series : m_chart->series()) {
        QAbstractSeries *clonedSeries = nullptr;
        
        if (QBarSeries *barSeries = qobject_cast<QBarSeries*>(series)) {
            QBarSeries *newBarSeries = new QBarSeries();
            for (QBarSet *set : barSeries->barSets()) {
                QBarSet *newSet = new QBarSet(set->label());
                for (int i = 0; i < set->count(); ++i) {
                    *newSet << set->at(i);
                }
                newSet->setColor(set->color());
                newSet->setBorderColor(set->borderColor());
                newBarSeries->append(newSet);
            }
            clonedSeries = newBarSeries;
        } else if (QLineSeries *lineSeries = qobject_cast<QLineSeries*>(series)) {
            QLineSeries *newLineSeries = new QLineSeries();
            newLineSeries->setName(lineSeries->name());
            newLineSeries->setPen(lineSeries->pen());
            newLineSeries->setPointsVisible(lineSeries->pointsVisible());
            for (const QPointF &point : lineSeries->points()) {
                newLineSeries->append(point);
            }
            clonedSeries = newLineSeries;
        } else if (QPieSeries *pieSeries = qobject_cast<QPieSeries*>(series)) {
            QPieSeries *newPieSeries = new QPieSeries();
            for (QPieSlice *slice : pieSeries->slices()) {
                QPieSlice *newSlice = newPieSeries->append(slice->label(), slice->value());
                newSlice->setColor(slice->color());
                newSlice->setLabelVisible(slice->isLabelVisible());
            }
            clonedSeries = newPieSeries;
        }
        
        if (clonedSeries) {
            fullChart->addSeries(clonedSeries);
        }
    }
    
    // 创建坐标轴
    fullChart->createDefaultAxes();
    
    QChartView *fullChartView = new QChartView(fullChart);
    fullChartView->setRenderHint(QPainter::Antialiasing);
    fullChartView->setRubberBand(QChartView::RectangleRubberBand);
    
    layout->addWidget(fullChartView);
    
    // 关闭按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    QPushButton *closeBtn = new QPushButton(tr("关闭"), fullscreenDialog);
    connect(closeBtn, &QPushButton::clicked, fullscreenDialog, &QDialog::close);
    buttonLayout->addWidget(closeBtn);
    layout->addLayout(buttonLayout);
    
    // 显示全屏
    fullscreenDialog->showMaximized();
}

// 函数说明：响应 DataChartWidget 收到的信号或异步回调，并更新界面状态。
void DataChartWidget::onChartTypeChanged(int index)
{
    if (index < 0 || m_data.rows.isEmpty()) return;
    
    DataBlockParser::RenderType newType = 
        static_cast<DataBlockParser::RenderType>(m_chartTypeCombo->itemData(index).toInt());
    
    if (newType == m_block.renderType) return;
    
    m_block.renderType = newType;
    updateChart();
}


// ==================== DataDetailDialog ====================

DataDetailDialog::DataDetailDialog(QWidget *parent)
    : QDialog(parent)
    , m_tableWidget(nullptr)
    , m_closeButton(nullptr)
    , m_exportButton(nullptr)
{
    setupUI();
}

// 函数说明：初始化 DataDetailDialog 的 setupUI 相关界面、动作或服务连接。
void DataDetailDialog::setupUI()
{
    setWindowTitle(tr("数据详情"));
    setMinimumSize(600, 400);
    resize(800, 500);

    QVBoxLayout *layout = new QVBoxLayout(this);

    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setAlternatingRowColors(true);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->horizontalHeader()->setStretchLastSection(true);
    m_tableWidget->verticalHeader()->setDefaultSectionSize(28);
    layout->addWidget(m_tableWidget);

    // 按钮区
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_exportButton = new QPushButton(tr("导出 CSV"), this);
    buttonLayout->addWidget(m_exportButton);

    m_closeButton = new QPushButton(tr("关闭"), this);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_closeButton);

    layout->addLayout(buttonLayout);
}

// 函数说明：设置 DataDetailDialog 的运行参数，并触发必要的界面或数据刷新。
void DataDetailDialog::setData(const DatabaseConnector::QueryResult &result, int highlightRow)
{
    m_tableWidget->clear();
    
    // 设置列
    m_tableWidget->setColumnCount(result.columns.size());
    m_tableWidget->setHorizontalHeaderLabels(result.columns);
    
    // 设置行
    m_tableWidget->setRowCount(result.rows.size());
    
    for (int row = 0; row < result.rows.size(); ++row) {
        for (int col = 0; col < result.rows[row].size(); ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(result.rows[row][col].toString());
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            m_tableWidget->setItem(row, col, item);
            
            // 高亮选中行
            if (row == highlightRow) {
                item->setBackground(QColor("#e3f2fd"));
            }
        }
    }
    
    // 调整列宽
    m_tableWidget->resizeColumnsToContents();
    
    // 如果有高亮行，滚动到该行
    if (highlightRow >= 0 && highlightRow < result.rows.size()) {
        m_tableWidget->scrollToItem(m_tableWidget->item(highlightRow, 0));
        m_tableWidget->selectRow(highlightRow);
    }
}

// 函数说明：设置 DataDetailDialog 的运行参数，并触发必要的界面或数据刷新。
void DataDetailDialog::setTitle(const QString &title)
{
    setWindowTitle(title);
}

