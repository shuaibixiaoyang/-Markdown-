// 文件说明：app-static\export\imageexporter.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef IMAGEEXPORTER_H
#define IMAGEEXPORTER_H

#include <QObject>
#include <QString>
#include <QSize>
#include <QColor>
#include <QImage>
#include "compat/webenginecompat.h"
#include <QtSvg/QSvgGenerator>

/**
 * @brief 图片导出器
 *
 * 功能：
 * - 将 Markdown 渲染结果导出为 PNG
 * - 将 Markdown 渲染结果导出为 SVG
 * - 自定义导出尺寸和缩放
 * - 支持透明背景
 * - 支持高 DPI 导出
 * - 支持选区导出
 * - 水印支持
 */
class ImageExporter : public QObject
{
    Q_OBJECT

public:
    // 导出格式
    enum class Format {
        PNG,
        JPEG,
        SVG,
        BMP,
        WEBP
    };
    Q_ENUM(Format)

    // 导出配置
    struct ExportConfig {
        Format format;              // 导出格式
        int width;                  // 宽度 (0 = 自动)
        int height;                 // 高度 (0 = 自动)
        qreal scale;                // 缩放比例
        QColor backgroundColor;     // 背景色
        bool transparentBackground; // 透明背景 (仅 PNG)
        int quality;                // 质量 (1-100, 仅 JPEG)
        int padding;                // 内边距
        bool addShadow;             // 添加阴影
        int shadowRadius;           // 阴影半径
        QColor shadowColor;         // 阴影颜色
        bool addWatermark;          // 添加水印
        QString watermarkText;      // 水印文本
        QColor watermarkColor;      // 水印颜色
        int watermarkOpacity;       // 水印透明度 (0-255)
        QString customCss;          // 自定义 CSS

        ExportConfig()
            : format(Format::PNG)
            , width(0)
            , height(0)
            , scale(2.0)
            , backgroundColor(Qt::white)
            , transparentBackground(false)
            , quality(90)
            , padding(20)
            , addShadow(false)
            , shadowRadius(10)
            , shadowColor(QColor(0, 0, 0, 100))
            , addWatermark(false)
            , watermarkColor(QColor(128, 128, 128))
            , watermarkOpacity(50)
        {}
    };

    // 导出结果
    struct ExportResult {
        bool success;
        QString filePath;
        QSize imageSize;
        qint64 fileSize;
        QString errorMessage;

        ExportResult() : success(false), fileSize(0) {}
    };

    explicit ImageExporter(QObject *parent = nullptr);
    ~ImageExporter();

    // 导出方法
    void exportHtmlToImage(const QString &html,
                          const QString &outputPath,
                          const ExportConfig &config = ExportConfig());

    void exportHtmlToPng(const QString &html,
                         const QString &outputPath,
                         int width = 0,
                         qreal scale = 2.0);

    void exportHtmlToSvg(const QString &html,
                         const QString &outputPath,
                         int width = 0);

    void exportHtmlToJpeg(const QString &html,
                          const QString &outputPath,
                          int width = 0,
                          int quality = 90);

    // 批量导出
    void exportHtmlToImages(const QStringList &htmlList,
                           const QString &outputDir,
                           const QString &baseName,
                           const ExportConfig &config = ExportConfig());

    // 选区导出
    void exportSelectionToImage(const QString &html,
                                int startOffset,
                                int endOffset,
                                const QString &outputPath,
                                const ExportConfig &config = ExportConfig());

    // 设置
    void setDefaultConfig(const ExportConfig &config);
    ExportConfig defaultConfig() const { return m_defaultConfig; }

    // 取消
    void cancelExport();
    bool isExporting() const { return m_isExporting; }

    // 最后的结果
    ExportResult lastResult() const { return m_lastResult; }

signals:
    void exportStarted(const QString &outputPath);
    void exportProgress(int percent);
    void exportCompleted(const ExportResult &result);
    void exportFailed(const QString &error);
    void batchExportProgress(int current, int total);

private slots:
    void onLoadFinished(bool ok);
    void onContentSizeChanged();

private:
    void performExport();
    void captureImage();
    QImage renderToImage(int width, int height, qreal scale);
    void saveImage(const QImage &image, const QString &path, Format format, int quality);
    void saveSvg(const QString &html, const QString &path, int width, int height);
    QImage addPaddingAndShadow(const QImage &image);
    QImage addWatermark(const QImage &image);
    QString wrapHtmlForExport(const QString &html, int width);
    QString getFormatExtension(Format format) const;
    QString getFormatMimeType(Format format) const;

    QWebEngineView *m_webView;
    ExportConfig m_currentConfig;
    ExportConfig m_defaultConfig;
    QString m_currentHtml;
    QString m_currentOutputPath;
    ExportResult m_lastResult;
    bool m_isExporting;
    bool m_cancelled;

    // 批量导出
    QStringList m_batchHtmlList;
    QString m_batchOutputDir;
    QString m_batchBaseName;
    int m_batchCurrentIndex;
};

#endif // IMAGEEXPORTER_H

