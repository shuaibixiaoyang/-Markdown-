// 文件说明：test\integration\mainwindowlfsintegrationtest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "mainwindowlfsintegrationtest.h"

#include <QtTest>

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QMessageBox>
#include <QProcess>
#include <QScreen>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>

#include <mainwindow.h>
#include <vcs/gitmanager.h>

namespace {

static const char kEnableLfsSlot[] = "gitEnableLfs";
static const char kManageLfsSlot[] = "gitManageLfsTracking";
static const char kQuickTrackSlot[] = "gitQuickTrackLargeFiles";

class ModalDialogAutoCloser
{
public:
    ModalDialogAutoCloser()
    {
        QObject::connect(&m_timer, &QTimer::timeout, [&]() {
            closeAllVisibleDialogs();
        });
        m_timer.start(30);
    }

    ~ModalDialogAutoCloser()
    {
        m_timer.stop();
        closeAllVisibleDialogs();
    }

private:
    QTimer m_timer;

    static void closeAllVisibleDialogs()
    {
        const auto topLevels = QApplication::topLevelWidgets();
        for (QWidget *widget : topLevels) {
            if (!widget || !widget->isVisible()) {
                continue;
            }

            if (QMessageBox *messageBox = qobject_cast<QMessageBox *>(widget)) {
                QMetaObject::invokeMethod(messageBox, "accept", Qt::QueuedConnection);
                continue;
            }

            if (QDialog *dialog = qobject_cast<QDialog *>(widget)) {
                QMetaObject::invokeMethod(dialog, "reject", Qt::QueuedConnection);
            }
        }
    }
};

bool runGit(const QString &workingDir, const QStringList &arguments, QString *stdErr)
{
    QProcess git;
    git.setProgram(QStringLiteral("git"));
    git.setArguments(arguments);
    git.setWorkingDirectory(workingDir);
    git.start();

    if (!git.waitForStarted(3000)) {
        if (stdErr) {
            *stdErr = QObject::tr("Cannot start git process");
        }
        return false;
    }

    if (!git.waitForFinished(5000)) {
        git.kill();
        git.waitForFinished(1000);
        if (stdErr) {
            *stdErr = QObject::tr("Git command timed out");
        }
        return false;
    }

    if (stdErr) {
        *stdErr = QString::fromUtf8(git.readAllStandardError()).trimmed();
    }

    return git.exitStatus() == QProcess::NormalExit && git.exitCode() == 0;
}

bool gitAvailable()
{
    QString err;
    return runGit(QDir::currentPath(), {QStringLiteral("--version")}, &err);
}

QString prepareGitRepositoryWithFile(QTemporaryDir &tempDir)
{
    QString err;

    if (!runGit(tempDir.path(), {QStringLiteral("init")}, &err)) {
        return QString();
    }

    const QString notePath = tempDir.filePath(QStringLiteral("note.md"));
    QFile file(notePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return QString();
    }
    file.write("# lfs menu integration\n");
    file.close();

    return notePath;
}

QAction *findActionByText(MainWindow &window, const QString &text)
{
    const QList<QAction *> actions = window.findChildren<QAction *>();
    for (QAction *action : actions) {
        if (action && action->text() == text) {
            return action;
        }
    }
    return nullptr;
}

void pumpEvents(int ms)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < ms) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }
}

void invokeSlotAndWait(MainWindow &window, const char *slotName)
{
    const ModalDialogAutoCloser autoCloser;
    const bool invoked = QMetaObject::invokeMethod(&window, slotName, Qt::QueuedConnection);
    QVERIFY2(invoked, "Failed to invoke MainWindow slot");
    pumpEvents(800);
}

} // namespace

// 函数说明：实现 MainWindowLfsIntegrationTest::initTestCase 的核心逻辑，供当前模块调用。
void MainWindowLfsIntegrationTest::initTestCase()
{
    const QString platform = QGuiApplication::platformName();
    if (platform == QStringLiteral("offscreen") ||
        platform == QStringLiteral("minimal") ||
        platform == QStringLiteral("minimalegl")) {
        QSKIP("MainWindow LFS UI integration tests require a non-headless Qt platform plugin.");
    }

    if (QGuiApplication::primaryScreen() == nullptr) {
        QSKIP("No screen available for MainWindow LFS integration test.");
    }

    QStandardPaths::setTestModeEnabled(true);
}

// 函数说明：实现 MainWindowLfsIntegrationTest::lfsActionsExistInGitMenu 的核心逻辑，供当前模块调用。
void MainWindowLfsIntegrationTest::lfsActionsExistInGitMenu()
{
    MainWindow window;

    QAction *enableAction = findActionByText(window, QStringLiteral("启用 Git LFS（当前仓库）"));
    QAction *manageAction = findActionByText(window, QStringLiteral("管理 LFS 跟踪规则..."));
    QAction *quickTrackAction = findActionByText(window, QStringLiteral("跟踪常见大文件类型"));

    QVERIFY(enableAction != nullptr);
    QVERIFY(manageAction != nullptr);
    QVERIFY(quickTrackAction != nullptr);
}

// 函数说明：实现 MainWindowLfsIntegrationTest::lfsActionEnablementReflectsRepositoryState 的核心逻辑，供当前模块调用。
void MainWindowLfsIntegrationTest::lfsActionEnablementReflectsRepositoryState()
{
    if (!gitAvailable()) {
        QSKIP("Git binary is unavailable in this environment.");
    }

    MainWindow nonRepoWindow;
    QAction *nonRepoEnableAction = findActionByText(nonRepoWindow, QStringLiteral("启用 Git LFS（当前仓库）"));
    QAction *nonRepoManageAction = findActionByText(nonRepoWindow, QStringLiteral("管理 LFS 跟踪规则..."));
    QAction *nonRepoQuickTrackAction = findActionByText(nonRepoWindow, QStringLiteral("跟踪常见大文件类型"));

    QVERIFY(nonRepoEnableAction != nullptr);
    QVERIFY(nonRepoManageAction != nullptr);
    QVERIFY(nonRepoQuickTrackAction != nullptr);

    QVERIFY(!nonRepoEnableAction->isEnabled());
    QVERIFY(!nonRepoManageAction->isEnabled());
    QVERIFY(!nonRepoQuickTrackAction->isEnabled());

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString markdownFile = prepareGitRepositoryWithFile(tempDir);
    QVERIFY(!markdownFile.isEmpty());

    MainWindow repoWindow(markdownFile);
    QAction *repoEnableAction = findActionByText(repoWindow, QStringLiteral("启用 Git LFS（当前仓库）"));
    QAction *repoManageAction = findActionByText(repoWindow, QStringLiteral("管理 LFS 跟踪规则..."));
    QAction *repoQuickTrackAction = findActionByText(repoWindow, QStringLiteral("跟踪常见大文件类型"));

    QVERIFY(repoEnableAction != nullptr);
    QVERIFY(repoManageAction != nullptr);
    QVERIFY(repoQuickTrackAction != nullptr);

    QVERIFY(repoEnableAction->isEnabled());

    const bool lfsInstalled = GitManager::isGitLfsInstalled();
    QCOMPARE(repoManageAction->isEnabled(), lfsInstalled);
    QCOMPARE(repoQuickTrackAction->isEnabled(), lfsInstalled);
}

// 函数说明：实现 MainWindowLfsIntegrationTest::lfsActionsTriggerWithoutHanging 的核心逻辑，供当前模块调用。
void MainWindowLfsIntegrationTest::lfsActionsTriggerWithoutHanging()
{
    if (!gitAvailable()) {
        QSKIP("Git binary is unavailable in this environment.");
    }

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString markdownFile = prepareGitRepositoryWithFile(tempDir);
    QVERIFY(!markdownFile.isEmpty());

    MainWindow window(markdownFile);

    // 直接调用槽函数，覆盖三个菜单动作的完整交互路径；
    // 在 LFS 未安装环境中会进入告警分支，在已安装环境中会进入启用/管理分支。
    invokeSlotAndWait(window, kEnableLfsSlot);
    invokeSlotAndWait(window, kManageLfsSlot);
    invokeSlotAndWait(window, kQuickTrackSlot);

    QVERIFY(true);
}

