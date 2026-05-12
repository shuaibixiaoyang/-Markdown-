// 文件说明：app-static\security\keycache.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef KEYCACHE_H
#define KEYCACHE_H

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QDateTime>
#include <QMutex>
#include <QCryptographicHash>

#include "securememory.h"
#include <QRandomGenerator>

/**
 * @brief 密钥派生缓存
 * 
 * PBKDF2 密钥派生是计算密集型操作，对于同一密码多次使用
 * 可以缓存派生密钥以提升性能。
 * 
 * 安全特性：
 * - 缓存条目有TTL（默认5分钟）
 * - 密钥存储在安全内存中
 * - 定期清理过期条目
 * - 可手动清除所有缓存
 */
class KeyCache : public QObject
{
    Q_OBJECT

public:
    // 缓存条目
    struct CacheEntry {
        SecureByteArray key;
        QByteArray salt;
        int iterations;
        QDateTime createdAt;
        QDateTime lastUsed;
        int useCount;
    };

    explicit KeyCache(QObject *parent = nullptr)
        : QObject(parent)
        , m_defaultTtlSeconds(300)  // 5分钟
        , m_maxEntries(10)
        , m_cleanupTimer(new QTimer(this))
    {
        connect(m_cleanupTimer, &QTimer::timeout, this, &KeyCache::cleanup);
        m_cleanupTimer->start(60000);  // 每分钟清理
    }

    ~KeyCache() {
        clearAll();
    }

    // 单例
    static KeyCache& instance() {
        static KeyCache instance;
        return instance;
    }

    // 获取或派生密钥
    SecureByteArray getOrDerive(
        const QString &password,
        const QByteArray &salt,
        int iterations,
        std::function<QByteArray(const QString&, const QByteArray&, int)> deriveFunc)
    {
        QMutexLocker locker(&m_mutex);

        QString cacheKey = generateCacheKey(password, salt, iterations);

        // 检查缓存
        if (m_cache.contains(cacheKey)) {
            CacheEntry &entry = m_cache[cacheKey];
            
            // 检查是否过期
            if (entry.createdAt.secsTo(QDateTime::currentDateTime()) < m_defaultTtlSeconds) {
                entry.lastUsed = QDateTime::currentDateTime();
                entry.useCount++;
                return entry.key;
            } else {
                // 过期，移除
                m_cache.remove(cacheKey);
            }
        }

        // 派生新密钥
        QByteArray derivedKey = deriveFunc(password, salt, iterations);

        // 缓存
        if (m_cache.size() >= m_maxEntries) {
            evictLeastUsed();
        }

        CacheEntry entry;
        entry.key = SecureByteArray(derivedKey);
        entry.salt = salt;
        entry.iterations = iterations;
        entry.createdAt = QDateTime::currentDateTime();
        entry.lastUsed = entry.createdAt;
        entry.useCount = 1;

        m_cache[cacheKey] = entry;

        // 清除原始密钥
        derivedKey.fill(0);

        return entry.key;
    }

    // 使缓存失效
    void invalidate(const QString &password) {
        QMutexLocker locker(&m_mutex);

        QStringList keysToRemove;
        for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
            // 检查是否匹配（通过哈希前缀）
            if (it.key().startsWith(hashPassword(password))) {
                keysToRemove << it.key();
            }
        }

        for (const QString &key : keysToRemove) {
            m_cache.remove(key);
        }
    }

    // 清除所有缓存
    void clearAll() {
        QMutexLocker locker(&m_mutex);
        m_cache.clear();
    }

    // 设置TTL
    void setTtl(int seconds) {
        m_defaultTtlSeconds = seconds;
    }

    // 设置最大条目数
    void setMaxEntries(int max) {
        m_maxEntries = max;
    }

    // 获取缓存统计
    struct Stats {
        int entryCount;
        int totalUses;
        qint64 memoryUsage;
    };

    Stats getStats() const {
        QMutexLocker locker(&m_mutex);

        Stats stats;
        stats.entryCount = m_cache.size();
        stats.totalUses = 0;
        stats.memoryUsage = 0;

        for (const CacheEntry &entry : m_cache) {
            stats.totalUses += entry.useCount;
            stats.memoryUsage += entry.key.size() + entry.salt.size();
        }

        return stats;
    }

public slots:
    void cleanup() {
        QMutexLocker locker(&m_mutex);

        QDateTime now = QDateTime::currentDateTime();
        QStringList expiredKeys;

        for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
            if (it->createdAt.secsTo(now) >= m_defaultTtlSeconds) {
                expiredKeys << it.key();
            }
        }

        for (const QString &key : expiredKeys) {
            m_cache.remove(key);
        }
    }

private:
    QString generateCacheKey(const QString &password, const QByteArray &salt, int iterations) const {
        // 使用密码哈希 + 盐 + 迭代次数生成缓存键
        // 注意：不存储原始密码
        QString data = hashPassword(password) + 
                       salt.toHex() + 
                       QString::number(iterations);
        return QCryptographicHash::hash(data.toUtf8(), QCryptographicHash::Sha256).toHex();
    }

    QString hashPassword(const QString &password) const {
        return QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex().left(16);
    }

    void evictLeastUsed() {
        if (m_cache.isEmpty()) return;

        QString leastUsedKey;
        int minUseCount = INT_MAX;

        for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
            if (it->useCount < minUseCount) {
                minUseCount = it->useCount;
                leastUsedKey = it.key();
            }
        }

        if (!leastUsedKey.isEmpty()) {
            m_cache.remove(leastUsedKey);
        }
    }

    mutable QMutex m_mutex;
    QMap<QString, CacheEntry> m_cache;
    int m_defaultTtlSeconds;
    int m_maxEntries;
    QTimer *m_cleanupTimer;
};


/**
 * @brief 会话密钥管理器
 * 
 * 管理当前会话的临时密钥，用于：
 * - 临时文件加密
 * - 内存数据保护
 * - 会话令牌
 */
class SessionKeyManager : public QObject
{
    Q_OBJECT

public:
    explicit SessionKeyManager(QObject *parent = nullptr)
        : QObject(parent)
        , m_sessionStarted(QDateTime::currentDateTime())
    {
        // 生成会话密钥
        regenerateSessionKey();

        // 定期轮换密钥
        m_rotationTimer = new QTimer(this);
        connect(m_rotationTimer, &QTimer::timeout, this, &SessionKeyManager::regenerateSessionKey);
        m_rotationTimer->start(3600000);  // 每小时轮换
    }

    static SessionKeyManager& instance() {
        static SessionKeyManager instance;
        return instance;
    }

    // 获取当前会话密钥
    SecureByteArray sessionKey() const {
        QMutexLocker locker(&m_mutex);
        return m_sessionKey;
    }

    // 获取会话ID
    QString sessionId() const {
        return m_sessionId;
    }

    // 会话是否有效
    bool isSessionValid() const {
        // 会话最长24小时
        return m_sessionStarted.secsTo(QDateTime::currentDateTime()) < 86400;
    }

    // 派生特定用途的密钥
    SecureByteArray deriveKey(const QString &purpose) const {
        QMutexLocker locker(&m_mutex);

        QByteArray data = m_sessionKey.toByteArray() + purpose.toUtf8();
        QByteArray derived = QCryptographicHash::hash(data, QCryptographicHash::Sha256);

        // 清除中间数据
        data.fill(0);

        return SecureByteArray(derived);
    }

signals:
    void sessionKeyRotated();
    void sessionExpired();

public slots:
    void regenerateSessionKey() {
        QMutexLocker locker(&m_mutex);

        // 生成新的会话密钥
        QByteArray randomBytes(32, 0);
        QRandomGenerator::global()->fillRange(reinterpret_cast<quint32*>(randomBytes.data()), 8);

        m_sessionKey = SecureByteArray(
            QCryptographicHash::hash(randomBytes, QCryptographicHash::Sha256));
        randomBytes.fill(0);

        // 生成会话ID
        m_sessionId = QCryptographicHash::hash(
            QDateTime::currentDateTime().toString(Qt::ISODate).toUtf8() +
            QByteArray::number(QRandomGenerator::global()->generate()),
            QCryptographicHash::Sha256).toHex().left(16);

        emit sessionKeyRotated();
    }

    void endSession() {
        QMutexLocker locker(&m_mutex);
        m_sessionKey.clear();
        m_sessionId.clear();
        m_rotationTimer->stop();
        emit sessionExpired();
    }

private:
    mutable QMutex m_mutex;
    SecureByteArray m_sessionKey;
    QString m_sessionId;
    QDateTime m_sessionStarted;
    QTimer *m_rotationTimer;
};

#endif // KEYCACHE_H

