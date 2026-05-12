// 文件说明：app-static\preview\printpreviewdialog.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "printpreviewdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QPushButton>
#include <QToolBar>
#include <QSplitter>
#include <QFileDialog>
#include <QPrintDialog>
#include <QMessageBox>
#include <QDateTime>
#include "compat/webenginecompat.h"

// 函数说明：构造 PrintPreviewDialog 对象，初始化本模块需要的状态、界面和资源。
PrintPreviewDialog::PrintPreviewDialog(QWidget *parent)
    : QDialog(parent)
    , m_previewView(new QWebEngineView(this))
    , m_printer(new QPrinter(QPrinter::HighResolution))
    , m_currentPage(1)
    , m_totalPages(1)
    , m_zoomLevel(100)
{
    setupUi();
    setupConnections();

    setWindowTitle(tr("打印预览"));
    resize(1000, 700);
}

// 函数说明：销毁 PrintPreviewDialog 对象，释放本模块持有的资源。
PrintPreviewDialog::~PrintPreviewDialog()
{
    delete m_printer;
}

// 函数说明：初始化 PrintPreviewDialog 的 setupUi 相关界面、动作或服务连接。
void PrintPreviewDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // 工具栏
    QToolBar *toolbar = new QToolBar(this);
    toolbar->setIconSize(QSize(24, 24));

    QPushButton *printBtn = new QPushButton(tr("🖨️ 打印"), this);
    connect(printBtn, &QPushButton::clicked, this, &PrintPreviewDialog::print);
    toolbar->addWidget(printBtn);

    QPushButton *pdfBtn = new QPushButton(tr("📄 导出 PDF"), this);
    connect(pdfBtn, &QPushButton::clicked, this, &PrintPreviewDialog::exportToPdf);
    toolbar->addWidget(pdfBtn);

    toolbar->addSeparator();

    QPushButton *zoomOutBtn = new QPushButton(tr("-"), this);
    zoomOutBtn->setFixedWidth(30);
    connect(zoomOutBtn, &QPushButton::clicked, this, &PrintPreviewDialog::onZoomOut);
    toolbar->addWidget(zoomOutBtn);

    m_zoomLabel = new QLabel("100%", this);
    m_zoomLabel->setFixedWidth(50);
    m_zoomLabel->setAlignment(Qt::AlignCenter);
    toolbar->addWidget(m_zoomLabel);

    QPushButton *zoomInBtn = new QPushButton(tr("+"), this);
    zoomInBtn->setFixedWidth(30);
    connect(zoomInBtn, &QPushButton::clicked, this, &PrintPreviewDialog::onZoomIn);
    toolbar->addWidget(zoomInBtn);

    QPushButton *zoomResetBtn = new QPushButton(tr("重置"), this);
    connect(zoomResetBtn, &QPushButton::clicked, this, &PrintPreviewDialog::onZoomReset);
    toolbar->addWidget(zoomResetBtn);

    toolbar->addSeparator();

    QPushButton *prevPageBtn = new QPushButton(tr("◀"), this);
    connect(prevPageBtn, &QPushButton::clicked, this, &PrintPreviewDialog::onPreviousPage);
    toolbar->addWidget(prevPageBtn);

    m_pageInfoLabel = new QLabel(tr("第 1 页 / 共 1 页"), this);
    toolbar->addWidget(m_pageInfoLabel);

    QPushButton *nextPageBtn = new QPushButton(tr("▶"), this);
    connect(nextPageBtn, &QPushButton::clicked, this, &PrintPreviewDialog::onNextPage);
    toolbar->addWidget(nextPageBtn);

    mainLayout->addWidget(toolbar);

    // 主内容区域
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

    // 左侧设置面板
    QWidget *settingsPanel = new QWidget(this);
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsPanel);
    settingsLayout->setContentsMargins(5, 5, 5, 5);

    // 页面设置
    QGroupBox *pageGroup = new QGroupBox(tr("页面设置"), this);
    QFormLayout *pageLayout = new QFormLayout(pageGroup);

    m_pageSizeCombo = new QComboBox(this);
    m_pageSizeCombo->addItem("A4", QPageSize::A4);
    m_pageSizeCombo->addItem("A3", QPageSize::A3);
    m_pageSizeCombo->addItem("A5", QPageSize::A5);
    m_pageSizeCombo->addItem("Letter", QPageSize::Letter);
    m_pageSizeCombo->addItem("Legal", QPageSize::Legal);
    pageLayout->addRow(tr("纸张大小:"), m_pageSizeCombo);

    m_orientationCombo = new QComboBox(this);
    m_orientationCombo->addItem(tr("纵向"), QPageLayout::Portrait);
    m_orientationCombo->addItem(tr("横向"), QPageLayout::Landscape);
    pageLayout->addRow(tr("方向:"), m_orientationCombo);

    settingsLayout->addWidget(pageGroup);

    // 边距设置
    QGroupBox *marginGroup = new QGroupBox(tr("边距 (mm)"), this);
    QFormLayout *marginLayout = new QFormLayout(marginGroup);

    m_marginTopSpin = new QSpinBox(this);
    m_marginTopSpin->setRange(0, 100);
    m_marginTopSpin->setValue(15);
    marginLayout->addRow(tr("上:"), m_marginTopSpin);

    m_marginBottomSpin = new QSpinBox(this);
    m_marginBottomSpin->setRange(0, 100);
    m_marginBottomSpin->setValue(15);
    marginLayout->addRow(tr("下:"), m_marginBottomSpin);

    m_marginLeftSpin = new QSpinBox(this);
    m_marginLeftSpin->setRange(0, 100);
    m_marginLeftSpin->setValue(15);
    marginLayout->addRow(tr("左:"), m_marginLeftSpin);

    m_marginRightSpin = new QSpinBox(this);
    m_marginRightSpin->setRange(0, 100);
    m_marginRightSpin->setValue(15);
    marginLayout->addRow(tr("右:"), m_marginRightSpin);

    settingsLayout->addWidget(marginGroup);

    // 打印选项
    QGroupBox *optionGroup = new QGroupBox(tr("打印选项"), this);
    QVBoxLayout *optionLayout = new QVBoxLayout(optionGroup);

    m_scaleSpin = new QSpinBox(this);
    m_scaleSpin->setRange(50, 200);
    m_scaleSpin->setValue(100);
    m_scaleSpin->setSuffix("%");
    QHBoxLayout *scaleLayout = new QHBoxLayout();
    scaleLayout->addWidget(new QLabel(tr("缩放:"), this));
    scaleLayout->addWidget(m_scaleSpin);
    optionLayout->addLayout(scaleLayout);

    m_printBackgroundCheck = new QCheckBox(tr("打印背景"), this);
    m_printBackgroundCheck->setChecked(true);
    optionLayout->addWidget(m_printBackgroundCheck);

    m_printHeaderFooterCheck = new QCheckBox(tr("打印页眉页脚"), this);
    m_printHeaderFooterCheck->setChecked(true);
    optionLayout->addWidget(m_printHeaderFooterCheck);

    settingsLayout->addWidget(optionGroup);
    settingsLayout->addStretch();

    settingsPanel->setFixedWidth(220);
    splitter->addWidget(settingsPanel);

    // 右侧预览区域
    QWidget *previewPanel = new QWidget(this);
    QVBoxLayout *previewLayout = new QVBoxLayout(previewPanel);
    previewLayout->setContentsMargins(0, 0, 0, 0);

    m_previewView->setMinimumSize(500, 500);
    m_previewView->settings()->setAttribute(QWebEngineSettings::ShowScrollBars, true);
    previewLayout->addWidget(m_previewView);

    splitter->addWidget(previewPanel);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter, 1);

    // 底部按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton *closeBtn = new QPushButton(tr("关闭"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(closeBtn);

    mainLayout->addLayout(buttonLayout);
}

// 函数说明：初始化 PrintPreviewDialog 的 setupConnections 相关界面、动作或服务连接。
void PrintPreviewDialog::setupConnections()
{
    connect(m_pageSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PrintPreviewDialog::onPageSizeChanged);
    connect(m_orientationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PrintPreviewDialog::onOrientationChanged);
    connect(m_marginTopSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PrintPreviewDialog::onMarginsChanged);
    connect(m_marginBottomSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PrintPreviewDialog::onMarginsChanged);
    connect(m_marginLeftSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PrintPreviewDialog::onMarginsChanged);
    connect(m_marginRightSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PrintPreviewDialog::onMarginsChanged);
    connect(m_scaleSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PrintPreviewDialog::onScaleChanged);
    connect(m_printBackgroundCheck, &QCheckBox::toggled,
            this, &PrintPreviewDialog::onPrintBackgroundChanged);
}

// 函数说明：设置 PrintPreviewDialog 的运行参数，并触发必要的界面或数据刷新。
void PrintPreviewDialog::setHtmlContent(const QString &html)
{
    m_htmlContent = html;
    updatePreview();
}

// 函数说明：设置 PrintPreviewDialog 的运行参数，并触发必要的界面或数据刷新。
void PrintPreviewDialog::setDocumentTitle(const QString &title)
{
    m_documentTitle = title;
    updatePreview();
}

// 函数说明：设置 PrintPreviewDialog 的运行参数，并触发必要的界面或数据刷新。
void PrintPreviewDialog::setConfig(const PrintConfig &config)
{
    m_config = config;

    // 更新 UI
    for (int i = 0; i < m_pageSizeCombo->count(); ++i) {
        if (m_pageSizeCombo->itemData(i).toInt() == config.pageSize) {
            m_pageSizeCombo->setCurrentIndex(i);
            break;
        }
    }

    m_orientationCombo->setCurrentIndex(config.orientation == QPageLayout::Portrait ? 0 : 1);
    m_marginTopSpin->setValue(static_cast<int>(config.margins.top()));
    m_marginBottomSpin->setValue(static_cast<int>(config.margins.bottom()));
    m_marginLeftSpin->setValue(static_cast<int>(config.margins.left()));
    m_marginRightSpin->setValue(static_cast<int>(config.margins.right()));
    m_scaleSpin->setValue(config.scaleFactor);
    m_printBackgroundCheck->setChecked(config.printBackground);
    m_printHeaderFooterCheck->setChecked(config.printHeaderFooter);

    updatePreview();
}

// 函数说明：响应 PrintPreviewDialog 收到的信号或异步回调，并更新界面状态。
void PrintPreviewDialog::onPageSizeChanged(int index)
{
    m_config.pageSize = static_cast<QPageSize::PageSizeId>(
        m_pageSizeCombo->itemData(index).toInt());
    updatePreview();
}

// 函数说明：响应 PrintPreviewDialog 收到的信号或异步回调，并更新界面状态。
void PrintPreviewDialog::onOrientationChanged(int index)
{
    m_config.orientation = index == 0 ?
        QPageLayout::Portrait : QPageLayout::Landscape;
    updatePreview();
}

// 函数说明：响应 PrintPreviewDialog 收到的信号或异步回调，并更新界面状态。
void PrintPreviewDialog::onMarginsChanged()
{
    m_config.margins = QMarginsF(
        m_marginLeftSpin->value(),
        m_marginTopSpin->value(),
        m_marginRightSpin->value(),
        m_marginBottomSpin->value()
    );
    updatePreview();
}

// 函数说明：响应 PrintPreviewDialog 收到的信号或异步回调，并更新界面状态。
void PrintPreviewDialog::onScaleChanged(int value)
{
    m_config.scaleFactor = value;
    updatePreview();
}

// 函数说明：响应 PrintPreviewDialog 收到的信号或异步回调，并更新界面状态。
void PrintPreviewDialog::onPrintBackgroundChanged(bool checked)
{
    m_config.printBackground = checked;
    updatePreview();
}

// 函数说明：响应 PrintPreviewDialog 收到的信号或异步回调，并更新界面状态。
void PrintPreviewDialog::onZoomIn()
{
    m_zoomLevel = qMin(m_zoomLevel + 25, 300);
    m_zoomLabel->setText(QString("%1%").arg(m_zoomLevel));
    m_previewView->setZoomFactor(m_zoomLevel / 100.0);
}

// 函数说明：响应 PrintPreviewDialog 收到的信号或异步回调，并更新界面状态。
void PrintPreviewDialog::onZoomOut()
{
    m_zoomLevel = qMax(m_zoomLevel - 25, 25);
    m_zoomLabel->setText(QString("%1%").arg(m_zoomLevel));
    m_previewView->setZoomFactor(m_zoomLevel / 100.0);
}

// 函数说明：响应 PrintPreviewDialog 收到的信号或异步回调，并更新界面状态。
void PrintPreviewDialog::onZoomReset()
{
    m_zoomLevel = 100;
    m_zoomLabel->setText("100%");
    m_previewView->setZoomFactor(1.0);
}

// 函数说明：响应 PrintPreviewDialog 收到的信号或异步回调，并更新界面状态。
void PrintPreviewDialog::onPreviousPage()
{
    if (m_currentPage > 1) {
        m_currentPage--;
        updatePageInfo();
        // 滚动到对应页面
    }
}

// 函数说明：响应 PrintPreviewDialog 收到的信号或异步回调，并更新界面状态。
void PrintPreviewDialog::onNextPage()
{
    if (m_currentPage < m_totalPages) {
        m_currentPage++;
        updatePageInfo();
        // 滚动到对应页面
    }
}

// 函数说明：刷新 PrintPreviewDialog 的内部状态，并同步到相关界面。
void PrintPreviewDialog::updatePreview()
{
    QString html = generatePrintHtml();
    m_previewView->setHtml(html);
}

// 函数说明：根据当前数据生成 PrintPreviewDialog 需要的输出结果。
QString PrintPreviewDialog::generatePrintHtml()
{
    QString css = generatePrintCss();

    QString header, footer;
    if (m_config.printHeaderFooter) {
        header = QString(
            "<div class='header'>"
            "<span class='left'>%1</span>"
            "<span class='center'>%2</span>"
            "<span class='right'>%3</span>"
            "</div>")
            .arg(processHeaderFooter(m_config.headerLeft))
            .arg(processHeaderFooter(m_config.headerCenter))
            .arg(processHeaderFooter(m_config.headerRight));

        footer = QString(
            "<div class='footer'>"
            "<span class='left'>%1</span>"
            "<span class='center'>%2</span>"
            "<span class='right'>%3</span>"
            "</div>")
            .arg(processHeaderFooter(m_config.footerLeft))
            .arg(processHeaderFooter(m_config.footerCenter))
            .arg(processHeaderFooter(m_config.footerRight));
    }

    return QString(R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <style>%1</style>
</head>
<body>
    %2
    <div class="content">%3</div>
    %4
</body>
</html>
)")
    .arg(css)
    .arg(header)
    .arg(m_htmlContent)
    .arg(footer);
}

// 函数说明：根据当前数据生成 PrintPreviewDialog 需要的输出结果。
QString PrintPreviewDialog::generatePrintCss()
{
    // 计算页面尺寸
    QPageSize pageSize(m_config.pageSize);
    QSizeF size = pageSize.size(QPageSize::Millimeter);
    if (m_config.orientation == QPageLayout::Landscape) {
        size.transpose();
    }

    QString background = m_config.printBackground ?
        "" : "background: white !important; background-color: white !important;";

    return QString(R"(
@page {
    size: %1mm %2mm;
    margin: %3mm %4mm %5mm %6mm;
}

* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

html, body {
    width: %1mm;
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
    font-size: %7px;
    line-height: 1.6;
    color: #333;
    %8
}

body {
    padding: %3mm %4mm %5mm %6mm;
    transform: scale(%9);
    transform-origin: top left;
}

.content {
    min-height: calc(%2mm - %3mm - %5mm - 20mm);
}

.header, .footer {
    position: fixed;
    left: %6mm;
    right: %4mm;
    display: flex;
    justify-content: space-between;
    font-size: 10px;
    color: #666;
}

.header {
    top: 5mm;
    border-bottom: 1px solid #ddd;
    padding-bottom: 2mm;
}

.footer {
    bottom: 5mm;
    border-top: 1px solid #ddd;
    padding-top: 2mm;
}

.header .left, .footer .left { text-align: left; }
.header .center, .footer .center { text-align: center; }
.header .right, .footer .right { text-align: right; }

h1, h2, h3, h4, h5, h6 {
    margin-top: 1.5em;
    margin-bottom: 0.5em;
    page-break-after: avoid;
}

p { margin: 1em 0; }

pre, blockquote, table, img {
    page-break-inside: avoid;
}

pre {
    background: #f5f5f5;
    padding: 10px;
    border-radius: 4px;
    overflow-x: auto;
    font-family: 'SF Mono', Consolas, monospace;
    font-size: 12px;
}

code {
    background: #f5f5f5;
    padding: 2px 5px;
    border-radius: 3px;
    font-family: 'SF Mono', Consolas, monospace;
}

blockquote {
    border-left: 3px solid #ddd;
    padding-left: 15px;
    margin: 1em 0;
    color: #666;
}

table {
    border-collapse: collapse;
    width: 100%%;
    margin: 1em 0;
}

th, td {
    border: 1px solid #ddd;
    padding: 8px;
    text-align: left;
}

th { background: #f5f5f5; }

img { max-width: 100%%; height: auto; }

%10
)")
    .arg(size.width())          // 1
    .arg(size.height())         // 2
    .arg(m_config.margins.top())    // 3
    .arg(m_config.margins.right())  // 4
    .arg(m_config.margins.bottom()) // 5
    .arg(m_config.margins.left())   // 6
    .arg(14)                        // 7 - 基础字号
    .arg(background)                // 8
    .arg(m_config.scaleFactor / 100.0)  // 9
    .arg(m_config.customCss);       // 10
}

// 函数说明：处理 PrintPreviewDialog 的核心业务数据，并输出处理结果。
QString PrintPreviewDialog::processHeaderFooter(const QString &text)
{
    QString result = text;
    result.replace("{{title}}", m_documentTitle);
    result.replace("{{date}}", QDateTime::currentDateTime().toString("yyyy-MM-dd"));
    result.replace("{{time}}", QDateTime::currentDateTime().toString("HH:mm"));
    result.replace("{{page}}", QString::number(m_currentPage));
    result.replace("{{pages}}", QString::number(m_totalPages));
    return result;
}

// 函数说明：刷新 PrintPreviewDialog 的内部状态，并同步到相关界面。
void PrintPreviewDialog::updatePageInfo()
{
    m_pageInfoLabel->setText(tr("第 %1 页 / 共 %2 页")
        .arg(m_currentPage).arg(m_totalPages));
}

// 函数说明：实现 PrintPreviewDialog::print 的核心逻辑，供当前模块调用。
void PrintPreviewDialog::print()
{
    // 设置打印机
    m_printer->setPageSize(QPageSize(m_config.pageSize));
    m_printer->setPageOrientation(m_config.orientation);
    m_printer->setPageMargins(m_config.margins, QPageLayout::Millimeter);

    QPrintDialog dialog(m_printer, this);
    if (dialog.exec() == QDialog::Accepted) {
        m_previewView->page()->printToPdf([this](const QByteArray &pdfData) {
            if (!pdfData.isEmpty()) {
                // Print the PDF data
                QMessageBox::information(this, tr("打印"),
                    tr("打印任务已发送"));
            } else {
                QMessageBox::warning(this, tr("打印"),
                    tr("打印失败"));
            }
        });

        emit printRequested();
    }
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
void PrintPreviewDialog::exportToPdf()
{
    QString filePath = QFileDialog::getSaveFileName(this,
        tr("导出为 PDF"),
        m_documentTitle + ".pdf",
        tr("PDF 文件 (*.pdf)"));

    if (filePath.isEmpty()) return;

    // 设置打印机
    m_printer->setOutputFormat(QPrinter::PdfFormat);
    m_printer->setOutputFileName(filePath);
    m_printer->setPageSize(QPageSize(m_config.pageSize));
    m_printer->setPageOrientation(m_config.orientation);
    m_printer->setPageMargins(m_config.margins, QPageLayout::Millimeter);

    m_previewView->page()->printToPdf(filePath, QPageLayout(
        QPageSize(m_config.pageSize),
        m_config.orientation,
        m_config.margins,
        QPageLayout::Millimeter
    ));

    QMessageBox::information(this, tr("导出 PDF"),
        tr("PDF 已导出到:\n%1").arg(filePath));

    emit pdfExportRequested(filePath);
}

