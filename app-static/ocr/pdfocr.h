// 文件说明：app-static\ocr\pdfocr.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef PDFOCR_H
#define PDFOCR_H

#include <QObject>
#include <QString>
#include <QImage>
#include <QVector>
#include "compat/pdfcompat.h"
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QtMath>

#include "ocrengine.h"

/**
 * @brief PDF OCR 处理器
 * 
 * 功能：
 * - PDF 页面渲染为图像
 * - 批量页面 OCR
 * - 表格检测和提取
 * - 导出为 Markdown
 */
class PdfOcr : public QObject
{
    Q_OBJECT

public:
    // 处理选项
    struct ProcessOptions {
        int dpi;                  // 渲染 DPI
        int startPage;              // 起始页（0-based）
        int endPage;               // 结束页（-1 = 最后一页）
        bool detectTables;       // 检测表格
        bool preserveLayout;    // 保留布局
        QStringList languages;
        int maxRenderPixels;      // 渲染像素上限（防止大页内存暴涨）
        qint64 maxRenderBytes;    // 渲染字节上限
        int maxRenderDimension;   // 单边最大尺寸
        
        ProcessOptions() 
            : dpi(300)
            , startPage(0)
            , endPage(-1)
            , detectTables(true)
            , preserveLayout(false)
            , languages({"eng", "chi_sim"})
            , maxRenderPixels(25000000)
            , maxRenderBytes(160LL * 1024LL * 1024LL)
            , maxRenderDimension(6000)
        {}
    };

    // 页面结果
    struct PageResult {
        int pageNumber;
        QString text;
        QVector<OcrEngine::Table> tables;
        float confidence;
        bool success;
        QString errorMessage;
    };

    // 文档结果
    struct DocumentResult {
        bool success;
        QString fullText;
        QString markdownText;
        QVector<PageResult> pages;
        int totalPages;
        int processedPages;
        float averageConfidence;
        QString errorMessage;
    };

    explicit PdfOcr(QObject *parent = nullptr);
    ~PdfOcr();

    // 设置 OCR 引擎
    void setOcrEngine(OcrEngine *engine);

    // 同步处理
    DocumentResult process(const QString &pdfPath, 
                           const ProcessOptions &options = ProcessOptions());

    // 异步处理
    void processAsync(const QString &pdfPath,
                      const ProcessOptions &options = ProcessOptions());

    // 渲染单页为图像
    QImage renderPage(const QString &pdfPath, int pageIndex, int dpi = 300);

    // 获取 PDF 页数
    static int pageCount(const QString &pdfPath);

    // 取消处理
    void cancel();
    bool isCanceled() const { return m_canceled; }

signals:
    void progressChanged(int current, int total);
    void pageCompleted(int pageNumber, const PageResult &result);
    void processingCompleted(const DocumentResult &result);
    void errorOccurred(const QString &error);

private:
    static QSize clampRenderSize(const QSizeF &pageSizePoints, const ProcessOptions &options);
    PageResult processPage(QPdfDocument *doc, int pageIndex, 
                           const ProcessOptions &options);
    QString generateMarkdown(const QVector<PageResult> &pages);

    OcrEngine *m_ocrEngine;
    bool m_canceled;
    QFutureWatcher<DocumentResult> *m_watcher;
};


/**
 * @brief 批量 OCR 处理器
 * 
 * 功能：
 * - 批量处理多个图片/PDF
 * - 进度跟踪
 * - 结果合并
 */
class BatchOcr : public QObject
{
    Q_OBJECT

public:
    struct FileResult {
        QString filePath;
        QString text;
        bool success;
        QString errorMessage;
        int processingTimeMs;
    };

    struct BatchResult {
        QVector<FileResult> files;
        int totalFiles;
        int successCount;
        int failedCount;
        int totalTimeMs;
    };

    explicit BatchOcr(QObject *parent = nullptr);

    void setOcrEngine(OcrEngine *engine);
    void setPdfOcr(PdfOcr *pdfOcr);

    // 添加文件
    void addFile(const QString &path);
    void addFiles(const QStringList &paths);
    void clearFiles();

    // 处理
    void process();
    void cancel();

    // 导出结果
    bool exportToMarkdown(const QString &outputPath) const;
    bool exportToText(const QString &outputPath) const;

signals:
    void progressChanged(int current, int total, const QString &currentFile);
    void fileCompleted(const FileResult &result);
    void batchCompleted(const BatchResult &result);

private:
    OcrEngine *m_ocrEngine;
    PdfOcr *m_pdfOcr;
    QStringList m_files;
    BatchResult m_result;
    bool m_canceled;
};


// ==================== PdfOcr 实现 ====================

inline PdfOcr::PdfOcr(QObject *parent)
    : QObject(parent)
    , m_ocrEngine(nullptr)
    , m_canceled(false)
    , m_watcher(new QFutureWatcher<DocumentResult>(this))
{
    connect(m_watcher, &QFutureWatcher<DocumentResult>::finished, this, [this]() {
        emit processingCompleted(m_watcher->result());
    });
}

inline PdfOcr::~PdfOcr()
{
}

inline void PdfOcr::setOcrEngine(OcrEngine *engine)
{
    m_ocrEngine = engine;
}

inline PdfOcr::DocumentResult PdfOcr::process(const QString &pdfPath,
                                               const ProcessOptions &options)
{
    DocumentResult result;
    result.success = false;
    
    QPdfDocument doc;
    QPdfDocument::Error error = doc.load(pdfPath);
    
    if (error != QPdfDocument::Error::None) {
        result.errorMessage = tr("无法加载 PDF 文件");
        emit errorOccurred(result.errorMessage);
        return result;
    }
    
    result.totalPages = doc.pageCount();
    
    int startPage = qMax(0, options.startPage);
    int endPage = options.endPage < 0 ? result.totalPages - 1 : 
                  qMin(options.endPage, result.totalPages - 1);
    
    m_canceled = false;
    float totalConfidence = 0;
    
    for (int i = startPage; i <= endPage && !m_canceled; ++i) {
        PageResult pageResult = processPage(&doc, i, options);
        result.pages.append(pageResult);
        result.processedPages++;
        
        if (pageResult.success) {
            totalConfidence += pageResult.confidence;
        }
        
        emit progressChanged(result.processedPages, endPage - startPage + 1);
        emit pageCompleted(i, pageResult);
    }
    
    if (m_canceled) {
        result.errorMessage = tr("处理被取消");
        return result;
    }
    
    // 合并结果
    QStringList texts;
    for (const PageResult &page : result.pages) {
        if (!page.text.isEmpty()) {
            texts << page.text;
        }
    }
    result.fullText = texts.join("\n\n---\n\n");
    
    // 生成 Markdown
    result.markdownText = generateMarkdown(result.pages);
    
    result.averageConfidence = result.processedPages > 0 ?
        totalConfidence / result.processedPages : 0;
    result.success = true;
    
    return result;
}

inline void PdfOcr::processAsync(const QString &pdfPath,
                                  const ProcessOptions &options)
{
    QFuture<DocumentResult> future = QtConcurrent::run([this, pdfPath, options]() {
        return process(pdfPath, options);
    });
    
    m_watcher->setFuture(future);
}

inline QImage PdfOcr::renderPage(const QString &pdfPath, int pageIndex, int dpi)
{
    QPdfDocument doc;
    if (doc.load(pdfPath) != QPdfDocument::Error::None) {
        return QImage();
    }
    
    if (pageIndex < 0 || pageIndex >= doc.pageCount()) {
        return QImage();
    }
    
    QSizeF pageSize = doc.pagePointSize(pageIndex);
    ProcessOptions options;
    options.dpi = dpi;
    const QSize renderSize = clampRenderSize(pageSize, options);
    
    QPdfDocumentRenderOptions renderOptions;
    
    return doc.render(pageIndex, renderSize, renderOptions);
}

inline int PdfOcr::pageCount(const QString &pdfPath)
{
    QPdfDocument doc;
    if (doc.load(pdfPath) != QPdfDocument::Error::None) {
        return 0;
    }
    return doc.pageCount();
}

inline void PdfOcr::cancel()
{
    m_canceled = true;
}

inline QSize PdfOcr::clampRenderSize(const QSizeF &pageSizePoints, const ProcessOptions &options)
{
    if (!pageSizePoints.isValid()) {
        return {};
    }

    const qreal dpiScale = qMax(1, options.dpi) / 72.0;
    QSize targetSize(qRound(pageSizePoints.width() * dpiScale),
                     qRound(pageSizePoints.height() * dpiScale));
    if (targetSize.width() <= 0 || targetSize.height() <= 0) {
        return {};
    }

    qreal scale = 1.0;
    const qint64 pixelCount = static_cast<qint64>(targetSize.width()) * targetSize.height();
    const qint64 estimatedBytes = pixelCount * 4;

    if (options.maxRenderPixels > 0 && pixelCount > options.maxRenderPixels) {
        scale = qMin(scale, qSqrt(static_cast<qreal>(options.maxRenderPixels) /
                                  static_cast<qreal>(pixelCount)));
    }

    if (options.maxRenderBytes > 0 && estimatedBytes > options.maxRenderBytes) {
        scale = qMin(scale, qSqrt(static_cast<qreal>(options.maxRenderBytes) /
                                  static_cast<qreal>(estimatedBytes)));
    }

    const int maxSide = qMax(targetSize.width(), targetSize.height());
    if (options.maxRenderDimension > 0 && maxSide > options.maxRenderDimension) {
        scale = qMin(scale, static_cast<qreal>(options.maxRenderDimension) /
                             static_cast<qreal>(maxSide));
    }

    if (scale < 0.999) {
        targetSize = QSize(qRound(targetSize.width() * scale),
                           qRound(targetSize.height() * scale));
    }

    return QSize(qMax(64, targetSize.width()), qMax(64, targetSize.height()));
}

inline PdfOcr::PageResult PdfOcr::processPage(QPdfDocument *doc, int pageIndex,
                                               const ProcessOptions &options)
{
    PageResult result;
    result.pageNumber = pageIndex + 1;
    result.success = false;
    
    if (!m_ocrEngine || !m_ocrEngine->isInitialized()) {
        result.errorMessage = tr("OCR 引擎未初始化");
        return result;
    }
    
    // 渲染页面
    QSizeF pageSize = doc->pagePointSize(pageIndex);
    const QSize renderSize = clampRenderSize(pageSize, options);
    
    QPdfDocumentRenderOptions renderOptions;
    QImage pageImage = doc->render(pageIndex, renderSize, renderOptions);
    
    if (pageImage.isNull()) {
        result.errorMessage = tr("渲染页面失败");
        return result;
    }
    
    // OCR 识别
    OcrEngine::OcrOptions ocrOptions;
    ocrOptions.languages = options.languages;
    ocrOptions.detectTables = options.detectTables;
    ocrOptions.preserveLayout = options.preserveLayout;
    ocrOptions.dpi = options.dpi;
    
    OcrEngine::OcrResult ocrResult = m_ocrEngine->recognize(pageImage, ocrOptions);
    
    if (!ocrResult.success) {
        result.errorMessage = ocrResult.errorMessage;
        return result;
    }
    
    result.text = ocrResult.fullText;
    result.tables = ocrResult.tables;
    result.confidence = ocrResult.averageConfidence;
    result.success = true;
    
    return result;
}

inline QString PdfOcr::generateMarkdown(const QVector<PageResult> &pages)
{
    QString markdown;
    
    for (const PageResult &page : pages) {
        if (!page.success) continue;
        
        markdown += QString("## 第 %1 页\n\n").arg(page.pageNumber);
        
        // 表格
        for (const OcrEngine::Table &table : page.tables) {
            markdown += OcrToMarkdown::convertTable(table);
            markdown += "\n";
        }
        
        // 文本
        if (!page.text.isEmpty() && page.tables.isEmpty()) {
            markdown += page.text;
            markdown += "\n\n";
        }
        
        markdown += "---\n\n";
    }
    
    return markdown;
}


// ==================== BatchOcr 实现 ====================

inline BatchOcr::BatchOcr(QObject *parent)
    : QObject(parent)
    , m_ocrEngine(nullptr)
    , m_pdfOcr(nullptr)
    , m_canceled(false)
{
}

inline void BatchOcr::setOcrEngine(OcrEngine *engine)
{
    m_ocrEngine = engine;
}

inline void BatchOcr::setPdfOcr(PdfOcr *pdfOcr)
{
    m_pdfOcr = pdfOcr;
}

inline void BatchOcr::addFile(const QString &path)
{
    if (!m_files.contains(path)) {
        m_files.append(path);
    }
}

inline void BatchOcr::addFiles(const QStringList &paths)
{
    for (const QString &path : paths) {
        addFile(path);
    }
}

inline void BatchOcr::clearFiles()
{
    m_files.clear();
}

inline void BatchOcr::process()
{
    m_result = BatchResult();
    m_result.totalFiles = m_files.size();
    m_canceled = false;
    
    QElapsedTimer timer;
    timer.start();
    
    for (int i = 0; i < m_files.size() && !m_canceled; ++i) {
        const QString &file = m_files[i];
        
        emit progressChanged(i + 1, m_result.totalFiles, file);
        
        FileResult fileResult;
        fileResult.filePath = file;
        
        QElapsedTimer fileTimer;
        fileTimer.start();
        
        QString ext = QFileInfo(file).suffix().toLower();
        
        if (ext == "pdf") {
            // PDF 处理
            if (m_pdfOcr) {
                PdfOcr::DocumentResult pdfResult = m_pdfOcr->process(file);
                fileResult.success = pdfResult.success;
                fileResult.text = pdfResult.markdownText;
                fileResult.errorMessage = pdfResult.errorMessage;
            } else {
                fileResult.success = false;
                fileResult.errorMessage = tr("PDF OCR 未配置");
            }
        } else {
            // 图片处理
            if (m_ocrEngine && m_ocrEngine->isInitialized()) {
                OcrEngine::OcrResult ocrResult = m_ocrEngine->recognize(file);
                fileResult.success = ocrResult.success;
                fileResult.text = OcrToMarkdown::convertSmart(ocrResult);
                fileResult.errorMessage = ocrResult.errorMessage;
            } else {
                fileResult.success = false;
                fileResult.errorMessage = tr("OCR 引擎未初始化");
            }
        }
        
        fileResult.processingTimeMs = fileTimer.elapsed();
        
        if (fileResult.success) {
            m_result.successCount++;
        } else {
            m_result.failedCount++;
        }
        
        m_result.files.append(fileResult);
        emit fileCompleted(fileResult);
    }
    
    m_result.totalTimeMs = timer.elapsed();
    emit batchCompleted(m_result);
}

inline void BatchOcr::cancel()
{
    m_canceled = true;
    if (m_pdfOcr) {
        m_pdfOcr->cancel();
    }
}

inline bool BatchOcr::exportToMarkdown(const QString &outputPath) const
{
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    
    out << "# OCR 批量识别结果\n\n";
    out << QString("处理时间: %1 秒\n").arg(m_result.totalTimeMs / 1000.0, 0, 'f', 2);
    out << QString("成功: %1 / %2\n\n").arg(m_result.successCount).arg(m_result.totalFiles);
    out << "---\n\n";
    
    for (const FileResult &result : m_result.files) {
        out << QString("## %1\n\n").arg(QFileInfo(result.filePath).fileName());
        
        if (result.success) {
            out << result.text << "\n\n";
        } else {
            out << QString("*识别失败: %1*\n\n").arg(result.errorMessage);
        }
        
        out << "---\n\n";
    }
    
    return true;
}

inline bool BatchOcr::exportToText(const QString &outputPath) const
{
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    
    for (const FileResult &result : m_result.files) {
        if (result.success) {
            out << result.text << "\n\n";
        }
    }
    
    return true;
}

#endif // PDFOCR_H

