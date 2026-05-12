// 文件说明：app-static\editor\multitabeditor.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef MULTITABEDITOR_H
#define MULTITABEDITOR_H

#include <QWidget>
#include <QTabWidget>
#include <QTabBar>
#include <QPlainTextEdit>
#include <QString>
#include <QMap>
#include <QDateTime>
#include <QFileSystemWatcher>
#include <QTimer>

class MarkdownEditor;

/**
 * @brief 多标签编辑器
 *
 * 功能：
 * - 同时打开和编辑多个文档
 * - 标签页管理（新建、关闭、重排序）
 * - 文件修改状态追踪
 * - 外部文件变化检测
 * - 最近关闭的标签恢复
 * - 标签页分组
 * - 快捷键导航
 */
class MultiTabEditor : public QWidget
{
    Q_OBJECT

public:
    // 标签页信息
    struct TabInfo {
        QString filePath;       // 文件路径（空表示新文件）
        QString title;          // 标签标题
        bool isModified;        // 是否已修改
        bool isNew;             // 是否是新文件
        int cursorPosition;     // 光标位置
        int scrollPosition;     // 滚动位置
        QDateTime lastModified; // 文件最后修改时间

        TabInfo()
            : isModified(false)
            , isNew(true)
            , cursorPosition(0)
            , scrollPosition(0)
        {}
    };

    // 关闭标签的历史记录
    struct ClosedTabRecord {
        QString filePath;
        QString content;
        int cursorPosition;
        int scrollPosition;
        QDateTime closedTime;
    };

    explicit MultiTabEditor(QWidget *parent = nullptr);
    ~MultiTabEditor();

    // 标签页管理
    int newTab(const QString &title = QString());
    int openFile(const QString &filePath);
    bool saveTab(int index = -1);
    bool saveTabAs(int index = -1);
    bool saveAllTabs();
    bool closeTab(int index = -1);
    bool closeAllTabs();
    bool closeOtherTabs(int index = -1);

    // 标签页信息
    int tabCount() const;
    int currentIndex() const;
    void setCurrentIndex(int index);
    QString currentFilePath() const;
    TabInfo tabInfo(int index) const;
    QList<TabInfo> allTabInfo() const;

    // 当前编辑器
    QPlainTextEdit* currentEditor() const;
    QPlainTextEdit* editorAt(int index) const;
    QString currentContent() const;
    void setCurrentContent(const QString &content);

    // 查找已打开的文件
    int findTab(const QString &filePath) const;
    bool isFileOpen(const QString &filePath) const;

    // 恢复关闭的标签
    bool canRestoreClosedTab() const;
    bool restoreClosedTab();
    QList<ClosedTabRecord> closedTabHistory() const;

    // 导航
    void nextTab();
    void previousTab();
    void goToTab(int index);

    // 配置
    void setMaxClosedTabHistory(int max);
    void setConfirmCloseModified(bool confirm);
    void setShowCloseButton(bool show);
    void setTabsMovable(bool movable);

    // 会话管理
    QByteArray saveSession() const;
    bool restoreSession(const QByteArray &data);

    // 外部文件变化
    void enableFileWatcher(bool enable);

signals:
    void tabCreated(int index);
    void tabClosed(int index);
    void tabChanged(int index);
    void tabMoved(int from, int to);
    void fileOpened(const QString &filePath);
    void fileSaved(const QString &filePath);
    void modificationChanged(int index, bool modified);
    void contentChanged(int index);
    void fileExternallyModified(const QString &filePath);
    void allTabsClosed();
    void currentEditorChanged(QPlainTextEdit *editor);

public slots:
    void onTabCloseRequested(int index);
    void onCurrentChanged(int index);
    void onTabMoved(int from, int to);

private slots:
    void onEditorTextChanged();
    void onEditorModificationChanged(bool modified);
    void onFileChanged(const QString &path);
    void checkExternalChanges();

private:
    QPlainTextEdit* createEditor();
    void updateTabTitle(int index);
    void updateTabIcon(int index);
    void addToClosedHistory(int index);
    bool promptSaveChanges(int index);
    QString generateNewTabTitle();
    void setupFileWatcher(const QString &filePath);
    void removeFileWatcher(const QString &filePath);

    QTabWidget *m_tabWidget;
    QMap<int, TabInfo> m_tabInfo;
    QList<ClosedTabRecord> m_closedHistory;
    QFileSystemWatcher *m_fileWatcher;
    QTimer *m_externalChangeTimer;

    int m_newTabCounter;
    int m_maxClosedHistory;
    bool m_confirmCloseModified;
    bool m_fileWatcherEnabled;

    // 用于检测外部变化
    QMap<QString, QDateTime> m_fileModTimes;
    QStringList m_pendingExternalChanges;
};

#endif // MULTITABEDITOR_H

