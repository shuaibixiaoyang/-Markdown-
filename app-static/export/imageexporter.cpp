// 文件说明：app-static\export\imageexporter.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "imageexporter.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QPainter>
#include <QBuffer>
#include <QEventLoop>
#include <QTimer>
#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QSvgGenerator>
#include "compat/webenginecompat.h"
#include <QDebug>

// 函数说明：构造 ImageExporter 对象，初始化本模块需要的状态、界面和资源。
ImageExporter::ImageExporter(QObject *parent)
    : QObject(parent)
    , m_webView(new QWebEngineView())
    , m_isExporting(false)
    , m_cancelled(false)
    , m_batchCurrentIndex(0)
{
    m_webView->setVisible(false);
    m_webView->settings()->setAttribute(QWebEngineSettings::ShowScrollBars, false);

    connect(m_webView, &QWebEngineView::loadFinished,
            this, &ImageExporter::onLoadFinished);
}

// 函数说明：销毁 ImageExporter 对象，释放本模块持有的资源。
ImageExporter::~ImageExporter()
{
    delete m_webView;
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
void ImageExporter::exportHtmlToImage(const QString &html,
                                       const QString &outputPath,
                                       const ExportConfig &config)
{
    if (m_isExporting) {
        emit exportFailed(tr("导出正在进行中"));
        return;
    }

    m_isExporting = true;
    m_cancelled = false;
    m_currentConfig = config;
    m_currentHtml = html;
    m_currentOutputPath = outputPath;

    m_lastResult = ExportResult();
    m_lastResult.filePath = outputPath;

    emit exportStarted(outputPath);
    emit exportProgress(10);

    // 设置页面大小
    int width = config.width > 0 ? config.width : 800;
    m_webView->resize(width, 600);

    // 加载 HTML
    QString wrappedHtml = wrapHtmlForExport(html, width);
    m_webView->setHtml(wrappedHtml);
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
void ImageExporter::exportHtmlToPng(const QString &html,
                                     const QString &outputPath,
                                     int width,
                                     qreal scale)
{
    ExportConfig config;
    config.format = Format::PNG;
    config.width = width;
    config.scale = scale;

    exportHtmlToImage(html, outputPath, config);
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
void ImageExporter::exportHtmlToSvg(const QString &html,
                                     const QString &outputPath,
                                     int width)
{
    ExportConfig config;
    config.format = Format::SVG;
    config.width = width;
    config.scale = 1.0;

    exportHtmlToImage(html, outputPath, config);
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
void ImageExporter::exportHtmlToJpeg(const QString &html,
                                      const QString &outputPath,
                                      int width,
                                      int quality)
{
    ExportConfig config;
    config.format = Format::JPEG;
    config.width = width;
    config.quality = quality;

    exportHtmlToImage(html, outputPath, config);
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
void ImageExporter::exportHtmlToImages(const QStringList &htmlList,
                                        const QString &outputDir,
                                        const QString &baseName,
                                        const ExportConfig &config)
{
    if (m_isExporting) {
        emit exportFailed(tr("导出正在进行中"));
        return;
    }

    if (htmlList.isEmpty()) {
        emit exportFailed(tr("没有内容可导出"));
        return;
    }

    QDir().mkpath(outputDir);

    m_batchHtmlList = htmlList;
    m_batchOutputDir = outputDir;
    m_batchBaseName = baseName;
    m_batchCurrentIndex = 0;
    m_currentConfig = config;

    emit batchExportProgress(0, htmlList.size());

    // 开始第一个导出
    QString ext = getFormatExtension(config.format);
    QString outputPath = QString("%1/%2_%3.%4")
        .arg(outputDir)
        .arg(baseName)
        .arg(m_batchCurrentIndex + 1, 3, 10, QChar('0'))
        .arg(ext);

    exportHtmlToImage(htmlList.first(), outputPath, config);
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
void ImageExporter::exportSelectionToImage(const QString &html,
                                            int startOffset,
                                            int endOffset,
                                            const QString &outputPath,
                                            const ExportConfig &config)
{
    Q_UNUSED(startOffset)
    Q_UNUSED(endOffset)

    // 简化实现：直接导出整个 HTML
    // 完整实现需要在 HTML 中标记选区并仅渲染该部分
    exportHtmlToImage(html, outputPath, config);
}

// 函数说明：设置 ImageExporter 的运行参数，并触发必要的界面或数据刷新。
void ImageExporter::setDefaultConfig(const ExportConfig &config)
{
    m_defaultConfig = config;
}

// 函数说明：实现 ImageExporter::cancelExport 的核心逻辑，供当前模块调用。
void ImageExporter::cancelExport()
{
    m_cancelled = true;
    m_isExporting = false;
    m_batchHtmlList.clear();
}

// 函数说明：响应 ImageExporter 收到的信号或异步回调，并更新界面状态。
void ImageExporter::onLoadFinished(bool ok)
{
    if (!ok) {
        m_lastResult.success = false;
        m_lastResult.errorMessage = tr("页面加载失败");
        m_isExporting = false;
        emit exportFailed(m_lastResult.errorMessage);
        return;
    }

    if (m_cancelled) {
        m_isExporting = false;
        return;
    }

    emit exportProgress(30);

    // 等待页面渲染完成
    QTimer::singleShot(500, this, &ImageExporter::performExport);
}

// 函数说明：响应 ImageExporter 收到的信号或异步回调，并更新界面状态。
void ImageExporter::onContentSizeChanged()
{
    // 内容大小变化时可以调整渲染
}

// 函数说明：实现 ImageExporter::performExport 的核心逻辑，供当前模块调用。
void ImageExporter::performExport()
{
    if (m_cancelled) {
        m_isExporting = false;
        return;
    }

    emit exportProgress(50);

    // 获取内容大小
    m_webView->page()->runJavaScript(
        "document.body.scrollHeight",
        [this](const QVariant &result) {
            int contentHeight = result.toInt();
            if (contentHeight < 100) contentHeight = 600;

            emit exportProgress(70);
            captureImage();
        });
}

// 函数说明：实现 ImageExporter::captureImage 的核心逻辑，供当前模块调用。
void ImageExporter::captureImage()
{
    if (m_cancelled) {
        m_isExporting = false;
        return;
    }

    int width = m_currentConfig.width > 0 ? m_currentConfig.width : m_webView->width();

    // 获取实际内容高度
    m_webView->page()->runJavaScript(
        "document.body.scrollHeight",
        [this, width](const QVariant &result) {
            int height = result.toInt();
            if (height < 100) height = 600;

            // 添加内边距
            height += m_currentConfig.padding * 2;

            if (m_currentConfig.format == Format::SVG) {
                saveSvg(m_currentHtml, m_currentOutputPath, width, height);
            } else {
                QImage image = renderToImage(width, height, m_currentConfig.scale);

                if (m_currentConfig.padding > 0 || m_currentConfig.addShadow) {
                    image = addPaddingAndShadow(image);
                }

                if (m_currentConfig.addWatermark && !m_currentConfig.watermarkText.isEmpty()) {
                    image = addWatermark(image);
                }

                saveImage(image, m_currentOutputPath, m_currentConfig.format, m_currentConfig.quality);
            }
        });
}

// 函数说明：渲染 ImageExporter 的显示内容或导出片段。
QImage ImageExporter::renderToImage(int width, int height, qreal scale)
{
    int scaledWidth = static_cast<int>(width * scale);
    int scaledHeight = static_cast<int>(height * scale);

    QImage image(scaledWidth, scaledHeight, QImage::Format_ARGB32);

    if (m_currentConfig.transparentBackground) {
        image.fill(Qt::transparent);
    } else {
        image.fill(m_currentConfig.backgroundColor);
    }

    // 使用 QWebEngineView 的 grab 方法
    m_webView->resize(width, height);
    QPixmap pixmap = m_webView->grab();

    // 缩放
    if (scale != 1.0) {
        pixmap = pixmap.scaled(scaledWidth, scaledHeight,
                               Qt::KeepAspectRatio,
                               Qt::SmoothTransformation);
    }

    return pixmap.toImage();
}

// 函数说明：保存 ImageExporter 当前状态，保证用户修改可以持久化。
void ImageExporter::saveImage(const QImage &image, const QString &path,
                               Format format, int quality)
{
    emit exportProgress(90);

    QString formatStr;
    switch (format) {
        case Format::PNG: formatStr = "PNG"; break;
        case Format::JPEG: formatStr = "JPEG"; break;
        case Format::BMP: formatStr = "BMP"; break;
        case Format::WEBP: formatStr = "WEBP"; break;
        default: formatStr = "PNG"; break;
    }

    bool success = image.save(path, formatStr.toLatin1().constData(),
                              format == Format::JPEG ? quality : -1);

    m_lastResult.success = success;
    m_lastResult.imageSize = image.size();

    if (success) {
        QFileInfo info(path);
        m_lastResult.fileSize = info.size();
    } else {
        m_lastResult.errorMessage = tr("保存图片失败");
    }

    emit exportProgress(100);

    // 处理批量导出
    if (!m_batchHtmlList.isEmpty()) {
        m_batchCurrentIndex++;
        emit batchExportProgress(m_batchCurrentIndex, m_batchHtmlList.size());

        if (m_batchCurrentIndex < m_batchHtmlList.size() && !m_cancelled) {
            // 继续下一个
            QString ext = getFormatExtension(m_currentConfig.format);
            QString outputPath = QString("%1/%2_%3.%4")
                .arg(m_batchOutputDir)
                .arg(m_batchBaseName)
                .arg(m_batchCurrentIndex + 1, 3, 10, QChar('0'))
                .arg(ext);

            m_isExporting = false;
            exportHtmlToImage(m_batchHtmlList[m_batchCurrentIndex], outputPath, m_currentConfig);
            return;
        } else {
            m_batchHtmlList.clear();
        }
    }

    m_isExporting = false;

    if (success) {
        emit exportCompleted(m_lastResult);
    } else {
        emit exportFailed(m_lastResult.errorMessage);
    }
}

// 函数说明：保存 ImageExporter 当前状态，保证用户修改可以持久化。
void ImageExporter::saveSvg(const QString &html, const QString &path,
                             int width, int height)
{
    emit exportProgress(90);

    QSvgGenerator generator;
    generator.setFileName(path);
    generator.setSize(QSize(width, height));
    generator.setViewBox(QRect(0, 0, width, height));
    generator.setTitle(tr("CuteMarkEd 导出"));
    generator.setDescription(tr("由 CuteMarkEd 生成的 SVG 文档"));

    QPainter painter;
    painter.begin(&generator);

    // 渲染 WebView 到 SVG
    m_webView->resize(width, height);
    m_webView->render(&painter);

    painter.end();

    QFileInfo info(path);
    m_lastResult.success = info.exists();
    m_lastResult.imageSize = QSize(width, height);
    m_lastResult.fileSize = info.size();

    emit exportProgress(100);

    m_isExporting = false;

    if (m_lastResult.success) {
        emit exportCompleted(m_lastResult);
    } else {
        m_lastResult.errorMessage = tr("保存 SVG 失败");
        emit exportFailed(m_lastResult.errorMessage);
    }
}

// 函数说明：向 ImageExporter 管理的数据集合中添加一项内容。
QImage ImageExporter::addPaddingAndShadow(const QImage &image)
{
    int padding = m_currentConfig.padding;
    int shadowOffset = m_currentConfig.addShadow ? m_currentConfig.shadowRadius : 0;

    int newWidth = image.width() + padding * 2 + shadowOffset;
    int newHeight = image.height() + padding * 2 + shadowOffset;

    QImage result(newWidth, newHeight, QImage::Format_ARGB32);

    if (m_currentConfig.transparentBackground) {
        result.fill(Qt::transparent);
    } else {
        result.fill(m_currentConfig.backgroundColor);
    }

    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 绘制阴影
    if (m_currentConfig.addShadow) {
        QImage shadow(image.size(), QImage::Format_ARGB32);
        shadow.fill(m_currentConfig.shadowColor);

        // 简化的阴影效果
        painter.setOpacity(0.3);
        painter.drawImage(padding + shadowOffset / 2, padding + shadowOffset / 2, shadow);
        painter.setOpacity(1.0);
    }

    // 绘制原图
    painter.drawImage(padding, padding, image);

    painter.end();
    return result;
}

// 函数说明：向 ImageExporter 管理的数据集合中添加一项内容。
QImage ImageExporter::addWatermark(const QImage &image)
{
    QImage result = image.copy();
    QPainter painter(&result);

    QFont font = painter.font();
    font.setPointSize(12);
    painter.setFont(font);

    QColor color = m_currentConfig.watermarkColor;
    color.setAlpha(m_currentConfig.watermarkOpacity);
    painter.setPen(color);

    // 在右下角绘制水印
    QRect textRect = painter.fontMetrics().boundingRect(m_currentConfig.watermarkText);
    int x = result.width() - textRect.width() - 20;
    int y = result.height() - 20;

    painter.drawText(x, y, m_currentConfig.watermarkText);

    painter.end();
    return result;
}

// 函数说明：实现 ImageExporter::wrapHtmlForExport 的核心逻辑，供当前模块调用。
QString ImageExporter::wrapHtmlForExport(const QString &html, int width)
{
    QString css = m_currentConfig.customCss.isEmpty() ? R"(
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            font-size: 16px;
            line-height: 1.6;
            color: #333;
            margin: 0;
            padding: 20px;
            background: transparent;
        }
        h1, h2, h3, h4, h5, h6 { margin-top: 1.5em; margin-bottom: 0.5em; }
        h1 { font-size: 2em; }
        h2 { font-size: 1.5em; }
        h3 { font-size: 1.25em; }
        p { margin: 1em 0; }
        pre { background: #f5f5f5; padding: 15px; overflow-x: auto; border-radius: 5px; }
        code { background: #f5f5f5; padding: 2px 5px; border-radius: 3px; }
        blockquote { border-left: 4px solid #ddd; margin: 0; padding-left: 20px; color: #666; }
        img { max-width: 100%; height: auto; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background: #f5f5f5; }
        ul, ol { padding-left: 2em; }
        hr { border: none; border-top: 1px solid #ddd; margin: 2em 0; }
    )" : m_currentConfig.customCss;

    QString bgColor = m_currentConfig.transparentBackground ?
                      "transparent" :
                      m_currentConfig.backgroundColor.name();

    return QString(R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=%1, initial-scale=1">
    <style>
        html { background: %2; }
        %3
    </style>
</head>
<body>
%4
</body>
</html>
)").arg(width).arg(bgColor).arg(css).arg(html);
}

// 函数说明：读取 ImageExporter 当前保存的状态或计算结果。
QString ImageExporter::getFormatExtension(Format format) const
{
    switch (format) {
        case Format::PNG: return "png";
        case Format::JPEG: return "jpg";
        case Format::SVG: return "svg";
        case Format::BMP: return "bmp";
        case Format::WEBP: return "webp";
        default: return "png";
    }
}

// 函数说明：读取 ImageExporter 当前保存的状态或计算结果。
QString ImageExporter::getFormatMimeType(Format format) const
{
    switch (format) {
        case Format::PNG: return "image/png";
        case Format::JPEG: return "image/jpeg";
        case Format::SVG: return "image/svg+xml";
        case Format::BMP: return "image/bmp";
        case Format::WEBP: return "image/webp";
        default: return "image/png";
    }
}

