// 文件说明：app-static\publishing\epubexporter.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef EPUBEXPORTER_H
#define EPUBEXPORTER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QDateTime>
#include <QImage>

/**
 * @brief EPUB 电子书导出器
 *
 * 将 Markdown 文档导出为 EPUB 3.0 格式电子书
 * EPUB 本质上是 ZIP 压缩的 XML/XHTML 文件集合
 *
 * 支持特性:
 * - 章节自动拆分（按标题级别）
 * - 目录生成 (NCX/NAV)
 * - 封面图片
 * - 内嵌图片
 * - 自定义 CSS 样式
 * - 元数据（作者、标题、语言等）
 */
class EpubExporter : public QObject
{
    Q_OBJECT

public:
    // CSS 输出模式
    enum class CssProfile {
        StrictCompatibility, // 最大化阅读器兼容
        Modern               // 现代阅读器样式
    };

    // 书籍元数据
    struct BookMetadata {
        QString title;
        QString author;
        QString language;
        QString publisher;
        QString description;
        QString subject;
        QString rights;
        QDateTime date;
        QString identifier;     // ISBN 或 UUID
        QImage coverImage;

        BookMetadata()
            : language("zh-CN")
            , date(QDateTime::currentDateTime())
        {}
    };

    // 章节信息
    struct Chapter {
        QString id;
        QString title;
        QString htmlContent;
        int level;              // 标题级别 1-6
        QStringList images;     // 章节中的图片路径
    };

    // 导出选项
    struct ExportOptions {
        bool splitByHeading;    // 按标题拆分章节
        int splitLevel;         // 拆分级别 (1-3)
        bool includeCSS;        // 包含自定义样式
        QString cssContent;     // 自定义 CSS
        bool embedFonts;        // 嵌入字体
        QStringList fontPaths;  // 字体文件路径
        bool generateTOC;       // 生成目录
        int tocDepth;           // 目录深度
        CssProfile cssProfile;  // CSS 输出模式

        ExportOptions()
            : splitByHeading(true)
            , splitLevel(2)
            , includeCSS(true)
            , embedFonts(false)
            , generateTOC(true)
            , tocDepth(3)
            , cssProfile(CssProfile::StrictCompatibility)
        {}
    };

    explicit EpubExporter(QObject *parent = nullptr);
    ~EpubExporter();

    // 设置元数据
    void setMetadata(const BookMetadata &metadata);
    BookMetadata metadata() const { return m_metadata; }

    // 设置导出选项
    void setOptions(const ExportOptions &options);
    ExportOptions options() const { return m_options; }

    // 从 Markdown 导出
    bool exportFromMarkdown(const QString &markdown, const QString &outputPath);

    // 从 HTML 导出
    bool exportFromHtml(const QString &html, const QString &outputPath);

    // 添加章节
    void addChapter(const Chapter &chapter);
    void clearChapters();

    // 直接导出（已添加的章节）
    bool exportToFile(const QString &outputPath);

    // 获取错误信息
    QString lastError() const { return m_lastError; }

signals:
    void progressChanged(int percent);
    void exportCompleted(bool success);

private:
    // 生成 EPUB 文件结构
    bool createEpubStructure(const QString &tempDir);

    // 生成各个文件
    QByteArray generateMimetype();
    QByteArray generateContainerXml();
    QByteArray generateContentOpf();
    QByteArray generateNavXhtml();
    QByteArray generateNcxToc();
    QByteArray generateChapterXhtml(const Chapter &chapter);
    QByteArray generateStylesheet();
    QByteArray generateCoverXhtml();

    // Markdown 转 HTML
    QString markdownToHtml(const QString &markdown);

    // 解析 HTML 为章节
    QVector<Chapter> parseHtmlToChapters(const QString &html);

    // 提取图片
    QStringList extractImages(const QString &html);

    // 压缩为 EPUB
    bool compressToEpub(const QString &tempDir, const QString &outputPath);

    // 保证导出时元数据完整
    void ensureMetadataDefaults();

    // 生成唯一 ID
    QString generateUUID();

    BookMetadata m_metadata;
    ExportOptions m_options;
    QVector<Chapter> m_chapters;
    QStringList m_images;
    QString m_lastError;
    QString m_basePath;         // 原始文档路径（用于解析相对图片路径）
};

#endif // EPUBEXPORTER_H

