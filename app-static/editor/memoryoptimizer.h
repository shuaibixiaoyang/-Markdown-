// 文件说明：app-static\editor\memoryoptimizer.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef MEMORYOPTIMIZER_H
#define MEMORYOPTIMIZER_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QList>
#include <QPixmap>
#include <QImage>
#include <QTimer>
#include <QMutex>
#include <QDateTime>
#include <QCache>
#include <QPointer>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <functional>

class QWebEngineView;

/**
 * @brief LRU 缓存模板类
 *
 * 基于最近最少使用策略的缓存实现
 * 支持：
 * - 最大项目数限制
 * - 最大内存限制
 * - 自动淘汰
 * - 访问统计
 */
template<typename Key, typename Value>
class LRUCache
{
public:
    struct CacheItem {
        Value value;
        qint64 size;            // 项目大小（字节）
        QDateTime lastAccess;   // 最后访问时间
        int accessCount;        // 访问次数

        CacheItem() : size(0), accessCount(0) {}
        CacheItem(const Value &v, qint64 s = 0)
            : value(v), size(s), lastAccess(QDateTime::currentDateTime()), accessCount(1) {}
    };

    explicit LRUCache(int maxItems = 100, qint64 maxMemory = 50 * 1024 * 1024)
        : m_maxItems(maxItems)
        , m_maxMemory(maxMemory)
        , m_currentMemory(0)
        , m_hitCount(0)
        , m_missCount(0)
    {}

    void setMaxItems(int max) { m_maxItems = max; evictIfNeeded(); }
    void setMaxMemory(qint64 max) { m_maxMemory = max; evictIfNeeded(); }

    int maxItems() const { return m_maxItems; }
    qint64 maxMemory() const { return m_maxMemory; }
    int count() const { return m_cache.count(); }
    qint64 currentMemory() const { return m_currentMemory; }

    bool contains(const Key &key) const {
        return m_cache.contains(key);
    }

    Value value(const Key &key, const Value &defaultValue = Value()) {
        if (m_cache.contains(key)) {
            m_hitCount++;
            CacheItem &item = m_cache[key];
            item.lastAccess = QDateTime::currentDateTime();
            item.accessCount++;
            // 移动到队列末尾（最近使用）
            m_accessOrder.removeAll(key);
            m_accessOrder.append(key);
            return item.value;
        }
        m_missCount++;
        return defaultValue;
    }

    void insert(const Key &key, const Value &value, qint64 size = 0) {
        // 如果已存在，先移除
        if (m_cache.contains(key)) {
            m_currentMemory -= m_cache[key].size;
            m_accessOrder.removeAll(key);
        }

        // 插入新项
        m_cache[key] = CacheItem(value, size);
        m_currentMemory += size;
        m_accessOrder.append(key);

        // 淘汰超出限制的项
        evictIfNeeded();
    }

    bool remove(const Key &key) {
        if (m_cache.contains(key)) {
            m_currentMemory -= m_cache[key].size;
            m_cache.remove(key);
            m_accessOrder.removeAll(key);
            return true;
        }
        return false;
    }

    void clear() {
        m_cache.clear();
        m_accessOrder.clear();
        m_currentMemory = 0;
    }

    QList<Key> keys() const {
        return m_cache.keys();
    }

    // 统计信息
    double hitRate() const {
        qint64 total = m_hitCount + m_missCount;
        return total > 0 ? (double)m_hitCount / total : 0.0;
    }

    qint64 hitCount() const { return m_hitCount; }
    qint64 missCount() const { return m_missCount; }
    void resetStats() { m_hitCount = m_missCount = 0; }

private:
    void evictIfNeeded() {
        // 按数量淘汰
        while (m_cache.count() > m_maxItems && !m_accessOrder.isEmpty()) {
            Key oldest = m_accessOrder.takeFirst();
            m_currentMemory -= m_cache[oldest].size;
            m_cache.remove(oldest);
        }

        // 按内存淘汰
        while (m_currentMemory > m_maxMemory && !m_accessOrder.isEmpty()) {
            Key oldest = m_accessOrder.takeFirst();
            m_currentMemory -= m_cache[oldest].size;
            m_cache.remove(oldest);
        }
    }

    QHash<Key, CacheItem> m_cache;
    QList<Key> m_accessOrder;
    int m_maxItems;
    qint64 m_maxMemory;
    qint64 m_currentMemory;
    qint64 m_hitCount;
    qint64 m_missCount;
};

/**
 * @brief 图片缓存管理器
 *
 * 功能：
 * - 图片延迟加载（滚动到可视区域才加载）
 * - 图片缓存与淘汰
 * - 占位符显示
 * - 预加载优化
 */
class ImageCacheManager : public QObject
{
    Q_OBJECT

public:
    struct ImageInfo {
        QString path;           // 图片路径
        QSize originalSize;     // 原始尺寸
        QSize displaySize;      // 显示尺寸
        bool isLoaded;          // 是否已加载
        bool isVisible;         // 是否在可视区域
        QRect viewportRect;     // 在视口中的位置

        ImageInfo() : isLoaded(false), isVisible(false) {}
    };

    struct Config {
        int maxCachedImages;        // 最大缓存图片数
        qint64 maxCacheMemory;      // 最大缓存内存（字节）
        int preloadDistance;        // 预加载距离（像素）
        int thumbnailSize;          // 缩略图尺寸
        bool enableLazyLoad;        // 启用延迟加载
        int loadDelay;              // 加载延迟（毫秒）

        Config()
            : maxCachedImages(50)
            , maxCacheMemory(100 * 1024 * 1024)  // 100MB
            , preloadDistance(500)
            , thumbnailSize(200)
            , enableLazyLoad(true)
            , loadDelay(100)
        {}
    };

    explicit ImageCacheManager(QObject *parent = nullptr);
    ~ImageCacheManager();

    void setConfig(const Config &config);
    Config config() const { return m_config; }

    // 注册图片（在解析 Markdown 时调用）
    void registerImage(const QString &id, const QString &path, const QSize &displaySize = QSize());
    void unregisterImage(const QString &id);
    void clearImages();

    // 获取图片（如果未加载则返回占位符）
    QPixmap getImage(const QString &id, const QSize &size = QSize());
    QPixmap getPlaceholder(const QSize &size);

    // 可视区域更新
    void setViewport(QWidget *viewport);
    void updateVisibleArea(const QRect &visibleRect);
    void onScroll(int scrollValue);

    // 强制加载/卸载
    void loadImage(const QString &id);
    void unloadImage(const QString &id);
    void preloadImages(const QStringList &ids);

    // 统计
    qint64 memoryUsage() const;
    int cachedCount() const;
    int totalCount() const;

signals:
    void imageLoaded(const QString &id);
    void imageUnloaded(const QString &id);
    void cacheCleared();
    void memoryWarning(qint64 currentUsage, qint64 maxUsage);

private slots:
    void processLoadQueue();
    void onViewportScrolled();

private:
    QPixmap loadImageFromPath(const QString &path, const QSize &size);
    qint64 calculateImageMemory(const QPixmap &pixmap);
    void evictLeastUsed();
    void updateVisibility();

    Config m_config;
    QHash<QString, ImageInfo> m_imageInfos;
    LRUCache<QString, QPixmap> m_cache;
    QStringList m_loadQueue;
    QTimer *m_loadTimer;
    QPointer<QWidget> m_viewport;
    QRect m_currentVisibleRect;
    QPixmap m_placeholder;
    mutable QMutex m_mutex;
};

/**
 * @brief 文档内存管理器
 *
 * 功能：
 * - 不活跃标签页内存释放
 * - 文档状态序列化/反序列化
 * - 智能预加载
 * - 内存使用监控
 */
class DocumentMemoryManager : public QObject
{
    Q_OBJECT

public:
    // 文档状态（用于序列化）
    struct DocumentState {
        QString filePath;           // 文件路径
        QString content;            // 文档内容（可能被卸载）
        int cursorPosition;         // 光标位置
        int scrollPosition;         // 滚动位置
        QDateTime lastAccess;       // 最后访问时间
        bool isModified;            // 是否已修改
        bool isUnloaded;            // 是否已卸载
        qint64 contentSize;         // 内容大小

        DocumentState()
            : cursorPosition(0)
            , scrollPosition(0)
            , isModified(false)
            , isUnloaded(false)
            , contentSize(0)
        {}
    };

    struct Config {
        int maxLoadedDocuments;     // 最大同时加载文档数
        qint64 maxMemoryUsage;      // 最大内存使用（字节）
        int unloadTimeout;          // 不活跃多久后卸载（秒）
        bool enableAutoUnload;      // 启用自动卸载
        QString cacheDirectory;     // 缓存目录

        Config()
            : maxLoadedDocuments(5)
            , maxMemoryUsage(200 * 1024 * 1024)  // 200MB
            , unloadTimeout(300)  // 5分钟
            , enableAutoUnload(true)
        {}
    };

    explicit DocumentMemoryManager(QObject *parent = nullptr);
    ~DocumentMemoryManager();

    void setConfig(const Config &config);
    Config config() const { return m_config; }

    // 文档管理
    void registerDocument(const QString &id, QPlainTextEdit *editor);
    void unregisterDocument(const QString &id);
    void setActiveDocument(const QString &id);
    QString activeDocumentId() const { return m_activeDocumentId; }

    // 状态管理
    DocumentState documentState(const QString &id) const;
    bool isDocumentLoaded(const QString &id) const;
    bool isDocumentModified(const QString &id) const;

    // 手动控制
    void unloadDocument(const QString &id);
    void reloadDocument(const QString &id);
    void unloadInactiveDocuments();

    // 统计
    qint64 totalMemoryUsage() const;
    int loadedDocumentCount() const;
    QStringList loadedDocumentIds() const;

signals:
    void documentUnloaded(const QString &id);
    void documentReloaded(const QString &id);
    void memoryUsageChanged(qint64 usage);
    void memoryWarning(qint64 currentUsage, qint64 maxUsage);

private slots:
    void checkInactiveDocuments();
    void onDocumentModified();

private:
    void saveDocumentState(const QString &id);
    bool restoreDocumentState(const QString &id);
    QString cacheFilePath(const QString &id) const;

    Config m_config;
    QHash<QString, DocumentState> m_documentStates;
    QHash<QString, QPointer<QPlainTextEdit>> m_editors;
    QString m_activeDocumentId;
    QTimer *m_checkTimer;
    mutable QMutex m_mutex;
};

/**
 * @brief 内存优化器（主协调器）
 *
 * 功能：
 * - 统一管理所有内存优化组件
 * - 全局内存监控
 * - 自动优化策略
 * - 内存使用报告
 */
class MemoryOptimizer : public QObject
{
    Q_OBJECT

public:
    struct Config {
        qint64 warningThreshold;    // 警告阈值（字节）
        qint64 criticalThreshold;   // 临界阈值（字节）
        int monitorInterval;        // 监控间隔（毫秒）
        bool enableAutoOptimize;    // 启用自动优化

        Config()
            : warningThreshold(500 * 1024 * 1024)   // 500MB
            , criticalThreshold(800 * 1024 * 1024)  // 800MB
            , monitorInterval(5000)
            , enableAutoOptimize(true)
        {}
    };

    struct MemoryReport {
        qint64 totalUsage;
        qint64 imageCacheUsage;
        qint64 documentUsage;
        qint64 otherUsage;
        int cachedImages;
        int loadedDocuments;
        double imageCacheHitRate;
        QDateTime timestamp;
    };

    explicit MemoryOptimizer(QObject *parent = nullptr);
    ~MemoryOptimizer();

    void setConfig(const Config &config);
    Config config() const { return m_config; }

    // 组件访问
    ImageCacheManager* imageCacheManager() const { return m_imageCacheManager; }
    DocumentMemoryManager* documentMemoryManager() const { return m_documentMemoryManager; }

    // 内存操作
    void optimizeNow();
    void clearAllCaches();
    MemoryReport generateReport() const;

    // 监控
    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;

    // 当前状态
    qint64 currentMemoryUsage() const;
    bool isMemoryWarning() const;
    bool isMemoryCritical() const;

signals:
    void memoryWarning(const MemoryReport &report);
    void memoryCritical(const MemoryReport &report);
    void optimizationCompleted(qint64 freedMemory);
    void reportGenerated(const MemoryReport &report);

private slots:
    void monitorMemory();
    void onImageCacheWarning(qint64 current, qint64 max);
    void onDocumentMemoryWarning(qint64 current, qint64 max);

private:
    qint64 estimateProcessMemory() const;

    Config m_config;
    ImageCacheManager *m_imageCacheManager;
    DocumentMemoryManager *m_documentMemoryManager;
    QTimer *m_monitorTimer;
    MemoryReport m_lastReport;
};

#endif // MEMORYOPTIMIZER_H

