// 文件说明：app-static\datavisualization\datachartwidget.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef DATACHARTWIDGET_H
#define DATACHARTWIDGET_H

#include <QWidget>
#include <QDialog>
#include <QVector>
#include <QVariant>
#include <QStringList>
#include <QMap>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QLineSeries>
#include <QtCharts/QPieSeries>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>

#include "datablockparser.h"
#include "databaseconnector.h"

// Qt 6 Charts is in Qt namespace by default

class QLabel;
class QVBoxLayout;
class QTableWidget;
class QPushButton;
class QMovie;
class QPropertyAnimation;
class QComboBox;

/**
 * @brief 交互式图表控件 - 使用 Qt Charts 渲染数据可视化
 * 
 * 特性:
 * - 支持多种图表类型 (柱状图、折线图、饼图等)
 * - 鼠标悬停显示数据值
 * - 点击数据点弹出详细数据对话框
 * - 支持图表缩放和平移
 */
class DataChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DataChartWidget(QWidget *parent = nullptr);
    ~DataChartWidget();

    // 设置数据块信息
    void setDataBlock(const DataBlockParser::DataBlock &block);
    DataBlockParser::DataBlock dataBlock() const { return m_block; }

    // 设置查询结果数据
    void setData(const DatabaseConnector::QueryResult &result);

    // 刷新图表
    void refresh();

    // 获取块ID
    QString blockId() const { return m_block.id; }

    // 图表选项
    void setTitle(const QString &title);
    void setAnimationEnabled(bool enabled);
    void setLegendVisible(bool visible);
    void setTooltipEnabled(bool enabled);

signals:
    // 请求刷新数据
    void refreshRequested(const QString &blockId);
    
    // 数据点被点击
    void dataPointClicked(const QString &category, const QVariant &value, int rowIndex);

    // 请求显示原始数据
    void showRawDataRequested(const QString &blockId);

public slots:
    // 显示加载状态
    void showLoading();
    void hideLoading();

    // 显示错误信息
    void showError(const QString &message);

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    // 悬停处理
    void onBarHovered(bool status, int index, QBarSet *barset);
    void onPieHovered(QPieSlice *slice, bool state);
    void onPointHovered(const QPointF &point, bool state);

    // 点击处理
    void onBarClicked(int index, QBarSet *barset);
    void onPieClicked(QPieSlice *slice);
    void onPointClicked(const QPointF &point);

    // 工具栏动作
    void onRefreshClicked();
    void onShowDataClicked();
    void onExportClicked();
    void onFullscreenClicked();
    void onChartTypeChanged(int index);

private:
    void setupUI();
    void createChart();
    void updateChart();

    // 创建不同类型的图表
    void createBarChart();
    void createLineChart();
    void createPieChart();
    void createAreaChart();
    void createScatterChart();
    void createTableView();

    // 创建工具提示
    void showTooltip(const QPoint &pos, const QString &text);
    void hideTooltip();

    // 创建详情对话框
    void showDetailDialog(const QString &title, int rowIndex);

    // 数据处理 - 支持多系列
    struct ChartData {
        QStringList categories;                     // X轴类别
        QMap<QString, QVector<double>> series;      // 多个数据系列
    };
    ChartData extractMultiSeriesData();

private:
    // UI 组件
    QVBoxLayout *m_mainLayout;
    QChartView *m_chartView;
    QChart *m_chart;
    QLabel *m_titleLabel;
    QLabel *m_loadingLabel;
    QLabel *m_errorLabel;
    QLabel *m_tooltipLabel;
    QWidget *m_toolbarWidget;

    // 数据
    DataBlockParser::DataBlock m_block;
    DatabaseConnector::QueryResult m_data;
    QMap<int, int> m_dataPointToRowMap;  // 图表数据点到原始行的映射

    // 状态
    bool m_isLoading;
    bool m_tooltipEnabled;
    bool m_animationEnabled;

    // 加载动画
    QLabel *m_spinnerLabel;
    QPropertyAnimation *m_spinnerAnimation;

    // 图表类型切换
    QComboBox *m_chartTypeCombo;

    // 颜色主题
    QList<QColor> m_colorPalette;
};


/**
 * @brief 原始数据详情对话框
 */
class DataDetailDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DataDetailDialog(QWidget *parent = nullptr);
    
    void setData(const DatabaseConnector::QueryResult &result, int highlightRow = -1);
    void setTitle(const QString &title);

private:
    void setupUI();

    QTableWidget *m_tableWidget;
    QPushButton *m_closeButton;
    QPushButton *m_exportButton;
};

#endif // DATACHARTWIDGET_H

