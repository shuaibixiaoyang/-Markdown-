// 文件说明：app-static\writing\bookmarkmanager.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef BOOKMARKMANAGER_H
#define BOOKMARKMANAGER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QPlainTextEdit>
#include <QColor>

/**
 * @brief 书签管理器
 *
 * 功能：
 * - 添加/删除书签
 * - 书签命名和分类
 * - 快速跳转到书签位置
 * - 书签持久化保存
 * - 书签列表显示
 */
class BookmarkManager : public QObject
{
    Q_OBJECT

public:
    // 书签类型
    enum class BookmarkType {
        Normal,         // 普通书签
        Important,      // 重要
        Todo,           // 待办
        Question,       // 疑问
        Reference       // 参考
    };
    Q_ENUM(BookmarkType)

    // 书签结构
    struct Bookmark {
        QString id;             // 唯一标识
        QString name;           // 书签名称
        QString description;    // 描述
        int lineNumber;         // 行号
        int position;           // 文本位置
        QString linePreview;    // 该行预览文本
        BookmarkType type;      // 类型
        QColor color;           // 颜色
        QDateTime createdTime;  // 创建时间
        QString documentPath;   // 文档路径

        Bookmark()
            : lineNumber(0), position(0)
            , type(BookmarkType::Normal)
            , color(Qt::blue) {}
    };

    explicit BookmarkManager(QObject *parent = nullptr);
    ~BookmarkManager();

    // 设置编辑器
    void setEditor(QPlainTextEdit *editor);
    void setDocumentPath(const QString &path);

    // 书签操作
    QString addBookmark(int lineNumber, const QString &name = QString(),
                        BookmarkType type = BookmarkType::Normal);
    QString addBookmarkAtCursor(const QString &name = QString(),
                                 BookmarkType type = BookmarkType::Normal);
    bool removeBookmark(const QString &id);
    bool removeBookmarkAtLine(int lineNumber);
    void clearAllBookmarks();
    bool updateBookmark(const QString &id, const QString &name,
                        const QString &description, BookmarkType type);

    // 查询
    Bookmark getBookmark(const QString &id) const;
    QVector<Bookmark> getAllBookmarks() const;
    QVector<Bookmark> getBookmarksByType(BookmarkType type) const;
    bool hasBookmarkAtLine(int lineNumber) const;
    Bookmark getBookmarkAtLine(int lineNumber) const;
    int bookmarkCount() const { return m_bookmarks.size(); }

    // 导航
    void gotoBookmark(const QString &id);
    void gotoNextBookmark();
    void gotoPreviousBookmark();
    void gotoFirstBookmark();
    void gotoLastBookmark();

    // 持久化
    bool saveBookmarks(const QString &filePath = QString());
    bool loadBookmarks(const QString &filePath = QString());
    void setAutoSave(bool enable);

    // 配置
    void setDefaultColor(BookmarkType type, const QColor &color);
    QColor getDefaultColor(BookmarkType type) const;

    // 书签同步（当文本变化时）
    void syncWithDocument();

signals:
    void bookmarkAdded(const QString &id);
    void bookmarkRemoved(const QString &id);
    void bookmarkUpdated(const QString &id);
    void bookmarksCleared();
    void bookmarkNavigated(const QString &id);
    void bookmarksLoaded();

private slots:
    void onTextChanged();
    void onCursorPositionChanged();

private:
    QString generateId();
    QString getLineText(int lineNumber);
    void updateBookmarkPositions();
    QString getBookmarksFilePath() const;
    QColor getTypeColor(BookmarkType type) const;
    QString typeToString(BookmarkType type) const;
    BookmarkType stringToType(const QString &str) const;

    QPlainTextEdit *m_editor;
    QString m_documentPath;
    QVector<Bookmark> m_bookmarks;
    bool m_autoSave;

    // 默认颜色
    QMap<BookmarkType, QColor> m_defaultColors;

    // 当前书签索引（用于导航）
    int m_currentIndex;
};

#endif // BOOKMARKMANAGER_H

