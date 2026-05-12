// 文件说明：app-static\export\wordexporter.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "wordexporter.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QBuffer>
#include <QDateTime>
#include <QRegularExpression>
#include <QImage>
#include <QUuid>
#include <QDebug>

// Qt 的 ZIP 支持
#include <QtCore/private/qzipreader_p.h>
#include <QtCore/private/qzipwriter_p.h>

// 函数说明：构造 WordExporter 对象，初始化本模块需要的状态、界面和资源。
WordExporter::WordExporter(QObject *parent)
    : QObject(parent)
    , m_imageCounter(0)
    , m_hyperlinkCounter(0)
{
    // 初始化默认标题样式
    for (int i = 1; i <= 6; ++i) {
        Style headingStyle;
        headingStyle.id = QString("Heading%1").arg(i);
        headingStyle.name = QString("标题 %1").arg(i);
        headingStyle.bold = (i <= 2);
        headingStyle.fontSize = 48 - (i - 1) * 4;  // H1=24pt, H2=22pt, ...
        headingStyle.spaceBefore = 240;
        headingStyle.spaceAfter = 120;
        m_headingStyles[i] = headingStyle;
    }
}

// 函数说明：销毁 WordExporter 对象，释放本模块持有的资源。
WordExporter::~WordExporter()
{
}

// 函数说明：设置 WordExporter 的运行参数，并触发必要的界面或数据刷新。
void WordExporter::setMetadata(const DocumentMetadata &metadata)
{
    m_metadata = metadata;
}

// 函数说明：设置 WordExporter 的运行参数，并触发必要的界面或数据刷新。
void WordExporter::setPageSetup(const PageSetup &setup)
{
    m_pageSetup = setup;
}

// 函数说明：设置 WordExporter 的运行参数，并触发必要的界面或数据刷新。
void WordExporter::setOptions(const ExportOptions &options)
{
    m_options = options;
}

// 函数说明：向 WordExporter 管理的数据集合中添加一项内容。
void WordExporter::addStyle(const Style &style)
{
    m_customStyles.append(style);
}

// 函数说明：设置 WordExporter 的运行参数，并触发必要的界面或数据刷新。
void WordExporter::setHeadingStyle(int level, const Style &style)
{
    if (level >= 1 && level <= 6) {
        m_headingStyles[level] = style;
    }
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
bool WordExporter::exportFromMarkdown(const QString &markdown, const QString &outputPath)
{
    emit exportProgress(10);

    // 转换 Markdown 为 WordML
    m_documentBody = convertToWordML(markdown);

    emit exportProgress(50);

    // 创建 DOCX 文件
    bool success = createDocx(outputPath);

    if (success) {
        emit exportProgress(100);
        emit exportCompleted(outputPath);
    }

    return success;
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
bool WordExporter::exportFromHtml(const QString &html, const QString &outputPath)
{
    // 简单的 HTML 到 Markdown 转换
    QString markdown = html;

    // 基本转换
    markdown.replace(QRegularExpression("<h1[^>]*>([^<]*)</h1>"), "# \\1\n\n");
    markdown.replace(QRegularExpression("<h2[^>]*>([^<]*)</h2>"), "## \\1\n\n");
    markdown.replace(QRegularExpression("<h3[^>]*>([^<]*)</h3>"), "### \\1\n\n");
    markdown.replace(QRegularExpression("<p[^>]*>([^<]*)</p>"), "\\1\n\n");
    markdown.replace(QRegularExpression("<strong>([^<]*)</strong>"), "**\\1**");
    markdown.replace(QRegularExpression("<em>([^<]*)</em>"), "*\\1*");
    markdown.replace(QRegularExpression("<[^>]+>"), "");

    return exportFromMarkdown(markdown, outputPath);
}

// 函数说明：创建 WordExporter 需要的对象、记录或输出内容。
bool WordExporter::createDocx(const QString &outputPath)
{
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly)) {
        m_lastError = tr("无法创建文件: %1").arg(outputPath);
        emit errorOccurred(m_lastError);
        return false;
    }

    QZipWriter zip(&file);
    zip.setCompressionPolicy(QZipWriter::AutoCompress);

    // [Content_Types].xml
    zip.addFile("[Content_Types].xml", createContentTypes());

    // _rels/.rels
    zip.addFile("_rels/.rels", createRels());

    // word/_rels/document.xml.rels
    zip.addFile("word/_rels/document.xml.rels", createDocumentRels());

    // word/document.xml
    zip.addFile("word/document.xml", createDocument());

    // word/styles.xml
    zip.addFile("word/styles.xml", createStyles());

    // word/settings.xml
    zip.addFile("word/settings.xml", createSettings());

    // word/fontTable.xml
    zip.addFile("word/fontTable.xml", createFontTable());

    // word/numbering.xml
    zip.addFile("word/numbering.xml", createNumbering());

    // docProps/core.xml
    zip.addFile("docProps/core.xml", createCoreProperties());

    // docProps/app.xml
    zip.addFile("docProps/app.xml", createAppProperties());

    // 添加嵌入的图片
    for (auto it = m_imageData.begin(); it != m_imageData.end(); ++it) {
        zip.addFile("word/media/" + it.key(), it.value());
    }

    zip.close();
    file.close();

    return true;
}

// 函数说明：创建 WordExporter 需要的对象、记录或输出内容。
QByteArray WordExporter::createContentTypes()
{
    QString xml = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Default Extension="png" ContentType="image/png"/>
  <Default Extension="jpg" ContentType="image/jpeg"/>
  <Default Extension="jpeg" ContentType="image/jpeg"/>
  <Default Extension="gif" ContentType="image/gif"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
  <Override PartName="/word/settings.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml"/>
  <Override PartName="/word/fontTable.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.fontTable+xml"/>
  <Override PartName="/word/numbering.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml"/>
  <Override PartName="/docProps/core.xml" ContentType="application/vnd.openxmlformats-package.core-properties+xml"/>
  <Override PartName="/docProps/app.xml" ContentType="application/vnd.openxmlformats-officedocument.extended-properties+xml"/>
</Types>)";

    return xml.toUtf8();
}

// 函数说明：创建 WordExporter 需要的对象、记录或输出内容。
QByteArray WordExporter::createRels()
{
    QString xml = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties" Target="docProps/core.xml"/>
  <Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties" Target="docProps/app.xml"/>
</Relationships>)";

    return xml.toUtf8();
}

// 函数说明：创建 WordExporter 需要的对象、记录或输出内容。
QByteArray WordExporter::createDocumentRels()
{
    QString xml = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings" Target="settings.xml"/>
  <Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/fontTable" Target="fontTable.xml"/>
  <Relationship Id="rId4" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering" Target="numbering.xml"/>)";

    // 添加图片关系
    int relId = 5;
    for (auto it = m_imageRelIds.begin(); it != m_imageRelIds.end(); ++it) {
        xml += QString(R"(
  <Relationship Id="%1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" Target="media/%2"/>)")
            .arg(it.value(), it.key());
    }

    // 添加超链接关系
    for (auto it = m_hyperlinkRelIds.begin(); it != m_hyperlinkRelIds.end(); ++it) {
        xml += QString(R"(
  <Relationship Id="%1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/hyperlink" Target="%2" TargetMode="External"/>)")
            .arg(it.value(), escapeXml(it.key()));
    }

    xml += "\n</Relationships>";

    return xml.toUtf8();
}

// 函数说明：创建 WordExporter 需要的对象、记录或输出内容。
QByteArray WordExporter::createDocument()
{
    QString xml = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"
            xmlns:wp="http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing"
            xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main"
            xmlns:pic="http://schemas.openxmlformats.org/drawingml/2006/picture">
  <w:body>
)";

    xml += m_documentBody;

    // 添加节属性（页面设置）
    xml += QString(R"(
    <w:sectPr>
      <w:pgSz w:w="%1" w:h="%2"%3/>
      <w:pgMar w:top="%4" w:right="%5" w:bottom="%6" w:left="%7"/>
    </w:sectPr>
  </w:body>
</w:document>)")
        .arg(m_pageSetup.pageWidth)
        .arg(m_pageSetup.pageHeight)
        .arg(m_pageSetup.landscape ? " w:orient=\"landscape\"" : "")
        .arg(m_pageSetup.marginTop)
        .arg(m_pageSetup.marginRight)
        .arg(m_pageSetup.marginBottom)
        .arg(m_pageSetup.marginLeft);

    return xml.toUtf8();
}

// 函数说明：创建 WordExporter 需要的对象、记录或输出内容。
QByteArray WordExporter::createStyles()
{
    QString xml = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:docDefaults>
    <w:rPrDefault>
      <w:rPr>
        <w:rFonts w:ascii=")" + m_options.defaultFontName + R"(" w:eastAsia=")" + m_options.defaultFontName + R"(" w:hAnsi=")" + m_options.defaultFontName + R"("/>
        <w:sz w:val=")" + QString::number(m_options.defaultFontSize) + R"("/>
        <w:szCs w:val=")" + QString::number(m_options.defaultFontSize) + R"("/>
        <w:lang w:val="en-US" w:eastAsia="zh-CN"/>
      </w:rPr>
    </w:rPrDefault>
    <w:pPrDefault>
      <w:pPr>
        <w:spacing w:after="200" w:line="276" w:lineRule="auto"/>
      </w:pPr>
    </w:pPrDefault>
  </w:docDefaults>
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
    <w:qFormat/>
  </w:style>)";

    // 添加标题样式
    for (auto it = m_headingStyles.begin(); it != m_headingStyles.end(); ++it) {
        const Style &style = it.value();
        xml += QString(R"(
  <w:style w:type="paragraph" w:styleId="%1">
    <w:name w:val="%2"/>
    <w:basedOn w:val="Normal"/>
    <w:next w:val="Normal"/>
    <w:qFormat/>
    <w:pPr>
      <w:spacing w:before="%3" w:after="%4"/>
      <w:outlineLvl w:val="%5"/>
    </w:pPr>
    <w:rPr>
      <w:sz w:val="%6"/>
      <w:szCs w:val="%6"/>%7
    </w:rPr>
  </w:style>)")
            .arg(style.id)
            .arg(style.name)
            .arg(style.spaceBefore)
            .arg(style.spaceAfter)
            .arg(it.key() - 1)
            .arg(style.fontSize)
            .arg(style.bold ? "\n      <w:b/>" : "");
    }

    // 代码块样式
    xml += R"(
  <w:style w:type="paragraph" w:styleId="Code">
    <w:name w:val="Code"/>
    <w:basedOn w:val="Normal"/>
    <w:pPr>
      <w:shd w:val="clear" w:fill="F5F5F5"/>
      <w:spacing w:before="120" w:after="120"/>
    </w:pPr>
    <w:rPr>
      <w:rFonts w:ascii="Consolas" w:hAnsi="Consolas" w:eastAsia="等线"/>
      <w:sz w:val="20"/>
      <w:szCs w:val="20"/>
    </w:rPr>
  </w:style>)";

    // 引用块样式
    xml += R"(
  <w:style w:type="paragraph" w:styleId="Quote">
    <w:name w:val="Quote"/>
    <w:basedOn w:val="Normal"/>
    <w:pPr>
      <w:pBdr>
        <w:left w:val="single" w:sz="24" w:space="4" w:color="CCCCCC"/>
      </w:pBdr>
      <w:ind w:left="720"/>
    </w:pPr>
    <w:rPr>
      <w:i/>
      <w:color w:val="666666"/>
    </w:rPr>
  </w:style>)";

    xml += "\n</w:styles>";

    return xml.toUtf8();
}

// 函数说明：创建 WordExporter 需要的对象、记录或输出内容。
QByteArray WordExporter::createSettings()
{
    QString xml = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:zoom w:percent="100"/>
  <w:defaultTabStop w:val="720"/>
  <w:characterSpacingControl w:val="doNotCompress"/>
  <w:compat>
    <w:useFELayout/>
  </w:compat>
</w:settings>)";

    return xml.toUtf8();
}

// 函数说明：创建 WordExporter 需要的对象、记录或输出内容。
QByteArray WordExporter::createFontTable()
{
    QString xml = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:fonts xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:font w:name="宋体">
    <w:panose1 w:val="02010600030101010101"/>
    <w:charset w:val="86"/>
    <w:family w:val="auto"/>
    <w:pitch w:val="variable"/>
  </w:font>
  <w:font w:name="Times New Roman">
    <w:panose1 w:val="02020603050405020304"/>
    <w:charset w:val="00"/>
    <w:family w:val="roman"/>
    <w:pitch w:val="variable"/>
  </w:font>
  <w:font w:name="Consolas">
    <w:panose1 w:val="020B0609020204030204"/>
    <w:charset w:val="00"/>
    <w:family w:val="modern"/>
    <w:pitch w:val="fixed"/>
  </w:font>
</w:fonts>)";

    return xml.toUtf8();
}

// 函数说明：创建 WordExporter 需要的对象、记录或输出内容。
QByteArray WordExporter::createNumbering()
{
    QString xml = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:abstractNum w:abstractNumId="0">
    <w:multiLevelType w:val="hybridMultilevel"/>
    <w:lvl w:ilvl="0">
      <w:start w:val="1"/>
      <w:numFmt w:val="bullet"/>
      <w:lvlText w:val="●"/>
      <w:lvlJc w:val="left"/>
      <w:pPr><w:ind w:left="720" w:hanging="360"/></w:pPr>
    </w:lvl>
    <w:lvl w:ilvl="1">
      <w:start w:val="1"/>
      <w:numFmt w:val="bullet"/>
      <w:lvlText w:val="○"/>
      <w:lvlJc w:val="left"/>
      <w:pPr><w:ind w:left="1440" w:hanging="360"/></w:pPr>
    </w:lvl>
  </w:abstractNum>
  <w:abstractNum w:abstractNumId="1">
    <w:multiLevelType w:val="hybridMultilevel"/>
    <w:lvl w:ilvl="0">
      <w:start w:val="1"/>
      <w:numFmt w:val="decimal"/>
      <w:lvlText w:val="%1."/>
      <w:lvlJc w:val="left"/>
      <w:pPr><w:ind w:left="720" w:hanging="360"/></w:pPr>
    </w:lvl>
    <w:lvl w:ilvl="1">
      <w:start w:val="1"/>
      <w:numFmt w:val="lowerLetter"/>
      <w:lvlText w:val="%2."/>
      <w:lvlJc w:val="left"/>
      <w:pPr><w:ind w:left="1440" w:hanging="360"/></w:pPr>
    </w:lvl>
  </w:abstractNum>
  <w:num w:numId="1"><w:abstractNumId w:val="0"/></w:num>
  <w:num w:numId="2"><w:abstractNumId w:val="1"/></w:num>
</w:numbering>)";

    return xml.toUtf8();
}

// 函数说明：创建 WordExporter 需要的对象、记录或输出内容。
QByteArray WordExporter::createCoreProperties()
{
    QString xml = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties"
                   xmlns:dc="http://purl.org/dc/elements/1.1/"
                   xmlns:dcterms="http://purl.org/dc/terms/"
                   xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">)";

    if (!m_metadata.title.isEmpty()) {
        xml += QString("\n  <dc:title>%1</dc:title>").arg(escapeXml(m_metadata.title));
    }
    if (!m_metadata.author.isEmpty()) {
        xml += QString("\n  <dc:creator>%1</dc:creator>").arg(escapeXml(m_metadata.author));
    }
    if (!m_metadata.subject.isEmpty()) {
        xml += QString("\n  <dc:subject>%1</dc:subject>").arg(escapeXml(m_metadata.subject));
    }
    if (!m_metadata.description.isEmpty()) {
        xml += QString("\n  <dc:description>%1</dc:description>").arg(escapeXml(m_metadata.description));
    }
    if (!m_metadata.keywords.isEmpty()) {
        xml += QString("\n  <cp:keywords>%1</cp:keywords>").arg(escapeXml(m_metadata.keywords));
    }

    xml += QString("\n  <dcterms:created xsi:type=\"dcterms:W3CDTF\">%1</dcterms:created>")
        .arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    xml += QString("\n  <dcterms:modified xsi:type=\"dcterms:W3CDTF\">%1</dcterms:modified>")
        .arg(QDateTime::currentDateTime().toString(Qt::ISODate));

    xml += "\n</cp:coreProperties>";

    return xml.toUtf8();
}

// 函数说明：创建 WordExporter 需要的对象、记录或输出内容。
QByteArray WordExporter::createAppProperties()
{
    QString xml = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Properties xmlns="http://schemas.openxmlformats.org/officeDocument/2006/extended-properties">
  <Application>CuteMarkEd</Application>
  <AppVersion>1.0</AppVersion>)";

    if (!m_metadata.company.isEmpty()) {
        xml += QString("\n  <Company>%1</Company>").arg(escapeXml(m_metadata.company));
    }

    xml += "\n</Properties>";

    return xml.toUtf8();
}

// 函数说明：实现 WordExporter::convertToWordML 的核心逻辑，供当前模块调用。
QString WordExporter::convertToWordML(const QString &markdown)
{
    QString result;
    QStringList lines = markdown.split('\n');

    bool inCodeBlock = false;
    QString codeBlockContent;
    QString codeLanguage;
    bool inList = false;
    bool orderedList = false;
    int listLevel = 0;

    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i];

        // 代码块
        if (line.startsWith("```")) {
            if (!inCodeBlock) {
                inCodeBlock = true;
                codeLanguage = line.mid(3).trimmed();
                codeBlockContent.clear();
            } else {
                result += createCodeBlock(codeBlockContent, codeLanguage);
                inCodeBlock = false;
            }
            continue;
        }

        if (inCodeBlock) {
            codeBlockContent += line + "\n";
            continue;
        }

        // 空行
        if (line.trimmed().isEmpty()) {
            if (inList) {
                inList = false;
            }
            continue;
        }

        // 标题
        QRegularExpression headingRegex("^(#{1,6})\\s+(.+)$");
        QRegularExpressionMatch headingMatch = headingRegex.match(line);
        if (headingMatch.hasMatch()) {
            int level = headingMatch.captured(1).length();
            QString text = headingMatch.captured(2);
            result += createHeading(text, level);
            continue;
        }

        // 无序列表
        QRegularExpression ulRegex("^(\\s*)[-*+]\\s+(.+)$");
        QRegularExpressionMatch ulMatch = ulRegex.match(line);
        if (ulMatch.hasMatch()) {
            int indent = ulMatch.captured(1).length() / 2;
            QString text = ulMatch.captured(2);
            result += createListItem(text, false, indent);
            inList = true;
            orderedList = false;
            continue;
        }

        // 有序列表
        QRegularExpression olRegex("^(\\s*)\\d+\\.\\s+(.+)$");
        QRegularExpressionMatch olMatch = olRegex.match(line);
        if (olMatch.hasMatch()) {
            int indent = olMatch.captured(1).length() / 2;
            QString text = olMatch.captured(2);
            result += createListItem(text, true, indent);
            inList = true;
            orderedList = true;
            continue;
        }

        // 引用块
        if (line.startsWith(">")) {
            QString text = line.mid(1).trimmed();
            result += createBlockquote(text);
            continue;
        }

        // 水平线
        if (line.trimmed().length() >= 3 &&
            (line.trimmed().replace("-", "").isEmpty() ||
             line.trimmed().replace("*", "").isEmpty() ||
             line.trimmed().replace("_", "").isEmpty())) {
            result += R"(<w:p><w:pPr><w:pBdr><w:bottom w:val="single" w:sz="6" w:space="1" w:color="auto"/></w:pBdr></w:pPr></w:p>)";
            continue;
        }

        // 普通段落（处理内联格式）
        result += createParagraph(line);
    }

    return result;
}

// 函数说明：创建 WordExporter 需要的对象、记录或输出内容。
QString WordExporter::createParagraph(const QString &text, const QString &styleId)
{
    QString xml = "<w:p>";

    if (!styleId.isEmpty()) {
        xml += QString("<w:pPr><w:pStyle w:val=\"%1\"/></w:pPr>").arg(styleId);
    }

    // 处理内联格式
    QString remaining = text;

    // 图片 ![alt](url)
    QRegularExpression imgRegex("!\\[([^\\]]*)\\]\\(([^)]+)\\)");
    remaining.replace(imgRegex, "");  // 图片单独处理

    // 链接 [text](url)
    QRegularExpression linkRegex("\\[([^\\]]+)\\]\\(([^)]+)\\)");
    QRegularExpressionMatchIterator linkIt = linkRegex.globalMatch(text);

    int lastPos = 0;
    QString processed;

    while (linkIt.hasNext()) {
        QRegularExpressionMatch match = linkIt.next();
        // 添加链接前的文本
        processed += createRunWithFormatting(text.mid(lastPos, match.capturedStart() - lastPos));
        // 添加链接
        processed += createHyperlink(match.captured(1), match.captured(2));
        lastPos = match.capturedEnd();
    }

    if (lastPos < text.length()) {
        processed += createRunWithFormatting(text.mid(lastPos));
    }

    xml += processed.isEmpty() ? createRunWithFormatting(text) : processed;
    xml += "</w:p>";

    return xml;
}

// 函数说明：创建 WordExporter 需要的对象、记录或输出内容。
QString WordExporter::createRunWithFormatting(const QString &text)
{
    QString result;
    QString remaining = text;

    // 处理 **bold** 和 __bold__
    QRegularExpression boldRegex("\\*\\*([^*]+)\\*\\*|__([^_]+)__");
    // 处理 *italic* 和 _italic_
    QRegularExpression italicRegex("\\*([^*]+)\\*|_([^_]+)_");
    // 处理 ~~strikethrough~~
    QRegularExpression strikeRegex("~~([^~]+)~~");
    // 处理 `code`
    QRegularExpression codeRegex("`([^`]+)`");

    // 简化处理：先处理粗体
    int pos = 0;
    QRegularExpressionMatchIterator boldIt = boldRegex.globalMatch(remaining);

    while (boldIt.hasNext()) {
        QRegularExpressionMatch match = boldIt.next();
        if (match.capturedStart() > pos) {
            result += createRun(remaining.mid(pos, match.capturedStart() - pos));
        }
        QString boldText = match.captured(1).isEmpty() ? match.captured(2) : match.captured(1);
        result += createRun(boldText, true, false, false);
        pos = match.capturedEnd();
    }

    if (pos == 0) {
        // 没有粗体，检查斜体
        QRegularExpressionMatchIterator italicIt = italicRegex.globalMatch(remaining);
        while (italicIt.hasNext()) {
            QRegularExpressionMatch match = italicIt.next();
            if (match.capturedStart() > pos) {
                result += createRun(remaining.mid(pos, match.capturedStart() - pos));
            }
            QString italicText = match.captured(1).isEmpty() ? match.captured(2) : match.captured(1);
            result += createRun(italicText, false, true, false);
            pos = match.capturedEnd();
        }
    }

    if (pos < remaining.length()) {
        result += createRun(remaining.mid(pos));
    }

    if (result.isEmpty()) {
        result = createRun(text);
    }

    return result;
}

// 函数说明：创建 WordExporter 需要的对象、记录或输出内容。
QString WordExporter::createHeading(const QString &text, int level)
{
    QString styleId = QString("Heading%1").arg(level);
    return createParagraph(text, styleId);
}

// 函数说明：创建 WordExporter 需要的对象、记录或输出内容。
QString WordExporter::createRun(const QString &text, bool bold, bool italic,
                                 bool strike, const QString &color)
{
    if (text.isEmpty()) return QString();

    QString xml = "<w:r>";

    bool hasProps = bold || italic || strike || !color.isEmpty();
    if (hasProps) {
        xml += "<w:rPr>";
        if (bold) xml += "<w:b/>";
        if (italic) xml += "<w:i/>";
        if (strike) xml += "<w:strike/>";
        if (!color.isEmpty()) {
            xml += QString("<w:color w:val=\"%1\"/>").arg(color);
        }
        xml += "</w:rPr>";
    }

    xml += QString("<w:t xml:space=\"preserve\">%1</w:t>").arg(escapeXml(text));
    xml += "</w:r>";

    return xml;
}

// 函数说明：创建 WordExporter 需要的对象、记录或输出内容。
QString WordExporter::createHyperlink(const QString &text, const QString &url)
{
    QString relId = getHyperlinkRelId(url);

    return QString(R"(<w:hyperlink r:id="%1"><w:r><w:rPr><w:color w:val="0000FF"/><w:u w:val="single"/></w:rPr><w:t>%2</w:t></w:r></w:hyperlink>)")
        .arg(relId, escapeXml(text));
}

// 函数说明：创建 WordExporter 需要的对象、记录或输出内容。
QString WordExporter::createImage(const QString &imagePath, int width, int height)
{
    QString relId = embedImage(imagePath);
    if (relId.isEmpty()) return QString();

    // EMU (English Metric Units): 1 inch = 914400 EMU
    int cx = width * 9525;   // 像素转 EMU (假设 96 DPI)
    int cy = height * 9525;

    return QString(R"(<w:p><w:r>
<w:drawing>
  <wp:inline distT="0" distB="0" distL="0" distR="0">
    <wp:extent cx="%1" cy="%2"/>
    <wp:docPr id="%3" name="Picture %3"/>
    <a:graphic>
      <a:graphicData uri="http://schemas.openxmlformats.org/drawingml/2006/picture">
        <pic:pic>
          <pic:nvPicPr>
            <pic:cNvPr id="%3" name="Picture %3"/>
            <pic:cNvPicPr/>
          </pic:nvPicPr>
          <pic:blipFill>
            <a:blip r:embed="%4"/>
            <a:stretch><a:fillRect/></a:stretch>
          </pic:blipFill>
          <pic:spPr>
            <a:xfrm><a:off x="0" y="0"/><a:ext cx="%1" cy="%2"/></a:xfrm>
            <a:prstGeom prst="rect"><a:avLst/></a:prstGeom>
          </pic:spPr>
        </pic:pic>
      </a:graphicData>
    </a:graphic>
  </wp:inline>
</w:drawing>
</w:r></w:p>)")
        .arg(cx).arg(cy).arg(m_imageCounter).arg(relId);
}

// 函数说明：创建 WordExporter 需要的对象、记录或输出内容。
QString WordExporter::createTable(const QStringList &headers, const QVector<QStringList> &rows)
{
    QString xml = R"(<w:tbl>
<w:tblPr>
  <w:tblW w:w="5000" w:type="pct"/>
  <w:tblBorders>
    <w:top w:val="single" w:sz="4" w:color="000000"/>
    <w:left w:val="single" w:sz="4" w:color="000000"/>
    <w:bottom w:val="single" w:sz="4" w:color="000000"/>
    <w:right w:val="single" w:sz="4" w:color="000000"/>
    <w:insideH w:val="single" w:sz="4" w:color="000000"/>
    <w:insideV w:val="single" w:sz="4" w:color="000000"/>
  </w:tblBorders>
</w:tblPr>
)";

    // 表头
    xml += "<w:tr>";
    for (const QString &header : headers) {
        xml += QString(R"(<w:tc><w:tcPr><w:shd w:val="clear" w:fill="E0E0E0"/></w:tcPr><w:p><w:r><w:rPr><w:b/></w:rPr><w:t>%1</w:t></w:r></w:p></w:tc>)")
            .arg(escapeXml(header));
    }
    xml += "</w:tr>";

    // 数据行
    for (const QStringList &row : rows) {
        xml += "<w:tr>";
        for (const QString &cell : row) {
            xml += QString("<w:tc><w:p><w:r><w:t>%1</w:t></w:r></w:p></w:tc>")
                .arg(escapeXml(cell));
        }
        xml += "</w:tr>";
    }

    xml += "</w:tbl>";
    return xml;
}

// 函数说明：创建 WordExporter 需要的对象、记录或输出内容。
QString WordExporter::createListItem(const QString &text, bool ordered, int level)
{
    int numId = ordered ? 2 : 1;

    return QString(R"(<w:p>
  <w:pPr>
    <w:numPr>
      <w:ilvl w:val="%1"/>
      <w:numId w:val="%2"/>
    </w:numPr>
  </w:pPr>
  %3
</w:p>)")
        .arg(level)
        .arg(numId)
        .arg(createRunWithFormatting(text));
}

// 函数说明：创建 WordExporter 需要的对象、记录或输出内容。
QString WordExporter::createCodeBlock(const QString &code, const QString &language)
{
    Q_UNUSED(language)

    QString result;
    QStringList lines = code.split('\n');

    for (const QString &line : lines) {
        if (line.isEmpty() && &line == &lines.last()) continue;

        result += QString(R"(<w:p>
  <w:pPr><w:pStyle w:val="Code"/></w:pPr>
  <w:r><w:rPr><w:rFonts w:ascii="Consolas" w:hAnsi="Consolas"/></w:rPr><w:t xml:space="preserve">%1</w:t></w:r>
</w:p>)").arg(escapeXml(line));
    }

    return result;
}

// 函数说明：创建 WordExporter 需要的对象、记录或输出内容。
QString WordExporter::createBlockquote(const QString &text)
{
    return QString(R"(<w:p>
  <w:pPr><w:pStyle w:val="Quote"/></w:pPr>
  %1
</w:p>)").arg(createRunWithFormatting(text));
}

// 函数说明：实现 WordExporter::embedImage 的核心逻辑，供当前模块调用。
QString WordExporter::embedImage(const QString &imagePath)
{
    if (m_imageRelIds.contains(imagePath)) {
        return m_imageRelIds[imagePath];
    }

    QFile file(imagePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }

    QByteArray imageData = file.readAll();
    file.close();

    m_imageCounter++;
    QString fileName = QString("image%1.%2")
        .arg(m_imageCounter)
        .arg(QFileInfo(imagePath).suffix().toLower());

    QString relId = QString("rIdImg%1").arg(m_imageCounter);

    m_imageRelIds[imagePath] = relId;
    m_imageData[fileName] = imageData;
    m_imageRelIds[fileName] = relId;

    return relId;
}

// 函数说明：读取 WordExporter 当前保存的状态或计算结果。
QString WordExporter::getHyperlinkRelId(const QString &url)
{
    if (m_hyperlinkRelIds.contains(url)) {
        return m_hyperlinkRelIds[url];
    }

    m_hyperlinkCounter++;
    QString relId = QString("rIdLink%1").arg(m_hyperlinkCounter);
    m_hyperlinkRelIds[url] = relId;

    return relId;
}

// 函数说明：读取 WordExporter 当前保存的状态或计算结果。
QString WordExporter::getImageRelId(const QString &imagePath)
{
    return m_imageRelIds.value(imagePath);
}

// 函数说明：实现 WordExporter::escapeXml 的核心逻辑，供当前模块调用。
QString WordExporter::escapeXml(const QString &text)
{
    QString escaped = text;
    escaped.replace("&", "&amp;");
    escaped.replace("<", "&lt;");
    escaped.replace(">", "&gt;");
    escaped.replace("\"", "&quot;");
    escaped.replace("'", "&apos;");
    return escaped;
}

// 函数说明：实现 WordExporter::twipsFromMm 的核心逻辑，供当前模块调用。
int WordExporter::twipsFromMm(double mm)
{
    return static_cast<int>(mm * 56.7);  // 1mm ≈ 56.7 twips
}

// 函数说明：实现 WordExporter::twipsFromPt 的核心逻辑，供当前模块调用。
int WordExporter::twipsFromPt(double pt)
{
    return static_cast<int>(pt * 20);  // 1pt = 20 twips
}

