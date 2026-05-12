// 文件说明：app-static\export\wordexporter.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef WORDEXPORTER_H
#define WORDEXPORTER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QByteArray>

/**
 * @brief Word 文档导出器
 *
 * 将 Markdown 导出为 .docx 格式（Office Open XML）
 * DOCX 文件实际是包含 XML 文件的 ZIP 压缩包
 *
 * 支持的格式：
 * - 标题（H1-H6）
 * - 段落
 * - 粗体、斜体、删除线
 * - 有序/无序列表
 * - 代码块
 * - 引用块
 * - 表格
 * - 图片（嵌入）
 * - 超链接
 */
class WordExporter : public QObject
{
    Q_OBJECT

public:
    // 文档元数据
    struct DocumentMetadata {
        QString title;
        QString author;
        QString subject;
        QString description;
        QString keywords;
        QString company;
        QString category;
        QString language;

        DocumentMetadata() : language("zh-CN") {}
    };

    // 页面设置
    struct PageSetup {
        int pageWidth;      // 单位: twips (1/20 点)
        int pageHeight;
        int marginTop;
        int marginBottom;
        int marginLeft;
        int marginRight;
        bool landscape;     // 横向

        PageSetup()
            : pageWidth(12240)   // A4 宽度 (210mm)
            , pageHeight(15840)  // A4 高度 (297mm)
            , marginTop(1440)    // 1 英寸
            , marginBottom(1440)
            , marginLeft(1440)
            , marginRight(1440)
            , landscape(false)
        {}
    };

    // 样式定义
    struct Style {
        QString id;
        QString name;
        QString basedOn;
        QString fontName;
        int fontSize;       // 半点
        bool bold;
        bool italic;
        QString color;      // RGB hex
        int spaceBefore;    // twips
        int spaceAfter;
        QString alignment;  // left, center, right, both

        Style()
            : fontSize(24)  // 12pt
            , bold(false)
            , italic(false)
            , spaceBefore(0)
            , spaceAfter(200)
            , alignment("left")
        {}
    };

    // 导出选项
    struct ExportOptions {
        bool embedImages;           // 嵌入图片
        bool generateTOC;           // 生成目录
        bool useBuiltinStyles;      // 使用内置样式
        bool convertCodeBlocks;     // 转换代码块
        QString defaultFontName;
        int defaultFontSize;

        ExportOptions()
            : embedImages(true)
            , generateTOC(false)
            , useBuiltinStyles(true)
            , convertCodeBlocks(true)
            , defaultFontName("宋体")
            , defaultFontSize(24)   // 12pt
        {}
    };

    explicit WordExporter(QObject *parent = nullptr);
    ~WordExporter();

    // 设置
    void setMetadata(const DocumentMetadata &metadata);
    DocumentMetadata metadata() const { return m_metadata; }

    void setPageSetup(const PageSetup &setup);
    PageSetup pageSetup() const { return m_pageSetup; }

    void setOptions(const ExportOptions &options);
    ExportOptions options() const { return m_options; }

    // 自定义样式
    void addStyle(const Style &style);
    void setHeadingStyle(int level, const Style &style);

    // 导出
    bool exportFromMarkdown(const QString &markdown, const QString &outputPath);
    bool exportFromHtml(const QString &html, const QString &outputPath);

    // 错误信息
    QString lastError() const { return m_lastError; }

signals:
    void exportProgress(int percent);
    void exportCompleted(const QString &path);
    void errorOccurred(const QString &error);

private:
    // DOCX 文件生成
    bool createDocx(const QString &outputPath);
    QByteArray createContentTypes();
    QByteArray createRels();
    QByteArray createDocumentRels();
    QByteArray createDocument();
    QByteArray createStyles();
    QByteArray createSettings();
    QByteArray createFontTable();
    QByteArray createNumbering();
    QByteArray createCoreProperties();
    QByteArray createAppProperties();

    // Markdown 解析
    void parseMarkdown(const QString &markdown);
    QString convertToWordML(const QString &markdown);

    // WordML 生成辅助
    QString createParagraph(const QString &text, const QString &styleId = QString());
    QString createHeading(const QString &text, int level);
    QString createRun(const QString &text, bool bold = false, bool italic = false,
                      bool strike = false, const QString &color = QString());
    QString createHyperlink(const QString &text, const QString &url);
    QString createImage(const QString &imagePath, int width, int height);
    QString createTable(const QStringList &headers, const QVector<QStringList> &rows);
    QString createListItem(const QString &text, bool ordered, int level);
    QString createCodeBlock(const QString &code, const QString &language);
    QString createBlockquote(const QString &text);

    // 图片处理
    QString embedImage(const QString &imagePath);
    QString getImageRelId(const QString &imagePath);
    QString getHyperlinkRelId(const QString &url);

    // 辅助函数
    QString escapeXml(const QString &text);
    QString createRunWithFormatting(const QString &text);
    int twipsFromMm(double mm);
    int twipsFromPt(double pt);

    DocumentMetadata m_metadata;
    PageSetup m_pageSetup;
    ExportOptions m_options;
    QMap<int, Style> m_headingStyles;
    QVector<Style> m_customStyles;
    QString m_lastError;

    // 文档内容
    QString m_documentBody;
    QMap<QString, QString> m_imageRelIds;
    QMap<QString, QByteArray> m_imageData;
    int m_imageCounter;
    int m_hyperlinkCounter;
    QMap<QString, QString> m_hyperlinkRelIds;
};

#endif // WORDEXPORTER_H

