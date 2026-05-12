// 文件说明：app-static\vcs\gitmanager.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "gitmanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>

namespace {

QProcessEnvironment gitProcessEnvironment()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    // Force stable, machine-readable output regardless of user locale.
    env.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    env.insert(QStringLiteral("LANG"), QStringLiteral("C"));
    // Never prompt on TTY-less invocations (prevents hidden hangs).
    env.insert(QStringLiteral("GIT_TERMINAL_PROMPT"), QStringLiteral("0"));
    return env;
}

QString simplifyProcessText(const QString &text)
{
    return text.trimmed();
}

QString composeGitFailureMessage(const QString &stderrOutput,
                                 const QProcess *process,
                                 int exitCode)
{
    const QString stderrText = simplifyProcessText(stderrOutput);
    if (!stderrText.isEmpty()) {
        return stderrText;
    }
    if (process && process->error() != QProcess::UnknownError) {
        return QObject::tr("Git 进程错误: %1").arg(process->errorString());
    }
    return QObject::tr("Git 命令失败 (exit code %1)").arg(exitCode);
}

} // namespace

// 函数说明：构造 GitManager 对象，初始化本模块需要的状态、界面和资源。
GitManager::GitManager(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
{
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        const QString output = QString::fromUtf8(m_process->readAllStandardOutput());
        if (output.isEmpty()) {
            return;
        }
        m_streamStdOut += output;
        emit outputReceived(output);
    });

    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        const QString error = QString::fromUtf8(m_process->readAllStandardError());
        if (error.isEmpty()) {
            return;
        }
        m_streamStdErr += error;
    });

    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        Q_UNUSED(error);
        if (m_process->error() != QProcess::UnknownError) {
            m_lastError = tr("Git 进程错误: %1").arg(m_process->errorString());
            emit errorOccurred(m_lastError);
        }
    });
}

// 函数说明：销毁 GitManager 对象，释放本模块持有的资源。
GitManager::~GitManager()
{
    closeRepository();
}

// 函数说明：判断 GitManager 当前是否满足指定状态。
bool GitManager::isGitInstalled()
{
    QProcess process;
    process.setProcessEnvironment(gitProcessEnvironment());
    process.start("git", QStringList() << "--version");
    if (!process.waitForStarted(3000)) {
        return false;
    }
    if (!process.waitForFinished(5000)) {
        process.kill();
        process.waitForFinished(1000);
        return false;
    }
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

// 函数说明：处理 Git 相关操作，维护当前文档目录的版本状态。
QString GitManager::gitVersion()
{
    QProcess process;
    process.setProcessEnvironment(gitProcessEnvironment());
    process.start("git", QStringList() << "--version");
    if (!process.waitForStarted(3000)) {
        return QString();
    }
    if (!process.waitForFinished(5000)) {
        process.kill();
        process.waitForFinished(1000);
        return QString();
    }
    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

// 函数说明：判断 GitManager 当前是否满足指定状态。
bool GitManager::isGitLfsInstalled()
{
    QProcess process;
    process.setProcessEnvironment(gitProcessEnvironment());
    process.start(QStringLiteral("git"), QStringList() << QStringLiteral("lfs") << QStringLiteral("version"));
    if (!process.waitForStarted(3000)) {
        return false;
    }
    if (!process.waitForFinished(5000)) {
        process.kill();
        process.waitForFinished(1000);
        return false;
    }
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

// 函数说明：判断 GitManager 当前是否满足指定状态。
bool GitManager::isRepository(const QString &path)
{
    QDir gitDir(path + "/.git");
    return gitDir.exists();
}

// 函数说明：实现 GitManager::initRepository 的核心逻辑，供当前模块调用。
bool GitManager::initRepository(const QString &path)
{
    QDir dir(path);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            m_lastError = tr("无法创建目录: %1").arg(path);
            return false;
        }
    }

    m_rootPath = path;
    QStringList args;
    args << "init";

    if (runGit(args)) {
        m_repoPath = path + "/.git";
        emit repositoryOpened(path);
        return true;
    }
    return false;
}

// 函数说明：实现 GitManager::cloneRepository 的核心逻辑，供当前模块调用。
bool GitManager::cloneRepository(const QString &url, const QString &path)
{
    QStringList args;
    args << "clone" << url << path;

    // 临时设置工作目录
    QString oldRoot = m_rootPath;
    m_rootPath = QFileInfo(path).path();

    bool success = runGit(args, 300000);  // 5分钟超时

    if (success) {
        m_rootPath = path;
        m_repoPath = path + "/.git";
        emit repositoryOpened(path);
    } else {
        m_rootPath = oldRoot;
    }

    return success;
}

// 函数说明：打开 GitManager 对应的文件、资源或功能入口。
bool GitManager::openRepository(const QString &path)
{
    // 查找 .git 目录
    QString searchPath = path;
    while (!searchPath.isEmpty()) {
        if (QDir(searchPath + "/.git").exists()) {
            m_rootPath = searchPath;
            m_repoPath = searchPath + "/.git";
            emit repositoryOpened(searchPath);
            return true;
        }

        QDir dir(searchPath);
        if (!dir.cdUp()) break;
        searchPath = dir.path();
    }

    m_lastError = tr("不是 Git 仓库: %1").arg(path);
    return false;
}

// 函数说明：关闭 GitManager 相关窗口或资源，并处理必要的保存确认。
void GitManager::closeRepository()
{
    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        m_process->waitForFinished(3000);
    }

    m_rootPath.clear();
    m_repoPath.clear();
    emit repositoryClosed();
}

// 函数说明：实现 GitManager::runGit 的核心逻辑，供当前模块调用。
bool GitManager::runGit(const QStringList &args, int timeout)
{
    if (m_rootPath.isEmpty()) {
        m_lastError = tr("未打开仓库");
        return false;
    }

    m_lastOutput.clear();
    m_lastError.clear();
    m_streamStdOut.clear();
    m_streamStdErr.clear();

    m_process->setWorkingDirectory(m_rootPath);
    m_process->setProcessEnvironment(gitProcessEnvironment());
    m_process->start(QStringLiteral("git"), args, QIODevice::ReadOnly);

    if (!m_process->waitForStarted(5000)) {
        m_lastError = tr("无法启动 Git 进程: %1").arg(m_process->errorString());
        emit errorOccurred(m_lastError);
        return false;
    }

    if (!m_process->waitForFinished(timeout)) {
        m_process->kill();
        m_process->waitForFinished(1000);
        m_lastOutput = m_streamStdOut + QString::fromUtf8(m_process->readAllStandardOutput());
        m_streamStdErr += QString::fromUtf8(m_process->readAllStandardError());
        m_lastError = tr("Git 命令超时: git %1").arg(args.join(' '));
        const QString details = simplifyProcessText(m_streamStdErr);
        if (!details.isEmpty()) {
            m_lastError += QStringLiteral("\n") + details;
        }
        emit errorOccurred(m_lastError);
        return false;
    }

    m_lastOutput = m_streamStdOut + QString::fromUtf8(m_process->readAllStandardOutput());
    const QString errorOutput = m_streamStdErr + QString::fromUtf8(m_process->readAllStandardError());

    if (m_process->exitStatus() != QProcess::NormalExit || m_process->exitCode() != 0) {
        m_lastError = composeGitFailureMessage(errorOutput, m_process, m_process->exitCode());
        emit errorOccurred(m_lastError);
        return false;
    }

    if (!errorOutput.trimmed().isEmpty()) {
        // 保留 stderr 作为诊断信息（不覆盖成功状态）。
        m_lastError = errorOutput.trimmed();
    }

    return true;
}

// 函数说明：实现 GitManager::runGitAsync 的核心逻辑，供当前模块调用。
bool GitManager::runGitAsync(const QStringList &args)
{
    if (m_rootPath.isEmpty()) {
        m_lastError = tr("未打开仓库");
        return false;
    }

    if (m_process->state() != QProcess::NotRunning) {
        m_lastError = tr("Git 进程正在运行");
        return false;
    }

    m_lastOutput.clear();
    m_lastError.clear();
    m_streamStdOut.clear();
    m_streamStdErr.clear();

    m_process->setWorkingDirectory(m_rootPath);
    m_process->setProcessEnvironment(gitProcessEnvironment());
    m_process->start(QStringLiteral("git"), args, QIODevice::ReadOnly);
    if (!m_process->waitForStarted(5000)) {
        m_lastError = tr("无法启动 Git 进程: %1").arg(m_process->errorString());
        emit errorOccurred(m_lastError);
        return false;
    }
    return true;
}

// 函数说明：实现 GitManager::runGitOutput 的核心逻辑，供当前模块调用。
QString GitManager::runGitOutput(const QStringList &args, int timeout)
{
    if (runGit(args, timeout)) {
        return m_lastOutput;
    }
    return QString();
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
QMap<QString, GitManager::FileStatus> GitManager::getStatus()
{
    QMap<QString, FileStatus> statusMap;

    QStringList args;
    args << "status" << "--porcelain=v1" << "-z" << "--untracked-files=all";

    QString output = runGitOutput(args);
    if (output.isEmpty()) return statusMap;

    const QStringList entries = output.split(QChar('\0'), Qt::SkipEmptyParts);
    for (int i = 0; i < entries.size(); ++i) {
        const QString entry = entries.at(i);
        if (entry.size() < 3) {
            continue;
        }

        const QString code = entry.left(2);
        QString filePath = entry.mid(3);

        // In -z mode, renames are emitted as two path fields.
        if (code.startsWith(QChar('R')) || code.startsWith(QChar('C'))) {
            if (i + 1 < entries.size()) {
                filePath = entries.at(i + 1);
                ++i;
            }
        }

        statusMap[filePath] = parseStatusCode(code);
    }

    return statusMap;
}

// 函数说明：解析输入内容，转换为 GitManager 后续处理使用的数据结构。
GitManager::FileStatus GitManager::parseStatusCode(const QString &code)
{
    if (code.size() < 2) {
        return FileStatus::Committed;
    }

    const QChar indexStatus = code.at(0);
    const QChar workStatus = code.at(1);

    if (indexStatus == '?' && workStatus == '?') {
        return FileStatus::Untracked;
    }
    if (indexStatus == '!' && workStatus == '!') {
        return FileStatus::Ignored;
    }

    const QString status(code);
    if (status.contains(QLatin1Char('U')) || status == QStringLiteral("AA") ||
        status == QStringLiteral("DD")) {
        return FileStatus::Conflicted;
    }
    if (indexStatus == 'R' || indexStatus == 'C' || workStatus == 'R' || workStatus == 'C') {
        return FileStatus::Renamed;
    }
    if (indexStatus == 'D' || workStatus == 'D') {
        return FileStatus::Deleted;
    }
    if (indexStatus != ' ' && indexStatus != '?') {
        return FileStatus::Staged;
    }
    if (workStatus != ' ' && workStatus != '?') {
        return FileStatus::Modified;
    }
    return FileStatus::Committed;
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
GitManager::FileStatus GitManager::getFileStatus(const QString &filePath)
{
    QString relativePath = filePath;
    if (filePath.startsWith(m_rootPath)) {
        relativePath = filePath.mid(m_rootPath.length() + 1);
    }

    QMap<QString, FileStatus> statusMap = getStatus();
    if (statusMap.contains(relativePath)) {
        return statusMap[relativePath];
    }

    return FileStatus::Committed;
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
QStringList GitManager::getUntrackedFiles()
{
    QStringList files;
    QMap<QString, FileStatus> statusMap = getStatus();

    for (auto it = statusMap.begin(); it != statusMap.end(); ++it) {
        if (it.value() == FileStatus::Untracked) {
            files.append(it.key());
        }
    }

    return files;
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
QStringList GitManager::getModifiedFiles()
{
    QStringList files;
    QMap<QString, FileStatus> statusMap = getStatus();

    for (auto it = statusMap.begin(); it != statusMap.end(); ++it) {
        if (it.value() == FileStatus::Modified) {
            files.append(it.key());
        }
    }

    return files;
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
QStringList GitManager::getStagedFiles()
{
    QStringList files;
    QMap<QString, FileStatus> statusMap = getStatus();

    for (auto it = statusMap.begin(); it != statusMap.end(); ++it) {
        if (it.value() == FileStatus::Staged) {
            files.append(it.key());
        }
    }

    return files;
}

// 函数说明：实现 GitManager::stageFile 的核心逻辑，供当前模块调用。
bool GitManager::stageFile(const QString &filePath)
{
    QStringList args;
    args << "add" << filePath;
    bool success = runGit(args);
    if (success) emit statusChanged();
    return success;
}

// 函数说明：实现 GitManager::stageFiles 的核心逻辑，供当前模块调用。
bool GitManager::stageFiles(const QStringList &filePaths)
{
    QStringList args;
    args << "add";
    args.append(filePaths);
    bool success = runGit(args);
    if (success) emit statusChanged();
    return success;
}

// 函数说明：实现 GitManager::stageAll 的核心逻辑，供当前模块调用。
bool GitManager::stageAll()
{
    QStringList args;
    args << "add" << "-A";
    bool success = runGit(args);
    if (success) emit statusChanged();
    return success;
}

// 函数说明：实现 GitManager::unstageFile 的核心逻辑，供当前模块调用。
bool GitManager::unstageFile(const QString &filePath)
{
    QStringList args;
    args << "reset" << "HEAD" << filePath;
    bool success = runGit(args);
    if (success) emit statusChanged();
    return success;
}

// 函数说明：实现 GitManager::unstageAll 的核心逻辑，供当前模块调用。
bool GitManager::unstageAll()
{
    QStringList args;
    args << "reset" << "HEAD";
    bool success = runGit(args);
    if (success) emit statusChanged();
    return success;
}

// 函数说明：实现 GitManager::commit 的核心逻辑，供当前模块调用。
bool GitManager::commit(const QString &message)
{
    QStringList args;
    args << "commit" << "-m" << message;

    if (runGit(args)) {
        // 获取新提交的哈希
        QString hash = runGitOutput(QStringList() << "rev-parse" << "HEAD").trimmed();
        emit commitCreated(hash);
        emit statusChanged();
        return true;
    }
    return false;
}

// 函数说明：实现 GitManager::amend 的核心逻辑，供当前模块调用。
bool GitManager::amend(const QString &message)
{
    QStringList args;
    args << "commit" << "--amend";
    if (!message.isEmpty()) {
        args << "-m" << message;
    } else {
        args << "--no-edit";
    }
    return runGit(args);
}

// 函数说明：实现 GitManager::revert 的核心逻辑，供当前模块调用。
bool GitManager::revert(const QString &commitHash)
{
    QStringList args;
    args << "revert" << "--no-commit" << commitHash;
    return runGit(args);
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
QVector<GitManager::CommitInfo> GitManager::getHistory(int maxCount, const QString &filePath)
{
    QVector<CommitInfo> history;

    QStringList args;
    args << "log"
         << "--format=%H|%h|%an|%ae|%aI|%s|%P"
         << QString("-n%1").arg(maxCount);

    if (!filePath.isEmpty()) {
        args << "--" << filePath;
    }

    QString output = runGitOutput(args);
    if (output.isEmpty()) return history;

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        history.append(parseCommitLine(line));
    }

    return history;
}

// 函数说明：解析输入内容，转换为 GitManager 后续处理使用的数据结构。
GitManager::CommitInfo GitManager::parseCommitLine(const QString &line)
{
    CommitInfo info;
    QStringList parts = line.split('|');

    if (parts.size() >= 6) {
        info.hash = parts[0];
        info.shortHash = parts[1];
        info.author = parts[2];
        info.email = parts[3];
        info.date = QDateTime::fromString(parts[4], Qt::ISODate);
        info.subject = parts[5];
        info.message = parts[5];

        if (parts.size() >= 7) {
            info.parents = parts[6].split(' ', Qt::SkipEmptyParts);
        }
    }

    return info;
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
GitManager::CommitInfo GitManager::getCommit(const QString &hash)
{
    CommitInfo info;

    QStringList args;
    args << "show"
         << "--format=%H|%h|%an|%ae|%aI|%s|%P|%B"
         << "--numstat"
         << hash;

    QString output = runGitOutput(args);
    if (output.isEmpty()) return info;

    QStringList lines = output.split('\n');
    if (!lines.isEmpty()) {
        info = parseCommitLine(lines[0]);

        // Parse numstat lines: "<additions>\t<deletions>\t<path>"
        for (const QString &line : lines) {
            const QStringList fields = line.split('\t');
            if (fields.size() < 3) {
                continue;
            }

            bool okInsertions = false;
            bool okDeletions = false;
            const int insertions = fields.at(0).toInt(&okInsertions);
            const int deletions = fields.at(1).toInt(&okDeletions);
            if (okInsertions) {
                info.insertions += insertions;
            }
            if (okDeletions) {
                info.deletions += deletions;
            }
        }
    }

    return info;
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
QStringList GitManager::getCommitFiles(const QString &hash)
{
    QStringList args;
    args << "diff-tree" << "--no-commit-id" << "--name-only" << "-r" << hash;

    QString output = runGitOutput(args);
    return output.split('\n', Qt::SkipEmptyParts);
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
QString GitManager::getDiff(const QString &filePath)
{
    QStringList args;
    args << "diff";

    if (!filePath.isEmpty()) {
        args << "--" << filePath;
    }

    return runGitOutput(args);
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
QString GitManager::getDiffCached(const QString &filePath)
{
    QStringList args;
    args << "diff" << "--cached";

    if (!filePath.isEmpty()) {
        args << "--" << filePath;
    }

    return runGitOutput(args);
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
QString GitManager::getDiffCommit(const QString &hash, const QString &filePath)
{
    QStringList args;
    args << "show" << hash;

    if (!filePath.isEmpty()) {
        args << "--" << filePath;
    }

    return runGitOutput(args);
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
QString GitManager::getDiffBetween(const QString &from, const QString &to, const QString &filePath)
{
    QStringList args;
    args << "diff" << from << to;

    if (!filePath.isEmpty()) {
        args << "--" << filePath;
    }

    return runGitOutput(args);
}

// 函数说明：解析输入内容，转换为 GitManager 后续处理使用的数据结构。
QVector<GitManager::FileDiff> GitManager::parseDiff(const QString &diffText)
{
    QVector<FileDiff> diffs;

    QRegularExpression diffHeaderRegex("^diff --git a/(.*) b/(.*)$", QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator it = diffHeaderRegex.globalMatch(diffText);

    QStringList sections;
    QList<int> positions;

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        positions.append(match.capturedStart());
    }

    for (int i = 0; i < positions.size(); ++i) {
        int start = positions[i];
        int end = (i + 1 < positions.size()) ? positions[i + 1] : diffText.length();
        sections.append(diffText.mid(start, end - start));
    }

    for (const QString &section : sections) {
        FileDiff diff;

        // 解析文件路径
        QRegularExpressionMatch headerMatch = diffHeaderRegex.match(section);
        if (headerMatch.hasMatch()) {
            diff.oldPath = headerMatch.captured(1);
            diff.filePath = headerMatch.captured(2);
        }

        // 检查是否为二进制文件
        if (section.contains("Binary files")) {
            diff.isBinary = true;
        }

        // 解析状态
        if (section.contains("new file mode")) {
            diff.status = FileStatus::Untracked;
        } else if (section.contains("deleted file mode")) {
            diff.status = FileStatus::Deleted;
        } else if (section.contains("rename from")) {
            diff.status = FileStatus::Renamed;
        }

        // 解析块
        diff.hunks = parseHunks(section);

        // 统计
        for (const DiffHunk &hunk : diff.hunks) {
            for (const QString &line : hunk.lines) {
                if (line.startsWith('+') && !line.startsWith("+++")) {
                    diff.additions++;
                } else if (line.startsWith('-') && !line.startsWith("---")) {
                    diff.deletions++;
                }
            }
        }

        diffs.append(diff);
    }

    return diffs;
}

// 函数说明：解析输入内容，转换为 GitManager 后续处理使用的数据结构。
QVector<GitManager::DiffHunk> GitManager::parseHunks(const QString &diffText)
{
    QVector<DiffHunk> hunks;

    QRegularExpression hunkRegex("^@@\\s+-?(\\d+),?(\\d*)\\s+\\+?(\\d+),?(\\d*)\\s+@@(.*)$",
                                  QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator it = hunkRegex.globalMatch(diffText);

    QList<int> positions;
    QList<DiffHunk> headers;

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        positions.append(match.capturedStart());

        DiffHunk hunk;
        hunk.oldStart = match.captured(1).toInt();
        hunk.oldCount = match.captured(2).isEmpty() ? 1 : match.captured(2).toInt();
        hunk.newStart = match.captured(3).toInt();
        hunk.newCount = match.captured(4).isEmpty() ? 1 : match.captured(4).toInt();
        hunk.header = match.captured(5).trimmed();
        headers.append(hunk);
    }

    for (int i = 0; i < positions.size(); ++i) {
        DiffHunk hunk = headers[i];

        int start = positions[i];
        int end = (i + 1 < positions.size()) ? positions[i + 1] : diffText.length();
        QString hunkText = diffText.mid(start, end - start);

        QStringList lines = hunkText.split('\n');
        // 跳过第一行（@@ 行）
        for (int j = 1; j < lines.size(); ++j) {
            hunk.lines.append(lines[j]);
        }

        hunks.append(hunk);
    }

    return hunks;
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
QString GitManager::getFileContent(const QString &filePath, const QString &revision)
{
    QStringList args;
    args << "show" << QString("%1:%2").arg(revision, filePath);
    return runGitOutput(args);
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
QVector<GitManager::CommitInfo> GitManager::getFileHistory(const QString &filePath, int maxCount)
{
    return getHistory(maxCount, filePath);
}

// 函数说明：实现 GitManager::restoreFile 的核心逻辑，供当前模块调用。
bool GitManager::restoreFile(const QString &filePath, const QString &revision)
{
    QStringList args;
    if (revision.isEmpty()) {
        args << "restore" << filePath;
    } else {
        args << "restore" << "--source" << revision << filePath;
    }
    bool success = runGit(args);
    if (success) emit statusChanged();
    return success;
}

// 函数说明：实现 GitManager::checkoutFile 的核心逻辑，供当前模块调用。
bool GitManager::checkoutFile(const QString &filePath, const QString &revision)
{
    QStringList args;
    args << "checkout" << revision << "--" << filePath;
    bool success = runGit(args);
    if (success) emit statusChanged();
    return success;
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
QVector<GitManager::BranchInfo> GitManager::getBranches()
{
    QVector<BranchInfo> branches;

    // 本地分支
    QStringList args;
    args << "branch" << "-v" << "--format=%(refname:short)|%(upstream:short)|%(objectname:short)";
    QString output = runGitOutput(args);

    QString currentBr = currentBranch();

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        QStringList parts = line.split('|');
        if (parts.size() >= 1) {
            BranchInfo info;
            info.name = parts[0];
            info.isLocal = true;
            info.isCurrent = (info.name == currentBr);
            if (parts.size() >= 2) info.upstream = parts[1];
            if (parts.size() >= 3) info.lastCommit = parts[2];
            branches.append(info);
        }
    }

    // 远程分支
    args.clear();
    args << "branch" << "-r" << "--format=%(refname:short)|%(objectname:short)";
    output = runGitOutput(args);

    lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        QStringList parts = line.split('|');
        if (parts.size() >= 1) {
            BranchInfo info;
            QString fullName = parts[0];
            int slashPos = fullName.indexOf('/');
            if (slashPos > 0) {
                info.remote = fullName.left(slashPos);
                info.name = fullName.mid(slashPos + 1);
            } else {
                info.name = fullName;
            }
            info.isLocal = false;
            if (parts.size() >= 2) info.lastCommit = parts[1];
            branches.append(info);
        }
    }

    return branches;
}

// 函数说明：实现 GitManager::currentBranch 的核心逻辑，供当前模块调用。
QString GitManager::currentBranch()
{
    QStringList args;
    args << "rev-parse" << "--abbrev-ref" << "HEAD";
    return runGitOutput(args).trimmed();
}

// 函数说明：创建 GitManager 需要的对象、记录或输出内容。
bool GitManager::createBranch(const QString &name, const QString &startPoint)
{
    QStringList args;
    args << "branch" << name;
    if (!startPoint.isEmpty()) {
        args << startPoint;
    }
    return runGit(args);
}

// 函数说明：删除 GitManager 管理的指定数据或资源。
bool GitManager::deleteBranch(const QString &name, bool force)
{
    QStringList args;
    args << "branch" << (force ? "-D" : "-d") << name;
    return runGit(args);
}

// 函数说明：实现 GitManager::renameBranch 的核心逻辑，供当前模块调用。
bool GitManager::renameBranch(const QString &oldName, const QString &newName)
{
    QStringList args;
    args << "branch" << "-m" << oldName << newName;
    return runGit(args);
}

// 函数说明：实现 GitManager::checkout 的核心逻辑，供当前模块调用。
bool GitManager::checkout(const QString &branch)
{
    QStringList args;
    args << "checkout" << branch;
    if (runGit(args)) {
        emit branchChanged(branch);
        emit statusChanged();
        return true;
    }
    return false;
}

// 函数说明：实现 GitManager::merge 的核心逻辑，供当前模块调用。
bool GitManager::merge(const QString &branch)
{
    QStringList args;
    args << "merge" << branch;
    bool success = runGit(args);
    if (success) emit statusChanged();
    return success;
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
QVector<GitManager::RemoteInfo> GitManager::getRemotes()
{
    QVector<RemoteInfo> remotes;

    QStringList args;
    args << "remote" << "-v";
    QString output = runGitOutput(args);

    QRegularExpression remoteRegex("^(\\S+)\\s+(\\S+)\\s+\\((\\w+)\\)$", QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator it = remoteRegex.globalMatch(output);

    QMap<QString, RemoteInfo> remoteMap;
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString name = match.captured(1);
        QString url = match.captured(2);
        QString type = match.captured(3);

        if (!remoteMap.contains(name)) {
            RemoteInfo info;
            info.name = name;
            remoteMap[name] = info;
        }

        if (type == "fetch") {
            remoteMap[name].fetchUrl = url;
        } else if (type == "push") {
            remoteMap[name].pushUrl = url;
        }
    }

    for (const RemoteInfo &info : remoteMap) {
        remotes.append(info);
    }

    return remotes;
}

// 函数说明：向 GitManager 管理的数据集合中添加一项内容。
bool GitManager::addRemote(const QString &name, const QString &url)
{
    QStringList args;
    args << "remote" << "add" << name << url;
    return runGit(args);
}

// 函数说明：从 GitManager 管理的数据集合中移除指定内容。
bool GitManager::removeRemote(const QString &name)
{
    QStringList args;
    args << "remote" << "remove" << name;
    return runGit(args);
}

// 函数说明：实现 GitManager::fetch 的核心逻辑，供当前模块调用。
bool GitManager::fetch(const QString &remote)
{
    QStringList args;
    args << "fetch";
    if (!remote.isEmpty()) {
        args << remote;
    }
    return runGit(args, 120000);  // 2分钟超时
}

// 函数说明：实现 GitManager::pull 的核心逻辑，供当前模块调用。
bool GitManager::pull(const QString &remote, const QString &branch)
{
    QStringList args;
    args << "pull";
    if (!remote.isEmpty()) {
        args << remote;
        if (!branch.isEmpty()) {
            args << branch;
        }
    }
    bool success = runGit(args, 120000);
    if (success) emit statusChanged();
    return success;
}

// 函数说明：实现 GitManager::push 的核心逻辑，供当前模块调用。
bool GitManager::push(const QString &remote, const QString &branch)
{
    QStringList args;
    args << "push";
    if (!remote.isEmpty()) {
        args << remote;
        if (!branch.isEmpty()) {
            args << branch;
        }
    }
    return runGit(args, 120000);
}

// 函数说明：判断 GitManager 当前是否满足指定状态。
bool GitManager::isLfsEnabled()
{
    if (!isGitLfsInstalled()) {
        return false;
    }

    const QString output = runGitOutput(
        QStringList() << QStringLiteral("config") << QStringLiteral("--local")
                      << QStringLiteral("--get") << QStringLiteral("filter.lfs.required"));
    return output.trimmed().compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0;
}

// 函数说明：实现 GitManager::installLfs 的核心逻辑，供当前模块调用。
bool GitManager::installLfs(bool local)
{
    if (!isGitLfsInstalled()) {
        m_lastError = tr("Git LFS 未安装");
        emit errorOccurred(m_lastError);
        return false;
    }

    QStringList args;
    args << QStringLiteral("lfs") << QStringLiteral("install");
    if (local) {
        args << QStringLiteral("--local");
    }
    return runGit(args);
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
QStringList GitManager::getLfsTrackedPatterns()
{
    QStringList patterns;
    const QString output = runGitOutput(
        QStringList() << QStringLiteral("lfs") << QStringLiteral("track") << QStringLiteral("--list"));
    if (output.isEmpty()) {
        return patterns;
    }

    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    QRegularExpression quotedPattern(QStringLiteral("^\\s*\"([^\"]+)\"\\s+"));
    for (const QString &line : lines) {
        QRegularExpressionMatch match = quotedPattern.match(line);
        if (!match.hasMatch()) {
            continue;
        }
        patterns.append(match.captured(1));
    }
    return patterns;
}

// 函数说明：实现 GitManager::trackWithLfs 的核心逻辑，供当前模块调用。
bool GitManager::trackWithLfs(const QString &pattern)
{
    if (pattern.trimmed().isEmpty()) {
        m_lastError = tr("LFS 跟踪规则不能为空");
        emit errorOccurred(m_lastError);
        return false;
    }

    if (!isGitLfsInstalled()) {
        m_lastError = tr("Git LFS 未安装");
        emit errorOccurred(m_lastError);
        return false;
    }

    if (!runGit(QStringList() << QStringLiteral("lfs") << QStringLiteral("track")
                              << pattern.trimmed())) {
        return false;
    }

    // Ensure .gitattributes update is staged for next commit.
    runGit(QStringList() << QStringLiteral("add") << QStringLiteral(".gitattributes"));
    emit statusChanged();
    return true;
}

// 函数说明：实现 GitManager::untrackFromLfs 的核心逻辑，供当前模块调用。
bool GitManager::untrackFromLfs(const QString &pattern)
{
    if (pattern.trimmed().isEmpty()) {
        m_lastError = tr("LFS 取消跟踪规则不能为空");
        emit errorOccurred(m_lastError);
        return false;
    }

    if (!isGitLfsInstalled()) {
        m_lastError = tr("Git LFS 未安装");
        emit errorOccurred(m_lastError);
        return false;
    }

    if (!runGit(QStringList() << QStringLiteral("lfs") << QStringLiteral("untrack")
                              << pattern.trimmed())) {
        return false;
    }

    runGit(QStringList() << QStringLiteral("add") << QStringLiteral(".gitattributes"));
    emit statusChanged();
    return true;
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
QVector<GitManager::StashInfo> GitManager::getStashes()
{
    QVector<StashInfo> stashes;

    QStringList args;
    args << "stash" << "list" << "--format=%gd|%s|%aI";
    QString output = runGitOutput(args);

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        QStringList parts = line.split('|');
        if (parts.size() >= 3) {
            StashInfo info;

            // 解析 stash@{0} 格式
            QRegularExpression indexRegex("stash@\\{(\\d+)\\}");
            QRegularExpressionMatch match = indexRegex.match(parts[0]);
            if (match.hasMatch()) {
                info.index = match.captured(1).toInt();
            }

            info.message = parts[1];
            info.date = QDateTime::fromString(parts[2], Qt::ISODate);
            stashes.append(info);
        }
    }

    return stashes;
}

// 函数说明：实现 GitManager::stash 的核心逻辑，供当前模块调用。
bool GitManager::stash(const QString &message)
{
    QStringList args;
    args << "stash" << "push";
    if (!message.isEmpty()) {
        args << "-m" << message;
    }
    bool success = runGit(args);
    if (success) emit statusChanged();
    return success;
}

// 函数说明：实现 GitManager::stashPop 的核心逻辑，供当前模块调用。
bool GitManager::stashPop(int index)
{
    QStringList args;
    args << "stash" << "pop" << QString("stash@{%1}").arg(index);
    bool success = runGit(args);
    if (success) emit statusChanged();
    return success;
}

// 函数说明：实现 GitManager::stashApply 的核心逻辑，供当前模块调用。
bool GitManager::stashApply(int index)
{
    QStringList args;
    args << "stash" << "apply" << QString("stash@{%1}").arg(index);
    bool success = runGit(args);
    if (success) emit statusChanged();
    return success;
}

// 函数说明：实现 GitManager::stashDrop 的核心逻辑，供当前模块调用。
bool GitManager::stashDrop(int index)
{
    QStringList args;
    args << "stash" << "drop" << QString("stash@{%1}").arg(index);
    return runGit(args);
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
QStringList GitManager::getTags()
{
    QStringList args;
    args << "tag" << "-l";
    QString output = runGitOutput(args);
    return output.split('\n', Qt::SkipEmptyParts);
}

// 函数说明：创建 GitManager 需要的对象、记录或输出内容。
bool GitManager::createTag(const QString &name, const QString &message, const QString &commit)
{
    QStringList args;
    args << "tag";
    if (!message.isEmpty()) {
        args << "-a" << name << "-m" << message;
    } else {
        args << name;
    }
    if (!commit.isEmpty()) {
        args << commit;
    }
    return runGit(args);
}

// 函数说明：删除 GitManager 管理的指定数据或资源。
bool GitManager::deleteTag(const QString &name)
{
    QStringList args;
    args << "tag" << "-d" << name;
    return runGit(args);
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
QString GitManager::getConfig(const QString &key)
{
    QStringList args;
    args << "config" << "--get" << key;
    return runGitOutput(args).trimmed();
}

// 函数说明：设置 GitManager 的运行参数，并触发必要的界面或数据刷新。
bool GitManager::setConfig(const QString &key, const QString &value, bool global)
{
    QStringList args;
    args << "config";
    if (global) {
        args << "--global";
    }
    args << key << value;
    return runGit(args);
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
QString GitManager::getUserName()
{
    return getConfig("user.name");
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
QString GitManager::getUserEmail()
{
    return getConfig("user.email");
}

// 函数说明：设置 GitManager 的运行参数，并触发必要的界面或数据刷新。
bool GitManager::setUser(const QString &name, const QString &email)
{
    bool success = true;
    if (!name.isEmpty()) {
        success = success && setConfig("user.name", name);
    }
    if (!email.isEmpty()) {
        success = success && setConfig("user.email", email);
    }
    return success;
}

// 函数说明：读取 GitManager 当前保存的状态或计算结果。
QStringList GitManager::getIgnored()
{
    QStringList args;
    args << "ls-files" << "--ignored" << "--exclude-standard";
    QString output = runGitOutput(args);
    return output.split('\n', Qt::SkipEmptyParts);
}

// 函数说明：向 GitManager 管理的数据集合中添加一项内容。
bool GitManager::addToGitignore(const QString &pattern)
{
    QString gitignorePath = m_rootPath + "/.gitignore";
    QFile file(gitignorePath);

    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << "\n" << pattern;
        file.close();
        return true;
    }
    return false;
}

// 函数说明：实现 GitManager::blame 的核心逻辑，供当前模块调用。
QVector<GitManager::BlameInfo> GitManager::blame(const QString &filePath)
{
    QVector<BlameInfo> blameList;

    QStringList args;
    args << "blame" << "--line-porcelain" << filePath;
    QString output = runGitOutput(args);

    QStringList lines = output.split('\n');
    BlameInfo currentInfo;

    for (const QString &line : lines) {
        if (line.isEmpty()) continue;

        if (line.startsWith('\t')) {
            // 代码行
            currentInfo.content = line.mid(1);
            blameList.append(currentInfo);
        } else if (line.length() >= 40 && !line.contains(' ', Qt::CaseSensitive)) {
            // 新的 blame 条目开始
            currentInfo = BlameInfo();
            currentInfo.hash = line.left(40);
        } else if (line.startsWith("author ")) {
            currentInfo.author = line.mid(7);
        } else if (line.startsWith("author-time ")) {
            qint64 timestamp = line.mid(12).toLongLong();
            currentInfo.date = QDateTime::fromSecsSinceEpoch(timestamp);
        }
    }

    // 添加行号
    for (int i = 0; i < blameList.size(); ++i) {
        blameList[i].lineNumber = i + 1;
    }

    return blameList;
}

