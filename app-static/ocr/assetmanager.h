// 文件说明：app-static\ocr\assetmanager.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QDir>
#include <QMap>
#include <QSet>
#include <QRegularExpression>
#include <QFileSystemWatcher>
#include <QImage>
#include <QCryptographicHash>

/**
 * @brief 资产管理器
 * 
 * 管理 Markdown 文档中引用的所有资产（图片、附件）：
 * - 扫描文档中的资产引用
 * - 管理资产目录
 * - 检测未使用的资产
 * - 检测损坏的引用
 * - 支持资产重命名和移动
 */
class AssetManager : public QObject
{
    Q_OBJECT

public:
    // 资产类型
    enum class AssetType {
        Unknown,
        Image,          // 图片
        PDF,            // PDF 文档
        Video,          // 视频
        Audio,          // 音频
        Archive,        // 压缩包
        Document,       // 其他文档
        Other
    };
    Q_ENUM(AssetType)

    // 资产状态
    enum class AssetStatus {
        Valid,          // 有效
        Missing,        // 文件不存在
        Orphaned,       // 未被引用
        Duplicate,      // 重复文件
        External        // 外部链接
    };
    Q_ENUM(AssetStatus)

    // 资产信息
    struct Asset {
        QString path;               // 相对路径
        QString absolutePath;       // 绝对路径
        QString name;               // 文件名
        AssetType type;
        AssetStatus status;
        qint64 size;
        QDateTime lastModified;
        QStringList referencedBy;   // 引用此资产的位置（行号）
        int referenceCount;
        QString hash;               // 文件哈希（用于检测重复）
        
        // 图片特有
        QSize dimensions;
        QString format;
    };

    // 引用信息
    struct Reference {
        QString assetPath;          // 资产路径
        int lineNumber;             // 行号
        int columnStart;            // 列起始
        int columnEnd;              // 列结束
        QString originalText;       // 原始引用文本
        bool isValid;               // 引用是否有效
        bool isExternal;            // 是否外部链接
    };

    // 扫描结果
    struct ScanResult {
        QVector<Asset> assets;
        QVector<Reference> references;
        QStringList orphanedFiles;  // 未被引用的文件
        QStringList brokenLinks;    // 损坏的引用
        QStringList duplicates;     // 重复文件
        qint64 totalSize;
        qint64 orphanedSize;
    };

    explicit AssetManager(QObject *parent = nullptr);

    // 设置资产目录
    void setAssetDirectory(const QString &dir);
    QString assetDirectory() const { return m_assetDir; }

    // 设置文档路径
    void setDocumentPath(const QString &path);
    QString documentPath() const { return m_documentPath; }

    // 扫描文档中的资产引用
    ScanResult scanDocument(const QString &markdownContent);
    
    // 扫描资产目录
    QVector<Asset> scanAssetDirectory();

    // 获取所有资产
    QVector<Asset> allAssets() const { return m_assets.values().toVector(); }
    
    // 获取资产
    Asset getAsset(const QString &path) const;
    bool hasAsset(const QString &path) const;

    // 添加资产
    bool addAsset(const QString &sourcePath, QString *destPath = nullptr);
    bool addAsset(const QImage &image, const QString &name, QString *destPath = nullptr);
    
    // 删除资产
    bool removeAsset(const QString &path, bool moveToTrash = true);
    
    // 重命名资产
    bool renameAsset(const QString &oldPath, const QString &newPath);
    
    // 移动资产
    bool moveAsset(const QString &path, const QString &newDir);

    // 清理未使用的资产
    QStringList cleanupOrphanedAssets(bool dryRun = true);
    
    // 修复损坏的引用
    QMap<QString, QString> fixBrokenReferences(bool dryRun = true);

    // 查找重复文件
    QMap<QString, QStringList> findDuplicates();

    // 获取资产统计
    struct Statistics {
        int totalCount;
        int imageCount;
        int orphanedCount;
        int brokenRefCount;
        qint64 totalSize;
        qint64 orphanedSize;
    };
    Statistics getStatistics() const;

    // 解析引用路径
    QString resolveAssetPath(const QString &referencePath) const;
    QString makeRelativePath(const QString &absolutePath) const;

    // 监控文件变化
    void enableWatching(bool enable);
    bool isWatching() const { return m_watchingEnabled; }

    // 文件类型判断
    static AssetType getAssetType(const QString &path);
    static bool isImageFile(const QString &path);
    static bool isSupportedAsset(const QString &path);

signals:
    void assetAdded(const Asset &asset);
    void assetRemoved(const QString &path);
    void assetChanged(const QString &path);
    void assetRenamed(const QString &oldPath, const QString &newPath);
    void scanCompleted(const ScanResult &result);
    void orphanedAssetsFound(const QStringList &paths);

private slots:
    void onFileChanged(const QString &path);
    void onDirectoryChanged(const QString &path);

private:
    QVector<Reference> parseReferences(const QString &markdownContent);
    QString calculateFileHash(const QString &path) const;
    void updateAssetInfo(Asset &asset);
    
    QString m_assetDir;
    QString m_documentPath;
    QMap<QString, Asset> m_assets;
    QFileSystemWatcher *m_watcher;
    bool m_watchingEnabled;
    
    // 支持的图片格式
    static const QStringList IMAGE_EXTENSIONS;
};


// ==================== 实现 ====================

inline const QStringList AssetManager::IMAGE_EXTENSIONS = {
    "png", "jpg", "jpeg", "gif", "bmp", "webp", "svg", "ico", "tiff", "tif"
};

inline AssetManager::AssetManager(QObject *parent)
    : QObject(parent)
    , m_watcher(new QFileSystemWatcher(this))
    , m_watchingEnabled(false)
{
    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &AssetManager::onFileChanged);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &AssetManager::onDirectoryChanged);
}

inline void AssetManager::setAssetDirectory(const QString &dir)
{
    m_assetDir = dir;
    
    // 确保目录存在
    QDir().mkpath(m_assetDir);
    
    if (m_watchingEnabled) {
        m_watcher->addPath(m_assetDir);
    }
}

inline void AssetManager::setDocumentPath(const QString &path)
{
    m_documentPath = path;
    
    // 如果未设置资产目录，使用文档同级的 assets 目录
    if (m_assetDir.isEmpty()) {
        QFileInfo info(path);
        m_assetDir = info.absolutePath() + "/assets";
        QDir().mkpath(m_assetDir);
    }
}

inline AssetManager::ScanResult AssetManager::scanDocument(const QString &markdownContent)
{
    ScanResult result;
    result.totalSize = 0;
    result.orphanedSize = 0;
    
    // 解析文档中的引用
    result.references = parseReferences(markdownContent);
    
    // 收集引用的资产路径
    QSet<QString> referencedPaths;
    for (const Reference &ref : result.references) {
        if (!ref.isExternal) {
            QString resolved = resolveAssetPath(ref.assetPath);
            referencedPaths.insert(resolved);
            
            if (!QFile::exists(resolved)) {
                result.brokenLinks << ref.assetPath;
            }
        }
    }
    
    // 扫描资产目录
    QVector<Asset> dirAssets = scanAssetDirectory();
    
    for (Asset &asset : dirAssets) {
        if (referencedPaths.contains(asset.absolutePath)) {
            asset.status = AssetStatus::Valid;
            
            // 统计引用
            for (const Reference &ref : result.references) {
                if (resolveAssetPath(ref.assetPath) == asset.absolutePath) {
                    asset.referencedBy << QString::number(ref.lineNumber);
                    asset.referenceCount++;
                }
            }
        } else {
            asset.status = AssetStatus::Orphaned;
            result.orphanedFiles << asset.path;
            result.orphanedSize += asset.size;
        }
        
        result.assets.append(asset);
        result.totalSize += asset.size;
    }
    
    // 检测重复
    QMap<QString, QStringList> hashGroups;
    for (const Asset &asset : result.assets) {
        if (!asset.hash.isEmpty()) {
            hashGroups[asset.hash] << asset.path;
        }
    }
    for (auto it = hashGroups.begin(); it != hashGroups.end(); ++it) {
        if (it.value().size() > 1) {
            result.duplicates << it.value();
        }
    }
    
    emit scanCompleted(result);
    
    if (!result.orphanedFiles.isEmpty()) {
        emit orphanedAssetsFound(result.orphanedFiles);
    }
    
    return result;
}

inline QVector<AssetManager::Asset> AssetManager::scanAssetDirectory()
{
    QVector<Asset> assets;
    
    if (m_assetDir.isEmpty() || !QDir(m_assetDir).exists()) {
        return assets;
    }
    
    QDir dir(m_assetDir);
    QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    
    for (const QFileInfo &info : files) {
        if (isSupportedAsset(info.filePath())) {
            Asset asset;
            asset.absolutePath = info.absoluteFilePath();
            asset.path = makeRelativePath(asset.absolutePath);
            asset.name = info.fileName();
            asset.type = getAssetType(info.filePath());
            asset.size = info.size();
            asset.lastModified = info.lastModified();
            asset.referenceCount = 0;
            asset.status = AssetStatus::Valid;
            
            updateAssetInfo(asset);
            
            assets.append(asset);
            m_assets[asset.path] = asset;
        }
    }
    
    return assets;
}

inline AssetManager::Asset AssetManager::getAsset(const QString &path) const
{
    return m_assets.value(path);
}

inline bool AssetManager::hasAsset(const QString &path) const
{
    return m_assets.contains(path);
}

inline bool AssetManager::addAsset(const QString &sourcePath, QString *destPath)
{
    if (!QFile::exists(sourcePath)) {
        return false;
    }
    
    QFileInfo info(sourcePath);
    QString destName = info.fileName();
    QString destination = m_assetDir + "/" + destName;
    
    // 处理同名文件
    int counter = 1;
    while (QFile::exists(destination)) {
        destName = QString("%1_%2.%3")
            .arg(info.completeBaseName())
            .arg(counter++)
            .arg(info.suffix());
        destination = m_assetDir + "/" + destName;
    }
    
    if (!QFile::copy(sourcePath, destination)) {
        return false;
    }
    
    if (destPath) {
        *destPath = makeRelativePath(destination);
    }
    
    // 添加到管理器
    Asset asset;
    asset.absolutePath = destination;
    asset.path = makeRelativePath(destination);
    asset.name = destName;
    asset.type = getAssetType(destination);
    updateAssetInfo(asset);
    
    m_assets[asset.path] = asset;
    emit assetAdded(asset);
    
    return true;
}

inline bool AssetManager::addAsset(const QImage &image, const QString &name, QString *destPath)
{
    if (image.isNull()) {
        return false;
    }
    
    QString destination = m_assetDir + "/" + name;
    
    // 处理同名文件
    QFileInfo info(destination);
    int counter = 1;
    while (QFile::exists(destination)) {
        destination = m_assetDir + "/" + 
            QString("%1_%2.%3")
                .arg(info.completeBaseName())
                .arg(counter++)
                .arg(info.suffix());
    }
    
    if (!image.save(destination)) {
        return false;
    }
    
    if (destPath) {
        *destPath = makeRelativePath(destination);
    }
    
    // 添加到管理器
    Asset asset;
    asset.absolutePath = destination;
    asset.path = makeRelativePath(destination);
    asset.name = QFileInfo(destination).fileName();
    asset.type = AssetType::Image;
    asset.dimensions = image.size();
    updateAssetInfo(asset);
    
    m_assets[asset.path] = asset;
    emit assetAdded(asset);
    
    return true;
}

inline bool AssetManager::removeAsset(const QString &path, bool moveToTrash)
{
    QString absolutePath = resolveAssetPath(path);
    
    if (!QFile::exists(absolutePath)) {
        m_assets.remove(path);
        return true;
    }
    
    bool success;
    if (moveToTrash) {
        success = QFile::moveToTrash(absolutePath);
    } else {
        success = QFile::remove(absolutePath);
    }
    
    if (success) {
        m_assets.remove(path);
        emit assetRemoved(path);
    }
    
    return success;
}

inline bool AssetManager::renameAsset(const QString &oldPath, const QString &newPath)
{
    QString oldAbsolute = resolveAssetPath(oldPath);
    QString newAbsolute = resolveAssetPath(newPath);
    
    if (!QFile::exists(oldAbsolute)) {
        return false;
    }
    
    if (QFile::exists(newAbsolute)) {
        return false;
    }
    
    if (!QFile::rename(oldAbsolute, newAbsolute)) {
        return false;
    }
    
    // 更新管理器
    Asset asset = m_assets.take(oldPath);
    asset.path = newPath;
    asset.absolutePath = newAbsolute;
    asset.name = QFileInfo(newAbsolute).fileName();
    m_assets[newPath] = asset;
    
    emit assetRenamed(oldPath, newPath);
    
    return true;
}

inline bool AssetManager::moveAsset(const QString &path, const QString &newDir)
{
    Asset asset = getAsset(path);
    QString newPath = newDir + "/" + asset.name;
    return renameAsset(path, newPath);
}

inline QStringList AssetManager::cleanupOrphanedAssets(bool dryRun)
{
    QStringList cleaned;
    
    for (auto it = m_assets.begin(); it != m_assets.end(); ) {
        if (it->status == AssetStatus::Orphaned) {
            cleaned << it->path;
            
            if (!dryRun) {
                removeAsset(it->path, true);
                it = m_assets.erase(it);
                continue;
            }
        }
        ++it;
    }
    
    return cleaned;
}

inline QMap<QString, QString> AssetManager::fixBrokenReferences(bool dryRun)
{
    Q_UNUSED(dryRun)
    // 需要文档编辑能力来修复引用
    // 返回建议的修复映射
    return QMap<QString, QString>();
}

inline QMap<QString, QStringList> AssetManager::findDuplicates()
{
    QMap<QString, QStringList> duplicates;
    QMap<QString, QStringList> hashGroups;
    
    for (const Asset &asset : m_assets) {
        QString hash = calculateFileHash(asset.absolutePath);
        hashGroups[hash] << asset.path;
    }
    
    for (auto it = hashGroups.begin(); it != hashGroups.end(); ++it) {
        if (it.value().size() > 1) {
            duplicates[it.key()] = it.value();
        }
    }
    
    return duplicates;
}

inline AssetManager::Statistics AssetManager::getStatistics() const
{
    Statistics stats = {0, 0, 0, 0, 0, 0};
    
    for (const Asset &asset : m_assets) {
        stats.totalCount++;
        stats.totalSize += asset.size;
        
        if (asset.type == AssetType::Image) {
            stats.imageCount++;
        }
        
        if (asset.status == AssetStatus::Orphaned) {
            stats.orphanedCount++;
            stats.orphanedSize += asset.size;
        }
    }
    
    return stats;
}

inline QString AssetManager::resolveAssetPath(const QString &referencePath) const
{
    if (QFileInfo(referencePath).isAbsolute()) {
        return referencePath;
    }
    
    // 相对于资产目录
    if (QFile::exists(m_assetDir + "/" + referencePath)) {
        return m_assetDir + "/" + referencePath;
    }
    
    // 相对于文档目录
    if (!m_documentPath.isEmpty()) {
        QString docDir = QFileInfo(m_documentPath).absolutePath();
        if (QFile::exists(docDir + "/" + referencePath)) {
            return docDir + "/" + referencePath;
        }
    }
    
    return referencePath;
}

inline QString AssetManager::makeRelativePath(const QString &absolutePath) const
{
    if (m_assetDir.isEmpty()) {
        return absolutePath;
    }
    
    QDir assetDir(m_assetDir);
    return assetDir.relativeFilePath(absolutePath);
}

inline void AssetManager::enableWatching(bool enable)
{
    m_watchingEnabled = enable;
    
    if (enable && !m_assetDir.isEmpty()) {
        m_watcher->addPath(m_assetDir);
    } else {
        m_watcher->removePaths(m_watcher->directories());
    }
}

inline AssetManager::AssetType AssetManager::getAssetType(const QString &path)
{
    QString ext = QFileInfo(path).suffix().toLower();
    
    if (IMAGE_EXTENSIONS.contains(ext)) {
        return AssetType::Image;
    }
    if (ext == "pdf") {
        return AssetType::PDF;
    }
    if (ext == "mp4" || ext == "avi" || ext == "mov" || ext == "webm") {
        return AssetType::Video;
    }
    if (ext == "mp3" || ext == "wav" || ext == "ogg" || ext == "flac") {
        return AssetType::Audio;
    }
    if (ext == "zip" || ext == "rar" || ext == "7z" || ext == "tar" || ext == "gz") {
        return AssetType::Archive;
    }
    if (ext == "doc" || ext == "docx" || ext == "xls" || ext == "xlsx" || ext == "ppt") {
        return AssetType::Document;
    }
    
    return AssetType::Unknown;
}

inline bool AssetManager::isImageFile(const QString &path)
{
    QString ext = QFileInfo(path).suffix().toLower();
    return IMAGE_EXTENSIONS.contains(ext);
}

inline bool AssetManager::isSupportedAsset(const QString &path)
{
    AssetType type = getAssetType(path);
    return type != AssetType::Unknown;
}

inline void AssetManager::onFileChanged(const QString &path)
{
    QString relativePath = makeRelativePath(path);
    if (m_assets.contains(relativePath)) {
        updateAssetInfo(m_assets[relativePath]);
        emit assetChanged(relativePath);
    }
}

inline void AssetManager::onDirectoryChanged(const QString &path)
{
    Q_UNUSED(path)
    // 重新扫描目录
    scanAssetDirectory();
}

inline QVector<AssetManager::Reference> AssetManager::parseReferences(
    const QString &markdownContent)
{
    QVector<Reference> refs;
    
    // 匹配 Markdown 图片语法: ![alt](path) 或 ![alt](path "title")
    QRegularExpression imgRegex(
        R"(!\[([^\]]*)\]\(([^)\s]+)(?:\s+"[^"]*")?\))");
    
    // 匹配 HTML img 标签: <img src="path" ...>
    QRegularExpression htmlImgRegex(
        R"(<img[^>]*\ssrc=["']([^"']+)["'][^>]*>)", 
        QRegularExpression::CaseInsensitiveOption);
    
    QStringList lines = markdownContent.split('\n');
    
    for (int i = 0; i < lines.size(); ++i) {
        const QString &line = lines[i];
        
        // Markdown 图片
        QRegularExpressionMatchIterator it = imgRegex.globalMatch(line);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            
            Reference ref;
            ref.lineNumber = i + 1;
            ref.columnStart = match.capturedStart();
            ref.columnEnd = match.capturedEnd();
            ref.originalText = match.captured(0);
            ref.assetPath = match.captured(2);
            ref.isExternal = ref.assetPath.startsWith("http://") || 
                             ref.assetPath.startsWith("https://");
            ref.isValid = ref.isExternal || 
                          QFile::exists(resolveAssetPath(ref.assetPath));
            
            refs.append(ref);
        }
        
        // HTML img
        it = htmlImgRegex.globalMatch(line);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            
            Reference ref;
            ref.lineNumber = i + 1;
            ref.columnStart = match.capturedStart();
            ref.columnEnd = match.capturedEnd();
            ref.originalText = match.captured(0);
            ref.assetPath = match.captured(1);
            ref.isExternal = ref.assetPath.startsWith("http://") || 
                             ref.assetPath.startsWith("https://");
            ref.isValid = ref.isExternal || 
                          QFile::exists(resolveAssetPath(ref.assetPath));
            
            refs.append(ref);
        }
    }
    
    return refs;
}

inline QString AssetManager::calculateFileHash(const QString &path) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(&file);
    
    return hash.result().toHex();
}

inline void AssetManager::updateAssetInfo(Asset &asset)
{
    QFileInfo info(asset.absolutePath);
    
    asset.size = info.size();
    asset.lastModified = info.lastModified();
    asset.hash = calculateFileHash(asset.absolutePath);
    
    if (asset.type == AssetType::Image) {
        QImage image(asset.absolutePath);
        if (!image.isNull()) {
            asset.dimensions = image.size();
            asset.format = info.suffix().toUpper();
        }
    }
}

#endif // ASSETMANAGER_H

