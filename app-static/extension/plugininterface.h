// 文件说明：app-static\extension\plugininterface.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include <QtPlugin>
#include <QString>
#include <QWidget>
#include <QMenu>
#include <QAction>
#include <QPlainTextEdit>
#include <QDockWidget>

/**
 * @brief 插件元数据
 */
struct PluginMetadata {
    QString id;              // 唯一标识符
    QString name;            // 显示名称
    QString version;         // 版本号
    QString author;          // 作者
    QString description;     // 描述
    QString website;         // 网站
    QString license;         // 许可证
    QStringList dependencies; // 依赖的其他插件
    int apiVersion;          // API 版本要求

    PluginMetadata()
        : apiVersion(1) {}
};

/**
 * @brief 插件上下文 - 提供给插件的接口
 */
class PluginContext
{
public:
    virtual ~PluginContext() = default;

    // 编辑器访问
    virtual QPlainTextEdit* editor() = 0;
    virtual QString currentDocument() = 0;
    virtual void setDocument(const QString &text) = 0;
    virtual QString selectedText() = 0;
    virtual void insertText(const QString &text) = 0;
    virtual void replaceSelection(const QString &text) = 0;

    // 文件操作
    virtual QString currentFilePath() = 0;
    virtual bool openFile(const QString &path) = 0;
    virtual bool saveFile() = 0;
    virtual bool saveFileAs(const QString &path) = 0;

    // UI 扩展
    virtual QMenu* addMenu(const QString &title) = 0;
    virtual void removeMenu(QMenu *menu) = 0;
    virtual QAction* addAction(QMenu *menu, const QString &text, const QKeySequence &shortcut = QKeySequence()) = 0;
    virtual QDockWidget* addDockWidget(const QString &title, QWidget *widget, Qt::DockWidgetArea area = Qt::RightDockWidgetArea) = 0;
    virtual void removeDockWidget(QDockWidget *dock) = 0;
    virtual void showStatusMessage(const QString &message, int timeout = 3000) = 0;

    // 设置
    virtual QVariant getSetting(const QString &key, const QVariant &defaultValue = QVariant()) = 0;
    virtual void setSetting(const QString &key, const QVariant &value) = 0;

    // 日志
    virtual void log(const QString &message) = 0;
    virtual void logWarning(const QString &message) = 0;
    virtual void logError(const QString &message) = 0;
};

/**
 * @brief 插件接口 - 所有插件必须实现此接口
 */
class PluginInterface
{
public:
    virtual ~PluginInterface() = default;

    // 获取插件元数据
    virtual PluginMetadata metadata() const = 0;

    // 生命周期
    virtual bool initialize(PluginContext *context) = 0;
    virtual void shutdown() = 0;

    // 启用/禁用
    virtual void setEnabled(bool enabled) = 0;
    virtual bool isEnabled() const = 0;

    // 配置界面（可选）
    virtual QWidget* configWidget() { return nullptr; }
    virtual void applyConfig() {}
};

#define PluginInterface_iid "com.cutemarked.PluginInterface/1.0"
Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)

#endif // PLUGININTERFACE_H

