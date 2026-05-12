// 文件说明：app-static\export\latexexporter.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef LATEXEXPORTER_H
#define LATEXEXPORTER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>

/**
 * @brief LaTeX 导出器
 *
 * 将 Markdown 导出为 LaTeX 格式，支持学术论文格式
 *
 * 支持的文档类：
 * - article (普通文章)
 * - report (报告)
 * - book (书籍)
 * - beamer (演示文稿)
 * - 自定义模板
 *
 * 支持的格式：
 * - 标题和章节
 * - 粗体、斜体
 * - 列表
 * - 代码块（listings/minted）
 * - 数学公式（保留原始 LaTeX）
 * - 表格
 * - 图片
 * - 引用（BibTeX）
 * - 脚注
 */
class LaTeXExporter : public QObject
{
    Q_OBJECT

public:
    // 文档类型
    enum class DocumentClass {
        Article,
        Report,
        Book,
        Beamer,
        CTEXArticle,    // 中文文章
        CTEXReport,     // 中文报告
        CTEXBook,       // 中文书籍
        Custom
    };
    Q_ENUM(DocumentClass)

    // 代码高亮方案
    enum class CodeHighlight {
        None,
        Listings,
        Minted,
        Auto
    };
    Q_ENUM(CodeHighlight)

    // 文档元数据
    struct DocumentMetadata {
        QString title;
        QString author;
        QString date;           // 留空则使用 \today
        QString institution;    // 机构
        QString email;
        QString abstract;
        QStringList keywords;

        DocumentMetadata() {}
    };

    // 文档选项
    struct DocumentOptions {
        DocumentClass documentClass;
        QString paperSize;          // a4paper, letterpaper, etc.
        QString fontSize;           // 10pt, 11pt, 12pt
        bool twoside;
        bool twoColumn;
        QString customPreamble;     // 自定义导言区
        QString customTemplate;     // 自定义模板路径
        CodeHighlight codeHighlight;
        bool numberSections;
        bool tableOfContents;
        bool listOfFigures;
        bool listOfTables;
        QString bibliographyFile;   // .bib 文件路径
        QString bibliographyStyle;  // plain, alpha, ieee, etc.
        bool useHyperref;           // 超链接支持
        bool useCJK;                // 中文支持

        DocumentOptions()
            : documentClass(DocumentClass::Article)
            , paperSize("a4paper")
            , fontSize("12pt")
            , twoside(false)
            , twoColumn(false)
            , codeHighlight(CodeHighlight::Auto)
            , numberSections(true)
            , tableOfContents(false)
            , listOfFigures(false)
            , listOfTables(false)
            , bibliographyStyle("plain")
            , useHyperref(true)
            , useCJK(true)
        {}
    };

    explicit LaTeXExporter(QObject *parent = nullptr);
    ~LaTeXExporter();

    // 设置
    void setMetadata(const DocumentMetadata &metadata);
    DocumentMetadata metadata() const { return m_metadata; }

    void setOptions(const DocumentOptions &options);
    DocumentOptions options() const { return m_options; }

    // 自定义命令
    void addPackage(const QString &package, const QString &options = QString());
    void addPreambleCommand(const QString &command);

    // 导出
    bool exportFromMarkdown(const QString &markdown, const QString &outputPath);
    QString convertToLaTeX(const QString &markdown);

    // 错误信息
    QString lastError() const { return m_lastError; }

signals:
    void exportCompleted(const QString &path);
    void errorOccurred(const QString &error);

private:
    // LaTeX 生成
    QString generatePreamble();
    QString generateDocumentClass();
    QString generatePackages();
    QString generateTitlePage();
    QString generateTableOfContents();
    QString generateBibliography();

    // Markdown 转换
    QString convertHeading(const QString &text, int level);
    QString convertParagraph(const QString &text);
    QString convertBold(const QString &text);
    QString convertItalic(const QString &text);
    QString convertStrikethrough(const QString &text);
    QString convertCode(const QString &code);
    QString convertCodeBlock(const QString &code, const QString &language);
    QString convertBlockquote(const QString &text);
    QString convertUnorderedList(const QStringList &items);
    QString convertOrderedList(const QStringList &items);
    QString convertTable(const QStringList &headers, const QVector<QStringList> &rows);
    QString convertImage(const QString &alt, const QString &path, const QString &caption = QString());
    QString convertLink(const QString &text, const QString &url);
    QString convertFootnote(const QString &text);
    QString convertMath(const QString &math, bool display);
    QString convertCitation(const QString &key);

    // 辅助函数
    QString escapeLatex(const QString &text);
    QString languageToListings(const QString &lang);
    QString convertMarkdownBody(const QString &markdown);
    QString processInlineFormatting(const QString &text);
    CodeHighlight effectiveCodeHighlight() const;

    DocumentMetadata m_metadata;
    DocumentOptions m_options;
    QStringList m_packages;
    QMap<QString, QString> m_packageOptions;
    QStringList m_preambleCommands;
    QString m_lastError;

    int m_figureCounter;
    int m_tableCounter;
};

#endif // LATEXEXPORTER_H

