// 文件说明：app-static\security\securememory.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef SECUREMEMORY_H
#define SECUREMEMORY_H

#include <QByteArray>
#include <QString>
#include <cstring>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <sys/mman.h>
#endif

/**
 * @brief 安全字节数组 - 自动清零的内存管理
 * 
 * 特性：
 * - 析构时自动安全清零
 * - 防止编译器优化掉清零操作
 * - 可选内存锁定（防止交换到磁盘）
 */
class SecureByteArray
{
public:
    SecureByteArray() = default;
    
    explicit SecureByteArray(int size) : m_data(size, 0) {
        lockMemory();
    }
    
    SecureByteArray(const char *data, int size) : m_data(data, size) {
        lockMemory();
    }
    
    explicit SecureByteArray(const QByteArray &other) : m_data(other) {
        lockMemory();
    }
    
    SecureByteArray(const SecureByteArray &other) : m_data(other.m_data) {
        lockMemory();
    }
    
    SecureByteArray(SecureByteArray &&other) noexcept : m_data(std::move(other.m_data)) {
        lockMemory();
    }
    
    ~SecureByteArray() {
        secureWipe();
        unlockMemory();
    }
    
    SecureByteArray& operator=(const SecureByteArray &other) {
        if (this != &other) {
            secureWipe();
            m_data = other.m_data;
        }
        return *this;
    }
    
    SecureByteArray& operator=(SecureByteArray &&other) noexcept {
        if (this != &other) {
            secureWipe();
            m_data = std::move(other.m_data);
        }
        return *this;
    }
    
    // 数据访问
    char* data() { return m_data.data(); }
    const char* data() const { return m_data.constData(); }
    const char* constData() const { return m_data.constData(); }
    int size() const { return m_data.size(); }
    bool isEmpty() const { return m_data.isEmpty(); }
    
    void resize(int size) {
        m_data.resize(size);
    }
    
    void clear() {
        secureWipe();
        m_data.clear();
    }
    
    // 转换
    QByteArray toByteArray() const { return m_data; }
    
    // 比较（恒定时间）
    bool equals(const SecureByteArray &other) const {
        return constantTimeCompare(m_data, other.m_data);
    }
    
    // 静态工具方法
    static bool constantTimeCompare(const QByteArray &a, const QByteArray &b) {
        if (a.size() != b.size()) {
            // 仍然进行完整比较以保持恒定时间
            volatile int dummy = 0;
            for (int i = 0; i < qMax(a.size(), b.size()); ++i) {
                dummy ^= 1;
            }
            Q_UNUSED(dummy)
            return false;
        }
        
        volatile unsigned char result = 0;
        for (int i = 0; i < a.size(); ++i) {
            result |= static_cast<unsigned char>(a[i]) ^ static_cast<unsigned char>(b[i]);
        }
        return result == 0;
    }
    
private:
    void secureWipe() {
        if (!m_data.isEmpty()) {
            // 使用 volatile 防止编译器优化
            volatile char *p = m_data.data();
            size_t size = static_cast<size_t>(m_data.size());
            
            // 多模式覆写
            memset(const_cast<char*>(p), 0x00, size);
            memset(const_cast<char*>(p), 0xFF, size);
            memset(const_cast<char*>(p), 0x00, size);
            
            // 显式内存屏障
#ifdef Q_CC_MSVC
            _ReadWriteBarrier();
#else
            __asm__ __volatile__("" ::: "memory");
#endif
        }
    }
    
    void lockMemory() {
#ifdef Q_OS_WIN
        if (!m_data.isEmpty()) {
            VirtualLock(m_data.data(), m_data.size());
        }
#else
        if (!m_data.isEmpty()) {
            mlock(m_data.data(), m_data.size());
        }
#endif
    }
    
    void unlockMemory() {
#ifdef Q_OS_WIN
        if (!m_data.isEmpty()) {
            VirtualUnlock(m_data.data(), m_data.size());
        }
#else
        if (!m_data.isEmpty()) {
            munlock(m_data.data(), m_data.size());
        }
#endif
    }
    
    QByteArray m_data;
};


/**
 * @brief 安全字符串 - 自动清零的 QString 替代
 */
class SecureString
{
public:
    SecureString() = default;
    
    explicit SecureString(const QString &str) {
        m_data = str.toUtf8();
        lockMemory();
    }
    
    SecureString(const SecureString &other) : m_data(other.m_data) {
        lockMemory();
    }
    
    SecureString(SecureString &&other) noexcept : m_data(std::move(other.m_data)) {
        lockMemory();
    }
    
    ~SecureString() {
        secureWipe();
        unlockMemory();
    }
    
    SecureString& operator=(const SecureString &other) {
        if (this != &other) {
            secureWipe();
            m_data = other.m_data;
        }
        return *this;
    }
    
    SecureString& operator=(const QString &str) {
        secureWipe();
        m_data = str.toUtf8();
        lockMemory();
        return *this;
    }
    
    QString toString() const {
        return QString::fromUtf8(m_data);
    }
    
    QByteArray toUtf8() const {
        return m_data;
    }
    
    bool isEmpty() const {
        return m_data.isEmpty();
    }
    
    int length() const {
        return toString().length();
    }
    
    void clear() {
        secureWipe();
        m_data.clear();
    }
    
    bool equals(const SecureString &other) const {
        return SecureByteArray::constantTimeCompare(m_data, other.m_data);
    }
    
    bool equals(const QString &other) const {
        return SecureByteArray::constantTimeCompare(m_data, other.toUtf8());
    }
    
private:
    void secureWipe() {
        if (!m_data.isEmpty()) {
            volatile char *p = m_data.data();
            size_t size = static_cast<size_t>(m_data.size());
            memset(const_cast<char*>(p), 0x00, size);
            memset(const_cast<char*>(p), 0xFF, size);
            memset(const_cast<char*>(p), 0x00, size);
#ifdef Q_CC_MSVC
            _ReadWriteBarrier();
#else
            __asm__ __volatile__("" ::: "memory");
#endif
        }
    }
    
    void lockMemory() {
#ifdef Q_OS_WIN
        if (!m_data.isEmpty()) {
            VirtualLock(m_data.data(), m_data.size());
        }
#else
        if (!m_data.isEmpty()) {
            mlock(m_data.data(), m_data.size());
        }
#endif
    }
    
    void unlockMemory() {
#ifdef Q_OS_WIN
        if (!m_data.isEmpty()) {
            VirtualUnlock(m_data.data(), m_data.size());
        }
#else
        if (!m_data.isEmpty()) {
            munlock(m_data.data(), m_data.size());
        }
#endif
    }
    
    QByteArray m_data;
};

#endif // SECUREMEMORY_H

