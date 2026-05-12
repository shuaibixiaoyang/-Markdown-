// 文件说明：app-static\revision\revisiontracker.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef REVISIONTRACKER_H
#define REVISIONTRACKER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QMap>
#include <QVector>
#include <QByteArray>

/**
 * @brief 修订追踪管理器
 *
 * 功能：
 * - 自动保存文档修订版本
 * - 版本对比（差异显示）
 * - 版本恢复
 * - 版本标签和注释
 * - 版本压缩存储
 * - 自动清理旧版本
 */
class RevisionTracker : public QObject
{
    Q_OBJECT

public:
    // 修订信息
    struct Revision {
        QString id;                 // 修订 ID
        QString documentPath;       // 文档路径
        QString content;            // 文档内容
        QByteArray compressedContent; // 压缩内容
        QString description;        // 修订描述
        QString author;             // 作者
        QDateTime timestamp;        // 时间戳
        int wordCount;              // 字数
        int charCount;              // 字符数
        int lineCount;              // 行数
        bool isAutoSave;            // 是否自动保存
        bool isCompressed;          // 是否已压缩
        QString parentId;           // 父修订 ID
        QStringList tags;           // 标签

        Revision()
            : wordCount(0)
            , charCount(0)
            , lineCount(0)
            , isAutoSave(false)
            , isCompressed(false)
        {}
    };

    // 差异块
    struct DiffBlock {
        enum Type {
            Equal,      // 相同
            Insert,     // 插入
            Delete,     // 删除
            Replace     // 替换
        };

        Type type;
        QString oldText;        // 旧文本
        QString newText;        // 新文本
        int oldStartLine;       // 旧起始行
        int oldEndLine;         // 旧结束行
        int newStartLine;       // 新起始行
        int newEndLine;         // 新结束行

        DiffBlock()
            : type(Equal)
            , oldStartLine(0)
            , oldEndLine(0)
            , newStartLine(0)
            , newEndLine(0)
        {}
    };

    // 差异统计
    struct DiffStats {
        int insertedLines;
        int deletedLines;
        int modifiedLines;
        int unchangedLines;

        DiffStats()
            : insertedLines(0)
            , deletedLines(0)
            , modifiedLines(0)
            , unchangedLines(0)
        {}
    };

    // 配置
    struct Config {
        bool autoSaveEnabled;       // 启用自动保存
        int autoSaveIntervalSecs;   // 自动保存间隔（秒）
        int maxRevisions;           // 最大修订数量
        int maxAgeDays;             // 最大保留天数
        bool compressOldRevisions;  // 压缩旧修订
        int compressAfterDays;      // 多少天后压缩
        QString revisionDirectory;  // 修订存储目录
        QString defaultAuthor;      // 默认作者

        Config()
            : autoSaveEnabled(true)
            , autoSaveIntervalSecs(300)  // 5分钟
            , maxRevisions(100)
            , maxAgeDays(30)
            , compressOldRevisions(true)
            , compressAfterDays(7)
        {}
    };

    explicit RevisionTracker(QObject *parent = nullptr);
    ~RevisionTracker();

    // 修订管理
    QString createRevision(const QString &documentPath,
                          const QString &content,
                          const QString &description = QString(),
                          bool isAutoSave = false);
    bool deleteRevision(const QString &revisionId);
    bool updateRevisionDescription(const QString &revisionId, const QString &description);
    bool addRevisionTag(const QString &revisionId, const QString &tag);
    bool removeRevisionTag(const QString &revisionId, const QString &tag);

    // 获取修订
    Revision getRevision(const QString &revisionId) const;
    QString getRevisionContent(const QString &revisionId) const;
    QVector<Revision> getRevisions(const QString &documentPath) const;
    QVector<Revision> getRecentRevisions(const QString &documentPath, int count = 10) const;
    QVector<Revision> getRevisionsByTag(const QString &documentPath, const QString &tag) const;
    Revision getLatestRevision(const QString &documentPath) const;

    // 差异比较
    QVector<DiffBlock> compareRevisions(const QString &oldRevisionId,
                                         const QString &newRevisionId) const;
    QVector<DiffBlock> compareWithCurrent(const QString &revisionId,
                                           const QString &currentContent) const;
    QString generateDiffHtml(const QVector<DiffBlock> &diffs) const;
    QString generateUnifiedDiff(const QString &oldContent,
                                const QString &newContent,
                                const QString &oldLabel = QString(),
                                const QString &newLabel = QString()) const;
    DiffStats getDiffStats(const QVector<DiffBlock> &diffs) const;

    // 恢复
    QString restoreRevision(const QString &revisionId) const;

    // 清理
    void cleanupOldRevisions(const QString &documentPath);
    void cleanupAllOldRevisions();
    void compressOldRevisions(const QString &documentPath);

    // 导入导出
    bool exportRevisions(const QString &documentPath, const QString &exportPath);
    bool importRevisions(const QString &importPath);

    // 统计
    int revisionCount(const QString &documentPath) const;
    qint64 totalStorageSize(const QString &documentPath) const;

    // 配置
    void setConfig(const Config &config);
    Config config() const { return m_config; }

    // 持久化
    bool saveRevisions(const QString &documentPath);
    bool loadRevisions(const QString &documentPath);

signals:
    void revisionCreated(const QString &revisionId);
    void revisionDeleted(const QString &revisionId);
    void revisionRestored(const QString &revisionId);
    void revisionsLoaded(const QString &documentPath, int count);
    void cleanupCompleted(int removedCount);

private:
    QString generateId();
    QString revisionFilePath(const QString &documentPath) const;
    QByteArray compressContent(const QString &content) const;
    QString decompressContent(const QByteArray &compressed) const;
    int countWords(const QString &text) const;
    int countLines(const QString &text) const;

    // 差异算法
    QVector<DiffBlock> computeDiff(const QString &oldText, const QString &newText) const;
    QStringList splitLines(const QString &text) const;

    QMap<QString, QVector<Revision>> m_revisions;  // documentPath -> revisions
    Config m_config;
};

#endif // REVISIONTRACKER_H

