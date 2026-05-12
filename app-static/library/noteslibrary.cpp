// 文件说明：app-static\library\noteslibrary.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "noteslibrary.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDirIterator>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QTextStream>
#include <QtConcurrent>
#include <QDebug>
#include <algorithm>

// 函数说明：构造 NotesLibrary 对象，初始化本模块需要的状态、界面和资源。
NotesLibrary::NotesLibrary(QObject *parent)
    : QObject(parent)
    , m_watcher(new QFileSystemWatcher(this))
    , m_isOpen(false)
    , m_isIndexing(false)
    , m_cacheValid(false)
{
    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &NotesLibrary::onFileChanged);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &NotesLibrary::onDirectoryChanged);
}

// 函数说明：销毁 NotesLibrary 对象，释放本模块持有的资源。
NotesLibrary::~NotesLibrary()
{
    closeLibrary();
}

// 函数说明：打开 NotesLibrary 对应的文件、资源或功能入口。
bool NotesLibrary::openLibrary(const QString &rootPath)
{
    if (m_isOpen) {
        closeLibrary();
    }

    QDir dir(rootPath);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            m_lastError = tr("无法创建笔记库目录: %1").arg(rootPath);
            emit errorOccurred(m_lastError);
            return false;
        }
    }

    m_libraryPath = rootPath;

    // 初始化数据库
    if (!initDatabase()) {
        return false;
    }

    // 添加文件监控
    m_watcher->addPath(rootPath);

    m_isOpen = true;
    emit libraryOpened(rootPath);

    // 开始索引
    rebuildIndex();

    return true;
}

// 函数说明：关闭 NotesLibrary 相关窗口或资源，并处理必要的保存确认。
void NotesLibrary::closeLibrary()
{
    if (!m_isOpen) return;

    // 移除文件监控
    if (!m_watcher->directories().isEmpty()) {
        m_watcher->removePaths(m_watcher->directories());
    }
    if (!m_watcher->files().isEmpty()) {
        m_watcher->removePaths(m_watcher->files());
    }

    // 关闭数据库
    if (m_database.isOpen()) {
        m_database.close();
    }

    m_noteCache.clear();
    m_tagCache.clear();
    m_cacheValid = false;
    m_isOpen = false;
    m_libraryPath.clear();

    emit libraryClosed();
}

// 函数说明：实现 NotesLibrary::initDatabase 的核心逻辑，供当前模块调用。
bool NotesLibrary::initDatabase()
{
    QString dbPath = m_libraryPath + "/.noteslibrary.db";

    m_database = QSqlDatabase::addDatabase("QSQLITE", "noteslibrary");
    m_database.setDatabaseName(dbPath);

    if (!m_database.open()) {
        m_lastError = tr("无法打开数据库: %1").arg(m_database.lastError().text());
        emit errorOccurred(m_lastError);
        return false;
    }

    createTables();
    return true;
}

// 函数说明：创建 NotesLibrary 需要的对象、记录或输出内容。
void NotesLibrary::createTables()
{
    QSqlQuery query(m_database);

    // 笔记表
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS notes (
            id TEXT PRIMARY KEY,
            file_path TEXT UNIQUE NOT NULL,
            title TEXT,
            preview TEXT,
            folder TEXT,
            created_time TEXT,
            modified_time TEXT,
            file_size INTEGER,
            word_count INTEGER,
            is_favorite INTEGER DEFAULT 0,
            is_archived INTEGER DEFAULT 0
        )
    )");

    // 标签表
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS tags (
            name TEXT PRIMARY KEY,
            color TEXT
        )
    )");

    // 笔记-标签关联表
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS note_tags (
            note_id TEXT,
            tag_name TEXT,
            PRIMARY KEY (note_id, tag_name),
            FOREIGN KEY (note_id) REFERENCES notes(id),
            FOREIGN KEY (tag_name) REFERENCES tags(name)
        )
    )");

    // 全文搜索表
    query.exec(R"(
        CREATE VIRTUAL TABLE IF NOT EXISTS notes_fts USING fts5(
            note_id,
            title,
            content,
            tags
        )
    )");

    // 索引
    query.exec("CREATE INDEX IF NOT EXISTS idx_notes_folder ON notes(folder)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_notes_modified ON notes(modified_time)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_notes_favorite ON notes(is_favorite)");
}

// 函数说明：实现 NotesLibrary::rebuildIndex 的核心逻辑，供当前模块调用。
void NotesLibrary::rebuildIndex()
{
    if (!m_isOpen || m_isIndexing) return;

    m_isIndexing = true;
    emit indexingStarted();

    // 清空现有索引
    QSqlQuery query(m_database);
    query.exec("DELETE FROM notes_fts");
    query.exec("DELETE FROM note_tags");
    query.exec("DELETE FROM notes");

    m_noteCache.clear();
    m_tagCache.clear();

    // 扫描目录
    indexDirectory(m_libraryPath);

    m_cacheValid = true;
    m_isIndexing = false;
    emit indexingCompleted();
}

// 函数说明：实现 NotesLibrary::indexDirectory 的核心逻辑，供当前模块调用。
void NotesLibrary::indexDirectory(const QString &dirPath)
{
    QDir dir(dirPath);
    QStringList filters;
    filters << "*.md" << "*.markdown" << "*.txt";

    // 计算总文件数
    int totalFiles = 0;
    QDirIterator countIt(dirPath, filters, QDir::Files, QDirIterator::Subdirectories);
    while (countIt.hasNext()) {
        countIt.next();
        totalFiles++;
    }

    // 索引文件
    int current = 0;
    QDirIterator it(dirPath, filters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString filePath = it.next();
        indexFile(filePath);

        current++;
        emit indexingProgress(current, totalFiles);

        // 添加文件监控
        m_watcher->addPath(filePath);
    }

    // 监控子目录
    QDirIterator dirIt(dirPath, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (dirIt.hasNext()) {
        m_watcher->addPath(dirIt.next());
    }
}

// 函数说明：实现 NotesLibrary::indexFile 的核心逻辑，供当前模块调用。
void NotesLibrary::indexFile(const QString &filePath)
{
    NoteInfo note = extractNoteInfo(filePath);
    insertNote(note);

    // 更新全文搜索索引
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QTextStream(&file).readAll();
        buildSearchIndex(note.id, content);
        file.close();
    }

    m_noteCache[filePath] = note;
    emit noteAdded(filePath);
}

// 函数说明：实现 NotesLibrary::extractNoteInfo 的核心逻辑，供当前模块调用。
NotesLibrary::NoteInfo NotesLibrary::extractNoteInfo(const QString &filePath)
{
    NoteInfo note;
    note.id = generateNoteId(filePath);
    note.filePath = filePath;

    QFileInfo fileInfo(filePath);
    note.createdTime = fileInfo.birthTime();
    note.modifiedTime = fileInfo.lastModified();
    note.fileSize = fileInfo.size();
    note.folder = getRelativePath(fileInfo.path());

    // 读取文件内容
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QTextStream(&file).readAll();
        note.title = extractTitle(content);
        note.preview = extractPreview(content);
        note.tags = extractTags(content);
        note.wordCount = countWords(content);
        file.close();
    }

    if (note.title.isEmpty()) {
        note.title = fileInfo.baseName();
    }

    return note;
}

// 函数说明：实现 NotesLibrary::extractTitle 的核心逻辑，供当前模块调用。
QString NotesLibrary::extractTitle(const QString &content)
{
    // 查找第一个标题
    QRegularExpression headingRegex("^#+\\s+(.+)$", QRegularExpression::MultilineOption);
    QRegularExpressionMatch match = headingRegex.match(content);

    if (match.hasMatch()) {
        return match.captured(1).trimmed();
    }

    // 使用第一行作为标题
    int newlinePos = content.indexOf('\n');
    if (newlinePos > 0) {
        return content.left(newlinePos).trimmed();
    }

    return content.left(50).trimmed();
}

// 函数说明：实现 NotesLibrary::extractPreview 的核心逻辑，供当前模块调用。
QString NotesLibrary::extractPreview(const QString &content, int maxLength)
{
    // 跳过 YAML front matter
    QString text = content;
    if (text.startsWith("---")) {
        int endPos = text.indexOf("---", 3);
        if (endPos > 0) {
            text = text.mid(endPos + 3).trimmed();
        }
    }

    // 跳过标题
    QRegularExpression headingRegex("^#+\\s+.+$", QRegularExpression::MultilineOption);
    text.remove(headingRegex);

    // 移除 Markdown 格式
    text.remove(QRegularExpression("\\*\\*|__|\\*|_|`|#|\\[|\\]|\\(|\\)"));
    text = text.trimmed();

    if (text.length() > maxLength) {
        text = text.left(maxLength) + "...";
    }

    return text;
}

// 函数说明：实现 NotesLibrary::extractTags 的核心逻辑，供当前模块调用。
QStringList NotesLibrary::extractTags(const QString &content)
{
    QStringList tags;

    // 从 YAML front matter 中提取
    if (content.startsWith("---")) {
        int endPos = content.indexOf("---", 3);
        if (endPos > 0) {
            QString yaml = content.mid(3, endPos - 3);
            QRegularExpression tagRegex("tags:\\s*\\[([^\\]]+)\\]|tags:\\s*\\n((?:\\s*-\\s*.+\\n)+)");
            QRegularExpressionMatch match = tagRegex.match(yaml);

            if (match.hasMatch()) {
                QString tagStr = match.captured(1).isEmpty() ? match.captured(2) : match.captured(1);
                QRegularExpression itemRegex("[,\\n]\\s*-?\\s*");
                tags = tagStr.split(itemRegex, Qt::SkipEmptyParts);
                for (QString &tag : tags) {
                    tag = tag.trimmed().remove('"').remove('\'');
                }
            }
        }
    }

    // 从内容中提取 #tag 格式的标签
    QRegularExpression hashTagRegex("#([\\w\\u4e00-\\u9fff]+)");
    QRegularExpressionMatchIterator it = hashTagRegex.globalMatch(content);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString tag = match.captured(1);
        if (!tags.contains(tag) && tag.length() > 1) {
            tags.append(tag);
        }
    }

    return tags;
}

// 函数说明：实现 NotesLibrary::countWords 的核心逻辑，供当前模块调用。
int NotesLibrary::countWords(const QString &content)
{
    // 中文按字符计数，英文按单词计数
    int count = 0;

    // 统计中文字符
    QRegularExpression chineseRegex("[\\u4e00-\\u9fff]");
    count += content.count(chineseRegex);

    // 统计英文单词
    QString englishText = content;
    englishText.remove(QRegularExpression("[\\u4e00-\\u9fff]"));
    QRegularExpression wordRegex("\\b\\w+\\b");
    QRegularExpressionMatchIterator it = wordRegex.globalMatch(englishText);
    while (it.hasNext()) {
        it.next();
        count++;
    }

    return count;
}

// 函数说明：实现 NotesLibrary::insertNote 的核心逻辑，供当前模块调用。
bool NotesLibrary::insertNote(const NoteInfo &note)
{
    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT OR REPLACE INTO notes
        (id, file_path, title, preview, folder, created_time, modified_time,
         file_size, word_count, is_favorite, is_archived)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    query.addBindValue(note.id);
    query.addBindValue(note.filePath);
    query.addBindValue(note.title);
    query.addBindValue(note.preview);
    query.addBindValue(note.folder);
    query.addBindValue(note.createdTime.toString(Qt::ISODate));
    query.addBindValue(note.modifiedTime.toString(Qt::ISODate));
    query.addBindValue(note.fileSize);
    query.addBindValue(note.wordCount);
    query.addBindValue(note.isFavorite ? 1 : 0);
    query.addBindValue(note.isArchived ? 1 : 0);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    // 插入标签
    for (const QString &tag : note.tags) {
        // 确保标签存在
        QSqlQuery tagQuery(m_database);
        tagQuery.prepare("INSERT OR IGNORE INTO tags (name) VALUES (?)");
        tagQuery.addBindValue(tag);
        tagQuery.exec();

        // 关联笔记和标签
        QSqlQuery linkQuery(m_database);
        linkQuery.prepare("INSERT OR IGNORE INTO note_tags (note_id, tag_name) VALUES (?, ?)");
        linkQuery.addBindValue(note.id);
        linkQuery.addBindValue(tag);
        linkQuery.exec();
    }

    return true;
}

// 函数说明：实现 NotesLibrary::buildSearchIndex 的核心逻辑，供当前模块调用。
void NotesLibrary::buildSearchIndex(const QString &noteId, const QString &content)
{
    // 获取笔记信息
    NoteInfo note;
    if (m_noteCache.contains(noteId)) {
        note = m_noteCache[noteId];
    }

    QSqlQuery query(m_database);
    query.prepare("INSERT OR REPLACE INTO notes_fts (note_id, title, content, tags) VALUES (?, ?, ?, ?)");
    query.addBindValue(noteId);
    query.addBindValue(note.title);
    query.addBindValue(content);
    query.addBindValue(note.tags.join(" "));
    query.exec();
}

// 函数说明：刷新 NotesLibrary 的内部状态，并同步到相关界面。
void NotesLibrary::updateIndex(const QString &filePath)
{
    if (!m_isOpen) return;

    // 移除旧索引
    removeFromIndex(filePath);

    // 重新索引
    indexFile(filePath);

    emit noteModified(filePath);
}

// 函数说明：从 NotesLibrary 管理的数据集合中移除指定内容。
void NotesLibrary::removeFromIndex(const QString &filePath)
{
    QString noteId = generateNoteId(filePath);

    QSqlQuery query(m_database);

    // 删除全文索引
    query.prepare("DELETE FROM notes_fts WHERE note_id = ?");
    query.addBindValue(noteId);
    query.exec();

    // 删除标签关联
    query.prepare("DELETE FROM note_tags WHERE note_id = ?");
    query.addBindValue(noteId);
    query.exec();

    // 删除笔记记录
    query.prepare("DELETE FROM notes WHERE id = ?");
    query.addBindValue(noteId);
    query.exec();

    m_noteCache.remove(filePath);
    emit noteRemoved(filePath);
}

// 函数说明：实现 NotesLibrary::indexedNoteCount 的核心逻辑，供当前模块调用。
int NotesLibrary::indexedNoteCount() const
{
    if (!m_isOpen) return 0;

    QSqlQuery query(m_database);
    query.exec("SELECT COUNT(*) FROM notes");
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

// 函数说明：读取 NotesLibrary 当前保存的状态或计算结果。
NotesLibrary::NoteInfo NotesLibrary::getNoteInfo(const QString &filePath)
{
    if (m_noteCache.contains(filePath)) {
        return m_noteCache[filePath];
    }

    NoteInfo note;
    QString noteId = generateNoteId(filePath);

    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM notes WHERE id = ?");
    query.addBindValue(noteId);

    if (query.exec() && query.next()) {
        note.id = query.value("id").toString();
        note.filePath = query.value("file_path").toString();
        note.title = query.value("title").toString();
        note.preview = query.value("preview").toString();
        note.folder = query.value("folder").toString();
        note.createdTime = QDateTime::fromString(query.value("created_time").toString(), Qt::ISODate);
        note.modifiedTime = QDateTime::fromString(query.value("modified_time").toString(), Qt::ISODate);
        note.fileSize = query.value("file_size").toLongLong();
        note.wordCount = query.value("word_count").toInt();
        note.isFavorite = query.value("is_favorite").toBool();
        note.isArchived = query.value("is_archived").toBool();

        // 获取标签
        QSqlQuery tagQuery(m_database);
        tagQuery.prepare("SELECT tag_name FROM note_tags WHERE note_id = ?");
        tagQuery.addBindValue(noteId);
        if (tagQuery.exec()) {
            while (tagQuery.next()) {
                note.tags.append(tagQuery.value(0).toString());
            }
        }
    }

    return note;
}

// 函数说明：读取 NotesLibrary 当前保存的状态或计算结果。
QVector<NotesLibrary::NoteInfo> NotesLibrary::getAllNotes(SortBy sortBy, bool ascending)
{
    QVector<NoteInfo> notes;
    if (!m_isOpen) return notes;

    QString orderColumn;
    switch (sortBy) {
        case SortBy::Title: orderColumn = "title"; break;
        case SortBy::ModifiedTime: orderColumn = "modified_time"; break;
        case SortBy::CreatedTime: orderColumn = "created_time"; break;
        case SortBy::FileSize: orderColumn = "file_size"; break;
        case SortBy::WordCount: orderColumn = "word_count"; break;
    }

    QString sql = QString("SELECT file_path FROM notes WHERE is_archived = 0 ORDER BY %1 %2")
        .arg(orderColumn)
        .arg(ascending ? "ASC" : "DESC");

    QSqlQuery query(m_database);
    query.exec(sql);

    while (query.next()) {
        notes.append(getNoteInfo(query.value(0).toString()));
    }

    return notes;
}

// 函数说明：读取 NotesLibrary 当前保存的状态或计算结果。
QVector<NotesLibrary::NoteInfo> NotesLibrary::getRecentNotes(int count)
{
    QVector<NoteInfo> notes;
    if (!m_isOpen) return notes;

    QSqlQuery query(m_database);
    query.prepare("SELECT file_path FROM notes WHERE is_archived = 0 ORDER BY modified_time DESC LIMIT ?");
    query.addBindValue(count);

    if (query.exec()) {
        while (query.next()) {
            notes.append(getNoteInfo(query.value(0).toString()));
        }
    }

    return notes;
}

// 函数说明：读取 NotesLibrary 当前保存的状态或计算结果。
QVector<NotesLibrary::NoteInfo> NotesLibrary::getFavoriteNotes()
{
    QVector<NoteInfo> notes;
    if (!m_isOpen) return notes;

    QSqlQuery query(m_database);
    query.exec("SELECT file_path FROM notes WHERE is_favorite = 1 ORDER BY modified_time DESC");

    while (query.next()) {
        notes.append(getNoteInfo(query.value(0).toString()));
    }

    return notes;
}

// 函数说明：读取 NotesLibrary 当前保存的状态或计算结果。
QVector<NotesLibrary::NoteInfo> NotesLibrary::getArchivedNotes()
{
    QVector<NoteInfo> notes;
    if (!m_isOpen) return notes;

    QSqlQuery query(m_database);
    query.exec("SELECT file_path FROM notes WHERE is_archived = 1 ORDER BY modified_time DESC");

    while (query.next()) {
        notes.append(getNoteInfo(query.value(0).toString()));
    }

    return notes;
}

// 函数说明：实现 NotesLibrary::search 的核心逻辑，供当前模块调用。
QVector<NotesLibrary::SearchResult> NotesLibrary::search(const QString &query, const SearchOptions &options)
{
    return performSearch(query, options);
}

// 函数说明：实现 NotesLibrary::performSearch 的核心逻辑，供当前模块调用。
QVector<NotesLibrary::SearchResult> NotesLibrary::performSearch(const QString &queryStr, const SearchOptions &options)
{
    QVector<SearchResult> results;
    if (!m_isOpen || queryStr.isEmpty()) return results;

    // 使用 FTS5 全文搜索
    QString ftsQuery = queryStr;
    if (!options.useRegex) {
        // 转义特殊字符
        ftsQuery.replace("\"", "\"\"");
        ftsQuery = "\"" + ftsQuery + "\"";
    }

    QSqlQuery query(m_database);
    query.prepare(R"(
        SELECT note_id, snippet(notes_fts, 2, '<mark>', '</mark>', '...', 32) as snippet
        FROM notes_fts
        WHERE notes_fts MATCH ?
        LIMIT ?
    )");
    query.addBindValue(ftsQuery);
    query.addBindValue(options.maxResults);

    if (query.exec()) {
        while (query.next()) {
            SearchResult result;
            QString noteId = query.value(0).toString();

            // 查找对应的文件路径
            QSqlQuery noteQuery(m_database);
            noteQuery.prepare("SELECT file_path FROM notes WHERE id = ?");
            noteQuery.addBindValue(noteId);
            if (noteQuery.exec() && noteQuery.next()) {
                result.note = getNoteInfo(noteQuery.value(0).toString());
            }

            result.matchedText = query.value(1).toString();
            result.matchCount = 1;

            // 应用过滤器
            bool passFilter = true;

            if (!options.filterTags.isEmpty()) {
                bool hasTag = false;
                for (const QString &tag : options.filterTags) {
                    if (result.note.tags.contains(tag)) {
                        hasTag = true;
                        break;
                    }
                }
                passFilter = passFilter && hasTag;
            }

            if (!options.filterFolder.isEmpty()) {
                passFilter = passFilter && result.note.folder.startsWith(options.filterFolder);
            }

            if (options.fromDate.isValid()) {
                passFilter = passFilter && result.note.modifiedTime >= options.fromDate;
            }

            if (options.toDate.isValid()) {
                passFilter = passFilter && result.note.modifiedTime <= options.toDate;
            }

            if (passFilter) {
                results.append(result);
            }
        }
    }

    return results;
}

// 函数说明：实现 NotesLibrary::quickSearch 的核心逻辑，供当前模块调用。
QVector<NotesLibrary::NoteInfo> NotesLibrary::quickSearch(const QString &query, int maxResults)
{
    QVector<NoteInfo> notes;
    if (!m_isOpen || query.isEmpty()) return notes;

    // 简单的标题搜索
    QSqlQuery sqlQuery(m_database);
    sqlQuery.prepare("SELECT file_path FROM notes WHERE title LIKE ? LIMIT ?");
    sqlQuery.addBindValue("%" + query + "%");
    sqlQuery.addBindValue(maxResults);

    if (sqlQuery.exec()) {
        while (sqlQuery.next()) {
            notes.append(getNoteInfo(sqlQuery.value(0).toString()));
        }
    }

    return notes;
}

// 函数说明：向 NotesLibrary 管理的数据集合中添加一项内容。
void NotesLibrary::addTag(const QString &filePath, const QString &tag)
{
    QString noteId = generateNoteId(filePath);

    // 确保标签存在
    QSqlQuery query(m_database);
    query.prepare("INSERT OR IGNORE INTO tags (name) VALUES (?)");
    query.addBindValue(tag);
    query.exec();

    // 关联
    query.prepare("INSERT OR IGNORE INTO note_tags (note_id, tag_name) VALUES (?, ?)");
    query.addBindValue(noteId);
    query.addBindValue(tag);
    query.exec();

    // 更新缓存
    if (m_noteCache.contains(filePath)) {
        m_noteCache[filePath].tags.append(tag);
    }

    emit tagAdded(tag);
}

// 函数说明：从 NotesLibrary 管理的数据集合中移除指定内容。
void NotesLibrary::removeTag(const QString &filePath, const QString &tag)
{
    QString noteId = generateNoteId(filePath);

    QSqlQuery query(m_database);
    query.prepare("DELETE FROM note_tags WHERE note_id = ? AND tag_name = ?");
    query.addBindValue(noteId);
    query.addBindValue(tag);
    query.exec();

    // 更新缓存
    if (m_noteCache.contains(filePath)) {
        m_noteCache[filePath].tags.removeAll(tag);
    }
}

// 函数说明：实现 NotesLibrary::renameTag 的核心逻辑，供当前模块调用。
void NotesLibrary::renameTag(const QString &oldName, const QString &newName)
{
    QSqlQuery query(m_database);

    // 更新标签表
    query.prepare("UPDATE tags SET name = ? WHERE name = ?");
    query.addBindValue(newName);
    query.addBindValue(oldName);
    query.exec();

    // 更新关联表
    query.prepare("UPDATE note_tags SET tag_name = ? WHERE tag_name = ?");
    query.addBindValue(newName);
    query.addBindValue(oldName);
    query.exec();

    // 更新缓存
    for (auto it = m_noteCache.begin(); it != m_noteCache.end(); ++it) {
        it.value().tags.replaceInStrings(oldName, newName);
    }
}

// 函数说明：删除 NotesLibrary 管理的指定数据或资源。
void NotesLibrary::deleteTag(const QString &tag)
{
    QSqlQuery query(m_database);

    // 删除关联
    query.prepare("DELETE FROM note_tags WHERE tag_name = ?");
    query.addBindValue(tag);
    query.exec();

    // 删除标签
    query.prepare("DELETE FROM tags WHERE name = ?");
    query.addBindValue(tag);
    query.exec();

    // 更新缓存
    for (auto it = m_noteCache.begin(); it != m_noteCache.end(); ++it) {
        it.value().tags.removeAll(tag);
    }

    emit tagRemoved(tag);
}

// 函数说明：设置 NotesLibrary 的运行参数，并触发必要的界面或数据刷新。
void NotesLibrary::setTagColor(const QString &tag, const QString &color)
{
    QSqlQuery query(m_database);
    query.prepare("UPDATE tags SET color = ? WHERE name = ?");
    query.addBindValue(color);
    query.addBindValue(tag);
    query.exec();
}

// 函数说明：读取 NotesLibrary 当前保存的状态或计算结果。
QVector<NotesLibrary::TagInfo> NotesLibrary::getAllTags()
{
    QVector<TagInfo> tags;
    if (!m_isOpen) return tags;

    QSqlQuery query(m_database);
    query.exec(R"(
        SELECT t.name, t.color, COUNT(nt.note_id) as count
        FROM tags t
        LEFT JOIN note_tags nt ON t.name = nt.tag_name
        GROUP BY t.name
        ORDER BY count DESC
    )");

    while (query.next()) {
        TagInfo tag;
        tag.name = query.value(0).toString();
        tag.color = query.value(1).toString();
        tag.noteCount = query.value(2).toInt();
        tags.append(tag);
    }

    return tags;
}

// 函数说明：读取 NotesLibrary 当前保存的状态或计算结果。
QVector<NotesLibrary::NoteInfo> NotesLibrary::getNotesByTag(const QString &tag)
{
    QVector<NoteInfo> notes;
    if (!m_isOpen) return notes;

    QSqlQuery query(m_database);
    query.prepare(R"(
        SELECT n.file_path
        FROM notes n
        JOIN note_tags nt ON n.id = nt.note_id
        WHERE nt.tag_name = ?
        ORDER BY n.modified_time DESC
    )");
    query.addBindValue(tag);

    if (query.exec()) {
        while (query.next()) {
            notes.append(getNoteInfo(query.value(0).toString()));
        }
    }

    return notes;
}

// 函数说明：读取 NotesLibrary 当前保存的状态或计算结果。
QVector<NotesLibrary::FolderInfo> NotesLibrary::getFolders()
{
    QVector<FolderInfo> folders;
    if (!m_isOpen) return folders;

    QDir rootDir(m_libraryPath);
    QDirIterator it(m_libraryPath, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    // 添加根目录
    FolderInfo root;
    root.path = "";
    root.name = QFileInfo(m_libraryPath).fileName();
    root.noteCount = indexedNoteCount();
    folders.append(root);

    while (it.hasNext()) {
        QString path = it.next();
        QDir dir(path);

        FolderInfo folder;
        folder.path = rootDir.relativeFilePath(path);
        folder.name = dir.dirName();

        // 统计笔记数
        QSqlQuery query(m_database);
        query.prepare("SELECT COUNT(*) FROM notes WHERE folder = ? OR folder LIKE ?");
        query.addBindValue(folder.path);
        query.addBindValue(folder.path + "/%");
        if (query.exec() && query.next()) {
            folder.noteCount = query.value(0).toInt();
        }

        // 子文件夹
        QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        folder.subfolderCount = subdirs.size();
        folder.subfolders = subdirs;

        folders.append(folder);
    }

    return folders;
}

// 函数说明：读取 NotesLibrary 当前保存的状态或计算结果。
QVector<NotesLibrary::NoteInfo> NotesLibrary::getNotesByFolder(const QString &folderPath, bool recursive)
{
    QVector<NoteInfo> notes;
    if (!m_isOpen) return notes;

    QSqlQuery query(m_database);
    if (recursive) {
        query.prepare("SELECT file_path FROM notes WHERE folder = ? OR folder LIKE ? ORDER BY modified_time DESC");
        query.addBindValue(folderPath);
        query.addBindValue(folderPath + "/%");
    } else {
        query.prepare("SELECT file_path FROM notes WHERE folder = ? ORDER BY modified_time DESC");
        query.addBindValue(folderPath);
    }

    if (query.exec()) {
        while (query.next()) {
            notes.append(getNoteInfo(query.value(0).toString()));
        }
    }

    return notes;
}

// 函数说明：创建 NotesLibrary 需要的对象、记录或输出内容。
bool NotesLibrary::createFolder(const QString &folderPath)
{
    QString fullPath = m_libraryPath + "/" + folderPath;
    QDir dir;
    if (dir.mkpath(fullPath)) {
        m_watcher->addPath(fullPath);
        return true;
    }
    return false;
}

// 函数说明：实现 NotesLibrary::renameFolder 的核心逻辑，供当前模块调用。
bool NotesLibrary::renameFolder(const QString &oldPath, const QString &newPath)
{
    QString oldFullPath = m_libraryPath + "/" + oldPath;
    QString newFullPath = m_libraryPath + "/" + newPath;

    QDir dir;
    if (dir.rename(oldFullPath, newFullPath)) {
        // 更新数据库中的路径
        QSqlQuery query(m_database);
        query.prepare("UPDATE notes SET folder = REPLACE(folder, ?, ?) WHERE folder = ? OR folder LIKE ?");
        query.addBindValue(oldPath);
        query.addBindValue(newPath);
        query.addBindValue(oldPath);
        query.addBindValue(oldPath + "/%");
        query.exec();

        return true;
    }
    return false;
}

// 函数说明：删除 NotesLibrary 管理的指定数据或资源。
bool NotesLibrary::deleteFolder(const QString &folderPath, bool deleteFiles)
{
    QString fullPath = m_libraryPath + "/" + folderPath;
    QDir dir(fullPath);

    if (deleteFiles) {
        return dir.removeRecursively();
    } else {
        // 只删除空文件夹
        if (dir.isEmpty()) {
            return dir.rmdir(fullPath);
        }
    }
    return false;
}

// 函数说明：设置 NotesLibrary 的运行参数，并触发必要的界面或数据刷新。
void NotesLibrary::setFavorite(const QString &filePath, bool favorite)
{
    QString noteId = generateNoteId(filePath);

    QSqlQuery query(m_database);
    query.prepare("UPDATE notes SET is_favorite = ? WHERE id = ?");
    query.addBindValue(favorite ? 1 : 0);
    query.addBindValue(noteId);
    query.exec();

    if (m_noteCache.contains(filePath)) {
        m_noteCache[filePath].isFavorite = favorite;
    }
}

// 函数说明：设置 NotesLibrary 的运行参数，并触发必要的界面或数据刷新。
void NotesLibrary::setArchived(const QString &filePath, bool archived)
{
    QString noteId = generateNoteId(filePath);

    QSqlQuery query(m_database);
    query.prepare("UPDATE notes SET is_archived = ? WHERE id = ?");
    query.addBindValue(archived ? 1 : 0);
    query.addBindValue(noteId);
    query.exec();

    if (m_noteCache.contains(filePath)) {
        m_noteCache[filePath].isArchived = archived;
    }
}

// 函数说明：创建 NotesLibrary 需要的对象、记录或输出内容。
QString NotesLibrary::createNote(const QString &title, const QString &folder)
{
    QString folderPath = folder.isEmpty() ? m_libraryPath : m_libraryPath + "/" + folder;
    QDir dir(folderPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 生成文件名
    QString fileName = title.isEmpty() ? tr("新笔记") : title;
    fileName = fileName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");

    QString filePath = folderPath + "/" + fileName + ".md";
    int counter = 1;
    while (QFile::exists(filePath)) {
        filePath = folderPath + "/" + fileName + QString(" (%1).md").arg(counter++);
    }

    // 创建文件
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "# " << (title.isEmpty() ? tr("新笔记") : title) << "\n\n";
        file.close();

        // 索引新文件
        indexFile(filePath);
        m_watcher->addPath(filePath);

        return filePath;
    }

    return QString();
}

// 函数说明：删除 NotesLibrary 管理的指定数据或资源。
bool NotesLibrary::deleteNote(const QString &filePath)
{
    QFile file(filePath);
    if (file.remove()) {
        removeFromIndex(filePath);
        m_watcher->removePath(filePath);
        return true;
    }
    return false;
}

// 函数说明：实现 NotesLibrary::moveNote 的核心逻辑，供当前模块调用。
bool NotesLibrary::moveNote(const QString &filePath, const QString &newFolder)
{
    QFileInfo fileInfo(filePath);
    QString newPath = m_libraryPath + "/" + newFolder + "/" + fileInfo.fileName();

    // 确保目标目录存在
    QDir dir(m_libraryPath + "/" + newFolder);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    if (QFile::rename(filePath, newPath)) {
        removeFromIndex(filePath);
        indexFile(newPath);
        m_watcher->removePath(filePath);
        m_watcher->addPath(newPath);
        return true;
    }
    return false;
}

// 函数说明：实现 NotesLibrary::duplicateNote 的核心逻辑，供当前模块调用。
bool NotesLibrary::duplicateNote(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    QString baseName = fileInfo.baseName();
    QString suffix = fileInfo.suffix();
    QString dirPath = fileInfo.path();

    QString newPath = dirPath + "/" + baseName + tr(" (副本).") + suffix;
    int counter = 2;
    while (QFile::exists(newPath)) {
        newPath = dirPath + "/" + baseName + QString(tr(" (副本 %1).")).arg(counter++) + suffix;
    }

    if (QFile::copy(filePath, newPath)) {
        indexFile(newPath);
        m_watcher->addPath(newPath);
        return true;
    }
    return false;
}

// 函数说明：读取 NotesLibrary 当前保存的状态或计算结果。
int NotesLibrary::getTotalNoteCount()
{
    return indexedNoteCount();
}

// 函数说明：读取 NotesLibrary 当前保存的状态或计算结果。
int NotesLibrary::getTotalWordCount()
{
    if (!m_isOpen) return 0;

    QSqlQuery query(m_database);
    query.exec("SELECT SUM(word_count) FROM notes");
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

// 函数说明：读取 NotesLibrary 当前保存的状态或计算结果。
QMap<QString, int> NotesLibrary::getTagStatistics()
{
    QMap<QString, int> stats;
    if (!m_isOpen) return stats;

    QSqlQuery query(m_database);
    query.exec(R"(
        SELECT tag_name, COUNT(*) as count
        FROM note_tags
        GROUP BY tag_name
        ORDER BY count DESC
    )");

    while (query.next()) {
        stats[query.value(0).toString()] = query.value(1).toInt();
    }

    return stats;
}

// 函数说明：读取 NotesLibrary 当前保存的状态或计算结果。
QMap<QDate, int> NotesLibrary::getActivityStatistics(int days)
{
    QMap<QDate, int> stats;
    if (!m_isOpen) return stats;

    QDate startDate = QDate::currentDate().addDays(-days);

    QSqlQuery query(m_database);
    query.prepare(R"(
        SELECT DATE(modified_time) as date, COUNT(*) as count
        FROM notes
        WHERE modified_time >= ?
        GROUP BY DATE(modified_time)
    )");
    query.addBindValue(startDate.toString(Qt::ISODate));

    if (query.exec()) {
        while (query.next()) {
            QDate date = QDate::fromString(query.value(0).toString(), Qt::ISODate);
            stats[date] = query.value(1).toInt();
        }
    }

    return stats;
}

// 函数说明：响应 NotesLibrary 收到的信号或异步回调，并更新界面状态。
void NotesLibrary::onFileChanged(const QString &path)
{
    if (QFile::exists(path)) {
        updateIndex(path);
    } else {
        removeFromIndex(path);
    }
}

// 函数说明：响应 NotesLibrary 收到的信号或异步回调，并更新界面状态。
void NotesLibrary::onDirectoryChanged(const QString &path)
{
    // 重新扫描目录
    QDir dir(path);
    QStringList filters;
    filters << "*.md" << "*.markdown" << "*.txt";

    QStringList files = dir.entryList(filters, QDir::Files);
    for (const QString &file : files) {
        QString filePath = path + "/" + file;
        if (!m_noteCache.contains(filePath)) {
            indexFile(filePath);
            m_watcher->addPath(filePath);
        }
    }
}

// 函数说明：根据当前数据生成 NotesLibrary 需要的输出结果。
QString NotesLibrary::generateNoteId(const QString &filePath)
{
    return QString(QCryptographicHash::hash(
        filePath.toUtf8(), QCryptographicHash::Md5).toHex());
}

// 函数说明：读取 NotesLibrary 当前保存的状态或计算结果。
QString NotesLibrary::getRelativePath(const QString &filePath)
{
    if (filePath.startsWith(m_libraryPath)) {
        QString rel = filePath.mid(m_libraryPath.length());
        if (rel.startsWith('/')) rel = rel.mid(1);
        return rel;
    }
    return filePath;
}

// 函数说明：读取 NotesLibrary 当前保存的状态或计算结果。
QString NotesLibrary::getAbsolutePath(const QString &relativePath)
{
    if (relativePath.isEmpty()) return m_libraryPath;
    return m_libraryPath + "/" + relativePath;
}

// 函数说明：实现 NotesLibrary::importNotes 的核心逻辑，供当前模块调用。
bool NotesLibrary::importNotes(const QStringList &filePaths, const QString &folder)
{
    QString targetFolder = folder.isEmpty() ? m_libraryPath : m_libraryPath + "/" + folder;
    QDir dir(targetFolder);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    bool success = true;
    for (const QString &filePath : filePaths) {
        QFileInfo fileInfo(filePath);
        QString newPath = targetFolder + "/" + fileInfo.fileName();

        if (QFile::copy(filePath, newPath)) {
            indexFile(newPath);
            m_watcher->addPath(newPath);
        } else {
            success = false;
        }
    }

    return success;
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
bool NotesLibrary::exportNotes(const QStringList &filePaths, const QString &exportPath)
{
    QDir dir(exportPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    bool success = true;
    for (const QString &filePath : filePaths) {
        QFileInfo fileInfo(filePath);
        QString newPath = exportPath + "/" + fileInfo.fileName();

        if (!QFile::copy(filePath, newPath)) {
            success = false;
        }
    }

    return success;
}

