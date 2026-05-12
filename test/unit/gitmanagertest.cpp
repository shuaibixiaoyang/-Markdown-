// 文件说明：test\unit\gitmanagertest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "gitmanagertest.h"

#include <QtTest>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>

#include <vcs/gitmanager.h>

namespace {

bool initRepo(GitManager &manager, const QString &path)
{
    if (!manager.initRepository(path)) {
        return false;
    }
    return manager.openRepository(path);
}

} // namespace

// 函数说明：实现 GitManagerTest::checkoutFailureProvidesDetailedError 的核心逻辑，供当前模块调用。
void GitManagerTest::checkoutFailureProvidesDetailedError()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    GitManager manager;
    QVERIFY(initRepo(manager, dir.path()));

    QSignalSpy errorSpy(&manager, &GitManager::errorOccurred);
    QVERIFY(!manager.checkout(QStringLiteral("this-branch-does-not-exist")));
    QVERIFY(!manager.lastError().trimmed().isEmpty());
    QVERIFY(errorSpy.count() > 0);

    // Ensure we no longer return a generic silent failure message.
    QVERIFY(manager.lastError() != QStringLiteral("Git 命令失败"));
}

// 函数说明：实现 GitManagerTest::statusParsingSupportsSpacesAndUnicode 的核心逻辑，供当前模块调用。
void GitManagerTest::statusParsingSupportsSpacesAndUnicode()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    GitManager manager;
    QVERIFY(initRepo(manager, dir.path()));

    const QString fileName = QStringLiteral("folder/空 格.txt");
    const QString fullPath = dir.filePath(fileName);
    QVERIFY(QDir().mkpath(QFileInfo(fullPath).path()));

    QFile file(fullPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));
    file.write("line-1\n");
    file.close();

    QVERIFY(manager.stageFile(fileName));

    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text));
    file.write("line-2\n");
    file.close();

    const QMap<QString, GitManager::FileStatus> status = manager.getStatus();
    QVERIFY(status.contains(fileName));
    QCOMPARE(status.value(fileName), GitManager::FileStatus::Staged);
}

// 函数说明：实现 GitManagerTest::lfsTrackAndUntrackRoundTrip 的核心逻辑，供当前模块调用。
void GitManagerTest::lfsTrackAndUntrackRoundTrip()
{
    if (!GitManager::isGitLfsInstalled()) {
        QSKIP("Git LFS is not installed in this environment.");
    }

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    GitManager manager;
    QVERIFY(initRepo(manager, dir.path()));
    QVERIFY(manager.installLfs(true));

    const QString pattern = QStringLiteral("*.bin");
    QVERIFY(manager.trackWithLfs(pattern));
    QVERIFY(manager.getLfsTrackedPatterns().contains(pattern));

    QVERIFY(manager.untrackFromLfs(pattern));
    QVERIFY(!manager.getLfsTrackedPatterns().contains(pattern));
}

