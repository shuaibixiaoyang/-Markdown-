// 文件说明：app-static\extension\pluginmanager.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "pluginmanager.h"

#include <QDir>
#include <QFileInfo>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QMenuBar>
#include <QDockWidget>
#include <QStatusBar>
#include <QSettings>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QFile>

// 函数说明：构造 PluginManager 对象，初始化本模块需要的状态、界面和资源。
PluginManager::PluginManager(QObject *parent)
    : QObject(parent)
    , m_mainWindow(nullptr)
    , m_editor(nullptr)
{
    // 添加默认插件路径
    m_pluginPaths << pluginsDir();

    loadSettings();
}

// 函数说明：销毁 PluginManager 对象，释放本模块持有的资源。
PluginManager::~PluginManager()
{
    unloadAllPlugins();
    saveSettings();
}

// 函数说明：设置 PluginManager 的运行参数，并触发必要的界面或数据刷新。
void PluginManager::setMainWindow(QMainWindow *window)
{
    m_mainWindow = window;
}

// 函数说明：设置 PluginManager 的运行参数，并触发必要的界面或数据刷新。
void PluginManager::setEditor(QPlainTextEdit *editor)
{
    m_editor = editor;
}

// 函数说明：向 PluginManager 管理的数据集合中添加一项内容。
void PluginManager::addPluginPath(const QString &path)
{
    if (!m_pluginPaths.contains(path)) {
        m_pluginPaths.append(path);
    }
}

// 函数说明：实现 PluginManager::pluginPaths 的核心逻辑，供当前模块调用。
QStringList PluginManager::pluginPaths() const
{
    return m_pluginPaths;
}

// 函数说明：实现 PluginManager::discoverPlugins 的核心逻辑，供当前模块调用。
void PluginManager::discoverPlugins()
{
    for (const QString &path : m_pluginPaths) {
        QDir dir(path);
        if (!dir.exists()) continue;

        // 根据平台过滤插件文件
#ifdef Q_OS_WIN
        QStringList filters = {"*.dll"};
#elif defined(Q_OS_MAC)
        QStringList filters = {"*.dylib", "*.bundle"};
#else
        QStringList filters = {"*.so"};
#endif
        dir.setNameFilters(filters);

        for (const QString &fileName : dir.entryList(QDir::Files)) {
            QString filePath = dir.absoluteFilePath(fileName);

            // 检查是否已发现
            bool found = false;
            for (const auto &info : m_plugins) {
                if (info.filePath == filePath) {
                    found = true;
                    break;
                }
            }
            if (found) continue;

            // 尝试加载元数据
            QPluginLoader loader(filePath);
            QJsonObject metaData = loader.metaData().value("MetaData").toObject();

            if (metaData.isEmpty()) {
                // 尝试实际加载以获取元数据
                if (loader.load()) {
                    QObject *obj = loader.instance();
                    PluginInterface *plugin = qobject_cast<PluginInterface*>(obj);
                    if (plugin) {
                        PluginInfo info;
                        info.filePath = filePath;
                        info.metadata = plugin->metadata();
                        info.loader = nullptr;
                        info.instance = nullptr;
                        info.enabled = true;
                        info.loaded = false;

                        m_plugins[info.metadata.id] = info;
                        emit pluginDiscovered(info.metadata.id);
                    }
                    loader.unload();
                }
            } else {
                // 从 JSON 元数据读取
                PluginInfo info;
                info.filePath = filePath;
                info.metadata.id = metaData.value("id").toString();
                info.metadata.name = metaData.value("name").toString();
                info.metadata.version = metaData.value("version").toString();
                info.metadata.author = metaData.value("author").toString();
                info.metadata.description = metaData.value("description").toString();
                info.metadata.website = metaData.value("website").toString();
                info.metadata.license = metaData.value("license").toString();
                info.metadata.apiVersion = metaData.value("apiVersion").toInt(1);

                QJsonArray deps = metaData.value("dependencies").toArray();
                for (const auto &dep : deps) {
                    info.metadata.dependencies.append(dep.toString());
                }

                info.loader = nullptr;
                info.instance = nullptr;
                info.enabled = true;
                info.loaded = false;

                if (!info.metadata.id.isEmpty()) {
                    m_plugins[info.metadata.id] = info;
                    emit pluginDiscovered(info.metadata.id);
                }
            }
        }
    }

    log(tr("发现 %1 个插件").arg(m_plugins.size()));
}

// 函数说明：加载 PluginManager 需要的数据、配置或外部资源。
bool PluginManager::loadPlugin(const QString &pluginId)
{
    if (!m_plugins.contains(pluginId)) {
        logError(tr("插件 %1 不存在").arg(pluginId));
        return false;
    }

    PluginInfo &info = m_plugins[pluginId];

    if (info.loaded) {
        return true; // 已加载
    }

    // 检查依赖
    if (!checkDependencies(info)) {
        info.error = tr("依赖检查失败");
        emit pluginError(pluginId, info.error);
        return false;
    }

    // 检查 API 版本
    if (info.metadata.apiVersion > apiVersion()) {
        info.error = tr("插件需要更高版本的 API");
        emit pluginError(pluginId, info.error);
        return false;
    }

    // 加载插件
    info.loader = new QPluginLoader(info.filePath, this);

    if (!info.loader->load()) {
        info.error = info.loader->errorString();
        delete info.loader;
        info.loader = nullptr;
        logError(tr("加载插件 %1 失败: %2").arg(pluginId, info.error));
        emit pluginError(pluginId, info.error);
        return false;
    }

    QObject *obj = info.loader->instance();
    info.instance = qobject_cast<PluginInterface*>(obj);

    if (!info.instance) {
        info.error = tr("无效的插件接口");
        info.loader->unload();
        delete info.loader;
        info.loader = nullptr;
        logError(tr("插件 %1 接口无效").arg(pluginId));
        emit pluginError(pluginId, info.error);
        return false;
    }

    // 初始化插件
    if (!info.instance->initialize(this)) {
        info.error = tr("插件初始化失败");
        info.loader->unload();
        delete info.loader;
        info.loader = nullptr;
        info.instance = nullptr;
        logError(tr("插件 %1 初始化失败").arg(pluginId));
        emit pluginError(pluginId, info.error);
        return false;
    }

    info.loaded = true;
    info.instance->setEnabled(info.enabled);

    log(tr("插件 %1 已加载").arg(info.metadata.name));
    emit pluginLoaded(pluginId);

    return true;
}

// 函数说明：实现 PluginManager::unloadPlugin 的核心逻辑，供当前模块调用。
bool PluginManager::unloadPlugin(const QString &pluginId)
{
    if (!m_plugins.contains(pluginId)) {
        return false;
    }

    PluginInfo &info = m_plugins[pluginId];

    if (!info.loaded) {
        return true; // 未加载
    }

    // 检查是否有其他插件依赖此插件
    for (const auto &other : m_plugins) {
        if (other.loaded && other.metadata.dependencies.contains(pluginId)) {
            logWarning(tr("无法卸载 %1: 被 %2 依赖")
                .arg(pluginId, other.metadata.id));
            return false;
        }
    }

    // 关闭插件
    if (info.instance) {
        info.instance->shutdown();
    }

    // 卸载
    if (info.loader) {
        info.loader->unload();
        delete info.loader;
        info.loader = nullptr;
    }

    info.instance = nullptr;
    info.loaded = false;

    log(tr("插件 %1 已卸载").arg(info.metadata.name));
    emit pluginUnloaded(pluginId);

    return true;
}

// 函数说明：加载 PluginManager 需要的数据、配置或外部资源。
void PluginManager::loadAllPlugins()
{
    sortByDependencies();

    for (const QString &pluginId : m_plugins.keys()) {
        if (m_plugins[pluginId].enabled) {
            loadPlugin(pluginId);
        }
    }
}

// 函数说明：实现 PluginManager::unloadAllPlugins 的核心逻辑，供当前模块调用。
void PluginManager::unloadAllPlugins()
{
    // 逆序卸载
    QStringList ids = m_plugins.keys();
    for (int i = ids.size() - 1; i >= 0; --i) {
        unloadPlugin(ids[i]);
    }
}

// 函数说明：实现 PluginManager::availablePlugins 的核心逻辑，供当前模块调用。
QStringList PluginManager::availablePlugins() const
{
    return m_plugins.keys();
}

// 函数说明：加载 PluginManager 需要的数据、配置或外部资源。
QStringList PluginManager::loadedPlugins() const
{
    QStringList result;
    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it) {
        if (it->loaded) {
            result.append(it.key());
        }
    }
    return result;
}

// 函数说明：实现 PluginManager::pluginInfo 的核心逻辑，供当前模块调用。
PluginInfo PluginManager::pluginInfo(const QString &pluginId) const
{
    return m_plugins.value(pluginId);
}

// 函数说明：判断 PluginManager 当前是否满足指定状态。
bool PluginManager::isPluginLoaded(const QString &pluginId) const
{
    return m_plugins.contains(pluginId) && m_plugins[pluginId].loaded;
}

// 函数说明：判断 PluginManager 当前是否满足指定状态。
bool PluginManager::isPluginEnabled(const QString &pluginId) const
{
    return m_plugins.contains(pluginId) && m_plugins[pluginId].enabled;
}

// 函数说明：设置 PluginManager 的运行参数，并触发必要的界面或数据刷新。
void PluginManager::setPluginEnabled(const QString &pluginId, bool enabled)
{
    if (!m_plugins.contains(pluginId)) return;

    PluginInfo &info = m_plugins[pluginId];
    info.enabled = enabled;

    if (info.loaded && info.instance) {
        info.instance->setEnabled(enabled);
    }

    if (enabled) {
        emit pluginEnabled(pluginId);
    } else {
        emit pluginDisabled(pluginId);
    }

    saveSettings();
}

// 函数说明：实现 PluginManager::installPlugin 的核心逻辑，供当前模块调用。
bool PluginManager::installPlugin(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        logError(tr("文件不存在: %1").arg(filePath));
        return false;
    }

    // 复制到插件目录
    QString destPath = QDir(pluginsDir()).absoluteFilePath(fileInfo.fileName());

    if (QFile::exists(destPath)) {
        QFile::remove(destPath);
    }

    if (!QFile::copy(filePath, destPath)) {
        logError(tr("复制插件失败"));
        return false;
    }

    // 重新发现并加载
    discoverPlugins();

    return true;
}

// 函数说明：实现 PluginManager::uninstallPlugin 的核心逻辑，供当前模块调用。
bool PluginManager::uninstallPlugin(const QString &pluginId)
{
    if (!m_plugins.contains(pluginId)) {
        return false;
    }

    PluginInfo &info = m_plugins[pluginId];

    // 先卸载
    if (info.loaded) {
        if (!unloadPlugin(pluginId)) {
            return false;
        }
    }

    // 删除文件
    if (!QFile::remove(info.filePath)) {
        logError(tr("删除插件文件失败"));
        return false;
    }

    m_plugins.remove(pluginId);
    log(tr("插件 %1 已卸载").arg(pluginId));

    return true;
}

// PluginContext 实现
QPlainTextEdit* PluginManager::editor()
{
    return m_editor;
}

// 函数说明：实现 PluginManager::currentDocument 的核心逻辑，供当前模块调用。
QString PluginManager::currentDocument()
{
    if (m_editor) {
        return m_editor->toPlainText();
    }
    return QString();
}

// 函数说明：设置 PluginManager 的运行参数，并触发必要的界面或数据刷新。
void PluginManager::setDocument(const QString &text)
{
    if (m_editor) {
        m_editor->setPlainText(text);
    }
}

// 函数说明：实现 PluginManager::selectedText 的核心逻辑，供当前模块调用。
QString PluginManager::selectedText()
{
    if (m_editor) {
        return m_editor->textCursor().selectedText();
    }
    return QString();
}

// 函数说明：实现 PluginManager::insertText 的核心逻辑，供当前模块调用。
void PluginManager::insertText(const QString &text)
{
    if (m_editor) {
        m_editor->textCursor().insertText(text);
    }
}

// 函数说明：实现 PluginManager::replaceSelection 的核心逻辑，供当前模块调用。
void PluginManager::replaceSelection(const QString &text)
{
    if (m_editor) {
        QTextCursor cursor = m_editor->textCursor();
        cursor.insertText(text);
    }
}

// 函数说明：实现 PluginManager::currentFilePath 的核心逻辑，供当前模块调用。
QString PluginManager::currentFilePath()
{
    return m_currentFilePath;
}

// 函数说明：打开 PluginManager 对应的文件、资源或功能入口。
bool PluginManager::openFile(const QString &path)
{
    emit requestOpenFile(path);
    return true;
}

// 函数说明：保存 PluginManager 当前状态，保证用户修改可以持久化。
bool PluginManager::saveFile()
{
    emit requestSaveFile();
    return true;
}

// 函数说明：保存 PluginManager 当前状态，保证用户修改可以持久化。
bool PluginManager::saveFileAs(const QString &path)
{
    emit requestSaveFileAs(path);
    return true;
}

// 函数说明：向 PluginManager 管理的数据集合中添加一项内容。
QMenu* PluginManager::addMenu(const QString &title)
{
    if (!m_mainWindow) return nullptr;

    QMenu *menu = m_mainWindow->menuBar()->addMenu(title);
    m_createdMenus.append(menu);
    return menu;
}

// 函数说明：从 PluginManager 管理的数据集合中移除指定内容。
void PluginManager::removeMenu(QMenu *menu)
{
    if (!m_mainWindow || !menu) return;

    m_mainWindow->menuBar()->removeAction(menu->menuAction());
    m_createdMenus.removeOne(menu);
    menu->deleteLater();
}

// 函数说明：向 PluginManager 管理的数据集合中添加一项内容。
QAction* PluginManager::addAction(QMenu *menu, const QString &text, const QKeySequence &shortcut)
{
    if (!menu) return nullptr;

    QAction *action = menu->addAction(text);
    if (!shortcut.isEmpty()) {
        action->setShortcut(shortcut);
    }
    return action;
}

// 函数说明：向 PluginManager 管理的数据集合中添加一项内容。
QDockWidget* PluginManager::addDockWidget(const QString &title, QWidget *widget, Qt::DockWidgetArea area)
{
    if (!m_mainWindow || !widget) return nullptr;

    QDockWidget *dock = new QDockWidget(title, m_mainWindow);
    dock->setWidget(widget);
    m_mainWindow->addDockWidget(area, dock);
    m_createdDocks.append(dock);

    return dock;
}

// 函数说明：从 PluginManager 管理的数据集合中移除指定内容。
void PluginManager::removeDockWidget(QDockWidget *dock)
{
    if (!m_mainWindow || !dock) return;

    m_mainWindow->removeDockWidget(dock);
    m_createdDocks.removeOne(dock);
    dock->deleteLater();
}

// 函数说明：显示 PluginManager 管理的面板、对话框或提示信息。
void PluginManager::showStatusMessage(const QString &message, int timeout)
{
    emit requestShowStatusMessage(message, timeout);
}

// 函数说明：读取 PluginManager 当前保存的状态或计算结果。
QVariant PluginManager::getSetting(const QString &key, const QVariant &defaultValue)
{
    return m_pluginSettings.value(key, defaultValue);
}

// 函数说明：设置 PluginManager 的运行参数，并触发必要的界面或数据刷新。
void PluginManager::setSetting(const QString &key, const QVariant &value)
{
    m_pluginSettings[key] = value;
    saveSettings();
}

// 函数说明：实现 PluginManager::log 的核心逻辑，供当前模块调用。
void PluginManager::log(const QString &message)
{
    emit logMessage("INFO", message);
    qDebug() << "[Plugin]" << message;
}

// 函数说明：实现 PluginManager::logWarning 的核心逻辑，供当前模块调用。
void PluginManager::logWarning(const QString &message)
{
    emit logMessage("WARNING", message);
    qWarning() << "[Plugin]" << message;
}

// 函数说明：实现 PluginManager::logError 的核心逻辑，供当前模块调用。
void PluginManager::logError(const QString &message)
{
    emit logMessage("ERROR", message);
    qCritical() << "[Plugin]" << message;
}

// 函数说明：加载 PluginManager 需要的数据、配置或外部资源。
void PluginManager::loadSettings()
{
    QSettings settings;
    settings.beginGroup("Plugins");

    // 加载启用状态
    int size = settings.beginReadArray("enabled");
    QSet<QString> enabledPlugins;
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        enabledPlugins.insert(settings.value("id").toString());
    }
    settings.endArray();

    // 加载插件设置
    settings.beginGroup("Settings");
    for (const QString &key : settings.childKeys()) {
        m_pluginSettings[key] = settings.value(key);
    }
    settings.endGroup();

    settings.endGroup();
}

// 函数说明：保存 PluginManager 当前状态，保证用户修改可以持久化。
void PluginManager::saveSettings()
{
    QSettings settings;
    settings.beginGroup("Plugins");

    // 保存启用状态
    settings.beginWriteArray("enabled");
    int index = 0;
    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it) {
        if (it->enabled) {
            settings.setArrayIndex(index++);
            settings.setValue("id", it.key());
        }
    }
    settings.endArray();

    // 保存插件设置
    settings.beginGroup("Settings");
    for (auto it = m_pluginSettings.begin(); it != m_pluginSettings.end(); ++it) {
        settings.setValue(it.key(), it.value());
    }
    settings.endGroup();

    settings.endGroup();
}

// 函数说明：实现 PluginManager::checkDependencies 的核心逻辑，供当前模块调用。
bool PluginManager::checkDependencies(const PluginInfo &info)
{
    for (const QString &dep : info.metadata.dependencies) {
        if (!m_plugins.contains(dep)) {
            logError(tr("缺少依赖: %1").arg(dep));
            return false;
        }

        // 确保依赖已加载
        if (!m_plugins[dep].loaded) {
            if (!loadPlugin(dep)) {
                return false;
            }
        }
    }
    return true;
}

// 函数说明：实现 PluginManager::sortByDependencies 的核心逻辑，供当前模块调用。
void PluginManager::sortByDependencies()
{
    // 拓扑排序（简化版）
    QStringList sorted;
    QSet<QString> visited;

    std::function<void(const QString&)> visit = [&](const QString &id) {
        if (visited.contains(id)) return;
        visited.insert(id);

        if (m_plugins.contains(id)) {
            for (const QString &dep : m_plugins[id].metadata.dependencies) {
                visit(dep);
            }
            sorted.append(id);
        }
    };

    for (const QString &id : m_plugins.keys()) {
        visit(id);
    }

    // 重新排列 m_plugins（Qt的QMap已排序，这里只是确保依赖顺序正确）
}

// 函数说明：实现 PluginManager::pluginsDir 的核心逻辑，供当前模块调用。
QString PluginManager::pluginsDir() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + "/plugins";
    QDir().mkpath(dir);
    return dir;
}

