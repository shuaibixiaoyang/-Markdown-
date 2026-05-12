// 文件说明：app-static\editor\memoryoptimizer.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "memoryoptimizer.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QScrollBar>
#include <QAbstractScrollArea>
#include <QCryptographicHash>
#include <QDataStream>
#include <QImageReader>
#include <QApplication>
#include <QPainter>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#elif defined(Q_OS_MAC)
#include <mach/mach.h>
#elif defined(Q_OS_LINUX)
#include <sys/resource.h>
#include <unistd.h>
#endif

// ============================================================================
// ImageCacheManager 实现
// ============================================================================

ImageCacheManager::ImageCacheManager(QObject *parent)
    : QObject(parent)
    , m_cache(50, 100 * 1024 * 1024)
{
    m_loadTimer = new QTimer(this);
    m_loadTimer->setSingleShot(true);
    connect(m_loadTimer, &QTimer::timeout, this, &ImageCacheManager::processLoadQueue);

    // 创建默认占位符
    m_placeholder = QPixmap(200, 150);
    m_placeholder.fill(QColor(240, 240, 240));
    QPainter painter(&m_placeholder);
    painter.setPen(QColor(180, 180, 180));
    painter.drawRect(0, 0, 199, 149);
    painter.drawText(m_placeholder.rect(), Qt::AlignCenter, QObject::tr("加载中..."));
}

// 函数说明：销毁 ImageCacheManager 对象，释放本模块持有的资源。
ImageCacheManager::~ImageCacheManager()
{
}

// 函数说明：设置 ImageCacheManager 的运行参数，并触发必要的界面或数据刷新。
void ImageCacheManager::setConfig(const Config &config)
{
    m_config = config;
    m_cache.setMaxItems(config.maxCachedImages);
    m_cache.setMaxMemory(config.maxCacheMemory);
}

// 函数说明：实现 ImageCacheManager::registerImage 的核心逻辑，供当前模块调用。
void ImageCacheManager::registerImage(const QString &id, const QString &path, const QSize &displaySize)
{
    QMutexLocker locker(&m_mutex);

    ImageInfo info;
    info.path = path;
    info.displaySize = displaySize;
    info.isLoaded = false;
    info.isVisible = false;

    // 获取原始尺寸（不加载完整图片）
    QImageReader reader(path);
    if (reader.canRead()) {
        info.originalSize = reader.size();
    }

    m_imageInfos[id] = info;
}

// 函数说明：实现 ImageCacheManager::unregisterImage 的核心逻辑，供当前模块调用。
void ImageCacheManager::unregisterImage(const QString &id)
{
    QMutexLocker locker(&m_mutex);
    m_imageInfos.remove(id);
    m_cache.remove(id);
    m_loadQueue.removeAll(id);
}

// 函数说明：清空 ImageCacheManager 保存的临时状态或缓存数据。
void ImageCacheManager::clearImages()
{
    QMutexLocker locker(&m_mutex);
    m_imageInfos.clear();
    m_cache.clear();
    m_loadQueue.clear();
    emit cacheCleared();
}

// 函数说明：读取 ImageCacheManager 当前保存的状态或计算结果。
QPixmap ImageCacheManager::getImage(const QString &id, const QSize &size)
{
    QMutexLocker locker(&m_mutex);

    // 如果在缓存中，直接返回
    if (m_cache.contains(id)) {
        QPixmap pixmap = m_cache.value(id);
        if (!size.isEmpty() && pixmap.size() != size) {
            return pixmap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        return pixmap;
    }

    // 如果启用延迟加载，返回占位符并加入加载队列
    if (m_config.enableLazyLoad) {
        if (!m_loadQueue.contains(id)) {
            m_loadQueue.append(id);
            if (!m_loadTimer->isActive()) {
                m_loadTimer->start(m_config.loadDelay);
            }
        }
        return getPlaceholder(size.isEmpty() ? QSize(200, 150) : size);
    }

    // 立即加载
    locker.unlock();
    loadImage(id);
    locker.relock();

    return m_cache.value(id, getPlaceholder(size));
}

// 函数说明：读取 ImageCacheManager 当前保存的状态或计算结果。
QPixmap ImageCacheManager::getPlaceholder(const QSize &size)
{
    if (size == m_placeholder.size()) {
        return m_placeholder;
    }
    return m_placeholder.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

// 函数说明：设置 ImageCacheManager 的运行参数，并触发必要的界面或数据刷新。
void ImageCacheManager::setViewport(QWidget *viewport)
{
    if (m_viewport) {
        // 断开旧连接
        if (QAbstractScrollArea *scrollArea = qobject_cast<QAbstractScrollArea*>(m_viewport->parent())) {
            disconnect(scrollArea->verticalScrollBar(), &QScrollBar::valueChanged,
                       this, &ImageCacheManager::onViewportScrolled);
        }
    }

    m_viewport = viewport;

    if (m_viewport) {
        // 连接滚动事件
        if (QAbstractScrollArea *scrollArea = qobject_cast<QAbstractScrollArea*>(m_viewport->parent())) {
            connect(scrollArea->verticalScrollBar(), &QScrollBar::valueChanged,
                    this, &ImageCacheManager::onViewportScrolled);
        }
    }
}

// 函数说明：刷新 ImageCacheManager 的内部状态，并同步到相关界面。
void ImageCacheManager::updateVisibleArea(const QRect &visibleRect)
{
    m_currentVisibleRect = visibleRect;
    updateVisibility();
}

// 函数说明：响应 ImageCacheManager 收到的信号或异步回调，并更新界面状态。
void ImageCacheManager::onScroll(int scrollValue)
{
    Q_UNUSED(scrollValue)
    updateVisibility();
}

// 函数说明：加载 ImageCacheManager 需要的数据、配置或外部资源。
void ImageCacheManager::loadImage(const QString &id)
{
    QMutexLocker locker(&m_mutex);

    if (!m_imageInfos.contains(id)) return;

    ImageInfo &info = m_imageInfos[id];
    if (info.isLoaded) return;

    QSize loadSize = info.displaySize.isEmpty() ? QSize() : info.displaySize;
    QPixmap pixmap = loadImageFromPath(info.path, loadSize);

    if (!pixmap.isNull()) {
        qint64 memSize = calculateImageMemory(pixmap);
        m_cache.insert(id, pixmap, memSize);
        info.isLoaded = true;

        locker.unlock();
        emit imageLoaded(id);

        // 检查内存警告
        if (m_cache.currentMemory() > m_config.maxCacheMemory * 0.9) {
            emit memoryWarning(m_cache.currentMemory(), m_config.maxCacheMemory);
        }
    }
}

// 函数说明：实现 ImageCacheManager::unloadImage 的核心逻辑，供当前模块调用。
void ImageCacheManager::unloadImage(const QString &id)
{
    QMutexLocker locker(&m_mutex);

    if (m_cache.remove(id)) {
        if (m_imageInfos.contains(id)) {
            m_imageInfos[id].isLoaded = false;
        }
        locker.unlock();
        emit imageUnloaded(id);
    }
}

// 函数说明：实现 ImageCacheManager::preloadImages 的核心逻辑，供当前模块调用。
void ImageCacheManager::preloadImages(const QStringList &ids)
{
    QMutexLocker locker(&m_mutex);

    for (const QString &id : ids) {
        if (!m_cache.contains(id) && !m_loadQueue.contains(id)) {
            m_loadQueue.append(id);
        }
    }

    if (!m_loadQueue.isEmpty() && !m_loadTimer->isActive()) {
        m_loadTimer->start(m_config.loadDelay);
    }
}

// 函数说明：实现 ImageCacheManager::memoryUsage 的核心逻辑，供当前模块调用。
qint64 ImageCacheManager::memoryUsage() const
{
    return m_cache.currentMemory();
}

// 函数说明：实现 ImageCacheManager::cachedCount 的核心逻辑，供当前模块调用。
int ImageCacheManager::cachedCount() const
{
    return m_cache.count();
}

// 函数说明：实现 ImageCacheManager::totalCount 的核心逻辑，供当前模块调用。
int ImageCacheManager::totalCount() const
{
    return m_imageInfos.count();
}

// 函数说明：处理 ImageCacheManager 的核心业务数据，并输出处理结果。
void ImageCacheManager::processLoadQueue()
{
    if (m_loadQueue.isEmpty()) return;

    // 每次处理一批图片
    int batchSize = qMin(5, m_loadQueue.size());
    for (int i = 0; i < batchSize; ++i) {
        QString id = m_loadQueue.takeFirst();
        loadImage(id);
    }

    // 如果还有待加载的图片，继续定时加载
    if (!m_loadQueue.isEmpty()) {
        m_loadTimer->start(m_config.loadDelay);
    }
}

// 函数说明：响应 ImageCacheManager 收到的信号或异步回调，并更新界面状态。
void ImageCacheManager::onViewportScrolled()
{
    updateVisibility();
}

// 函数说明：加载 ImageCacheManager 需要的数据、配置或外部资源。
QPixmap ImageCacheManager::loadImageFromPath(const QString &path, const QSize &size)
{
    QImageReader reader(path);

    // 如果指定了尺寸，使用缩放读取以节省内存
    if (!size.isEmpty()) {
        QSize originalSize = reader.size();
        if (originalSize.isValid()) {
            QSize scaledSize = originalSize.scaled(size, Qt::KeepAspectRatio);
            reader.setScaledSize(scaledSize);
        }
    }

    QImage image = reader.read();
    if (image.isNull()) {
        qWarning() << "Failed to load image:" << path << reader.errorString();
        return QPixmap();
    }

    return QPixmap::fromImage(image);
}

// 函数说明：实现 ImageCacheManager::calculateImageMemory 的核心逻辑，供当前模块调用。
qint64 ImageCacheManager::calculateImageMemory(const QPixmap &pixmap)
{
    // 估算 QPixmap 的内存使用
    return pixmap.width() * pixmap.height() * pixmap.depth() / 8;
}

// 函数说明：实现 ImageCacheManager::evictLeastUsed 的核心逻辑，供当前模块调用。
void ImageCacheManager::evictLeastUsed()
{
    // LRUCache 自动处理淘汰
    // 这里可以添加额外的淘汰逻辑
}

// 函数说明：刷新 ImageCacheManager 的内部状态，并同步到相关界面。
void ImageCacheManager::updateVisibility()
{
    if (!m_viewport || m_currentVisibleRect.isEmpty()) return;

    QMutexLocker locker(&m_mutex);

    // 计算扩展的可视区域（用于预加载）
    QRect extendedRect = m_currentVisibleRect.adjusted(
        0, -m_config.preloadDistance,
        0, m_config.preloadDistance
    );

    QStringList toPreload;

    for (auto it = m_imageInfos.begin(); it != m_imageInfos.end(); ++it) {
        ImageInfo &info = it.value();
        bool wasVisible = info.isVisible;
        info.isVisible = extendedRect.intersects(info.viewportRect);

        if (info.isVisible && !info.isLoaded && !m_loadQueue.contains(it.key())) {
            toPreload.append(it.key());
        }
    }

    locker.unlock();

    if (!toPreload.isEmpty()) {
        preloadImages(toPreload);
    }
}

// ============================================================================
// DocumentMemoryManager 实现
// ============================================================================

DocumentMemoryManager::DocumentMemoryManager(QObject *parent)
    : QObject(parent)
{
    // 设置默认缓存目录
    m_config.cacheDirectory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                              + "/CuteMarkEd/documents";
    QDir().mkpath(m_config.cacheDirectory);

    m_checkTimer = new QTimer(this);
    connect(m_checkTimer, &QTimer::timeout, this, &DocumentMemoryManager::checkInactiveDocuments);
    m_checkTimer->start(30000);  // 每30秒检查一次
}

// 函数说明：销毁 DocumentMemoryManager 对象，释放本模块持有的资源。
DocumentMemoryManager::~DocumentMemoryManager()
{
    // 清理缓存文件
    // 注意：生产环境可能需要保留这些文件用于崩溃恢复
}

// 函数说明：设置 DocumentMemoryManager 的运行参数，并触发必要的界面或数据刷新。
void DocumentMemoryManager::setConfig(const Config &config)
{
    m_config = config;

    if (!m_config.cacheDirectory.isEmpty()) {
        QDir().mkpath(m_config.cacheDirectory);
    }
}

// 函数说明：实现 DocumentMemoryManager::registerDocument 的核心逻辑，供当前模块调用。
void DocumentMemoryManager::registerDocument(const QString &id, QPlainTextEdit *editor)
{
    QMutexLocker locker(&m_mutex);

    DocumentState state;
    state.filePath = id;
    state.lastAccess = QDateTime::currentDateTime();
    state.isModified = editor->document()->isModified();
    state.isUnloaded = false;
    state.contentSize = editor->toPlainText().size() * sizeof(QChar);

    m_documentStates[id] = state;
    m_editors[id] = editor;

    // 连接修改信号
    connect(editor->document(), &QTextDocument::modificationChanged,
            this, &DocumentMemoryManager::onDocumentModified);
}

// 函数说明：实现 DocumentMemoryManager::unregisterDocument 的核心逻辑，供当前模块调用。
void DocumentMemoryManager::unregisterDocument(const QString &id)
{
    QMutexLocker locker(&m_mutex);

    if (m_editors.contains(id) && m_editors[id]) {
        disconnect(m_editors[id]->document(), nullptr, this, nullptr);
    }

    m_documentStates.remove(id);
    m_editors.remove(id);

    // 删除缓存文件
    QFile::remove(cacheFilePath(id));

    if (m_activeDocumentId == id) {
        m_activeDocumentId.clear();
    }
}

// 函数说明：设置 DocumentMemoryManager 的运行参数，并触发必要的界面或数据刷新。
void DocumentMemoryManager::setActiveDocument(const QString &id)
{
    QMutexLocker locker(&m_mutex);

    if (m_activeDocumentId == id) return;

    // 更新旧活跃文档的状态
    if (!m_activeDocumentId.isEmpty() && m_documentStates.contains(m_activeDocumentId)) {
        // 记录光标和滚动位置
        if (m_editors.contains(m_activeDocumentId) && m_editors[m_activeDocumentId]) {
            QPlainTextEdit *editor = m_editors[m_activeDocumentId];
            DocumentState &state = m_documentStates[m_activeDocumentId];
            state.cursorPosition = editor->textCursor().position();
            state.scrollPosition = editor->verticalScrollBar()->value();
        }
    }

    m_activeDocumentId = id;

    // 更新新活跃文档
    if (m_documentStates.contains(id)) {
        DocumentState &state = m_documentStates[id];
        state.lastAccess = QDateTime::currentDateTime();

        // 如果文档已被卸载，重新加载
        if (state.isUnloaded) {
            locker.unlock();
            reloadDocument(id);
        }
    }
}

// 函数说明：实现 DocumentMemoryManager::documentState 的核心逻辑，供当前模块调用。
DocumentMemoryManager::DocumentState DocumentMemoryManager::documentState(const QString &id) const
{
    QMutexLocker locker(&m_mutex);
    return m_documentStates.value(id);
}

// 函数说明：判断 DocumentMemoryManager 当前是否满足指定状态。
bool DocumentMemoryManager::isDocumentLoaded(const QString &id) const
{
    QMutexLocker locker(&m_mutex);
    if (!m_documentStates.contains(id)) return false;
    return !m_documentStates[id].isUnloaded;
}

// 函数说明：判断 DocumentMemoryManager 当前是否满足指定状态。
bool DocumentMemoryManager::isDocumentModified(const QString &id) const
{
    QMutexLocker locker(&m_mutex);
    if (!m_documentStates.contains(id)) return false;
    return m_documentStates[id].isModified;
}

// 函数说明：实现 DocumentMemoryManager::unloadDocument 的核心逻辑，供当前模块调用。
void DocumentMemoryManager::unloadDocument(const QString &id)
{
    QMutexLocker locker(&m_mutex);

    if (!m_documentStates.contains(id) || !m_editors.contains(id)) return;
    if (m_documentStates[id].isUnloaded) return;
    if (id == m_activeDocumentId) return;  // 不卸载当前活跃文档

    // 保存状态
    saveDocumentState(id);

    // 清空编辑器内容以释放内存
    QPlainTextEdit *editor = m_editors[id];
    if (editor) {
        editor->clear();
    }

    m_documentStates[id].isUnloaded = true;

    locker.unlock();
    emit documentUnloaded(id);
    emit memoryUsageChanged(totalMemoryUsage());
}

// 函数说明：实现 DocumentMemoryManager::reloadDocument 的核心逻辑，供当前模块调用。
void DocumentMemoryManager::reloadDocument(const QString &id)
{
    QMutexLocker locker(&m_mutex);

    if (!m_documentStates.contains(id) || !m_editors.contains(id)) return;
    if (!m_documentStates[id].isUnloaded) return;

    if (restoreDocumentState(id)) {
        m_documentStates[id].isUnloaded = false;
        m_documentStates[id].lastAccess = QDateTime::currentDateTime();

        locker.unlock();
        emit documentReloaded(id);
        emit memoryUsageChanged(totalMemoryUsage());
    }
}

// 函数说明：实现 DocumentMemoryManager::unloadInactiveDocuments 的核心逻辑，供当前模块调用。
void DocumentMemoryManager::unloadInactiveDocuments()
{
    QMutexLocker locker(&m_mutex);

    QStringList toUnload;

    for (auto it = m_documentStates.begin(); it != m_documentStates.end(); ++it) {
        if (it.key() == m_activeDocumentId) continue;
        if (it.value().isUnloaded) continue;
        if (it.value().isModified) continue;  // 不卸载已修改的文档

        toUnload.append(it.key());
    }

    locker.unlock();

    for (const QString &id : toUnload) {
        unloadDocument(id);
    }
}

// 函数说明：实现 DocumentMemoryManager::totalMemoryUsage 的核心逻辑，供当前模块调用。
qint64 DocumentMemoryManager::totalMemoryUsage() const
{
    QMutexLocker locker(&m_mutex);

    qint64 total = 0;
    for (auto it = m_documentStates.begin(); it != m_documentStates.end(); ++it) {
        if (!it.value().isUnloaded) {
            total += it.value().contentSize;
        }
    }
    return total;
}

// 函数说明：加载 DocumentMemoryManager 需要的数据、配置或外部资源。
int DocumentMemoryManager::loadedDocumentCount() const
{
    QMutexLocker locker(&m_mutex);

    int count = 0;
    for (auto it = m_documentStates.begin(); it != m_documentStates.end(); ++it) {
        if (!it.value().isUnloaded) {
            count++;
        }
    }
    return count;
}

// 函数说明：加载 DocumentMemoryManager 需要的数据、配置或外部资源。
QStringList DocumentMemoryManager::loadedDocumentIds() const
{
    QMutexLocker locker(&m_mutex);

    QStringList ids;
    for (auto it = m_documentStates.begin(); it != m_documentStates.end(); ++it) {
        if (!it.value().isUnloaded) {
            ids.append(it.key());
        }
    }
    return ids;
}

// 函数说明：实现 DocumentMemoryManager::checkInactiveDocuments 的核心逻辑，供当前模块调用。
void DocumentMemoryManager::checkInactiveDocuments()
{
    if (!m_config.enableAutoUnload) return;

    QMutexLocker locker(&m_mutex);

    QDateTime now = QDateTime::currentDateTime();
    int loadedCount = 0;
    qint64 totalMemory = 0;
    QList<QPair<QString, QDateTime>> candidates;

    // 收集候选文档
    for (auto it = m_documentStates.begin(); it != m_documentStates.end(); ++it) {
        if (it.value().isUnloaded) continue;
        if (it.key() == m_activeDocumentId) continue;

        loadedCount++;
        totalMemory += it.value().contentSize;

        // 检查是否超过不活跃时间
        if (it.value().lastAccess.secsTo(now) > m_config.unloadTimeout) {
            if (!it.value().isModified) {
                candidates.append(qMakePair(it.key(), it.value().lastAccess));
            }
        }
    }

    // 按最后访问时间排序（最老的优先）
    std::sort(candidates.begin(), candidates.end(),
              [](const QPair<QString, QDateTime> &a, const QPair<QString, QDateTime> &b) {
                  return a.second < b.second;
              });

    locker.unlock();

    // 卸载超出限制的文档
    int toUnloadCount = qMax(0, loadedCount - m_config.maxLoadedDocuments + 1);
    for (int i = 0; i < qMin(toUnloadCount, candidates.size()); ++i) {
        unloadDocument(candidates[i].first);
    }

    // 检查内存限制
    if (totalMemory > m_config.maxMemoryUsage) {
        emit memoryWarning(totalMemory, m_config.maxMemoryUsage);

        // 继续卸载直到内存降到限制以下
        for (int i = toUnloadCount; i < candidates.size() && totalMemory > m_config.maxMemoryUsage; ++i) {
            unloadDocument(candidates[i].first);
            totalMemory -= m_documentStates.value(candidates[i].first).contentSize;
        }
    }
}

// 函数说明：响应 DocumentMemoryManager 收到的信号或异步回调，并更新界面状态。
void DocumentMemoryManager::onDocumentModified()
{
    QTextDocument *doc = qobject_cast<QTextDocument*>(sender());
    if (!doc) return;

    QMutexLocker locker(&m_mutex);

    // 找到对应的文档ID
    for (auto it = m_editors.begin(); it != m_editors.end(); ++it) {
        if (it.value() && it.value()->document() == doc) {
            m_documentStates[it.key()].isModified = doc->isModified();
            break;
        }
    }
}

// 函数说明：保存 DocumentMemoryManager 当前状态，保证用户修改可以持久化。
void DocumentMemoryManager::saveDocumentState(const QString &id)
{
    if (!m_editors.contains(id) || !m_editors[id]) return;

    QPlainTextEdit *editor = m_editors[id];
    DocumentState &state = m_documentStates[id];

    // 保存内容到缓存文件
    QFile file(cacheFilePath(id));
    if (file.open(QIODevice::WriteOnly)) {
        QDataStream stream(&file);
        stream << editor->toPlainText();
        stream << editor->textCursor().position();
        stream << editor->verticalScrollBar()->value();
        stream << editor->document()->isModified();
        file.close();
    }

    state.cursorPosition = editor->textCursor().position();
    state.scrollPosition = editor->verticalScrollBar()->value();
    state.content.clear();  // 清空内存中的内容
}

// 函数说明：实现 DocumentMemoryManager::restoreDocumentState 的核心逻辑，供当前模块调用。
bool DocumentMemoryManager::restoreDocumentState(const QString &id)
{
    if (!m_editors.contains(id) || !m_editors[id]) return false;

    QFile file(cacheFilePath(id));
    if (!file.open(QIODevice::ReadOnly)) {
        // 如果缓存文件不存在，尝试从原文件加载
        DocumentState &state = m_documentStates[id];
        if (!state.filePath.isEmpty() && QFile::exists(state.filePath)) {
            QFile sourceFile(state.filePath);
            if (sourceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString content = QString::fromUtf8(sourceFile.readAll());
                m_editors[id]->setPlainText(content);
                return true;
            }
        }
        return false;
    }

    QDataStream stream(&file);
    QString content;
    int cursorPos, scrollPos;
    bool isModified;

    stream >> content >> cursorPos >> scrollPos >> isModified;
    file.close();

    QPlainTextEdit *editor = m_editors[id];
    editor->setPlainText(content);

    // 恢复光标位置
    QTextCursor cursor = editor->textCursor();
    cursor.setPosition(qMin(cursorPos, editor->document()->characterCount() - 1));
    editor->setTextCursor(cursor);

    // 恢复滚动位置
    editor->verticalScrollBar()->setValue(scrollPos);

    // 恢复修改状态
    editor->document()->setModified(isModified);

    return true;
}

// 函数说明：实现 DocumentMemoryManager::cacheFilePath 的核心逻辑，供当前模块调用。
QString DocumentMemoryManager::cacheFilePath(const QString &id) const
{
    // 使用 MD5 哈希生成缓存文件名
    QByteArray hash = QCryptographicHash::hash(id.toUtf8(), QCryptographicHash::Md5);
    return m_config.cacheDirectory + "/" + hash.toHex() + ".cache";
}

// ============================================================================
// MemoryOptimizer 实现
// ============================================================================

MemoryOptimizer::MemoryOptimizer(QObject *parent)
    : QObject(parent)
{
    m_imageCacheManager = new ImageCacheManager(this);
    m_documentMemoryManager = new DocumentMemoryManager(this);

    connect(m_imageCacheManager, &ImageCacheManager::memoryWarning,
            this, &MemoryOptimizer::onImageCacheWarning);
    connect(m_documentMemoryManager, &DocumentMemoryManager::memoryWarning,
            this, &MemoryOptimizer::onDocumentMemoryWarning);

    m_monitorTimer = new QTimer(this);
    connect(m_monitorTimer, &QTimer::timeout, this, &MemoryOptimizer::monitorMemory);
}

// 函数说明：销毁 MemoryOptimizer 对象，释放本模块持有的资源。
MemoryOptimizer::~MemoryOptimizer()
{
}

// 函数说明：设置 MemoryOptimizer 的运行参数，并触发必要的界面或数据刷新。
void MemoryOptimizer::setConfig(const Config &config)
{
    m_config = config;
    m_monitorTimer->setInterval(config.monitorInterval);
}

// 函数说明：实现 MemoryOptimizer::optimizeNow 的核心逻辑，供当前模块调用。
void MemoryOptimizer::optimizeNow()
{
    qint64 beforeUsage = currentMemoryUsage();

    // 卸载不活跃文档
    m_documentMemoryManager->unloadInactiveDocuments();

    // 清理图片缓存（保留最近使用的）
    // LRU缓存会自动处理

    // 建议 Qt 进行垃圾回收
    QApplication::processEvents();

    qint64 afterUsage = currentMemoryUsage();
    qint64 freed = beforeUsage - afterUsage;

    if (freed > 0) {
        emit optimizationCompleted(freed);
    }
}

// 函数说明：清空 MemoryOptimizer 保存的临时状态或缓存数据。
void MemoryOptimizer::clearAllCaches()
{
    m_imageCacheManager->clearImages();
    m_documentMemoryManager->unloadInactiveDocuments();
}

// 函数说明：根据当前数据生成 MemoryOptimizer 需要的输出结果。
MemoryOptimizer::MemoryReport MemoryOptimizer::generateReport() const
{
    MemoryReport report;
    report.timestamp = QDateTime::currentDateTime();
    report.imageCacheUsage = m_imageCacheManager->memoryUsage();
    report.documentUsage = m_documentMemoryManager->totalMemoryUsage();
    report.cachedImages = m_imageCacheManager->cachedCount();
    report.loadedDocuments = m_documentMemoryManager->loadedDocumentCount();
    report.imageCacheHitRate = m_imageCacheManager->config().maxCachedImages > 0 ? 0.0 : 0.0;
    report.totalUsage = estimateProcessMemory();
    report.otherUsage = report.totalUsage - report.imageCacheUsage - report.documentUsage;

    return report;
}

// 函数说明：启动 MemoryOptimizer 的异步任务、会话或后台流程。
void MemoryOptimizer::startMonitoring()
{
    m_monitorTimer->start(m_config.monitorInterval);
}

// 函数说明：停止 MemoryOptimizer 正在运行的任务或会话。
void MemoryOptimizer::stopMonitoring()
{
    m_monitorTimer->stop();
}

// 函数说明：判断 MemoryOptimizer 当前是否满足指定状态。
bool MemoryOptimizer::isMonitoring() const
{
    return m_monitorTimer->isActive();
}

// 函数说明：实现 MemoryOptimizer::currentMemoryUsage 的核心逻辑，供当前模块调用。
qint64 MemoryOptimizer::currentMemoryUsage() const
{
    return estimateProcessMemory();
}

// 函数说明：判断 MemoryOptimizer 当前是否满足指定状态。
bool MemoryOptimizer::isMemoryWarning() const
{
    return currentMemoryUsage() > m_config.warningThreshold;
}

// 函数说明：判断 MemoryOptimizer 当前是否满足指定状态。
bool MemoryOptimizer::isMemoryCritical() const
{
    return currentMemoryUsage() > m_config.criticalThreshold;
}

// 函数说明：实现 MemoryOptimizer::monitorMemory 的核心逻辑，供当前模块调用。
void MemoryOptimizer::monitorMemory()
{
    MemoryReport report = generateReport();
    m_lastReport = report;

    emit reportGenerated(report);

    if (report.totalUsage > m_config.criticalThreshold) {
        emit memoryCritical(report);
        if (m_config.enableAutoOptimize) {
            optimizeNow();
        }
    } else if (report.totalUsage > m_config.warningThreshold) {
        emit memoryWarning(report);
    }
}

// 函数说明：响应 MemoryOptimizer 收到的信号或异步回调，并更新界面状态。
void MemoryOptimizer::onImageCacheWarning(qint64 current, qint64 max)
{
    Q_UNUSED(current)
    Q_UNUSED(max)
    // 可以在这里触发额外的优化
}

// 函数说明：响应 MemoryOptimizer 收到的信号或异步回调，并更新界面状态。
void MemoryOptimizer::onDocumentMemoryWarning(qint64 current, qint64 max)
{
    Q_UNUSED(current)
    Q_UNUSED(max)
    // 可以在这里触发额外的优化
}

// 函数说明：实现 MemoryOptimizer::estimateProcessMemory 的核心逻辑，供当前模块调用。
qint64 MemoryOptimizer::estimateProcessMemory() const
{
    qint64 memoryUsage = 0;

#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        memoryUsage = pmc.WorkingSetSize;
    }
#elif defined(Q_OS_MAC)
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &infoCount) == KERN_SUCCESS) {
        memoryUsage = info.resident_size;
    }
#elif defined(Q_OS_LINUX)
    // 读取 /proc/self/statm
    QFile file("/proc/self/statm");
    if (file.open(QIODevice::ReadOnly)) {
        QString line = file.readLine();
        QStringList parts = line.split(' ');
        if (parts.size() >= 2) {
            // 第二个值是 RSS（以页为单位）
            long pages = parts[1].toLong();
            memoryUsage = pages * sysconf(_SC_PAGESIZE);
        }
        file.close();
    }
#endif

    // 如果无法获取系统内存信息，使用估算值
    if (memoryUsage == 0) {
        memoryUsage = m_imageCacheManager->memoryUsage() +
                      m_documentMemoryManager->totalMemoryUsage() +
                      50 * 1024 * 1024;  // 估算基础内存使用
    }

    return memoryUsage;
}

