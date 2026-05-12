// 文件说明：app-static\academic\citationmanager.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "citationmanager.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>

// 函数说明：构造 CitationManager 对象，初始化本模块需要的状态、界面和资源。
CitationManager::CitationManager(QObject *parent)
    : QObject(parent)
    , m_citationStyle(CitationStyle::APA)
{
}

// 函数说明：销毁 CitationManager 对象，释放本模块持有的资源。
CitationManager::~CitationManager()
{
}

// 函数说明：加载 CitationManager 需要的数据、配置或外部资源。
bool CitationManager::loadLibrary(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = tr("无法打开文件: %1").arg(filePath);
        emit errorOccurred(m_lastError);
        return false;
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    m_library.entries = parseBibTeX(content);
    m_library.filePath = filePath;
    m_library.lastModified = QDateTime::currentDateTime();

    // 从文件名提取库名
    QFileInfo fileInfo(filePath);
    m_library.name = fileInfo.baseName();

    emit libraryLoaded(filePath);
    return true;
}

// 函数说明：保存 CitationManager 当前状态，保证用户修改可以持久化。
bool CitationManager::saveLibrary(const QString &filePath)
{
    QString savePath = filePath.isEmpty() ? m_library.filePath : filePath;

    if (savePath.isEmpty()) {
        m_lastError = tr("未指定保存路径");
        emit errorOccurred(m_lastError);
        return false;
    }

    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = tr("无法写入文件: %1").arg(savePath);
        emit errorOccurred(m_lastError);
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << generateBibTeX(m_library.entries);
    file.close();

    m_library.filePath = savePath;
    m_library.lastModified = QDateTime::currentDateTime();

    emit librarySaved();
    return true;
}

// 函数说明：创建 CitationManager 需要的对象、记录或输出内容。
bool CitationManager::createLibrary(const QString &filePath, const QString &name)
{
    m_library = Library();
    m_library.filePath = filePath;
    m_library.name = name;
    m_library.lastModified = QDateTime::currentDateTime();

    return saveLibrary();
}

// 函数说明：关闭 CitationManager 相关窗口或资源，并处理必要的保存确认。
void CitationManager::closeLibrary()
{
    m_library = Library();
    m_citedKeys.clear();
}

// 函数说明：向 CitationManager 管理的数据集合中添加一项内容。
bool CitationManager::addEntry(const BibEntry &entry)
{
    // 检查键是否已存在
    for (const auto &e : m_library.entries) {
        if (e.key == entry.key) {
            m_lastError = tr("引用键 '%1' 已存在").arg(entry.key);
            emit errorOccurred(m_lastError);
            return false;
        }
    }

    BibEntry newEntry = entry;
    if (newEntry.key.isEmpty()) {
        newEntry.key = generateUniqueKey(newEntry);
    }
    newEntry.addedDate = QDateTime::currentDateTime();
    newEntry.modifiedDate = QDateTime::currentDateTime();

    m_library.entries.append(newEntry);
    emit entryAdded(newEntry.key);
    emit libraryModified();
    return true;
}

// 函数说明：刷新 CitationManager 的内部状态，并同步到相关界面。
bool CitationManager::updateEntry(const QString &key, const BibEntry &entry)
{
    for (int i = 0; i < m_library.entries.size(); ++i) {
        if (m_library.entries[i].key == key) {
            BibEntry updated = entry;
            updated.modifiedDate = QDateTime::currentDateTime();
            m_library.entries[i] = updated;
            emit entryUpdated(key);
            emit libraryModified();
            return true;
        }
    }

    m_lastError = tr("未找到引用键: %1").arg(key);
    emit errorOccurred(m_lastError);
    return false;
}

// 函数说明：从 CitationManager 管理的数据集合中移除指定内容。
bool CitationManager::removeEntry(const QString &key)
{
    for (int i = 0; i < m_library.entries.size(); ++i) {
        if (m_library.entries[i].key == key) {
            m_library.entries.removeAt(i);
            emit entryRemoved(key);
            emit libraryModified();
            return true;
        }
    }

    m_lastError = tr("未找到引用键: %1").arg(key);
    return false;
}

// 函数说明：实现 CitationManager::findEntry 的核心逻辑，供当前模块调用。
CitationManager::BibEntry CitationManager::findEntry(const QString &key) const
{
    for (const auto &entry : m_library.entries) {
        if (entry.key == key) {
            return entry;
        }
    }
    return BibEntry();
}

// 函数说明：实现 CitationManager::searchEntries 的核心逻辑，供当前模块调用。
QVector<CitationManager::BibEntry> CitationManager::searchEntries(const QString &query) const
{
    QVector<BibEntry> results;
    QString lowerQuery = query.toLower();

    for (const auto &entry : m_library.entries) {
        if (entry.title.toLower().contains(lowerQuery) ||
            entry.key.toLower().contains(lowerQuery) ||
            entry.abstract.toLower().contains(lowerQuery) ||
            entry.keywords.toLower().contains(lowerQuery)) {
            results.append(entry);
            continue;
        }

        // 搜索作者
        for (const QString &author : entry.authors) {
            if (author.toLower().contains(lowerQuery)) {
                results.append(entry);
                break;
            }
        }
    }

    return results;
}

// 函数说明：实现 CitationManager::filterByType 的核心逻辑，供当前模块调用。
QVector<CitationManager::BibEntry> CitationManager::filterByType(EntryType type) const
{
    QVector<BibEntry> results;
    for (const auto &entry : m_library.entries) {
        if (entry.type == type) {
            results.append(entry);
        }
    }
    return results;
}

// 函数说明：实现 CitationManager::filterByTag 的核心逻辑，供当前模块调用。
QVector<CitationManager::BibEntry> CitationManager::filterByTag(const QString &tag) const
{
    QVector<BibEntry> results;
    for (const auto &entry : m_library.entries) {
        if (entry.tags.contains(tag, Qt::CaseInsensitive)) {
            results.append(entry);
        }
    }
    return results;
}

// 函数说明：实现 CitationManager::filterByYear 的核心逻辑，供当前模块调用。
QVector<CitationManager::BibEntry> CitationManager::filterByYear(const QString &year) const
{
    QVector<BibEntry> results;
    for (const auto &entry : m_library.entries) {
        if (entry.year == year) {
            results.append(entry);
        }
    }
    return results;
}

// 函数说明：实现 CitationManager::filterByAuthor 的核心逻辑，供当前模块调用。
QVector<CitationManager::BibEntry> CitationManager::filterByAuthor(const QString &author) const
{
    QVector<BibEntry> results;
    QString lowerAuthor = author.toLower();

    for (const auto &entry : m_library.entries) {
        for (const QString &a : entry.authors) {
            if (a.toLower().contains(lowerAuthor)) {
                results.append(entry);
                break;
            }
        }
    }
    return results;
}

// 函数说明：解析输入内容，转换为 CitationManager 后续处理使用的数据结构。
QVector<CitationManager::BibEntry> CitationManager::parseBibTeX(const QString &content)
{
    QVector<BibEntry> entries;

    // 匹配 BibTeX 条目: @type{key, ... }
    QRegularExpression entryRegex(
        R"(@(\w+)\s*\{\s*([^,\s]+)\s*,([^@]*)\})",
        QRegularExpression::DotMatchesEverythingOption);

    QRegularExpressionMatchIterator it = entryRegex.globalMatch(content);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();

        BibEntry entry;
        entry.type = parseEntryType(match.captured(1));
        entry.key = match.captured(2).trimmed();

        QString fieldsStr = match.captured(3);

        // 解析字段
        QRegularExpression fieldRegex(
            "(\\w+)\\s*=\\s*(?:\\{([^{}]*(?:\\{[^{}]*\\}[^{}]*)*)\\}|\"([^\"]*)\"|([0-9]+))",
            QRegularExpression::DotMatchesEverythingOption);

        QRegularExpressionMatchIterator fieldIt = fieldRegex.globalMatch(fieldsStr);

        while (fieldIt.hasNext()) {
            QRegularExpressionMatch fieldMatch = fieldIt.next();
            QString fieldName = fieldMatch.captured(1).toLower();
            QString fieldValue = fieldMatch.captured(2);
            if (fieldValue.isEmpty()) fieldValue = fieldMatch.captured(3);
            if (fieldValue.isEmpty()) fieldValue = fieldMatch.captured(4);

            // 清理值
            fieldValue = fieldValue.trimmed();
            fieldValue.replace(QRegularExpression("\\s+"), " ");

            if (fieldName == "title") {
                entry.title = fieldValue;
            } else if (fieldName == "author") {
                entry.authors = parseAuthors(fieldValue);
            } else if (fieldName == "year") {
                entry.year = fieldValue;
            } else if (fieldName == "month") {
                entry.month = fieldValue;
            } else if (fieldName == "journal") {
                entry.journal = fieldValue;
            } else if (fieldName == "booktitle") {
                entry.booktitle = fieldValue;
            } else if (fieldName == "publisher") {
                entry.publisher = fieldValue;
            } else if (fieldName == "address") {
                entry.address = fieldValue;
            } else if (fieldName == "volume") {
                entry.volume = fieldValue;
            } else if (fieldName == "number") {
                entry.number = fieldValue;
            } else if (fieldName == "pages") {
                entry.pages = fieldValue;
            } else if (fieldName == "edition") {
                entry.edition = fieldValue;
            } else if (fieldName == "editor") {
                entry.editor = fieldValue;
            } else if (fieldName == "chapter") {
                entry.chapter = fieldValue;
            } else if (fieldName == "series") {
                entry.series = fieldValue;
            } else if (fieldName == "school") {
                entry.school = fieldValue;
            } else if (fieldName == "institution") {
                entry.institution = fieldValue;
            } else if (fieldName == "organization") {
                entry.organization = fieldValue;
            } else if (fieldName == "howpublished") {
                entry.howpublished = fieldValue;
            } else if (fieldName == "note") {
                entry.note = fieldValue;
            } else if (fieldName == "doi") {
                entry.doi = fieldValue;
            } else if (fieldName == "url") {
                entry.url = fieldValue;
            } else if (fieldName == "urldate") {
                entry.urldate = fieldValue;
            } else if (fieldName == "isbn") {
                entry.isbn = fieldValue;
            } else if (fieldName == "issn") {
                entry.issn = fieldValue;
            } else if (fieldName == "abstract") {
                entry.abstract = fieldValue;
            } else if (fieldName == "keywords") {
                entry.keywords = fieldValue;
            } else if (fieldName == "language") {
                entry.language = fieldValue;
            } else {
                entry.customFields[fieldName] = fieldValue;
            }
        }

        entries.append(entry);
    }

    return entries;
}

// 函数说明：根据当前数据生成 CitationManager 需要的输出结果。
QString CitationManager::generateBibTeX(const QVector<BibEntry> &entries)
{
    QString output;
    for (const auto &entry : entries) {
        output += generateBibTeX(entry);
        output += "\n";
    }
    return output;
}

// 函数说明：根据当前数据生成 CitationManager 需要的输出结果。
QString CitationManager::generateBibTeX(const BibEntry &entry)
{
    QString output;
    output += QString("@%1{%2,\n").arg(entryTypeToString(entry.type), entry.key);

    auto addField = [&output](const QString &name, const QString &value) {
        if (!value.isEmpty()) {
            output += QString("  %1 = {%2},\n").arg(name, value);
        }
    };

    addField("title", entry.title);

    if (!entry.authors.isEmpty()) {
        output += QString("  author = {%1},\n").arg(entry.authors.join(" and "));
    }

    addField("year", entry.year);
    addField("month", entry.month);
    addField("journal", entry.journal);
    addField("booktitle", entry.booktitle);
    addField("publisher", entry.publisher);
    addField("address", entry.address);
    addField("volume", entry.volume);
    addField("number", entry.number);
    addField("pages", entry.pages);
    addField("edition", entry.edition);
    addField("editor", entry.editor);
    addField("chapter", entry.chapter);
    addField("series", entry.series);
    addField("school", entry.school);
    addField("institution", entry.institution);
    addField("organization", entry.organization);
    addField("howpublished", entry.howpublished);
    addField("note", entry.note);
    addField("doi", entry.doi);
    addField("url", entry.url);
    addField("urldate", entry.urldate);
    addField("isbn", entry.isbn);
    addField("issn", entry.issn);
    addField("abstract", entry.abstract);
    addField("keywords", entry.keywords);
    addField("language", entry.language);

    // 自定义字段
    for (auto it = entry.customFields.begin(); it != entry.customFields.end(); ++it) {
        addField(it.key(), it.value());
    }

    // 移除最后的逗号
    if (output.endsWith(",\n")) {
        output.chop(2);
        output += "\n";
    }

    output += "}\n";
    return output;
}

// 函数说明：设置 CitationManager 的运行参数，并触发必要的界面或数据刷新。
void CitationManager::setCitationStyle(CitationStyle style)
{
    m_citationStyle = style;
}

// 函数说明：实现 CitationManager::formatCitation 的核心逻辑，供当前模块调用。
QString CitationManager::formatCitation(const QString &key) const
{
    BibEntry entry = findEntry(key);
    if (entry.key.isEmpty()) {
        return QString("[%1: 未找到]").arg(key);
    }
    return formatCitation(entry);
}

// 函数说明：实现 CitationManager::formatCitation 的核心逻辑，供当前模块调用。
QString CitationManager::formatCitation(const BibEntry &entry) const
{
    switch (m_citationStyle) {
        case CitationStyle::APA:
            return formatAPA(entry);
        case CitationStyle::MLA:
            return formatMLA(entry);
        case CitationStyle::Chicago:
            return formatChicago(entry);
        case CitationStyle::IEEE:
            return formatIEEE(entry);
        case CitationStyle::Harvard:
            return formatHarvard(entry);
        case CitationStyle::Vancouver:
            return formatVancouver(entry);
        case CitationStyle::GB7714:
            return formatGB7714(entry);
        default:
            return formatAPA(entry);
    }
}

// 函数说明：实现 CitationManager::formatInTextCitation 的核心逻辑，供当前模块调用。
QString CitationManager::formatInTextCitation(const QString &key) const
{
    BibEntry entry = findEntry(key);
    if (entry.key.isEmpty()) {
        return QString("(%1)").arg(key);
    }

    QString author;
    if (!entry.authors.isEmpty()) {
        // 提取第一作者的姓
        QString firstAuthor = entry.authors.first();
        int commaPos = firstAuthor.indexOf(',');
        if (commaPos > 0) {
            author = firstAuthor.left(commaPos);
        } else {
            QStringList parts = firstAuthor.split(' ');
            author = parts.last();
        }

        if (entry.authors.size() > 2) {
            author += " et al.";
        } else if (entry.authors.size() == 2) {
            QString secondAuthor = entry.authors[1];
            int comma2 = secondAuthor.indexOf(',');
            QString author2;
            if (comma2 > 0) {
                author2 = secondAuthor.left(comma2);
            } else {
                QStringList parts = secondAuthor.split(' ');
                author2 = parts.last();
            }
            author += " & " + author2;
        }
    }

    switch (m_citationStyle) {
        case CitationStyle::APA:
        case CitationStyle::Harvard:
            return QString("(%1, %2)").arg(author, entry.year);
        case CitationStyle::MLA:
            return QString("(%1 %2)").arg(author, entry.pages.isEmpty() ? "" : entry.pages);
        case CitationStyle::Chicago:
            return QString("(%1 %2)").arg(author, entry.year);
        case CitationStyle::IEEE:
            return QString("[%1]").arg(key);
        case CitationStyle::GB7714:
            return QString("[%1]").arg(key);
        default:
            return QString("(%1, %2)").arg(author, entry.year);
    }
}

// 函数说明：实现 CitationManager::formatInTextCitation 的核心逻辑，供当前模块调用。
QString CitationManager::formatInTextCitation(const QStringList &keys) const
{
    if (keys.isEmpty()) return QString();
    if (keys.size() == 1) return formatInTextCitation(keys.first());

    QStringList citations;
    for (const QString &key : keys) {
        BibEntry entry = findEntry(key);
        if (!entry.key.isEmpty()) {
            QString author;
            if (!entry.authors.isEmpty()) {
                QString firstAuthor = entry.authors.first();
                int commaPos = firstAuthor.indexOf(',');
                if (commaPos > 0) {
                    author = firstAuthor.left(commaPos);
                } else {
                    QStringList parts = firstAuthor.split(' ');
                    author = parts.last();
                }
                if (entry.authors.size() > 1) {
                    author += " et al.";
                }
            }
            citations << QString("%1, %2").arg(author, entry.year);
        }
    }

    return QString("(%1)").arg(citations.join("; "));
}

// 函数说明：实现 CitationManager::formatBibliography 的核心逻辑，供当前模块调用。
QString CitationManager::formatBibliography(const QStringList &keys) const
{
    QString output;
    int num = 1;

    for (const QString &key : keys) {
        BibEntry entry = findEntry(key);
        if (!entry.key.isEmpty()) {
            if (m_citationStyle == CitationStyle::IEEE ||
                m_citationStyle == CitationStyle::Vancouver) {
                output += QString("[%1] %2\n\n").arg(num++).arg(formatCitation(entry));
            } else {
                output += formatCitation(entry) + "\n\n";
            }
        }
    }

    return output;
}

// 函数说明：实现 CitationManager::formatBibliography 的核心逻辑，供当前模块调用。
QString CitationManager::formatBibliography() const
{
    return formatBibliography(m_citedKeys);
}

// 函数说明：实现 CitationManager::insertCitation 的核心逻辑，供当前模块调用。
QString CitationManager::insertCitation(const QString &key)
{
    if (!m_citedKeys.contains(key)) {
        m_citedKeys.append(key);
    }
    return QString("[@%1]").arg(key);
}

// 函数说明：实现 CitationManager::insertCitations 的核心逻辑，供当前模块调用。
QString CitationManager::insertCitations(const QStringList &keys)
{
    QStringList citationKeys;
    for (const QString &key : keys) {
        if (!m_citedKeys.contains(key)) {
            m_citedKeys.append(key);
        }
        citationKeys << QString("@%1").arg(key);
    }
    return QString("[%1]").arg(citationKeys.join("; "));
}

// 函数说明：根据当前数据生成 CitationManager 需要的输出结果。
QString CitationManager::generateMarkdownBibliography(const QStringList &keys)
{
    QString output = "## 参考文献\n\n";

    int num = 1;
    for (const QString &key : keys) {
        BibEntry entry = findEntry(key);
        if (!entry.key.isEmpty()) {
            QString citation = formatCitation(entry);

            // 添加 DOI 链接
            if (!entry.doi.isEmpty()) {
                citation += QString(" [DOI](https://doi.org/%1)").arg(entry.doi);
            } else if (!entry.url.isEmpty()) {
                citation += QString(" [链接](%1)").arg(entry.url);
            }

            if (m_citationStyle == CitationStyle::IEEE ||
                m_citationStyle == CitationStyle::Vancouver ||
                m_citationStyle == CitationStyle::GB7714) {
                output += QString("[%1] %2\n\n").arg(num++).arg(citation);
            } else {
                output += QString("- %1\n\n").arg(citation);
            }
        }
    }

    return output;
}

// 函数说明：实现 CitationManager::importFromDOI 的核心逻辑，供当前模块调用。
void CitationManager::importFromDOI(const QString &doi)
{
    // 使用 CrossRef API 获取元数据
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    QString cleanDoi = doi;
    cleanDoi.remove(QRegularExpression("^https?://doi\\.org/"));

    QUrl url(QString("https://api.crossref.org/works/%1").arg(cleanDoi));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "CuteMarkEd/1.0");

    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, manager]() {
        reply->deleteLater();
        manager->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            m_lastError = tr("DOI 查询失败: %1").arg(reply->errorString());
            emit importFailed(m_lastError);
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();
        QJsonObject message = obj["message"].toObject();

        BibEntry entry;
        entry.type = EntryType::Article;

        // 标题
        QJsonArray titleArray = message["title"].toArray();
        if (!titleArray.isEmpty()) {
            entry.title = titleArray.first().toString();
        }

        // 作者
        QJsonArray authorsArray = message["author"].toArray();
        for (const auto &authorVal : authorsArray) {
            QJsonObject author = authorVal.toObject();
            QString name = author["family"].toString();
            if (!author["given"].toString().isEmpty()) {
                name += ", " + author["given"].toString();
            }
            entry.authors.append(name);
        }

        // 年份
        QJsonObject published = message["published-print"].toObject();
        if (published.isEmpty()) {
            published = message["published-online"].toObject();
        }
        QJsonArray dateParts = published["date-parts"].toArray();
        if (!dateParts.isEmpty()) {
            QJsonArray date = dateParts.first().toArray();
            if (!date.isEmpty()) {
                entry.year = QString::number(date.first().toInt());
            }
        }

        // 期刊
        QJsonArray containerArray = message["container-title"].toArray();
        if (!containerArray.isEmpty()) {
            entry.journal = containerArray.first().toString();
        }

        // 卷期页
        entry.volume = message["volume"].toString();
        entry.number = message["issue"].toString();
        entry.pages = message["page"].toString();
        entry.doi = message["DOI"].toString();

        // ISSN
        QJsonArray issnArray = message["ISSN"].toArray();
        if (!issnArray.isEmpty()) {
            entry.issn = issnArray.first().toString();
        }

        // 生成键
        entry.key = generateUniqueKey(entry);

        addEntry(entry);
        emit importCompleted(entry);
    });
}

// 函数说明：实现 CitationManager::importFromISBN 的核心逻辑，供当前模块调用。
void CitationManager::importFromISBN(const QString &isbn)
{
    // 使用 Open Library API
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    QString cleanIsbn = isbn;
    cleanIsbn.remove('-');

    QUrl url(QString("https://openlibrary.org/api/books?bibkeys=ISBN:%1&format=json&jscmd=data").arg(cleanIsbn));
    QNetworkRequest request(url);

    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, manager, cleanIsbn]() {
        reply->deleteLater();
        manager->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            m_lastError = tr("ISBN 查询失败: %1").arg(reply->errorString());
            emit importFailed(m_lastError);
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();

        QString key = QString("ISBN:%1").arg(cleanIsbn);
        if (!obj.contains(key)) {
            m_lastError = tr("未找到 ISBN: %1").arg(cleanIsbn);
            emit importFailed(m_lastError);
            return;
        }

        QJsonObject book = obj[key].toObject();
        BibEntry entry;
        entry.type = EntryType::Book;
        entry.title = book["title"].toString();
        entry.isbn = cleanIsbn;

        // 作者
        QJsonArray authorsArray = book["authors"].toArray();
        for (const auto &authorVal : authorsArray) {
            entry.authors.append(authorVal.toObject()["name"].toString());
        }

        // 出版社
        QJsonArray publishersArray = book["publishers"].toArray();
        if (!publishersArray.isEmpty()) {
            entry.publisher = publishersArray.first().toObject()["name"].toString();
        }

        // 年份
        entry.year = book["publish_date"].toString();
        if (entry.year.length() > 4) {
            // 提取年份
            QRegularExpression yearRegex("(\\d{4})");
            QRegularExpressionMatch match = yearRegex.match(entry.year);
            if (match.hasMatch()) {
                entry.year = match.captured(1);
            }
        }

        entry.key = generateUniqueKey(entry);

        addEntry(entry);
        emit importCompleted(entry);
    });
}

// 函数说明：实现 CitationManager::importFromArXiv 的核心逻辑，供当前模块调用。
void CitationManager::importFromArXiv(const QString &arxivId)
{
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    QString cleanId = arxivId;
    cleanId.remove(QRegularExpression("^arXiv:"));

    QUrl url(QString("https://export.arxiv.org/api/query?id_list=%1").arg(cleanId));
    QNetworkRequest request(url);

    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, manager, cleanId]() {
        reply->deleteLater();
        manager->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            m_lastError = tr("arXiv 查询失败: %1").arg(reply->errorString());
            emit importFailed(m_lastError);
            return;
        }

        QString xml = QString::fromUtf8(reply->readAll());

        BibEntry entry;
        entry.type = EntryType::Article;

        // 简单 XML 解析（实际应使用 QXmlStreamReader）
        QRegularExpression titleRegex("<title>([^<]+)</title>");
        QRegularExpressionMatch titleMatch = titleRegex.match(xml);
        if (titleMatch.hasMatch()) {
            entry.title = titleMatch.captured(1).trimmed();
        }

        QRegularExpression authorRegex("<name>([^<]+)</name>");
        QRegularExpressionMatchIterator authorIt = authorRegex.globalMatch(xml);
        while (authorIt.hasNext()) {
            QRegularExpressionMatch authorMatch = authorIt.next();
            entry.authors.append(authorMatch.captured(1));
        }

        QRegularExpression publishedRegex("<published>([^<]+)</published>");
        QRegularExpressionMatch publishedMatch = publishedRegex.match(xml);
        if (publishedMatch.hasMatch()) {
            entry.year = publishedMatch.captured(1).left(4);
        }

        entry.url = QString("https://arxiv.org/abs/%1").arg(cleanId);
        entry.note = QString("arXiv:%1").arg(cleanId);
        entry.key = generateUniqueKey(entry);

        addEntry(entry);
        emit importCompleted(entry);
    });
}

// 函数说明：实现 CitationManager::importFromPubMed 的核心逻辑，供当前模块调用。
void CitationManager::importFromPubMed(const QString &pmid)
{
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    QUrl url(QString("https://eutils.ncbi.nlm.nih.gov/entrez/eutils/efetch.fcgi?db=pubmed&id=%1&retmode=xml").arg(pmid));
    QNetworkRequest request(url);

    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, manager]() {
        reply->deleteLater();
        manager->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            m_lastError = tr("PubMed 查询失败: %1").arg(reply->errorString());
            emit importFailed(m_lastError);
            return;
        }

        QString xml = QString::fromUtf8(reply->readAll());

        BibEntry entry;
        entry.type = EntryType::Article;

        // 简单 XML 解析
        QRegularExpression titleRegex("<ArticleTitle>([^<]+)</ArticleTitle>");
        QRegularExpressionMatch titleMatch = titleRegex.match(xml);
        if (titleMatch.hasMatch()) {
            entry.title = titleMatch.captured(1);
        }

        QRegularExpression journalRegex("<Title>([^<]+)</Title>");
        QRegularExpressionMatch journalMatch = journalRegex.match(xml);
        if (journalMatch.hasMatch()) {
            entry.journal = journalMatch.captured(1);
        }

        QRegularExpression yearRegex("<Year>(\\d{4})</Year>");
        QRegularExpressionMatch yearMatch = yearRegex.match(xml);
        if (yearMatch.hasMatch()) {
            entry.year = yearMatch.captured(1);
        }

        QRegularExpression authorRegex("<LastName>([^<]+)</LastName>\\s*<ForeName>([^<]+)</ForeName>");
        QRegularExpressionMatchIterator authorIt = authorRegex.globalMatch(xml);
        while (authorIt.hasNext()) {
            QRegularExpressionMatch authorMatch = authorIt.next();
            entry.authors.append(QString("%1, %2").arg(
                authorMatch.captured(1), authorMatch.captured(2)));
        }

        entry.key = generateUniqueKey(entry);

        addEntry(entry);
        emit importCompleted(entry);
    });
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
bool CitationManager::exportToRIS(const QString &filePath, const QVector<BibEntry> &entries)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = tr("无法写入文件: %1").arg(filePath);
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    for (const auto &entry : entries) {
        // 类型映射
        QString type;
        switch (entry.type) {
            case EntryType::Article: type = "JOUR"; break;
            case EntryType::Book: type = "BOOK"; break;
            case EntryType::InProceedings:
            case EntryType::Conference: type = "CONF"; break;
            case EntryType::PhdThesis:
            case EntryType::MastersThesis: type = "THES"; break;
            case EntryType::TechReport: type = "RPRT"; break;
            default: type = "GEN"; break;
        }

        out << "TY  - " << type << "\n";
        out << "TI  - " << entry.title << "\n";

        for (const QString &author : entry.authors) {
            out << "AU  - " << author << "\n";
        }

        out << "PY  - " << entry.year << "\n";

        if (!entry.journal.isEmpty()) {
            out << "JO  - " << entry.journal << "\n";
        }
        if (!entry.volume.isEmpty()) {
            out << "VL  - " << entry.volume << "\n";
        }
        if (!entry.number.isEmpty()) {
            out << "IS  - " << entry.number << "\n";
        }
        if (!entry.pages.isEmpty()) {
            out << "SP  - " << entry.pages << "\n";
        }
        if (!entry.doi.isEmpty()) {
            out << "DO  - " << entry.doi << "\n";
        }
        if (!entry.url.isEmpty()) {
            out << "UR  - " << entry.url << "\n";
        }
        if (!entry.abstract.isEmpty()) {
            out << "AB  - " << entry.abstract << "\n";
        }

        out << "ER  - \n\n";
    }

    file.close();
    return true;
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
bool CitationManager::exportToEndNote(const QString &filePath, const QVector<BibEntry> &entries)
{
    // EndNote XML 格式
    QString risPath = filePath;
    risPath.replace(".enw", ".ris");
    return exportToRIS(risPath, entries);
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
bool CitationManager::exportToCSV(const QString &filePath, const QVector<BibEntry> &entries)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = tr("无法写入文件: %1").arg(filePath);
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // 表头
    out << "Key,Type,Title,Authors,Year,Journal,Volume,Number,Pages,DOI,URL\n";

    for (const auto &entry : entries) {
        auto escapeCSV = [](const QString &str) {
            QString escaped = str;
            escaped.replace("\"", "\"\"");
            if (escaped.contains(',') || escaped.contains('"') || escaped.contains('\n')) {
                escaped = "\"" + escaped + "\"";
            }
            return escaped;
        };

        out << escapeCSV(entry.key) << ","
            << escapeCSV(entryTypeToString(entry.type)) << ","
            << escapeCSV(entry.title) << ","
            << escapeCSV(entry.authors.join("; ")) << ","
            << escapeCSV(entry.year) << ","
            << escapeCSV(entry.journal) << ","
            << escapeCSV(entry.volume) << ","
            << escapeCSV(entry.number) << ","
            << escapeCSV(entry.pages) << ","
            << escapeCSV(entry.doi) << ","
            << escapeCSV(entry.url) << "\n";
    }

    file.close();
    return true;
}

// 函数说明：解析输入内容，转换为 CitationManager 后续处理使用的数据结构。
CitationManager::EntryType CitationManager::parseEntryType(const QString &typeStr)
{
    QString lower = typeStr.toLower();

    if (lower == "article") return EntryType::Article;
    if (lower == "book") return EntryType::Book;
    if (lower == "booklet") return EntryType::Booklet;
    if (lower == "conference") return EntryType::Conference;
    if (lower == "inbook") return EntryType::InBook;
    if (lower == "incollection") return EntryType::InCollection;
    if (lower == "inproceedings") return EntryType::InProceedings;
    if (lower == "manual") return EntryType::Manual;
    if (lower == "mastersthesis") return EntryType::MastersThesis;
    if (lower == "misc") return EntryType::Misc;
    if (lower == "phdthesis") return EntryType::PhdThesis;
    if (lower == "proceedings") return EntryType::Proceedings;
    if (lower == "techreport") return EntryType::TechReport;
    if (lower == "unpublished") return EntryType::Unpublished;
    if (lower == "online") return EntryType::Online;
    if (lower == "patent") return EntryType::Patent;

    return EntryType::Unknown;
}

// 函数说明：实现 CitationManager::entryTypeToString 的核心逻辑，供当前模块调用。
QString CitationManager::entryTypeToString(EntryType type)
{
    switch (type) {
        case EntryType::Article: return "article";
        case EntryType::Book: return "book";
        case EntryType::Booklet: return "booklet";
        case EntryType::Conference: return "conference";
        case EntryType::InBook: return "inbook";
        case EntryType::InCollection: return "incollection";
        case EntryType::InProceedings: return "inproceedings";
        case EntryType::Manual: return "manual";
        case EntryType::MastersThesis: return "mastersthesis";
        case EntryType::Misc: return "misc";
        case EntryType::PhdThesis: return "phdthesis";
        case EntryType::Proceedings: return "proceedings";
        case EntryType::TechReport: return "techreport";
        case EntryType::Unpublished: return "unpublished";
        case EntryType::Online: return "online";
        case EntryType::Patent: return "patent";
        default: return "misc";
    }
}

// 函数说明：解析输入内容，转换为 CitationManager 后续处理使用的数据结构。
QStringList CitationManager::parseAuthors(const QString &authorsStr)
{
    QStringList authors;

    // BibTeX 作者格式: "Last1, First1 and Last2, First2"
    QStringList parts = authorsStr.split(" and ", Qt::SkipEmptyParts);

    for (const QString &part : parts) {
        authors.append(part.trimmed());
    }

    return authors;
}

// 函数说明：实现 CitationManager::formatAuthorsAPA 的核心逻辑，供当前模块调用。
QString CitationManager::formatAuthorsAPA(const QStringList &authors)
{
    if (authors.isEmpty()) return QString();

    QStringList formatted;
    for (int i = 0; i < authors.size() && i < 20; ++i) {
        QString author = authors[i];
        int commaPos = author.indexOf(',');
        if (commaPos > 0) {
            QString lastName = author.left(commaPos).trimmed();
            QString firstName = author.mid(commaPos + 1).trimmed();
            // 取首字母
            QStringList firstNames = firstName.split(' ');
            QString initials;
            for (const QString &name : firstNames) {
                if (!name.isEmpty()) {
                    initials += name[0].toUpper() + QString(". ");
                }
            }
            formatted << lastName + ", " + initials.trimmed();
        } else {
            formatted << author;
        }
    }

    if (authors.size() > 20) {
        formatted << "...";
        formatted << formatAuthorsAPA(QStringList() << authors.last());
    }

    if (formatted.size() == 1) {
        return formatted.first();
    } else if (formatted.size() == 2) {
        return formatted.join(" & ");
    } else {
        QString result = formatted.mid(0, formatted.size() - 1).join(", ");
        result += ", & " + formatted.last();
        return result;
    }
}

// 函数说明：实现 CitationManager::formatAuthorsMLA 的核心逻辑，供当前模块调用。
QString CitationManager::formatAuthorsMLA(const QStringList &authors)
{
    if (authors.isEmpty()) return QString();

    if (authors.size() == 1) {
        return authors.first();
    } else if (authors.size() == 2) {
        return authors[0] + ", and " + authors[1];
    } else {
        return authors[0] + ", et al.";
    }
}

// 函数说明：实现 CitationManager::formatAuthorsChicago 的核心逻辑，供当前模块调用。
QString CitationManager::formatAuthorsChicago(const QStringList &authors)
{
    return formatAuthorsMLA(authors);
}

// 函数说明：实现 CitationManager::formatAuthorsIEEE 的核心逻辑，供当前模块调用。
QString CitationManager::formatAuthorsIEEE(const QStringList &authors)
{
    if (authors.isEmpty()) return QString();

    QStringList formatted;
    for (const QString &author : authors) {
        int commaPos = author.indexOf(',');
        if (commaPos > 0) {
            QString lastName = author.left(commaPos).trimmed();
            QString firstName = author.mid(commaPos + 1).trimmed();
            QStringList firstNames = firstName.split(' ');
            QString initials;
            for (const QString &name : firstNames) {
                if (!name.isEmpty()) {
                    initials += QString(name[0].toUpper()) + QStringLiteral(". ");
                }
            }
            formatted << initials.trimmed() + " " + lastName;
        } else {
            formatted << author;
        }
    }

    if (formatted.size() <= 3) {
        if (formatted.size() == 1) return formatted.first();
        if (formatted.size() == 2) return formatted.join(" and ");
        return formatted[0] + ", " + formatted[1] + ", and " + formatted[2];
    } else {
        return formatted.first() + " et al.";
    }
}

// 函数说明：实现 CitationManager::formatAuthorsGB7714 的核心逻辑，供当前模块调用。
QString CitationManager::formatAuthorsGB7714(const QStringList &authors)
{
    if (authors.isEmpty()) return QString();

    QStringList formatted;
    for (int i = 0; i < authors.size() && i < 3; ++i) {
        formatted << authors[i];
    }

    if (authors.size() > 3) {
        formatted << "等";
    }

    return formatted.join(", ");
}

// 函数说明：实现 CitationManager::formatAPA 的核心逻辑，供当前模块调用。
QString CitationManager::formatAPA(const BibEntry &entry) const
{
    QString citation;

    // 作者 (年份). 标题. 期刊, 卷(期), 页码. DOI
    citation += formatAuthorsAPA(entry.authors);
    citation += QString(" (%1). ").arg(entry.year);
    citation += entry.title + ". ";

    if (!entry.journal.isEmpty()) {
        citation += "*" + entry.journal + "*";
        if (!entry.volume.isEmpty()) {
            citation += ", *" + entry.volume + "*";
            if (!entry.number.isEmpty()) {
                citation += "(" + entry.number + ")";
            }
        }
        if (!entry.pages.isEmpty()) {
            citation += ", " + entry.pages;
        }
        citation += ".";
    } else if (!entry.publisher.isEmpty()) {
        citation += entry.publisher + ".";
    }

    if (!entry.doi.isEmpty()) {
        citation += " https://doi.org/" + entry.doi;
    }

    return citation;
}

// 函数说明：实现 CitationManager::formatMLA 的核心逻辑，供当前模块调用。
QString CitationManager::formatMLA(const BibEntry &entry) const
{
    QString citation;

    // 作者. "标题." 期刊, 卷号, 期号, 年份, 页码.
    citation += formatAuthorsMLA(entry.authors) + ". ";
    citation += "\"" + entry.title + ".\" ";

    if (!entry.journal.isEmpty()) {
        citation += "*" + entry.journal + "*";
        if (!entry.volume.isEmpty()) {
            citation += ", vol. " + entry.volume;
        }
        if (!entry.number.isEmpty()) {
            citation += ", no. " + entry.number;
        }
        citation += ", " + entry.year;
        if (!entry.pages.isEmpty()) {
            citation += ", pp. " + entry.pages;
        }
        citation += ".";
    } else if (!entry.publisher.isEmpty()) {
        citation += entry.publisher + ", " + entry.year + ".";
    }

    return citation;
}

// 函数说明：实现 CitationManager::formatChicago 的核心逻辑，供当前模块调用。
QString CitationManager::formatChicago(const BibEntry &entry) const
{
    QString citation;

    // 作者. "标题." 期刊 卷, no. 期 (年份): 页码.
    citation += formatAuthorsChicago(entry.authors) + ". ";
    citation += "\"" + entry.title + ".\" ";

    if (!entry.journal.isEmpty()) {
        citation += "*" + entry.journal + "* ";
        if (!entry.volume.isEmpty()) {
            citation += entry.volume;
        }
        if (!entry.number.isEmpty()) {
            citation += ", no. " + entry.number;
        }
        citation += " (" + entry.year + ")";
        if (!entry.pages.isEmpty()) {
            citation += ": " + entry.pages;
        }
        citation += ".";
    } else if (!entry.publisher.isEmpty()) {
        if (!entry.address.isEmpty()) {
            citation += entry.address + ": ";
        }
        citation += entry.publisher + ", " + entry.year + ".";
    }

    return citation;
}

// 函数说明：实现 CitationManager::formatIEEE 的核心逻辑，供当前模块调用。
QString CitationManager::formatIEEE(const BibEntry &entry) const
{
    QString citation;

    // 作者, "标题," 期刊, vol. 卷, no. 期, pp. 页码, 年份.
    citation += formatAuthorsIEEE(entry.authors) + ", ";
    citation += "\"" + entry.title + ",\" ";

    if (!entry.journal.isEmpty()) {
        citation += "*" + entry.journal + "*";
        if (!entry.volume.isEmpty()) {
            citation += ", vol. " + entry.volume;
        }
        if (!entry.number.isEmpty()) {
            citation += ", no. " + entry.number;
        }
        if (!entry.pages.isEmpty()) {
            citation += ", pp. " + entry.pages;
        }
        citation += ", " + entry.year + ".";
    } else if (!entry.booktitle.isEmpty()) {
        citation += "in *" + entry.booktitle + "*";
        citation += ", " + entry.year + ".";
    } else if (!entry.publisher.isEmpty()) {
        citation += entry.publisher + ", " + entry.year + ".";
    }

    return citation;
}

// 函数说明：实现 CitationManager::formatHarvard 的核心逻辑，供当前模块调用。
QString CitationManager::formatHarvard(const BibEntry &entry) const
{
    // Harvard 格式与 APA 类似
    return formatAPA(entry);
}

// 函数说明：实现 CitationManager::formatVancouver 的核心逻辑，供当前模块调用。
QString CitationManager::formatVancouver(const BibEntry &entry) const
{
    QString citation;

    // 作者. 标题. 期刊. 年份;卷(期):页码.
    QStringList shortAuthors;
    for (const QString &author : entry.authors) {
        int commaPos = author.indexOf(',');
        if (commaPos > 0) {
            QString lastName = author.left(commaPos).trimmed();
            QString firstName = author.mid(commaPos + 1).trimmed();
            QString initials;
            for (const QChar &c : firstName) {
                if (c.isLetter() && c.isUpper()) {
                    initials += c;
                }
            }
            shortAuthors << lastName + " " + initials;
        } else {
            shortAuthors << author;
        }
    }

    if (shortAuthors.size() > 6) {
        citation += shortAuthors.mid(0, 6).join(", ") + ", et al. ";
    } else {
        citation += shortAuthors.join(", ") + ". ";
    }

    citation += entry.title + ". ";

    if (!entry.journal.isEmpty()) {
        citation += entry.journal + ". " + entry.year;
        if (!entry.volume.isEmpty()) {
            citation += ";" + entry.volume;
            if (!entry.number.isEmpty()) {
                citation += "(" + entry.number + ")";
            }
        }
        if (!entry.pages.isEmpty()) {
            citation += ":" + entry.pages;
        }
        citation += ".";
    }

    return citation;
}

// 函数说明：实现 CitationManager::formatGB7714 的核心逻辑，供当前模块调用。
QString CitationManager::formatGB7714(const BibEntry &entry) const
{
    QString citation;

    // 作者. 标题[文献类型]. 期刊, 年份, 卷(期): 页码.
    citation += formatAuthorsGB7714(entry.authors) + ". ";
    citation += entry.title;

    // 文献类型标识
    QString typeCode;
    switch (entry.type) {
        case EntryType::Article: typeCode = "J"; break;
        case EntryType::Book: typeCode = "M"; break;
        case EntryType::InProceedings:
        case EntryType::Conference: typeCode = "C"; break;
        case EntryType::PhdThesis:
        case EntryType::MastersThesis: typeCode = "D"; break;
        case EntryType::TechReport: typeCode = "R"; break;
        case EntryType::Online: typeCode = "EB/OL"; break;
        case EntryType::Patent: typeCode = "P"; break;
        default: typeCode = "Z"; break;
    }
    citation += "[" + typeCode + "]. ";

    if (!entry.journal.isEmpty()) {
        citation += entry.journal + ", " + entry.year;
        if (!entry.volume.isEmpty()) {
            citation += ", " + entry.volume;
            if (!entry.number.isEmpty()) {
                citation += "(" + entry.number + ")";
            }
        }
        if (!entry.pages.isEmpty()) {
            citation += ": " + entry.pages;
        }
        citation += ".";
    } else if (!entry.publisher.isEmpty()) {
        if (!entry.address.isEmpty()) {
            citation += entry.address + ": ";
        }
        citation += entry.publisher + ", " + entry.year + ".";
    }

    return citation;
}

// 函数说明：根据当前数据生成 CitationManager 需要的输出结果。
QString CitationManager::generateUniqueKey(const BibEntry &entry) const
{
    QString baseKey;

    // 从第一作者的姓生成基础键
    if (!entry.authors.isEmpty()) {
        QString firstAuthor = entry.authors.first();
        int commaPos = firstAuthor.indexOf(',');
        if (commaPos > 0) {
            baseKey = firstAuthor.left(commaPos).toLower();
        } else {
            QStringList parts = firstAuthor.split(' ');
            baseKey = parts.last().toLower();
        }
    } else {
        baseKey = "unknown";
    }

    // 添加年份
    baseKey += entry.year;

    // 检查唯一性
    QString key = baseKey;
    int suffix = 0;
    while (findEntry(key).key == key) {
        suffix++;
        key = baseKey + QChar('a' + suffix - 1);
    }

    return key;
}

