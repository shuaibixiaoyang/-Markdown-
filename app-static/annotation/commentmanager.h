// 文件说明：app-static\annotation\commentmanager.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef COMMENTMANAGER_H
#define COMMENTMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QMap>
#include <QVector>
#include <QColor>
#include <QPlainTextEdit>
#include <QTextCharFormat>

/**
 * @brief 评论和批注管理器
 *
 * 功能：
 * - 在文档中添加评论
 * - 高亮标记文本
 * - 评论线程（回复）
 * - 评论状态管理（已解决/未解决）
 * - 评论导出和导入
 * - 与文本位置同步
 */
class CommentManager : public QObject
{
    Q_OBJECT

public:
    // 评论状态
    enum class CommentStatus {
        Open,           // 未解决
        Resolved,       // 已解决
        WontFix,        // 不处理
        Pending         // 待定
    };
    Q_ENUM(CommentStatus)

    // 高亮类型
    enum class HighlightType {
        Comment,        // 普通评论
        Important,      // 重要
        Question,       // 问题
        Todo,           // 待办
        Note,           // 笔记
        Custom          // 自定义
    };
    Q_ENUM(HighlightType)

    // 评论信息
    struct Comment {
        QString id;                 // 评论 ID
        QString text;               // 评论内容
        QString author;             // 作者
        int startPosition;          // 起始位置（字符索引）
        int endPosition;            // 结束位置
        int startLine;              // 起始行号
        int endLine;                // 结束行号
        QString selectedText;       // 选中的文本
        HighlightType highlightType;// 高亮类型
        QColor highlightColor;      // 高亮颜色
        CommentStatus status;       // 状态
        QDateTime createTime;       // 创建时间
        QDateTime modifyTime;       // 修改时间
        QString parentId;           // 父评论 ID（用于回复）
        QStringList replies;        // 回复 ID 列表

        Comment()
            : startPosition(0)
            , endPosition(0)
            , startLine(0)
            , endLine(0)
            , highlightType(HighlightType::Comment)
            , highlightColor(QColor(255, 255, 0, 100))
            , status(CommentStatus::Open)
        {}
    };

    // 配置
    struct Config {
        QString defaultAuthor;
        QColor defaultHighlightColor;
        bool showResolvedComments;
        bool autoSave;
        QMap<HighlightType, QColor> typeColors;

        Config()
            : defaultHighlightColor(QColor(255, 255, 0, 100))
            , showResolvedComments(true)
            , autoSave(true)
        {
            typeColors[HighlightType::Comment] = QColor(255, 255, 0, 100);
            typeColors[HighlightType::Important] = QColor(255, 100, 100, 100);
            typeColors[HighlightType::Question] = QColor(100, 200, 255, 100);
            typeColors[HighlightType::Todo] = QColor(100, 255, 100, 100);
            typeColors[HighlightType::Note] = QColor(200, 200, 200, 100);
            typeColors[HighlightType::Custom] = QColor(200, 150, 255, 100);
        }
    };

    explicit CommentManager(QPlainTextEdit *editor, QObject *parent = nullptr);
    ~CommentManager();

    // 评论管理
    QString addComment(int startPos, int endPos, const QString &text,
                      HighlightType type = HighlightType::Comment);
    QString addCommentAtSelection(const QString &text,
                                  HighlightType type = HighlightType::Comment);
    bool updateComment(const QString &commentId, const QString &newText);
    bool deleteComment(const QString &commentId);
    bool resolveComment(const QString &commentId);
    bool reopenComment(const QString &commentId);

    // 回复
    QString replyToComment(const QString &parentId, const QString &text);

    // 获取评论
    Comment getComment(const QString &commentId) const;
    QVector<Comment> getAllComments() const;
    QVector<Comment> getOpenComments() const;
    QVector<Comment> getResolvedComments() const;
    QVector<Comment> getCommentsAtPosition(int position) const;
    QVector<Comment> getCommentsInRange(int startPos, int endPos) const;
    QVector<Comment> getCommentsByType(HighlightType type) const;
    QVector<Comment> getReplies(const QString &parentId) const;

    // 高亮显示
    void showHighlights();
    void hideHighlights();
    void updateHighlights();
    bool isHighlightVisible() const { return m_highlightVisible; }

    // 导航
    void goToNextComment();
    void goToPreviousComment();
    void goToComment(const QString &commentId);

    // 导入导出
    bool saveComments(const QString &filePath);
    bool loadComments(const QString &filePath);
    QString exportCommentsToJson() const;
    bool importCommentsFromJson(const QString &json);
    QString exportCommentsToHtml() const;

    // 统计
    int totalComments() const;
    int openCommentCount() const;
    int resolvedCommentCount() const;

    // 配置
    void setConfig(const Config &config);
    Config config() const { return m_config; }
    void setAuthor(const QString &author);
    QString author() const { return m_config.defaultAuthor; }

    // 文档关联
    void setDocumentPath(const QString &path);
    QString documentPath() const { return m_documentPath; }

    // 位置同步（当文档内容变化时调用）
    void onTextInserted(int position, int charsAdded);
    void onTextRemoved(int position, int charsRemoved);

signals:
    void commentAdded(const QString &commentId);
    void commentUpdated(const QString &commentId);
    void commentDeleted(const QString &commentId);
    void commentResolved(const QString &commentId);
    void commentReopened(const QString &commentId);
    void highlightsChanged();
    void currentCommentChanged(const QString &commentId);

private slots:
    void onCursorPositionChanged();
    void onTextChanged();

private:
    QString generateId();
    void applyHighlight(const Comment &comment);
    void removeHighlight(const QString &commentId);
    void updateCommentPositions();
    QColor getColorForType(HighlightType type) const;
    QString commentFilePath() const;

    QPlainTextEdit *m_editor;
    QMap<QString, Comment> m_comments;
    Config m_config;
    QString m_documentPath;
    QString m_currentCommentId;
    bool m_highlightVisible;
    bool m_updatingText;

    // 用于追踪高亮
    QMap<QString, QList<QTextEdit::ExtraSelection>> m_highlightSelections;
};

#endif // COMMENTMANAGER_H

