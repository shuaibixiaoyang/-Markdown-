// 文件说明：app-static\editor\multitabeditor.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "multitabeditor.h"

#include <QVBoxLayout>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <QStyle>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QScrollBar>
#include <QDebug>

// 函数说明：构造 MultiTabEditor 对象，初始化本模块需要的状态、界面和资源。
MultiTabEditor::MultiTabEditor(QWidget *parent)
    : QWidget(parent)
    , m_tabWidget(new QTabWidget(this))
    , m_fileWatcher(new QFileSystemWatcher(this))
    , m_externalChangeTimer(new QTimer(this))
    , m_newTabCounter(0)
    , m_maxClosedHistory(10)
    , m_confirmCloseModified(true)
    , m_fileWatcherEnabled(true)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_tabWidget);

    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);

    connect(m_tabWidget, &QTabWidget::tabCloseRequested,
            this, &MultiTabEditor::onTabCloseRequested);
    connect(m_tabWidget, &QTabWidget::currentChanged,
            this, &MultiTabEditor::onCurrentChanged);
    connect(m_tabWidget->tabBar(), &QTabBar::tabMoved,
            this, &MultiTabEditor::onTabMoved);

    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged,
            this, &MultiTabEditor::onFileChanged);

    m_externalChangeTimer->setSingleShot(true);
    m_externalChangeTimer->setInterval(500);
    connect(m_externalChangeTimer, &QTimer::timeout,
            this, &MultiTabEditor::checkExternalChanges);
}

// 函数说明：销毁 MultiTabEditor 对象，释放本模块持有的资源。
MultiTabEditor::~MultiTabEditor()
{
}

// 函数说明：实现 MultiTabEditor::newTab 的核心逻辑，供当前模块调用。
int MultiTabEditor::newTab(const QString &title)
{
    QPlainTextEdit *editor = createEditor();

    TabInfo info;
    info.title = title.isEmpty() ? generateNewTabTitle() : title;
    info.isNew = true;
    info.isModified = false;

    int index = m_tabWidget->addTab(editor, info.title);
    m_tabInfo[index] = info;

    m_tabWidget->setCurrentIndex(index);
    updateTabIcon(index);

    emit tabCreated(index);
    return index;
}

// 函数说明：打开 MultiTabEditor 对应的文件、资源或功能入口。
int MultiTabEditor::openFile(const QString &filePath)
{
    // 检查文件是否已打开
    int existingIndex = findTab(filePath);
    if (existingIndex >= 0) {
        m_tabWidget->setCurrentIndex(existingIndex);
        return existingIndex;
    }

    // 读取文件内容
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("打开文件失败"),
                            tr("无法打开文件: %1\n%2")
                            .arg(filePath)
                            .arg(file.errorString()));
        return -1;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    QString content = stream.readAll();
    file.close();

    // 创建新标签
    QPlainTextEdit *editor = createEditor();
    editor->setPlainText(content);
    editor->document()->setModified(false);

    QFileInfo fileInfo(filePath);
    TabInfo info;
    info.filePath = filePath;
    info.title = fileInfo.fileName();
    info.isNew = false;
    info.isModified = false;
    info.lastModified = fileInfo.lastModified();

    int index = m_tabWidget->addTab(editor, info.title);
    m_tabInfo[index] = info;

    // 设置文件监视
    if (m_fileWatcherEnabled) {
        setupFileWatcher(filePath);
    }
    m_fileModTimes[filePath] = fileInfo.lastModified();

    m_tabWidget->setCurrentIndex(index);
    updateTabIcon(index);

    emit tabCreated(index);
    emit fileOpened(filePath);

    return index;
}

// 函数说明：保存 MultiTabEditor 当前状态，保证用户修改可以持久化。
bool MultiTabEditor::saveTab(int index)
{
    if (index < 0) index = m_tabWidget->currentIndex();
    if (index < 0 || index >= m_tabWidget->count()) return false;

    TabInfo &info = m_tabInfo[index];

    if (info.isNew || info.filePath.isEmpty()) {
        return saveTabAs(index);
    }

    QPlainTextEdit *editor = editorAt(index);
    if (!editor) return false;

    // 临时移除文件监视
    removeFileWatcher(info.filePath);

    QFile file(info.filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("保存文件失败"),
                            tr("无法保存文件: %1\n%2")
                            .arg(info.filePath)
                            .arg(file.errorString()));
        setupFileWatcher(info.filePath);
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << editor->toPlainText();
    file.close();

    // 更新信息
    QFileInfo fileInfo(info.filePath);
    info.isModified = false;
    info.lastModified = fileInfo.lastModified();
    m_fileModTimes[info.filePath] = info.lastModified;

    editor->document()->setModified(false);
    updateTabTitle(index);
    updateTabIcon(index);

    // 重新设置文件监视
    setupFileWatcher(info.filePath);

    emit fileSaved(info.filePath);
    return true;
}

// 函数说明：保存 MultiTabEditor 当前状态，保证用户修改可以持久化。
bool MultiTabEditor::saveTabAs(int index)
{
    if (index < 0) index = m_tabWidget->currentIndex();
    if (index < 0 || index >= m_tabWidget->count()) return false;

    QString filePath = QFileDialog::getSaveFileName(this,
        tr("另存为"),
        QString(),
        tr("Markdown 文件 (*.md *.markdown);;所有文件 (*)"));

    if (filePath.isEmpty()) return false;

    TabInfo &info = m_tabInfo[index];

    // 移除旧的文件监视
    if (!info.filePath.isEmpty()) {
        removeFileWatcher(info.filePath);
        m_fileModTimes.remove(info.filePath);
    }

    info.filePath = filePath;
    info.isNew = false;

    QFileInfo fileInfo(filePath);
    info.title = fileInfo.fileName();
    m_tabWidget->setTabText(index, info.title);

    return saveTab(index);
}

// 函数说明：保存 MultiTabEditor 当前状态，保证用户修改可以持久化。
bool MultiTabEditor::saveAllTabs()
{
    bool allSaved = true;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (m_tabInfo[i].isModified) {
            if (!saveTab(i)) {
                allSaved = false;
            }
        }
    }
    return allSaved;
}

// 函数说明：关闭 MultiTabEditor 相关窗口或资源，并处理必要的保存确认。
bool MultiTabEditor::closeTab(int index)
{
    if (index < 0) index = m_tabWidget->currentIndex();
    if (index < 0 || index >= m_tabWidget->count()) return false;

    TabInfo &info = m_tabInfo[index];

    // 检查是否需要保存
    if (info.isModified && m_confirmCloseModified) {
        if (!promptSaveChanges(index)) {
            return false;
        }
    }

    // 保存到关闭历史
    addToClosedHistory(index);

    // 移除文件监视
    if (!info.filePath.isEmpty()) {
        removeFileWatcher(info.filePath);
        m_fileModTimes.remove(info.filePath);
    }

    // 移除标签
    QWidget *widget = m_tabWidget->widget(index);
    m_tabWidget->removeTab(index);
    delete widget;

    // 更新标签信息映射
    QMap<int, TabInfo> newTabInfo;
    for (auto it = m_tabInfo.begin(); it != m_tabInfo.end(); ++it) {
        if (it.key() < index) {
            newTabInfo[it.key()] = it.value();
        } else if (it.key() > index) {
            newTabInfo[it.key() - 1] = it.value();
        }
    }
    m_tabInfo = newTabInfo;

    emit tabClosed(index);

    if (m_tabWidget->count() == 0) {
        emit allTabsClosed();
    }

    return true;
}

// 函数说明：关闭 MultiTabEditor 相关窗口或资源，并处理必要的保存确认。
bool MultiTabEditor::closeAllTabs()
{
    while (m_tabWidget->count() > 0) {
        if (!closeTab(0)) {
            return false;
        }
    }
    return true;
}

// 函数说明：关闭 MultiTabEditor 相关窗口或资源，并处理必要的保存确认。
bool MultiTabEditor::closeOtherTabs(int index)
{
    if (index < 0) index = m_tabWidget->currentIndex();
    if (index < 0 || index >= m_tabWidget->count()) return false;

    // 先关闭后面的标签
    while (m_tabWidget->count() > index + 1) {
        if (!closeTab(index + 1)) {
            return false;
        }
    }

    // 再关闭前面的标签
    while (index > 0) {
        if (!closeTab(0)) {
            return false;
        }
        index--;
    }

    return true;
}

// 函数说明：实现 MultiTabEditor::tabCount 的核心逻辑，供当前模块调用。
int MultiTabEditor::tabCount() const
{
    return m_tabWidget->count();
}

// 函数说明：实现 MultiTabEditor::currentIndex 的核心逻辑，供当前模块调用。
int MultiTabEditor::currentIndex() const
{
    return m_tabWidget->currentIndex();
}

// 函数说明：设置 MultiTabEditor 的运行参数，并触发必要的界面或数据刷新。
void MultiTabEditor::setCurrentIndex(int index)
{
    if (index >= 0 && index < m_tabWidget->count()) {
        m_tabWidget->setCurrentIndex(index);
    }
}

// 函数说明：实现 MultiTabEditor::currentFilePath 的核心逻辑，供当前模块调用。
QString MultiTabEditor::currentFilePath() const
{
    int index = m_tabWidget->currentIndex();
    if (index >= 0 && m_tabInfo.contains(index)) {
        return m_tabInfo[index].filePath;
    }
    return QString();
}

// 函数说明：实现 MultiTabEditor::tabInfo 的核心逻辑，供当前模块调用。
MultiTabEditor::TabInfo MultiTabEditor::tabInfo(int index) const
{
    if (m_tabInfo.contains(index)) {
        return m_tabInfo[index];
    }
    return TabInfo();
}

// 函数说明：实现 MultiTabEditor::allTabInfo 的核心逻辑，供当前模块调用。
QList<MultiTabEditor::TabInfo> MultiTabEditor::allTabInfo() const
{
    QList<TabInfo> result;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (m_tabInfo.contains(i)) {
            result.append(m_tabInfo[i]);
        }
    }
    return result;
}

// 函数说明：实现 MultiTabEditor::currentEditor 的核心逻辑，供当前模块调用。
QPlainTextEdit* MultiTabEditor::currentEditor() const
{
    return qobject_cast<QPlainTextEdit*>(m_tabWidget->currentWidget());
}

// 函数说明：处理编辑命令，将用户操作转换为 Markdown 文本变更。
QPlainTextEdit* MultiTabEditor::editorAt(int index) const
{
    return qobject_cast<QPlainTextEdit*>(m_tabWidget->widget(index));
}

// 函数说明：实现 MultiTabEditor::currentContent 的核心逻辑，供当前模块调用。
QString MultiTabEditor::currentContent() const
{
    QPlainTextEdit *editor = currentEditor();
    return editor ? editor->toPlainText() : QString();
}

// 函数说明：设置 MultiTabEditor 的运行参数，并触发必要的界面或数据刷新。
void MultiTabEditor::setCurrentContent(const QString &content)
{
    QPlainTextEdit *editor = currentEditor();
    if (editor) {
        editor->setPlainText(content);
    }
}

// 函数说明：实现 MultiTabEditor::findTab 的核心逻辑，供当前模块调用。
int MultiTabEditor::findTab(const QString &filePath) const
{
    for (auto it = m_tabInfo.begin(); it != m_tabInfo.end(); ++it) {
        if (it.value().filePath == filePath) {
            return it.key();
        }
    }
    return -1;
}

// 函数说明：判断 MultiTabEditor 当前是否满足指定状态。
bool MultiTabEditor::isFileOpen(const QString &filePath) const
{
    return findTab(filePath) >= 0;
}

// 函数说明：实现 MultiTabEditor::canRestoreClosedTab 的核心逻辑，供当前模块调用。
bool MultiTabEditor::canRestoreClosedTab() const
{
    return !m_closedHistory.isEmpty();
}

// 函数说明：实现 MultiTabEditor::restoreClosedTab 的核心逻辑，供当前模块调用。
bool MultiTabEditor::restoreClosedTab()
{
    if (m_closedHistory.isEmpty()) return false;

    ClosedTabRecord record = m_closedHistory.takeLast();

    int index;
    if (!record.filePath.isEmpty()) {
        // 尝试重新打开文件
        QFile file(record.filePath);
        if (file.exists()) {
            index = openFile(record.filePath);
        } else {
            // 文件不存在，使用保存的内容创建新标签
            index = newTab(QFileInfo(record.filePath).fileName());
            QPlainTextEdit *editor = editorAt(index);
            if (editor) {
                editor->setPlainText(record.content);
            }
        }
    } else {
        // 新文件，使用保存的内容
        index = newTab();
        QPlainTextEdit *editor = editorAt(index);
        if (editor) {
            editor->setPlainText(record.content);
        }
    }

    // 恢复光标和滚动位置
    if (index >= 0) {
        QPlainTextEdit *editor = editorAt(index);
        if (editor) {
            QTextCursor cursor = editor->textCursor();
            cursor.setPosition(qMin(record.cursorPosition, editor->toPlainText().length()));
            editor->setTextCursor(cursor);
            editor->verticalScrollBar()->setValue(record.scrollPosition);
        }
    }

    return index >= 0;
}

// 函数说明：关闭 MultiTabEditor 相关窗口或资源，并处理必要的保存确认。
QList<MultiTabEditor::ClosedTabRecord> MultiTabEditor::closedTabHistory() const
{
    return m_closedHistory;
}

// 函数说明：实现 MultiTabEditor::nextTab 的核心逻辑，供当前模块调用。
void MultiTabEditor::nextTab()
{
    int current = m_tabWidget->currentIndex();
    int count = m_tabWidget->count();
    if (count > 1) {
        m_tabWidget->setCurrentIndex((current + 1) % count);
    }
}

// 函数说明：实现 MultiTabEditor::previousTab 的核心逻辑，供当前模块调用。
void MultiTabEditor::previousTab()
{
    int current = m_tabWidget->currentIndex();
    int count = m_tabWidget->count();
    if (count > 1) {
        m_tabWidget->setCurrentIndex((current - 1 + count) % count);
    }
}

// 函数说明：实现 MultiTabEditor::goToTab 的核心逻辑，供当前模块调用。
void MultiTabEditor::goToTab(int index)
{
    setCurrentIndex(index);
}

// 函数说明：设置 MultiTabEditor 的运行参数，并触发必要的界面或数据刷新。
void MultiTabEditor::setMaxClosedTabHistory(int max)
{
    m_maxClosedHistory = max;
    while (m_closedHistory.size() > max) {
        m_closedHistory.removeFirst();
    }
}

// 函数说明：设置 MultiTabEditor 的运行参数，并触发必要的界面或数据刷新。
void MultiTabEditor::setConfirmCloseModified(bool confirm)
{
    m_confirmCloseModified = confirm;
}

// 函数说明：设置 MultiTabEditor 的运行参数，并触发必要的界面或数据刷新。
void MultiTabEditor::setShowCloseButton(bool show)
{
    m_tabWidget->setTabsClosable(show);
}

// 函数说明：设置 MultiTabEditor 的运行参数，并触发必要的界面或数据刷新。
void MultiTabEditor::setTabsMovable(bool movable)
{
    m_tabWidget->setMovable(movable);
}

// 函数说明：保存 MultiTabEditor 当前状态，保证用户修改可以持久化。
QByteArray MultiTabEditor::saveSession() const
{
    QJsonArray tabs;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        const TabInfo &info = m_tabInfo[i];
        QPlainTextEdit *editor = editorAt(i);

        QJsonObject tabObj;
        tabObj["filePath"] = info.filePath;
        tabObj["title"] = info.title;
        tabObj["isNew"] = info.isNew;
        tabObj["cursorPosition"] = editor ? editor->textCursor().position() : 0;
        tabObj["scrollPosition"] = editor ? editor->verticalScrollBar()->value() : 0;

        if (info.isNew && editor) {
            tabObj["content"] = editor->toPlainText();
        }

        tabs.append(tabObj);
    }

    QJsonObject session;
    session["tabs"] = tabs;
    session["currentIndex"] = m_tabWidget->currentIndex();

    return QJsonDocument(session).toJson(QJsonDocument::Compact);
}

// 函数说明：实现 MultiTabEditor::restoreSession 的核心逻辑，供当前模块调用。
bool MultiTabEditor::restoreSession(const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        return false;
    }

    QJsonObject session = doc.object();
    QJsonArray tabs = session["tabs"].toArray();

    for (const QJsonValue &tabVal : tabs) {
        QJsonObject tabObj = tabVal.toObject();
        QString filePath = tabObj["filePath"].toString();
        bool isNew = tabObj["isNew"].toBool();
        int cursorPos = tabObj["cursorPosition"].toInt();
        int scrollPos = tabObj["scrollPosition"].toInt();

        int index = -1;
        if (!isNew && !filePath.isEmpty()) {
            index = openFile(filePath);
        } else {
            QString title = tabObj["title"].toString();
            index = newTab(title);
            if (tabObj.contains("content")) {
                QPlainTextEdit *editor = editorAt(index);
                if (editor) {
                    editor->setPlainText(tabObj["content"].toString());
                }
            }
        }

        if (index >= 0) {
            QPlainTextEdit *editor = editorAt(index);
            if (editor) {
                QTextCursor cursor = editor->textCursor();
                cursor.setPosition(qMin(cursorPos, editor->toPlainText().length()));
                editor->setTextCursor(cursor);
                editor->verticalScrollBar()->setValue(scrollPos);
            }
        }
    }

    int currentIndex = session["currentIndex"].toInt();
    if (currentIndex >= 0 && currentIndex < m_tabWidget->count()) {
        m_tabWidget->setCurrentIndex(currentIndex);
    }

    return true;
}

// 函数说明：实现 MultiTabEditor::enableFileWatcher 的核心逻辑，供当前模块调用。
void MultiTabEditor::enableFileWatcher(bool enable)
{
    m_fileWatcherEnabled = enable;

    if (!enable) {
        // 移除所有文件监视
        QStringList paths = m_fileWatcher->files();
        if (!paths.isEmpty()) {
            m_fileWatcher->removePaths(paths);
        }
    } else {
        // 重新添加文件监视
        for (auto it = m_tabInfo.begin(); it != m_tabInfo.end(); ++it) {
            if (!it.value().filePath.isEmpty()) {
                setupFileWatcher(it.value().filePath);
            }
        }
    }
}

// 函数说明：响应 MultiTabEditor 收到的信号或异步回调，并更新界面状态。
void MultiTabEditor::onTabCloseRequested(int index)
{
    closeTab(index);
}

// 函数说明：响应 MultiTabEditor 收到的信号或异步回调，并更新界面状态。
void MultiTabEditor::onCurrentChanged(int index)
{
    emit tabChanged(index);
    emit currentEditorChanged(currentEditor());
}

// 函数说明：响应 MultiTabEditor 收到的信号或异步回调，并更新界面状态。
void MultiTabEditor::onTabMoved(int from, int to)
{
    // 更新标签信息映射
    TabInfo info = m_tabInfo[from];

    if (from < to) {
        for (int i = from; i < to; ++i) {
            m_tabInfo[i] = m_tabInfo[i + 1];
        }
    } else {
        for (int i = from; i > to; --i) {
            m_tabInfo[i] = m_tabInfo[i - 1];
        }
    }

    m_tabInfo[to] = info;
    emit tabMoved(from, to);
}

// 函数说明：响应 MultiTabEditor 收到的信号或异步回调，并更新界面状态。
void MultiTabEditor::onEditorTextChanged()
{
    int index = m_tabWidget->currentIndex();
    if (index >= 0 && m_tabInfo.contains(index)) {
        emit contentChanged(index);
    }
}

// 函数说明：响应 MultiTabEditor 收到的信号或异步回调，并更新界面状态。
void MultiTabEditor::onEditorModificationChanged(bool modified)
{
    QPlainTextEdit *editor = qobject_cast<QPlainTextEdit*>(sender());
    if (!editor) return;

    int index = m_tabWidget->indexOf(editor);
    if (index >= 0 && m_tabInfo.contains(index)) {
        m_tabInfo[index].isModified = modified;
        updateTabTitle(index);
        updateTabIcon(index);
        emit modificationChanged(index, modified);
    }
}

// 函数说明：响应 MultiTabEditor 收到的信号或异步回调，并更新界面状态。
void MultiTabEditor::onFileChanged(const QString &path)
{
    if (!m_pendingExternalChanges.contains(path)) {
        m_pendingExternalChanges.append(path);
    }
    m_externalChangeTimer->start();
}

// 函数说明：实现 MultiTabEditor::checkExternalChanges 的核心逻辑，供当前模块调用。
void MultiTabEditor::checkExternalChanges()
{
    for (const QString &path : m_pendingExternalChanges) {
        QFileInfo fileInfo(path);

        // 检查文件是否被删除
        if (!fileInfo.exists()) {
            int index = findTab(path);
            if (index >= 0) {
                m_tabInfo[index].isModified = true;
                updateTabTitle(index);
            }
            continue;
        }

        // 检查修改时间
        QDateTime newModTime = fileInfo.lastModified();
        QDateTime oldModTime = m_fileModTimes.value(path);

        if (newModTime != oldModTime) {
            m_fileModTimes[path] = newModTime;
            emit fileExternallyModified(path);

            int index = findTab(path);
            if (index >= 0) {
                // 显示提示对话框
                QMessageBox::StandardButton reply = QMessageBox::question(this,
                    tr("文件已被修改"),
                    tr("文件 \"%1\" 在外部被修改。\n是否重新加载？")
                    .arg(QFileInfo(path).fileName()),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::Yes);

                if (reply == QMessageBox::Yes) {
                    // 重新加载文件
                    QFile file(path);
                    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        QTextStream stream(&file);
                        stream.setEncoding(QStringConverter::Utf8);
                        QString content = stream.readAll();
                        file.close();

                        QPlainTextEdit *editor = editorAt(index);
                        if (editor) {
                            int cursorPos = editor->textCursor().position();
                            editor->setPlainText(content);
                            editor->document()->setModified(false);

                            QTextCursor cursor = editor->textCursor();
                            cursor.setPosition(qMin(cursorPos, content.length()));
                            editor->setTextCursor(cursor);
                        }

                        m_tabInfo[index].isModified = false;
                        m_tabInfo[index].lastModified = newModTime;
                        updateTabTitle(index);
                        updateTabIcon(index);
                    }
                }
            }
        }

        // 重新添加监视（因为文件修改后监视会被移除）
        setupFileWatcher(path);
    }

    m_pendingExternalChanges.clear();
}

// 函数说明：创建 MultiTabEditor 需要的对象、记录或输出内容。
QPlainTextEdit* MultiTabEditor::createEditor()
{
    QPlainTextEdit *editor = new QPlainTextEdit(this);

    // 设置基本属性
    editor->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    editor->setTabStopDistance(40);

    // 连接信号
    connect(editor, &QPlainTextEdit::textChanged,
            this, &MultiTabEditor::onEditorTextChanged);
    connect(editor->document(), &QTextDocument::modificationChanged,
            this, &MultiTabEditor::onEditorModificationChanged);

    return editor;
}

// 函数说明：刷新 MultiTabEditor 的内部状态，并同步到相关界面。
void MultiTabEditor::updateTabTitle(int index)
{
    if (!m_tabInfo.contains(index)) return;

    const TabInfo &info = m_tabInfo[index];
    QString title = info.title;

    if (info.isModified) {
        title += " *";
    }

    m_tabWidget->setTabText(index, title);
}

// 函数说明：刷新 MultiTabEditor 的内部状态，并同步到相关界面。
void MultiTabEditor::updateTabIcon(int index)
{
    if (!m_tabInfo.contains(index)) return;

    const TabInfo &info = m_tabInfo[index];
    QIcon icon;

    if (info.isModified) {
        icon = QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton);
    } else if (info.isNew) {
        icon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
    } else {
        icon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
    }

    m_tabWidget->setTabIcon(index, icon);
}

// 函数说明：向 MultiTabEditor 管理的数据集合中添加一项内容。
void MultiTabEditor::addToClosedHistory(int index)
{
    if (!m_tabInfo.contains(index)) return;

    const TabInfo &info = m_tabInfo[index];
    QPlainTextEdit *editor = editorAt(index);

    ClosedTabRecord record;
    record.filePath = info.filePath;
    record.content = editor ? editor->toPlainText() : QString();
    record.cursorPosition = editor ? editor->textCursor().position() : 0;
    record.scrollPosition = editor ? editor->verticalScrollBar()->value() : 0;
    record.closedTime = QDateTime::currentDateTime();

    m_closedHistory.append(record);

    // 限制历史记录数量
    while (m_closedHistory.size() > m_maxClosedHistory) {
        m_closedHistory.removeFirst();
    }
}

// 函数说明：实现 MultiTabEditor::promptSaveChanges 的核心逻辑，供当前模块调用。
bool MultiTabEditor::promptSaveChanges(int index)
{
    if (!m_tabInfo.contains(index)) return true;

    const TabInfo &info = m_tabInfo[index];

    QMessageBox::StandardButton reply = QMessageBox::question(this,
        tr("保存更改"),
        tr("文件 \"%1\" 已被修改。\n是否保存更改？").arg(info.title),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);

    if (reply == QMessageBox::Save) {
        return saveTab(index);
    } else if (reply == QMessageBox::Cancel) {
        return false;
    }

    return true;  // Discard
}

// 函数说明：根据当前数据生成 MultiTabEditor 需要的输出结果。
QString MultiTabEditor::generateNewTabTitle()
{
    return tr("未命名 %1").arg(++m_newTabCounter);
}

// 函数说明：初始化 MultiTabEditor 的 setupFileWatcher 相关界面、动作或服务连接。
void MultiTabEditor::setupFileWatcher(const QString &filePath)
{
    if (!m_fileWatcherEnabled || filePath.isEmpty()) return;

    QStringList paths = m_fileWatcher->files();
    if (!paths.contains(filePath)) {
        m_fileWatcher->addPath(filePath);
    }
}

// 函数说明：从 MultiTabEditor 管理的数据集合中移除指定内容。
void MultiTabEditor::removeFileWatcher(const QString &filePath)
{
    if (filePath.isEmpty()) return;

    QStringList paths = m_fileWatcher->files();
    if (paths.contains(filePath)) {
        m_fileWatcher->removePath(filePath);
    }
}

