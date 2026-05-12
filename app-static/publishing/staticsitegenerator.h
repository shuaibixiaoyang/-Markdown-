// 文件说明：app-static\publishing\staticsitegenerator.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef STATICSITEGENERATOR_H
#define STATICSITEGENERATOR_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QProcess>
#include <QMap>
#include <QVariant>

/**
 * @brief 静态网站生成器集成
 *
 * 支持的静态网站生成器：
 * - Hugo (Go)
 * - Jekyll (Ruby)
 * - Hexo (Node.js)
 * - VuePress (Node.js)
 * - MkDocs (Python)
 *
 * 功能：
 * - 创建新项目
 * - 创建/编辑文章
 * - 本地预览
 * - 构建发布
 * - 主题管理
 */
class StaticSiteGenerator : public QObject
{
    Q_OBJECT

public:
    // 支持的生成器类型
    enum class GeneratorType {
        Hugo,
        Jekyll,
        Hexo,
        VuePress,
        MkDocs
    };
    Q_ENUM(GeneratorType)

    // 文章 Front Matter 元数据
    struct FrontMatter {
        QString title;
        QString date;
        QString author;
        QStringList tags;
        QStringList categories;
        QString description;
        QString slug;
        bool draft;
        int weight;                     // 排序权重
        QString layout;                 // 布局模板
        QString featuredImage;          // 封面图
        QMap<QString, QVariant> custom; // 自定义字段

        FrontMatter()
            : draft(false)
            , weight(0)
        {}
    };

    // 站点配置
    struct SiteConfig {
        GeneratorType type;
        QString sitePath;               // 站点根目录
        QString title;
        QString baseUrl;
        QString language;
        QString theme;
        QString author;
        QString description;

        // Hugo 特定
        QString hugoVersion;

        // Jekyll 特定
        QString rubyVersion;

        // Hexo 特定
        QString hexoVersion;

        SiteConfig()
            : type(GeneratorType::Hugo)
            , language("zh-CN")
        {}
    };

    // 文章信息
    struct Post {
        QString filePath;
        QString relativePath;
        FrontMatter frontMatter;
        QString content;
        QDateTime createdAt;
        QDateTime modifiedAt;
    };

    // 主题信息
    struct Theme {
        QString name;
        QString path;
        QString version;
        QString author;
        QString description;
        QString previewUrl;
        bool isInstalled;
    };

    explicit StaticSiteGenerator(QObject *parent = nullptr);
    ~StaticSiteGenerator();

    // 检测已安装的生成器
    static QMap<GeneratorType, QString> detectInstalledGenerators();
    static bool isGeneratorInstalled(GeneratorType type);
    static QString getGeneratorCommand(GeneratorType type);
    static QString getGeneratorVersion(GeneratorType type);

    // 设置站点配置
    void setSiteConfig(const SiteConfig &config);
    SiteConfig siteConfig() const { return m_config; }

    // 项目管理
    bool createNewSite(const QString &path, const QString &name);
    bool openSite(const QString &path);
    bool isValidSite() const;

    // 文章管理
    bool createPost(const QString &title, const FrontMatter &frontMatter = FrontMatter());
    bool createPage(const QString &title, const FrontMatter &frontMatter = FrontMatter());
    bool savePost(const Post &post);
    bool deletePost(const QString &filePath);
    QVector<Post> listPosts(bool includeDrafts = true);
    QVector<Post> listPages();
    Post loadPost(const QString &filePath);

    // 从当前 Markdown 导出
    bool exportToSite(const QString &markdown, const FrontMatter &frontMatter);

    // 本地预览
    bool startServer(int port = 0);
    void stopServer();
    bool isServerRunning() const;
    QString serverUrl() const;
    int serverPort() const;

    // 构建
    bool build(const QString &outputDir = QString());
    bool clean();

    // 主题管理
    QVector<Theme> listAvailableThemes();
    QVector<Theme> listInstalledThemes();
    bool installTheme(const QString &themeName);
    bool setTheme(const QString &themeName);
    QString currentTheme() const;

    // 部署
    bool deployToGitHubPages(const QString &repo, const QString &branch = "gh-pages");
    bool deployToNetlify(const QString &siteId, const QString &token);
    bool deployToVercel(const QString &token);

    // 错误信息
    QString lastError() const { return m_lastError; }
    QString lastOutput() const { return m_lastOutput; }

signals:
    void siteOpened(const QString &path);
    void siteClosed();
    void postCreated(const QString &filePath);
    void postSaved(const QString &filePath);
    void postDeleted(const QString &filePath);
    void serverStarted(const QString &url);
    void serverStopped();
    void serverOutput(const QString &output);
    void buildStarted();
    void buildFinished(bool success);
    void buildProgress(const QString &message);
    void deployStarted();
    void deployFinished(bool success);
    void errorOccurred(const QString &error);

private slots:
    void onServerReadyRead();
    void onServerFinished(int exitCode, QProcess::ExitStatus status);
    void onBuildReadyRead();
    void onBuildFinished(int exitCode, QProcess::ExitStatus status);

private:
    // Front Matter 处理
    QString generateFrontMatter(const FrontMatter &fm);
    FrontMatter parseFrontMatter(const QString &content);
    QString extractContent(const QString &fullContent);

    // 文件路径
    QString postsDirectory() const;
    QString pagesDirectory() const;
    QString contentDirectory() const;
    QString themesDirectory() const;
    QString publicDirectory() const;
    QString configFilePath() const;

    // 生成器特定操作
    bool createHugoSite(const QString &path, const QString &name);
    bool createJekyllSite(const QString &path, const QString &name);
    bool createHexoSite(const QString &path, const QString &name);

    QString hugoPostPath(const QString &title);
    QString jekyllPostPath(const QString &title);
    QString hexoPostPath(const QString &title);

    // 配置文件操作
    bool readSiteConfig();
    bool writeSiteConfig();

    // 进程执行
    bool runCommand(const QString &command, const QStringList &args,
                    const QString &workDir = QString(), int timeout = 30000);

    SiteConfig m_config;
    QProcess *m_serverProcess;
    QProcess *m_buildProcess;
    QString m_lastError;
    QString m_lastOutput;
    int m_serverPort;
    bool m_isServerRunning;
};

#endif // STATICSITEGENERATOR_H

