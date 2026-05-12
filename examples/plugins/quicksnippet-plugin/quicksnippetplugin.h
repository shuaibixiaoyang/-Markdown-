#ifndef QUICKSNIPPETPLUGIN_H
#define QUICKSNIPPETPLUGIN_H

#include <QObject>
#include <QMap>
#include <extension/plugininterface.h>

class QMenu;

/**
 * @brief 快捷片段插件
 *
 * 通过菜单快速插入常用的 Markdown 代码片段。
 * 展示如何：
 * - 创建菜单和快捷键
 * - 使用 PluginContext 的 insertText/replaceSelection
 * - 使用持久化设置
 */
class QuickSnippetPlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid)
    Q_INTERFACES(PluginInterface)

public:
    explicit QuickSnippetPlugin(QObject *parent = nullptr);
    ~QuickSnippetPlugin() override;

    PluginMetadata metadata() const override;
    bool initialize(PluginContext *context) override;
    void shutdown() override;
    void setEnabled(bool enabled) override;
    bool isEnabled() const override;

private slots:
    void insertSnippet(const QString &key);

private:
    void registerSnippets();

    PluginContext *m_context = nullptr;
    QMenu *m_menu = nullptr;
    QMap<QString, QString> m_snippets;
    bool m_enabled = true;
};

#endif // QUICKSNIPPETPLUGIN_H
