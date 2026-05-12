// 文件说明：app-static\extension\scriptengine.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include <QObject>
#include <QJSEngine>
#include <QJSValue>
#include <QMap>
#include <QStringList>

class QPlainTextEdit;
class QMainWindow;

/**
 * @brief 脚本信息
 */
struct ScriptInfo {
    QString id;              // 唯一标识符
    QString name;            // 显示名称
    QString description;     // 描述
    QString author;          // 作者
    QString version;         // 版本
    QString filePath;        // 文件路径
    QString shortcut;        // 快捷键
    QStringList triggers;    // 触发条件
    bool enabled;            // 是否启用

    ScriptInfo() : enabled(true) {}
};

/**
 * @brief 编辑器 API（暴露给脚本）
 */
class EditorAPI : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QString selectedText READ selectedText)
    Q_PROPERTY(int cursorPosition READ cursorPosition WRITE setCursorPosition)
    Q_PROPERTY(int lineNumber READ lineNumber)
    Q_PROPERTY(int columnNumber READ columnNumber)

public:
    explicit EditorAPI(QPlainTextEdit *editor, QObject *parent = nullptr);

    QString text() const;
    void setText(const QString &text);
    QString selectedText() const;
    int cursorPosition() const;
    void setCursorPosition(int pos);
    int lineNumber() const;
    int columnNumber() const;

public slots:
    // 文本操作
    void insertText(const QString &text);
    void replaceSelection(const QString &text);
    void deleteSelection();
    void selectAll();
    void selectLine(int line);
    void selectRange(int start, int end);

    // 光标操作
    void moveCursor(int position);
    void moveToLine(int line);
    void moveToLineStart();
    void moveToLineEnd();
    void moveToDocumentStart();
    void moveToDocumentEnd();

    // 行操作
    QString getLine(int line);
    void insertLine(int line, const QString &text);
    void deleteLine(int line);
    void replaceLine(int line, const QString &text);
    int lineCount();

    // 查找替换
    bool find(const QString &text, bool caseSensitive = false, bool wholeWord = false);
    int replaceAll(const QString &find, const QString &replace, bool caseSensitive = false);

    // 剪贴板
    void copy();
    void cut();
    void paste();

    // 撤销重做
    void undo();
    void redo();

private:
    QPlainTextEdit *m_editor;
};

/**
 * @brief 应用 API（暴露给脚本）
 */
class AppAPI : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString filePath READ filePath)
    Q_PROPERTY(QString fileName READ fileName)

public:
    explicit AppAPI(QObject *parent = nullptr);

    void setMainWindow(QMainWindow *window);
    void setFilePath(const QString &path);

    QString filePath() const;
    QString fileName() const;

public slots:
    // 文件操作
    void newFile();
    void openFile(const QString &path = QString());
    void saveFile();
    void saveFileAs(const QString &path = QString());

    // UI
    void showMessage(const QString &message, int timeout = 3000);
    void showInfo(const QString &title, const QString &message);
    void showWarning(const QString &title, const QString &message);
    void showError(const QString &title, const QString &message);
    QString inputText(const QString &title, const QString &label, const QString &defaultValue = QString());
    bool confirm(const QString &title, const QString &message);

    // 日志
    void log(const QString &message);

signals:
    void requestNewFile();
    void requestOpenFile(const QString &path);
    void requestSaveFile();
    void requestSaveFileAs(const QString &path);
    void requestShowMessage(const QString &message, int timeout);

private:
    QMainWindow *m_mainWindow;
    QString m_filePath;
};

/**
 * @brief 工具 API（暴露给脚本）
 */
class UtilAPI : public QObject
{
    Q_OBJECT

public:
    explicit UtilAPI(QObject *parent = nullptr);

public slots:
    // 字符串操作
    QString trim(const QString &text);
    QString toUpperCase(const QString &text);
    QString toLowerCase(const QString &text);
    QString repeat(const QString &text, int count);
    QStringList split(const QString &text, const QString &separator);
    QString join(const QStringList &list, const QString &separator);

    // 正则表达式
    bool matches(const QString &text, const QString &pattern);
    QString replace(const QString &text, const QString &pattern, const QString &replacement);
    QStringList findAll(const QString &text, const QString &pattern);

    // 文件操作
    QString readFile(const QString &path);
    bool writeFile(const QString &path, const QString &content);
    bool fileExists(const QString &path);
    QStringList listFiles(const QString &dir, const QString &filter = "*");

    // 日期时间
    QString now(const QString &format = "yyyy-MM-dd HH:mm:ss");
    QString formatDate(const QString &date, const QString &fromFormat, const QString &toFormat);

    // 剪贴板
    QString clipboard();
    void setClipboard(const QString &text);

    // 网络（简单）
    QString httpGet(const QString &url);

    // 延迟
    void sleep(int ms);
};

/**
 * @brief 脚本引擎
 *
 * 功能：
 * - 运行 JavaScript 脚本
 * - 提供编辑器/应用/工具 API
 * - 管理脚本库
 * - 支持脚本热键绑定
 */
class ScriptEngine : public QObject
{
    Q_OBJECT

public:
    explicit ScriptEngine(QObject *parent = nullptr);
    ~ScriptEngine();

    // 设置
    void setMainWindow(QMainWindow *window);
    void setEditor(QPlainTextEdit *editor);
    void setCurrentFilePath(const QString &path);

    // 脚本管理
    void addScriptPath(const QString &path);
    QStringList scriptPaths() const;
    void discoverScripts();

    // 脚本库
    QStringList availableScripts() const;
    ScriptInfo scriptInfo(const QString &scriptId) const;
    bool isScriptEnabled(const QString &scriptId) const;
    void setScriptEnabled(const QString &scriptId, bool enabled);

    // 运行脚本
    QJSValue runScript(const QString &code);
    QJSValue runScriptFile(const QString &path);
    QJSValue runScriptById(const QString &scriptId);

    // 内置脚本
    void registerBuiltinScripts();

    // 事件触发
    void triggerEvent(const QString &event);

signals:
    void scriptStarted(const QString &scriptId);
    void scriptFinished(const QString &scriptId);
    void scriptError(const QString &scriptId, const QString &error);
    void scriptOutput(const QString &message);

    // 请求操作
    void requestNewFile();
    void requestOpenFile(const QString &path);
    void requestSaveFile();
    void requestSaveFileAs(const QString &path);
    void requestShowMessage(const QString &message, int timeout);

private:
    void setupEngine();
    void loadScriptMetadata(const QString &filePath);
    ScriptInfo parseScriptHeader(const QString &content, const QString &filePath);
    QString scriptsDir() const;
    void loadSettings();
    void saveSettings();

    QJSEngine *m_engine;
    EditorAPI *m_editorAPI;
    AppAPI *m_appAPI;
    UtilAPI *m_utilAPI;

    QMainWindow *m_mainWindow;
    QPlainTextEdit *m_editor;

    QStringList m_scriptPaths;
    QMap<QString, ScriptInfo> m_scripts;
    QMap<QString, QString> m_builtinScripts;
};

#endif // SCRIPTENGINE_H

