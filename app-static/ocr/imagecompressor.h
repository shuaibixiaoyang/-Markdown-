// 文件说明：app-static\ocr\imagecompressor.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef IMAGECOMPRESSOR_H
#define IMAGECOMPRESSOR_H

#include <QObject>
#include <QImage>
#include <QPixmap>
#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QBuffer>
#include <QImageWriter>
#include <QPainter>
#include <QtConcurrent>
#include <QFutureWatcher>

/**
 * @brief 图片压缩器
 * 
 * 功能：
 * - 支持多种格式输出（WebP, JPEG, PNG）
 * - 自动调整尺寸
 * - 质量控制
 * - 批量压缩
 * - 异步处理
 */
class ImageCompressor : public QObject
{
    Q_OBJECT

public:
    // 输出格式
    enum class Format {
        WebP,       // 最佳压缩比
        JPEG,       // 兼容性好
        PNG,        // 无损
        Auto        // 自动选择
    };
    Q_ENUM(Format)

    // 压缩选项
    struct CompressionOptions {
        Format format;
        int quality;           // 0-100
        int maxWidth;           // 0 = 不限制
        int maxHeight;          // 0 = 不限制
        bool preserveAspectRatio;
        bool stripMetadata;  // 移除 EXIF 等元数据
        bool progressive;    // 渐进式加载（JPEG）
        
        CompressionOptions()
            : format(Format::WebP)
            , quality(80)
            , maxWidth(0)
            , maxHeight(0)
            , preserveAspectRatio(true)
            , stripMetadata(true)
            , progressive(true)
        {}
    };

    // 压缩结果
    struct CompressionResult {
        bool success;
        QString outputPath;
        qint64 originalSize;
        qint64 compressedSize;
        double compressionRatio;
        QSize originalDimensions;
        QSize compressedDimensions;
        Format outputFormat;
        QString errorMessage;
    };

    explicit ImageCompressor(QObject *parent = nullptr);

    // 压缩图片
    CompressionResult compress(const QString &inputPath, 
                               const QString &outputPath,
                               const CompressionOptions &options = CompressionOptions());

    CompressionResult compress(const QImage &image,
                               const QString &outputPath,
                               const CompressionOptions &options = CompressionOptions());

    CompressionResult compress(const QPixmap &pixmap,
                               const QString &outputPath,
                               const CompressionOptions &options = CompressionOptions());

    // 压缩到内存
    QByteArray compressToMemory(const QImage &image,
                                 const CompressionOptions &options = CompressionOptions());

    // 异步压缩
    void compressAsync(const QString &inputPath,
                       const QString &outputPath,
                       const CompressionOptions &options = CompressionOptions());

    // 批量压缩
    void compressBatch(const QStringList &inputPaths,
                       const QString &outputDir,
                       const CompressionOptions &options = CompressionOptions());

    // 估算压缩后大小
    qint64 estimateCompressedSize(const QImage &image, const CompressionOptions &options) const;

    // 获取格式扩展名
    static QString formatExtension(Format format);

    // 检查 WebP 支持
    static bool isWebPSupported();

    // 优化选项
    static CompressionOptions optimizeForWeb();
    static CompressionOptions optimizeForQuality();
    static CompressionOptions optimizeForSize();

signals:
    void compressionCompleted(const CompressionResult &result);
    void compressionProgress(int current, int total);
    void batchCompleted(const QVector<CompressionResult> &results);

private:
    QImage resizeIfNeeded(const QImage &image, const CompressionOptions &options) const;
    QImage removeMetadata(const QImage &image) const;
    QString determineOutputFormat(const QImage &image, Format format) const;
    bool saveImage(const QImage &image, const QString &path, 
                   const CompressionOptions &options) const;
};


// ==================== 实现 ====================

inline ImageCompressor::ImageCompressor(QObject *parent)
    : QObject(parent)
{
}

inline ImageCompressor::CompressionResult ImageCompressor::compress(
    const QString &inputPath,
    const QString &outputPath,
    const CompressionOptions &options)
{
    CompressionResult result;
    result.success = false;

    // 检查输入文件
    QFileInfo inputInfo(inputPath);
    if (!inputInfo.exists()) {
        result.errorMessage = tr("输入文件不存在");
        return result;
    }
    result.originalSize = inputInfo.size();

    // 加载图片
    QImage image(inputPath);
    if (image.isNull()) {
        result.errorMessage = tr("无法加载图片");
        return result;
    }
    result.originalDimensions = image.size();

    return compress(image, outputPath, options);
}

inline ImageCompressor::CompressionResult ImageCompressor::compress(
    const QImage &image,
    const QString &outputPath,
    const CompressionOptions &options)
{
    CompressionResult result;
    result.success = false;
    result.originalDimensions = image.size();
    
    // 估算原始大小（如果不知道的话）
    if (result.originalSize == 0) {
        result.originalSize = image.sizeInBytes();
    }

    if (image.isNull()) {
        result.errorMessage = tr("无效的图片");
        return result;
    }

    // 处理图片
    QImage processed = image;
    
    // 调整尺寸
    processed = resizeIfNeeded(processed, options);
    result.compressedDimensions = processed.size();
    
    // 移除元数据
    if (options.stripMetadata) {
        processed = removeMetadata(processed);
    }

    // 确定输出格式
    QString actualOutputPath = outputPath;
    QString formatStr = determineOutputFormat(processed, options.format);
    
    // 确保扩展名正确
    if (!actualOutputPath.endsWith("." + formatStr, Qt::CaseInsensitive)) {
        actualOutputPath = actualOutputPath + "." + formatStr;
    }
    result.outputPath = actualOutputPath;
    
    // 确定输出格式枚举
    if (formatStr == "webp") {
        result.outputFormat = Format::WebP;
    } else if (formatStr == "jpg" || formatStr == "jpeg") {
        result.outputFormat = Format::JPEG;
    } else {
        result.outputFormat = Format::PNG;
    }

    // 保存
    if (!saveImage(processed, actualOutputPath, options)) {
        result.errorMessage = tr("保存图片失败");
        return result;
    }

    // 获取压缩后大小
    QFileInfo outputInfo(actualOutputPath);
    result.compressedSize = outputInfo.size();
    result.compressionRatio = result.originalSize > 0 ? 
        static_cast<double>(result.compressedSize) / result.originalSize : 1.0;
    result.success = true;

    return result;
}

inline ImageCompressor::CompressionResult ImageCompressor::compress(
    const QPixmap &pixmap,
    const QString &outputPath,
    const CompressionOptions &options)
{
    return compress(pixmap.toImage(), outputPath, options);
}

inline QByteArray ImageCompressor::compressToMemory(
    const QImage &image,
    const CompressionOptions &options)
{
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);

    QImage processed = resizeIfNeeded(image, options);
    if (options.stripMetadata) {
        processed = removeMetadata(processed);
    }

    QString format = determineOutputFormat(processed, options.format);
    
    QImageWriter writer(&buffer, format.toUtf8());
    writer.setQuality(options.quality);
    
    if (format == "jpeg" || format == "jpg") {
        writer.setOptimizedWrite(true);
        writer.setProgressiveScanWrite(options.progressive);
    }
    
    writer.write(processed);
    buffer.close();

    return data;
}

inline void ImageCompressor::compressAsync(
    const QString &inputPath,
    const QString &outputPath,
    const CompressionOptions &options)
{
    QFutureWatcher<CompressionResult> *watcher = new QFutureWatcher<CompressionResult>(this);
    
    connect(watcher, &QFutureWatcher<CompressionResult>::finished, this, [this, watcher]() {
        emit compressionCompleted(watcher->result());
        watcher->deleteLater();
    });

    QFuture<CompressionResult> future = QtConcurrent::run([this, inputPath, outputPath, options]() {
        return compress(inputPath, outputPath, options);
    });

    watcher->setFuture(future);
}

inline void ImageCompressor::compressBatch(
    const QStringList &inputPaths,
    const QString &outputDir,
    const CompressionOptions &options)
{
    QtConcurrent::run([this, inputPaths, outputDir, options]() {
        QVector<CompressionResult> results;
        int total = inputPaths.size();
        int current = 0;

        for (const QString &inputPath : inputPaths) {
            QFileInfo info(inputPath);
            QString outputPath = outputDir + "/" + info.completeBaseName();
            
            CompressionResult result = compress(inputPath, outputPath, options);
            results.append(result);
            
            current++;
            emit compressionProgress(current, total);
        }

        emit batchCompleted(results);
    });
}

inline qint64 ImageCompressor::estimateCompressedSize(
    const QImage &image, 
    const CompressionOptions &options) const
{
    // 简单估算
    QImage processed = resizeIfNeeded(image, options);
    int pixels = processed.width() * processed.height();
    
    double bytesPerPixel;
    switch (options.format) {
        case Format::WebP:
            bytesPerPixel = 0.15 * (100 - options.quality) / 100.0 + 0.05;
            break;
        case Format::JPEG:
            bytesPerPixel = 0.2 * (100 - options.quality) / 100.0 + 0.1;
            break;
        case Format::PNG:
            bytesPerPixel = 0.8;  // PNG 是无损的
            break;
        default:
            bytesPerPixel = 0.15;
    }
    
    return static_cast<qint64>(pixels * bytesPerPixel);
}

inline QString ImageCompressor::formatExtension(Format format)
{
    switch (format) {
        case Format::WebP: return "webp";
        case Format::JPEG: return "jpg";
        case Format::PNG: return "png";
        default: return "webp";
    }
}

inline bool ImageCompressor::isWebPSupported()
{
    return QImageWriter::supportedImageFormats().contains("webp");
}

inline ImageCompressor::CompressionOptions ImageCompressor::optimizeForWeb()
{
    CompressionOptions opts;
    opts.format = isWebPSupported() ? Format::WebP : Format::JPEG;
    opts.quality = 75;
    opts.maxWidth = 1920;
    opts.maxHeight = 1080;
    opts.stripMetadata = true;
    opts.progressive = true;
    return opts;
}

inline ImageCompressor::CompressionOptions ImageCompressor::optimizeForQuality()
{
    CompressionOptions opts;
    opts.format = Format::PNG;
    opts.quality = 100;
    opts.maxWidth = 0;
    opts.maxHeight = 0;
    opts.stripMetadata = false;
    return opts;
}

inline ImageCompressor::CompressionOptions ImageCompressor::optimizeForSize()
{
    CompressionOptions opts;
    opts.format = isWebPSupported() ? Format::WebP : Format::JPEG;
    opts.quality = 60;
    opts.maxWidth = 1280;
    opts.maxHeight = 720;
    opts.stripMetadata = true;
    opts.progressive = true;
    return opts;
}

inline QImage ImageCompressor::resizeIfNeeded(
    const QImage &image, 
    const CompressionOptions &options) const
{
    if (options.maxWidth <= 0 && options.maxHeight <= 0) {
        return image;
    }

    int targetWidth = options.maxWidth > 0 ? options.maxWidth : image.width();
    int targetHeight = options.maxHeight > 0 ? options.maxHeight : image.height();

    if (image.width() <= targetWidth && image.height() <= targetHeight) {
        return image;
    }

    if (options.preserveAspectRatio) {
        return image.scaled(targetWidth, targetHeight, 
                           Qt::KeepAspectRatio, Qt::SmoothTransformation);
    } else {
        return image.scaled(targetWidth, targetHeight, 
                           Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
}

inline QImage ImageCompressor::removeMetadata(const QImage &image) const
{
    // 创建干净的副本，不带任何元数据
    QImage clean(image.size(), image.format());
    clean.fill(Qt::transparent);
    
    QPainter painter(&clean);
    painter.drawImage(0, 0, image);
    painter.end();
    
    return clean;
}

inline QString ImageCompressor::determineOutputFormat(
    const QImage &image, 
    Format format) const
{
    if (format == Format::Auto) {
        // 如果有透明度，使用 PNG 或 WebP
        if (image.hasAlphaChannel()) {
            return isWebPSupported() ? "webp" : "png";
        }
        // 否则使用 WebP 或 JPEG
        return isWebPSupported() ? "webp" : "jpg";
    }
    
    return formatExtension(format);
}

inline bool ImageCompressor::saveImage(
    const QImage &image, 
    const QString &path,
    const CompressionOptions &options) const
{
    QImageWriter writer(path);
    writer.setQuality(options.quality);
    
    QString format = QFileInfo(path).suffix().toLower();
    
    if (format == "jpg" || format == "jpeg") {
        writer.setOptimizedWrite(true);
        writer.setProgressiveScanWrite(options.progressive);
    }
    
    return writer.write(image);
}

#endif // IMAGECOMPRESSOR_H

