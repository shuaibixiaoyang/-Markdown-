// 文件说明：app-static\writing\bookmarkmanager.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "bookmarkmanager.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTextBlock>
#include <QUuid>
#include <QFileInfo>
#include <QDir>

// 函数说明：构造 BookmarkManager 对象，初始化本模块需要的状态、界面和资源。
BookmarkManager::BookmarkManager(QObject *parent)
    : QObject(parent)
    , m_editor(nullptr)
    , m_autoSave(true)
    , m_currentIndex(-1)
{
    // 设置默认颜色
    m_defaultColors[BookmarkType::Normal] = QColor(66, 133, 244);      // 蓝色
    m_defaultColors[BookmarkType::Important] = QColor(234, 67, 53);    // 红色
    m_defaultColors[BookmarkType::Todo] = QColor(251, 188, 5);         // 黄色
    m_defaultColors[BookmarkType::Question] = QColor(52, 168, 83);     // 绿色
    m_defaultColors[BookmarkType::Reference] = QColor(154, 160, 166);  // 灰色
}

// 函数说明：销毁 BookmarkManager 对象，释放本模块持有的资源。
BookmarkManager::~BookmarkManager()
{
    if (m_autoSave && !m_documentPath.isEmpty()) {
        saveBookmarks();
    }
}

// 函数说明：设置 BookmarkManager 的运行参数，并触发必要的界面或数据刷新。
void BookmarkManager::setEditor(QPlainTextEdit *editor)
{
    if (m_editor) {
        disconnect(m_editor, nullptr, this, nullptr);
    }

    m_editor = editor;

    if (m_editor) {
        connect(m_editor, &QPlainTextEdit::textChanged,
                this, &BookmarkManager::onTextChanged);
        connect(m_editor, &QPlainTextEdit::cursorPositionChanged,
                this, &BookmarkManager::onCursorPositionChanged);
    }
}

// 函数说明：设置 BookmarkManager 的运行参数，并触发必要的界面或数据刷新。
void BookmarkManager::setDocumentPath(const QString &path)
{
    if (m_documentPath != path) {
        // 保存旧文档的书签
        if (m_autoSave && !m_documentPath.isEmpty()) {
            saveBookmarks();
        }

        m_documentPath = path;
        m_bookmarks.clear();
        m_currentIndex = -1;

        // 加载新文档的书签
        if (!path.isEmpty()) {
            loadBookmarks();
        }
    }
}

// 函数说明：向 BookmarkManager 管理的数据集合中添加一项内容。
QString BookmarkManager::addBookmark(int lineNumber, const QString &name, BookmarkType type)
{
    // 检查是否已有书签
    if (hasBookmarkAtLine(lineNumber)) {
        return QString();
    }

    Bookmark bookmark;
    bookmark.id = generateId();
    bookmark.lineNumber = lineNumber;
    bookmark.type = type;
    bookmark.color = getTypeColor(type);
    bookmark.createdTime = QDateTime::currentDateTime();
    bookmark.documentPath = m_documentPath;
    bookmark.linePreview = getLineText(lineNumber);

    // 设置名称
    if (name.isEmpty()) {
        bookmark.name = tr("书签 %1").arg(m_bookmarks.size() + 1);
    } else {
        bookmark.name = name;
    }

    // 计算位置
    if (m_editor) {
        QTextBlock block = m_editor->document()->findBlockByLineNumber(lineNumber - 1);
        if (block.isValid()) {
            bookmark.position = block.position();
        }
    }

    // 按行号排序插入
    int insertIndex = 0;
    for (int i = 0; i < m_bookmarks.size(); ++i) {
        if (m_bookmarks[i].lineNumber > lineNumber) {
            insertIndex = i;
            break;
        }
        insertIndex = i + 1;
    }
    m_bookmarks.insert(insertIndex, bookmark);

    if (m_autoSave) {
        saveBookmarks();
    }

    emit bookmarkAdded(bookmark.id);
    return bookmark.id;
}

// 函数说明：向 BookmarkManager 管理的数据集合中添加一项内容。
QString BookmarkManager::addBookmarkAtCursor(const QString &name, BookmarkType type)
{
    if (!m_editor) return QString();

    QTextCursor cursor = m_editor->textCursor();
    int lineNumber = cursor.blockNumber() + 1;

    return addBookmark(lineNumber, name, type);
}

// 函数说明：从 BookmarkManager 管理的数据集合中移除指定内容。
bool BookmarkManager::removeBookmark(const QString &id)
{
    for (int i = 0; i < m_bookmarks.size(); ++i) {
        if (m_bookmarks[i].id == id) {
            m_bookmarks.removeAt(i);

            if (m_currentIndex >= m_bookmarks.size()) {
                m_currentIndex = m_bookmarks.size() - 1;
            }

            if (m_autoSave) {
                saveBookmarks();
            }

            emit bookmarkRemoved(id);
            return true;
        }
    }
    return false;
}

// 函数说明：从 BookmarkManager 管理的数据集合中移除指定内容。
bool BookmarkManager::removeBookmarkAtLine(int lineNumber)
{
    for (int i = 0; i < m_bookmarks.size(); ++i) {
        if (m_bookmarks[i].lineNumber == lineNumber) {
            QString id = m_bookmarks[i].id;
            m_bookmarks.removeAt(i);

            if (m_currentIndex >= m_bookmarks.size()) {
                m_currentIndex = m_bookmarks.size() - 1;
            }

            if (m_autoSave) {
                saveBookmarks();
            }

            emit bookmarkRemoved(id);
            return true;
        }
    }
    return false;
}

// 函数说明：清空 BookmarkManager 保存的临时状态或缓存数据。
void BookmarkManager::clearAllBookmarks()
{
    m_bookmarks.clear();
    m_currentIndex = -1;

    if (m_autoSave) {
        saveBookmarks();
    }

    emit bookmarksCleared();
}

// 函数说明：刷新 BookmarkManager 的内部状态，并同步到相关界面。
bool BookmarkManager::updateBookmark(const QString &id, const QString &name,
                                      const QString &description, BookmarkType type)
{
    for (int i = 0; i < m_bookmarks.size(); ++i) {
        if (m_bookmarks[i].id == id) {
            m_bookmarks[i].name = name;
            m_bookmarks[i].description = description;
            m_bookmarks[i].type = type;
            m_bookmarks[i].color = getTypeColor(type);

            if (m_autoSave) {
                saveBookmarks();
            }

            emit bookmarkUpdated(id);
            return true;
        }
    }
    return false;
}

// 函数说明：读取 BookmarkManager 当前保存的状态或计算结果。
BookmarkManager::Bookmark BookmarkManager::getBookmark(const QString &id) const
{
    for (const auto &bookmark : m_bookmarks) {
        if (bookmark.id == id) {
            return bookmark;
        }
    }
    return Bookmark();
}

// 函数说明：读取 BookmarkManager 当前保存的状态或计算结果。
QVector<BookmarkManager::Bookmark> BookmarkManager::getAllBookmarks() const
{
    return m_bookmarks;
}

// 函数说明：读取 BookmarkManager 当前保存的状态或计算结果。
QVector<BookmarkManager::Bookmark> BookmarkManager::getBookmarksByType(BookmarkType type) const
{
    QVector<Bookmark> result;
    for (const auto &bookmark : m_bookmarks) {
        if (bookmark.type == type) {
            result.append(bookmark);
        }
    }
    return result;
}

// 函数说明：检查 BookmarkManager 是否具备对应的数据或能力。
bool BookmarkManager::hasBookmarkAtLine(int lineNumber) const
{
    for (const auto &bookmark : m_bookmarks) {
        if (bookmark.lineNumber == lineNumber) {
            return true;
        }
    }
    return false;
}

// 函数说明：读取 BookmarkManager 当前保存的状态或计算结果。
BookmarkManager::Bookmark BookmarkManager::getBookmarkAtLine(int lineNumber) const
{
    for (const auto &bookmark : m_bookmarks) {
        if (bookmark.lineNumber == lineNumber) {
            return bookmark;
        }
    }
    return Bookmark();
}

// 函数说明：实现 BookmarkManager::gotoBookmark 的核心逻辑，供当前模块调用。
void BookmarkManager::gotoBookmark(const QString &id)
{
    if (!m_editor) return;

    for (int i = 0; i < m_bookmarks.size(); ++i) {
        if (m_bookmarks[i].id == id) {
            m_currentIndex = i;
            const Bookmark &bookmark = m_bookmarks[i];

            QTextBlock block = m_editor->document()->findBlockByLineNumber(bookmark.lineNumber - 1);
            if (block.isValid()) {
                QTextCursor cursor(block);
                m_editor->setTextCursor(cursor);
                m_editor->centerCursor();
                m_editor->setFocus();
            }

            emit bookmarkNavigated(id);
            return;
        }
    }
}

// 函数说明：实现 BookmarkManager::gotoNextBookmark 的核心逻辑，供当前模块调用。
void BookmarkManager::gotoNextBookmark()
{
    if (m_bookmarks.isEmpty() || !m_editor) return;

    m_currentIndex++;
    if (m_currentIndex >= m_bookmarks.size()) {
        m_currentIndex = 0;
    }

    gotoBookmark(m_bookmarks[m_currentIndex].id);
}

// 函数说明：实现 BookmarkManager::gotoPreviousBookmark 的核心逻辑，供当前模块调用。
void BookmarkManager::gotoPreviousBookmark()
{
    if (m_bookmarks.isEmpty() || !m_editor) return;

    m_currentIndex--;
    if (m_currentIndex < 0) {
        m_currentIndex = m_bookmarks.size() - 1;
    }

    gotoBookmark(m_bookmarks[m_currentIndex].id);
}

// 函数说明：实现 BookmarkManager::gotoFirstBookmark 的核心逻辑，供当前模块调用。
void BookmarkManager::gotoFirstBookmark()
{
    if (m_bookmarks.isEmpty()) return;

    m_currentIndex = 0;
    gotoBookmark(m_bookmarks[m_currentIndex].id);
}

// 函数说明：实现 BookmarkManager::gotoLastBookmark 的核心逻辑，供当前模块调用。
void BookmarkManager::gotoLastBookmark()
{
    if (m_bookmarks.isEmpty()) return;

    m_currentIndex = m_bookmarks.size() - 1;
    gotoBookmark(m_bookmarks[m_currentIndex].id);
}

// 函数说明：保存 BookmarkManager 当前状态，保证用户修改可以持久化。
bool BookmarkManager::saveBookmarks(const QString &filePath)
{
    QString path = filePath.isEmpty() ? getBookmarksFilePath() : filePath;
    if (path.isEmpty()) return false;

    QJsonArray bookmarksArray;
    for (const auto &bookmark : m_bookmarks) {
        QJsonObject obj;
        obj["id"] = bookmark.id;
        obj["name"] = bookmark.name;
        obj["description"] = bookmark.description;
        obj["lineNumber"] = bookmark.lineNumber;
        obj["position"] = bookmark.position;
        obj["linePreview"] = bookmark.linePreview;
        obj["type"] = typeToString(bookmark.type);
        obj["color"] = bookmark.color.name();
        obj["createdTime"] = bookmark.createdTime.toString(Qt::ISODate);
        bookmarksArray.append(obj);
    }

    QJsonObject root;
    root["version"] = 1;
    root["documentPath"] = m_documentPath;
    root["bookmarks"] = bookmarksArray;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QJsonDocument doc(root);
    file.write(doc.toJson());
    file.close();

    return true;
}

// 函数说明：加载 BookmarkManager 需要的数据、配置或外部资源。
bool BookmarkManager::loadBookmarks(const QString &filePath)
{
    QString path = filePath.isEmpty() ? getBookmarksFilePath() : filePath;
    if (path.isEmpty()) return false;

    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) return false;

    QJsonObject root = doc.object();
    QJsonArray bookmarksArray = root["bookmarks"].toArray();

    m_bookmarks.clear();
    for (const QJsonValue &value : bookmarksArray) {
        QJsonObject obj = value.toObject();

        Bookmark bookmark;
        bookmark.id = obj["id"].toString();
        bookmark.name = obj["name"].toString();
        bookmark.description = obj["description"].toString();
        bookmark.lineNumber = obj["lineNumber"].toInt();
        bookmark.position = obj["position"].toInt();
        bookmark.linePreview = obj["linePreview"].toString();
        bookmark.type = stringToType(obj["type"].toString());
        bookmark.color = QColor(obj["color"].toString());
        bookmark.createdTime = QDateTime::fromString(obj["createdTime"].toString(), Qt::ISODate);
        bookmark.documentPath = m_documentPath;

        m_bookmarks.append(bookmark);
    }

    m_currentIndex = m_bookmarks.isEmpty() ? -1 : 0;

    emit bookmarksLoaded();
    return true;
}

// 函数说明：设置 BookmarkManager 的运行参数，并触发必要的界面或数据刷新。
void BookmarkManager::setAutoSave(bool enable)
{
    m_autoSave = enable;
}

// 函数说明：设置 BookmarkManager 的运行参数，并触发必要的界面或数据刷新。
void BookmarkManager::setDefaultColor(BookmarkType type, const QColor &color)
{
    m_defaultColors[type] = color;
}

// 函数说明：读取 BookmarkManager 当前保存的状态或计算结果。
QColor BookmarkManager::getDefaultColor(BookmarkType type) const
{
    return m_defaultColors.value(type, QColor(66, 133, 244));
}

// 函数说明：处理云同步操作，把本地文档状态同步到配置的远端。
void BookmarkManager::syncWithDocument()
{
    updateBookmarkPositions();
}

// 函数说明：响应 BookmarkManager 收到的信号或异步回调，并更新界面状态。
void BookmarkManager::onTextChanged()
{
    // 文本变化时更新书签位置
    updateBookmarkPositions();
}

// 函数说明：响应 BookmarkManager 收到的信号或异步回调，并更新界面状态。
void BookmarkManager::onCursorPositionChanged()
{
    // 更新当前索引以匹配光标位置
    if (!m_editor || m_bookmarks.isEmpty()) return;

    int currentLine = m_editor->textCursor().blockNumber() + 1;

    for (int i = 0; i < m_bookmarks.size(); ++i) {
        if (m_bookmarks[i].lineNumber == currentLine) {
            m_currentIndex = i;
            return;
        }
    }
}

// 函数说明：根据当前数据生成 BookmarkManager 需要的输出结果。
QString BookmarkManager::generateId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

// 函数说明：读取 BookmarkManager 当前保存的状态或计算结果。
QString BookmarkManager::getLineText(int lineNumber)
{
    if (!m_editor) return QString();

    QTextBlock block = m_editor->document()->findBlockByLineNumber(lineNumber - 1);
    if (block.isValid()) {
        QString text = block.text().trimmed();
        // 截断过长的预览
        if (text.length() > 50) {
            text = text.left(47) + "...";
        }
        return text;
    }
    return QString();
}

// 函数说明：刷新 BookmarkManager 的内部状态，并同步到相关界面。
void BookmarkManager::updateBookmarkPositions()
{
    if (!m_editor) return;

    bool changed = false;
    for (int i = 0; i < m_bookmarks.size(); ++i) {
        QTextBlock block = m_editor->document()->findBlockByLineNumber(m_bookmarks[i].lineNumber - 1);
        if (block.isValid()) {
            int newPosition = block.position();
            if (m_bookmarks[i].position != newPosition) {
                m_bookmarks[i].position = newPosition;
                m_bookmarks[i].linePreview = getLineText(m_bookmarks[i].lineNumber);
                changed = true;
            }
        }
    }

    if (changed && m_autoSave) {
        saveBookmarks();
    }
}

// 函数说明：读取 BookmarkManager 当前保存的状态或计算结果。
QString BookmarkManager::getBookmarksFilePath() const
{
    if (m_documentPath.isEmpty()) return QString();

    QFileInfo info(m_documentPath);
    QString dir = info.absolutePath();
    QString baseName = info.completeBaseName();

    // 创建 .bookmarks 目录
    QString bookmarksDir = dir + "/.bookmarks";
    QDir().mkpath(bookmarksDir);

    return bookmarksDir + "/" + baseName + ".json";
}

// 函数说明：读取 BookmarkManager 当前保存的状态或计算结果。
QColor BookmarkManager::getTypeColor(BookmarkType type) const
{
    return m_defaultColors.value(type, QColor(66, 133, 244));
}

// 函数说明：实现 BookmarkManager::typeToString 的核心逻辑，供当前模块调用。
QString BookmarkManager::typeToString(BookmarkType type) const
{
    switch (type) {
        case BookmarkType::Normal: return "normal";
        case BookmarkType::Important: return "important";
        case BookmarkType::Todo: return "todo";
        case BookmarkType::Question: return "question";
        case BookmarkType::Reference: return "reference";
        default: return "normal";
    }
}

// 函数说明：实现 BookmarkManager::stringToType 的核心逻辑，供当前模块调用。
BookmarkManager::BookmarkType BookmarkManager::stringToType(const QString &str) const
{
    if (str == "important") return BookmarkType::Important;
    if (str == "todo") return BookmarkType::Todo;
    if (str == "question") return BookmarkType::Question;
    if (str == "reference") return BookmarkType::Reference;
    return BookmarkType::Normal;
}

