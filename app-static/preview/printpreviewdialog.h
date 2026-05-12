// 文件说明：app-static\preview\printpreviewdialog.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef PRINTPREVIEWDIALOG_H
#define PRINTPREVIEWDIALOG_H

#include <QDialog>
#include "compat/webenginecompat.h"
#include <QPrinter>
#include <QPageLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>

/**
 * @brief 打印预览对话框
 *
 * 功能：
 * - 显示打印预览效果
 * - 页面设置（纸张大小、方向、边距）
 * - 缩放控制
 * - 页眉页脚设置
 * - 直接打印或导出 PDF
 */
class PrintPreviewDialog : public QDialog
{
    Q_OBJECT

public:
    // 打印配置
    struct PrintConfig {
        QPageSize::PageSizeId pageSize;
        QPageLayout::Orientation orientation;
        QMarginsF margins;          // 毫米
        bool printBackground;
        bool printHeaderFooter;
        QString headerLeft;
        QString headerCenter;
        QString headerRight;
        QString footerLeft;
        QString footerCenter;
        QString footerRight;
        int scaleFactor;            // 百分比
        QString customCss;

        PrintConfig() :
            pageSize(QPageSize::A4),
            orientation(QPageLayout::Portrait),
            margins(15, 15, 15, 15),
            printBackground(true),
            printHeaderFooter(true),
            headerLeft(""),
            headerCenter("{{title}}"),
            headerRight(""),
            footerLeft("{{date}}"),
            footerCenter(""),
            footerRight("{{page}} / {{pages}}"),
            scaleFactor(100) {}
    };

    explicit PrintPreviewDialog(QWidget *parent = nullptr);
    ~PrintPreviewDialog();

    // 设置内容
    void setHtmlContent(const QString &html);
    void setDocumentTitle(const QString &title);

    // 配置
    void setConfig(const PrintConfig &config);
    PrintConfig config() const { return m_config; }

    // 获取打印机
    QPrinter* printer() { return m_printer; }

signals:
    void printRequested();
    void pdfExportRequested(const QString &filePath);

public slots:
    void print();
    void exportToPdf();
    void updatePreview();

private slots:
    void onPageSizeChanged(int index);
    void onOrientationChanged(int index);
    void onMarginsChanged();
    void onScaleChanged(int value);
    void onPrintBackgroundChanged(bool checked);
    void onZoomIn();
    void onZoomOut();
    void onZoomReset();
    void onPreviousPage();
    void onNextPage();

private:
    void setupUi();
    void setupConnections();
    QString generatePrintHtml();
    QString generatePrintCss();
    QString processHeaderFooter(const QString &text);
    void updatePageInfo();

    QWebEngineView *m_previewView;
    QPrinter *m_printer;
    PrintConfig m_config;
    QString m_htmlContent;
    QString m_documentTitle;

    // 控件
    QComboBox *m_pageSizeCombo;
    QComboBox *m_orientationCombo;
    QSpinBox *m_marginTopSpin;
    QSpinBox *m_marginBottomSpin;
    QSpinBox *m_marginLeftSpin;
    QSpinBox *m_marginRightSpin;
    QSpinBox *m_scaleSpin;
    QCheckBox *m_printBackgroundCheck;
    QCheckBox *m_printHeaderFooterCheck;
    QLabel *m_pageInfoLabel;
    QLabel *m_zoomLabel;

    int m_currentPage;
    int m_totalPages;
    int m_zoomLevel;
};

#endif // PRINTPREVIEWDIALOG_H

