// 文件说明：app-static\export\latexexporter.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "latexexporter.h"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDebug>
#include <QVector>
#include <functional>

namespace {
bool isEscapedAt(const QString &text, int index)
{
    int backslashCount = 0;
    for (int i = index - 1; i >= 0 && text[i] == '\\'; --i) {
        ++backslashCount;
    }
    return (backslashCount % 2) == 1;
}
} // namespace

// 函数说明：构造 LaTeXExporter 对象，初始化本模块需要的状态、界面和资源。
LaTeXExporter::LaTeXExporter(QObject *parent)
    : QObject(parent)
    , m_figureCounter(0)
    , m_tableCounter(0)
{
}

// 函数说明：销毁 LaTeXExporter 对象，释放本模块持有的资源。
LaTeXExporter::~LaTeXExporter()
{
}

// 函数说明：设置 LaTeXExporter 的运行参数，并触发必要的界面或数据刷新。
void LaTeXExporter::setMetadata(const DocumentMetadata &metadata)
{
    m_metadata = metadata;
}

// 函数说明：设置 LaTeXExporter 的运行参数，并触发必要的界面或数据刷新。
void LaTeXExporter::setOptions(const DocumentOptions &options)
{
    m_options = options;
}

// 函数说明：向 LaTeXExporter 管理的数据集合中添加一项内容。
void LaTeXExporter::addPackage(const QString &package, const QString &options)
{
    if (!m_packages.contains(package)) {
        m_packages.append(package);
        if (!options.isEmpty()) {
            m_packageOptions[package] = options;
        }
    }
}

// 函数说明：向 LaTeXExporter 管理的数据集合中添加一项内容。
void LaTeXExporter::addPreambleCommand(const QString &command)
{
    m_preambleCommands.append(command);
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
bool LaTeXExporter::exportFromMarkdown(const QString &markdown, const QString &outputPath)
{
    QString latex = convertToLaTeX(markdown);

    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = tr("无法创建文件: %1").arg(outputPath);
        emit errorOccurred(m_lastError);
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << latex;
    file.close();

    emit exportCompleted(outputPath);
    return true;
}

// 函数说明：实现 LaTeXExporter::convertToLaTeX 的核心逻辑，供当前模块调用。
QString LaTeXExporter::convertToLaTeX(const QString &markdown)
{
    QString result;

    // 导言区
    result += generatePreamble();

    // 文档开始
    result += "\\begin{document}\n\n";

    // 标题页
    if (!m_metadata.title.isEmpty()) {
        result += generateTitlePage();
    }

    // 目录
    if (m_options.tableOfContents) {
        result += generateTableOfContents();
    }

    // 正文
    result += convertMarkdownBody(markdown);

    // 参考文献
    if (!m_options.bibliographyFile.isEmpty()) {
        result += generateBibliography();
    }

    // 文档结束
    result += "\n\\end{document}\n";

    return result;
}

// 函数说明：根据当前数据生成 LaTeXExporter 需要的输出结果。
QString LaTeXExporter::generatePreamble()
{
    QString preamble;

    // 文档类
    preamble += generateDocumentClass();

    // 包
    preamble += generatePackages();

    // 标题信息
    if (!m_metadata.title.isEmpty()) {
        preamble += QString("\\title{%1}\n").arg(escapeLatex(m_metadata.title));
    }
    if (!m_metadata.author.isEmpty()) {
        QString authorLine = escapeLatex(m_metadata.author);
        if (!m_metadata.institution.isEmpty()) {
            authorLine += QString(" \\\\ %1").arg(escapeLatex(m_metadata.institution));
        }
        if (!m_metadata.email.isEmpty()) {
            authorLine += QString(" \\\\ \\texttt{%1}").arg(escapeLatex(m_metadata.email));
        }
        preamble += QString("\\author{%1}\n").arg(authorLine);
    }
    if (!m_metadata.date.isEmpty()) {
        preamble += QString("\\date{%1}\n").arg(escapeLatex(m_metadata.date));
    } else {
        preamble += "\\date{\\today}\n";
    }

    // 自定义命令
    for (const QString &cmd : m_preambleCommands) {
        preamble += cmd + "\n";
    }

    // 自定义导言区
    if (!m_options.customPreamble.isEmpty()) {
        preamble += m_options.customPreamble + "\n";
    }

    preamble += "\n";
    return preamble;
}

// 函数说明：根据当前数据生成 LaTeXExporter 需要的输出结果。
QString LaTeXExporter::generateDocumentClass()
{
    QString docClass;
    QStringList options;

    options << m_options.paperSize << m_options.fontSize;

    if (m_options.twoside) options << "twoside";
    if (m_options.twoColumn) options << "twocolumn";

    switch (m_options.documentClass) {
        case DocumentClass::Article:
            docClass = "article";
            break;
        case DocumentClass::Report:
            docClass = "report";
            break;
        case DocumentClass::Book:
            docClass = "book";
            break;
        case DocumentClass::Beamer:
            docClass = "beamer";
            break;
        case DocumentClass::CTEXArticle:
            docClass = "ctexart";
            break;
        case DocumentClass::CTEXReport:
            docClass = "ctexrep";
            break;
        case DocumentClass::CTEXBook:
            docClass = "ctexbook";
            break;
        case DocumentClass::Custom:
            docClass = "article";
            break;
    }

    return QString("\\documentclass[%1]{%2}\n").arg(options.join(","), docClass);
}

// 函数说明：根据当前数据生成 LaTeXExporter 需要的输出结果。
QString LaTeXExporter::generatePackages()
{
    QString packages;
    const CodeHighlight highlight = effectiveCodeHighlight();

    // 基础包
    QStringList defaultPackages;

    // 编码和字体
    if (m_options.useCJK && m_options.documentClass != DocumentClass::CTEXArticle &&
        m_options.documentClass != DocumentClass::CTEXReport &&
        m_options.documentClass != DocumentClass::CTEXBook) {
        defaultPackages << "ctex";
    }

    defaultPackages << "inputenc" << "fontenc";

    // 图形
    defaultPackages << "graphicx" << "float";

    // 表格
    defaultPackages << "booktabs" << "array" << "longtable";

    // 数学
    defaultPackages << "amsmath" << "amssymb" << "amsfonts";

    // 代码高亮
    if (highlight == CodeHighlight::Listings) {
        defaultPackages << "listings" << "xcolor";
    } else if (highlight == CodeHighlight::Minted) {
        defaultPackages << "minted";
    }

    // 超链接
    if (m_options.useHyperref) {
        defaultPackages << "hyperref";
    }

    // 引用
    if (!m_options.bibliographyFile.isEmpty()) {
        defaultPackages << "natbib";
    }

    // 其他
    defaultPackages << "geometry" << "fancyhdr" << "enumitem";

    // 删除线支持
    defaultPackages << "ulem";

    // 合并用户指定的包
    for (const QString &pkg : m_packages) {
        if (!defaultPackages.contains(pkg)) {
            defaultPackages << pkg;
        }
    }

    // 生成 usepackage 命令
    for (const QString &pkg : defaultPackages) {
        if (m_packageOptions.contains(pkg)) {
            packages += QString("\\usepackage[%1]{%2}\n")
                .arg(m_packageOptions[pkg], pkg);
        } else if (pkg == "inputenc") {
            packages += "\\usepackage[utf8]{inputenc}\n";
        } else if (pkg == "fontenc") {
            packages += "\\usepackage[T1]{fontenc}\n";
        } else if (pkg == "geometry") {
            packages += "\\usepackage[margin=1in]{geometry}\n";
        } else if (pkg == "hyperref") {
            packages += "\\usepackage[colorlinks=true,linkcolor=blue,urlcolor=blue,citecolor=blue]{hyperref}\n";
        } else {
            packages += QString("\\usepackage{%1}\n").arg(pkg);
        }
    }

    // listings 设置
    if (highlight == CodeHighlight::Listings) {
        packages += R"(
\lstset{
    basicstyle=\ttfamily\small,
    keywordstyle=\color{blue},
    commentstyle=\color{green!60!black},
    stringstyle=\color{red},
    numbers=left,
    numberstyle=\tiny\color{gray},
    breaklines=true,
    frame=single,
    backgroundcolor=\color{gray!10},
    tabsize=4,
    showstringspaces=false
}
)";
    }

    return packages;
}

// 函数说明：根据当前数据生成 LaTeXExporter 需要的输出结果。
QString LaTeXExporter::generateTitlePage()
{
    QString titlePage;

    titlePage += "\\maketitle\n\n";

    // 摘要
    if (!m_metadata.abstract.isEmpty()) {
        titlePage += "\\begin{abstract}\n";
        titlePage += escapeLatex(m_metadata.abstract) + "\n";
        titlePage += "\\end{abstract}\n\n";
    }

    // 关键词
    if (!m_metadata.keywords.isEmpty()) {
        titlePage += "\\textbf{关键词：} ";
        titlePage += escapeLatex(m_metadata.keywords.join("，")) + "\n\n";
    }

    if (m_options.documentClass == DocumentClass::Report ||
        m_options.documentClass == DocumentClass::Book ||
        m_options.documentClass == DocumentClass::CTEXReport ||
        m_options.documentClass == DocumentClass::CTEXBook) {
        titlePage += "\\newpage\n\n";
    }

    return titlePage;
}

// 函数说明：根据当前数据生成 LaTeXExporter 需要的输出结果。
QString LaTeXExporter::generateTableOfContents()
{
    QString toc;

    toc += "\\tableofcontents\n";

    if (m_options.listOfFigures) {
        toc += "\\listoffigures\n";
    }

    if (m_options.listOfTables) {
        toc += "\\listoftables\n";
    }

    toc += "\\newpage\n\n";

    return toc;
}

// 函数说明：根据当前数据生成 LaTeXExporter 需要的输出结果。
QString LaTeXExporter::generateBibliography()
{
    QString bib;

    bib += "\n\\bibliographystyle{" + m_options.bibliographyStyle + "}\n";
    bib += "\\bibliography{" + QFileInfo(m_options.bibliographyFile).baseName() + "}\n";

    return bib;
}

// 函数说明：实现 LaTeXExporter::convertMarkdownBody 的核心逻辑，供当前模块调用。
QString LaTeXExporter::convertMarkdownBody(const QString &markdown)
{
    QString result;
    QStringList lines = markdown.split('\n');

    bool inCodeBlock = false;
    QString codeBlockContent;
    QString codeLanguage;
    QStringList listItems;
    bool inUnorderedList = false;
    bool inOrderedList = false;

    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i];

        // 代码块
        if (line.startsWith("```")) {
            if (!inCodeBlock) {
                inCodeBlock = true;
                codeLanguage = line.mid(3).trimmed();
                codeBlockContent.clear();
            } else {
                result += convertCodeBlock(codeBlockContent, codeLanguage);
                inCodeBlock = false;
            }
            continue;
        }

        if (inCodeBlock) {
            codeBlockContent += line + "\n";
            continue;
        }

        // 结束列表
        if (inUnorderedList && !line.trimmed().startsWith("-") &&
            !line.trimmed().startsWith("*") && !line.trimmed().startsWith("+")) {
            result += convertUnorderedList(listItems);
            listItems.clear();
            inUnorderedList = false;
        }

        if (inOrderedList && !QRegularExpression("^\\d+\\.").match(line.trimmed()).hasMatch()) {
            result += convertOrderedList(listItems);
            listItems.clear();
            inOrderedList = false;
        }

        // 空行
        if (line.trimmed().isEmpty()) {
            result += "\n";
            continue;
        }

        // 标题
        QRegularExpression headingRegex("^(#{1,6})\\s+(.+)$");
        QRegularExpressionMatch headingMatch = headingRegex.match(line);
        if (headingMatch.hasMatch()) {
            int level = headingMatch.captured(1).length();
            QString text = headingMatch.captured(2);
            result += convertHeading(text, level);
            continue;
        }

        // 无序列表
        QRegularExpression ulRegex("^[-*+]\\s+(.+)$");
        QRegularExpressionMatch ulMatch = ulRegex.match(line.trimmed());
        if (ulMatch.hasMatch()) {
            listItems << ulMatch.captured(1);
            inUnorderedList = true;
            continue;
        }

        // 有序列表
        QRegularExpression olRegex("^\\d+\\.\\s+(.+)$");
        QRegularExpressionMatch olMatch = olRegex.match(line.trimmed());
        if (olMatch.hasMatch()) {
            listItems << olMatch.captured(1);
            inOrderedList = true;
            continue;
        }

        // 引用块
        if (line.startsWith(">")) {
            result += convertBlockquote(line.mid(1).trimmed());
            continue;
        }

        // 水平线
        if (line.trimmed().length() >= 3 &&
            (line.trimmed().replace("-", "").isEmpty() ||
             line.trimmed().replace("*", "").isEmpty())) {
            result += "\\hrulefill\n\n";
            continue;
        }

        // 图片
        QRegularExpression imgRegex("!\\[([^\\]]*)\\]\\(([^)]+)\\)");
        QRegularExpressionMatch imgMatch = imgRegex.match(line);
        if (imgMatch.hasMatch()) {
            result += convertImage(imgMatch.captured(1), imgMatch.captured(2));
            continue;
        }

        // 普通段落
        result += convertParagraph(line);
    }

    // 处理剩余列表
    if (inUnorderedList && !listItems.isEmpty()) {
        result += convertUnorderedList(listItems);
    }
    if (inOrderedList && !listItems.isEmpty()) {
        result += convertOrderedList(listItems);
    }

    return result;
}

// 函数说明：实现 LaTeXExporter::convertHeading 的核心逻辑，供当前模块调用。
QString LaTeXExporter::convertHeading(const QString &text, int level)
{
    QString cmd;

    switch (m_options.documentClass) {
        case DocumentClass::Book:
        case DocumentClass::CTEXBook:
            switch (level) {
                case 1: cmd = "chapter"; break;
                case 2: cmd = "section"; break;
                case 3: cmd = "subsection"; break;
                case 4: cmd = "subsubsection"; break;
                case 5: cmd = "paragraph"; break;
                case 6: cmd = "subparagraph"; break;
            }
            break;

        case DocumentClass::Report:
        case DocumentClass::CTEXReport:
            switch (level) {
                case 1: cmd = "chapter"; break;
                case 2: cmd = "section"; break;
                case 3: cmd = "subsection"; break;
                case 4: cmd = "subsubsection"; break;
                default: cmd = "paragraph"; break;
            }
            break;

        default:
            switch (level) {
                case 1: cmd = "section"; break;
                case 2: cmd = "subsection"; break;
                case 3: cmd = "subsubsection"; break;
                case 4: cmd = "paragraph"; break;
                default: cmd = "subparagraph"; break;
            }
    }

    if (!m_options.numberSections) {
        cmd += "*";
    }

    return QString("\\%1{%2}\n\n").arg(cmd, processInlineFormatting(text));
}

// 函数说明：实现 LaTeXExporter::convertParagraph 的核心逻辑，供当前模块调用。
QString LaTeXExporter::convertParagraph(const QString &text)
{
    return processInlineFormatting(text) + "\n\n";
}

// 函数说明：处理 LaTeXExporter 的核心业务数据，并输出处理结果。
QString LaTeXExporter::processInlineFormatting(const QString &text)
{
    QString result = text;
    QVector<QPair<QString, QString>> placeholders;
    int placeholderCounter = 0;

    auto escapePlainText = [](const QString &input) -> QString {
        QString escaped;
        escaped.reserve(input.size() * 2);

        for (int i = 0; i < input.size(); ++i) {
            const QChar c = input[i];
            if (c == '\\') {
                if (i + 1 < input.size()) {
                    const QChar next = input[i + 1];
                    if (next == '#' || next == '$' || next == '%' || next == '&' ||
                        next == '_' || next == '{' || next == '}') {
                        escaped += c;
                        escaped += next;
                        ++i;
                        continue;
                    }
                }
                escaped += c;
                continue;
            }

            if (c == '#') escaped += "\\#";
            else if (c == '$') escaped += "\\$";
            else if (c == '%') escaped += "\\%";
            else if (c == '&') escaped += "\\&";
            else if (c == '_') escaped += "\\_";
            else if (c == '{') escaped += "\\{";
            else if (c == '}') escaped += "\\}";
            else if (c == '^') escaped += "\\textasciicircum{}";
            else if (c == '~') escaped += "\\textasciitilde{}";
            else escaped += c;
        }

        return escaped;
    };

    auto createPlaceholder = [&](const QString &replacement, const QString &prefix) -> QString {
        const QString token = QString("@@%1%2@@").arg(prefix).arg(placeholderCounter++);
        placeholders.append({token, replacement});
        return token;
    };

    auto replaceWithPlaceholders = [&](const QRegularExpression &regex,
                                       const std::function<QString(const QRegularExpressionMatch &)> &replacementBuilder) {
        QString replaced;
        int cursor = 0;
        QRegularExpressionMatchIterator it = regex.globalMatch(result);

        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            replaced += result.mid(cursor, match.capturedStart() - cursor);
            replaced += createPlaceholder(replacementBuilder(match), "FMT");
            cursor = match.capturedEnd();
        }

        replaced += result.mid(cursor);
        result = replaced;
    };

    // 先保护行内代码，避免其中内容被格式化或被数学公式识别。
    {
        QString replaced;
        int cursor = 0;
        const QRegularExpression codeRegex("`([^`\\n]+)`");
        QRegularExpressionMatchIterator it = codeRegex.globalMatch(result);

        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            replaced += result.mid(cursor, match.capturedStart() - cursor);
            const QString codeLatex = QString("\\texttt{%1}").arg(escapeLatex(match.captured(1)));
            replaced += createPlaceholder(codeLatex, "CODE");
            cursor = match.capturedEnd();
        }

        replaced += result.mid(cursor);
        result = replaced;
    }

    // 明确定义数学公式检测：
    // 1) 优先匹配 $$...$$（显示公式）
    // 2) 再匹配 $...$（行内公式）
    // 3) 忽略转义美元符号 \$ 和无效包围（如 $ x $）
    {
        QString replaced;
        int i = 0;
        while (i < result.size()) {
            if (result[i] != '$' || isEscapedAt(result, i)) {
                replaced += result[i];
                ++i;
                continue;
            }

            const bool displayMathStart = (i + 1 < result.size() && result[i + 1] == '$');
            if (displayMathStart) {
                int closePos = -1;
                for (int j = i + 2; j + 1 < result.size(); ++j) {
                    if (result[j] == '$' && result[j + 1] == '$' && !isEscapedAt(result, j)) {
                        closePos = j;
                        break;
                    }
                }

                if (closePos != -1) {
                    const QString content = result.mid(i + 2, closePos - (i + 2));
                    if (!content.trimmed().isEmpty()) {
                        replaced += createPlaceholder(convertMath(content, true), "MATH");
                        i = closePos + 2;
                        continue;
                    }
                }
            } else {
                int closePos = -1;
                for (int j = i + 1; j < result.size(); ++j) {
                    if (result[j] != '$' || isEscapedAt(result, j)) {
                        continue;
                    }

                    const bool isDisplayDelimiter = (j + 1 < result.size() && result[j + 1] == '$') ||
                                                    (j > 0 && result[j - 1] == '$');
                    if (!isDisplayDelimiter) {
                        closePos = j;
                        break;
                    }
                }

                if (closePos != -1) {
                    const QString content = result.mid(i + 1, closePos - (i + 1));
                    if (!content.isEmpty() && content.trimmed() == content) {
                        replaced += createPlaceholder(convertMath(content, false), "MATH");
                        i = closePos + 1;
                        continue;
                    }
                }
            }

            replaced += result[i];
            ++i;
        }

        result = replaced;
    }

    // 粗体 **text** 或 __text__
    replaceWithPlaceholders(QRegularExpression("\\*\\*([^*\\n]+)\\*\\*"),
                            [&](const QRegularExpressionMatch &match) {
                                return QString("\\textbf{%1}").arg(escapePlainText(match.captured(1)));
                            });
    replaceWithPlaceholders(QRegularExpression("__([^_\\n]+)__"),
                            [&](const QRegularExpressionMatch &match) {
                                return QString("\\textbf{%1}").arg(escapePlainText(match.captured(1)));
                            });

    // 斜体 *text* 或 _text_
    replaceWithPlaceholders(QRegularExpression("\\*([^*\\n]+)\\*"),
                            [&](const QRegularExpressionMatch &match) {
                                return QString("\\textit{%1}").arg(escapePlainText(match.captured(1)));
                            });
    replaceWithPlaceholders(QRegularExpression("_([^_\\n]+)_"),
                            [&](const QRegularExpressionMatch &match) {
                                return QString("\\textit{%1}").arg(escapePlainText(match.captured(1)));
                            });

    // 删除线 ~~text~~
    replaceWithPlaceholders(QRegularExpression("~~([^~\\n]+)~~"),
                            [&](const QRegularExpressionMatch &match) {
                                return QString("\\sout{%1}").arg(escapePlainText(match.captured(1)));
                            });

    // 行内代码 `code`
    // 链接 [text](url)
    replaceWithPlaceholders(QRegularExpression("\\[([^\\]]+)\\]\\(([^)]+)\\)"),
                            [&](const QRegularExpressionMatch &match) {
                                return QString("\\href{%1}{%2}")
                                    .arg(escapePlainText(match.captured(2)),
                                         escapePlainText(match.captured(1)));
                            });

    // 引用 [@key]
    replaceWithPlaceholders(QRegularExpression("\\[@([^\\]]+)\\]"),
                            [](const QRegularExpressionMatch &match) {
                                return QString("\\cite{%1}").arg(match.captured(1));
                            });

    // 脚注 [^note]
    replaceWithPlaceholders(QRegularExpression("\\[\\^([^\\]]+)\\]"),
                            [&](const QRegularExpressionMatch &match) {
                                return QString("\\footnote{%1}").arg(escapePlainText(match.captured(1)));
                            });

    // 统一转义普通文本，再恢复占位符对应的 LaTeX 片段。
    result = escapePlainText(result);
    for (int idx = placeholders.size() - 1; idx >= 0; --idx) {
        result.replace(placeholders[idx].first, placeholders[idx].second);
    }

    return result;
}

// 函数说明：实现 LaTeXExporter::convertCodeBlock 的核心逻辑，供当前模块调用。
QString LaTeXExporter::convertCodeBlock(const QString &code, const QString &language)
{
    QString result;
    const CodeHighlight highlight = effectiveCodeHighlight();

    if (highlight == CodeHighlight::Listings) {
        QString lang = languageToListings(language);
        if (!lang.isEmpty()) {
            result += QString("\\begin{lstlisting}[language=%1]\n").arg(lang);
        } else {
            result += "\\begin{lstlisting}\n";
        }
        result += code;
        result += "\\end{lstlisting}\n\n";
    } else if (highlight == CodeHighlight::Minted) {
        QString lang = language.isEmpty() ? "text" : language;
        result += QString("\\begin{minted}{%1}\n").arg(lang);
        result += code;
        result += "\\end{minted}\n\n";
    } else {
        result += "\\begin{verbatim}\n";
        result += code;
        result += "\\end{verbatim}\n\n";
    }

    return result;
}

// 函数说明：实现 LaTeXExporter::convertBlockquote 的核心逻辑，供当前模块调用。
QString LaTeXExporter::convertBlockquote(const QString &text)
{
    return QString("\\begin{quote}\n%1\n\\end{quote}\n\n")
        .arg(processInlineFormatting(text));
}

// 函数说明：实现 LaTeXExporter::convertUnorderedList 的核心逻辑，供当前模块调用。
QString LaTeXExporter::convertUnorderedList(const QStringList &items)
{
    QString result = "\\begin{itemize}\n";
    for (const QString &item : items) {
        result += QString("    \\item %1\n").arg(processInlineFormatting(item));
    }
    result += "\\end{itemize}\n\n";
    return result;
}

// 函数说明：实现 LaTeXExporter::convertOrderedList 的核心逻辑，供当前模块调用。
QString LaTeXExporter::convertOrderedList(const QStringList &items)
{
    QString result = "\\begin{enumerate}\n";
    for (const QString &item : items) {
        result += QString("    \\item %1\n").arg(processInlineFormatting(item));
    }
    result += "\\end{enumerate}\n\n";
    return result;
}

// 函数说明：实现 LaTeXExporter::convertTable 的核心逻辑，供当前模块调用。
QString LaTeXExporter::convertTable(const QStringList &headers, const QVector<QStringList> &rows)
{
    m_tableCounter++;

    QString colSpec = QString("|%1|").arg(QString("c|").repeated(headers.size()));

    QString result = "\\begin{table}[H]\n\\centering\n";
    result += QString("\\begin{tabular}{%1}\n").arg(colSpec);
    result += "\\hline\n";

    // 表头
    QStringList escapedHeaders;
    for (const QString &h : headers) {
        escapedHeaders << QString("\\textbf{%1}").arg(escapeLatex(h));
    }
    result += escapedHeaders.join(" & ") + " \\\\\n";
    result += "\\hline\n";

    // 数据行
    for (const QStringList &row : rows) {
        QStringList escapedRow;
        for (const QString &cell : row) {
            escapedRow << escapeLatex(cell);
        }
        result += escapedRow.join(" & ") + " \\\\\n";
    }

    result += "\\hline\n";
    result += "\\end{tabular}\n";
    result += QString("\\caption{表 %1}\n").arg(m_tableCounter);
    result += "\\end{table}\n\n";

    return result;
}

// 函数说明：实现 LaTeXExporter::convertImage 的核心逻辑，供当前模块调用。
QString LaTeXExporter::convertImage(const QString &alt, const QString &path, const QString &caption)
{
    m_figureCounter++;

    QString cap = caption.isEmpty() ? alt : caption;

    QString result = "\\begin{figure}[H]\n\\centering\n";
    result += QString("\\includegraphics[width=0.8\\textwidth]{%1}\n").arg(path);
    if (!cap.isEmpty()) {
        result += QString("\\caption{%1}\n").arg(escapeLatex(cap));
    }
    result += QString("\\label{fig:%1}\n").arg(m_figureCounter);
    result += "\\end{figure}\n\n";

    return result;
}

// 函数说明：实现 LaTeXExporter::escapeLatex 的核心逻辑，供当前模块调用。
QString LaTeXExporter::escapeLatex(const QString &text)
{
    QString result = text;

    // 转义特殊字符
    result.replace("\\", "\\textbackslash{}");
    result.replace("{", "\\{");
    result.replace("}", "\\}");
    result.replace("#", "\\#");
    result.replace("$", "\\$");
    result.replace("%", "\\%");
    result.replace("&", "\\&");
    result.replace("_", "\\_");
    result.replace("^", "\\textasciicircum{}");
    result.replace("~", "\\textasciitilde{}");

    return result;
}

// 函数说明：实现 LaTeXExporter::languageToListings 的核心逻辑，供当前模块调用。
QString LaTeXExporter::languageToListings(const QString &lang)
{
    QMap<QString, QString> langMap = {
        {"c", "C"},
        {"cpp", "C++"},
        {"c++", "C++"},
        {"java", "Java"},
        {"python", "Python"},
        {"py", "Python"},
        {"javascript", "JavaScript"},
        {"js", "JavaScript"},
        {"html", "HTML"},
        {"css", "CSS"},
        {"sql", "SQL"},
        {"bash", "bash"},
        {"sh", "bash"},
        {"shell", "bash"},
        {"xml", "XML"},
        {"json", "JSON"},
        {"ruby", "Ruby"},
        {"php", "PHP"},
        {"perl", "Perl"},
        {"r", "R"},
        {"matlab", "Matlab"},
        {"latex", "[LaTeX]TeX"},
        {"tex", "TeX"}
    };

    return langMap.value(lang.toLower(), "");
}

// 函数说明：实现 LaTeXExporter::convertMath 的核心逻辑，供当前模块调用。
QString LaTeXExporter::convertMath(const QString &math, bool display)
{
    if (display) {
        return QString("$$%1$$").arg(math);
    }
    return QString("$%1$").arg(math);
}

// 函数说明：实现 LaTeXExporter::effectiveCodeHighlight 的核心逻辑，供当前模块调用。
LaTeXExporter::CodeHighlight LaTeXExporter::effectiveCodeHighlight() const
{
    if (m_options.codeHighlight == CodeHighlight::Auto) {
        if (qEnvironmentVariableIntValue("CUTEMARKED_LATEX_MINTED") == 1) {
            return CodeHighlight::Minted;
        }
        return CodeHighlight::Listings;
    }
    return m_options.codeHighlight;
}

