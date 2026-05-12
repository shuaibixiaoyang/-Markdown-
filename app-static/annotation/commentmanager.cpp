// 文件说明：app-static\annotation\commentmanager.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "commentmanager.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QTextBlock>
#include <QTextCursor>
#include <QScrollBar>
#include <QDebug>
//文档注释
CommentManager::CommentManager(QPlainTextEdit *editor, QObject *parent)
    : QObject(parent)
    , m_editor(editor)
    , m_highlightVisible(true)
    , m_updatingText(false)
{
    connect(m_editor, &QPlainTextEdit::cursorPositionChanged,
            this, &CommentManager::onCursorPositionChanged);
    connect(m_editor->document(), &QTextDocument::contentsChanged,
            this, &CommentManager::onTextChanged);
}

// 函数说明：销毁 CommentManager 对象，释放本模块持有的资源。
CommentManager::~CommentManager()
{
    if (m_config.autoSave && !m_documentPath.isEmpty()) {
        saveComments(commentFilePath());
    }
}

// 函数说明：向 CommentManager 管理的数据集合中添加一项内容。
QString CommentManager::addComment(int startPos, int endPos, const QString &text,
                                   HighlightType type)
{
    Comment comment;
    comment.id = generateId();
    comment.text = text;
    comment.author = m_config.defaultAuthor;
    comment.startPosition = startPos;
    comment.endPosition = endPos;
    comment.highlightType = type;
    comment.highlightColor = getColorForType(type);
    comment.status = CommentStatus::Open;
    comment.createTime = QDateTime::currentDateTime();
    comment.modifyTime = comment.createTime;

    // 获取行号
    QTextCursor cursor(m_editor->document());
    cursor.setPosition(startPos);
    comment.startLine = cursor.blockNumber();
    cursor.setPosition(endPos);
    comment.endLine = cursor.blockNumber();

    // 获取选中的文本
    cursor.setPosition(startPos);
    cursor.setPosition(endPos, QTextCursor::KeepAnchor);
    comment.selectedText = cursor.selectedText();

    m_comments[comment.id] = comment;

    if (m_highlightVisible) {
        applyHighlight(comment);
    }

    emit commentAdded(comment.id);
    return comment.id;
}

// 函数说明：向 CommentManager 管理的数据集合中添加一项内容。
QString CommentManager::addCommentAtSelection(const QString &text, HighlightType type)
{
    QTextCursor cursor = m_editor->textCursor();
    if (!cursor.hasSelection()) {
        return QString();
    }

    int startPos = cursor.selectionStart();
    int endPos = cursor.selectionEnd();

    return addComment(startPos, endPos, text, type);
}

// 函数说明：刷新 CommentManager 的内部状态，并同步到相关界面。
bool CommentManager::updateComment(const QString &commentId, const QString &newText)
{
    if (!m_comments.contains(commentId)) {
        return false;
    }

    m_comments[commentId].text = newText;
    m_comments[commentId].modifyTime = QDateTime::currentDateTime();

    emit commentUpdated(commentId);
    return true;
}

// 函数说明：删除 CommentManager 管理的指定数据或资源。
bool CommentManager::deleteComment(const QString &commentId)
{
    if (!m_comments.contains(commentId)) {
        return false;
    }

    // 删除所有回复
    Comment comment = m_comments[commentId];
    for (const QString &replyId : comment.replies) {
        m_comments.remove(replyId);
    }

    // 从父评论的回复列表中移除
    if (!comment.parentId.isEmpty() && m_comments.contains(comment.parentId)) {
        m_comments[comment.parentId].replies.removeAll(commentId);
    }

    removeHighlight(commentId);
    m_comments.remove(commentId);

    emit commentDeleted(commentId);
    return true;
}

// 函数说明：实现 CommentManager::resolveComment 的核心逻辑，供当前模块调用。
bool CommentManager::resolveComment(const QString &commentId)
{
    if (!m_comments.contains(commentId)) {
        return false;
    }

    m_comments[commentId].status = CommentStatus::Resolved;
    m_comments[commentId].modifyTime = QDateTime::currentDateTime();

    if (m_highlightVisible && !m_config.showResolvedComments) {
        removeHighlight(commentId);
    }

    emit commentResolved(commentId);
    return true;
}

// 函数说明：实现 CommentManager::reopenComment 的核心逻辑，供当前模块调用。
bool CommentManager::reopenComment(const QString &commentId)
{
    if (!m_comments.contains(commentId)) {
        return false;
    }

    m_comments[commentId].status = CommentStatus::Open;
    m_comments[commentId].modifyTime = QDateTime::currentDateTime();

    if (m_highlightVisible) {
        applyHighlight(m_comments[commentId]);
    }

    emit commentReopened(commentId);
    return true;
}

// 函数说明：实现 CommentManager::replyToComment 的核心逻辑，供当前模块调用。
QString CommentManager::replyToComment(const QString &parentId, const QString &text)
{
    if (!m_comments.contains(parentId)) {
        return QString();
    }

    Comment &parent = m_comments[parentId];

    Comment reply;
    reply.id = generateId();
    reply.text = text;
    reply.author = m_config.defaultAuthor;
    reply.parentId = parentId;
    reply.startPosition = parent.startPosition;
    reply.endPosition = parent.endPosition;
    reply.startLine = parent.startLine;
    reply.endLine = parent.endLine;
    reply.selectedText = parent.selectedText;
    reply.highlightType = parent.highlightType;
    reply.highlightColor = parent.highlightColor;
    reply.status = CommentStatus::Open;
    reply.createTime = QDateTime::currentDateTime();
    reply.modifyTime = reply.createTime;

    m_comments[reply.id] = reply;
    parent.replies.append(reply.id);

    emit commentAdded(reply.id);
    return reply.id;
}

// 函数说明：读取 CommentManager 当前保存的状态或计算结果。
CommentManager::Comment CommentManager::getComment(const QString &commentId) const
{
    return m_comments.value(commentId);
}

// 函数说明：读取 CommentManager 当前保存的状态或计算结果。
QVector<CommentManager::Comment> CommentManager::getAllComments() const
{
    QVector<Comment> result;
    for (const Comment &comment : m_comments) {
        if (comment.parentId.isEmpty()) {  // 只返回顶级评论
            result.append(comment);
        }
    }

    // 按位置排序
    std::sort(result.begin(), result.end(), [](const Comment &a, const Comment &b) {
        return a.startPosition < b.startPosition;
    });

    return result;
}

// 函数说明：读取 CommentManager 当前保存的状态或计算结果。
QVector<CommentManager::Comment> CommentManager::getOpenComments() const
{
    QVector<Comment> result;
    for (const Comment &comment : m_comments) {
        if (comment.status == CommentStatus::Open && comment.parentId.isEmpty()) {
            result.append(comment);
        }
    }
    return result;
}

// 函数说明：读取 CommentManager 当前保存的状态或计算结果。
QVector<CommentManager::Comment> CommentManager::getResolvedComments() const
{
    QVector<Comment> result;
    for (const Comment &comment : m_comments) {
        if (comment.status == CommentStatus::Resolved && comment.parentId.isEmpty()) {
            result.append(comment);
        }
    }
    return result;
}

// 函数说明：读取 CommentManager 当前保存的状态或计算结果。
QVector<CommentManager::Comment> CommentManager::getCommentsAtPosition(int position) const
{
    QVector<Comment> result;
    for (const Comment &comment : m_comments) {
        if (position >= comment.startPosition && position <= comment.endPosition) {
            result.append(comment);
        }
    }
    return result;
}

// 函数说明：读取 CommentManager 当前保存的状态或计算结果。
QVector<CommentManager::Comment> CommentManager::getCommentsInRange(int startPos, int endPos) const
{
    QVector<Comment> result;
    for (const Comment &comment : m_comments) {
        if (comment.startPosition <= endPos && comment.endPosition >= startPos) {
            result.append(comment);
        }
    }
    return result;
}

// 函数说明：读取 CommentManager 当前保存的状态或计算结果。
QVector<CommentManager::Comment> CommentManager::getCommentsByType(HighlightType type) const
{
    QVector<Comment> result;
    for (const Comment &comment : m_comments) {
        if (comment.highlightType == type) {
            result.append(comment);
        }
    }
    return result;
}

// 函数说明：读取 CommentManager 当前保存的状态或计算结果。
QVector<CommentManager::Comment> CommentManager::getReplies(const QString &parentId) const
{
    QVector<Comment> result;
    if (!m_comments.contains(parentId)) {
        return result;
    }

    const Comment &parent = m_comments[parentId];
    for (const QString &replyId : parent.replies) {
        if (m_comments.contains(replyId)) {
            result.append(m_comments[replyId]);
        }
    }

    // 按时间排序
    std::sort(result.begin(), result.end(), [](const Comment &a, const Comment &b) {
        return a.createTime < b.createTime;
    });

    return result;
}

// 函数说明：显示 CommentManager 管理的面板、对话框或提示信息。
void CommentManager::showHighlights()
{
    m_highlightVisible = true;
    updateHighlights();
}

// 函数说明：隐藏 CommentManager 管理的界面组件。
void CommentManager::hideHighlights()
{
    m_highlightVisible = false;

    // 清除所有高亮
    m_editor->setExtraSelections(QList<QTextEdit::ExtraSelection>());
    m_highlightSelections.clear();

    emit highlightsChanged();
}

// 函数说明：刷新 CommentManager 的内部状态，并同步到相关界面。
void CommentManager::updateHighlights()
{
    if (!m_highlightVisible) return;

    QList<QTextEdit::ExtraSelection> selections;

    for (const Comment &comment : m_comments) {
        // 跳过已解决的评论（如果配置为隐藏）
        if (!m_config.showResolvedComments && comment.status == CommentStatus::Resolved) {
            continue;
        }

        // 跳过回复（它们与父评论共享位置）
        if (!comment.parentId.isEmpty()) {
            continue;
        }

        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(comment.highlightColor);

        QTextCursor cursor(m_editor->document());
        cursor.setPosition(comment.startPosition);
        cursor.setPosition(qMin(comment.endPosition, m_editor->document()->characterCount() - 1),
                          QTextCursor::KeepAnchor);
        selection.cursor = cursor;

        selections.append(selection);
    }

    m_editor->setExtraSelections(selections);
    emit highlightsChanged();
}

// 函数说明：实现 CommentManager::goToNextComment 的核心逻辑，供当前模块调用。
void CommentManager::goToNextComment()
{
    QVector<Comment> comments = getAllComments();
    if (comments.isEmpty()) return;

    int currentPos = m_editor->textCursor().position();

    for (const Comment &comment : comments) {
        if (comment.startPosition > currentPos) {
            goToComment(comment.id);
            return;
        }
    }

    // 循环到第一个评论
    goToComment(comments.first().id);
}

// 函数说明：实现 CommentManager::goToPreviousComment 的核心逻辑，供当前模块调用。
void CommentManager::goToPreviousComment()
{
    QVector<Comment> comments = getAllComments();
    if (comments.isEmpty()) return;

    int currentPos = m_editor->textCursor().position();

    for (int i = comments.size() - 1; i >= 0; --i) {
        if (comments[i].startPosition < currentPos) {
            goToComment(comments[i].id);
            return;
        }
    }

    // 循环到最后一个评论
    goToComment(comments.last().id);
}

// 函数说明：实现 CommentManager::goToComment 的核心逻辑，供当前模块调用。
void CommentManager::goToComment(const QString &commentId)
{
    if (!m_comments.contains(commentId)) return;

    const Comment &comment = m_comments[commentId];

    QTextCursor cursor(m_editor->document());
    cursor.setPosition(comment.startPosition);
    cursor.setPosition(comment.endPosition, QTextCursor::KeepAnchor);

    m_editor->setTextCursor(cursor);
    m_editor->centerCursor();

    m_currentCommentId = commentId;
    emit currentCommentChanged(commentId);
}

// 函数说明：保存 CommentManager 当前状态，保证用户修改可以持久化。
bool CommentManager::saveComments(const QString &filePath)
{
    QString json = exportCommentsToJson();
    if (json.isEmpty()) {
        return true;  // 没有评论，不需要保存
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    file.write(json.toUtf8());
    file.close();
    return true;
}

// 函数说明：加载 CommentManager 需要的数据、配置或外部资源。
bool CommentManager::loadComments(const QString &filePath)
{
    QFile file(filePath);
    if (!file.exists()) {
        return true;  // 文件不存在不是错误
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QString json = QString::fromUtf8(file.readAll());
    file.close();

    return importCommentsFromJson(json);
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
QString CommentManager::exportCommentsToJson() const
{
    QJsonArray array;

    for (const Comment &comment : m_comments) {
        QJsonObject obj;
        obj["id"] = comment.id;
        obj["text"] = comment.text;
        obj["author"] = comment.author;
        obj["startPosition"] = comment.startPosition;
        obj["endPosition"] = comment.endPosition;
        obj["startLine"] = comment.startLine;
        obj["endLine"] = comment.endLine;
        obj["selectedText"] = comment.selectedText;
        obj["highlightType"] = static_cast<int>(comment.highlightType);
        obj["highlightColor"] = comment.highlightColor.name(QColor::HexArgb);
        obj["status"] = static_cast<int>(comment.status);
        obj["createTime"] = comment.createTime.toString(Qt::ISODate);
        obj["modifyTime"] = comment.modifyTime.toString(Qt::ISODate);
        obj["parentId"] = comment.parentId;

        QJsonArray repliesArray;
        for (const QString &replyId : comment.replies) {
            repliesArray.append(replyId);
        }
        obj["replies"] = repliesArray;

        array.append(obj);
    }

    QJsonDocument doc(array);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

// 函数说明：实现 CommentManager::importCommentsFromJson 的核心逻辑，供当前模块调用。
bool CommentManager::importCommentsFromJson(const QString &json)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError) {
        return false;
    }

    if (!doc.isArray()) {
        return false;
    }

    m_comments.clear();

    QJsonArray array = doc.array();
    for (const QJsonValue &value : array) {
        QJsonObject obj = value.toObject();

        Comment comment;
        comment.id = obj["id"].toString();
        comment.text = obj["text"].toString();
        comment.author = obj["author"].toString();
        comment.startPosition = obj["startPosition"].toInt();
        comment.endPosition = obj["endPosition"].toInt();
        comment.startLine = obj["startLine"].toInt();
        comment.endLine = obj["endLine"].toInt();
        comment.selectedText = obj["selectedText"].toString();
        comment.highlightType = static_cast<HighlightType>(obj["highlightType"].toInt());
        comment.highlightColor = QColor(obj["highlightColor"].toString());
        comment.status = static_cast<CommentStatus>(obj["status"].toInt());
        comment.createTime = QDateTime::fromString(obj["createTime"].toString(), Qt::ISODate);
        comment.modifyTime = QDateTime::fromString(obj["modifyTime"].toString(), Qt::ISODate);
        comment.parentId = obj["parentId"].toString();

        QJsonArray repliesArray = obj["replies"].toArray();
        for (const QJsonValue &replyVal : repliesArray) {
            comment.replies.append(replyVal.toString());
        }

        m_comments[comment.id] = comment;
    }

    updateHighlights();
    return true;
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
QString CommentManager::exportCommentsToHtml() const
{
    QString html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>文档评论</title>
    <style>
        body { font-family: sans-serif; max-width: 800px; margin: 0 auto; padding: 20px; }
        .comment { border: 1px solid #ddd; padding: 15px; margin: 10px 0; border-radius: 5px; }
        .comment.resolved { opacity: 0.6; }
        .comment-header { display: flex; justify-content: space-between; margin-bottom: 10px; }
        .author { font-weight: bold; }
        .time { color: #999; font-size: 12px; }
        .status { padding: 2px 8px; border-radius: 3px; font-size: 12px; }
        .status.open { background: #ffeeba; }
        .status.resolved { background: #c3e6cb; }
        .selected-text { background: #ffffcc; padding: 5px; margin: 10px 0; border-left: 3px solid #ffcc00; }
        .comment-text { margin-top: 10px; }
        .replies { margin-left: 20px; border-left: 2px solid #eee; padding-left: 15px; }
    </style>
</head>
<body>
    <h1>文档评论</h1>
)";

    QVector<Comment> comments = getAllComments();
    for (const Comment &comment : comments) {
        QString statusClass = (comment.status == CommentStatus::Resolved) ? "resolved" : "open";
        QString statusText = (comment.status == CommentStatus::Resolved) ? "已解决" : "未解决";

        html += QString(R"(
    <div class="comment %1">
        <div class="comment-header">
            <span class="author">%2</span>
            <span class="time">%3</span>
            <span class="status %1">%4</span>
        </div>
        <div class="selected-text">%5</div>
        <div class="comment-text">%6</div>
)").arg(statusClass)
   .arg(comment.author.isEmpty() ? "匿名" : comment.author)
   .arg(comment.createTime.toString("yyyy-MM-dd HH:mm"))
   .arg(statusText)
   .arg(comment.selectedText.toHtmlEscaped())
   .arg(comment.text.toHtmlEscaped());

        // 添加回复
        QVector<Comment> replies = getReplies(comment.id);
        if (!replies.isEmpty()) {
            html += "<div class=\"replies\">";
            for (const Comment &reply : replies) {
                html += QString(R"(
            <div class="comment">
                <div class="comment-header">
                    <span class="author">%1</span>
                    <span class="time">%2</span>
                </div>
                <div class="comment-text">%3</div>
            </div>
)").arg(reply.author.isEmpty() ? "匿名" : reply.author)
   .arg(reply.createTime.toString("yyyy-MM-dd HH:mm"))
   .arg(reply.text.toHtmlEscaped());
            }
            html += "</div>";
        }

        html += "</div>";
    }

    html += "</body></html>";
    return html;
}

// 函数说明：实现 CommentManager::totalComments 的核心逻辑，供当前模块调用。
int CommentManager::totalComments() const
{
    return m_comments.size();
}

// 函数说明：打开 CommentManager 对应的文件、资源或功能入口。
int CommentManager::openCommentCount() const
{
    int count = 0;
    for (const Comment &comment : m_comments) {
        if (comment.status == CommentStatus::Open) {
            count++;
        }
    }
    return count;
}

// 函数说明：实现 CommentManager::resolvedCommentCount 的核心逻辑，供当前模块调用。
int CommentManager::resolvedCommentCount() const
{
    int count = 0;
    for (const Comment &comment : m_comments) {
        if (comment.status == CommentStatus::Resolved) {
            count++;
        }
    }
    return count;
}

// 函数说明：设置 CommentManager 的运行参数，并触发必要的界面或数据刷新。
void CommentManager::setConfig(const Config &config)
{
    m_config = config;
    updateHighlights();
}

// 函数说明：设置 CommentManager 的运行参数，并触发必要的界面或数据刷新。
void CommentManager::setAuthor(const QString &author)
{
    m_config.defaultAuthor = author;
}

// 函数说明：设置 CommentManager 的运行参数，并触发必要的界面或数据刷新。
void CommentManager::setDocumentPath(const QString &path)
{
    // 保存旧文档的评论
    if (!m_documentPath.isEmpty() && m_config.autoSave) {
        saveComments(commentFilePath());
    }

    m_documentPath = path;
    m_comments.clear();

    // 加载新文档的评论
    if (!path.isEmpty()) {
        loadComments(commentFilePath());
    }
}

// 函数说明：响应 CommentManager 收到的信号或异步回调，并更新界面状态。
void CommentManager::onTextInserted(int position, int charsAdded)
{
    for (auto it = m_comments.begin(); it != m_comments.end(); ++it) {
        Comment &comment = it.value();

        if (comment.startPosition >= position) {
            comment.startPosition += charsAdded;
            comment.endPosition += charsAdded;
        } else if (comment.endPosition >= position) {
            comment.endPosition += charsAdded;
        }
    }

    updateHighlights();
}

// 函数说明：响应 CommentManager 收到的信号或异步回调，并更新界面状态。
void CommentManager::onTextRemoved(int position, int charsRemoved)
{
    QStringList toRemove;

    for (auto it = m_comments.begin(); it != m_comments.end(); ++it) {
        Comment &comment = it.value();

        if (comment.startPosition >= position + charsRemoved) {
            // 评论在删除区域之后
            comment.startPosition -= charsRemoved;
            comment.endPosition -= charsRemoved;
        } else if (comment.endPosition <= position) {
            // 评论在删除区域之前，不受影响
        } else if (comment.startPosition >= position && comment.endPosition <= position + charsRemoved) {
            // 评论完全在删除区域内，标记为删除
            toRemove.append(it.key());
        } else {
            // 评论部分在删除区域内
            if (comment.startPosition < position) {
                comment.endPosition = qMax(position, comment.endPosition - charsRemoved);
            } else {
                comment.startPosition = position;
                comment.endPosition = qMax(position, comment.endPosition - charsRemoved);
            }
        }
    }

    for (const QString &id : toRemove) {
        m_comments.remove(id);
        emit commentDeleted(id);
    }

    updateHighlights();
}

// 函数说明：响应 CommentManager 收到的信号或异步回调，并更新界面状态。
void CommentManager::onCursorPositionChanged()
{
    int pos = m_editor->textCursor().position();
    QVector<Comment> comments = getCommentsAtPosition(pos);

    if (!comments.isEmpty() && comments.first().id != m_currentCommentId) {
        m_currentCommentId = comments.first().id;
        emit currentCommentChanged(m_currentCommentId);
    } else if (comments.isEmpty() && !m_currentCommentId.isEmpty()) {
        m_currentCommentId.clear();
        emit currentCommentChanged(QString());
    }
}

// 函数说明：响应 CommentManager 收到的信号或异步回调，并更新界面状态。
void CommentManager::onTextChanged()
{
    // 由外部调用 onTextInserted/onTextRemoved 来处理位置同步
}

// 函数说明：根据当前数据生成 CommentManager 需要的输出结果。
QString CommentManager::generateId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(12);
}

// 函数说明：应用 CommentManager 当前配置，让编辑器或预览立即生效。
void CommentManager::applyHighlight(const Comment &comment)
{
    // 高亮通过 updateHighlights() 统一处理
    Q_UNUSED(comment)
}

// 函数说明：从 CommentManager 管理的数据集合中移除指定内容。
void CommentManager::removeHighlight(const QString &commentId)
{
    m_highlightSelections.remove(commentId);
    updateHighlights();
}

// 函数说明：刷新 CommentManager 的内部状态，并同步到相关界面。
void CommentManager::updateCommentPositions()
{
    for (auto it = m_comments.begin(); it != m_comments.end(); ++it) {
        Comment &comment = it.value();

        QTextCursor cursor(m_editor->document());
        cursor.setPosition(qMin(comment.startPosition, m_editor->document()->characterCount() - 1));
        comment.startLine = cursor.blockNumber();

        cursor.setPosition(qMin(comment.endPosition, m_editor->document()->characterCount() - 1));
        comment.endLine = cursor.blockNumber();
    }
}

// 函数说明：读取 CommentManager 当前保存的状态或计算结果。
QColor CommentManager::getColorForType(HighlightType type) const
{
    return m_config.typeColors.value(type, m_config.defaultHighlightColor);
}

// 函数说明：实现 CommentManager::commentFilePath 的核心逻辑，供当前模块调用。
QString CommentManager::commentFilePath() const
{
    if (m_documentPath.isEmpty()) {
        return QString();
    }

    QFileInfo info(m_documentPath);
    return info.path() + "/." + info.fileName() + ".comments.json";
}

