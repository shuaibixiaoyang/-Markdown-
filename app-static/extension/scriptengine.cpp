// 文件说明：app-static\extension\scriptengine.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "scriptengine.h"

#include <QPlainTextEdit>
#include <QMainWindow>
#include <QTextCursor>
#include <QTextBlock>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <QMessageBox>
#include <QInputDialog>
#include <QClipboard>
#include <QApplication>
#include <QStandardPaths>
#include <QDateTime>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QThread>
#include <QDebug>

// ===================== EditorAPI =====================

EditorAPI::EditorAPI(QPlainTextEdit *editor, QObject *parent)
    : QObject(parent)
    , m_editor(editor)
{
}

// 函数说明：实现 EditorAPI::text 的核心逻辑，供当前模块调用。
QString EditorAPI::text() const
{
    return m_editor ? m_editor->toPlainText() : QString();
}

// 函数说明：设置 EditorAPI 的运行参数，并触发必要的界面或数据刷新。
void EditorAPI::setText(const QString &text)
{
    if (m_editor) {
        m_editor->setPlainText(text);
    }
}

// 函数说明：实现 EditorAPI::selectedText 的核心逻辑，供当前模块调用。
QString EditorAPI::selectedText() const
{
    return m_editor ? m_editor->textCursor().selectedText() : QString();
}

// 函数说明：实现 EditorAPI::cursorPosition 的核心逻辑，供当前模块调用。
int EditorAPI::cursorPosition() const
{
    return m_editor ? m_editor->textCursor().position() : 0;
}

// 函数说明：设置 EditorAPI 的运行参数，并触发必要的界面或数据刷新。
void EditorAPI::setCursorPosition(int pos)
{
    if (m_editor) {
        QTextCursor cursor = m_editor->textCursor();
        cursor.setPosition(pos);
        m_editor->setTextCursor(cursor);
    }
}

// 函数说明：实现 EditorAPI::lineNumber 的核心逻辑，供当前模块调用。
int EditorAPI::lineNumber() const
{
    return m_editor ? m_editor->textCursor().blockNumber() + 1 : 0;
}

// 函数说明：实现 EditorAPI::columnNumber 的核心逻辑，供当前模块调用。
int EditorAPI::columnNumber() const
{
    return m_editor ? m_editor->textCursor().columnNumber() + 1 : 0;
}

// 函数说明：实现 EditorAPI::insertText 的核心逻辑，供当前模块调用。
void EditorAPI::insertText(const QString &text)
{
    if (m_editor) {
        m_editor->textCursor().insertText(text);
    }
}

// 函数说明：实现 EditorAPI::replaceSelection 的核心逻辑，供当前模块调用。
void EditorAPI::replaceSelection(const QString &text)
{
    if (m_editor) {
        QTextCursor cursor = m_editor->textCursor();
        cursor.insertText(text);
    }
}

// 函数说明：删除 EditorAPI 管理的指定数据或资源。
void EditorAPI::deleteSelection()
{
    if (m_editor) {
        m_editor->textCursor().removeSelectedText();
    }
}

// 函数说明：实现 EditorAPI::selectAll 的核心逻辑，供当前模块调用。
void EditorAPI::selectAll()
{
    if (m_editor) {
        m_editor->selectAll();
    }
}

// 函数说明：实现 EditorAPI::selectLine 的核心逻辑，供当前模块调用。
void EditorAPI::selectLine(int line)
{
    if (m_editor && line > 0 && line <= m_editor->blockCount()) {
        QTextBlock block = m_editor->document()->findBlockByLineNumber(line - 1);
        QTextCursor cursor(block);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        m_editor->setTextCursor(cursor);
    }
}

// 函数说明：实现 EditorAPI::selectRange 的核心逻辑，供当前模块调用。
void EditorAPI::selectRange(int start, int end)
{
    if (m_editor) {
        QTextCursor cursor = m_editor->textCursor();
        cursor.setPosition(start);
        cursor.setPosition(end, QTextCursor::KeepAnchor);
        m_editor->setTextCursor(cursor);
    }
}

// 函数说明：实现 EditorAPI::moveCursor 的核心逻辑，供当前模块调用。
void EditorAPI::moveCursor(int position)
{
    setCursorPosition(position);
}

// 函数说明：实现 EditorAPI::moveToLine 的核心逻辑，供当前模块调用。
void EditorAPI::moveToLine(int line)
{
    if (m_editor && line > 0 && line <= m_editor->blockCount()) {
        QTextBlock block = m_editor->document()->findBlockByLineNumber(line - 1);
        QTextCursor cursor(block);
        m_editor->setTextCursor(cursor);
    }
}

// 函数说明：实现 EditorAPI::moveToLineStart 的核心逻辑，供当前模块调用。
void EditorAPI::moveToLineStart()
{
    if (m_editor) {
        QTextCursor cursor = m_editor->textCursor();
        cursor.movePosition(QTextCursor::StartOfBlock);
        m_editor->setTextCursor(cursor);
    }
}

// 函数说明：实现 EditorAPI::moveToLineEnd 的核心逻辑，供当前模块调用。
void EditorAPI::moveToLineEnd()
{
    if (m_editor) {
        QTextCursor cursor = m_editor->textCursor();
        cursor.movePosition(QTextCursor::EndOfBlock);
        m_editor->setTextCursor(cursor);
    }
}

// 函数说明：实现 EditorAPI::moveToDocumentStart 的核心逻辑，供当前模块调用。
void EditorAPI::moveToDocumentStart()
{
    if (m_editor) {
        QTextCursor cursor = m_editor->textCursor();
        cursor.movePosition(QTextCursor::Start);
        m_editor->setTextCursor(cursor);
    }
}

// 函数说明：实现 EditorAPI::moveToDocumentEnd 的核心逻辑，供当前模块调用。
void EditorAPI::moveToDocumentEnd()
{
    if (m_editor) {
        QTextCursor cursor = m_editor->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_editor->setTextCursor(cursor);
    }
}

// 函数说明：读取 EditorAPI 当前保存的状态或计算结果。
QString EditorAPI::getLine(int line)
{
    if (m_editor && line > 0 && line <= m_editor->blockCount()) {
        return m_editor->document()->findBlockByLineNumber(line - 1).text();
    }
    return QString();
}

// 函数说明：实现 EditorAPI::insertLine 的核心逻辑，供当前模块调用。
void EditorAPI::insertLine(int line, const QString &text)
{
    if (m_editor && line > 0) {
        QTextCursor cursor = m_editor->textCursor();
        if (line <= m_editor->blockCount()) {
            QTextBlock block = m_editor->document()->findBlockByLineNumber(line - 1);
            cursor.setPosition(block.position());
        } else {
            cursor.movePosition(QTextCursor::End);
            cursor.insertText("\n");
        }
        cursor.insertText(text + "\n");
    }
}

// 函数说明：删除 EditorAPI 管理的指定数据或资源。
void EditorAPI::deleteLine(int line)
{
    if (m_editor && line > 0 && line <= m_editor->blockCount()) {
        QTextBlock block = m_editor->document()->findBlockByLineNumber(line - 1);
        QTextCursor cursor(block);
        cursor.select(QTextCursor::BlockUnderCursor);
        cursor.removeSelectedText();
        if (!cursor.atEnd()) {
            cursor.deleteChar(); // 删除换行符
        }
    }
}

// 函数说明：实现 EditorAPI::replaceLine 的核心逻辑，供当前模块调用。
void EditorAPI::replaceLine(int line, const QString &text)
{
    if (m_editor && line > 0 && line <= m_editor->blockCount()) {
        QTextBlock block = m_editor->document()->findBlockByLineNumber(line - 1);
        QTextCursor cursor(block);
        cursor.select(QTextCursor::BlockUnderCursor);
        cursor.insertText(text);
    }
}

// 函数说明：实现 EditorAPI::lineCount 的核心逻辑，供当前模块调用。
int EditorAPI::lineCount()
{
    return m_editor ? m_editor->blockCount() : 0;
}

// 函数说明：实现 EditorAPI::find 的核心逻辑，供当前模块调用。
bool EditorAPI::find(const QString &text, bool caseSensitive, bool wholeWord)
{
    if (!m_editor) return false;

    QTextDocument::FindFlags flags;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;
    if (wholeWord) flags |= QTextDocument::FindWholeWords;

    return m_editor->find(text, flags);
}

// 函数说明：实现 EditorAPI::replaceAll 的核心逻辑，供当前模块调用。
int EditorAPI::replaceAll(const QString &find, const QString &replace, bool caseSensitive)
{
    if (!m_editor) return 0;

    QString text = m_editor->toPlainText();
    int count = 0;

    if (caseSensitive) {
        count = text.count(find);
        text.replace(find, replace);
    } else {
        QRegularExpression regex(QRegularExpression::escape(find),
                                 QRegularExpression::CaseInsensitiveOption);
        count = text.count(regex);
        text.replace(regex, replace);
    }

    if (count > 0) {
        m_editor->setPlainText(text);
    }

    return count;
}

// 函数说明：实现 EditorAPI::copy 的核心逻辑，供当前模块调用。
void EditorAPI::copy()
{
    if (m_editor) {
        m_editor->copy();
    }
}

// 函数说明：实现 EditorAPI::cut 的核心逻辑，供当前模块调用。
void EditorAPI::cut()
{
    if (m_editor) {
        m_editor->cut();
    }
}

// 函数说明：实现 EditorAPI::paste 的核心逻辑，供当前模块调用。
void EditorAPI::paste()
{
    if (m_editor) {
        m_editor->paste();
    }
}

// 函数说明：实现 EditorAPI::undo 的核心逻辑，供当前模块调用。
void EditorAPI::undo()
{
    if (m_editor) {
        m_editor->undo();
    }
}

// 函数说明：实现 EditorAPI::redo 的核心逻辑，供当前模块调用。
void EditorAPI::redo()
{
    if (m_editor) {
        m_editor->redo();
    }
}

// ===================== AppAPI =====================

AppAPI::AppAPI(QObject *parent)
    : QObject(parent)
    , m_mainWindow(nullptr)
{
}

// 函数说明：设置 AppAPI 的运行参数，并触发必要的界面或数据刷新。
void AppAPI::setMainWindow(QMainWindow *window)
{
    m_mainWindow = window;
}

// 函数说明：设置 AppAPI 的运行参数，并触发必要的界面或数据刷新。
void AppAPI::setFilePath(const QString &path)
{
    m_filePath = path;
}

// 函数说明：处理主窗口的文件菜单动作，衔接文档读写和界面状态。
QString AppAPI::filePath() const
{
    return m_filePath;
}

// 函数说明：处理主窗口的文件菜单动作，衔接文档读写和界面状态。
QString AppAPI::fileName() const
{
    return QFileInfo(m_filePath).fileName();
}

// 函数说明：实现 AppAPI::newFile 的核心逻辑，供当前模块调用。
void AppAPI::newFile()
{
    emit requestNewFile();
}

// 函数说明：打开 AppAPI 对应的文件、资源或功能入口。
void AppAPI::openFile(const QString &path)
{
    emit requestOpenFile(path);
}

// 函数说明：保存 AppAPI 当前状态，保证用户修改可以持久化。
void AppAPI::saveFile()
{
    emit requestSaveFile();
}

// 函数说明：保存 AppAPI 当前状态，保证用户修改可以持久化。
void AppAPI::saveFileAs(const QString &path)
{
    emit requestSaveFileAs(path);
}

// 函数说明：显示 AppAPI 管理的面板、对话框或提示信息。
void AppAPI::showMessage(const QString &message, int timeout)
{
    emit requestShowMessage(message, timeout);
}

// 函数说明：显示 AppAPI 管理的面板、对话框或提示信息。
void AppAPI::showInfo(const QString &title, const QString &message)
{
    QMessageBox::information(m_mainWindow, title, message);
}

// 函数说明：显示 AppAPI 管理的面板、对话框或提示信息。
void AppAPI::showWarning(const QString &title, const QString &message)
{
    QMessageBox::warning(m_mainWindow, title, message);
}

// 函数说明：显示 AppAPI 管理的面板、对话框或提示信息。
void AppAPI::showError(const QString &title, const QString &message)
{
    QMessageBox::critical(m_mainWindow, title, message);
}

// 函数说明：实现 AppAPI::inputText 的核心逻辑，供当前模块调用。
QString AppAPI::inputText(const QString &title, const QString &label, const QString &defaultValue)
{
    bool ok;
    QString text = QInputDialog::getText(m_mainWindow, title, label,
                                        QLineEdit::Normal, defaultValue, &ok);
    return ok ? text : QString();
}

// 函数说明：实现 AppAPI::confirm 的核心逻辑，供当前模块调用。
bool AppAPI::confirm(const QString &title, const QString &message)
{
    return QMessageBox::question(m_mainWindow, title, message,
                                QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
}

// 函数说明：实现 AppAPI::log 的核心逻辑，供当前模块调用。
void AppAPI::log(const QString &message)
{
    qDebug() << "[Script]" << message;
}

// ===================== UtilAPI =====================

UtilAPI::UtilAPI(QObject *parent)
    : QObject(parent)
{
}

// 函数说明：实现 UtilAPI::trim 的核心逻辑，供当前模块调用。
QString UtilAPI::trim(const QString &text)
{
    return text.trimmed();
}

// 函数说明：实现 UtilAPI::toUpperCase 的核心逻辑，供当前模块调用。
QString UtilAPI::toUpperCase(const QString &text)
{
    return text.toUpper();
}

// 函数说明：实现 UtilAPI::toLowerCase 的核心逻辑，供当前模块调用。
QString UtilAPI::toLowerCase(const QString &text)
{
    return text.toLower();
}

// 函数说明：实现 UtilAPI::repeat 的核心逻辑，供当前模块调用。
QString UtilAPI::repeat(const QString &text, int count)
{
    return text.repeated(count);
}

// 函数说明：实现 UtilAPI::split 的核心逻辑，供当前模块调用。
QStringList UtilAPI::split(const QString &text, const QString &separator)
{
    return text.split(separator);
}

// 函数说明：实现 UtilAPI::join 的核心逻辑，供当前模块调用。
QString UtilAPI::join(const QStringList &list, const QString &separator)
{
    return list.join(separator);
}

// 函数说明：实现 UtilAPI::matches 的核心逻辑，供当前模块调用。
bool UtilAPI::matches(const QString &text, const QString &pattern)
{
    QRegularExpression regex(pattern);
    return regex.match(text).hasMatch();
}

// 函数说明：实现 UtilAPI::replace 的核心逻辑，供当前模块调用。
QString UtilAPI::replace(const QString &text, const QString &pattern, const QString &replacement)
{
    QRegularExpression regex(pattern);
    QString result = text;
    result.replace(regex, replacement);
    return result;
}

// 函数说明：实现 UtilAPI::findAll 的核心逻辑，供当前模块调用。
QStringList UtilAPI::findAll(const QString &text, const QString &pattern)
{
    QStringList results;
    QRegularExpression regex(pattern);
    QRegularExpressionMatchIterator it = regex.globalMatch(text);
    while (it.hasNext()) {
        results.append(it.next().captured());
    }
    return results;
}

// 函数说明：读取 UtilAPI 的配置或数据，并同步到运行时状态。
QString UtilAPI::readFile(const QString &path)
{
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QTextStream(&file).readAll();
    }
    return QString();
}

// 函数说明：写入 UtilAPI 的配置或数据，用于下次启动恢复。
bool UtilAPI::writeFile(const QString &path, const QString &content)
{
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << content;
        return true;
    }
    return false;
}

// 函数说明：处理主窗口的文件菜单动作，衔接文档读写和界面状态。
bool UtilAPI::fileExists(const QString &path)
{
    return QFile::exists(path);
}

// 函数说明：实现 UtilAPI::listFiles 的核心逻辑，供当前模块调用。
QStringList UtilAPI::listFiles(const QString &dir, const QString &filter)
{
    QDir d(dir);
    return d.entryList(QStringList() << filter, QDir::Files);
}

// 函数说明：实现 UtilAPI::now 的核心逻辑，供当前模块调用。
QString UtilAPI::now(const QString &format)
{
    return QDateTime::currentDateTime().toString(format);
}

// 函数说明：实现 UtilAPI::formatDate 的核心逻辑，供当前模块调用。
QString UtilAPI::formatDate(const QString &date, const QString &fromFormat, const QString &toFormat)
{
    QDateTime dt = QDateTime::fromString(date, fromFormat);
    return dt.toString(toFormat);
}

// 函数说明：实现 UtilAPI::clipboard 的核心逻辑，供当前模块调用。
QString UtilAPI::clipboard()
{
    return QApplication::clipboard()->text();
}

// 函数说明：设置 UtilAPI 的运行参数，并触发必要的界面或数据刷新。
void UtilAPI::setClipboard(const QString &text)
{
    QApplication::clipboard()->setText(text);
}

// 函数说明：实现 UtilAPI::httpGet 的核心逻辑，供当前模块调用。
QString UtilAPI::httpGet(const QString &url)
{
    QNetworkAccessManager manager;
    QNetworkRequest request{QUrl{url}};
    QNetworkReply *reply = manager.get(request);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QString result;
    if (reply->error() == QNetworkReply::NoError) {
        result = QString::fromUtf8(reply->readAll());
    }

    reply->deleteLater();
    return result;
}

// 函数说明：实现 UtilAPI::sleep 的核心逻辑，供当前模块调用。
void UtilAPI::sleep(int ms)
{
    QThread::msleep(ms);
}

// ===================== ScriptEngine =====================

ScriptEngine::ScriptEngine(QObject *parent)
    : QObject(parent)
    , m_engine(new QJSEngine(this))
    , m_editorAPI(nullptr)
    , m_appAPI(new AppAPI(this))
    , m_utilAPI(new UtilAPI(this))
    , m_mainWindow(nullptr)
    , m_editor(nullptr)
{
    m_scriptPaths << scriptsDir();

    setupEngine();
    loadSettings();
    registerBuiltinScripts();

    // 连接 AppAPI 信号
    connect(m_appAPI, &AppAPI::requestNewFile, this, &ScriptEngine::requestNewFile);
    connect(m_appAPI, &AppAPI::requestOpenFile, this, &ScriptEngine::requestOpenFile);
    connect(m_appAPI, &AppAPI::requestSaveFile, this, &ScriptEngine::requestSaveFile);
    connect(m_appAPI, &AppAPI::requestSaveFileAs, this, &ScriptEngine::requestSaveFileAs);
    connect(m_appAPI, &AppAPI::requestShowMessage, this, &ScriptEngine::requestShowMessage);
}

// 函数说明：销毁 ScriptEngine 对象，释放本模块持有的资源。
ScriptEngine::~ScriptEngine()
{
    saveSettings();
}

// 函数说明：设置 ScriptEngine 的运行参数，并触发必要的界面或数据刷新。
void ScriptEngine::setMainWindow(QMainWindow *window)
{
    m_mainWindow = window;
    m_appAPI->setMainWindow(window);
}

// 函数说明：设置 ScriptEngine 的运行参数，并触发必要的界面或数据刷新。
void ScriptEngine::setEditor(QPlainTextEdit *editor)
{
    m_editor = editor;

    // 重新创建 EditorAPI
    if (m_editorAPI) {
        delete m_editorAPI;
    }
    m_editorAPI = new EditorAPI(editor, this);

    // 更新引擎中的对象
    m_engine->globalObject().setProperty("editor",
        m_engine->newQObject(m_editorAPI));
}

// 函数说明：设置 ScriptEngine 的运行参数，并触发必要的界面或数据刷新。
void ScriptEngine::setCurrentFilePath(const QString &path)
{
    m_appAPI->setFilePath(path);
}

// 函数说明：向 ScriptEngine 管理的数据集合中添加一项内容。
void ScriptEngine::addScriptPath(const QString &path)
{
    if (!m_scriptPaths.contains(path)) {
        m_scriptPaths.append(path);
    }
}

// 函数说明：实现 ScriptEngine::scriptPaths 的核心逻辑，供当前模块调用。
QStringList ScriptEngine::scriptPaths() const
{
    return m_scriptPaths;
}

// 函数说明：实现 ScriptEngine::discoverScripts 的核心逻辑，供当前模块调用。
void ScriptEngine::discoverScripts()
{
    for (const QString &path : m_scriptPaths) {
        QDir dir(path);
        if (!dir.exists()) continue;

        dir.setNameFilters({"*.js"});
        for (const QString &fileName : dir.entryList(QDir::Files)) {
            QString filePath = dir.absoluteFilePath(fileName);
            loadScriptMetadata(filePath);
        }
    }
}

// 函数说明：实现 ScriptEngine::availableScripts 的核心逻辑，供当前模块调用。
QStringList ScriptEngine::availableScripts() const
{
    return m_scripts.keys();
}

// 函数说明：实现 ScriptEngine::scriptInfo 的核心逻辑，供当前模块调用。
ScriptInfo ScriptEngine::scriptInfo(const QString &scriptId) const
{
    return m_scripts.value(scriptId);
}

// 函数说明：判断 ScriptEngine 当前是否满足指定状态。
bool ScriptEngine::isScriptEnabled(const QString &scriptId) const
{
    return m_scripts.contains(scriptId) && m_scripts[scriptId].enabled;
}

// 函数说明：设置 ScriptEngine 的运行参数，并触发必要的界面或数据刷新。
void ScriptEngine::setScriptEnabled(const QString &scriptId, bool enabled)
{
    if (m_scripts.contains(scriptId)) {
        m_scripts[scriptId].enabled = enabled;
        saveSettings();
    }
}

// 函数说明：实现 ScriptEngine::runScript 的核心逻辑，供当前模块调用。
QJSValue ScriptEngine::runScript(const QString &code)
{
    QJSValue result = m_engine->evaluate(code);

    if (result.isError()) {
        QString error = QString("Line %1: %2")
            .arg(result.property("lineNumber").toInt())
            .arg(result.toString());
        emit scriptError("", error);
        qWarning() << "[Script Error]" << error;
    }

    return result;
}

// 函数说明：实现 ScriptEngine::runScriptFile 的核心逻辑，供当前模块调用。
QJSValue ScriptEngine::runScriptFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit scriptError(path, tr("无法读取脚本文件"));
        return QJSValue();
    }

    QString code = QTextStream(&file).readAll();
    file.close();

    return runScript(code);
}

// 函数说明：实现 ScriptEngine::runScriptById 的核心逻辑，供当前模块调用。
QJSValue ScriptEngine::runScriptById(const QString &scriptId)
{
    // 检查内置脚本
    if (m_builtinScripts.contains(scriptId)) {
        emit scriptStarted(scriptId);
        QJSValue result = runScript(m_builtinScripts[scriptId]);
        emit scriptFinished(scriptId);
        return result;
    }

    // 检查用户脚本
    if (!m_scripts.contains(scriptId)) {
        emit scriptError(scriptId, tr("脚本不存在"));
        return QJSValue();
    }

    const ScriptInfo &info = m_scripts[scriptId];
    if (!info.enabled) {
        emit scriptError(scriptId, tr("脚本已禁用"));
        return QJSValue();
    }

    emit scriptStarted(scriptId);
    QJSValue result = runScriptFile(info.filePath);
    emit scriptFinished(scriptId);

    return result;
}

// 函数说明：实现 ScriptEngine::registerBuiltinScripts 的核心逻辑，供当前模块调用。
void ScriptEngine::registerBuiltinScripts()
{
    // 转换为大写
    m_builtinScripts["toUpperCase"] = R"(
        var text = editor.selectedText || editor.text;
        if (editor.selectedText) {
            editor.replaceSelection(text.toUpperCase());
        } else {
            editor.text = text.toUpperCase();
        }
    )";

    // 转换为小写
    m_builtinScripts["toLowerCase"] = R"(
        var text = editor.selectedText || editor.text;
        if (editor.selectedText) {
            editor.replaceSelection(text.toLowerCase());
        } else {
            editor.text = text.toLowerCase();
        }
    )";

    // 行排序
    m_builtinScripts["sortLines"] = R"(
        var text = editor.selectedText || editor.text;
        var lines = text.split('\n');
        lines.sort();
        var result = lines.join('\n');
        if (editor.selectedText) {
            editor.replaceSelection(result);
        } else {
            editor.text = result;
        }
    )";

    // 去除重复行
    m_builtinScripts["removeDuplicateLines"] = R"(
        var text = editor.selectedText || editor.text;
        var lines = text.split('\n');
        var unique = [];
        var seen = {};
        for (var i = 0; i < lines.length; i++) {
            if (!seen[lines[i]]) {
                unique.push(lines[i]);
                seen[lines[i]] = true;
            }
        }
        var result = unique.join('\n');
        if (editor.selectedText) {
            editor.replaceSelection(result);
        } else {
            editor.text = result;
        }
    )";

    // 反转行顺序
    m_builtinScripts["reverseLines"] = R"(
        var text = editor.selectedText || editor.text;
        var lines = text.split('\n');
        lines.reverse();
        var result = lines.join('\n');
        if (editor.selectedText) {
            editor.replaceSelection(result);
        } else {
            editor.text = result;
        }
    )";

    // 添加行号
    m_builtinScripts["addLineNumbers"] = R"(
        var text = editor.selectedText || editor.text;
        var lines = text.split('\n');
        for (var i = 0; i < lines.length; i++) {
            lines[i] = (i + 1) + '. ' + lines[i];
        }
        var result = lines.join('\n');
        if (editor.selectedText) {
            editor.replaceSelection(result);
        } else {
            editor.text = result;
        }
    )";

    // 插入日期时间
    m_builtinScripts["insertDateTime"] = R"(
        editor.insertText(util.now('yyyy-MM-dd HH:mm:ss'));
    )";

    // 插入日期
    m_builtinScripts["insertDate"] = R"(
        editor.insertText(util.now('yyyy-MM-dd'));
    )";

    // 统计字数
    m_builtinScripts["wordCount"] = R"(
        var text = editor.selectedText || editor.text;
        var chars = text.length;
        var words = text.split(/\s+/).filter(function(w) { return w.length > 0; }).length;
        var lines = text.split('\n').length;
        app.showInfo('字数统计',
            '字符: ' + chars + '\n' +
            '单词: ' + words + '\n' +
            '行数: ' + lines);
    )";

    // Markdown 加粗
    m_builtinScripts["markdownBold"] = R"(
        var text = editor.selectedText;
        if (text) {
            editor.replaceSelection('**' + text + '**');
        } else {
            editor.insertText('****');
            editor.moveCursor(editor.cursorPosition - 2);
        }
    )";

    // Markdown 斜体
    m_builtinScripts["markdownItalic"] = R"(
        var text = editor.selectedText;
        if (text) {
            editor.replaceSelection('*' + text + '*');
        } else {
            editor.insertText('**');
            editor.moveCursor(editor.cursorPosition - 1);
        }
    )";

    // Markdown 代码块
    m_builtinScripts["markdownCodeBlock"] = R"(
        var text = editor.selectedText;
        if (text) {
            editor.replaceSelection('```\n' + text + '\n```');
        } else {
            editor.insertText('```\n\n```');
            editor.moveCursor(editor.cursorPosition - 4);
        }
    )";

    // Markdown 链接
    m_builtinScripts["markdownLink"] = R"(
        var text = editor.selectedText;
        var url = util.clipboard();
        if (text) {
            editor.replaceSelection('[' + text + '](' + (url || '') + ')');
        } else {
            editor.insertText('[](url)');
        }
    )";
}

// 函数说明：实现 ScriptEngine::triggerEvent 的核心逻辑，供当前模块调用。
void ScriptEngine::triggerEvent(const QString &event)
{
    for (auto it = m_scripts.begin(); it != m_scripts.end(); ++it) {
        if (it->enabled && it->triggers.contains(event)) {
            runScriptById(it.key());
        }
    }
}

// 函数说明：初始化 ScriptEngine 的 setupEngine 相关界面、动作或服务连接。
void ScriptEngine::setupEngine()
{
    // 注册全局对象
    m_engine->globalObject().setProperty("app",
        m_engine->newQObject(m_appAPI));
    m_engine->globalObject().setProperty("util",
        m_engine->newQObject(m_utilAPI));

    // console.log
    QString consoleCode = R"(
        var console = {
            log: function() {
                var args = Array.prototype.slice.call(arguments);
                app.log(args.join(' '));
            },
            warn: function() {
                var args = Array.prototype.slice.call(arguments);
                app.log('[WARN] ' + args.join(' '));
            },
            error: function() {
                var args = Array.prototype.slice.call(arguments);
                app.log('[ERROR] ' + args.join(' '));
            }
        };
    )";
    m_engine->evaluate(consoleCode);
}

// 函数说明：加载 ScriptEngine 需要的数据、配置或外部资源。
void ScriptEngine::loadScriptMetadata(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QString content = QTextStream(&file).readAll();
    file.close();

    ScriptInfo info = parseScriptHeader(content, filePath);
    if (!info.id.isEmpty()) {
        m_scripts[info.id] = info;
    }
}

// 函数说明：解析输入内容，转换为 ScriptEngine 后续处理使用的数据结构。
ScriptInfo ScriptEngine::parseScriptHeader(const QString &content, const QString &filePath)
{
    ScriptInfo info;
    info.filePath = filePath;
    info.id = QFileInfo(filePath).baseName();
    info.name = info.id;

    // 解析脚本头部注释
    // 格式: // @name 脚本名称
    //       // @description 描述
    //       // @author 作者
    //       // @version 1.0
    //       // @shortcut Ctrl+Shift+X
    //       // @trigger onSave, onOpen

    QRegularExpression headerRegex(R"(\/\/\s*@(\w+)\s+(.+))");
    QRegularExpressionMatchIterator it = headerRegex.globalMatch(content);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString key = match.captured(1).toLower();
        QString value = match.captured(2).trimmed();

        if (key == "name") {
            info.name = value;
        } else if (key == "description") {
            info.description = value;
        } else if (key == "author") {
            info.author = value;
        } else if (key == "version") {
            info.version = value;
        } else if (key == "shortcut") {
            info.shortcut = value;
        } else if (key == "trigger") {
            info.triggers = value.split(',', Qt::SkipEmptyParts);
            for (QString &t : info.triggers) {
                t = t.trimmed();
            }
        }
    }

    return info;
}

// 函数说明：实现 ScriptEngine::scriptsDir 的核心逻辑，供当前模块调用。
QString ScriptEngine::scriptsDir() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + "/scripts";
    QDir().mkpath(dir);
    return dir;
}

// 函数说明：加载 ScriptEngine 需要的数据、配置或外部资源。
void ScriptEngine::loadSettings()
{
    QSettings settings;
    settings.beginGroup("Scripts");

    int size = settings.beginReadArray("disabled");
    QSet<QString> disabledScripts;
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        disabledScripts.insert(settings.value("id").toString());
    }
    settings.endArray();

    for (auto it = m_scripts.begin(); it != m_scripts.end(); ++it) {
        it->enabled = !disabledScripts.contains(it.key());
    }

    settings.endGroup();
}

// 函数说明：保存 ScriptEngine 当前状态，保证用户修改可以持久化。
void ScriptEngine::saveSettings()
{
    QSettings settings;
    settings.beginGroup("Scripts");

    settings.beginWriteArray("disabled");
    int index = 0;
    for (auto it = m_scripts.begin(); it != m_scripts.end(); ++it) {
        if (!it->enabled) {
            settings.setArrayIndex(index++);
            settings.setValue("id", it.key());
        }
    }
    settings.endArray();

    settings.endGroup();
}

