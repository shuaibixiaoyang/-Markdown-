// 文件说明：app-static\publishing\staticsitegenerator.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "staticsitegenerator.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDebug>

// 函数说明：构造 StaticSiteGenerator 对象，初始化本模块需要的状态、界面和资源。
StaticSiteGenerator::StaticSiteGenerator(QObject *parent)
    : QObject(parent)
    , m_serverProcess(new QProcess(this))
    , m_buildProcess(new QProcess(this))
    , m_serverPort(0)
    , m_isServerRunning(false)
{
    connect(m_serverProcess, &QProcess::readyReadStandardOutput,
            this, &StaticSiteGenerator::onServerReadyRead);
    connect(m_serverProcess, &QProcess::readyReadStandardError,
            this, &StaticSiteGenerator::onServerReadyRead);
    connect(m_serverProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &StaticSiteGenerator::onServerFinished);

    connect(m_buildProcess, &QProcess::readyReadStandardOutput,
            this, &StaticSiteGenerator::onBuildReadyRead);
    connect(m_buildProcess, &QProcess::readyReadStandardError,
            this, &StaticSiteGenerator::onBuildReadyRead);
    connect(m_buildProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &StaticSiteGenerator::onBuildFinished);
}

// 函数说明：销毁 StaticSiteGenerator 对象，释放本模块持有的资源。
StaticSiteGenerator::~StaticSiteGenerator()
{
    stopServer();
}

// 函数说明：实现 StaticSiteGenerator::detectInstalledGenerators 的核心逻辑，供当前模块调用。
QMap<StaticSiteGenerator::GeneratorType, QString> StaticSiteGenerator::detectInstalledGenerators()
{
    QMap<GeneratorType, QString> installed;

    // 检测 Hugo
    QProcess hugoProcess;
    hugoProcess.start("hugo", QStringList() << "version");
    if (hugoProcess.waitForFinished(5000) && hugoProcess.exitCode() == 0) {
        QString version = QString::fromUtf8(hugoProcess.readAllStandardOutput()).trimmed();
        installed[GeneratorType::Hugo] = version;
    }

    // 检测 Jekyll
    QProcess jekyllProcess;
    jekyllProcess.start("jekyll", QStringList() << "--version");
    if (jekyllProcess.waitForFinished(5000) && jekyllProcess.exitCode() == 0) {
        QString version = QString::fromUtf8(jekyllProcess.readAllStandardOutput()).trimmed();
        installed[GeneratorType::Jekyll] = version;
    }

    // 检测 Hexo
    QProcess hexoProcess;
    hexoProcess.start("hexo", QStringList() << "version");
    if (hexoProcess.waitForFinished(5000) && hexoProcess.exitCode() == 0) {
        QString version = QString::fromUtf8(hexoProcess.readAllStandardOutput()).trimmed();
        installed[GeneratorType::Hexo] = version;
    }

    // 检测 MkDocs
    QProcess mkdocsProcess;
    mkdocsProcess.start("mkdocs", QStringList() << "--version");
    if (mkdocsProcess.waitForFinished(5000) && mkdocsProcess.exitCode() == 0) {
        QString version = QString::fromUtf8(mkdocsProcess.readAllStandardOutput()).trimmed();
        installed[GeneratorType::MkDocs] = version;
    }

    return installed;
}

// 函数说明：判断 StaticSiteGenerator 当前是否满足指定状态。
bool StaticSiteGenerator::isGeneratorInstalled(GeneratorType type)
{
    QString cmd = getGeneratorCommand(type);
    QProcess process;
    process.start(cmd, QStringList() << "--version");
    return process.waitForFinished(5000) && process.exitCode() == 0;
}

// 函数说明：读取 StaticSiteGenerator 当前保存的状态或计算结果。
QString StaticSiteGenerator::getGeneratorCommand(GeneratorType type)
{
    switch (type) {
        case GeneratorType::Hugo: return "hugo";
        case GeneratorType::Jekyll: return "jekyll";
        case GeneratorType::Hexo: return "hexo";
        case GeneratorType::VuePress: return "vuepress";
        case GeneratorType::MkDocs: return "mkdocs";
    }
    return QString();
}

// 函数说明：读取 StaticSiteGenerator 当前保存的状态或计算结果。
QString StaticSiteGenerator::getGeneratorVersion(GeneratorType type)
{
    QString cmd = getGeneratorCommand(type);
    QProcess process;

    if (type == GeneratorType::Hugo) {
        process.start(cmd, QStringList() << "version");
    } else {
        process.start(cmd, QStringList() << "--version");
    }

    if (process.waitForFinished(5000) && process.exitCode() == 0) {
        return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    }
    return QString();
}

// 函数说明：设置 StaticSiteGenerator 的运行参数，并触发必要的界面或数据刷新。
void StaticSiteGenerator::setSiteConfig(const SiteConfig &config)
{
    m_config = config;
}

// 函数说明：创建 StaticSiteGenerator 需要的对象、记录或输出内容。
bool StaticSiteGenerator::createNewSite(const QString &path, const QString &name)
{
    if (!isGeneratorInstalled(m_config.type)) {
        m_lastError = tr("生成器未安装: %1").arg(getGeneratorCommand(m_config.type));
        emit errorOccurred(m_lastError);
        return false;
    }

    QString fullPath = path + "/" + name;

    switch (m_config.type) {
        case GeneratorType::Hugo:
            return createHugoSite(path, name);
        case GeneratorType::Jekyll:
            return createJekyllSite(path, name);
        case GeneratorType::Hexo:
            return createHexoSite(path, name);
        default:
            m_lastError = tr("不支持的生成器类型");
            emit errorOccurred(m_lastError);
            return false;
    }
}

// 函数说明：打开 StaticSiteGenerator 对应的文件、资源或功能入口。
bool StaticSiteGenerator::openSite(const QString &path)
{
    QDir dir(path);
    if (!dir.exists()) {
        m_lastError = tr("目录不存在: %1").arg(path);
        emit errorOccurred(m_lastError);
        return false;
    }

    m_config.sitePath = path;

    // 自动检测生成器类型
    if (QFile::exists(path + "/config.toml") ||
        QFile::exists(path + "/config.yaml") ||
        QFile::exists(path + "/hugo.toml")) {
        m_config.type = GeneratorType::Hugo;
    } else if (QFile::exists(path + "/_config.yml")) {
        // 可能是 Jekyll 或 Hexo
        if (QDir(path + "/node_modules/hexo").exists()) {
            m_config.type = GeneratorType::Hexo;
        } else {
            m_config.type = GeneratorType::Jekyll;
        }
    } else if (QFile::exists(path + "/mkdocs.yml")) {
        m_config.type = GeneratorType::MkDocs;
    } else {
        m_lastError = tr("无法识别的站点类型");
        emit errorOccurred(m_lastError);
        return false;
    }

    if (!readSiteConfig()) {
        return false;
    }

    emit siteOpened(path);
    return true;
}

// 函数说明：判断 StaticSiteGenerator 当前是否满足指定状态。
bool StaticSiteGenerator::isValidSite() const
{
    return !m_config.sitePath.isEmpty() && QDir(m_config.sitePath).exists();
}

// 函数说明：创建 StaticSiteGenerator 需要的对象、记录或输出内容。
bool StaticSiteGenerator::createPost(const QString &title, const FrontMatter &frontMatter)
{
    if (!isValidSite()) {
        m_lastError = tr("请先打开一个站点");
        emit errorOccurred(m_lastError);
        return false;
    }

    FrontMatter fm = frontMatter;
    if (fm.title.isEmpty()) fm.title = title;
    if (fm.date.isEmpty()) fm.date = QDateTime::currentDateTime().toString(Qt::ISODate);

    QString filePath;
    switch (m_config.type) {
        case GeneratorType::Hugo:
            filePath = hugoPostPath(title);
            break;
        case GeneratorType::Jekyll:
            filePath = jekyllPostPath(title);
            break;
        case GeneratorType::Hexo:
            filePath = hexoPostPath(title);
            break;
        default:
            filePath = postsDirectory() + "/" + title + ".md";
    }

    // 确保目录存在
    QFileInfo fileInfo(filePath);
    QDir().mkpath(fileInfo.absolutePath());

    // 生成文件内容
    QString content = generateFrontMatter(fm);
    content += "\n\n";
    content += tr("# %1\n\n在此编写文章内容...").arg(title);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = tr("无法创建文件: %1").arg(filePath);
        emit errorOccurred(m_lastError);
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << content;
    file.close();

    emit postCreated(filePath);
    return true;
}

// 函数说明：创建 StaticSiteGenerator 需要的对象、记录或输出内容。
bool StaticSiteGenerator::createPage(const QString &title, const FrontMatter &frontMatter)
{
    if (!isValidSite()) {
        m_lastError = tr("请先打开一个站点");
        emit errorOccurred(m_lastError);
        return false;
    }

    FrontMatter fm = frontMatter;
    if (fm.title.isEmpty()) fm.title = title;
    if (fm.layout.isEmpty()) fm.layout = "page";

    QString filePath = pagesDirectory() + "/" + title.toLower().replace(" ", "-") + ".md";

    QFileInfo fileInfo(filePath);
    QDir().mkpath(fileInfo.absolutePath());

    QString content = generateFrontMatter(fm);
    content += "\n\n";
    content += tr("# %1\n\n在此编写页面内容...").arg(title);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = tr("无法创建文件: %1").arg(filePath);
        emit errorOccurred(m_lastError);
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << content;
    file.close();

    emit postCreated(filePath);
    return true;
}

// 函数说明：保存 StaticSiteGenerator 当前状态，保证用户修改可以持久化。
bool StaticSiteGenerator::savePost(const Post &post)
{
    QString content = generateFrontMatter(post.frontMatter);
    content += "\n\n";
    content += post.content;

    QFile file(post.filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = tr("无法保存文件: %1").arg(post.filePath);
        emit errorOccurred(m_lastError);
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << content;
    file.close();

    emit postSaved(post.filePath);
    return true;
}

// 函数说明：删除 StaticSiteGenerator 管理的指定数据或资源。
bool StaticSiteGenerator::deletePost(const QString &filePath)
{
    if (!QFile::exists(filePath)) {
        m_lastError = tr("文件不存在: %1").arg(filePath);
        emit errorOccurred(m_lastError);
        return false;
    }

    if (!QFile::remove(filePath)) {
        m_lastError = tr("无法删除文件: %1").arg(filePath);
        emit errorOccurred(m_lastError);
        return false;
    }

    emit postDeleted(filePath);
    return true;
}

// 函数说明：实现 StaticSiteGenerator::listPosts 的核心逻辑，供当前模块调用。
QVector<StaticSiteGenerator::Post> StaticSiteGenerator::listPosts(bool includeDrafts)
{
    QVector<Post> posts;

    QString postsDir = postsDirectory();
    QDir dir(postsDir);

    if (!dir.exists()) {
        return posts;
    }

    QStringList filters;
    filters << "*.md" << "*.markdown";

    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Time);

    for (const QFileInfo &fileInfo : files) {
        Post post = loadPost(fileInfo.absoluteFilePath());

        if (!includeDrafts && post.frontMatter.draft) {
            continue;
        }

        posts.append(post);
    }

    return posts;
}

// 函数说明：实现 StaticSiteGenerator::listPages 的核心逻辑，供当前模块调用。
QVector<StaticSiteGenerator::Post> StaticSiteGenerator::listPages()
{
    QVector<Post> pages;

    QString pagesDir = pagesDirectory();
    QDir dir(pagesDir);

    if (!dir.exists()) {
        return pages;
    }

    QStringList filters;
    filters << "*.md" << "*.markdown";

    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Time);

    for (const QFileInfo &fileInfo : files) {
        pages.append(loadPost(fileInfo.absoluteFilePath()));
    }

    return pages;
}

// 函数说明：加载 StaticSiteGenerator 需要的数据、配置或外部资源。
StaticSiteGenerator::Post StaticSiteGenerator::loadPost(const QString &filePath)
{
    Post post;
    post.filePath = filePath;

    QFileInfo fileInfo(filePath);
    post.relativePath = fileInfo.fileName();
    post.createdAt = fileInfo.birthTime();
    post.modifiedAt = fileInfo.lastModified();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return post;
    }

    QString fullContent = QString::fromUtf8(file.readAll());
    file.close();

    post.frontMatter = parseFrontMatter(fullContent);
    post.content = extractContent(fullContent);

    return post;
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
bool StaticSiteGenerator::exportToSite(const QString &markdown, const FrontMatter &frontMatter)
{
    if (!isValidSite()) {
        m_lastError = tr("请先打开一个站点");
        emit errorOccurred(m_lastError);
        return false;
    }

    // 生成文件名
    QString title = frontMatter.title;
    if (title.isEmpty()) {
        // 从 Markdown 提取标题
        QRegularExpression titleRegex("^#\\s+(.+)$", QRegularExpression::MultilineOption);
        QRegularExpressionMatch match = titleRegex.match(markdown);
        if (match.hasMatch()) {
            title = match.captured(1);
        } else {
            title = tr("未命名文章");
        }
    }

    FrontMatter fm = frontMatter;
    fm.title = title;
    if (fm.date.isEmpty()) {
        fm.date = QDateTime::currentDateTime().toString(Qt::ISODate);
    }

    QString filePath;
    switch (m_config.type) {
        case GeneratorType::Hugo:
            filePath = hugoPostPath(title);
            break;
        case GeneratorType::Jekyll:
            filePath = jekyllPostPath(title);
            break;
        case GeneratorType::Hexo:
            filePath = hexoPostPath(title);
            break;
        default:
            filePath = postsDirectory() + "/" + title + ".md";
    }

    QFileInfo fileInfo(filePath);
    QDir().mkpath(fileInfo.absolutePath());

    QString content = generateFrontMatter(fm);
    content += "\n\n";
    content += markdown;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = tr("无法创建文件: %1").arg(filePath);
        emit errorOccurred(m_lastError);
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << content;
    file.close();

    emit postCreated(filePath);
    return true;
}

// 函数说明：启动 StaticSiteGenerator 的异步任务、会话或后台流程。
bool StaticSiteGenerator::startServer(int port)
{
    if (m_isServerRunning) {
        return true;
    }

    if (!isValidSite()) {
        m_lastError = tr("请先打开一个站点");
        emit errorOccurred(m_lastError);
        return false;
    }

    m_serverPort = port > 0 ? port : 1313;
    QString cmd = getGeneratorCommand(m_config.type);
    QStringList args;

    switch (m_config.type) {
        case GeneratorType::Hugo:
            args << "server" << "-p" << QString::number(m_serverPort) << "-D";
            break;
        case GeneratorType::Jekyll:
            args << "serve" << "--port" << QString::number(m_serverPort) << "--drafts";
            break;
        case GeneratorType::Hexo:
            args << "server" << "-p" << QString::number(m_serverPort) << "--draft";
            break;
        case GeneratorType::MkDocs:
            args << "serve" << "-a" << QString("127.0.0.1:%1").arg(m_serverPort);
            break;
        default:
            m_lastError = tr("不支持的生成器类型");
            emit errorOccurred(m_lastError);
            return false;
    }

    m_serverProcess->setWorkingDirectory(m_config.sitePath);
    m_serverProcess->start(cmd, args);

    if (!m_serverProcess->waitForStarted(5000)) {
        m_lastError = tr("无法启动服务器");
        emit errorOccurred(m_lastError);
        return false;
    }

    m_isServerRunning = true;
    emit serverStarted(serverUrl());
    return true;
}

// 函数说明：停止 StaticSiteGenerator 正在运行的任务或会话。
void StaticSiteGenerator::stopServer()
{
    if (!m_isServerRunning) {
        return;
    }

    m_serverProcess->terminate();
    if (!m_serverProcess->waitForFinished(3000)) {
        m_serverProcess->kill();
    }

    m_isServerRunning = false;
    emit serverStopped();
}

// 函数说明：判断 StaticSiteGenerator 当前是否满足指定状态。
bool StaticSiteGenerator::isServerRunning() const
{
    return m_isServerRunning;
}

// 函数说明：实现 StaticSiteGenerator::serverUrl 的核心逻辑，供当前模块调用。
QString StaticSiteGenerator::serverUrl() const
{
    if (!m_isServerRunning) {
        return QString();
    }
    return QString("http://localhost:%1").arg(m_serverPort);
}

// 函数说明：实现 StaticSiteGenerator::serverPort 的核心逻辑，供当前模块调用。
int StaticSiteGenerator::serverPort() const
{
    return m_serverPort;
}

// 函数说明：实现 StaticSiteGenerator::build 的核心逻辑，供当前模块调用。
bool StaticSiteGenerator::build(const QString &outputDir)
{
    if (!isValidSite()) {
        m_lastError = tr("请先打开一个站点");
        emit errorOccurred(m_lastError);
        return false;
    }

    QString cmd = getGeneratorCommand(m_config.type);
    QStringList args;

    switch (m_config.type) {
        case GeneratorType::Hugo:
            args << "--minify";
            if (!outputDir.isEmpty()) {
                args << "-d" << outputDir;
            }
            break;
        case GeneratorType::Jekyll:
            args << "build";
            if (!outputDir.isEmpty()) {
                args << "-d" << outputDir;
            }
            break;
        case GeneratorType::Hexo:
            args << "generate";
            break;
        case GeneratorType::MkDocs:
            args << "build";
            if (!outputDir.isEmpty()) {
                args << "-d" << outputDir;
            }
            break;
        default:
            m_lastError = tr("不支持的生成器类型");
            emit errorOccurred(m_lastError);
            return false;
    }

    emit buildStarted();

    m_buildProcess->setWorkingDirectory(m_config.sitePath);
    m_buildProcess->start(cmd, args);

    return true;
}

// 函数说明：实现 StaticSiteGenerator::clean 的核心逻辑，供当前模块调用。
bool StaticSiteGenerator::clean()
{
    if (!isValidSite()) {
        return false;
    }

    QString publicDir = publicDirectory();
    QDir dir(publicDir);

    if (dir.exists()) {
        return dir.removeRecursively();
    }

    return true;
}

// 函数说明：实现 StaticSiteGenerator::listInstalledThemes 的核心逻辑，供当前模块调用。
QVector<StaticSiteGenerator::Theme> StaticSiteGenerator::listInstalledThemes()
{
    QVector<Theme> themes;

    QString themesDir = themesDirectory();
    QDir dir(themesDir);

    if (!dir.exists()) {
        return themes;
    }

    QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString &subdir : subdirs) {
        Theme theme;
        theme.name = subdir;
        theme.path = themesDir + "/" + subdir;
        theme.isInstalled = true;

        // 尝试读取主题信息
        QString infoFile = theme.path + "/theme.toml";
        if (!QFile::exists(infoFile)) {
            infoFile = theme.path + "/theme.yaml";
        }

        themes.append(theme);
    }

    return themes;
}

// 函数说明：设置 StaticSiteGenerator 的运行参数，并触发必要的界面或数据刷新。
bool StaticSiteGenerator::setTheme(const QString &themeName)
{
    m_config.theme = themeName;
    return writeSiteConfig();
}

// 函数说明：实现 StaticSiteGenerator::currentTheme 的核心逻辑，供当前模块调用。
QString StaticSiteGenerator::currentTheme() const
{
    return m_config.theme;
}

// 函数说明：实现 StaticSiteGenerator::deployToGitHubPages 的核心逻辑，供当前模块调用。
bool StaticSiteGenerator::deployToGitHubPages(const QString &repo, const QString &branch)
{
    if (!isValidSite()) {
        m_lastError = tr("请先打开一个站点");
        emit errorOccurred(m_lastError);
        return false;
    }

    emit deployStarted();

    // 先构建
    if (!build()) {
        emit deployFinished(false);
        return false;
    }

    // 等待构建完成
    if (!m_buildProcess->waitForFinished(60000)) {
        m_lastError = tr("构建超时");
        emit errorOccurred(m_lastError);
        emit deployFinished(false);
        return false;
    }

    QString publicDir = publicDirectory();

    // 初始化 Git（如果需要）
    QProcess gitProcess;
    gitProcess.setWorkingDirectory(publicDir);

    // git init
    gitProcess.start("git", QStringList() << "init");
    gitProcess.waitForFinished();

    // git add
    gitProcess.start("git", QStringList() << "add" << "-A");
    gitProcess.waitForFinished();

    // git commit
    QString commitMsg = QString("Deploy at %1").arg(QDateTime::currentDateTime().toString());
    gitProcess.start("git", QStringList() << "commit" << "-m" << commitMsg);
    gitProcess.waitForFinished();

    // git push
    gitProcess.start("git", QStringList() << "push" << "-f" << repo << QString("HEAD:%1").arg(branch));
    bool success = gitProcess.waitForFinished(120000) && gitProcess.exitCode() == 0;

    if (!success) {
        m_lastError = tr("Git 推送失败: %1").arg(QString::fromUtf8(gitProcess.readAllStandardError()));
        emit errorOccurred(m_lastError);
    }

    emit deployFinished(success);
    return success;
}

// 函数说明：实现 StaticSiteGenerator::deployToNetlify 的核心逻辑，供当前模块调用。
bool StaticSiteGenerator::deployToNetlify(const QString &siteId, const QString &token)
{
    Q_UNUSED(siteId)
    Q_UNUSED(token)

    // Netlify CLI 部署
    emit deployStarted();

    if (!build()) {
        emit deployFinished(false);
        return false;
    }

    m_buildProcess->waitForFinished(60000);

    QProcess netlifyProcess;
    netlifyProcess.setWorkingDirectory(m_config.sitePath);
    netlifyProcess.start("netlify", QStringList() << "deploy" << "--prod" << "--dir" << publicDirectory());

    bool success = netlifyProcess.waitForFinished(120000) && netlifyProcess.exitCode() == 0;

    if (!success) {
        m_lastError = tr("Netlify 部署失败");
        emit errorOccurred(m_lastError);
    }

    emit deployFinished(success);
    return success;
}

// 函数说明：实现 StaticSiteGenerator::deployToVercel 的核心逻辑，供当前模块调用。
bool StaticSiteGenerator::deployToVercel(const QString &token)
{
    Q_UNUSED(token)

    emit deployStarted();

    QProcess vercelProcess;
    vercelProcess.setWorkingDirectory(m_config.sitePath);
    vercelProcess.start("vercel", QStringList() << "--prod");

    bool success = vercelProcess.waitForFinished(120000) && vercelProcess.exitCode() == 0;

    if (!success) {
        m_lastError = tr("Vercel 部署失败");
        emit errorOccurred(m_lastError);
    }

    emit deployFinished(success);
    return success;
}

// 函数说明：响应 StaticSiteGenerator 收到的信号或异步回调，并更新界面状态。
void StaticSiteGenerator::onServerReadyRead()
{
    QString output = QString::fromUtf8(m_serverProcess->readAllStandardOutput());
    output += QString::fromUtf8(m_serverProcess->readAllStandardError());

    m_lastOutput += output;
    emit serverOutput(output);
}

// 函数说明：响应 StaticSiteGenerator 收到的信号或异步回调，并更新界面状态。
void StaticSiteGenerator::onServerFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(status)

    m_isServerRunning = false;
    emit serverStopped();
}

// 函数说明：响应 StaticSiteGenerator 收到的信号或异步回调，并更新界面状态。
void StaticSiteGenerator::onBuildReadyRead()
{
    QString output = QString::fromUtf8(m_buildProcess->readAllStandardOutput());
    output += QString::fromUtf8(m_buildProcess->readAllStandardError());

    m_lastOutput += output;
    emit buildProgress(output);
}

// 函数说明：响应 StaticSiteGenerator 收到的信号或异步回调，并更新界面状态。
void StaticSiteGenerator::onBuildFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(status)

    bool success = exitCode == 0;
    if (!success) {
        m_lastError = tr("构建失败，退出码: %1").arg(exitCode);
        emit errorOccurred(m_lastError);
    }

    emit buildFinished(success);
}

// 函数说明：根据当前数据生成 StaticSiteGenerator 需要的输出结果。
QString StaticSiteGenerator::generateFrontMatter(const FrontMatter &fm)
{
    QString yaml;

    switch (m_config.type) {
        case GeneratorType::Hugo:
            // Hugo 使用 TOML 或 YAML，这里用 YAML
            yaml = "---\n";
            break;
        case GeneratorType::Jekyll:
        case GeneratorType::Hexo:
        default:
            yaml = "---\n";
            break;
    }

    if (!fm.title.isEmpty()) {
        QString escapedTitle = fm.title;
        escapedTitle.replace("\"", "\\\"");
        yaml += QString("title: \"%1\"\n").arg(escapedTitle);
    }

    if (!fm.date.isEmpty()) {
        yaml += QString("date: %1\n").arg(fm.date);
    }

    if (!fm.author.isEmpty()) {
        yaml += QString("author: \"%1\"\n").arg(fm.author);
    }

    if (!fm.description.isEmpty()) {
        QString escapedDesc = fm.description;
        escapedDesc.replace("\"", "\\\"");
        yaml += QString("description: \"%1\"\n").arg(escapedDesc);
    }

    if (!fm.tags.isEmpty()) {
        yaml += "tags:\n";
        for (const QString &tag : fm.tags) {
            yaml += QString("  - %1\n").arg(tag);
        }
    }

    if (!fm.categories.isEmpty()) {
        yaml += "categories:\n";
        for (const QString &cat : fm.categories) {
            yaml += QString("  - %1\n").arg(cat);
        }
    }

    if (!fm.layout.isEmpty()) {
        yaml += QString("layout: %1\n").arg(fm.layout);
    }

    if (!fm.slug.isEmpty()) {
        yaml += QString("slug: %1\n").arg(fm.slug);
    }

    if (!fm.featuredImage.isEmpty()) {
        yaml += QString("featuredImage: \"%1\"\n").arg(fm.featuredImage);
    }

    if (fm.draft) {
        yaml += "draft: true\n";
    }

    if (fm.weight > 0) {
        yaml += QString("weight: %1\n").arg(fm.weight);
    }

    // 自定义字段
    for (auto it = fm.custom.begin(); it != fm.custom.end(); ++it) {
        yaml += QString("%1: %2\n").arg(it.key(), it.value().toString());
    }

    yaml += "---";
    return yaml;
}

// 函数说明：解析输入内容，转换为 StaticSiteGenerator 后续处理使用的数据结构。
StaticSiteGenerator::FrontMatter StaticSiteGenerator::parseFrontMatter(const QString &content)
{
    FrontMatter fm;

    // 检查是否有 Front Matter
    if (!content.startsWith("---")) {
        return fm;
    }

    int endPos = content.indexOf("---", 3);
    if (endPos < 0) {
        return fm;
    }

    QString yamlContent = content.mid(3, endPos - 3).trimmed();
    QStringList lines = yamlContent.split('\n');

    QString currentKey;
    bool inArray = false;
    QStringList arrayValues;

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();

        if (trimmed.isEmpty()) continue;

        // 数组项
        if (trimmed.startsWith("- ")) {
            if (inArray) {
                arrayValues << trimmed.mid(2);
            }
            continue;
        }

        // 结束数组
        if (inArray && !trimmed.startsWith("  ")) {
            if (currentKey == "tags") {
                fm.tags = arrayValues;
            } else if (currentKey == "categories") {
                fm.categories = arrayValues;
            }
            inArray = false;
            arrayValues.clear();
        }

        // 键值对
        int colonPos = trimmed.indexOf(':');
        if (colonPos > 0) {
            QString key = trimmed.left(colonPos).trimmed();
            QString value = trimmed.mid(colonPos + 1).trimmed();

            // 移除引号
            if (value.startsWith('"') && value.endsWith('"')) {
                value = value.mid(1, value.length() - 2);
            }

            if (value.isEmpty()) {
                // 可能是数组开始
                currentKey = key;
                inArray = true;
                arrayValues.clear();
                continue;
            }

            if (key == "title") {
                fm.title = value;
            } else if (key == "date") {
                fm.date = value;
            } else if (key == "author") {
                fm.author = value;
            } else if (key == "description") {
                fm.description = value;
            } else if (key == "layout") {
                fm.layout = value;
            } else if (key == "slug") {
                fm.slug = value;
            } else if (key == "featuredImage") {
                fm.featuredImage = value;
            } else if (key == "draft") {
                fm.draft = (value.toLower() == "true");
            } else if (key == "weight") {
                fm.weight = value.toInt();
            } else {
                fm.custom[key] = value;
            }
        }
    }

    // 处理最后的数组
    if (inArray) {
        if (currentKey == "tags") {
            fm.tags = arrayValues;
        } else if (currentKey == "categories") {
            fm.categories = arrayValues;
        }
    }

    return fm;
}

// 函数说明：实现 StaticSiteGenerator::extractContent 的核心逻辑，供当前模块调用。
QString StaticSiteGenerator::extractContent(const QString &fullContent)
{
    if (!fullContent.startsWith("---")) {
        return fullContent;
    }

    int endPos = fullContent.indexOf("---", 3);
    if (endPos < 0) {
        return fullContent;
    }

    return fullContent.mid(endPos + 3).trimmed();
}

// 函数说明：实现 StaticSiteGenerator::postsDirectory 的核心逻辑，供当前模块调用。
QString StaticSiteGenerator::postsDirectory() const
{
    switch (m_config.type) {
        case GeneratorType::Hugo:
            return m_config.sitePath + "/content/posts";
        case GeneratorType::Jekyll:
            return m_config.sitePath + "/_posts";
        case GeneratorType::Hexo:
            return m_config.sitePath + "/source/_posts";
        case GeneratorType::MkDocs:
            return m_config.sitePath + "/docs";
        default:
            return m_config.sitePath + "/content";
    }
}

// 函数说明：实现 StaticSiteGenerator::pagesDirectory 的核心逻辑，供当前模块调用。
QString StaticSiteGenerator::pagesDirectory() const
{
    switch (m_config.type) {
        case GeneratorType::Hugo:
            return m_config.sitePath + "/content";
        case GeneratorType::Jekyll:
            return m_config.sitePath;
        case GeneratorType::Hexo:
            return m_config.sitePath + "/source";
        default:
            return m_config.sitePath + "/content";
    }
}

// 函数说明：实现 StaticSiteGenerator::contentDirectory 的核心逻辑，供当前模块调用。
QString StaticSiteGenerator::contentDirectory() const
{
    switch (m_config.type) {
        case GeneratorType::Hugo:
            return m_config.sitePath + "/content";
        case GeneratorType::Jekyll:
            return m_config.sitePath;
        case GeneratorType::Hexo:
            return m_config.sitePath + "/source";
        case GeneratorType::MkDocs:
            return m_config.sitePath + "/docs";
        default:
            return m_config.sitePath + "/content";
    }
}

// 函数说明：实现 StaticSiteGenerator::themesDirectory 的核心逻辑，供当前模块调用。
QString StaticSiteGenerator::themesDirectory() const
{
    switch (m_config.type) {
        case GeneratorType::Hugo:
            return m_config.sitePath + "/themes";
        case GeneratorType::Jekyll:
            return m_config.sitePath + "/_themes";
        case GeneratorType::Hexo:
            return m_config.sitePath + "/themes";
        default:
            return m_config.sitePath + "/themes";
    }
}

// 函数说明：实现 StaticSiteGenerator::publicDirectory 的核心逻辑，供当前模块调用。
QString StaticSiteGenerator::publicDirectory() const
{
    switch (m_config.type) {
        case GeneratorType::Hugo:
            return m_config.sitePath + "/public";
        case GeneratorType::Jekyll:
            return m_config.sitePath + "/_site";
        case GeneratorType::Hexo:
            return m_config.sitePath + "/public";
        case GeneratorType::MkDocs:
            return m_config.sitePath + "/site";
        default:
            return m_config.sitePath + "/public";
    }
}

// 函数说明：实现 StaticSiteGenerator::configFilePath 的核心逻辑，供当前模块调用。
QString StaticSiteGenerator::configFilePath() const
{
    switch (m_config.type) {
        case GeneratorType::Hugo:
            if (QFile::exists(m_config.sitePath + "/hugo.toml")) {
                return m_config.sitePath + "/hugo.toml";
            }
            return m_config.sitePath + "/config.toml";
        case GeneratorType::Jekyll:
        case GeneratorType::Hexo:
            return m_config.sitePath + "/_config.yml";
        case GeneratorType::MkDocs:
            return m_config.sitePath + "/mkdocs.yml";
        default:
            return m_config.sitePath + "/config.toml";
    }
}

// 函数说明：创建 StaticSiteGenerator 需要的对象、记录或输出内容。
bool StaticSiteGenerator::createHugoSite(const QString &path, const QString &name)
{
    QProcess process;
    process.setWorkingDirectory(path);
    process.start("hugo", QStringList() << "new" << "site" << name);

    if (!process.waitForFinished(30000) || process.exitCode() != 0) {
        m_lastError = tr("Hugo 创建站点失败: %1")
            .arg(QString::fromUtf8(process.readAllStandardError()));
        emit errorOccurred(m_lastError);
        return false;
    }

    m_config.sitePath = path + "/" + name;
    return true;
}

// 函数说明：创建 StaticSiteGenerator 需要的对象、记录或输出内容。
bool StaticSiteGenerator::createJekyllSite(const QString &path, const QString &name)
{
    QProcess process;
    process.setWorkingDirectory(path);
    process.start("jekyll", QStringList() << "new" << name);

    if (!process.waitForFinished(60000) || process.exitCode() != 0) {
        m_lastError = tr("Jekyll 创建站点失败: %1")
            .arg(QString::fromUtf8(process.readAllStandardError()));
        emit errorOccurred(m_lastError);
        return false;
    }

    m_config.sitePath = path + "/" + name;
    return true;
}

// 函数说明：创建 StaticSiteGenerator 需要的对象、记录或输出内容。
bool StaticSiteGenerator::createHexoSite(const QString &path, const QString &name)
{
    QProcess process;
    process.setWorkingDirectory(path);
    process.start("hexo", QStringList() << "init" << name);

    if (!process.waitForFinished(120000) || process.exitCode() != 0) {
        m_lastError = tr("Hexo 创建站点失败: %1")
            .arg(QString::fromUtf8(process.readAllStandardError()));
        emit errorOccurred(m_lastError);
        return false;
    }

    m_config.sitePath = path + "/" + name;

    // 安装依赖
    QProcess npmProcess;
    npmProcess.setWorkingDirectory(m_config.sitePath);
    npmProcess.start("npm", QStringList() << "install");
    npmProcess.waitForFinished(180000);

    return true;
}

// 函数说明：实现 StaticSiteGenerator::hugoPostPath 的核心逻辑，供当前模块调用。
QString StaticSiteGenerator::hugoPostPath(const QString &title)
{
    QString slug = title.toLower()
        .replace(QRegularExpression("[^a-z0-9\\u4e00-\\u9fff]+"), "-")
        .replace(QRegularExpression("^-|-$"), "");

    return postsDirectory() + "/" + slug + ".md";
}

// 函数说明：实现 StaticSiteGenerator::jekyllPostPath 的核心逻辑，供当前模块调用。
QString StaticSiteGenerator::jekyllPostPath(const QString &title)
{
    QString date = QDate::currentDate().toString("yyyy-MM-dd");
    QString slug = title.toLower()
        .replace(QRegularExpression("[^a-z0-9\\u4e00-\\u9fff]+"), "-")
        .replace(QRegularExpression("^-|-$"), "");

    return postsDirectory() + "/" + date + "-" + slug + ".md";
}

// 函数说明：实现 StaticSiteGenerator::hexoPostPath 的核心逻辑，供当前模块调用。
QString StaticSiteGenerator::hexoPostPath(const QString &title)
{
    QString slug = title.toLower()
        .replace(QRegularExpression("[^a-z0-9\\u4e00-\\u9fff]+"), "-")
        .replace(QRegularExpression("^-|-$"), "");

    return postsDirectory() + "/" + slug + ".md";
}

// 函数说明：读取 StaticSiteGenerator 的配置或数据，并同步到运行时状态。
bool StaticSiteGenerator::readSiteConfig()
{
    QString configPath = configFilePath();

    if (!QFile::exists(configPath)) {
        m_lastError = tr("配置文件不存在: %1").arg(configPath);
        return false;
    }

    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = tr("无法读取配置文件");
        return false;
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    // 简单解析（支持 TOML 和 YAML 的基本格式）
    QRegularExpression titleRegex("(?:title|site_name)\\s*[:=]\\s*[\"']?([^\"'\\n]+)[\"']?");
    QRegularExpressionMatch titleMatch = titleRegex.match(content);
    if (titleMatch.hasMatch()) {
        m_config.title = titleMatch.captured(1).trimmed();
    }

    QRegularExpression baseUrlRegex("(?:baseURL|baseurl|url|site_url)\\s*[:=]\\s*[\"']?([^\"'\\n]+)[\"']?");
    QRegularExpressionMatch baseUrlMatch = baseUrlRegex.match(content);
    if (baseUrlMatch.hasMatch()) {
        m_config.baseUrl = baseUrlMatch.captured(1).trimmed();
    }

    QRegularExpression themeRegex("theme\\s*[:=]\\s*[\"']?([^\"'\\n]+)[\"']?");
    QRegularExpressionMatch themeMatch = themeRegex.match(content);
    if (themeMatch.hasMatch()) {
        m_config.theme = themeMatch.captured(1).trimmed();
    }

    return true;
}

// 函数说明：写入 StaticSiteGenerator 的配置或数据，用于下次启动恢复。
bool StaticSiteGenerator::writeSiteConfig()
{
    // 读取现有配置
    QString configPath = configFilePath();
    QFile file(configPath);

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        m_lastError = tr("无法写入配置文件");
        return false;
    }

    QString content = QString::fromUtf8(file.readAll());

    // 更新主题设置
    QRegularExpression themeRegex("(theme\\s*[:=]\\s*)[\"']?[^\"'\\n]+[\"']?");
    if (themeRegex.match(content).hasMatch()) {
        content.replace(themeRegex, QString("\\1\"%1\"").arg(m_config.theme));
    }

    file.seek(0);
    file.resize(0);

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << content;
    file.close();

    return true;
}

// 函数说明：实现 StaticSiteGenerator::runCommand 的核心逻辑，供当前模块调用。
bool StaticSiteGenerator::runCommand(const QString &command, const QStringList &args,
                                      const QString &workDir, int timeout)
{
    QProcess process;

    if (!workDir.isEmpty()) {
        process.setWorkingDirectory(workDir);
    } else if (!m_config.sitePath.isEmpty()) {
        process.setWorkingDirectory(m_config.sitePath);
    }

    process.start(command, args);

    if (!process.waitForFinished(timeout)) {
        m_lastError = tr("命令执行超时: %1").arg(command);
        return false;
    }

    m_lastOutput = QString::fromUtf8(process.readAllStandardOutput());
    m_lastOutput += QString::fromUtf8(process.readAllStandardError());

    if (process.exitCode() != 0) {
        m_lastError = tr("命令执行失败: %1").arg(m_lastOutput);
        return false;
    }

    return true;
}

// 函数说明：实现 StaticSiteGenerator::listAvailableThemes 的核心逻辑，供当前模块调用。
QVector<StaticSiteGenerator::Theme> StaticSiteGenerator::listAvailableThemes()
{
    // 返回一些常用主题
    QVector<Theme> themes;

    if (m_config.type == GeneratorType::Hugo) {
        themes.append({"ananke", "", "", "theNewDynamic", "A theme built for the Hugo framework", "https://themes.gohugo.io/themes/gohugo-theme-ananke/", false});
        themes.append({"PaperMod", "", "", "adityatelange", "A fast, clean Hugo theme", "https://themes.gohugo.io/themes/hugo-papermod/", false});
        themes.append({"stack", "", "", "CaiJimmy", "Card-style Hugo theme", "https://themes.gohugo.io/themes/hugo-theme-stack/", false});
    } else if (m_config.type == GeneratorType::Hexo) {
        themes.append({"next", "", "", "theme-next", "Elegant theme for Hexo", "https://theme-next.js.org/", false});
        themes.append({"butterfly", "", "", "jerryc127", "A butterfly theme for Hexo", "https://butterfly.js.org/", false});
    }

    return themes;
}

// 函数说明：实现 StaticSiteGenerator::installTheme 的核心逻辑，供当前模块调用。
bool StaticSiteGenerator::installTheme(const QString &themeName)
{
    if (!isValidSite()) {
        m_lastError = tr("请先打开一个站点");
        emit errorOccurred(m_lastError);
        return false;
    }

    QString themesDir = themesDirectory();
    QDir().mkpath(themesDir);

    // 使用 git clone 安装主题
    QProcess process;
    process.setWorkingDirectory(themesDir);

    QString repoUrl;
    if (m_config.type == GeneratorType::Hugo) {
        // Hugo 主题仓库
        repoUrl = QString("https://github.com/theNewDynamic/gohugo-theme-%1.git").arg(themeName);
    } else if (m_config.type == GeneratorType::Hexo) {
        repoUrl = QString("https://github.com/theme-next/hexo-theme-%1.git").arg(themeName);
    }

    process.start("git", QStringList() << "clone" << repoUrl << themeName);

    if (!process.waitForFinished(120000) || process.exitCode() != 0) {
        m_lastError = tr("主题安装失败: %1")
            .arg(QString::fromUtf8(process.readAllStandardError()));
        emit errorOccurred(m_lastError);
        return false;
    }

    return true;
}

