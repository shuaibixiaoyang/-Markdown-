// 文件说明：app-static\ocr\ocrengine.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef OCRENGINE_H
#define OCRENGINE_H

#include <QObject>
#include <QImage>
#include <QPixmap>
#include <QRect>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QSize>

// 前向声明 Tesseract 类型
namespace tesseract {
    class TessBaseAPI;
}

/**
 * @brief OCR 引擎抽象层
 * 
 * 支持的引擎：
 * - Tesseract OCR（推荐）
 * - 可扩展支持其他引擎
 * 
 * 功能：
 * - 文字识别
 * - 表格识别
 * - 多语言支持
 * - 置信度评估
 */
class OcrEngine : public QObject
{
    Q_OBJECT

public:
    // OCR 引擎类型
    enum class EngineType {
        Tesseract,
        PaddleOCR,      // 未来扩展
        WindowsOCR,     // Windows 10+ 内置
        MacVision       // macOS Vision 框架
    };
    Q_ENUM(EngineType)

    // 识别模式
    enum class RecognitionMode {
        Auto,           // 自动检测
        Text,           // 纯文本
        Table,          // 表格
        SingleLine,     // 单行
        SingleWord,     // 单词
        SingleChar      // 单字符
    };
    Q_ENUM(RecognitionMode)

    // 文字区域
    struct TextRegion {
        QRect boundingBox;
        QString text;
        float confidence;       // 0.0 - 1.0
        QString language;
        int blockNum;
        int paragraphNum;
        int lineNum;
        int wordNum;
    };

    // 表格单元格
    struct TableCell {
        int row;
        int column;
        int rowSpan;
        int colSpan;
        QString text;
        QRect boundingBox;
    };

    // 表格结构
    struct Table {
        int rows;
        int columns;
        QVector<TableCell> cells;
        QRect boundingBox;
    };

    // OCR 结果
    struct OcrResult {
        bool success;
        QString fullText;
        QVector<TextRegion> regions;
        QVector<Table> tables;
        QString errorMessage;
        int processingTimeMs;
        QString detectedLanguage;
        float averageConfidence;
    };

    // OCR 选项
    struct OcrOptions {
        RecognitionMode mode;
        QStringList languages;
        bool detectTables;
        bool preserveLayout;
        int dpi;
        bool preprocess;
        float minConfidence;
        bool autoDownscaleLargeImages;
        int maxImagePixels;
        qint64 maxImageBytes;
        int maxImageDimension;
        
        OcrOptions()
            : mode(RecognitionMode::Auto)
            , languages(QStringList() << "eng" << "chi_sim")
            , detectTables(true)
            , preserveLayout(false)
            , dpi(300)
            , preprocess(true)
            , minConfidence(0.5f)
            , autoDownscaleLargeImages(true)
            , maxImagePixels(25000000)
            , maxImageBytes(128LL * 1024LL * 1024LL)
            , maxImageDimension(6000)
        {}
    };

    explicit OcrEngine(QObject *parent = nullptr);
    ~OcrEngine();

    // 初始化引擎
    bool initialize(const QString &dataPath = QString());
    bool isInitialized() const { return m_initialized; }
    void setTessdataPath(const QString &path);
    QString tessdataPath() const { return m_dataPath; }
    QString resolvedTessdataPath() const { return m_resolvedDataPath; }
    QString lastInitializationError() const { return m_lastInitializationError; }

    // 设置语言
    bool setLanguages(const QStringList &languages);
    QStringList availableLanguages() const;

    // 同步识别
    OcrResult recognize(const QImage &image, const OcrOptions &options = OcrOptions());
    OcrResult recognize(const QPixmap &pixmap, const OcrOptions &options = OcrOptions());
    OcrResult recognize(const QString &imagePath, const OcrOptions &options = OcrOptions());

    // 异步识别
    void recognizeAsync(const QImage &image, const OcrOptions &options = OcrOptions());
    void recognizeAsync(const QString &imagePath, const OcrOptions &options = OcrOptions());

    // 识别表格
    Table recognizeTable(const QImage &image);

    // 转换为 Markdown
    QString toMarkdownText(const OcrResult &result) const;
    QString toMarkdownTable(const Table &table) const;

    // 预处理图像
    static QImage preprocessImage(const QImage &image, int targetDpi = 300);
    static QImage binarize(const QImage &image, int threshold = 128);
    static QImage deskew(const QImage &image);
    static QImage removeNoise(const QImage &image);

    // 获取引擎信息
    QString engineVersion() const;
    EngineType engineType() const { return m_engineType; }

signals:
    void recognitionCompleted(const OcrResult &result);
    void recognitionProgress(int percent);
    void errorOccurred(const QString &error);

private:
    bool initTesseract(const QString &dataPath);
    void cleanupTesseract();
    OcrResult recognizeWithTesseract(const QImage &image, const OcrOptions &options);
    QVector<TextRegion> extractTextRegions();
    Table extractTable(const QImage &image);
    QStringList tessdataSearchPaths(const QString &requestedPath) const;
    static QString normalizeTessdataCandidate(const QString &candidate);
    static QStringList missingLanguageFiles(const QString &tessdataDir,
                                            const QStringList &languages);
    static QString parentForTessdata(const QString &tessdataDir);
    static QImage clampImageForOcr(const QImage &image, const OcrOptions &options,
                                   QString *notice = nullptr);
    void persistTessdataPath(const QString &path) const;
    QString loadPersistedTessdataPath() const;

    EngineType m_engineType;
    bool m_initialized;
    QString m_dataPath;
    QString m_resolvedDataPath;
    QString m_lastInitializationError;
    QStringList m_currentLanguages;
    
    // Tesseract 实例
    tesseract::TessBaseAPI *m_tesseract;
};


// ==================== 辅助类：表格检测器 ====================

class TableDetector
{
public:
    struct Line {
        QPoint start;
        QPoint end;
        bool isHorizontal;
    };

    static QVector<OcrEngine::Table> detectTables(const QImage &image);
    
private:
    static QVector<Line> detectLines(const QImage &image);
    static QVector<QRect> findCells(const QVector<Line> &lines, const QSize &imageSize);
};


// ==================== 辅助类：Markdown 转换器 ====================

class OcrToMarkdown
{
public:
    // 转换纯文本结果
    static QString convertText(const OcrEngine::OcrResult &result, bool preserveLayout = false);
    
    // 转换表格
    static QString convertTable(const OcrEngine::Table &table);
    
    // 智能转换（自动检测表格）
    static QString convertSmart(const OcrEngine::OcrResult &result);

private:
    static QString escapeMarkdown(const QString &text);
    static QString normalizeWhitespace(const QString &text);
};

#endif // OCRENGINE_H

