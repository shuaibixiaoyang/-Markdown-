// 文件说明：app-static\search\searchindexmanager.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "searchindexmanager.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDirIterator>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtConcurrent>
#include <algorithm>

const QStringList SearchIndexManager::DefaultExtensions = {
    "*.md", "*.markdown", "*.txt", "*.text", "*.rst", "*.adoc"
};

// 函数说明：构造 SearchIndexManager 对象，初始化本模块需要的状态、界面和资源。
SearchIndexManager::SearchIndexManager(QObject *parent)
    : QObject(parent)
    , m_isIndexing(false)
{
}

// 函数说明：销毁 SearchIndexManager 对象，释放本模块持有的资源。
SearchIndexManager::~SearchIndexManager()
{
}

// 函数说明：向 SearchIndexManager 管理的数据集合中添加一项内容。
void SearchIndexManager::addSearchPath(const QString &path)
{
    QMutexLocker locker(&m_mutex);
    if (!m_searchPaths.contains(path)) {
        m_searchPaths.append(path);
    }
}

// 函数说明：从 SearchIndexManager 管理的数据集合中移除指定内容。
void SearchIndexManager::removeSearchPath(const QString &path)
{
    QMutexLocker locker(&m_mutex);
    m_searchPaths.removeAll(path);
}

// 函数说明：实现 SearchIndexManager::buildIndex 的核心逻辑，供当前模块调用。
void SearchIndexManager::buildIndex()
{
    if (m_isIndexing) return;

    m_isIndexing = true;
    emit indexingStarted();

    QtConcurrent::run([this]() {
        int totalFiles = 0;
        int currentFile = 0;

        // 先计算文件总数
        for (const QString &path : m_searchPaths) {
            QDirIterator it(path, DefaultExtensions, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                it.next();
                totalFiles++;
            }
        }

        // 索引文件
        for (const QString &path : m_searchPaths) {
            QDirIterator it(path, DefaultExtensions, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString filePath = it.next();
                indexFile(filePath);
                currentFile++;

                QMetaObject::invokeMethod(this, [this, currentFile, totalFiles, filePath]() {
                    emit indexingProgress(currentFile, totalFiles, filePath);
                }, Qt::QueuedConnection);
            }
        }

        m_lastIndexTime = QDateTime::currentDateTime();
        m_isIndexing = false;

        QMetaObject::invokeMethod(this, [this]() {
            emit indexingFinished(m_documents.size());
        }, Qt::QueuedConnection);
    });
}

// 函数说明：实现 SearchIndexManager::rebuildIndex 的核心逻辑，供当前模块调用。
void SearchIndexManager::rebuildIndex()
{
    clearIndex();
    buildIndex();
}

// 函数说明：刷新 SearchIndexManager 的内部状态，并同步到相关界面。
void SearchIndexManager::updateIndex(const QString &filePath)
{
    QFileInfo fi(filePath);
    if (!fi.exists()) {
        removeFromIndex(filePath);
        return;
    }

    indexFile(filePath);
    emit indexUpdated(filePath);
}

// 函数说明：从 SearchIndexManager 管理的数据集合中移除指定内容。
void SearchIndexManager::removeFromIndex(const QString &filePath)
{
    QMutexLocker locker(&m_mutex);
    m_documents.remove(filePath);
}

// 函数说明：清空 SearchIndexManager 保存的临时状态或缓存数据。
void SearchIndexManager::clearIndex()
{
    QMutexLocker locker(&m_mutex);
    m_documents.clear();
}

// 函数说明：实现 SearchIndexManager::indexContent 的核心逻辑，供当前模块调用。
void SearchIndexManager::indexContent(const QString &virtualPath, const QString &content, const QString &title)
{
    DocumentIndex doc;
    doc.filePath = virtualPath;
    doc.content = content;
    doc.title = title.isEmpty() ? extractTitle(content) : title;
    if (doc.title.isEmpty()) {
        doc.title = virtualPath;
    }
    doc.words = tokenize(content);
    doc.lastModified = QDateTime::currentDateTime();
    doc.indexedAt = QDateTime::currentDateTime();
    doc.fileSize = content.size();

    QMutexLocker locker(&m_mutex);
    m_documents[virtualPath] = doc;
}

// 函数说明：实现 SearchIndexManager::indexFile 的核心逻辑，供当前模块调用。
void SearchIndexManager::indexFile(const QString &filePath)
{
    DocumentIndex doc = createDocumentIndex(filePath);
    if (doc.isValid()) {
        QMutexLocker locker(&m_mutex);
        m_documents[filePath] = doc;
    }
}

// 函数说明：创建 SearchIndexManager 需要的对象、记录或输出内容。
DocumentIndex SearchIndexManager::createDocumentIndex(const QString &filePath)
{
    DocumentIndex doc;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return doc;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString content = in.readAll();
    file.close();

    QFileInfo fi(filePath);

    doc.filePath = filePath;
    doc.content = content;
    doc.title = extractTitle(content);
    if (doc.title.isEmpty()) {
        doc.title = fi.baseName();
    }
    doc.words = tokenize(content);
    doc.lastModified = fi.lastModified();
    doc.indexedAt = QDateTime::currentDateTime();
    doc.fileSize = fi.size();

    return doc;
}

// 函数说明：实现 SearchIndexManager::tokenize 的核心逻辑，供当前模块调用。
QStringList SearchIndexManager::tokenize(const QString &text)
{
    QStringList words;

    // 使用正则表达式分词
    QRegularExpression wordRegex("\\b[\\w\\u4e00-\\u9fa5]+\\b");
    QRegularExpressionMatchIterator it = wordRegex.globalMatch(text.toLower());

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString word = match.captured();
        if (word.length() >= 2) { // 过滤太短的词
            words.append(word);
        }
    }

    return words;
}

// 函数说明：实现 SearchIndexManager::extractTitle 的核心逻辑，供当前模块调用。
QString SearchIndexManager::extractTitle(const QString &content)
{
    // 尝试从 Markdown 标题提取
    QRegularExpression h1Regex("^#\\s+(.+)$", QRegularExpression::MultilineOption);
    QRegularExpressionMatch match = h1Regex.match(content);
    if (match.hasMatch()) {
        return match.captured(1).trimmed();
    }

    // 尝试从 YAML front matter 提取
    QRegularExpression yamlTitleRegex("^---[\\s\\S]*?title:\\s*[\"']?(.+?)[\"']?\\s*$[\\s\\S]*?---",
                                       QRegularExpression::MultilineOption);
    match = yamlTitleRegex.match(content);
    if (match.hasMatch()) {
        return match.captured(1).trimmed();
    }

    // 使用第一行非空文本
    QStringList lines = content.split('\n');
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (!trimmed.isEmpty() && !trimmed.startsWith('#') && !trimmed.startsWith('-')) {
            return trimmed.left(50);
        }
    }

    return QString();
}

// 函数说明：创建 SearchIndexManager 需要的对象、记录或输出内容。
QString SearchIndexManager::createSnippet(const QString &content, int matchPos, int snippetLength)
{
    int start = qMax(0, matchPos - snippetLength / 2);
    int end = qMin(content.length(), matchPos + snippetLength / 2);

    // 调整到词边界
    while (start > 0 && !content[start].isSpace()) start--;
    while (end < content.length() && !content[end].isSpace()) end++;

    QString snippet = content.mid(start, end - start).trimmed();

    // 添加省略号
    if (start > 0) snippet = "..." + snippet;
    if (end < content.length()) snippet += "...";

    return snippet;
}

// 函数说明：实现 SearchIndexManager::search 的核心逻辑，供当前模块调用。
QList<SearchResult> SearchIndexManager::search(const QString &query, const SearchOptions &options)
{
    if (query.isEmpty()) {
        return QList<SearchResult>();
    }

    QList<SearchResult> results;

    if (options.useRegex) {
        results = regexSearch(query, options);
    } else if (options.fuzzyMatch) {
        results = fuzzySearch(query, options);
    } else {
        results = plainTextSearch(query, options);
    }

    // 按得分排序
    std::sort(results.begin(), results.end());

    // 限制结果数
    if (results.size() > options.maxResults) {
        results = results.mid(0, options.maxResults);
    }

    emit searchCompleted(results);
    return results;
}

// 函数说明：实现 SearchIndexManager::searchInDocument 的核心逻辑，供当前模块调用。
QList<SearchResult> SearchIndexManager::searchInDocument(const QString &filePath,
                                                          const QString &query,
                                                          const SearchOptions &options)
{
    QList<SearchResult> results;

    QMutexLocker locker(&m_mutex);
    if (!m_documents.contains(filePath)) {
        return results;
    }

    const DocumentIndex &doc = m_documents[filePath];
    locker.unlock();

    SearchOptions docOptions = options;
    // 在单文档搜索中不需要路径过滤

    QString searchText = options.caseSensitive ? doc.content : doc.content.toLower();
    QString searchQuery = options.caseSensitive ? query : query.toLower();

    // 检查搜索词是否包含CJK字符
    bool queryCJK = false;
    for (const QChar &ch : searchQuery) {
        if (ch.unicode() >= 0x4e00 && ch.unicode() <= 0x9fff) {
            queryCJK = true;
            break;
        }
    }

    if (options.useRegex) {
        QRegularExpression regex(query,
            options.caseSensitive ? QRegularExpression::NoPatternOption
                                  : QRegularExpression::CaseInsensitiveOption);

        if (!regex.isValid()) {
            emit error(tr("无效的正则表达式: %1").arg(regex.errorString()));
            return results;
        }

        QRegularExpressionMatchIterator it = regex.globalMatch(doc.content);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            SearchResult result;
            result.filePath = filePath;
            result.title = doc.title;
            result.matchStart = match.capturedStart();
            result.matchLength = match.capturedLength();
            result.snippet = createSnippet(doc.content, result.matchStart);
            result.score = 1.0;
            result.isFuzzyMatch = false;

            // 计算行号和列号
            int lineNumber = 1;
            int lastLineStart = 0;
            for (int i = 0; i < result.matchStart; ++i) {
                if (doc.content[i] == '\n') {
                    lineNumber++;
                    lastLineStart = i + 1;
                }
            }
            result.lineNumber = lineNumber;
            result.columnNumber = result.matchStart - lastLineStart + 1;

            results.append(result);
        }
    } else {
        int pos = 0;
        while ((pos = searchText.indexOf(searchQuery, pos)) != -1) {
            if (options.wholeWord && !queryCJK) {
                // 检查是否为完整单词（CJK字符跳过此检查）
                bool wordStart = (pos == 0 || !searchText[pos - 1].isLetterOrNumber());
                bool wordEnd = (pos + searchQuery.length() >= searchText.length() ||
                               !searchText[pos + searchQuery.length()].isLetterOrNumber());
                if (!wordStart || !wordEnd) {
                    pos++;
                    continue;
                }
            }

            SearchResult result;
            result.filePath = filePath;
            result.title = doc.title;
            result.matchStart = pos;
            result.matchLength = searchQuery.length();
            result.snippet = createSnippet(doc.content, pos);
            result.score = 1.0;
            result.isFuzzyMatch = false;

            // 计算行号和列号
            int lineNumber = 1;
            int lastLineStart = 0;
            for (int i = 0; i < pos; ++i) {
                if (doc.content[i] == '\n') {
                    lineNumber++;
                    lastLineStart = i + 1;
                }
            }
            result.lineNumber = lineNumber;
            result.columnNumber = pos - lastLineStart + 1;

            results.append(result);
            pos++;
        }
    }

    return results;
}

// 函数说明：实现 SearchIndexManager::plainTextSearch 的核心逻辑，供当前模块调用。
QList<SearchResult> SearchIndexManager::plainTextSearch(const QString &query, const SearchOptions &options)
{
    QList<SearchResult> results;
    QMutexLocker locker(&m_mutex);

    QString searchQuery = options.caseSensitive ? query : query.toLower();

    // 检查搜索词是否包含CJK字符（中日韩），CJK字符没有单词边界，全词匹配不适用
    bool queryCJK = false;
    for (const QChar &ch : searchQuery) {
        if (ch.unicode() >= 0x4e00 && ch.unicode() <= 0x9fff) {
            queryCJK = true;
            break;
        }
    }

    for (auto it = m_documents.constBegin(); it != m_documents.constEnd(); ++it) {
        const DocumentIndex &doc = it.value();

        // 文件扩展名过滤
        if (!options.fileExtensions.isEmpty()) {
            QFileInfo fi(doc.filePath);
            bool matchExt = false;
            for (const QString &ext : options.fileExtensions) {
                if (fi.suffix().compare(ext.mid(2), Qt::CaseInsensitive) == 0) { // 去掉 "*."
                    matchExt = true;
                    break;
                }
            }
            if (!matchExt) continue;
        }

        QString searchText = options.caseSensitive ? doc.content : doc.content.toLower();

        int pos = 0;
        int matchCount = 0;
        while ((pos = searchText.indexOf(searchQuery, pos)) != -1) {
            if (options.wholeWord && !queryCJK) {
                bool wordStart = (pos == 0 || !searchText[pos - 1].isLetterOrNumber());
                bool wordEnd = (pos + searchQuery.length() >= searchText.length() ||
                               !searchText[pos + searchQuery.length()].isLetterOrNumber());
                if (!wordStart || !wordEnd) {
                    pos++;
                    continue;
                }
            }

            SearchResult result;
            result.filePath = doc.filePath;
            result.title = doc.title;
            result.matchStart = pos;
            result.matchLength = searchQuery.length();
            result.snippet = createSnippet(doc.content, pos);
            result.isFuzzyMatch = false;

            // 计算行号
            int lineNumber = 1;
            int lastLineStart = 0;
            for (int i = 0; i < pos; ++i) {
                if (doc.content[i] == '\n') {
                    lineNumber++;
                    lastLineStart = i + 1;
                }
            }
            result.lineNumber = lineNumber;
            result.columnNumber = pos - lastLineStart + 1;

            // 计算得分
            result.score = 1.0;
            if (doc.title.contains(query, options.caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive)) {
                result.score += 2.0; // 标题匹配加分
            }

            results.append(result);
            matchCount++;
            pos++;

            if (results.size() >= options.maxResults) break;
        }
    }

    return results;
}

// 函数说明：实现 SearchIndexManager::regexSearch 的核心逻辑，供当前模块调用。
QList<SearchResult> SearchIndexManager::regexSearch(const QString &query, const SearchOptions &options)
{
    QList<SearchResult> results;

    QRegularExpression regex(query,
        options.caseSensitive ? QRegularExpression::NoPatternOption
                              : QRegularExpression::CaseInsensitiveOption);

    if (!regex.isValid()) {
        emit error(tr("无效的正则表达式: %1").arg(regex.errorString()));
        return results;
    }

    QMutexLocker locker(&m_mutex);

    for (auto it = m_documents.constBegin(); it != m_documents.constEnd(); ++it) {
        const DocumentIndex &doc = it.value();

        QRegularExpressionMatchIterator matchIt = regex.globalMatch(doc.content);
        while (matchIt.hasNext()) {
            QRegularExpressionMatch match = matchIt.next();

            SearchResult result;
            result.filePath = doc.filePath;
            result.title = doc.title;
            result.matchStart = match.capturedStart();
            result.matchLength = match.capturedLength();
            result.snippet = createSnippet(doc.content, result.matchStart);
            result.score = 1.0;
            result.isFuzzyMatch = false;

            // 计算行号
            int lineNumber = 1;
            int lastLineStart = 0;
            for (int i = 0; i < result.matchStart; ++i) {
                if (doc.content[i] == '\n') {
                    lineNumber++;
                    lastLineStart = i + 1;
                }
            }
            result.lineNumber = lineNumber;
            result.columnNumber = result.matchStart - lastLineStart + 1;

            results.append(result);

            if (results.size() >= options.maxResults) break;
        }

        if (results.size() >= options.maxResults) break;
    }

    return results;
}

// 函数说明：实现 SearchIndexManager::fuzzySearch 的核心逻辑，供当前模块调用。
QList<SearchResult> SearchIndexManager::fuzzySearch(const QString &query, const SearchOptions &options)
{
    QList<SearchResult> results;
    QMutexLocker locker(&m_mutex);

    QString searchQuery = query.toLower();

    for (auto it = m_documents.constBegin(); it != m_documents.constEnd(); ++it) {
        const DocumentIndex &doc = it.value();

        // 在分词列表中进行模糊匹配
        QList<QPair<int, int>> matches = findFuzzyMatches(doc.content.toLower(), searchQuery, options.fuzzyTolerance);

        for (const auto &match : matches) {
            int pos = match.first;
            int distance = match.second;

            SearchResult result;
            result.filePath = doc.filePath;
            result.title = doc.title;
            result.matchStart = pos;
            result.matchLength = searchQuery.length();
            result.snippet = createSnippet(doc.content, pos);
            result.isFuzzyMatch = (distance > 0);

            // 模糊匹配得分（距离越小得分越高）
            result.score = 1.0 - (double)distance / (options.fuzzyTolerance + 1);

            // 计算行号
            int lineNumber = 1;
            int lastLineStart = 0;
            for (int i = 0; i < pos; ++i) {
                if (doc.content[i] == '\n') {
                    lineNumber++;
                    lastLineStart = i + 1;
                }
            }
            result.lineNumber = lineNumber;
            result.columnNumber = pos - lastLineStart + 1;

            results.append(result);

            if (results.size() >= options.maxResults) break;
        }

        if (results.size() >= options.maxResults) break;
    }

    return results;
}

// 函数说明：实现 SearchIndexManager::levenshteinDistance 的核心逻辑，供当前模块调用。
int SearchIndexManager::levenshteinDistance(const QString &s1, const QString &s2)
{
    int m = s1.length();
    int n = s2.length();

    if (m == 0) return n;
    if (n == 0) return m;

    QVector<QVector<int>> dp(m + 1, QVector<int>(n + 1));

    for (int i = 0; i <= m; ++i) dp[i][0] = i;
    for (int j = 0; j <= n; ++j) dp[0][j] = j;

    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            dp[i][j] = qMin(qMin(dp[i - 1][j] + 1, dp[i][j - 1] + 1), dp[i - 1][j - 1] + cost);
        }
    }

    return dp[m][n];
}

// 函数说明：实现 SearchIndexManager::findFuzzyMatches 的核心逻辑，供当前模块调用。
QList<QPair<int, int>> SearchIndexManager::findFuzzyMatches(const QString &text,
                                                             const QString &pattern,
                                                             int maxDistance)
{
    QList<QPair<int, int>> matches;
    int patternLen = pattern.length();

    // 滑动窗口模糊匹配
    for (int i = 0; i <= text.length() - patternLen; ++i) {
        QString window = text.mid(i, patternLen);
        int distance = levenshteinDistance(window, pattern);

        if (distance <= maxDistance) {
            matches.append(qMakePair(i, distance));
            i += patternLen - 1; // 跳过已匹配的部分
        }
    }

    return matches;
}

// 函数说明：向 SearchIndexManager 管理的数据集合中添加一项内容。
void SearchIndexManager::addToHistory(const QString &query)
{
    if (query.isEmpty()) return;

    m_searchHistory.removeAll(query);
    m_searchHistory.prepend(query);

    while (m_searchHistory.size() > MaxHistorySize) {
        m_searchHistory.removeLast();
    }
}

// 函数说明：清空 SearchIndexManager 保存的临时状态或缓存数据。
void SearchIndexManager::clearHistory()
{
    m_searchHistory.clear();
}

// 函数说明：保存 SearchIndexManager 当前状态，保证用户修改可以持久化。
void SearchIndexManager::saveIndex(const QString &path)
{
    QMutexLocker locker(&m_mutex);

    QJsonObject root;
    root["version"] = 1;
    root["lastIndexTime"] = m_lastIndexTime.toString(Qt::ISODate);

    QJsonArray pathsArray;
    for (const QString &p : m_searchPaths) {
        pathsArray.append(p);
    }
    root["searchPaths"] = pathsArray;

    QJsonArray historyArray;
    for (const QString &h : m_searchHistory) {
        historyArray.append(h);
    }
    root["searchHistory"] = historyArray;

    QJsonArray docsArray;
    for (auto it = m_documents.constBegin(); it != m_documents.constEnd(); ++it) {
        const DocumentIndex &doc = it.value();
        QJsonObject docObj;
        docObj["filePath"] = doc.filePath;
        docObj["title"] = doc.title;
        docObj["lastModified"] = doc.lastModified.toString(Qt::ISODate);
        docObj["indexedAt"] = doc.indexedAt.toString(Qt::ISODate);
        docObj["fileSize"] = doc.fileSize;
        // 不保存内容和分词，重建时重新读取
        docsArray.append(docObj);
    }
    root["documents"] = docsArray;

    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}

// 函数说明：加载 SearchIndexManager 需要的数据、配置或外部资源。
void SearchIndexManager::loadIndex(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) return;

    QJsonObject root = doc.object();

    m_lastIndexTime = QDateTime::fromString(root["lastIndexTime"].toString(), Qt::ISODate);

    m_searchPaths.clear();
    QJsonArray pathsArray = root["searchPaths"].toArray();
    for (const auto &p : pathsArray) {
        m_searchPaths.append(p.toString());
    }

    m_searchHistory.clear();
    QJsonArray historyArray = root["searchHistory"].toArray();
    for (const auto &h : historyArray) {
        m_searchHistory.append(h.toString());
    }

    // 重新加载文档内容
    QJsonArray docsArray = root["documents"].toArray();
    for (const auto &d : docsArray) {
        QJsonObject docObj = d.toObject();
        QString filePath = docObj["filePath"].toString();

        QFileInfo fi(filePath);
        QDateTime savedModified = QDateTime::fromString(docObj["lastModified"].toString(), Qt::ISODate);

        // 如果文件存在且未修改，重新索引
        if (fi.exists()) {
            indexFile(filePath);
        }
    }
}

