// 文件说明：app-static\search\searchindexmanager.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef SEARCHINDEXMANAGER_H
#define SEARCHINDEXMANAGER_H

#include <QObject>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QDateTime>
#include <QDir>
#include <QFuture>
#include <QMutex>

/**
 * @brief 文档索引项
 */
struct DocumentIndex {
    QString filePath;           // 文件路径
    QString title;              // 文档标题
    QString content;            // 文档内容（用于搜索）
    QStringList words;          // 分词后的词语列表
    QDateTime lastModified;     // 最后修改时间
    QDateTime indexedAt;        // 索引时间
    qint64 fileSize;            // 文件大小

    bool isValid() const { return !filePath.isEmpty(); }
};

/**
 * @brief 搜索结果项
 */
struct SearchResult {
    QString filePath;           // 文件路径
    QString title;              // 文档标题
    QString snippet;            // 匹配的文本片段
    int lineNumber;             // 行号
    int columnNumber;           // 列号
    int matchStart;             // 匹配开始位置
    int matchLength;            // 匹配长度
    double score;               // 相关性得分
    bool isFuzzyMatch;          // 是否为模糊匹配

    bool operator<(const SearchResult &other) const {
        return score > other.score; // 按得分降序
    }
};

/**
 * @brief 搜索选项
 */
struct SearchOptions {
    bool caseSensitive = false;     // 区分大小写
    bool wholeWord = false;         // 全词匹配
    bool useRegex = false;          // 使用正则表达式
    bool fuzzyMatch = false;        // 模糊匹配
    int fuzzyTolerance = 2;         // 模糊匹配容错字符数
    int maxResults = 100;           // 最大结果数
    QStringList fileExtensions;     // 文件扩展名过滤
    QString searchPath;             // 搜索路径
};

/**
 * @brief 搜索索引管理器
 *
 * 提供全文搜索、模糊匹配、正则表达式搜索等功能
 */
class SearchIndexManager : public QObject
{
    Q_OBJECT

public:
    explicit SearchIndexManager(QObject *parent = nullptr);
    ~SearchIndexManager();

    // 索引管理
    void addSearchPath(const QString &path);
    void removeSearchPath(const QString &path);
    QStringList searchPaths() const { return m_searchPaths; }

    void buildIndex();
    void rebuildIndex();
    void updateIndex(const QString &filePath);
    void removeFromIndex(const QString &filePath);
    void clearIndex();

    // 索引状态
    bool isIndexing() const { return m_isIndexing; }
    int indexedDocumentCount() const { return m_documents.size(); }
    QDateTime lastIndexTime() const { return m_lastIndexTime; }

    // 搜索功能
    QList<SearchResult> search(const QString &query, const SearchOptions &options = SearchOptions());
    QList<SearchResult> searchInDocument(const QString &filePath, const QString &query, const SearchOptions &options = SearchOptions());

    // 直接索引文本内容（用于未保存的编辑器内容）
    void indexContent(const QString &virtualPath, const QString &content, const QString &title = QString());

    // 搜索历史
    void addToHistory(const QString &query);
    QStringList searchHistory() const { return m_searchHistory; }
    void clearHistory();

    // 持久化
    void saveIndex(const QString &path);
    void loadIndex(const QString &path);

signals:
    void indexingStarted();
    void indexingProgress(int current, int total, const QString &currentFile);
    void indexingFinished(int documentCount);
    void indexUpdated(const QString &filePath);
    void searchCompleted(const QList<SearchResult> &results);
    void error(const QString &message);

private:
    void indexDirectory(const QString &path);
    void indexFile(const QString &filePath);
    DocumentIndex createDocumentIndex(const QString &filePath);
    QStringList tokenize(const QString &text);
    QString extractTitle(const QString &content);
    QString createSnippet(const QString &content, int matchPos, int snippetLength = 100);

    // 搜索算法
    QList<SearchResult> plainTextSearch(const QString &query, const SearchOptions &options);
    QList<SearchResult> regexSearch(const QString &query, const SearchOptions &options);
    QList<SearchResult> fuzzySearch(const QString &query, const SearchOptions &options);

    // 模糊匹配
    int levenshteinDistance(const QString &s1, const QString &s2);
    QList<QPair<int, int>> findFuzzyMatches(const QString &text, const QString &pattern, int maxDistance);

    QStringList m_searchPaths;
    QMap<QString, DocumentIndex> m_documents;
    QStringList m_searchHistory;
    QDateTime m_lastIndexTime;

    bool m_isIndexing;
    mutable QMutex m_mutex;

    static const int MaxHistorySize = 50;
    static const QStringList DefaultExtensions;
};

#endif // SEARCHINDEXMANAGER_H

