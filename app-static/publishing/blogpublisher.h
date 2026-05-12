// 文件说明：app-static\publishing\blogpublisher.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef BLOGPUBLISHER_H
#define BLOGPUBLISHER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>

/**
 * @brief 博客发布器 - 支持国内主流平台
 *
 * 支持的平台:
 * - CSDN
 * - 知乎
 * - 牛客
 *
 * 工作方式: 复制 Markdown 内容到剪贴板，打开平台编辑器页面，
 * 用户在浏览器中粘贴内容并填写标题等信息后发布。
 */
class BlogPublisher : public QObject
{
    Q_OBJECT

public:
    enum class Platform {
        CSDN,
        Zhihu,
        Nowcoder
    };
    Q_ENUM(Platform)

    struct BlogConfig {
        Platform platform;
        QString name;

        BlogConfig() : platform(Platform::CSDN) {}
    };

    struct PublishResult {
        bool success;
        QUrl editorUrl;
        QString errorMessage;
    };

    explicit BlogPublisher(QObject *parent = nullptr);
    ~BlogPublisher();

    void setConfig(const BlogConfig &config);
    BlogConfig config() const { return m_config; }

    static bool saveConfigs(const QList<BlogConfig> &configs, const QString &path);
    static QList<BlogConfig> loadConfigs(const QString &path);

    // 发布：复制内容到剪贴板并打开平台编辑器
    void publishPost(const QString &markdownContent);

    static QString editorUrl(Platform platform);
    static QString platformName(Platform platform);

signals:
    void postPublished(const PublishResult &result);

private:
    BlogConfig m_config;
};

#endif // BLOGPUBLISHER_H

