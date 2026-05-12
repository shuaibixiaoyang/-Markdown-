// 文件说明：app-static\ocr\thumbnailcache.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef THUMBNAILCACHE_H
#define THUMBNAILCACHE_H

#include <QObject>
#include <QPixmap>
#include <QImage>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QDateTime>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>

/**
 * @brief 缩略图缓存管理器
 * 
 * 功能：
 * - 内存缓存（LRU 策略）
 * - 磁盘缓存（持久化）
 * - 异步生成
 * - 自动清理过期缓存
 */
class ThumbnailCache : public QObject
{
    Q_OBJECT

public:
    // 缓存配置
    struct Config {
        int memoryCacheSize = 100;          // 内存中最多缓存数量
        int diskCacheSizeMB = 100;          // 磁盘缓存最大大小（MB）
        int thumbnailSize = 128;            // 缩略图尺寸
        int diskCacheExpireDays = 30;       // 磁盘缓存过期天数
        bool useDiskCache = true;           // 是否使用磁盘缓存
    };

    static ThumbnailCache& instance();

    // 配置
    void setConfig(const Config &config);
    Config config() const { return m_config; }

    // 获取缩略图（同步）
    QPixmap getThumbnail(const QString &filePath, int size = 0);

    // 获取缩略图（异步）
    void getThumbnailAsync(const QString &filePath, int size = 0);

    // 检查缓存
    bool hasCached(const QString &filePath) const;

    // 清除缓存
    void clearMemoryCache();
    void clearDiskCache();
    void clearAll();

    // 移除特定文件的缓存
    void remove(const QString &filePath);

    // 预加载
    void preload(const QStringList &filePaths);

    // 缓存统计
    struct Statistics {
        int memoryCacheCount;
        int memoryCacheHits;
        int memoryCacheMisses;
        qint64 diskCacheSizeBytes;
        int diskCacheCount;
    };
    Statistics statistics() const;

signals:
    void thumbnailReady(const QString &filePath, const QPixmap &thumbnail);
    void preloadProgress(int current, int total);
    void preloadCompleted();

private:
    explicit ThumbnailCache(QObject *parent = nullptr);
    ~ThumbnailCache();

    // 禁止拷贝
    ThumbnailCache(const ThumbnailCache&) = delete;
    ThumbnailCache& operator=(const ThumbnailCache&) = delete;

    QPixmap generateThumbnail(const QString &filePath, int size);
    QString getCacheKey(const QString &filePath, int size) const;
    QString getDiskCachePath(const QString &key) const;
    void loadFromDisk(const QString &key);
    void saveToDisk(const QString &key, const QPixmap &thumbnail);
    void evictOldest();
    void cleanupExpiredDiskCache();

    Config m_config;
    QString m_diskCacheDir;
    
    // 内存缓存
    mutable QMutex m_mutex;
    QHash<QString, QPixmap> m_cache;
    QList<QString> m_accessOrder;  // LRU 顺序
    
    // 统计
    mutable int m_hits;
    mutable int m_misses;
};


/**
 * @brief 截图历史记录管理器
 * 
 * 功能：
 * - 记录截图历史
 * - 缩略图预览
 * - 快速重新使用
 * - 自动清理
 */
class ScreenshotHistory : public QObject
{
    Q_OBJECT

public:
    struct HistoryItem {
        QString id;
        QString filePath;
        QString thumbnailPath;
        QDateTime timestamp;
        QSize originalSize;
        qint64 fileSize;
        QString ocrText;
        bool hasOcr;
    };

    static ScreenshotHistory& instance();

    // 添加记录
    void addItem(const QString &filePath, const QPixmap &thumbnail,
                 const QString &ocrText = QString());

    // 获取历史
    QVector<HistoryItem> getHistory(int limit = 50) const;
    HistoryItem getItem(const QString &id) const;

    // 获取缩略图
    QPixmap getThumbnail(const QString &id) const;

    // 删除记录
    void removeItem(const QString &id);
    void clearHistory();

    // 搜索
    QVector<HistoryItem> search(const QString &query) const;

    // 配置
    void setMaxItems(int count);
    void setHistoryDir(const QString &dir);

    // 导出
    bool exportItem(const QString &id, const QString &outputPath) const;

signals:
    void historyChanged();
    void itemAdded(const HistoryItem &item);
    void itemRemoved(const QString &id);

private:
    explicit ScreenshotHistory(QObject *parent = nullptr);
    ~ScreenshotHistory();

    ScreenshotHistory(const ScreenshotHistory&) = delete;
    ScreenshotHistory& operator=(const ScreenshotHistory&) = delete;

    void loadHistory();
    void saveHistory();
    void cleanupOldItems();
    QString generateId() const;

    QVector<HistoryItem> m_history;
    QString m_historyDir;
    int m_maxItems;
    mutable QMutex m_mutex;
};


/**
 * @brief 剪贴板监控器
 * 
 * 功能：
 * - 监控剪贴板图片
 * - 自动触发处理
 */
class ClipboardMonitor : public QObject
{
    Q_OBJECT

public:
    static ClipboardMonitor& instance();

    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const { return m_monitoring; }

    // 获取剪贴板图片
    QImage currentImage() const;
    bool hasImage() const;

signals:
    void imageAvailable(const QImage &image);

private slots:
    void onClipboardChanged();

private:
    explicit ClipboardMonitor(QObject *parent = nullptr);

    bool m_monitoring;
    QImage m_lastImage;
};


// ==================== ThumbnailCache 实现 ====================

inline ThumbnailCache& ThumbnailCache::instance()
{
    static ThumbnailCache instance;
    return instance;
}

inline ThumbnailCache::ThumbnailCache(QObject *parent)
    : QObject(parent)
    , m_hits(0)
    , m_misses(0)
{
    m_diskCacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                     + "/thumbnails";
    QDir().mkpath(m_diskCacheDir);
    
    // 启动时清理过期缓存
    QTimer::singleShot(5000, this, &ThumbnailCache::cleanupExpiredDiskCache);
}

inline ThumbnailCache::~ThumbnailCache()
{
}

inline void ThumbnailCache::setConfig(const Config &config)
{
    m_config = config;
}

inline QPixmap ThumbnailCache::getThumbnail(const QString &filePath, int size)
{
    if (size <= 0) size = m_config.thumbnailSize;
    
    QString key = getCacheKey(filePath, size);
    
    QMutexLocker locker(&m_mutex);
    
    // 检查内存缓存
    if (m_cache.contains(key)) {
        m_hits++;
        // 更新 LRU
        m_accessOrder.removeOne(key);
        m_accessOrder.prepend(key);
        return m_cache[key];
    }
    
    m_misses++;
    locker.unlock();
    
    // 检查磁盘缓存
    QString diskPath = getDiskCachePath(key);
    if (m_config.useDiskCache && QFile::exists(diskPath)) {
        QPixmap thumbnail(diskPath);
        if (!thumbnail.isNull()) {
            locker.relock();
            m_cache[key] = thumbnail;
            m_accessOrder.prepend(key);
            evictOldest();
            return thumbnail;
        }
    }
    
    // 生成缩略图
    QPixmap thumbnail = generateThumbnail(filePath, size);
    
    if (!thumbnail.isNull()) {
        locker.relock();
        m_cache[key] = thumbnail;
        m_accessOrder.prepend(key);
        evictOldest();
        locker.unlock();
        
        // 保存到磁盘
        if (m_config.useDiskCache) {
            saveToDisk(key, thumbnail);
        }
    }
    
    return thumbnail;
}

inline void ThumbnailCache::getThumbnailAsync(const QString &filePath, int size)
{
    if (size <= 0) size = m_config.thumbnailSize;
    
    QString key = getCacheKey(filePath, size);
    
    // 先检查缓存
    {
        QMutexLocker locker(&m_mutex);
        if (m_cache.contains(key)) {
            emit thumbnailReady(filePath, m_cache[key]);
            return;
        }
    }
    
    // 异步生成
    QFutureWatcher<QPixmap> *watcher = new QFutureWatcher<QPixmap>(this);
    
    connect(watcher, &QFutureWatcher<QPixmap>::finished, this, [this, watcher, filePath, key]() {
        QPixmap thumbnail = watcher->result();
        
        if (!thumbnail.isNull()) {
            QMutexLocker locker(&m_mutex);
            m_cache[key] = thumbnail;
            m_accessOrder.prepend(key);
            evictOldest();
        }
        
        emit thumbnailReady(filePath, thumbnail);
        watcher->deleteLater();
    });
    
    QFuture<QPixmap> future = QtConcurrent::run([this, filePath, size]() {
        return generateThumbnail(filePath, size);
    });
    
    watcher->setFuture(future);
}

inline bool ThumbnailCache::hasCached(const QString &filePath) const
{
    QString key = getCacheKey(filePath, m_config.thumbnailSize);
    
    QMutexLocker locker(&m_mutex);
    if (m_cache.contains(key)) return true;
    
    if (m_config.useDiskCache) {
        return QFile::exists(getDiskCachePath(key));
    }
    
    return false;
}

inline void ThumbnailCache::clearMemoryCache()
{
    QMutexLocker locker(&m_mutex);
    m_cache.clear();
    m_accessOrder.clear();
}

inline void ThumbnailCache::clearDiskCache()
{
    QDir dir(m_diskCacheDir);
    for (const QString &file : dir.entryList(QDir::Files)) {
        dir.remove(file);
    }
}

inline void ThumbnailCache::clearAll()
{
    clearMemoryCache();
    clearDiskCache();
}

inline void ThumbnailCache::remove(const QString &filePath)
{
    QString key = getCacheKey(filePath, m_config.thumbnailSize);
    
    QMutexLocker locker(&m_mutex);
    m_cache.remove(key);
    m_accessOrder.removeOne(key);
    
    QString diskPath = getDiskCachePath(key);
    QFile::remove(diskPath);
}

inline void ThumbnailCache::preload(const QStringList &filePaths)
{
    QtConcurrent::run([this, filePaths]() {
        int total = filePaths.size();
        int current = 0;
        
        for (const QString &path : filePaths) {
            getThumbnail(path);
            current++;
            emit preloadProgress(current, total);
        }
        
        emit preloadCompleted();
    });
}

inline ThumbnailCache::Statistics ThumbnailCache::statistics() const
{
    Statistics stats;
    
    QMutexLocker locker(&m_mutex);
    stats.memoryCacheCount = m_cache.size();
    stats.memoryCacheHits = m_hits;
    stats.memoryCacheMisses = m_misses;
    
    QDir dir(m_diskCacheDir);
    QFileInfoList files = dir.entryInfoList(QDir::Files);
    stats.diskCacheCount = files.size();
    stats.diskCacheSizeBytes = 0;
    for (const QFileInfo &info : files) {
        stats.diskCacheSizeBytes += info.size();
    }
    
    return stats;
}

inline QPixmap ThumbnailCache::generateThumbnail(const QString &filePath, int size)
{
    QImage image(filePath);
    if (image.isNull()) {
        return QPixmap();
    }
    
    QImage scaled = image.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return QPixmap::fromImage(scaled);
}

inline QString ThumbnailCache::getCacheKey(const QString &filePath, int size) const
{
    QFileInfo info(filePath);
    QString data = filePath + QString::number(info.lastModified().toMSecsSinceEpoch()) 
                   + QString::number(size);
    return QCryptographicHash::hash(data.toUtf8(), QCryptographicHash::Md5).toHex();
}

inline QString ThumbnailCache::getDiskCachePath(const QString &key) const
{
    return m_diskCacheDir + "/" + key + ".png";
}

inline void ThumbnailCache::loadFromDisk(const QString &key)
{
    QString path = getDiskCachePath(key);
    if (QFile::exists(path)) {
        QPixmap thumbnail(path);
        if (!thumbnail.isNull()) {
            QMutexLocker locker(&m_mutex);
            m_cache[key] = thumbnail;
            m_accessOrder.prepend(key);
        }
    }
}

inline void ThumbnailCache::saveToDisk(const QString &key, const QPixmap &thumbnail)
{
    QString path = getDiskCachePath(key);
    thumbnail.save(path, "PNG");
}

inline void ThumbnailCache::evictOldest()
{
    while (m_cache.size() > m_config.memoryCacheSize && !m_accessOrder.isEmpty()) {
        QString oldest = m_accessOrder.takeLast();
        m_cache.remove(oldest);
    }
}

inline void ThumbnailCache::cleanupExpiredDiskCache()
{
    if (!m_config.useDiskCache) return;
    
    QDir dir(m_diskCacheDir);
    QDateTime expireDate = QDateTime::currentDateTime().addDays(-m_config.diskCacheExpireDays);
    
    for (const QFileInfo &info : dir.entryInfoList(QDir::Files)) {
        if (info.lastModified() < expireDate) {
            QFile::remove(info.absoluteFilePath());
        }
    }
}


// ==================== ScreenshotHistory 实现 ====================

inline ScreenshotHistory& ScreenshotHistory::instance()
{
    static ScreenshotHistory instance;
    return instance;
}

inline ScreenshotHistory::ScreenshotHistory(QObject *parent)
    : QObject(parent)
    , m_maxItems(100)
{
    m_historyDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                   + "/screenshot_history";
    QDir().mkpath(m_historyDir);
    
    loadHistory();
}

inline ScreenshotHistory::~ScreenshotHistory()
{
    saveHistory();
}

inline void ScreenshotHistory::addItem(const QString &filePath, const QPixmap &thumbnail,
                                        const QString &ocrText)
{
    QMutexLocker locker(&m_mutex);
    
    HistoryItem item;
    item.id = generateId();
    item.filePath = filePath;
    item.timestamp = QDateTime::currentDateTime();
    item.hasOcr = !ocrText.isEmpty();
    item.ocrText = ocrText;
    
    QFileInfo info(filePath);
    item.fileSize = info.size();
    
    QImage img(filePath);
    if (!img.isNull()) {
        item.originalSize = img.size();
    }
    
    // 保存缩略图
    item.thumbnailPath = m_historyDir + "/" + item.id + "_thumb.png";
    thumbnail.save(item.thumbnailPath, "PNG");
    
    m_history.prepend(item);
    cleanupOldItems();
    
    locker.unlock();
    
    saveHistory();
    emit itemAdded(item);
    emit historyChanged();
}

inline QVector<ScreenshotHistory::HistoryItem> ScreenshotHistory::getHistory(int limit) const
{
    QMutexLocker locker(&m_mutex);
    
    if (limit <= 0 || limit >= m_history.size()) {
        return m_history;
    }
    
    return m_history.mid(0, limit);
}

inline ScreenshotHistory::HistoryItem ScreenshotHistory::getItem(const QString &id) const
{
    QMutexLocker locker(&m_mutex);
    
    for (const HistoryItem &item : m_history) {
        if (item.id == id) {
            return item;
        }
    }
    
    return HistoryItem();
}

inline QPixmap ScreenshotHistory::getThumbnail(const QString &id) const
{
    HistoryItem item = getItem(id);
    if (!item.thumbnailPath.isEmpty()) {
        return QPixmap(item.thumbnailPath);
    }
    return QPixmap();
}

inline void ScreenshotHistory::removeItem(const QString &id)
{
    QMutexLocker locker(&m_mutex);
    
    for (int i = 0; i < m_history.size(); ++i) {
        if (m_history[i].id == id) {
            // 删除缩略图文件
            QFile::remove(m_history[i].thumbnailPath);
            m_history.removeAt(i);
            break;
        }
    }
    
    locker.unlock();
    
    saveHistory();
    emit itemRemoved(id);
    emit historyChanged();
}

inline void ScreenshotHistory::clearHistory()
{
    QMutexLocker locker(&m_mutex);
    
    for (const HistoryItem &item : m_history) {
        QFile::remove(item.thumbnailPath);
    }
    
    m_history.clear();
    
    locker.unlock();
    
    saveHistory();
    emit historyChanged();
}

inline QVector<ScreenshotHistory::HistoryItem> ScreenshotHistory::search(const QString &query) const
{
    QMutexLocker locker(&m_mutex);
    
    QVector<HistoryItem> results;
    QString lowerQuery = query.toLower();
    
    for (const HistoryItem &item : m_history) {
        if (item.filePath.toLower().contains(lowerQuery) ||
            item.ocrText.toLower().contains(lowerQuery)) {
            results.append(item);
        }
    }
    
    return results;
}

inline void ScreenshotHistory::setMaxItems(int count)
{
    m_maxItems = count;
    cleanupOldItems();
}

inline void ScreenshotHistory::setHistoryDir(const QString &dir)
{
    m_historyDir = dir;
    QDir().mkpath(m_historyDir);
}

inline bool ScreenshotHistory::exportItem(const QString &id, const QString &outputPath) const
{
    HistoryItem item = getItem(id);
    if (item.filePath.isEmpty()) return false;
    
    return QFile::copy(item.filePath, outputPath);
}

inline void ScreenshotHistory::loadHistory()
{
    QString historyFile = m_historyDir + "/history.json";
    QFile file(historyFile);
    
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonArray array = doc.array();
    
    for (const QJsonValue &val : array) {
        QJsonObject obj = val.toObject();
        
        HistoryItem item;
        item.id = obj["id"].toString();
        item.filePath = obj["filePath"].toString();
        item.thumbnailPath = obj["thumbnailPath"].toString();
        item.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
        item.originalSize = QSize(obj["width"].toInt(), obj["height"].toInt());
        item.fileSize = obj["fileSize"].toVariant().toLongLong();
        item.ocrText = obj["ocrText"].toString();
        item.hasOcr = obj["hasOcr"].toBool();
        
        m_history.append(item);
    }
}

inline void ScreenshotHistory::saveHistory()
{
    QString historyFile = m_historyDir + "/history.json";
    QFile file(historyFile);
    
    if (!file.open(QIODevice::WriteOnly)) return;
    
    QJsonArray array;
    
    for (const HistoryItem &item : m_history) {
        QJsonObject obj;
        obj["id"] = item.id;
        obj["filePath"] = item.filePath;
        obj["thumbnailPath"] = item.thumbnailPath;
        obj["timestamp"] = item.timestamp.toString(Qt::ISODate);
        obj["width"] = item.originalSize.width();
        obj["height"] = item.originalSize.height();
        obj["fileSize"] = item.fileSize;
        obj["ocrText"] = item.ocrText;
        obj["hasOcr"] = item.hasOcr;
        array.append(obj);
    }
    
    file.write(QJsonDocument(array).toJson());
}

inline void ScreenshotHistory::cleanupOldItems()
{
    while (m_history.size() > m_maxItems) {
        HistoryItem item = m_history.takeLast();
        QFile::remove(item.thumbnailPath);
    }
}

inline QString ScreenshotHistory::generateId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}


// ==================== ClipboardMonitor 实现 ====================

inline ClipboardMonitor& ClipboardMonitor::instance()
{
    static ClipboardMonitor instance;
    return instance;
}

inline ClipboardMonitor::ClipboardMonitor(QObject *parent)
    : QObject(parent)
    , m_monitoring(false)
{
}

inline void ClipboardMonitor::startMonitoring()
{
    if (m_monitoring) return;
    
    m_monitoring = true;
    connect(QApplication::clipboard(), &QClipboard::dataChanged,
            this, &ClipboardMonitor::onClipboardChanged);
}

inline void ClipboardMonitor::stopMonitoring()
{
    if (!m_monitoring) return;
    
    m_monitoring = false;
    disconnect(QApplication::clipboard(), &QClipboard::dataChanged,
               this, &ClipboardMonitor::onClipboardChanged);
}

inline QImage ClipboardMonitor::currentImage() const
{
    return QApplication::clipboard()->image();
}

inline bool ClipboardMonitor::hasImage() const
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    return mimeData && mimeData->hasImage();
}

inline void ClipboardMonitor::onClipboardChanged()
{
    if (hasImage()) {
        QImage image = currentImage();
        if (!image.isNull() && image != m_lastImage) {
            m_lastImage = image;
            emit imageAvailable(image);
        }
    }
}

#endif // THUMBNAILCACHE_H

