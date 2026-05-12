// 文件说明：app-static\ocr\smartnamer.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef SMARTNAMER_H
#define SMARTNAMER_H

#include <QObject>
#include <QString>
#include <QImage>
#include <QPixmap>
#include <QDateTime>
#include <QStringList>
#include <QRegularExpression>
#include <QCryptographicHash>

#include "ocrengine.h"

/**
 * @brief 智能命名器
 * 
 * 基于图片内容自动生成有意义的文件名：
 * - OCR 文字识别提取关键词
 * - 图像特征分析
 * - 时间戳和序号
 * - 安全字符过滤
 */
class SmartNamer : public QObject
{
    Q_OBJECT

public:
    // 命名策略
    enum class NamingStrategy {
        OcrKeywords,        // OCR 关键词
        Timestamp,          // 时间戳
        Sequential,         // 序号
        Hash,               // 哈希值
        Mixed              // 混合（关键词 + 时间戳）
    };
    Q_ENUM(NamingStrategy)

    // 命名选项
    struct NamingOptions {
        NamingStrategy strategy;
        int maxKeywords;
        int maxLength;
        QString prefix;
        QString suffix;
        QString separator;
        bool lowercase;
        bool includeDimensions;
        QString dateFormat;
        
        NamingOptions()
            : strategy(NamingStrategy::Mixed)
            , maxKeywords(3)
            , maxLength(50)
            , separator("_")
            , lowercase(true)
            , includeDimensions(false)
            , dateFormat("yyyyMMdd_HHmmss")
        {}
    };

    // 命名结果
    struct NamingResult {
        QString suggestedName;
        QString baseName;           // 不含扩展名
        QString extension;
        QStringList keywords;
        float confidence;
        QString source;             // 命名来源说明
    };

    explicit SmartNamer(QObject *parent = nullptr);

    // 设置 OCR 引擎（可选）
    void setOcrEngine(OcrEngine *engine);

    // 生成文件名
    NamingResult generateName(const QImage &image, 
                               const NamingOptions &options = NamingOptions());
    
    NamingResult generateName(const QPixmap &pixmap,
                               const NamingOptions &options = NamingOptions());
    
    NamingResult generateName(const QString &imagePath,
                               const NamingOptions &options = NamingOptions());

    // 清理文件名（移除非法字符）
    static QString sanitizeFileName(const QString &name);

    // 提取关键词
    QStringList extractKeywords(const QString &text, int maxCount = 5) const;

    // 生成唯一文件名（避免冲突）
    static QString makeUnique(const QString &basePath, const QString &baseName, 
                               const QString &extension);

    // 预设命名模板
    static NamingOptions screenshotOptions();
    static NamingOptions documentOptions();
    static NamingOptions photoOptions();

signals:
    void namingCompleted(const NamingResult &result);

private:
    QString generateOcrBasedName(const QImage &image, const NamingOptions &options);
    QString generateTimestampName(const NamingOptions &options);
    QString generateSequentialName(const NamingOptions &options);
    QString generateHashName(const QImage &image);
    QString combineNameParts(const QStringList &parts, const NamingOptions &options);
    
    // 中文分词（简化版）
    QStringList segmentChinese(const QString &text) const;
    
    // 停用词
    bool isStopWord(const QString &word) const;
    
    // 词频统计
    QMap<QString, int> calculateWordFrequency(const QStringList &words) const;

    OcrEngine *m_ocrEngine;
    int m_sequenceCounter;
    QSet<QString> m_stopWords;
};


// ==================== 实现 ====================

inline SmartNamer::SmartNamer(QObject *parent)
    : QObject(parent)
    , m_ocrEngine(nullptr)
    , m_sequenceCounter(1)
{
    // 初始化停用词
    m_stopWords = {
        // 英语停用词
        "the", "a", "an", "is", "are", "was", "were", "be", "been", "being",
        "have", "has", "had", "do", "does", "did", "will", "would", "could",
        "should", "may", "might", "must", "shall", "can", "need", "dare",
        "of", "in", "to", "for", "with", "on", "at", "by", "from", "as",
        "into", "through", "during", "before", "after", "above", "below",
        "and", "or", "but", "if", "then", "else", "when", "where", "why",
        "how", "all", "each", "every", "both", "few", "more", "most", "other",
        "some", "such", "no", "nor", "not", "only", "own", "same", "so",
        "than", "too", "very", "just", "also", "now", "here", "there",
        // 中文停用词
        "的", "了", "是", "在", "我", "有", "和", "就", "不", "人", "都",
        "一", "一个", "上", "也", "很", "到", "说", "要", "去", "你",
        "会", "着", "没有", "看", "好", "自己", "这", "那", "什么", "为"
    };
}

inline void SmartNamer::setOcrEngine(OcrEngine *engine)
{
    m_ocrEngine = engine;
}

inline SmartNamer::NamingResult SmartNamer::generateName(
    const QImage &image, 
    const NamingOptions &options)
{
    NamingResult result;
    result.confidence = 0.0f;
    
    QStringList nameParts;
    
    // 添加前缀
    if (!options.prefix.isEmpty()) {
        nameParts << options.prefix;
    }
    
    switch (options.strategy) {
        case NamingStrategy::OcrKeywords:
            result.baseName = generateOcrBasedName(image, options);
            result.source = tr("基于 OCR 关键词");
            break;
            
        case NamingStrategy::Timestamp:
            result.baseName = generateTimestampName(options);
            result.source = tr("基于时间戳");
            break;
            
        case NamingStrategy::Sequential:
            result.baseName = generateSequentialName(options);
            result.source = tr("基于序号");
            break;
            
        case NamingStrategy::Hash:
            result.baseName = generateHashName(image);
            result.source = tr("基于哈希");
            break;
            
        case NamingStrategy::Mixed:
        default:
            {
                QString ocrName = generateOcrBasedName(image, options);
                QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
                
                if (!ocrName.isEmpty() && ocrName != "image") {
                    result.baseName = ocrName + options.separator + timestamp;
                    result.source = tr("OCR + 时间戳");
                } else {
                    result.baseName = "screenshot" + options.separator + timestamp;
                    result.source = tr("时间戳（无 OCR 结果）");
                }
            }
            break;
    }
    
    // 添加尺寸信息
    if (options.includeDimensions) {
        result.baseName += QString("%1%2x%3")
            .arg(options.separator)
            .arg(image.width())
            .arg(image.height());
    }
    
    // 添加后缀
    if (!options.suffix.isEmpty()) {
        result.baseName += options.separator + options.suffix;
    }
    
    // 清理和限制长度
    result.baseName = sanitizeFileName(result.baseName);
    if (result.baseName.length() > options.maxLength) {
        result.baseName = result.baseName.left(options.maxLength);
    }
    
    // 转换大小写
    if (options.lowercase) {
        result.baseName = result.baseName.toLower();
    }
    
    // 默认扩展名
    result.extension = "png";
    result.suggestedName = result.baseName + "." + result.extension;
    
    return result;
}

inline SmartNamer::NamingResult SmartNamer::generateName(
    const QPixmap &pixmap,
    const NamingOptions &options)
{
    return generateName(pixmap.toImage(), options);
}

inline SmartNamer::NamingResult SmartNamer::generateName(
    const QString &imagePath,
    const NamingOptions &options)
{
    QImage image(imagePath);
    NamingResult result = generateName(image, options);
    
    // 保留原扩展名
    QFileInfo info(imagePath);
    result.extension = info.suffix().toLower();
    if (result.extension.isEmpty()) {
        result.extension = "png";
    }
    result.suggestedName = result.baseName + "." + result.extension;
    
    return result;
}

inline QString SmartNamer::sanitizeFileName(const QString &name)
{
    QString safe = name;
    
    // 移除或替换非法字符
    static QRegularExpression illegalChars("[\\\\/:*?\"<>|\\x00-\\x1F]");
    safe.replace(illegalChars, "_");
    
    // 移除连续的分隔符
    safe.replace(QRegularExpression("_+"), "_");
    safe.replace(QRegularExpression("-+"), "-");
    
    // 移除首尾的分隔符
    safe = safe.trimmed();
    while (safe.startsWith("_") || safe.startsWith("-") || safe.startsWith(".")) {
        safe = safe.mid(1);
    }
    while (safe.endsWith("_") || safe.endsWith("-")) {
        safe.chop(1);
    }
    
    // 确保不为空
    if (safe.isEmpty()) {
        safe = "image";
    }
    
    return safe;
}

inline QStringList SmartNamer::extractKeywords(const QString &text, int maxCount) const
{
    if (text.isEmpty()) {
        return QStringList();
    }
    
    QStringList words;
    
    // 分词
    // 英文：按空格和标点分割
    // 中文：简单按字符处理
    
    QStringList englishWords = text.split(QRegularExpression("[\\s\\p{P}]+"), 
                                           Qt::SkipEmptyParts);
    
    for (const QString &word : englishWords) {
        QString cleaned = word.trimmed().toLower();
        if (cleaned.length() >= 2 && !isStopWord(cleaned)) {
            // 检查是否包含中文
            bool hasChinese = false;
            for (const QChar &c : cleaned) {
                if (c.unicode() >= 0x4E00 && c.unicode() <= 0x9FFF) {
                    hasChinese = true;
                    break;
                }
            }
            
            if (hasChinese) {
                words << segmentChinese(cleaned);
            } else if (cleaned.length() >= 3) {
                words << cleaned;
            }
        }
    }
    
    // 计算词频并排序
    QMap<QString, int> frequency = calculateWordFrequency(words);
    
    QList<QPair<QString, int>> sorted;
    for (auto it = frequency.begin(); it != frequency.end(); ++it) {
        sorted.append(qMakePair(it.key(), it.value()));
    }
    std::sort(sorted.begin(), sorted.end(), 
              [](const auto &a, const auto &b) { return a.second > b.second; });
    
    QStringList keywords;
    for (int i = 0; i < qMin(maxCount, sorted.size()); ++i) {
        keywords << sorted[i].first;
    }
    
    return keywords;
}

inline QString SmartNamer::makeUnique(const QString &basePath, 
                                       const QString &baseName,
                                       const QString &extension)
{
    QString fullPath = basePath + "/" + baseName + "." + extension;
    
    if (!QFile::exists(fullPath)) {
        return baseName + "." + extension;
    }
    
    int counter = 1;
    while (true) {
        QString newName = QString("%1_%2.%3").arg(baseName).arg(counter).arg(extension);
        fullPath = basePath + "/" + newName;
        
        if (!QFile::exists(fullPath)) {
            return newName;
        }
        
        counter++;
        if (counter > 9999) {
            // 使用时间戳作为后备
            return QString("%1_%2.%3")
                .arg(baseName)
                .arg(QDateTime::currentMSecsSinceEpoch())
                .arg(extension);
        }
    }
}

inline SmartNamer::NamingOptions SmartNamer::screenshotOptions()
{
    NamingOptions opts;
    opts.strategy = NamingStrategy::Mixed;
    opts.prefix = "screenshot";
    opts.maxKeywords = 2;
    opts.includeDimensions = false;
    return opts;
}

inline SmartNamer::NamingOptions SmartNamer::documentOptions()
{
    NamingOptions opts;
    opts.strategy = NamingStrategy::OcrKeywords;
    opts.maxKeywords = 4;
    opts.lowercase = false;
    return opts;
}

inline SmartNamer::NamingOptions SmartNamer::photoOptions()
{
    NamingOptions opts;
    opts.strategy = NamingStrategy::Timestamp;
    opts.prefix = "photo";
    opts.dateFormat = "yyyy-MM-dd_HH-mm-ss";
    return opts;
}

inline QString SmartNamer::generateOcrBasedName(const QImage &image, 
                                                  const NamingOptions &options)
{
    if (!m_ocrEngine || !m_ocrEngine->isInitialized()) {
        return "image";
    }
    
    // 执行 OCR
    OcrEngine::OcrOptions ocrOpts;
    ocrOpts.mode = OcrEngine::RecognitionMode::Auto;
    ocrOpts.preprocess = true;
    
    OcrEngine::OcrResult ocrResult = m_ocrEngine->recognize(image, ocrOpts);
    
    if (!ocrResult.success || ocrResult.fullText.isEmpty()) {
        return "image";
    }
    
    // 提取关键词
    QStringList keywords = extractKeywords(ocrResult.fullText, options.maxKeywords);
    
    if (keywords.isEmpty()) {
        return "image";
    }
    
    return keywords.join(options.separator);
}

inline QString SmartNamer::generateTimestampName(const NamingOptions &options)
{
    return QDateTime::currentDateTime().toString(options.dateFormat);
}

inline QString SmartNamer::generateSequentialName(const NamingOptions &options)
{
    Q_UNUSED(options)
    return QString("image_%1").arg(m_sequenceCounter++, 4, 10, QChar('0'));
}

inline QString SmartNamer::generateHashName(const QImage &image)
{
    QByteArray data(reinterpret_cast<const char*>(image.constBits()), 
                    image.sizeInBytes());
    QString hash = QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
    return hash.left(12);
}

inline QString SmartNamer::combineNameParts(const QStringList &parts, 
                                             const NamingOptions &options)
{
    QStringList filtered;
    for (const QString &part : parts) {
        QString cleaned = part.trimmed();
        if (!cleaned.isEmpty()) {
            filtered << cleaned;
        }
    }
    return filtered.join(options.separator);
}

inline QStringList SmartNamer::segmentChinese(const QString &text) const
{
    QStringList segments;
    
    // 简化的中文分词：每 2-4 个字符作为一个词
    // 完整实现应使用 jieba 等分词库
    
    QString current;
    for (const QChar &c : text) {
        if (c.unicode() >= 0x4E00 && c.unicode() <= 0x9FFF) {
            current += c;
            if (current.length() >= 2) {
                if (!isStopWord(current)) {
                    segments << current;
                }
                current.clear();
            }
        } else {
            if (!current.isEmpty() && !isStopWord(current)) {
                segments << current;
            }
            current.clear();
        }
    }
    
    if (!current.isEmpty() && !isStopWord(current)) {
        segments << current;
    }
    
    return segments;
}

inline bool SmartNamer::isStopWord(const QString &word) const
{
    return m_stopWords.contains(word.toLower());
}

inline QMap<QString, int> SmartNamer::calculateWordFrequency(const QStringList &words) const
{
    QMap<QString, int> frequency;
    for (const QString &word : words) {
        frequency[word]++;
    }
    return frequency;
}

#endif // SMARTNAMER_H

