// 文件说明：app-static\vcs\gitmanager.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef GITMANAGER_H
#define GITMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QProcess>
#include <QVector>

/**
 * @brief Git 版本控制管理器
 *
 * 功能：
 * - Git 仓库初始化和管理
 * - 提交历史查看
 * - 差异对比
 * - 版本回滚
 * - 分支管理
 * - 文件状态跟踪
 */
class GitManager : public QObject
{
    Q_OBJECT

public:
    // 文件状态
    enum class FileStatus {
        Untracked,      // 未跟踪
        Modified,       // 已修改
        Staged,         // 已暂存
        Committed,      // 已提交
        Conflicted,     // 冲突
        Deleted,        // 已删除
        Renamed,        // 已重命名
        Ignored         // 已忽略
    };
    Q_ENUM(FileStatus)

    // 提交信息
    struct CommitInfo {
        QString hash;           // 完整哈希
        QString shortHash;      // 短哈希
        QString author;         // 作者
        QString email;          // 邮箱
        QDateTime date;         // 日期
        QString message;        // 提交信息
        QString subject;        // 提交标题
        QStringList parents;    // 父提交
        int insertions;         // 添加行数
        int deletions;          // 删除行数

        CommitInfo() : insertions(0), deletions(0) {}
    };

    // 差异块
    struct DiffHunk {
        int oldStart;           // 旧文件起始行
        int oldCount;           // 旧文件行数
        int newStart;           // 新文件起始行
        int newCount;           // 新文件行数
        QString header;         // 块头信息
        QStringList lines;      // 差异行

        DiffHunk() : oldStart(0), oldCount(0), newStart(0), newCount(0) {}
    };

    // 文件差异
    struct FileDiff {
        QString filePath;
        QString oldPath;        // 重命名前路径
        FileStatus status;
        QVector<DiffHunk> hunks;
        int additions;
        int deletions;
        bool isBinary;

        FileDiff() : status(FileStatus::Modified), additions(0), deletions(0), isBinary(false) {}
    };

    // 分支信息
    struct BranchInfo {
        QString name;
        QString remote;         // 远程名称（如果是远程分支）
        QString upstream;       // 上游分支
        bool isLocal;
        bool isCurrent;
        QString lastCommit;

        BranchInfo() : isLocal(true), isCurrent(false) {}
    };

    // 远程信息
    struct RemoteInfo {
        QString name;
        QString fetchUrl;
        QString pushUrl;
    };

    // 暂存文件
    struct StashInfo {
        int index;
        QString message;
        QString branch;
        QDateTime date;

        StashInfo() : index(0) {}
    };

    explicit GitManager(QObject *parent = nullptr);
    ~GitManager();

    // Git 可用性
    static bool isGitInstalled();
    static QString gitVersion();
    static bool isGitLfsInstalled();

    // 仓库管理
    bool isRepository(const QString &path);
    bool initRepository(const QString &path);
    bool cloneRepository(const QString &url, const QString &path);
    bool openRepository(const QString &path);
    void closeRepository();
    QString repositoryPath() const { return m_repoPath; }
    QString rootPath() const { return m_rootPath; }

    // 文件状态
    QMap<QString, FileStatus> getStatus();
    FileStatus getFileStatus(const QString &filePath);
    QStringList getUntrackedFiles();
    QStringList getModifiedFiles();
    QStringList getStagedFiles();

    // 暂存操作
    bool stageFile(const QString &filePath);
    bool stageFiles(const QStringList &filePaths);
    bool stageAll();
    bool unstageFile(const QString &filePath);
    bool unstageAll();

    // 提交操作
    bool commit(const QString &message);
    bool amend(const QString &message = QString());
    bool revert(const QString &commitHash);

    // 历史记录
    QVector<CommitInfo> getHistory(int maxCount = 100, const QString &filePath = QString());
    CommitInfo getCommit(const QString &hash);
    QStringList getCommitFiles(const QString &hash);

    // 差异对比
    QString getDiff(const QString &filePath = QString());
    QString getDiffCached(const QString &filePath = QString());
    QString getDiffCommit(const QString &hash, const QString &filePath = QString());
    QString getDiffBetween(const QString &from, const QString &to, const QString &filePath = QString());
    QVector<FileDiff> parseDiff(const QString &diffText);

    // 文件版本
    QString getFileContent(const QString &filePath, const QString &revision = "HEAD");
    QVector<CommitInfo> getFileHistory(const QString &filePath, int maxCount = 50);
    bool restoreFile(const QString &filePath, const QString &revision = QString());
    bool checkoutFile(const QString &filePath, const QString &revision);

    // 分支管理
    QVector<BranchInfo> getBranches();
    QString currentBranch();
    bool createBranch(const QString &name, const QString &startPoint = QString());
    bool deleteBranch(const QString &name, bool force = false);
    bool renameBranch(const QString &oldName, const QString &newName);
    bool checkout(const QString &branch);
    bool merge(const QString &branch);

    // 远程操作
    QVector<RemoteInfo> getRemotes();
    bool addRemote(const QString &name, const QString &url);
    bool removeRemote(const QString &name);
    bool fetch(const QString &remote = QString());
    bool pull(const QString &remote = QString(), const QString &branch = QString());
    bool push(const QString &remote = QString(), const QString &branch = QString());

    // Git LFS
    bool isLfsEnabled();
    bool installLfs(bool local = true);
    QStringList getLfsTrackedPatterns();
    bool trackWithLfs(const QString &pattern);
    bool untrackFromLfs(const QString &pattern);

    // 暂存区（Stash）
    QVector<StashInfo> getStashes();
    bool stash(const QString &message = QString());
    bool stashPop(int index = 0);
    bool stashApply(int index = 0);
    bool stashDrop(int index = 0);

    // 标签
    QStringList getTags();
    bool createTag(const QString &name, const QString &message = QString(), const QString &commit = QString());
    bool deleteTag(const QString &name);

    // 配置
    QString getConfig(const QString &key);
    bool setConfig(const QString &key, const QString &value, bool global = false);
    QString getUserName();
    QString getUserEmail();
    bool setUser(const QString &name, const QString &email);

    // 忽略文件
    QStringList getIgnored();
    bool addToGitignore(const QString &pattern);

    // Blame
    struct BlameInfo {
        QString hash;
        QString author;
        QDateTime date;
        int lineNumber;
        QString content;

        BlameInfo() : lineNumber(0) {}
    };
    QVector<BlameInfo> blame(const QString &filePath);

    // 错误信息
    QString lastError() const { return m_lastError; }
    QString lastOutput() const { return m_lastOutput; }

signals:
    void repositoryOpened(const QString &path);
    void repositoryClosed();
    void statusChanged();
    void commitCreated(const QString &hash);
    void branchChanged(const QString &branch);
    void progressChanged(int percent, const QString &message);
    void errorOccurred(const QString &error);
    void outputReceived(const QString &output);

private:
    // Git 命令执行
    bool runGit(const QStringList &args, int timeout = 30000);
    bool runGitAsync(const QStringList &args);
    QString runGitOutput(const QStringList &args, int timeout = 30000);

    // 解析辅助
    CommitInfo parseCommitLine(const QString &line);
    QVector<DiffHunk> parseHunks(const QString &diffText);
    FileStatus parseStatusCode(const QString &code);

    QString m_repoPath;     // .git 目录路径
    QString m_rootPath;     // 仓库根目录
    QString m_lastError;
    QString m_lastOutput;
    QProcess *m_process;
    QString m_streamStdOut;
    QString m_streamStdErr;
};

#endif // GITMANAGER_H

