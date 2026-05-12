// 文件说明：app-static\editor\largefilemanager.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef LARGEFILEMANAGER_H
#define LARGEFILEMANAGER_H

#include <QObject>
#include <QString>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QTimer>
#include <QElapsedTimer>
#include <QFuture>
#include <QMutex>
#include <QFile>

class MarkdownHighlighter;

/**
 * @brief 文件块信息
 */
struct FileChunk {
    qint64 startOffset;      // 文件起始偏移
    qint64 endOffset;        // 文件结束偏移
    int startLine;           // 起始行号
    int endLine;             // 结束行号
    QString content;         // 内容（可能为空，表示未加载）
    bool isLoaded;           // 是否已加载
    bool isModified;         // 是否已修改

    FileChunk()
        : startOffset(0)
        , endOffset(0)
        , startLine(0)
        , endLine(0)
        , isLoaded(false)
        , isModified(false)
    {}
};

/**
 * @brief 大文件管理器
 *
 * 功能：
 * - 检测大文件并自动启用优化模式
 * - 分块加载文件内容
 * - 延迟高亮处理
 * - 虚拟滚动支持
 * - 内存使用优化
 */
class LargeFileManager : public QObject
{
    Q_OBJECT

public:
    // 配置选项
    struct Config {
        qint64 largeFileThreshold;      // 大文件阈值（字节）
        qint64 veryLargeFileThreshold;  // 超大文件阈值
        int chunkSize;                   // 块大小（行数）
        int preloadChunks;               // 预加载块数
        int highlightDelay;              // 高亮延迟（毫秒）
        bool enableVirtualization;       // 启用虚拟化
        int maxMemoryUsage;              // 最大内存使用（MB）

        Config()
            : largeFileThreshold(1024 * 1024)       // 1MB
            , veryLargeFileThreshold(10 * 1024 * 1024)  // 10MB
            , chunkSize(500)                         // 500行一块
            , preloadChunks(3)                       // 预加载3块
            , highlightDelay(100)                    // 100ms延迟
            , enableVirtualization(true)
            , maxMemoryUsage(256)                    // 256MB
        {}
    };

    // 文件大小级别
    enum FileSizeLevel {
        Normal,         // 普通文件（无需优化）
        Large,          // 大文件（启用延迟高亮）
        VeryLarge       // 超大文件（启用分块加载）
    };

    explicit LargeFileManager(QObject *parent = nullptr);
    ~LargeFileManager();

    // 配置
    void setConfig(const Config &config);
    Config config() const { return m_config; }

    // 文件操作
    bool openFile(const QString &filePath, QPlainTextEdit *editor);
    bool saveFile(const QString &filePath, QPlainTextEdit *editor);
    void closeFile();

    // 文件信息
    QString currentFilePath() const { return m_currentFilePath; }
    qint64 fileSize() const { return m_fileSize; }
    int totalLines() const { return m_totalLines; }
    FileSizeLevel fileSizeLevel() const { return m_sizeLevel; }
    bool isLargeFile() const { return m_sizeLevel != Normal; }

    // 块管理
    int chunkCount() const { return m_chunks.size(); }
    bool isChunkLoaded(int chunkIndex) const;
    void loadChunk(int chunkIndex);
    void unloadChunk(int chunkIndex);
    void preloadChunksAround(int lineNumber);

    // 进度
    int loadProgress() const { return m_loadProgress; }
    bool isLoading() const { return m_isLoading; }
    void cancelLoading();

    // 内存管理
    qint64 memoryUsage() const;
    void optimizeMemory();

    // 高亮控制
    void setHighlighter(MarkdownHighlighter *highlighter);
    void enableDeferredHighlighting(bool enable);
    void highlightVisibleBlocks();

signals:
    void loadingStarted(const QString &filePath);
    void loadingProgress(int percent, const QString &message);
    void loadingFinished(bool success);
    void chunkLoaded(int chunkIndex);
    void chunkUnloaded(int chunkIndex);
    void memoryWarning(qint64 currentUsage, qint64 maxUsage);
    void fileSizeLevelChanged(FileSizeLevel level);

private slots:
    void onScrollPositionChanged();
    void onDeferredHighlight();
    void processNextChunk();

private:
    FileSizeLevel detectFileSizeLevel(qint64 size) const;
    void buildChunkMap(const QString &filePath);
    QString loadChunkContent(int chunkIndex);
    void updateEditorContent();
    int lineNumberToChunkIndex(int lineNumber) const;
    void scheduleHighlight();

    // 不同大小级别的加载方法
    bool loadNormalFile(const QString &filePath, QPlainTextEdit *editor);
    bool loadLargeFile(const QString &filePath, QPlainTextEdit *editor);
    bool loadVeryLargeFile(const QString &filePath, QPlainTextEdit *editor);

    Config m_config;
    QString m_currentFilePath;
    qint64 m_fileSize;
    int m_totalLines;
    FileSizeLevel m_sizeLevel;

    QVector<FileChunk> m_chunks;
    QPlainTextEdit *m_editor;
    MarkdownHighlighter *m_highlighter;

    bool m_isLoading;
    int m_loadProgress;
    int m_currentLoadingChunk;

    QTimer *m_highlightTimer;
    QTimer *m_preloadTimer;
    bool m_deferredHighlightEnabled;

    mutable QMutex m_mutex;
    QElapsedTimer m_performanceTimer;
};

/**
 * @brief 优化后的文本文档
 *
 * 针对大文件优化的 QTextDocument 包装器
 */
class OptimizedTextDocument : public QObject
{
    Q_OBJECT

public:
    explicit OptimizedTextDocument(QObject *parent = nullptr);
    ~OptimizedTextDocument();

    void setDocument(QTextDocument *doc);
    QTextDocument* document() const { return m_document; }

    // 优化设置
    void setLazyHighlighting(bool enable);
    void setMaxUndoSteps(int steps);
    void setBlockSignals(bool block);

    // 批量操作
    void beginBatchOperation();
    void endBatchOperation();

    // 内存优化
    void clearUndoStack();
    void compressDocument();

private:
    QTextDocument *m_document;
    bool m_lazyHighlighting;
    bool m_inBatchOperation;
    int m_originalUndoLimit;
};

/**
 * @brief 虚拟滚动辅助类
 *
 * 提供虚拟滚动所需的辅助功能
 */
class VirtualScrollHelper : public QObject
{
    Q_OBJECT

public:
    explicit VirtualScrollHelper(QPlainTextEdit *editor, QObject *parent = nullptr);
    ~VirtualScrollHelper();

    void setLargeFileManager(LargeFileManager *manager);

    // 获取可见区域信息
    int firstVisibleLine() const;
    int lastVisibleLine() const;
    int visibleLineCount() const;

    // 滚动预测
    int predictScrollDirection() const;
    bool isScrollingFast() const;

signals:
    void visibleRangeChanged(int firstLine, int lastLine);
    void scrollDirectionChanged(int direction);

private slots:
    void onScrollValueChanged();

private:
    QPlainTextEdit *m_editor;
    LargeFileManager *m_manager;
    int m_lastScrollValue;
    int m_scrollDirection;
    QElapsedTimer m_scrollTimer;
    QVector<int> m_scrollHistory;
};

#endif // LARGEFILEMANAGER_H

