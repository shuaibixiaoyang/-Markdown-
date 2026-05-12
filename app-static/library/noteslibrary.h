// 文件说明：app-static\library\noteslibrary.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef NOTESLIBRARY_H
#define NOTESLIBRARY_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QMap>
#include <QVector>
#include <QSqlDatabase>
#include <QFileSystemWatcher>
#include <QFuture>

/**
 * @brief 笔记库管理器
 *
 * 功能：
 * - 笔记库索引和管理
 * - 标签系统
 * - 文件夹组织
 * - 全文搜索
 * - 笔记元数据管理
 * - 文件监控
 */
class NotesLibrary : public QObject
{
    Q_OBJECT

public:
    // 笔记信息
    struct NoteInfo {
        QString id;             // 唯一ID (文件路径的哈希)
        QString filePath;       // 文件路径
        QString title;          // 标题
        QString preview;        // 预览文本
        QStringList tags;       // 标签
        QString folder;         // 所属文件夹
        QDateTime createdTime;  // 创建时间
        QDateTime modifiedTime; // 修改时间
        qint64 fileSize;        // 文件大小
        int wordCount;          // 字数
        bool isFavorite;        // 是否收藏
        bool isArchived;        // 是否归档

        NoteInfo() : fileSize(0), wordCount(0), isFavorite(false), isArchived(false) {}
    };

    // 搜索结果
    struct SearchResult {
        NoteInfo note;
        QString matchedText;    // 匹配的文本片段
        int matchCount;         // 匹配次数
        double relevance;       // 相关度分数

        SearchResult() : matchCount(0), relevance(0.0) {}
    };

    // 标签信息
    struct TagInfo {
        QString name;
        QString color;          // 颜色 (hex)
        int noteCount;          // 关联笔记数

        TagInfo() : noteCount(0) {}
    };

    // 文件夹信息
    struct FolderInfo {
        QString path;
        QString name;
        int noteCount;
        int subfolderCount;
        QStringList subfolders;

        FolderInfo() : noteCount(0), subfolderCount(0) {}
    };

    // 搜索选项
    struct SearchOptions {
        bool caseSensitive;
        bool wholeWord;
        bool useRegex;
        bool searchInContent;
        bool searchInTitle;
        bool searchInTags;
        QStringList filterTags;
        QString filterFolder;
        QDateTime fromDate;
        QDateTime toDate;
        int maxResults;

        SearchOptions()
            : caseSensitive(false)
            , wholeWord(false)
            , useRegex(false)
            , searchInContent(true)
            , searchInTitle(true)
            , searchInTags(true)
            , maxResults(100)
        {}
    };

    // 排序方式
    enum class SortBy {
        Title,
        ModifiedTime,
        CreatedTime,
        FileSize,
        WordCount
    };
    Q_ENUM(SortBy)

    explicit NotesLibrary(QObject *parent = nullptr);
    ~NotesLibrary();

    // 库管理
    bool openLibrary(const QString &rootPath);
    void closeLibrary();
    bool isOpen() const { return m_isOpen; }
    QString libraryPath() const { return m_libraryPath; }

    // 索引管理
    void rebuildIndex();
    void updateIndex(const QString &filePath);
    void removeFromIndex(const QString &filePath);
    int indexedNoteCount() const;
    bool isIndexing() const { return m_isIndexing; }

    // 笔记操作
    NoteInfo getNoteInfo(const QString &filePath);
    QVector<NoteInfo> getAllNotes(SortBy sortBy = SortBy::ModifiedTime, bool ascending = false);
    QVector<NoteInfo> getRecentNotes(int count = 10);
    QVector<NoteInfo> getFavoriteNotes();
    QVector<NoteInfo> getArchivedNotes();

    // 搜索
    QVector<SearchResult> search(const QString &query, const SearchOptions &options = SearchOptions());
    QVector<SearchResult> searchAsync(const QString &query, const SearchOptions &options = SearchOptions());
    QVector<NoteInfo> quickSearch(const QString &query, int maxResults = 20);

    // 标签管理
    void addTag(const QString &filePath, const QString &tag);
    void removeTag(const QString &filePath, const QString &tag);
    void renameTag(const QString &oldName, const QString &newName);
    void deleteTag(const QString &tag);
    void setTagColor(const QString &tag, const QString &color);
    QVector<TagInfo> getAllTags();
    QVector<NoteInfo> getNotesByTag(const QString &tag);

    // 文件夹管理
    QVector<FolderInfo> getFolders();
    QVector<NoteInfo> getNotesByFolder(const QString &folderPath, bool recursive = false);
    bool createFolder(const QString &folderPath);
    bool renameFolder(const QString &oldPath, const QString &newPath);
    bool deleteFolder(const QString &folderPath, bool deleteFiles = false);

    // 收藏和归档
    void setFavorite(const QString &filePath, bool favorite);
    void setArchived(const QString &filePath, bool archived);

    // 笔记创建
    QString createNote(const QString &title, const QString &folder = QString());
    QString createNoteFromTemplate(const QString &templatePath, const QString &folder = QString());
    bool deleteNote(const QString &filePath);
    bool moveNote(const QString &filePath, const QString &newFolder);
    bool duplicateNote(const QString &filePath);

    // 导入导出
    bool importNotes(const QStringList &filePaths, const QString &folder = QString());
    bool exportNotes(const QStringList &filePaths, const QString &exportPath);

    // 统计
    int getTotalNoteCount();
    int getTotalWordCount();
    QMap<QString, int> getTagStatistics();
    QMap<QDate, int> getActivityStatistics(int days = 30);

    // 错误信息
    QString lastError() const { return m_lastError; }

signals:
    void libraryOpened(const QString &path);
    void libraryClosed();
    void indexingStarted();
    void indexingProgress(int current, int total);
    void indexingCompleted();
    void noteAdded(const QString &filePath);
    void noteModified(const QString &filePath);
    void noteRemoved(const QString &filePath);
    void tagAdded(const QString &tag);
    void tagRemoved(const QString &tag);
    void searchCompleted(const QVector<SearchResult> &results);
    void errorOccurred(const QString &error);

private slots:
    void onFileChanged(const QString &path);
    void onDirectoryChanged(const QString &path);

private:
    // 数据库操作
    bool initDatabase();
    void createTables();
    bool insertNote(const NoteInfo &note);
    bool updateNote(const NoteInfo &note);
    bool deleteNoteFromDb(const QString &filePath);

    // 索引操作
    void indexFile(const QString &filePath);
    void indexDirectory(const QString &dirPath);
    NoteInfo extractNoteInfo(const QString &filePath);
    QString extractTitle(const QString &content);
    QString extractPreview(const QString &content, int maxLength = 200);
    QStringList extractTags(const QString &content);
    int countWords(const QString &content);

    // 全文搜索
    void buildSearchIndex(const QString &filePath, const QString &content);
    QVector<SearchResult> performSearch(const QString &query, const SearchOptions &options);
    double calculateRelevance(const QString &content, const QString &query);

    // 辅助函数
    QString generateNoteId(const QString &filePath);
    QString getRelativePath(const QString &filePath);
    QString getAbsolutePath(const QString &relativePath);

    QString m_libraryPath;
    QSqlDatabase m_database;
    QFileSystemWatcher *m_watcher;
    bool m_isOpen;
    bool m_isIndexing;
    QString m_lastError;

    // 缓存
    QMap<QString, NoteInfo> m_noteCache;
    QMap<QString, TagInfo> m_tagCache;
    bool m_cacheValid;
};

#endif // NOTESLIBRARY_H

