// 文件说明：app-static\ocr\ocrengine.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "ocrengine.h"

#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QSet>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QtMath>
#include <limits>
#include <algorithm>
#include <utility>

// Tesseract 头文件
#ifdef ENABLE_TESSERACT
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#endif

namespace {

constexpr int kDefaultInputDpi = 150;
constexpr qreal kDpiPerMeter = 39.37007874015748; // 1 / 0.0254
constexpr qreal kMaxPreprocessUpscale = 2.0;
constexpr qint64 kPreprocessPixelBudget = 25000000;
constexpr int kMinImageSide = 32;

constexpr auto kOcrSettingsGroup = "ocr";
constexpr auto kTessdataPathKey = "tessdataPath";

qint64 pixelCount(const QImage &image)
{
    return static_cast<qint64>(image.width()) * static_cast<qint64>(image.height());
}

qint64 imageBytes(const QImage &image)
{
    const qint64 bytes = image.sizeInBytes();
    if (bytes > 0) {
        return bytes;
    }
    return static_cast<qint64>(image.bytesPerLine()) * static_cast<qint64>(image.height());
}

int estimateDpi(const QImage &image)
{
    if (image.dotsPerMeterX() > 0 && image.dotsPerMeterY() > 0) {
        const qreal avgDpi = (image.dotsPerMeterX() + image.dotsPerMeterY()) / (2.0 * kDpiPerMeter);
        return qMax(1, qRound(avgDpi));
    }
    return kDefaultInputDpi;
}

QSize scaledSize(const QSize &size, qreal scale)
{
    const int width = qMax(kMinImageSide, qRound(size.width() * scale));
    const int height = qMax(kMinImageSide, qRound(size.height() * scale));
    return QSize(width, height);
}

} // namespace

// 函数说明：构造 OcrEngine 对象，初始化本模块需要的状态、界面和资源。
OcrEngine::OcrEngine(QObject *parent)
    : QObject(parent)
    , m_engineType(EngineType::Tesseract)
    , m_initialized(false)
    , m_resolvedDataPath(QString())
    , m_lastInitializationError(QString())
    , m_tesseract(nullptr)
{
}

// 函数说明：销毁 OcrEngine 对象，释放本模块持有的资源。
OcrEngine::~OcrEngine()
{
    cleanupTesseract();
}

// 函数说明：执行 OcrEngine 的启动初始化流程，把延后加载的功能接入主界面。
bool OcrEngine::initialize(const QString &dataPath)
{
    m_resolvedDataPath.clear();

    if (!dataPath.trimmed().isEmpty()) {
        setTessdataPath(dataPath);
    }

    QStringList diagnostics;
    const QStringList candidates = tessdataSearchPaths(dataPath);
    const QStringList requestedLanguages =
        m_currentLanguages.isEmpty() ? QStringList({QStringLiteral("eng")}) : m_currentLanguages;

    for (const QString &candidate : candidates) {
        const QString tessdataDir = normalizeTessdataCandidate(candidate);
        if (tessdataDir.isEmpty()) {
            continue;
        }

        const QStringList missingLangs = missingLanguageFiles(tessdataDir, requestedLanguages);
        if (!missingLangs.isEmpty()) {
            diagnostics << tr("路径 %1 缺少语言包: %2")
                               .arg(tessdataDir, missingLangs.join(QStringLiteral(", ")));
            continue;
        }

        if (initTesseract(tessdataDir)) {
            m_resolvedDataPath = tessdataDir;
            m_dataPath = tessdataDir;
            persistTessdataPath(tessdataDir);
            m_lastInitializationError.clear();
            return true;
        }

        if (!m_lastInitializationError.isEmpty()) {
            diagnostics << m_lastInitializationError;
        }
    }

    if (diagnostics.isEmpty()) {
        diagnostics << tr("未找到可用的 tessdata 目录");
    }

    m_lastInitializationError = diagnostics.join(QStringLiteral("\n"));
    emit errorOccurred(m_lastInitializationError);
    m_initialized = false;
    return false;
}

// 函数说明：设置 OcrEngine 的运行参数，并触发必要的界面或数据刷新。
void OcrEngine::setTessdataPath(const QString &path)
{
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty()) {
        m_dataPath.clear();
        m_resolvedDataPath.clear();
        persistTessdataPath(QString());
        return;
    }

    const QString normalized = normalizeTessdataCandidate(trimmed);
    if (!normalized.isEmpty()) {
        m_dataPath = normalized;
    } else {
        QFileInfo info(trimmed);
        m_dataPath = QDir::cleanPath(info.isAbsolute()
                                         ? info.absoluteFilePath()
                                         : QDir::current().absoluteFilePath(trimmed));
    }
    m_resolvedDataPath.clear();
    persistTessdataPath(m_dataPath);
}

// 函数说明：设置 OcrEngine 的运行参数，并触发必要的界面或数据刷新。
bool OcrEngine::setLanguages(const QStringList &languages)
{
    m_currentLanguages = languages;
    m_currentLanguages.removeAll(QString());
    if (m_currentLanguages.isEmpty()) {
        m_currentLanguages << QStringLiteral("eng");
    }

    if (!m_initialized) {
        return true;
    }

    cleanupTesseract();
    return initialize(m_dataPath);
}

// 函数说明：实现 OcrEngine::availableLanguages 的核心逻辑，供当前模块调用。
QStringList OcrEngine::availableLanguages() const
{
    QStringList languages;

    const QString tessdataDirPath =
        !m_resolvedDataPath.isEmpty() ? m_resolvedDataPath : normalizeTessdataCandidate(m_dataPath);
    QDir tessDataDir(tessdataDirPath);
    if (tessDataDir.exists()) {
        QStringList trainedDataFiles = tessDataDir.entryList(
            QStringList() << QStringLiteral("*.traineddata"), QDir::Files);

        for (const QString &file : trainedDataFiles) {
            QString lang = file;
            lang.replace(QStringLiteral(".traineddata"), QString());
            languages << lang;
        }
    }

    return languages;
}

// 函数说明：实现 OcrEngine::recognize 的核心逻辑，供当前模块调用。
OcrEngine::OcrResult OcrEngine::recognize(const QImage &image, const OcrOptions &options)
{
    OcrResult result;
    result.success = false;
    
    if (!m_initialized) {
        result.errorMessage = tr("OCR 引擎未初始化");
        emit errorOccurred(result.errorMessage);
        return result;
    }
    
    if (image.isNull()) {
        result.errorMessage = tr("无效的图片");
        emit errorOccurred(result.errorMessage);
        return result;
    }
    
    QElapsedTimer timer;
    timer.start();

    QString downscaleNotice;
    QImage source = clampImageForOcr(image, options, &downscaleNotice);
    if (!downscaleNotice.isEmpty()) {
        qInfo() << downscaleNotice;
    }

    // 预处理
    QImage processed = source;
    if (options.preprocess) {
        processed = preprocessImage(source, options.dpi);
        processed = clampImageForOcr(processed, options, nullptr);
    }

    emit recognitionProgress(10);

    // 执行识别
    result = recognizeWithTesseract(processed, options);

    emit recognitionProgress(80);

    // 检测表格
    if (options.detectTables && result.success) {
        result.tables = TableDetector::detectTables(processed);
    }
    
    emit recognitionProgress(100);
    
    result.processingTimeMs = timer.elapsed();
    
    return result;
}

// 函数说明：实现 OcrEngine::recognize 的核心逻辑，供当前模块调用。
OcrEngine::OcrResult OcrEngine::recognize(const QPixmap &pixmap, const OcrOptions &options)
{
    return recognize(pixmap.toImage(), options);
}

// 函数说明：实现 OcrEngine::recognize 的核心逻辑，供当前模块调用。
OcrEngine::OcrResult OcrEngine::recognize(const QString &imagePath, const OcrOptions &options)
{
    QImage image(imagePath);
    if (image.isNull()) {
        OcrResult result;
        result.success = false;
        result.errorMessage = tr("无法加载图片: %1").arg(imagePath);
        return result;
    }
    return recognize(image, options);
}

// 函数说明：实现 OcrEngine::recognizeAsync 的核心逻辑，供当前模块调用。
void OcrEngine::recognizeAsync(const QImage &image, const OcrOptions &options)
{
    QFutureWatcher<OcrResult> *watcher = new QFutureWatcher<OcrResult>(this);
    
    connect(watcher, &QFutureWatcher<OcrResult>::finished, this, [this, watcher]() {
        emit recognitionCompleted(watcher->result());
        watcher->deleteLater();
    });
    
    QFuture<OcrResult> future = QtConcurrent::run([this, image, options]() {
        return recognize(image, options);
    });
    
    watcher->setFuture(future);
}

// 函数说明：实现 OcrEngine::recognizeAsync 的核心逻辑，供当前模块调用。
void OcrEngine::recognizeAsync(const QString &imagePath, const OcrOptions &options)
{
    QImage image(imagePath);
    recognizeAsync(image, options);
}

// 函数说明：实现 OcrEngine::recognizeTable 的核心逻辑，供当前模块调用。
OcrEngine::Table OcrEngine::recognizeTable(const QImage &image)
{
    QVector<Table> tables = TableDetector::detectTables(image);
    
    if (tables.isEmpty()) {
        return Table();
    }
    
    // 对每个单元格进行 OCR
    Table &table = tables.first();
    
    for (TableCell &cell : table.cells) {
        if (cell.boundingBox.isValid()) {
            QImage cellImage = image.copy(cell.boundingBox);
            OcrOptions opts;
            opts.mode = RecognitionMode::SingleLine;
            opts.detectTables = false;
            
            OcrResult result = recognize(cellImage, opts);
            if (result.success) {
                cell.text = result.fullText.trimmed();
            }
        }
    }
    
    return table;
}

// 函数说明：实现 OcrEngine::toMarkdownText 的核心逻辑，供当前模块调用。
QString OcrEngine::toMarkdownText(const OcrResult &result) const
{
    return OcrToMarkdown::convertText(result);
}

// 函数说明：实现 OcrEngine::toMarkdownTable 的核心逻辑，供当前模块调用。
QString OcrEngine::toMarkdownTable(const Table &table) const
{
    return OcrToMarkdown::convertTable(table);
}

// 函数说明：实现 OcrEngine::preprocessImage 的核心逻辑，供当前模块调用。
QImage OcrEngine::preprocessImage(const QImage &image, int targetDpi)
{
    QImage processed = image;

    // 转换为灰度
    if (processed.format() != QImage::Format_Grayscale8) {
        processed = processed.convertToFormat(QImage::Format_Grayscale8);
    }

    // 基于图像 DPI 做有限放大，避免极端分辨率导致内存爆炸。
    const int sourceDpi = estimateDpi(processed);
    qreal scale = static_cast<qreal>(qMax(1, targetDpi)) / static_cast<qreal>(sourceDpi);
    scale = qBound<qreal>(1.0, scale, kMaxPreprocessUpscale);

    const qint64 sourcePixels = pixelCount(processed);
    if (sourcePixels > 0 && scale > 1.0) {
        const qreal budgetScale = qSqrt(static_cast<qreal>(kPreprocessPixelBudget) /
                                        static_cast<qreal>(sourcePixels));
        scale = qMin(scale, budgetScale);
    }

    if (scale > 1.01) {
        processed = processed.scaled(scaledSize(processed.size(), scale),
                                     Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation);
    }

    // 二值化
    processed = binarize(processed);

    return processed;
}

// 函数说明：实现 OcrEngine::binarize 的核心逻辑，供当前模块调用。
QImage OcrEngine::binarize(const QImage &image, int threshold)
{
    QImage binary = image.convertToFormat(QImage::Format_Grayscale8);
    
    for (int y = 0; y < binary.height(); ++y) {
        uchar *line = binary.scanLine(y);
        for (int x = 0; x < binary.width(); ++x) {
            line[x] = line[x] > threshold ? 255 : 0;
        }
    }
    
    return binary;
}

// 函数说明：实现 OcrEngine::deskew 的核心逻辑，供当前模块调用。
QImage OcrEngine::deskew(const QImage &image)
{
    // 简化实现：使用霍夫变换检测倾斜角度
    // 完整实现需要更复杂的算法
    return image;
}

// 函数说明：从 OcrEngine 管理的数据集合中移除指定内容。
QImage OcrEngine::removeNoise(const QImage &image)
{
    // 简单的中值滤波降噪
    QImage denoised = image;
    
    for (int y = 1; y < image.height() - 1; ++y) {
        for (int x = 1; x < image.width() - 1; ++x) {
            QVector<int> neighbors;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    neighbors.append(qGray(image.pixel(x + dx, y + dy)));
                }
            }
            std::sort(neighbors.begin(), neighbors.end());
            int median = neighbors[4];
            denoised.setPixel(x, y, qRgb(median, median, median));
        }
    }
    
    return denoised;
}

// 函数说明：实现 OcrEngine::tessdataSearchPaths 的核心逻辑，供当前模块调用。
QStringList OcrEngine::tessdataSearchPaths(const QString &requestedPath) const
{
    QStringList searchPaths;
    QSet<QString> seen;

    auto addPath = [&](const QString &path) {
        const QString cleaned = QDir::cleanPath(path.trimmed());
        if (cleaned.isEmpty() || seen.contains(cleaned)) {
            return;
        }
        seen.insert(cleaned);
        searchPaths << cleaned;
    };

    if (!requestedPath.trimmed().isEmpty()) {
        addPath(requestedPath);
    }

    const QByteArray envPath = qgetenv("TESSDATA_PREFIX");
    if (!envPath.isEmpty()) {
        addPath(QString::fromLocal8Bit(envPath));
    }

    const QString persisted = loadPersistedTessdataPath();
    if (!persisted.isEmpty()) {
        addPath(persisted);
    }

    if (!m_dataPath.isEmpty()) {
        addPath(m_dataPath);
    }

    addPath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/tessdata"));
    addPath(QCoreApplication::applicationDirPath() + QStringLiteral("/tessdata"));
    addPath(QStringLiteral("/usr/share/tesseract-ocr/5/tessdata"));
    addPath(QStringLiteral("/usr/share/tesseract-ocr/4.00/tessdata"));
    addPath(QStringLiteral("/usr/share/tessdata"));
    addPath(QStringLiteral("/usr/local/share/tessdata"));
    addPath(QStringLiteral("/opt/homebrew/share/tessdata"));

    return searchPaths;
}

// 函数说明：实现 OcrEngine::normalizeTessdataCandidate 的核心逻辑，供当前模块调用。
QString OcrEngine::normalizeTessdataCandidate(const QString &candidate)
{
    const QString trimmed = candidate.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }

    QFileInfo info(trimmed);
    const QString normalizedPath = QDir::cleanPath(info.isAbsolute()
                                                       ? info.absoluteFilePath()
                                                       : QDir::current().absoluteFilePath(trimmed));

    QFileInfo normalizedInfo(normalizedPath);
    if (normalizedInfo.isFile()) {
        if (normalizedInfo.suffix() == QStringLiteral("traineddata")) {
            return normalizedInfo.absolutePath();
        }
        return QString();
    }

    QDir dir(normalizedPath);
    if (!dir.exists()) {
        return QString();
    }

    if (dir.dirName().compare(QStringLiteral("tessdata"), Qt::CaseInsensitive) == 0) {
        return dir.absolutePath();
    }

    if (!dir.entryList(QStringList() << QStringLiteral("*.traineddata"), QDir::Files).isEmpty()) {
        return dir.absolutePath();
    }

    const QDir childTessdata(dir.filePath(QStringLiteral("tessdata")));
    if (childTessdata.exists()) {
        return childTessdata.absolutePath();
    }

    return QString();
}

// 函数说明：实现 OcrEngine::missingLanguageFiles 的核心逻辑，供当前模块调用。
QStringList OcrEngine::missingLanguageFiles(const QString &tessdataDir, const QStringList &languages)
{
    QStringList missing;
    const QStringList requested =
        languages.isEmpty() ? QStringList({QStringLiteral("eng")}) : languages;
    QDir dir(tessdataDir);

    for (const QString &lang : requested) {
        const QString language = lang.trimmed();
        if (language.isEmpty()) {
            continue;
        }
        const QString trainedData = dir.filePath(language + QStringLiteral(".traineddata"));
        if (!QFileInfo::exists(trainedData)) {
            missing << language;
        }
    }

    return missing;
}

// 函数说明：实现 OcrEngine::parentForTessdata 的核心逻辑，供当前模块调用。
QString OcrEngine::parentForTessdata(const QString &tessdataDir)
{
    QDir dir(tessdataDir);
    if (dir.dirName().compare(QStringLiteral("tessdata"), Qt::CaseInsensitive) == 0) {
        dir.cdUp();
        return dir.absolutePath();
    }
    return tessdataDir;
}

// 函数说明：实现 OcrEngine::clampImageForOcr 的核心逻辑，供当前模块调用。
QImage OcrEngine::clampImageForOcr(const QImage &image, const OcrOptions &options, QString *notice)
{
    if (notice) {
        notice->clear();
    }
    if (image.isNull() || !options.autoDownscaleLargeImages) {
        return image;
    }

    const int maxPixels = options.maxImagePixels > 0 ? options.maxImagePixels : OcrOptions().maxImagePixels;
    const qint64 maxBytes = options.maxImageBytes > 0 ? options.maxImageBytes : OcrOptions().maxImageBytes;
    const int maxDimension = options.maxImageDimension > 0 ? options.maxImageDimension
                                                            : OcrOptions().maxImageDimension;

    qreal scale = 1.0;
    const qint64 pixels = pixelCount(image);
    const qint64 bytes = imageBytes(image);

    if (pixels > maxPixels && maxPixels > 0) {
        scale = qMin(scale, qSqrt(static_cast<qreal>(maxPixels) / static_cast<qreal>(pixels)));
    }

    if (bytes > maxBytes && maxBytes > 0) {
        scale = qMin(scale, qSqrt(static_cast<qreal>(maxBytes) / static_cast<qreal>(bytes)));
    }

    const int maxSide = qMax(image.width(), image.height());
    if (maxSide > maxDimension && maxDimension > 0) {
        scale = qMin(scale, static_cast<qreal>(maxDimension) / static_cast<qreal>(maxSide));
    }

    if (scale >= 0.999) {
        return image;
    }

    const QSize targetSize = scaledSize(image.size(), scale);
    QImage downscaled = image.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (notice) {
        *notice = QObject::tr("OCR 输入图片过大，已自动缩放: %1x%2 -> %3x%4")
                      .arg(image.width())
                      .arg(image.height())
                      .arg(downscaled.width())
                      .arg(downscaled.height());
    }
    return downscaled;
}

// 函数说明：实现 OcrEngine::persistTessdataPath 的核心逻辑，供当前模块调用。
void OcrEngine::persistTessdataPath(const QString &path) const
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(kOcrSettingsGroup));
    if (path.trimmed().isEmpty()) {
        settings.remove(QString::fromLatin1(kTessdataPathKey));
    } else {
        settings.setValue(QString::fromLatin1(kTessdataPathKey), path);
    }
    settings.endGroup();
}

// 函数说明：加载 OcrEngine 需要的数据、配置或外部资源。
QString OcrEngine::loadPersistedTessdataPath() const
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(kOcrSettingsGroup));
    const QString path = settings.value(QString::fromLatin1(kTessdataPathKey)).toString().trimmed();
    settings.endGroup();
    return path;
}

// 函数说明：实现 OcrEngine::engineVersion 的核心逻辑，供当前模块调用。
QString OcrEngine::engineVersion() const
{
#ifdef ENABLE_TESSERACT
    return QString("Tesseract %1").arg(tesseract::TessBaseAPI::Version());
#else
    return tr("OCR 引擎未启用");
#endif
}

// 函数说明：实现 OcrEngine::initTesseract 的核心逻辑，供当前模块调用。
bool OcrEngine::initTesseract(const QString &dataPath)
{
#ifdef ENABLE_TESSERACT
    if (m_tesseract) {
        cleanupTesseract();
    }

    m_tesseract = new tesseract::TessBaseAPI();

    const QString langStr = m_currentLanguages.isEmpty()
                                ? QStringLiteral("eng")
                                : m_currentLanguages.join(QStringLiteral("+"));
    const QStringList initPaths = {
        parentForTessdata(dataPath),
        dataPath
    };

    int initStatus = -1;
    QString attemptedPath;
    for (const QString &initPath : initPaths) {
        if (initPath.isEmpty()) {
            continue;
        }
        attemptedPath = initPath;
        initStatus = m_tesseract->Init(initPath.toUtf8().constData(), langStr.toUtf8().constData());
        if (initStatus == 0) {
            break;
        }
        m_tesseract->End();
    }

    if (initStatus != 0) {
        m_lastInitializationError =
            tr("Tesseract 初始化失败。tessdata=%1，尝试路径=%2，语言=%3")
                .arg(dataPath, attemptedPath, langStr);
        qWarning().noquote() << m_lastInitializationError;
        delete m_tesseract;
        m_tesseract = nullptr;
        m_initialized = false;
        return false;
    }

    m_tesseract->SetPageSegMode(tesseract::PSM_AUTO);
    m_initialized = true;
    return true;
#else
    Q_UNUSED(dataPath)
    m_lastInitializationError = tr("Tesseract 未编译启用");
    qWarning() << m_lastInitializationError;
    return false;
#endif
}

// 函数说明：实现 OcrEngine::cleanupTesseract 的核心逻辑，供当前模块调用。
void OcrEngine::cleanupTesseract()
{
#ifdef ENABLE_TESSERACT
    if (m_tesseract) {
        m_tesseract->End();
        delete m_tesseract;
        m_tesseract = nullptr;
    }
#endif
    m_initialized = false;
    m_resolvedDataPath.clear();
}

// 函数说明：实现 OcrEngine::recognizeWithTesseract 的核心逻辑，供当前模块调用。
OcrEngine::OcrResult OcrEngine::recognizeWithTesseract(
    const QImage &image, 
    const OcrOptions &options)
{
    OcrResult result;
    result.success = false;

#if !defined(ENABLE_TESSERACT)
    Q_UNUSED(image)
    Q_UNUSED(options)
#endif
    
#ifdef ENABLE_TESSERACT
    if (!m_tesseract) {
        result.errorMessage = tr("Tesseract 未初始化");
        return result;
    }
    
    // 设置页面分割模式
    tesseract::PageSegMode psm = tesseract::PSM_AUTO;
    switch (options.mode) {
        case RecognitionMode::Text:
            psm = tesseract::PSM_AUTO;
            break;
        case RecognitionMode::Table:
            psm = tesseract::PSM_AUTO_OSD;
            break;
        case RecognitionMode::SingleLine:
            psm = tesseract::PSM_SINGLE_LINE;
            break;
        case RecognitionMode::SingleWord:
            psm = tesseract::PSM_SINGLE_WORD;
            break;
        case RecognitionMode::SingleChar:
            psm = tesseract::PSM_SINGLE_CHAR;
            break;
        default:
            psm = tesseract::PSM_AUTO;
    }
    m_tesseract->SetPageSegMode(psm);
    
    int bytesPerPixel = 0;
    const uchar *imageBits = nullptr;
    int imageBytesPerLine = 0;
    QImage convertedImage;

    switch (image.format()) {
    case QImage::Format_Grayscale8:
        bytesPerPixel = 1;
        imageBits = image.constBits();
        imageBytesPerLine = image.bytesPerLine();
        break;
    case QImage::Format_RGB888:
        bytesPerPixel = 3;
        imageBits = image.constBits();
        imageBytesPerLine = image.bytesPerLine();
        break;
    case QImage::Format_ARGB32:
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32_Premultiplied:
        bytesPerPixel = 4;
        imageBits = image.constBits();
        imageBytesPerLine = image.bytesPerLine();
        break;
    default:
        convertedImage = image.convertToFormat(QImage::Format_RGB888);
        bytesPerPixel = 3;
        imageBits = convertedImage.constBits();
        imageBytesPerLine = convertedImage.bytesPerLine();
        break;
    }

    if (!imageBits) {
        result.errorMessage = tr("无法访问图像像素数据");
        return result;
    }

    m_tesseract->SetImage(
        imageBits,
        image.width(),
        image.height(),
        bytesPerPixel,
        imageBytesPerLine
    );
    
    // 执行识别
    char *text = m_tesseract->GetUTF8Text();
    if (text) {
        result.fullText = QString::fromUtf8(text);
        delete[] text;
    }
    
    // 获取置信度
    result.averageConfidence = m_tesseract->MeanTextConf() / 100.0f;
    
    // 提取文字区域
    result.regions = extractTextRegions();
    
    result.success = true;
    
#else
    result.errorMessage = tr("Tesseract 未编译启用");
#endif
    
    return result;
}

// 函数说明：实现 OcrEngine::extractTextRegions 的核心逻辑，供当前模块调用。
QVector<OcrEngine::TextRegion> OcrEngine::extractTextRegions()
{
    QVector<TextRegion> regions;
    
#ifdef ENABLE_TESSERACT
    if (!m_tesseract) {
        return regions;
    }
    
    tesseract::ResultIterator *ri = m_tesseract->GetIterator();
    tesseract::PageIteratorLevel level = tesseract::RIL_WORD;
    
    if (ri) {
        do {
            const char *word = ri->GetUTF8Text(level);
            if (word) {
                TextRegion region;
                region.text = QString::fromUtf8(word);
                region.confidence = ri->Confidence(level) / 100.0f;
                
                int x1, y1, x2, y2;
                ri->BoundingBox(level, &x1, &y1, &x2, &y2);
                region.boundingBox = QRect(x1, y1, x2 - x1, y2 - y1);
                
                regions.append(region);
                delete[] word;
            }
        } while (ri->Next(level));
        
        delete ri;
    }
#endif
    
    return regions;
}

// 函数说明：实现 OcrEngine::extractTable 的核心逻辑，供当前模块调用。
OcrEngine::Table OcrEngine::extractTable(const QImage &image)
{
    QVector<Table> tables = TableDetector::detectTables(image);
    return tables.isEmpty() ? Table() : tables.first();
}


// ==================== TableDetector 实现 ====================

namespace {

QVector<TableDetector::Line> mergeParallelLines(const QVector<TableDetector::Line> &inputLines,
                                                bool horizontal,
                                                int tolerance)
{
    if (inputLines.isEmpty()) {
        return {};
    }

    QVector<TableDetector::Line> sorted = inputLines;
    std::sort(sorted.begin(), sorted.end(),
              [horizontal](const TableDetector::Line &a, const TableDetector::Line &b) {
                  const int coordA = horizontal ? a.start.y() : a.start.x();
                  const int coordB = horizontal ? b.start.y() : b.start.x();
                  if (coordA == coordB) {
                      const int startA = horizontal ? a.start.x() : a.start.y();
                      const int startB = horizontal ? b.start.x() : b.start.y();
                      return startA < startB;
                  }
                  return coordA < coordB;
              });

    QVector<TableDetector::Line> merged;
    TableDetector::Line current = sorted.first();

    auto mergeIntoCurrent = [&](const TableDetector::Line &line) {
        if (horizontal) {
            current.start.setX(qMin(current.start.x(), line.start.x()));
            current.end.setX(qMax(current.end.x(), line.end.x()));
            current.start.setY((current.start.y() + line.start.y()) / 2);
            current.end.setY(current.start.y());
        } else {
            current.start.setY(qMin(current.start.y(), line.start.y()));
            current.end.setY(qMax(current.end.y(), line.end.y()));
            current.start.setX((current.start.x() + line.start.x()) / 2);
            current.end.setX(current.start.x());
        }
    };

    for (int i = 1; i < sorted.size(); ++i) {
        const TableDetector::Line &line = sorted.at(i);
        const int lineCoord = horizontal ? line.start.y() : line.start.x();
        const int currentCoord = horizontal ? current.start.y() : current.start.x();

        if (qAbs(lineCoord - currentCoord) <= tolerance) {
            mergeIntoCurrent(line);
            continue;
        }

        merged.append(current);
        current = line;
    }

    merged.append(current);
    return merged;
}

QVector<int> uniqueSortedCoords(const QVector<int> &coords, int tolerance)
{
    if (coords.isEmpty()) {
        return {};
    }

    QVector<int> sorted = coords;
    std::sort(sorted.begin(), sorted.end());

    QVector<int> unique;
    unique.reserve(sorted.size());
    for (const int value : std::as_const(sorted)) {
        if (unique.isEmpty() || qAbs(unique.last() - value) > tolerance) {
            unique.append(value);
        } else {
            unique.last() = (unique.last() + value) / 2;
        }
    }
    return unique;
}

int nearestIndex(const QVector<int> &values, int target)
{
    if (values.isEmpty()) {
        return -1;
    }

    int bestIndex = 0;
    int bestDistance = std::numeric_limits<int>::max();
    for (int i = 0; i < values.size(); ++i) {
        const int distance = qAbs(values.at(i) - target);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = i;
        }
    }
    return bestIndex;
}

bool linesIntersect(const TableDetector::Line &horizontal,
                    const TableDetector::Line &vertical,
                    int tolerance)
{
    const int x = vertical.start.x();
    const int y = horizontal.start.y();
    const bool xInRange = x >= horizontal.start.x() - tolerance &&
                          x <= horizontal.end.x() + tolerance;
    const bool yInRange = y >= vertical.start.y() - tolerance &&
                          y <= vertical.end.y() + tolerance;
    return xInRange && yInRange;
}

} // namespace

// 函数说明：实现 TableDetector::detectTables 的核心逻辑，供当前模块调用。
QVector<OcrEngine::Table> TableDetector::detectTables(const QImage &image)
{
    QVector<OcrEngine::Table> tables;

    if (image.isNull()) {
        return tables;
    }

    QVector<Line> lines = detectLines(image);
    if (lines.size() < 4) {
        return tables;
    }

    QVector<QRect> cells = findCells(lines, image.size());
    if (cells.isEmpty()) {
        return tables;
    }

    OcrEngine::Table table;
    QVector<int> rowYs;
    QVector<int> colXs;

    for (const QRect &cell : cells) {
        rowYs.append(cell.top());
        colXs.append(cell.left());
    }

    const int tolerance = qMax(2, qMin(image.width(), image.height()) / 250);
    rowYs = uniqueSortedCoords(rowYs, tolerance);
    colXs = uniqueSortedCoords(colXs, tolerance);

    table.rows = rowYs.size();
    table.columns = colXs.size();

    QRect boundingBox;
    for (const QRect &cellRect : cells) {
        OcrEngine::TableCell cell;
        cell.row = nearestIndex(rowYs, cellRect.top());
        cell.column = nearestIndex(colXs, cellRect.left());
        if (cell.row < 0 || cell.column < 0) {
            continue;
        }
        cell.rowSpan = 1;
        cell.colSpan = 1;
        cell.boundingBox = cellRect;
        table.cells.append(cell);
        boundingBox = boundingBox.isNull() ? cellRect : (boundingBox | cellRect);
    }

    if (table.rows <= 0 || table.columns <= 0 || table.cells.isEmpty()) {
        return tables;
    }

    table.boundingBox = boundingBox;
    tables.append(table);
    return tables;
}

// 函数说明：实现 TableDetector::detectLines 的核心逻辑，供当前模块调用。
QVector<TableDetector::Line> TableDetector::detectLines(const QImage &image)
{
    QVector<Line> lines;

    if (image.isNull()) {
        return lines;
    }

    const QImage binary = OcrEngine::binarize(image, 170);
    const int width = binary.width();
    const int height = binary.height();
    if (width < 32 || height < 32) {
        return lines;
    }

    const int horizontalThreshold = qMax(10, static_cast<int>(width * 0.55));
    const int verticalThreshold = qMax(10, static_cast<int>(height * 0.55));
    const int minHorizontalLength = qMax(24, static_cast<int>(width * 0.35));
    const int minVerticalLength = qMax(24, static_cast<int>(height * 0.35));

    QVector<Line> horizontalLines;
    QVector<Line> verticalLines;

    auto rowBlackCount = [&](int y) {
        int count = 0;
        const uchar *row = binary.constScanLine(y);
        for (int x = 0; x < width; ++x) {
            if (row[x] < 128) {
                ++count;
            }
        }
        return count;
    };

    auto columnBlackCount = [&](int x) {
        int count = 0;
        for (int y = 0; y < height; ++y) {
            if (binary.constScanLine(y)[x] < 128) {
                ++count;
            }
        }
        return count;
    };

    QVector<int> candidateRows;
    candidateRows.reserve(height);
    for (int y = 0; y < height; ++y) {
        if (rowBlackCount(y) >= horizontalThreshold) {
            candidateRows.append(y);
        }
    }

    for (int i = 0; i < candidateRows.size();) {
        int runStart = candidateRows.at(i);
        int runEnd = runStart;
        while (i + 1 < candidateRows.size() && candidateRows.at(i + 1) <= runEnd + 1) {
            runEnd = candidateRows.at(i + 1);
            ++i;
        }

        const int y = (runStart + runEnd) / 2;
        int bestStart = -1;
        int bestLength = 0;
        int start = -1;

        for (int x = 0; x < width; ++x) {
            const bool isBlack = binary.constScanLine(y)[x] < 128;
            if (isBlack && start < 0) {
                start = x;
            }
            const bool isRunEnd = (!isBlack || x == width - 1) && start >= 0;
            if (!isRunEnd) {
                continue;
            }
            const int end = (isBlack && x == width - 1) ? x : (x - 1);
            const int length = end - start + 1;
            if (length > bestLength) {
                bestLength = length;
                bestStart = start;
            }
            start = -1;
        }

        if (bestLength >= minHorizontalLength && bestStart >= 0) {
            Line line;
            line.start = QPoint(bestStart, y);
            line.end = QPoint(bestStart + bestLength - 1, y);
            line.isHorizontal = true;
            horizontalLines.append(line);
        }
        ++i;
    }

    QVector<int> candidateCols;
    candidateCols.reserve(width);
    for (int x = 0; x < width; ++x) {
        if (columnBlackCount(x) >= verticalThreshold) {
            candidateCols.append(x);
        }
    }

    for (int i = 0; i < candidateCols.size();) {
        int runStart = candidateCols.at(i);
        int runEnd = runStart;
        while (i + 1 < candidateCols.size() && candidateCols.at(i + 1) <= runEnd + 1) {
            runEnd = candidateCols.at(i + 1);
            ++i;
        }

        const int x = (runStart + runEnd) / 2;
        int bestStart = -1;
        int bestLength = 0;
        int start = -1;

        for (int y = 0; y < height; ++y) {
            const bool isBlack = binary.constScanLine(y)[x] < 128;
            if (isBlack && start < 0) {
                start = y;
            }
            const bool isRunEnd = (!isBlack || y == height - 1) && start >= 0;
            if (!isRunEnd) {
                continue;
            }
            const int end = (isBlack && y == height - 1) ? y : (y - 1);
            const int length = end - start + 1;
            if (length > bestLength) {
                bestLength = length;
                bestStart = start;
            }
            start = -1;
        }

        if (bestLength >= minVerticalLength && bestStart >= 0) {
            Line line;
            line.start = QPoint(x, bestStart);
            line.end = QPoint(x, bestStart + bestLength - 1);
            line.isHorizontal = false;
            verticalLines.append(line);
        }
        ++i;
    }

    const int mergeTolerance = qMax(2, qMin(width, height) / 200);
    horizontalLines = mergeParallelLines(horizontalLines, true, mergeTolerance);
    verticalLines = mergeParallelLines(verticalLines, false, mergeTolerance);

    lines.reserve(horizontalLines.size() + verticalLines.size());
    for (const Line &line : std::as_const(horizontalLines)) {
        lines.append(line);
    }
    for (const Line &line : std::as_const(verticalLines)) {
        lines.append(line);
    }

    return lines;
}

// 函数说明：实现 TableDetector::findCells 的核心逻辑，供当前模块调用。
QVector<QRect> TableDetector::findCells(const QVector<Line> &lines, const QSize &imageSize)
{
    QVector<QRect> cells;
    if (lines.isEmpty()) {
        return cells;
    }

    QVector<Line> horizontalLines;
    QVector<Line> verticalLines;
    for (const Line &line : lines) {
        if (line.isHorizontal) {
            horizontalLines.append(line);
        } else {
            verticalLines.append(line);
        }
    }

    if (horizontalLines.size() < 2 || verticalLines.size() < 2) {
        return cells;
    }

    std::sort(horizontalLines.begin(), horizontalLines.end(),
              [](const Line &a, const Line &b) { return a.start.y() < b.start.y(); });
    std::sort(verticalLines.begin(), verticalLines.end(),
              [](const Line &a, const Line &b) { return a.start.x() < b.start.x(); });

    const int tolerance = qMax(2, qMin(imageSize.width(), imageSize.height()) / 250);
    const int minCellWidth = qMax(12, imageSize.width() / 60);
    const int minCellHeight = qMax(12, imageSize.height() / 60);

    QSet<QString> dedupe;

    for (int row = 0; row < horizontalLines.size() - 1; ++row) {
        const Line &top = horizontalLines.at(row);
        const Line &bottom = horizontalLines.at(row + 1);

        for (int col = 0; col < verticalLines.size() - 1; ++col) {
            const Line &left = verticalLines.at(col);
            const Line &right = verticalLines.at(col + 1);

            if (!linesIntersect(top, left, tolerance) ||
                !linesIntersect(top, right, tolerance) ||
                !linesIntersect(bottom, left, tolerance) ||
                !linesIntersect(bottom, right, tolerance)) {
                continue;
            }

            QRect cell(QPoint(left.start.x(), top.start.y()),
                       QPoint(right.start.x(), bottom.start.y()));
            cell = cell.normalized();
            if (cell.width() < minCellWidth || cell.height() < minCellHeight) {
                continue;
            }

            const QString key = QStringLiteral("%1_%2_%3_%4")
                                    .arg(cell.x())
                                    .arg(cell.y())
                                    .arg(cell.width())
                                    .arg(cell.height());
            if (dedupe.contains(key)) {
                continue;
            }
            dedupe.insert(key);
            cells.append(cell);
        }
    }

    return cells;
}


// ==================== OcrToMarkdown 实现 ====================

QString OcrToMarkdown::convertText(const OcrEngine::OcrResult &result, bool preserveLayout)
{
    if (!result.success || result.fullText.isEmpty()) {
        return QString();
    }

    QString text = result.fullText;

    if (!preserveLayout) {
        // 规范化空白
        text = normalizeWhitespace(text);
    }

    // 转义 Markdown 特殊字符
    text = escapeMarkdown(text);

    return text;
}

// 函数说明：实现 OcrToMarkdown::convertTable 的核心逻辑，供当前模块调用。
QString OcrToMarkdown::convertTable(const OcrEngine::Table &table)
{
    if (table.rows == 0 || table.columns == 0) {
        return QString();
    }

    // 创建二维数组
    QVector<QVector<QString>> grid(table.rows, QVector<QString>(table.columns));

    for (const auto &cell : table.cells) {
        if (cell.row < table.rows && cell.column < table.columns) {
            grid[cell.row][cell.column] = cell.text.trimmed();
        }
    }

    // 计算每列最大宽度
    QVector<int> colWidths(table.columns, 3);  // 最小宽度 3
    for (int row = 0; row < table.rows; ++row) {
        for (int col = 0; col < table.columns; ++col) {
            colWidths[col] = qMax(colWidths[col], grid[row][col].length());
        }
    }

    // 生成 Markdown 表格
    QString markdown;

    for (int row = 0; row < table.rows; ++row) {
        markdown += "|";
        for (int col = 0; col < table.columns; ++col) {
            QString cell = grid[row][col];
            cell = cell.leftJustified(colWidths[col]);
            markdown += " " + escapeMarkdown(cell) + " |";
        }
        markdown += "\n";

        // 添加分隔行（在表头之后）
        if (row == 0) {
            markdown += "|";
            for (int col = 0; col < table.columns; ++col) {
                markdown += QString("-").repeated(colWidths[col] + 2) + "|";
            }
            markdown += "\n";
        }
    }

    return markdown;
}

// 函数说明：实现 OcrToMarkdown::convertSmart 的核心逻辑，供当前模块调用。
QString OcrToMarkdown::convertSmart(const OcrEngine::OcrResult &result)
{
    QString markdown;

    // 先添加表格
    for (const auto &table : result.tables) {
        markdown += convertTable(table);
        markdown += "\n";
    }

    // 添加非表格文本
    if (!result.fullText.isEmpty() && result.tables.isEmpty()) {
        markdown += convertText(result);
    }

    return markdown;
}

// 函数说明：实现 OcrToMarkdown::escapeMarkdown 的核心逻辑，供当前模块调用。
QString OcrToMarkdown::escapeMarkdown(const QString &text)
{
    QString escaped = text;
    // 转义 Markdown 特殊字符
    escaped.replace("\\", "\\\\");
    escaped.replace("|", "\\|");
    escaped.replace("*", "\\*");
    escaped.replace("_", "\\_");
    escaped.replace("`", "\\`");
    escaped.replace("#", "\\#");
    escaped.replace("[", "\\[");
    escaped.replace("]", "\\]");
    return escaped;
}

// 函数说明：实现 OcrToMarkdown::normalizeWhitespace 的核心逻辑，供当前模块调用。
QString OcrToMarkdown::normalizeWhitespace(const QString &text)
{
    QString normalized = text;
    // 将多个空白字符替换为单个空格
    normalized.replace(QRegularExpression("[ \\t]+"), " ");
    // 将多个换行替换为两个换行（段落分隔）
    normalized.replace(QRegularExpression("\\n{3,}"), "\n\n");
    return normalized.trimmed();
}

