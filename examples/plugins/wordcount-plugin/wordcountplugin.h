#ifndef WORDCOUNTPLUGIN_H
#define WORDCOUNTPLUGIN_H

#include <QObject>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <extension/plugininterface.h>

/**
 * @brief 字数统计插件
 *
 * 在右侧 Dock 面板中实时显示字数统计信息。
 * 这是一个 CuteMarkEd 插件的示例，展示了如何：
 * - 实现 PluginInterface 接口
 * - 使用 PluginContext 访问编辑器
 * - 创建 Dock Widget UI
 * - 响应文本变化
 */
class WordCountPlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid)
    Q_INTERFACES(PluginInterface)

public:
    explicit WordCountPlugin(QObject *parent = nullptr);
    ~WordCountPlugin() override;

    // PluginInterface 实现
    PluginMetadata metadata() const override;
    bool initialize(PluginContext *context) override;
    void shutdown() override;
    void setEnabled(bool enabled) override;
    bool isEnabled() const override;

private slots:
    void updateStats();

private:
    PluginContext *m_context = nullptr;
    QDockWidget *m_dock = nullptr;
    QWidget *m_panel = nullptr;
    QLabel *m_charLabel = nullptr;
    QLabel *m_wordLabel = nullptr;
    QLabel *m_lineLabel = nullptr;
    QLabel *m_chineseLabel = nullptr;
    QLabel *m_readTimeLabel = nullptr;
    QTimer *m_timer = nullptr;
    bool m_enabled = true;
};

#endif // WORDCOUNTPLUGIN_H
