// 文件说明：app-static\publishing\blogpublisher.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "blogpublisher.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QDesktopServices>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// 函数说明：构造 BlogPublisher 对象，初始化本模块需要的状态、界面和资源。
BlogPublisher::BlogPublisher(QObject *parent)
    : QObject(parent)
{
}

// 函数说明：销毁 BlogPublisher 对象，释放本模块持有的资源。
BlogPublisher::~BlogPublisher()
{
}

// 函数说明：设置 BlogPublisher 的运行参数，并触发必要的界面或数据刷新。
void BlogPublisher::setConfig(const BlogConfig &config)
{
    m_config = config;
}

// 函数说明：实现 BlogPublisher::platformName 的核心逻辑，供当前模块调用。
QString BlogPublisher::platformName(Platform platform)
{
    switch (platform) {
        case Platform::CSDN:     return QStringLiteral("CSDN");
        case Platform::Zhihu:    return QStringLiteral("\u77e5\u4e4e");
        case Platform::Nowcoder: return QStringLiteral("\u725b\u5ba2");
    }
    return QString();
}

// 函数说明：处理编辑命令，将用户操作转换为 Markdown 文本变更。
QString BlogPublisher::editorUrl(Platform platform)
{
    switch (platform) {
        case Platform::CSDN:
            return QStringLiteral("https://mp.csdn.net/mp_blog/creation/editor");
        case Platform::Zhihu:
            return QStringLiteral("https://zhuanlan.zhihu.com/write");
        case Platform::Nowcoder:
            return QStringLiteral("https://www.nowcoder.com/creation/manager/content/edit");
    }
    return QString();
}

// 函数说明：保存 BlogPublisher 当前状态，保证用户修改可以持久化。
bool BlogPublisher::saveConfigs(const QList<BlogConfig> &configs, const QString &path)
{
    QJsonArray array;
    for (const BlogConfig &config : configs) {
        QJsonObject obj;
        obj["platform"] = static_cast<int>(config.platform);
        obj["name"] = config.name;
        array.append(obj);
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(array).toJson());
    return true;
}

// 函数说明：加载 BlogPublisher 需要的数据、配置或外部资源。
QList<BlogPublisher::BlogConfig> BlogPublisher::loadConfigs(const QString &path)
{
    QList<BlogConfig> configs;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return configs;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    for (const QJsonValue &value : doc.array()) {
        QJsonObject obj = value.toObject();
        BlogConfig config;
        config.platform = static_cast<Platform>(obj["platform"].toInt());
        config.name = obj["name"].toString();
        configs.append(config);
    }
    return configs;
}

// 函数说明：处理发布流程，将当前文档输出到电子书、博客或静态站点。
void BlogPublisher::publishPost(const QString &markdownContent)
{
    // 复制 Markdown 内容到剪贴板
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(markdownContent);

    // 打开平台编辑器页面
    QString url = editorUrl(m_config.platform);
    QDesktopServices::openUrl(QUrl(url));

    PublishResult result;
    result.success = true;
    result.editorUrl = QUrl(url);
    emit postPublished(result);
}

