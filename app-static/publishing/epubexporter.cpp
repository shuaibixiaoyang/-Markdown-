// 文件说明：app-static\publishing\epubexporter.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "epubexporter.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QBuffer>
#include <QImageWriter>
#include <QUuid>
#include <QRegularExpression>
#include <QXmlStreamWriter>
#include <QProcess>

// 使用 QuaZip 或手动实现 ZIP（这里使用 QProcess 调用系统 zip）
// 如果项目集成了 QuaZip，可以替换为 QuaZip 实现

EpubExporter::EpubExporter(QObject *parent)
    : QObject(parent)
{
}

// 函数说明：销毁 EpubExporter 对象，释放本模块持有的资源。
EpubExporter::~EpubExporter()
{
}

// 函数说明：设置 EpubExporter 的运行参数，并触发必要的界面或数据刷新。
void EpubExporter::setMetadata(const BookMetadata &metadata)
{
    m_metadata = metadata;
    ensureMetadataDefaults();
}

// 函数说明：设置 EpubExporter 的运行参数，并触发必要的界面或数据刷新。
void EpubExporter::setOptions(const ExportOptions &options)
{
    m_options = options;
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
bool EpubExporter::exportFromMarkdown(const QString &markdown, const QString &outputPath)
{
    emit progressChanged(10);

    // Markdown 转 HTML
    QString html = markdownToHtml(markdown);

    emit progressChanged(30);

    return exportFromHtml(html, outputPath);
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
bool EpubExporter::exportFromHtml(const QString &html, const QString &outputPath)
{
    // 解析 HTML 为章节
    m_chapters = parseHtmlToChapters(html);

    if (m_chapters.isEmpty()) {
        // 如果没有章节，创建一个默认章节
        Chapter chapter;
        chapter.id = "chapter1";
        chapter.title = m_metadata.title.isEmpty() ? tr("正文") : m_metadata.title;
        chapter.htmlContent = html;
        chapter.level = 1;
        m_chapters.append(chapter);
    }

    emit progressChanged(50);

    return exportToFile(outputPath);
}

// 函数说明：向 EpubExporter 管理的数据集合中添加一项内容。
void EpubExporter::addChapter(const Chapter &chapter)
{
    m_chapters.append(chapter);
}

// 函数说明：清空 EpubExporter 保存的临时状态或缓存数据。
void EpubExporter::clearChapters()
{
    m_chapters.clear();
    m_images.clear();
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
bool EpubExporter::exportToFile(const QString &outputPath)
{
    ensureMetadataDefaults();

    // 创建临时目录
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        m_lastError = tr("无法创建临时目录");
        emit exportCompleted(false);
        return false;
    }

    // 创建 EPUB 结构
    if (!createEpubStructure(tempDir.path())) {
        emit exportCompleted(false);
        return false;
    }

    emit progressChanged(80);

    // 压缩为 EPUB
    if (!compressToEpub(tempDir.path(), outputPath)) {
        emit exportCompleted(false);
        return false;
    }

    emit progressChanged(100);
    emit exportCompleted(true);
    return true;
}

// 函数说明：创建 EpubExporter 需要的对象、记录或输出内容。
bool EpubExporter::createEpubStructure(const QString &tempDir)
{
    QDir dir(tempDir);

    // 创建目录结构
    // EPUB/
    //   mimetype
    //   META-INF/
    //     container.xml
    //   OEBPS/
    //     content.opf
    //     nav.xhtml
    //     toc.ncx
    //     styles/
    //       stylesheet.css
    //     chapters/
    //       chapter1.xhtml
    //       ...
    //     images/
    //       ...

    dir.mkpath("META-INF");
    dir.mkpath("OEBPS/styles");
    dir.mkpath("OEBPS/chapters");
    dir.mkpath("OEBPS/images");

    // 写入 mimetype（必须是第一个文件，不压缩）
    QFile mimetypeFile(dir.filePath("mimetype"));
    if (mimetypeFile.open(QIODevice::WriteOnly)) {
        mimetypeFile.write(generateMimetype());
        mimetypeFile.close();
    }

    // 写入 container.xml
    QFile containerFile(dir.filePath("META-INF/container.xml"));
    if (containerFile.open(QIODevice::WriteOnly)) {
        containerFile.write(generateContainerXml());
        containerFile.close();
    }

    // 写入 content.opf
    QFile opfFile(dir.filePath("OEBPS/content.opf"));
    if (opfFile.open(QIODevice::WriteOnly)) {
        opfFile.write(generateContentOpf());
        opfFile.close();
    }

    // 写入 nav.xhtml (EPUB 3)
    QFile navFile(dir.filePath("OEBPS/nav.xhtml"));
    if (navFile.open(QIODevice::WriteOnly)) {
        navFile.write(generateNavXhtml());
        navFile.close();
    }

    // 写入 toc.ncx (EPUB 2 兼容)
    QFile ncxFile(dir.filePath("OEBPS/toc.ncx"));
    if (ncxFile.open(QIODevice::WriteOnly)) {
        ncxFile.write(generateNcxToc());
        ncxFile.close();
    }

    // 写入样式表
    QFile cssFile(dir.filePath("OEBPS/styles/stylesheet.css"));
    if (cssFile.open(QIODevice::WriteOnly)) {
        cssFile.write(generateStylesheet());
        cssFile.close();
    }

    // 写入封面
    if (!m_metadata.coverImage.isNull()) {
        QString coverPath = dir.filePath("OEBPS/images/cover.jpg");
        m_metadata.coverImage.save(coverPath, "JPEG", 90);

        // 生成封面页
        QFile coverXhtml(dir.filePath("OEBPS/cover.xhtml"));
        if (coverXhtml.open(QIODevice::WriteOnly)) {
            coverXhtml.write(generateCoverXhtml());
            coverXhtml.close();
        }
    }

    // 写入各章节
    for (int i = 0; i < m_chapters.size(); ++i) {
        QString filename = QString("OEBPS/chapters/chapter%1.xhtml").arg(i + 1);
        QFile chapterFile(dir.filePath(filename));
        if (chapterFile.open(QIODevice::WriteOnly)) {
            chapterFile.write(generateChapterXhtml(m_chapters[i]));
            chapterFile.close();
        }

        // 复制章节中的图片
        for (const QString &imgPath : m_chapters[i].images) {
            QFileInfo imgInfo(imgPath);
            QString destPath = dir.filePath("OEBPS/images/" + imgInfo.fileName());
            QFile::copy(imgPath, destPath);
        }
    }

    return true;
}

// 函数说明：根据当前数据生成 EpubExporter 需要的输出结果。
QByteArray EpubExporter::generateMimetype()
{
    return "application/epub+zip";
}

// 函数说明：根据当前数据生成 EpubExporter 需要的输出结果。
QByteArray EpubExporter::generateContainerXml()
{
    QByteArray data;
    QXmlStreamWriter xml(&data);
    xml.setAutoFormatting(true);

    xml.writeStartDocument();
    xml.writeStartElement("container");
    xml.writeAttribute("version", "1.0");
    xml.writeAttribute("xmlns", "urn:oasis:names:tc:opendocument:xmlns:container");

    xml.writeStartElement("rootfiles");
    xml.writeEmptyElement("rootfile");
    xml.writeAttribute("full-path", "OEBPS/content.opf");
    xml.writeAttribute("media-type", "application/oebps-package+xml");
    xml.writeEndElement(); // rootfiles

    xml.writeEndElement(); // container
    xml.writeEndDocument();

    return data;
}

// 函数说明：根据当前数据生成 EpubExporter 需要的输出结果。
QByteArray EpubExporter::generateContentOpf()
{
    QByteArray data;
    QXmlStreamWriter xml(&data);
    xml.setAutoFormatting(true);

    xml.writeStartDocument();
    xml.writeStartElement("package");
    xml.writeAttribute("xmlns", "http://www.idpf.org/2007/opf");
    xml.writeAttribute("version", "3.0");
    xml.writeAttribute("unique-identifier", "BookId");

    // Metadata
    xml.writeStartElement("metadata");
    xml.writeAttribute("xmlns:dc", "http://purl.org/dc/elements/1.1/");

    xml.writeStartElement("dc:identifier");
    xml.writeAttribute("id", "BookId");
    xml.writeCharacters(m_metadata.identifier);
    xml.writeEndElement();

    xml.writeTextElement("dc:title", m_metadata.title);
    xml.writeTextElement("dc:creator", m_metadata.author);
    xml.writeTextElement("dc:language", m_metadata.language);

    if (!m_metadata.publisher.isEmpty()) {
        xml.writeTextElement("dc:publisher", m_metadata.publisher);
    }
    if (!m_metadata.description.isEmpty()) {
        xml.writeTextElement("dc:description", m_metadata.description);
    }
    if (!m_metadata.subject.isEmpty()) {
        xml.writeTextElement("dc:subject", m_metadata.subject);
    }
    if (!m_metadata.rights.isEmpty()) {
        xml.writeTextElement("dc:rights", m_metadata.rights);
    }

    xml.writeTextElement("dc:date", m_metadata.date.toString(Qt::ISODate));

    xml.writeEmptyElement("meta");
    xml.writeAttribute("property", "dcterms:modified");
    xml.writeCharacters(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    xml.writeEndElement(); // metadata

    // Manifest
    xml.writeStartElement("manifest");

    // NAV
    xml.writeEmptyElement("item");
    xml.writeAttribute("id", "nav");
    xml.writeAttribute("href", "nav.xhtml");
    xml.writeAttribute("media-type", "application/xhtml+xml");
    xml.writeAttribute("properties", "nav");

    // NCX
    xml.writeEmptyElement("item");
    xml.writeAttribute("id", "ncx");
    xml.writeAttribute("href", "toc.ncx");
    xml.writeAttribute("media-type", "application/x-dtbncx+xml");

    // Stylesheet
    xml.writeEmptyElement("item");
    xml.writeAttribute("id", "css");
    xml.writeAttribute("href", "styles/stylesheet.css");
    xml.writeAttribute("media-type", "text/css");

    // Cover
    if (!m_metadata.coverImage.isNull()) {
        xml.writeEmptyElement("item");
        xml.writeAttribute("id", "cover-image");
        xml.writeAttribute("href", "images/cover.jpg");
        xml.writeAttribute("media-type", "image/jpeg");
        xml.writeAttribute("properties", "cover-image");

        xml.writeEmptyElement("item");
        xml.writeAttribute("id", "cover");
        xml.writeAttribute("href", "cover.xhtml");
        xml.writeAttribute("media-type", "application/xhtml+xml");
    }

    // Chapters
    for (int i = 0; i < m_chapters.size(); ++i) {
        xml.writeEmptyElement("item");
        xml.writeAttribute("id", QString("chapter%1").arg(i + 1));
        xml.writeAttribute("href", QString("chapters/chapter%1.xhtml").arg(i + 1));
        xml.writeAttribute("media-type", "application/xhtml+xml");
    }

    xml.writeEndElement(); // manifest

    // Spine
    xml.writeStartElement("spine");
    xml.writeAttribute("toc", "ncx");

    if (!m_metadata.coverImage.isNull()) {
        xml.writeEmptyElement("itemref");
        xml.writeAttribute("idref", "cover");
        xml.writeAttribute("linear", "no");
    }

    for (int i = 0; i < m_chapters.size(); ++i) {
        xml.writeEmptyElement("itemref");
        xml.writeAttribute("idref", QString("chapter%1").arg(i + 1));
    }

    xml.writeEndElement(); // spine

    xml.writeEndElement(); // package
    xml.writeEndDocument();

    return data;
}

// 函数说明：根据当前数据生成 EpubExporter 需要的输出结果。
QByteArray EpubExporter::generateNavXhtml()
{
    QByteArray data;
    QXmlStreamWriter xml(&data);
    xml.setAutoFormatting(true);

    xml.writeStartDocument();
    xml.writeDTD("<!DOCTYPE html>");
    xml.writeStartElement("html");
    xml.writeAttribute("xmlns", "http://www.w3.org/1999/xhtml");
    xml.writeAttribute("xmlns:epub", "http://www.idpf.org/2007/ops");

    xml.writeStartElement("head");
    xml.writeTextElement("title", tr("目录"));
    xml.writeEndElement();

    xml.writeStartElement("body");
    xml.writeStartElement("nav");
    xml.writeAttribute("epub:type", "toc");
    xml.writeAttribute("id", "toc");

    xml.writeTextElement("h1", tr("目录"));

    xml.writeStartElement("ol");
    for (int i = 0; i < m_chapters.size(); ++i) {
        xml.writeStartElement("li");
        xml.writeStartElement("a");
        xml.writeAttribute("href", QString("chapters/chapter%1.xhtml").arg(i + 1));
        xml.writeCharacters(m_chapters[i].title);
        xml.writeEndElement(); // a
        xml.writeEndElement(); // li
    }
    xml.writeEndElement(); // ol

    xml.writeEndElement(); // nav
    xml.writeEndElement(); // body
    xml.writeEndElement(); // html
    xml.writeEndDocument();

    return data;
}

// 函数说明：根据当前数据生成 EpubExporter 需要的输出结果。
QByteArray EpubExporter::generateNcxToc()
{
    QByteArray data;
    QXmlStreamWriter xml(&data);
    xml.setAutoFormatting(true);

    xml.writeStartDocument();
    xml.writeDTD("<!DOCTYPE ncx PUBLIC \"-//NISO//DTD ncx 2005-1//EN\" "
                 "\"http://www.daisy.org/z3986/2005/ncx-2005-1.dtd\">");
    xml.writeStartElement("ncx");
    xml.writeAttribute("xmlns", "http://www.daisy.org/z3986/2005/ncx/");
    xml.writeAttribute("version", "2005-1");

    xml.writeStartElement("head");
    xml.writeEmptyElement("meta");
    xml.writeAttribute("name", "dtb:uid");
    xml.writeAttribute("content", m_metadata.identifier);
    xml.writeEndElement();

    xml.writeStartElement("docTitle");
    xml.writeTextElement("text", m_metadata.title);
    xml.writeEndElement();

    xml.writeStartElement("navMap");
    for (int i = 0; i < m_chapters.size(); ++i) {
        xml.writeStartElement("navPoint");
        xml.writeAttribute("id", QString("navpoint%1").arg(i + 1));
        xml.writeAttribute("playOrder", QString::number(i + 1));

        xml.writeStartElement("navLabel");
        xml.writeTextElement("text", m_chapters[i].title);
        xml.writeEndElement();

        xml.writeEmptyElement("content");
        xml.writeAttribute("src", QString("chapters/chapter%1.xhtml").arg(i + 1));

        xml.writeEndElement(); // navPoint
    }
    xml.writeEndElement(); // navMap

    xml.writeEndElement(); // ncx
    xml.writeEndDocument();

    return data;
}

// 函数说明：根据当前数据生成 EpubExporter 需要的输出结果。
QByteArray EpubExporter::generateChapterXhtml(const Chapter &chapter)
{
    QByteArray data;
    QXmlStreamWriter xml(&data);
    xml.setAutoFormatting(true);

    xml.writeStartDocument();
    xml.writeDTD("<!DOCTYPE html>");
    xml.writeStartElement("html");
    xml.writeAttribute("xmlns", "http://www.w3.org/1999/xhtml");

    xml.writeStartElement("head");
    xml.writeTextElement("title", chapter.title);
    xml.writeEmptyElement("link");
    xml.writeAttribute("rel", "stylesheet");
    xml.writeAttribute("type", "text/css");
    xml.writeAttribute("href", "../styles/stylesheet.css");
    xml.writeEndElement();

    xml.writeStartElement("body");

    // 写入章节内容（已经是 HTML）
    xml.writeStartElement("div");
    xml.writeAttribute("class", "chapter");
    xml.device()->write(chapter.htmlContent.toUtf8());
    xml.writeEndElement();

    xml.writeEndElement(); // body
    xml.writeEndElement(); // html
    xml.writeEndDocument();

    return data;
}

// 函数说明：根据当前数据生成 EpubExporter 需要的输出结果。
QByteArray EpubExporter::generateStylesheet()
{
    QString css = m_options.cssContent;
    const QString compatibilityCss = R"(
/* CuteMarkEd EPUB compatibility layer */
html, body {
    margin: 0;
    padding: 0;
}

article, aside, figcaption, figure, footer, header, main, nav, section {
    display: block;
}

body {
    -webkit-text-size-adjust: 100%;
    text-size-adjust: 100%;
    word-wrap: break-word;
    overflow-wrap: break-word;
    -webkit-hyphens: auto;
    -epub-hyphens: auto;
    hyphens: auto;
}

h1, h2, h3, h4, h5, h6 {
    page-break-after: avoid;
    break-after: avoid-page;
}

p, li {
    widows: 2;
    orphans: 2;
}

pre {
    white-space: pre-wrap;
    word-break: break-word;
    overflow: visible;
    page-break-inside: avoid;
    break-inside: avoid;
}

table {
    display: block;
    width: auto;
    overflow-x: auto;
    -webkit-overflow-scrolling: touch;
}

img, svg {
    max-width: 100%;
    height: auto;
    page-break-inside: avoid;
    break-inside: avoid;
}

a {
    word-break: break-word;
}

.chapter {
    page-break-before: always;
    break-before: page;
}
)";
    const QString modernCss = R"(
/* CuteMarkEd EPUB modern layer */
:root {
    color-scheme: light;
}

body {
    max-width: 42em;
    margin: 0 auto;
    padding: 0 1rem 1.5rem;
    text-rendering: optimizeLegibility;
}

h1, h2, h3, h4, h5, h6 {
    line-height: 1.25;
}

p {
    hanging-punctuation: first last;
}

pre {
    overflow-x: auto;
    white-space: pre;
}

.chapter {
    break-before: page;
}
)";

    if (css.trimmed().isEmpty()) {
        css = R"(
/* EPUB 默认样式 */
body {
    font-family: "Noto Serif SC", "Source Han Serif CN", "PingFang SC", "Microsoft YaHei", serif;
    font-size: 1em;
    line-height: 1.6;
    margin: 1em;
    text-align: justify;
}

h1, h2, h3, h4, h5, h6 {
    font-weight: bold;
    margin-top: 1.5em;
    margin-bottom: 0.5em;
}

h1 { font-size: 2em; text-align: center; }
h2 { font-size: 1.5em; }
h3 { font-size: 1.25em; }
h4 { font-size: 1.1em; }

p {
    margin: 0.5em 0;
    text-indent: 2em;
}

blockquote {
    margin: 1em 2em;
    padding-left: 1em;
    border-left: 3px solid #ccc;
    color: #666;
}

pre, code {
    font-family: "Source Code Pro", monospace;
    font-size: 0.9em;
    background: #f5f5f5;
}

pre {
    padding: 1em;
    overflow-x: auto;
    border-radius: 4px;
}

code {
    padding: 0.2em 0.4em;
    border-radius: 3px;
}

img {
    max-width: 100%;
    height: auto;
    display: block;
    margin: 1em auto;
}

table {
    width: 100%;
    border-collapse: collapse;
    margin: 1em 0;
}

th, td {
    border: 1px solid #ddd;
    padding: 0.5em;
    text-align: left;
}

th {
    background: #f0f0f0;
}

a {
    color: #0066cc;
    text-decoration: none;
}

.chapter {
    page-break-before: always;
}
)";
    }

    if (!css.endsWith('\n')) {
        css += '\n';
    }
    if (m_options.cssProfile == CssProfile::Modern) {
        css += modernCss;
    } else {
        css += compatibilityCss;
    }

    return css.toUtf8();
}

// 函数说明：根据当前数据生成 EpubExporter 需要的输出结果。
QByteArray EpubExporter::generateCoverXhtml()
{
    QString xhtml = R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
    <title>封面</title>
    <style>
        body { margin: 0; padding: 0; text-align: center; }
        img { max-width: 100%; max-height: 100%; }
    </style>
</head>
<body>
    <img src="images/cover.jpg" alt="封面"/>
</body>
</html>)";

    return xhtml.toUtf8();
}

// 函数说明：实现 EpubExporter::markdownToHtml 的核心逻辑，供当前模块调用。
QString EpubExporter::markdownToHtml(const QString &markdown)
{
    // 使用项目已有的 Markdown 转换器
    // 这里提供一个简化实现，实际应调用 DiscountMarkdownConverter

    QString html = markdown;

    // 基本转换（简化版，实际应使用完整的 Markdown 解析器）
    // 标题
    html.replace(QRegularExpression("^###### (.+)$", QRegularExpression::MultilineOption), "<h6>\\1</h6>");
    html.replace(QRegularExpression("^##### (.+)$", QRegularExpression::MultilineOption), "<h5>\\1</h5>");
    html.replace(QRegularExpression("^#### (.+)$", QRegularExpression::MultilineOption), "<h4>\\1</h4>");
    html.replace(QRegularExpression("^### (.+)$", QRegularExpression::MultilineOption), "<h3>\\1</h3>");
    html.replace(QRegularExpression("^## (.+)$", QRegularExpression::MultilineOption), "<h2>\\1</h2>");
    html.replace(QRegularExpression("^# (.+)$", QRegularExpression::MultilineOption), "<h1>\\1</h1>");

    // 粗体和斜体
    html.replace(QRegularExpression("\\*\\*(.+?)\\*\\*"), "<strong>\\1</strong>");
    html.replace(QRegularExpression("\\*(.+?)\\*"), "<em>\\1</em>");

    // 代码
    html.replace(QRegularExpression("`([^`]+)`"), "<code>\\1</code>");

    // 段落
    html.replace(QRegularExpression("\n\n"), "</p><p>");
    html = "<p>" + html + "</p>";

    return html;
}

// 函数说明：解析输入内容，转换为 EpubExporter 后续处理使用的数据结构。
QVector<EpubExporter::Chapter> EpubExporter::parseHtmlToChapters(const QString &html)
{
    QVector<Chapter> chapters;

    if (!m_options.splitByHeading) {
        return chapters;
    }

    // 按标题拆分
    QString pattern = QString("<h([1-%1])[^>]*>(.+?)</h\\1>").arg(m_options.splitLevel);
    QRegularExpression headingRegex(pattern, QRegularExpression::DotMatchesEverythingOption);

    QRegularExpressionMatchIterator it = headingRegex.globalMatch(html);

    int lastPos = 0;
    int chapterNum = 0;

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();

        if (chapterNum > 0 && lastPos < match.capturedStart()) {
            // 保存上一章的内容
            QString content = html.mid(lastPos, match.capturedStart() - lastPos);
            if (!chapters.isEmpty()) {
                chapters.last().htmlContent = content;
            }
        }

        Chapter chapter;
        chapter.id = QString("chapter%1").arg(++chapterNum);
        chapter.title = match.captured(2).remove(QRegularExpression("<[^>]+>"));
        chapter.level = match.captured(1).toInt();
        chapters.append(chapter);

        lastPos = match.capturedStart();
    }

    // 最后一章
    if (!chapters.isEmpty() && lastPos < html.length()) {
        chapters.last().htmlContent = html.mid(lastPos);
    }

    return chapters;
}

// 函数说明：实现 EpubExporter::extractImages 的核心逻辑，供当前模块调用。
QStringList EpubExporter::extractImages(const QString &html)
{
    QStringList images;

    QRegularExpression imgRegex("<img[^>]+src=[\"']([^\"']+)[\"']");
    QRegularExpressionMatchIterator it = imgRegex.globalMatch(html);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        images.append(match.captured(1));
    }

    return images;
}

// 函数说明：实现 EpubExporter::compressToEpub 的核心逻辑，供当前模块调用。
bool EpubExporter::compressToEpub(const QString &tempDir, const QString &outputPath)
{
    // 删除已存在的文件
    if (QFile::exists(outputPath)) {
        QFile::remove(outputPath);
    }

    // 使用 QProcess 调用 zip 命令
    // EPUB 要求 mimetype 文件必须是第一个且不压缩
    QProcess process;
    process.setWorkingDirectory(tempDir);

#ifdef Q_OS_WIN
    // Windows 使用 PowerShell 的 Compress-Archive
    QString script = QString(
        "Add-Type -AssemblyName System.IO.Compression.FileSystem; "
        "[System.IO.Compression.ZipFile]::CreateFromDirectory('%1', '%2')"
    ).arg(tempDir).arg(outputPath);

    process.start("powershell", QStringList() << "-Command" << script);
#else
    // Unix/Mac 使用 zip 命令
    // 先添加 mimetype（不压缩）
    process.start("zip", QStringList() << "-0" << "-X" << outputPath << "mimetype");
    if (!process.waitForFinished(30000)) {
        m_lastError = tr("压缩失败: %1").arg(process.errorString());
        return false;
    }

    // 添加其他文件
    process.start("zip", QStringList() << "-r" << "-X" << outputPath
                  << "META-INF" << "OEBPS");
#endif

    if (!process.waitForFinished(60000)) {
        m_lastError = tr("压缩超时");
        return false;
    }

    if (process.exitCode() != 0) {
        m_lastError = tr("压缩失败: %1").arg(QString::fromUtf8(process.readAllStandardError()));
        return false;
    }

    return true;
}

// 函数说明：根据当前数据生成 EpubExporter 需要的输出结果。
QString EpubExporter::generateUUID()
{
    return "urn:uuid:" + QUuid::createUuid().toString(QUuid::WithoutBraces);
}

// 函数说明：实现 EpubExporter::ensureMetadataDefaults 的核心逻辑，供当前模块调用。
void EpubExporter::ensureMetadataDefaults()
{
    if (m_metadata.identifier.trimmed().isEmpty()) {
        m_metadata.identifier = generateUUID();
    }

    if (m_metadata.language.trimmed().isEmpty()) {
        m_metadata.language = QStringLiteral("zh-CN");
    }

    if (!m_metadata.date.isValid()) {
        m_metadata.date = QDateTime::currentDateTime();
    }

    if (m_metadata.title.trimmed().isEmpty()) {
        m_metadata.title = tr("未命名文档");
    }
}

