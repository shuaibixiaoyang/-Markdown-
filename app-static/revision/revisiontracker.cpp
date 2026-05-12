// 文件说明：app-static\revision\revisiontracker.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "revisiontracker.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QDebug>

// 函数说明：构造 RevisionTracker 对象，初始化本模块需要的状态、界面和资源。
RevisionTracker::RevisionTracker(QObject *parent)
    : QObject(parent)
{
    m_config.revisionDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/revisions";
    QDir().mkpath(m_config.revisionDirectory);
}

// 函数说明：销毁 RevisionTracker 对象，释放本模块持有的资源。
RevisionTracker::~RevisionTracker()
{
    // 保存所有修订
    for (const QString &docPath : m_revisions.keys()) {
        saveRevisions(docPath);
    }
}

// 函数说明：创建 RevisionTracker 需要的对象、记录或输出内容。
QString RevisionTracker::createRevision(const QString &documentPath,
                                        const QString &content,
                                        const QString &description,
                                        bool isAutoSave)
{
    Revision revision;
    revision.id = generateId();
    revision.documentPath = documentPath;
    revision.content = content;
    revision.description = description;
    revision.author = m_config.defaultAuthor;
    revision.timestamp = QDateTime::currentDateTime();
    revision.wordCount = countWords(content);
    revision.charCount = content.length();
    revision.lineCount = countLines(content);
    revision.isAutoSave = isAutoSave;
    revision.isCompressed = false;

    // 设置父修订
    if (m_revisions.contains(documentPath) && !m_revisions[documentPath].isEmpty()) {
        revision.parentId = m_revisions[documentPath].last().id;
    }

    m_revisions[documentPath].append(revision);

    // 检查是否需要清理
    if (m_revisions[documentPath].size() > m_config.maxRevisions) {
        cleanupOldRevisions(documentPath);
    }

    emit revisionCreated(revision.id);
    return revision.id;
}

// 函数说明：删除 RevisionTracker 管理的指定数据或资源。
bool RevisionTracker::deleteRevision(const QString &revisionId)
{
    for (auto it = m_revisions.begin(); it != m_revisions.end(); ++it) {
        for (int i = 0; i < it.value().size(); ++i) {
            if (it.value()[i].id == revisionId) {
                it.value().removeAt(i);
                emit revisionDeleted(revisionId);
                return true;
            }
        }
    }
    return false;
}

// 函数说明：刷新 RevisionTracker 的内部状态，并同步到相关界面。
bool RevisionTracker::updateRevisionDescription(const QString &revisionId, const QString &description)
{
    for (auto it = m_revisions.begin(); it != m_revisions.end(); ++it) {
        for (int i = 0; i < it.value().size(); ++i) {
            if (it.value()[i].id == revisionId) {
                it.value()[i].description = description;
                return true;
            }
        }
    }
    return false;
}

// 函数说明：向 RevisionTracker 管理的数据集合中添加一项内容。
bool RevisionTracker::addRevisionTag(const QString &revisionId, const QString &tag)
{
    for (auto it = m_revisions.begin(); it != m_revisions.end(); ++it) {
        for (int i = 0; i < it.value().size(); ++i) {
            if (it.value()[i].id == revisionId) {
                if (!it.value()[i].tags.contains(tag)) {
                    it.value()[i].tags.append(tag);
                }
                return true;
            }
        }
    }
    return false;
}

// 函数说明：从 RevisionTracker 管理的数据集合中移除指定内容。
bool RevisionTracker::removeRevisionTag(const QString &revisionId, const QString &tag)
{
    for (auto it = m_revisions.begin(); it != m_revisions.end(); ++it) {
        for (int i = 0; i < it.value().size(); ++i) {
            if (it.value()[i].id == revisionId) {
                it.value()[i].tags.removeAll(tag);
                return true;
            }
        }
    }
    return false;
}

// 函数说明：读取 RevisionTracker 当前保存的状态或计算结果。
RevisionTracker::Revision RevisionTracker::getRevision(const QString &revisionId) const
{
    for (const auto &revisions : m_revisions) {
        for (const Revision &revision : revisions) {
            if (revision.id == revisionId) {
                return revision;
            }
        }
    }
    return Revision();
}

// 函数说明：读取 RevisionTracker 当前保存的状态或计算结果。
QString RevisionTracker::getRevisionContent(const QString &revisionId) const
{
    Revision revision = getRevision(revisionId);
    if (revision.id.isEmpty()) {
        return QString();
    }

    if (revision.isCompressed) {
        return decompressContent(revision.compressedContent);
    }
    return revision.content;
}

// 函数说明：读取 RevisionTracker 当前保存的状态或计算结果。
QVector<RevisionTracker::Revision> RevisionTracker::getRevisions(const QString &documentPath) const
{
    return m_revisions.value(documentPath);
}

// 函数说明：读取 RevisionTracker 当前保存的状态或计算结果。
QVector<RevisionTracker::Revision> RevisionTracker::getRecentRevisions(const QString &documentPath, int count) const
{
    QVector<Revision> revisions = m_revisions.value(documentPath);
    if (revisions.size() <= count) {
        return revisions;
    }

    return revisions.mid(revisions.size() - count);
}

// 函数说明：读取 RevisionTracker 当前保存的状态或计算结果。
QVector<RevisionTracker::Revision> RevisionTracker::getRevisionsByTag(const QString &documentPath, const QString &tag) const
{
    QVector<Revision> result;
    QVector<Revision> revisions = m_revisions.value(documentPath);

    for (const Revision &revision : revisions) {
        if (revision.tags.contains(tag)) {
            result.append(revision);
        }
    }

    return result;
}

// 函数说明：读取 RevisionTracker 当前保存的状态或计算结果。
RevisionTracker::Revision RevisionTracker::getLatestRevision(const QString &documentPath) const
{
    QVector<Revision> revisions = m_revisions.value(documentPath);
    if (revisions.isEmpty()) {
        return Revision();
    }
    return revisions.last();
}

// 函数说明：实现 RevisionTracker::compareRevisions 的核心逻辑，供当前模块调用。
QVector<RevisionTracker::DiffBlock> RevisionTracker::compareRevisions(const QString &oldRevisionId,
                                                                       const QString &newRevisionId) const
{
    QString oldContent = getRevisionContent(oldRevisionId);
    QString newContent = getRevisionContent(newRevisionId);

    return computeDiff(oldContent, newContent);
}

// 函数说明：实现 RevisionTracker::compareWithCurrent 的核心逻辑，供当前模块调用。
QVector<RevisionTracker::DiffBlock> RevisionTracker::compareWithCurrent(const QString &revisionId,
                                                                         const QString &currentContent) const
{
    QString oldContent = getRevisionContent(revisionId);
    return computeDiff(oldContent, currentContent);
}

// 函数说明：根据当前数据生成 RevisionTracker 需要的输出结果。
QString RevisionTracker::generateDiffHtml(const QVector<DiffBlock> &diffs) const
{
    QString html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>版本比较</title>
    <style>
        body { font-family: monospace; margin: 20px; }
        .diff-container { display: flex; }
        .diff-side { flex: 1; padding: 10px; overflow-x: auto; }
        .diff-old { background: #fff8f8; }
        .diff-new { background: #f8fff8; }
        .diff-line { padding: 2px 5px; white-space: pre-wrap; word-wrap: break-word; }
        .diff-line-number { color: #999; min-width: 40px; display: inline-block; }
        .diff-equal { background: #fff; }
        .diff-insert { background: #e6ffec; }
        .diff-delete { background: #ffebe9; }
        .diff-replace-old { background: #ffd7d5; }
        .diff-replace-new { background: #ccffd8; }
        .diff-stats { margin-bottom: 20px; padding: 10px; background: #f5f5f5; border-radius: 5px; }
        .diff-stat-insert { color: #22863a; }
        .diff-stat-delete { color: #cb2431; }
    </style>
</head>
<body>
)";

    // 统计信息
    DiffStats stats = getDiffStats(diffs);
    html += QString(R"(
    <div class="diff-stats">
        <span class="diff-stat-insert">+%1 行插入</span> |
        <span class="diff-stat-delete">-%2 行删除</span> |
        %3 行修改
    </div>
)").arg(stats.insertedLines).arg(stats.deletedLines).arg(stats.modifiedLines);

    html += "<div class=\"diff-container\">";

    // 旧版本
    html += "<div class=\"diff-side diff-old\"><h3>旧版本</h3>";
    for (const DiffBlock &block : diffs) {
        QString cssClass;
        switch (block.type) {
            case DiffBlock::Equal: cssClass = "diff-equal"; break;
            case DiffBlock::Delete: cssClass = "diff-delete"; break;
            case DiffBlock::Replace: cssClass = "diff-replace-old"; break;
            case DiffBlock::Insert: continue;  // 旧版本没有插入
        }

        QString text = block.type == DiffBlock::Replace ? block.oldText : block.oldText;
        if (!text.isEmpty()) {
            html += QString("<div class=\"diff-line %1\"><span class=\"diff-line-number\">%2</span>%3</div>")
                    .arg(cssClass)
                    .arg(block.oldStartLine)
                    .arg(text.toHtmlEscaped().replace("\n", "<br>"));
        }
    }
    html += "</div>";

    // 新版本
    html += "<div class=\"diff-side diff-new\"><h3>新版本</h3>";
    for (const DiffBlock &block : diffs) {
        QString cssClass;
        switch (block.type) {
            case DiffBlock::Equal: cssClass = "diff-equal"; break;
            case DiffBlock::Insert: cssClass = "diff-insert"; break;
            case DiffBlock::Replace: cssClass = "diff-replace-new"; break;
            case DiffBlock::Delete: continue;  // 新版本没有删除
        }

        QString text = block.type == DiffBlock::Replace ? block.newText : block.newText;
        if (block.type == DiffBlock::Equal) {
            text = block.oldText;  // Equal 块使用 oldText
        }
        if (!text.isEmpty()) {
            html += QString("<div class=\"diff-line %1\"><span class=\"diff-line-number\">%2</span>%3</div>")
                    .arg(cssClass)
                    .arg(block.newStartLine)
                    .arg(text.toHtmlEscaped().replace("\n", "<br>"));
        }
    }
    html += "</div>";

    html += "</div></body></html>";
    return html;
}

// 函数说明：根据当前数据生成 RevisionTracker 需要的输出结果。
QString RevisionTracker::generateUnifiedDiff(const QString &oldContent,
                                              const QString &newContent,
                                              const QString &oldLabel,
                                              const QString &newLabel) const
{
    QStringList oldLines = splitLines(oldContent);
    QStringList newLines = splitLines(newContent);

    QString diff;
    diff += QString("--- %1\n").arg(oldLabel.isEmpty() ? "old" : oldLabel);
    diff += QString("+++ %1\n").arg(newLabel.isEmpty() ? "new" : newLabel);

    QVector<DiffBlock> diffs = computeDiff(oldContent, newContent);

    for (const DiffBlock &block : diffs) {
        switch (block.type) {
            case DiffBlock::Equal:
                for (const QString &line : block.oldText.split('\n')) {
                    diff += QString(" %1\n").arg(line);
                }
                break;
            case DiffBlock::Delete:
                for (const QString &line : block.oldText.split('\n')) {
                    diff += QString("-%1\n").arg(line);
                }
                break;
            case DiffBlock::Insert:
                for (const QString &line : block.newText.split('\n')) {
                    diff += QString("+%1\n").arg(line);
                }
                break;
            case DiffBlock::Replace:
                for (const QString &line : block.oldText.split('\n')) {
                    diff += QString("-%1\n").arg(line);
                }
                for (const QString &line : block.newText.split('\n')) {
                    diff += QString("+%1\n").arg(line);
                }
                break;
        }
    }

    return diff;
}

// 函数说明：读取 RevisionTracker 当前保存的状态或计算结果。
RevisionTracker::DiffStats RevisionTracker::getDiffStats(const QVector<DiffBlock> &diffs) const
{
    DiffStats stats;

    for (const DiffBlock &block : diffs) {
        int lines;
        switch (block.type) {
            case DiffBlock::Equal:
                lines = block.oldText.count('\n') + 1;
                stats.unchangedLines += lines;
                break;
            case DiffBlock::Insert:
                lines = block.newText.count('\n') + 1;
                stats.insertedLines += lines;
                break;
            case DiffBlock::Delete:
                lines = block.oldText.count('\n') + 1;
                stats.deletedLines += lines;
                break;
            case DiffBlock::Replace:
                lines = qMax(block.oldText.count('\n'), block.newText.count('\n')) + 1;
                stats.modifiedLines += lines;
                break;
        }
    }

    return stats;
}

// 函数说明：实现 RevisionTracker::restoreRevision 的核心逻辑，供当前模块调用。
QString RevisionTracker::restoreRevision(const QString &revisionId) const
{
    QString content = getRevisionContent(revisionId);
    if (!content.isEmpty()) {
        emit const_cast<RevisionTracker*>(this)->revisionRestored(revisionId);
    }
    return content;
}

// 函数说明：实现 RevisionTracker::cleanupOldRevisions 的核心逻辑，供当前模块调用。
void RevisionTracker::cleanupOldRevisions(const QString &documentPath)
{
    if (!m_revisions.contains(documentPath)) return;

    QVector<Revision> &revisions = m_revisions[documentPath];
    int removedCount = 0;
    QDateTime cutoffDate = QDateTime::currentDateTime().addDays(-m_config.maxAgeDays);

    // 移除超过最大数量的旧修订
    while (revisions.size() > m_config.maxRevisions) {
        // 保留有标签的修订
        int removeIndex = -1;
        for (int i = 0; i < revisions.size() - 1; ++i) {  // 不删除最新的
            if (revisions[i].tags.isEmpty() && revisions[i].isAutoSave) {
                removeIndex = i;
                break;
            }
        }

        if (removeIndex < 0) {
            removeIndex = 0;  // 删除最旧的
        }

        revisions.removeAt(removeIndex);
        removedCount++;
    }

    // 移除超过最大天数的修订
    for (int i = revisions.size() - 2; i >= 0; --i) {  // 不检查最新的
        if (revisions[i].timestamp < cutoffDate &&
            revisions[i].tags.isEmpty() &&
            revisions[i].isAutoSave) {
            revisions.removeAt(i);
            removedCount++;
        }
    }

    if (removedCount > 0) {
        emit cleanupCompleted(removedCount);
    }
}

// 函数说明：实现 RevisionTracker::cleanupAllOldRevisions 的核心逻辑，供当前模块调用。
void RevisionTracker::cleanupAllOldRevisions()
{
    for (const QString &docPath : m_revisions.keys()) {
        cleanupOldRevisions(docPath);
    }
}

// 函数说明：实现 RevisionTracker::compressOldRevisions 的核心逻辑，供当前模块调用。
void RevisionTracker::compressOldRevisions(const QString &documentPath)
{
    if (!m_revisions.contains(documentPath)) return;

    QVector<Revision> &revisions = m_revisions[documentPath];
    QDateTime cutoffDate = QDateTime::currentDateTime().addDays(-m_config.compressAfterDays);

    for (int i = 0; i < revisions.size(); ++i) {
        if (!revisions[i].isCompressed &&
            revisions[i].timestamp < cutoffDate) {
            revisions[i].compressedContent = compressContent(revisions[i].content);
            revisions[i].content.clear();
            revisions[i].isCompressed = true;
        }
    }
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
bool RevisionTracker::exportRevisions(const QString &documentPath, const QString &exportPath)
{
    QVector<Revision> revisions = m_revisions.value(documentPath);

    QJsonArray array;
    for (const Revision &rev : revisions) {
        QJsonObject obj;
        obj["id"] = rev.id;
        obj["description"] = rev.description;
        obj["author"] = rev.author;
        obj["timestamp"] = rev.timestamp.toString(Qt::ISODate);
        obj["wordCount"] = rev.wordCount;
        obj["charCount"] = rev.charCount;
        obj["lineCount"] = rev.lineCount;
        obj["isAutoSave"] = rev.isAutoSave;
        obj["content"] = rev.isCompressed ? decompressContent(rev.compressedContent) : rev.content;

        QJsonArray tagsArray;
        for (const QString &tag : rev.tags) {
            tagsArray.append(tag);
        }
        obj["tags"] = tagsArray;

        array.append(obj);
    }

    QJsonDocument doc(array);

    QFile file(exportPath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(doc.toJson());
    file.close();
    return true;
}

// 函数说明：实现 RevisionTracker::importRevisions 的核心逻辑，供当前模块调用。
bool RevisionTracker::importRevisions(const QString &importPath)
{
    QFile file(importPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();

    if (error.error != QJsonParseError::NoError) {
        return false;
    }

    // 暂不实现导入逻辑
    return true;
}

// 函数说明：实现 RevisionTracker::revisionCount 的核心逻辑，供当前模块调用。
int RevisionTracker::revisionCount(const QString &documentPath) const
{
    return m_revisions.value(documentPath).size();
}

// 函数说明：实现 RevisionTracker::totalStorageSize 的核心逻辑，供当前模块调用。
qint64 RevisionTracker::totalStorageSize(const QString &documentPath) const
{
    qint64 size = 0;
    QVector<Revision> revisions = m_revisions.value(documentPath);

    for (const Revision &rev : revisions) {
        if (rev.isCompressed) {
            size += rev.compressedContent.size();
        } else {
            size += rev.content.toUtf8().size();
        }
    }

    return size;
}

// 函数说明：设置 RevisionTracker 的运行参数，并触发必要的界面或数据刷新。
void RevisionTracker::setConfig(const Config &config)
{
    m_config = config;
    QDir().mkpath(m_config.revisionDirectory);
}

// 函数说明：保存 RevisionTracker 当前状态，保证用户修改可以持久化。
bool RevisionTracker::saveRevisions(const QString &documentPath)
{
    QString filePath = revisionFilePath(documentPath);

    QVector<Revision> revisions = m_revisions.value(documentPath);

    QJsonArray array;
    for (const Revision &rev : revisions) {
        QJsonObject obj;
        obj["id"] = rev.id;
        obj["description"] = rev.description;
        obj["author"] = rev.author;
        obj["timestamp"] = rev.timestamp.toString(Qt::ISODate);
        obj["wordCount"] = rev.wordCount;
        obj["charCount"] = rev.charCount;
        obj["lineCount"] = rev.lineCount;
        obj["isAutoSave"] = rev.isAutoSave;
        obj["isCompressed"] = rev.isCompressed;
        obj["parentId"] = rev.parentId;

        if (rev.isCompressed) {
            obj["compressedContent"] = QString::fromLatin1(rev.compressedContent.toBase64());
        } else {
            obj["content"] = rev.content;
        }

        QJsonArray tagsArray;
        for (const QString &tag : rev.tags) {
            tagsArray.append(tag);
        }
        obj["tags"] = tagsArray;

        array.append(obj);
    }

    QJsonDocument doc(array);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Compact));
    file.close();
    return true;
}

// 函数说明：加载 RevisionTracker 需要的数据、配置或外部资源。
bool RevisionTracker::loadRevisions(const QString &documentPath)
{
    QString filePath = revisionFilePath(documentPath);

    QFile file(filePath);
    if (!file.exists()) {
        return true;  // 没有修订不是错误
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();

    if (error.error != QJsonParseError::NoError) {
        return false;
    }

    QVector<Revision> revisions;
    QJsonArray array = doc.array();

    for (const QJsonValue &value : array) {
        QJsonObject obj = value.toObject();

        Revision rev;
        rev.id = obj["id"].toString();
        rev.documentPath = documentPath;
        rev.description = obj["description"].toString();
        rev.author = obj["author"].toString();
        rev.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
        rev.wordCount = obj["wordCount"].toInt();
        rev.charCount = obj["charCount"].toInt();
        rev.lineCount = obj["lineCount"].toInt();
        rev.isAutoSave = obj["isAutoSave"].toBool();
        rev.isCompressed = obj["isCompressed"].toBool();
        rev.parentId = obj["parentId"].toString();

        if (rev.isCompressed) {
            rev.compressedContent = QByteArray::fromBase64(obj["compressedContent"].toString().toLatin1());
        } else {
            rev.content = obj["content"].toString();
        }

        QJsonArray tagsArray = obj["tags"].toArray();
        for (const QJsonValue &tagVal : tagsArray) {
            rev.tags.append(tagVal.toString());
        }

        revisions.append(rev);
    }

    m_revisions[documentPath] = revisions;
    emit revisionsLoaded(documentPath, revisions.size());

    return true;
}

// 函数说明：根据当前数据生成 RevisionTracker 需要的输出结果。
QString RevisionTracker::generateId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(12);
}

// 函数说明：实现 RevisionTracker::revisionFilePath 的核心逻辑，供当前模块调用。
QString RevisionTracker::revisionFilePath(const QString &documentPath) const
{
    QString hash = QString::fromLatin1(
        QCryptographicHash::hash(documentPath.toUtf8(), QCryptographicHash::Md5).toHex().left(16));
    return m_config.revisionDirectory + "/" + hash + ".revisions.json";
}

// 函数说明：实现 RevisionTracker::compressContent 的核心逻辑，供当前模块调用。
QByteArray RevisionTracker::compressContent(const QString &content) const
{
    return qCompress(content.toUtf8(), 9);
}

// 函数说明：实现 RevisionTracker::decompressContent 的核心逻辑，供当前模块调用。
QString RevisionTracker::decompressContent(const QByteArray &compressed) const
{
    return QString::fromUtf8(qUncompress(compressed));
}

// 函数说明：实现 RevisionTracker::countWords 的核心逻辑，供当前模块调用。
int RevisionTracker::countWords(const QString &text) const
{
    // 简单的字数统计
    int count = 0;
    bool inWord = false;

    for (const QChar &ch : text) {
        if (ch.isLetterOrNumber() || ch.unicode() > 0x4E00) {
            if (!inWord) {
                count++;
                inWord = true;
            }
            // 中文字符单独计数
            if (ch.unicode() >= 0x4E00 && ch.unicode() <= 0x9FFF) {
                count++;
                inWord = false;
            }
        } else {
            inWord = false;
        }
    }

    return count;
}

// 函数说明：实现 RevisionTracker::countLines 的核心逻辑，供当前模块调用。
int RevisionTracker::countLines(const QString &text) const
{
    return text.count('\n') + 1;
}

// 函数说明：实现 RevisionTracker::computeDiff 的核心逻辑，供当前模块调用。
QVector<RevisionTracker::DiffBlock> RevisionTracker::computeDiff(const QString &oldText,
                                                                  const QString &newText) const
{
    QVector<DiffBlock> result;

    QStringList oldLines = splitLines(oldText);
    QStringList newLines = splitLines(newText);

    // 简化的 LCS（最长公共子序列）差异算法
    int oldLen = oldLines.size();
    int newLen = newLines.size();

    // 使用动态规划找 LCS
    QVector<QVector<int>> dp(oldLen + 1, QVector<int>(newLen + 1, 0));

    for (int i = 1; i <= oldLen; ++i) {
        for (int j = 1; j <= newLen; ++j) {
            if (oldLines[i-1] == newLines[j-1]) {
                dp[i][j] = dp[i-1][j-1] + 1;
            } else {
                dp[i][j] = qMax(dp[i-1][j], dp[i][j-1]);
            }
        }
    }

    // 回溯生成差异
    int i = oldLen, j = newLen;
    QVector<QPair<int, int>> lcs;  // (oldIndex, newIndex)

    while (i > 0 && j > 0) {
        if (oldLines[i-1] == newLines[j-1]) {
            lcs.prepend(qMakePair(i-1, j-1));
            i--; j--;
        } else if (dp[i-1][j] > dp[i][j-1]) {
            i--;
        } else {
            j--;
        }
    }

    // 生成差异块
    int oldIndex = 0, newIndex = 0;
    for (const auto &pair : lcs) {
        // 处理差异
        if (pair.first > oldIndex || pair.second > newIndex) {
            DiffBlock block;
            if (pair.first > oldIndex && pair.second > newIndex) {
                block.type = DiffBlock::Replace;
                block.oldText = oldLines.mid(oldIndex, pair.first - oldIndex).join('\n');
                block.newText = newLines.mid(newIndex, pair.second - newIndex).join('\n');
            } else if (pair.first > oldIndex) {
                block.type = DiffBlock::Delete;
                block.oldText = oldLines.mid(oldIndex, pair.first - oldIndex).join('\n');
            } else {
                block.type = DiffBlock::Insert;
                block.newText = newLines.mid(newIndex, pair.second - newIndex).join('\n');
            }
            block.oldStartLine = oldIndex + 1;
            block.oldEndLine = pair.first;
            block.newStartLine = newIndex + 1;
            block.newEndLine = pair.second;
            result.append(block);
        }

        // 相同行
        DiffBlock equalBlock;
        equalBlock.type = DiffBlock::Equal;
        equalBlock.oldText = oldLines[pair.first];
        equalBlock.oldStartLine = pair.first + 1;
        equalBlock.oldEndLine = pair.first + 1;
        equalBlock.newStartLine = pair.second + 1;
        equalBlock.newEndLine = pair.second + 1;
        result.append(equalBlock);

        oldIndex = pair.first + 1;
        newIndex = pair.second + 1;
    }

    // 处理尾部差异
    if (oldIndex < oldLen || newIndex < newLen) {
        DiffBlock block;
        if (oldIndex < oldLen && newIndex < newLen) {
            block.type = DiffBlock::Replace;
            block.oldText = oldLines.mid(oldIndex).join('\n');
            block.newText = newLines.mid(newIndex).join('\n');
        } else if (oldIndex < oldLen) {
            block.type = DiffBlock::Delete;
            block.oldText = oldLines.mid(oldIndex).join('\n');
        } else {
            block.type = DiffBlock::Insert;
            block.newText = newLines.mid(newIndex).join('\n');
        }
        block.oldStartLine = oldIndex + 1;
        block.oldEndLine = oldLen;
        block.newStartLine = newIndex + 1;
        block.newEndLine = newLen;
        result.append(block);
    }

    return result;
}

// 函数说明：实现 RevisionTracker::splitLines 的核心逻辑，供当前模块调用。
QStringList RevisionTracker::splitLines(const QString &text) const
{
    return text.split('\n');
}

