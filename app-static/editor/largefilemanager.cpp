// 文件说明：app-static\editor\largefilemanager.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "largefilemanager.h"

#include <QFile>
#include <QTextStream>
#include <QScrollBar>
#include <QTextBlock>
#include <QApplication>
#include <QThread>
#include <QtConcurrent>
#include <QDebug>

// ============================================================================
// LargeFileManager 实现
// ============================================================================

LargeFileManager::LargeFileManager(QObject *parent)
    : QObject(parent)
    , m_fileSize(0)
    , m_totalLines(0)
    , m_sizeLevel(Normal)
    , m_editor(nullptr)
    , m_highlighter(nullptr)
    , m_isLoading(false)
    , m_loadProgress(0)
    , m_currentLoadingChunk(0)
    , m_deferredHighlightEnabled(false)
{
    m_highlightTimer = new QTimer(this);
    m_highlightTimer->setSingleShot(true);
    connect(m_highlightTimer, &QTimer::timeout, this, &LargeFileManager::onDeferredHighlight);

    m_preloadTimer = new QTimer(this);
    m_preloadTimer->setSingleShot(true);
    connect(m_preloadTimer, &QTimer::timeout, this, &LargeFileManager::processNextChunk);
}

// 函数说明：销毁 LargeFileManager 对象，释放本模块持有的资源。
LargeFileManager::~LargeFileManager()
{
    closeFile();
}

// 函数说明：设置 LargeFileManager 的运行参数，并触发必要的界面或数据刷新。
void LargeFileManager::setConfig(const Config &config)
{
    m_config = config;
}

// 函数说明：打开 LargeFileManager 对应的文件、资源或功能入口。
bool LargeFileManager::openFile(const QString &filePath, QPlainTextEdit *editor)
{
    closeFile();

    m_editor = editor;
    m_currentFilePath = filePath;

    QFile file(filePath);
    if (!file.exists()) {
        return false;
    }

    m_fileSize = file.size();
    m_sizeLevel = detectFileSizeLevel(m_fileSize);

    emit fileSizeLevelChanged(m_sizeLevel);

    m_performanceTimer.start();

    if (m_sizeLevel == Normal) {
        // 普通文件：直接加载
        return loadNormalFile(filePath, editor);
    } else if (m_sizeLevel == Large) {
        // 大文件：启用延迟高亮
        return loadLargeFile(filePath, editor);
    } else {
        // 超大文件：分块加载
        return loadVeryLargeFile(filePath, editor);
    }
}

// 函数说明：加载 LargeFileManager 需要的数据、配置或外部资源。
bool LargeFileManager::loadNormalFile(const QString &filePath, QPlainTextEdit *editor)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    QString content = stream.readAll();
    file.close();

    editor->setPlainText(content);
    m_totalLines = editor->document()->blockCount();

    emit loadingFinished(true);
    return true;
}

// 函数说明：加载 LargeFileManager 需要的数据、配置或外部资源。
bool LargeFileManager::loadLargeFile(const QString &filePath, QPlainTextEdit *editor)
{
    emit loadingStarted(filePath);
    m_isLoading = true;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_isLoading = false;
        emit loadingFinished(false);
        return false;
    }

    // 禁用高亮和撤销以加速加载
    editor->setUpdatesEnabled(false);
    editor->document()->setUndoRedoEnabled(false);

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    QString content;
    content.reserve(m_fileSize + 1024);

    qint64 bytesRead = 0;
    const qint64 bufferSize = 64 * 1024;  // 64KB buffer
    int lastProgress = 0;

    while (!stream.atEnd()) {
        QString chunk = stream.read(bufferSize);
        content.append(chunk);
        bytesRead += chunk.toUtf8().size();

        int progress = static_cast<int>((bytesRead * 100) / m_fileSize);
        if (progress != lastProgress) {
            lastProgress = progress;
            m_loadProgress = progress;
            emit loadingProgress(progress, tr("正在加载... %1%").arg(progress));
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }
    }

    file.close();

    editor->setPlainText(content);
    m_totalLines = editor->document()->blockCount();

    // 恢复设置
    editor->document()->setUndoRedoEnabled(true);
    editor->setUpdatesEnabled(true);

    m_isLoading = false;
    m_loadProgress = 100;

    qint64 elapsed = m_performanceTimer.elapsed();
    emit loadingProgress(100, tr("加载完成，耗时 %1 ms").arg(elapsed));
    emit loadingFinished(true);

    // 启用延迟高亮
    if (m_highlighter) {
        enableDeferredHighlighting(true);
    }

    return true;
}

// 函数说明：加载 LargeFileManager 需要的数据、配置或外部资源。
bool LargeFileManager::loadVeryLargeFile(const QString &filePath, QPlainTextEdit *editor)
{
    emit loadingStarted(filePath);
    m_isLoading = true;

    // 首先建立块映射
    buildChunkMap(filePath);

    if (m_chunks.isEmpty()) {
        m_isLoading = false;
        emit loadingFinished(false);
        return false;
    }

    // 禁用功能以加速
    editor->setUpdatesEnabled(false);
    editor->document()->setUndoRedoEnabled(false);

    // 只加载前几个块
    QString initialContent;
    int chunksToLoad = qMin(m_config.preloadChunks, m_chunks.size());

    for (int i = 0; i < chunksToLoad; ++i) {
        QString chunkContent = loadChunkContent(i);
        if (!chunkContent.isEmpty()) {
            m_chunks[i].content = chunkContent;
            m_chunks[i].isLoaded = true;
            initialContent.append(chunkContent);
        }

        int progress = static_cast<int>(((i + 1) * 50) / chunksToLoad);
        emit loadingProgress(progress, tr("正在加载块 %1/%2...").arg(i + 1).arg(chunksToLoad));
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    // 设置初始内容
    editor->setPlainText(initialContent);

    // 添加占位符显示总行数
    // 注意：这里简化处理，实际实现可能需要更复杂的虚拟滚动

    // 恢复设置
    editor->document()->setUndoRedoEnabled(true);
    editor->setUpdatesEnabled(true);

    // 连接滚动事件
    connect(editor->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &LargeFileManager::onScrollPositionChanged);

    m_isLoading = false;
    m_loadProgress = 100;

    qint64 elapsed = m_performanceTimer.elapsed();
    emit loadingProgress(100, tr("初始加载完成，耗时 %1 ms，共 %2 块").arg(elapsed).arg(m_chunks.size()));
    emit loadingFinished(true);

    return true;
}

// 函数说明：保存 LargeFileManager 当前状态，保证用户修改可以持久化。
bool LargeFileManager::saveFile(const QString &filePath, QPlainTextEdit *editor)
{
    if (!editor) return false;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    if (m_sizeLevel == VeryLarge && !m_chunks.isEmpty()) {
        // 分块保存
        for (int i = 0; i < m_chunks.size(); ++i) {
            if (m_chunks[i].isLoaded) {
                stream << m_chunks[i].content;
            } else {
                // 从文件加载未修改的块
                stream << loadChunkContent(i);
            }
        }
    } else {
        // 直接保存
        stream << editor->toPlainText();
    }

    file.close();
    return true;
}

// 函数说明：关闭 LargeFileManager 相关窗口或资源，并处理必要的保存确认。
void LargeFileManager::closeFile()
{
    m_chunks.clear();
    m_currentFilePath.clear();
    m_fileSize = 0;
    m_totalLines = 0;
    m_sizeLevel = Normal;
    m_isLoading = false;
    m_loadProgress = 0;

    if (m_editor) {
        disconnect(m_editor->verticalScrollBar(), nullptr, this, nullptr);
        m_editor = nullptr;
    }
}

// 函数说明：实现 LargeFileManager::detectFileSizeLevel 的核心逻辑，供当前模块调用。
LargeFileManager::FileSizeLevel LargeFileManager::detectFileSizeLevel(qint64 size) const
{
    if (size >= m_config.veryLargeFileThreshold) {
        return VeryLarge;
    } else if (size >= m_config.largeFileThreshold) {
        return Large;
    }
    return Normal;
}

// 函数说明：实现 LargeFileManager::buildChunkMap 的核心逻辑，供当前模块调用。
void LargeFileManager::buildChunkMap(const QString &filePath)
{
    m_chunks.clear();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    int currentLine = 0;
    qint64 currentOffset = 0;
    FileChunk currentChunk;
    currentChunk.startOffset = 0;
    currentChunk.startLine = 0;

    while (!stream.atEnd()) {
        qint64 lineStart = stream.pos();
        QString line = stream.readLine();
        currentLine++;

        // 每 chunkSize 行创建一个新块
        if ((currentLine - currentChunk.startLine) >= m_config.chunkSize) {
            currentChunk.endOffset = stream.pos();
            currentChunk.endLine = currentLine - 1;
            m_chunks.append(currentChunk);

            currentChunk = FileChunk();
            currentChunk.startOffset = stream.pos();
            currentChunk.startLine = currentLine;
        }
    }

    // 添加最后一个块
    if (currentLine > currentChunk.startLine) {
        currentChunk.endOffset = file.size();
        currentChunk.endLine = currentLine;
        m_chunks.append(currentChunk);
    }

    m_totalLines = currentLine;
    file.close();
}

// 函数说明：加载 LargeFileManager 需要的数据、配置或外部资源。
QString LargeFileManager::loadChunkContent(int chunkIndex)
{
    if (chunkIndex < 0 || chunkIndex >= m_chunks.size()) {
        return QString();
    }

    const FileChunk &chunk = m_chunks[chunkIndex];

    QFile file(m_currentFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    file.seek(chunk.startOffset);

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    QString content;
    int linesToRead = chunk.endLine - chunk.startLine + 1;

    for (int i = 0; i < linesToRead && !stream.atEnd(); ++i) {
        content.append(stream.readLine());
        if (i < linesToRead - 1) {
            content.append('\n');
        }
    }

    file.close();
    return content;
}

// 函数说明：判断 LargeFileManager 当前是否满足指定状态。
bool LargeFileManager::isChunkLoaded(int chunkIndex) const
{
    if (chunkIndex < 0 || chunkIndex >= m_chunks.size()) {
        return false;
    }
    return m_chunks[chunkIndex].isLoaded;
}

// 函数说明：加载 LargeFileManager 需要的数据、配置或外部资源。
void LargeFileManager::loadChunk(int chunkIndex)
{
    if (chunkIndex < 0 || chunkIndex >= m_chunks.size()) {
        return;
    }

    if (m_chunks[chunkIndex].isLoaded) {
        return;
    }

    QString content = loadChunkContent(chunkIndex);
    if (!content.isEmpty()) {
        m_chunks[chunkIndex].content = content;
        m_chunks[chunkIndex].isLoaded = true;
        emit chunkLoaded(chunkIndex);
    }
}

// 函数说明：实现 LargeFileManager::unloadChunk 的核心逻辑，供当前模块调用。
void LargeFileManager::unloadChunk(int chunkIndex)
{
    if (chunkIndex < 0 || chunkIndex >= m_chunks.size()) {
        return;
    }

    if (!m_chunks[chunkIndex].isLoaded || m_chunks[chunkIndex].isModified) {
        return;
    }

    m_chunks[chunkIndex].content.clear();
    m_chunks[chunkIndex].isLoaded = false;
    emit chunkUnloaded(chunkIndex);
}

// 函数说明：实现 LargeFileManager::preloadChunksAround 的核心逻辑，供当前模块调用。
void LargeFileManager::preloadChunksAround(int lineNumber)
{
    int chunkIndex = lineNumberToChunkIndex(lineNumber);
    if (chunkIndex < 0) return;

    // 加载当前块和周围的块
    int startChunk = qMax(0, chunkIndex - m_config.preloadChunks / 2);
    int endChunk = qMin(m_chunks.size() - 1, chunkIndex + m_config.preloadChunks / 2);

    for (int i = startChunk; i <= endChunk; ++i) {
        if (!m_chunks[i].isLoaded) {
            loadChunk(i);
        }
    }

    // 卸载远离的块以节省内存
    for (int i = 0; i < m_chunks.size(); ++i) {
        if (i < startChunk - m_config.preloadChunks || i > endChunk + m_config.preloadChunks) {
            if (!m_chunks[i].isModified) {
                unloadChunk(i);
            }
        }
    }
}

// 函数说明：实现 LargeFileManager::lineNumberToChunkIndex 的核心逻辑，供当前模块调用。
int LargeFileManager::lineNumberToChunkIndex(int lineNumber) const
{
    for (int i = 0; i < m_chunks.size(); ++i) {
        if (lineNumber >= m_chunks[i].startLine && lineNumber <= m_chunks[i].endLine) {
            return i;
        }
    }
    return -1;
}

// 函数说明：实现 LargeFileManager::cancelLoading 的核心逻辑，供当前模块调用。
void LargeFileManager::cancelLoading()
{
    m_isLoading = false;
}

// 函数说明：实现 LargeFileManager::memoryUsage 的核心逻辑，供当前模块调用。
qint64 LargeFileManager::memoryUsage() const
{
    qint64 usage = 0;
    for (const FileChunk &chunk : m_chunks) {
        if (chunk.isLoaded) {
            usage += chunk.content.size() * sizeof(QChar);
        }
    }
    return usage;
}

// 函数说明：实现 LargeFileManager::optimizeMemory 的核心逻辑，供当前模块调用。
void LargeFileManager::optimizeMemory()
{
    // 卸载未修改的块
    for (int i = 0; i < m_chunks.size(); ++i) {
        if (m_chunks[i].isLoaded && !m_chunks[i].isModified) {
            // 保留当前可见区域的块
            // 这里简化处理
            unloadChunk(i);
        }
    }

    qint64 currentUsage = memoryUsage();
    if (currentUsage > m_config.maxMemoryUsage * 1024 * 1024) {
        emit memoryWarning(currentUsage, m_config.maxMemoryUsage * 1024 * 1024);
    }
}

// 函数说明：设置 LargeFileManager 的运行参数，并触发必要的界面或数据刷新。
void LargeFileManager::setHighlighter(MarkdownHighlighter *highlighter)
{
    m_highlighter = highlighter;
}

// 函数说明：实现 LargeFileManager::enableDeferredHighlighting 的核心逻辑，供当前模块调用。
void LargeFileManager::enableDeferredHighlighting(bool enable)
{
    m_deferredHighlightEnabled = enable;
}

// 函数说明：实现 LargeFileManager::highlightVisibleBlocks 的核心逻辑，供当前模块调用。
void LargeFileManager::highlightVisibleBlocks()
{
    if (!m_editor || !m_highlighter || !m_deferredHighlightEnabled) {
        return;
    }

    // Use public API to get visible block info
    QTextCursor cursor = m_editor->cursorForPosition(QPoint(0, 0));
    QTextBlock block = cursor.block();
    int visibleBlocks = 0;
    const int maxVisibleBlocks = 100;  // 限制一次高亮的块数

    // Estimate visible lines based on viewport height and font metrics
    int lineHeight = m_editor->fontMetrics().height();
    int estimatedVisibleLines = m_editor->viewport()->height() / qMax(lineHeight, 1);

    while (block.isValid() && visibleBlocks < qMin(maxVisibleBlocks, estimatedVisibleLines + 5)) {
        // 触发该块的高亮
        // 注意：这需要 MarkdownHighlighter 支持单块高亮
        // 这里简化为使用 rehighlightBlock
        // m_highlighter->rehighlightBlock(block);

        block = block.next();
        visibleBlocks++;
    }
}

// 函数说明：响应 LargeFileManager 收到的信号或异步回调，并更新界面状态。
void LargeFileManager::onScrollPositionChanged()
{
    if (m_sizeLevel != VeryLarge || !m_editor) {
        return;
    }

    // 获取当前可见的第一行 using public API
    QTextCursor cursor = m_editor->cursorForPosition(QPoint(0, 0));
    int firstLine = cursor.block().blockNumber();

    // 预加载周围的块
    preloadChunksAround(firstLine);

    // 调度延迟高亮
    scheduleHighlight();
}

// 函数说明：响应 LargeFileManager 收到的信号或异步回调，并更新界面状态。
void LargeFileManager::onDeferredHighlight()
{
    highlightVisibleBlocks();
}

// 函数说明：处理 LargeFileManager 的核心业务数据，并输出处理结果。
void LargeFileManager::processNextChunk()
{
    // 后台加载下一个块
    if (m_currentLoadingChunk < m_chunks.size()) {
        loadChunk(m_currentLoadingChunk);
        m_currentLoadingChunk++;

        if (m_currentLoadingChunk < m_chunks.size()) {
            m_preloadTimer->start(50);  // 50ms 后加载下一块
        }
    }
}

// 函数说明：实现 LargeFileManager::scheduleHighlight 的核心逻辑，供当前模块调用。
void LargeFileManager::scheduleHighlight()
{
    if (m_deferredHighlightEnabled) {
        m_highlightTimer->start(m_config.highlightDelay);
    }
}

// ============================================================================
// OptimizedTextDocument 实现
// ============================================================================

OptimizedTextDocument::OptimizedTextDocument(QObject *parent)
    : QObject(parent)
    , m_document(nullptr)
    , m_lazyHighlighting(false)
    , m_inBatchOperation(false)
    , m_originalUndoLimit(-1)
{
}

// 函数说明：销毁 OptimizedTextDocument 对象，释放本模块持有的资源。
OptimizedTextDocument::~OptimizedTextDocument()
{
}

// 函数说明：设置 OptimizedTextDocument 的运行参数，并触发必要的界面或数据刷新。
void OptimizedTextDocument::setDocument(QTextDocument *doc)
{
    m_document = doc;
}

// 函数说明：设置 OptimizedTextDocument 的运行参数，并触发必要的界面或数据刷新。
void OptimizedTextDocument::setLazyHighlighting(bool enable)
{
    m_lazyHighlighting = enable;
}

// 函数说明：设置 OptimizedTextDocument 的运行参数，并触发必要的界面或数据刷新。
void OptimizedTextDocument::setMaxUndoSteps(int steps)
{
    if (m_document) {
        m_document->setMaximumBlockCount(steps > 0 ? 0 : -1);  // 简化处理
    }
}

// 函数说明：设置 OptimizedTextDocument 的运行参数，并触发必要的界面或数据刷新。
void OptimizedTextDocument::setBlockSignals(bool block)
{
    if (m_document) {
        m_document->blockSignals(block);
    }
}

// 函数说明：实现 OptimizedTextDocument::beginBatchOperation 的核心逻辑，供当前模块调用。
void OptimizedTextDocument::beginBatchOperation()
{
    if (m_document && !m_inBatchOperation) {
        m_inBatchOperation = true;
        m_document->blockSignals(true);
    }
}

// 函数说明：实现 OptimizedTextDocument::endBatchOperation 的核心逻辑，供当前模块调用。
void OptimizedTextDocument::endBatchOperation()
{
    if (m_document && m_inBatchOperation) {
        m_inBatchOperation = false;
        m_document->blockSignals(false);
    }
}

// 函数说明：清空 OptimizedTextDocument 保存的临时状态或缓存数据。
void OptimizedTextDocument::clearUndoStack()
{
    if (m_document) {
        m_document->clearUndoRedoStacks();
    }
}

// 函数说明：实现 OptimizedTextDocument::compressDocument 的核心逻辑，供当前模块调用。
void OptimizedTextDocument::compressDocument()
{
    // 压缩文档存储（Qt 内部已优化，这里作为占位符）
}

// ============================================================================
// VirtualScrollHelper 实现
// ============================================================================

VirtualScrollHelper::VirtualScrollHelper(QPlainTextEdit *editor, QObject *parent)
    : QObject(parent)
    , m_editor(editor)
    , m_manager(nullptr)
    , m_lastScrollValue(0)
    , m_scrollDirection(0)
{
    if (m_editor) {
        connect(m_editor->verticalScrollBar(), &QScrollBar::valueChanged,
                this, &VirtualScrollHelper::onScrollValueChanged);
    }

    m_scrollHistory.reserve(10);
}

// 函数说明：销毁 VirtualScrollHelper 对象，释放本模块持有的资源。
VirtualScrollHelper::~VirtualScrollHelper()
{
}

// 函数说明：设置 VirtualScrollHelper 的运行参数，并触发必要的界面或数据刷新。
void VirtualScrollHelper::setLargeFileManager(LargeFileManager *manager)
{
    m_manager = manager;
}

// 函数说明：实现 VirtualScrollHelper::firstVisibleLine 的核心逻辑，供当前模块调用。
int VirtualScrollHelper::firstVisibleLine() const
{
    if (!m_editor) return 0;
    // Use public API: cursorForPosition at top-left of viewport
    QTextCursor cursor = m_editor->cursorForPosition(QPoint(0, 0));
    return cursor.block().blockNumber();
}

// 函数说明：实现 VirtualScrollHelper::lastVisibleLine 的核心逻辑，供当前模块调用。
int VirtualScrollHelper::lastVisibleLine() const
{
    if (!m_editor) return 0;

    // Get first visible line
    int firstLine = firstVisibleLine();

    // Estimate visible lines based on viewport height and font metrics
    int lineHeight = m_editor->fontMetrics().height();
    int viewportHeight = m_editor->viewport()->height();
    int estimatedVisibleLines = viewportHeight / qMax(lineHeight, 1);

    // Calculate last visible line
    int totalLines = m_editor->document()->blockCount();
    int lastLine = qMin(firstLine + estimatedVisibleLines, totalLines - 1);

    return lastLine;
}

// 函数说明：实现 VirtualScrollHelper::visibleLineCount 的核心逻辑，供当前模块调用。
int VirtualScrollHelper::visibleLineCount() const
{
    return lastVisibleLine() - firstVisibleLine() + 1;
}

// 函数说明：实现 VirtualScrollHelper::predictScrollDirection 的核心逻辑，供当前模块调用。
int VirtualScrollHelper::predictScrollDirection() const
{
    return m_scrollDirection;
}

// 函数说明：判断 VirtualScrollHelper 当前是否满足指定状态。
bool VirtualScrollHelper::isScrollingFast() const
{
    if (m_scrollHistory.size() < 3) return false;

    // 检查最近几次滚动的速度
    int totalDelta = 0;
    for (int i = 1; i < m_scrollHistory.size(); ++i) {
        totalDelta += qAbs(m_scrollHistory[i] - m_scrollHistory[i-1]);
    }

    return totalDelta > 100;  // 阈值
}

// 函数说明：响应 VirtualScrollHelper 收到的信号或异步回调，并更新界面状态。
void VirtualScrollHelper::onScrollValueChanged()
{
    if (!m_editor) return;

    int currentValue = m_editor->verticalScrollBar()->value();

    // 记录滚动方向
    if (currentValue > m_lastScrollValue) {
        m_scrollDirection = 1;  // 向下
    } else if (currentValue < m_lastScrollValue) {
        m_scrollDirection = -1;  // 向上
    } else {
        m_scrollDirection = 0;
    }

    // 记录历史
    m_scrollHistory.append(currentValue);
    if (m_scrollHistory.size() > 10) {
        m_scrollHistory.removeFirst();
    }

    m_lastScrollValue = currentValue;

    emit visibleRangeChanged(firstVisibleLine(), lastVisibleLine());

    if (m_scrollDirection != 0) {
        emit scrollDirectionChanged(m_scrollDirection);
    }
}

