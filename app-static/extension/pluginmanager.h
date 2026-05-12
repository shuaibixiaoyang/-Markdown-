// 文件说明：app-static\extension\pluginmanager.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QObject>
#include <QMap>
#include <QStringList>
#include <QPluginLoader>

#include "plugininterface.h"

class QMainWindow;
class QPlainTextEdit;

/**
 * @brief 插件信息
 */
struct PluginInfo {
    QString filePath;           // 插件文件路径
    PluginMetadata metadata;    // 元数据
    PluginInterface *instance;  // 插件实例
    QPluginLoader *loader;      // 加载器
    bool enabled;               // 是否启用
    bool loaded;                // 是否已加载
    QString error;              // 错误信息

    PluginInfo()
        : instance(nullptr)
        , loader(nullptr)
        , enabled(true)
        , loaded(false) {}
};

/**
 * @brief 插件管理器
 *
 * 功能：
 * - 发现和加载插件
 * - 管理插件生命周期
 * - 提供插件上下文
 * - 处理插件依赖
 * - 启用/禁用插件
 */
class PluginManager : public QObject, public PluginContext
{
    Q_OBJECT

public:
    explicit PluginManager(QObject *parent = nullptr);
    ~PluginManager();

    // 设置主窗口和编辑器
    void setMainWindow(QMainWindow *window);
    void setEditor(QPlainTextEdit *editor);

    // 插件目录
    void addPluginPath(const QString &path);
    QStringList pluginPaths() const;

    // 发现和加载
    void discoverPlugins();
    bool loadPlugin(const QString &pluginId);
    bool unloadPlugin(const QString &pluginId);
    void loadAllPlugins();
    void unloadAllPlugins();

    // 查询
    QStringList availablePlugins() const;
    QStringList loadedPlugins() const;
    PluginInfo pluginInfo(const QString &pluginId) const;
    bool isPluginLoaded(const QString &pluginId) const;
    bool isPluginEnabled(const QString &pluginId) const;

    // 启用/禁用
    void setPluginEnabled(const QString &pluginId, bool enabled);

    // 安装/卸载
    bool installPlugin(const QString &filePath);
    bool uninstallPlugin(const QString &pluginId);

    // API 版本
    static int apiVersion() { return 1; }

    // PluginContext 实现
    QPlainTextEdit* editor() override;
    QString currentDocument() override;
    void setDocument(const QString &text) override;
    QString selectedText() override;
    void insertText(const QString &text) override;
    void replaceSelection(const QString &text) override;

    QString currentFilePath() override;
    bool openFile(const QString &path) override;
    bool saveFile() override;
    bool saveFileAs(const QString &path) override;

    QMenu* addMenu(const QString &title) override;
    void removeMenu(QMenu *menu) override;
    QAction* addAction(QMenu *menu, const QString &text, const QKeySequence &shortcut = QKeySequence()) override;
    QDockWidget* addDockWidget(const QString &title, QWidget *widget, Qt::DockWidgetArea area = Qt::RightDockWidgetArea) override;
    void removeDockWidget(QDockWidget *dock) override;
    void showStatusMessage(const QString &message, int timeout = 3000) override;

    QVariant getSetting(const QString &key, const QVariant &defaultValue = QVariant()) override;
    void setSetting(const QString &key, const QVariant &value) override;

    void log(const QString &message) override;
    void logWarning(const QString &message) override;
    void logError(const QString &message) override;

signals:
    void pluginDiscovered(const QString &pluginId);
    void pluginLoaded(const QString &pluginId);
    void pluginUnloaded(const QString &pluginId);
    void pluginEnabled(const QString &pluginId);
    void pluginDisabled(const QString &pluginId);
    void pluginError(const QString &pluginId, const QString &error);
    void logMessage(const QString &level, const QString &message);

    // 请求主窗口执行操作
    void requestOpenFile(const QString &path);
    void requestSaveFile();
    void requestSaveFileAs(const QString &path);
    void requestShowStatusMessage(const QString &message, int timeout);

private:
    void loadSettings();
    void saveSettings();
    bool checkDependencies(const PluginInfo &info);
    void sortByDependencies();
    QString pluginsDir() const;

    QMainWindow *m_mainWindow;
    QPlainTextEdit *m_editor;
    QString m_currentFilePath;

    QStringList m_pluginPaths;
    QMap<QString, PluginInfo> m_plugins;
    QList<QMenu*> m_createdMenus;
    QList<QDockWidget*> m_createdDocks;

    QMap<QString, QVariant> m_pluginSettings;
};

#endif // PLUGINMANAGER_H

