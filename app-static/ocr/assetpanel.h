// 文件说明：app-static\ocr\assetpanel.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef ASSETPANEL_H
#define ASSETPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QMenu>
#include <QAction>
#include <QLineEdit>
#include <QComboBox>
#include <QProgressBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QClipboard>
#include <QApplication>
#include <QToolTip>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QInputDialog>
#include <QDesktopServices>
#include <QFileInfo>

#include "assetmanager.h"
#include "imagecompressor.h"
#include "smartnamer.h"
#include "ocrengine.h"
#include "screencapture.h"

/**
 * @brief 资产看板 UI
 * 
 * 功能：
 * - 显示当前文档的所有资产
 * - 缩略图预览
 * - 状态指示（有效/未使用/损坏）
 * - 拖放添加
 * - 右键菜单操作
 * - 筛选和搜索
 * - 批量操作
 */
class AssetPanel : public QWidget
{
    Q_OBJECT

public:
    explicit AssetPanel(QWidget *parent = nullptr);
    ~AssetPanel();

    // 设置资产管理器
    void setAssetManager(AssetManager *manager);
    
    // 设置当前文档内容（用于扫描引用）
    void setDocumentContent(const QString &content);
    
    // 刷新资产列表
    void refresh();

signals:
    // 请求在编辑器中插入资产引用
    void insertAssetRequested(const QString &markdownSyntax);
    
    // 请求定位到引用位置
    void locateReferenceRequested(int lineNumber);
    
    // 截图完成
    void screenshotTaken(const QString &assetPath);
    
    // OCR 完成
    void ocrCompleted(const QString &markdownText);

public slots:
    void onScreenshotClicked();
    void onAddImageClicked();
    void onCleanupClicked();
    void onRefreshClicked();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onItemDoubleClicked(QListWidgetItem *item);
    void onItemContextMenu(const QPoint &pos);
    void onSearchTextChanged(const QString &text);
    void onFilterChanged(int index);
    void onSelectionChanged();
    
    void copyAssetPath();
    void copyAssetMarkdown();
    void deleteSelectedAssets();
    void renameAsset();
    void openInExplorer();
    void performOcr();
    void compressAsset();
    void configureTessdataPath();

private:
    void setupUI();
    void setupToolbar();
    void setupAssetList();
    void setupContextMenu();
    void populateAssetList();
    void updateStatusBar();
    
    QListWidgetItem* createAssetItem(const AssetManager::Asset &asset);
    QIcon createThumbnail(const QString &path, AssetManager::AssetType type);
    QString formatFileSize(qint64 bytes) const;
    QString getStatusIcon(AssetManager::AssetStatus status) const;
    void updateOcrPathStatus(bool initialized, const QString &message = QString());
    
    void processDroppedFile(const QString &filePath);

    AssetManager *m_assetManager;
    OcrEngine *m_ocrEngine;
    ImageCompressor *m_compressor;
    SmartNamer *m_smartNamer;
    ScreenCapture *m_screenCapture;
    
    QString m_documentContent;
    
    // UI 组件
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_toolbarLayout;
    
    QToolButton *m_screenshotBtn;
    QToolButton *m_addBtn;
    QToolButton *m_cleanupBtn;
    QToolButton *m_refreshBtn;
    QToolButton *m_tessdataBtn;
    
    QLineEdit *m_searchEdit;
    QLineEdit *m_tessdataPathEdit;
    QComboBox *m_filterCombo;
    QLabel *m_ocrStatusLabel;
    
    QListWidget *m_assetList;
    
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
    
    QMenu *m_contextMenu;
    QAction *m_copyPathAction;
    QAction *m_copyMarkdownAction;
    QAction *m_deleteAction;
    QAction *m_renameAction;
    QAction *m_openFolderAction;
    QAction *m_ocrAction;
    QAction *m_compressAction;
    
    static const int THUMBNAIL_SIZE = 64;
};


/**
 * @brief 资产列表项代理（自定义绘制）
 */
class AssetItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit AssetItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

private:
    static const int ITEM_HEIGHT = 72;
    static const int THUMBNAIL_SIZE = 56;
    static const int PADDING = 8;
};


/**
 * @brief 截图对话框
 */
class ScreenshotDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ScreenshotDialog(const QPixmap &screenshot, QWidget *parent = nullptr);

    QString fileName() const;
    bool shouldCompress() const;
    bool shouldOcr() const;

private:
    void setupUI();
    void updatePreview();

    QPixmap m_screenshot;
    QLabel *m_previewLabel;
    QLineEdit *m_nameEdit;
    QCheckBox *m_compressCheck;
    QCheckBox *m_ocrCheck;
    QLabel *m_sizeLabel;
    QDialogButtonBox *m_buttonBox;
};


// ==================== AssetPanel 实现 ====================

inline AssetPanel::AssetPanel(QWidget *parent)
    : QWidget(parent)
    , m_assetManager(nullptr)
    , m_ocrEngine(new OcrEngine(this))
    , m_compressor(new ImageCompressor(this))
    , m_smartNamer(new SmartNamer(this))
    , m_screenCapture(new ScreenCapture())
{
    setupUI();
    
    // 初始化 OCR 引擎
    const bool ocrInitialized = m_ocrEngine->initialize();
    updateOcrPathStatus(ocrInitialized, m_ocrEngine->lastInitializationError());
    m_smartNamer->setOcrEngine(m_ocrEngine);

    connect(m_ocrEngine, &OcrEngine::errorOccurred, this, [this](const QString &error) {
        updateOcrPathStatus(false, error);
    });
    
    // 连接截图信号
    connect(m_screenCapture, &ScreenCapture::captureCompleted,
            this, [this](const ScreenCapture::CaptureResult &result) {
                if (result.success && m_assetManager) {
                    // 生成智能文件名
                    SmartNamer::NamingResult naming = m_smartNamer->generateName(
                        result.pixmap.toImage());
                    
                    // 压缩并保存
                    QString destPath;
                    ImageCompressor::CompressionOptions opts = 
                        ImageCompressor::optimizeForWeb();
                    
                    QImage image = result.pixmap.toImage();
                    QString tempPath = QDir::temp().filePath(naming.suggestedName);
                    
                    ImageCompressor::CompressionResult compResult = 
                        m_compressor->compress(image, tempPath, opts);
                    
                    if (compResult.success) {
                        if (m_assetManager->addAsset(compResult.outputPath, &destPath)) {
                            QFile::remove(compResult.outputPath);
                            emit screenshotTaken(destPath);
                            
                            // 自动插入
                            QString markdown = QString("![%1](%2)")
                                .arg(naming.baseName)
                                .arg(destPath);
                            emit insertAssetRequested(markdown);
                            
                            refresh();
                        }
                    }
                }
            });
    
    setAcceptDrops(true);
}

inline AssetPanel::~AssetPanel()
{
    delete m_screenCapture;
}

inline void AssetPanel::setAssetManager(AssetManager *manager)
{
    m_assetManager = manager;
    
    if (m_assetManager) {
        connect(m_assetManager, &AssetManager::assetAdded,
                this, &AssetPanel::refresh);
        connect(m_assetManager, &AssetManager::assetRemoved,
                this, &AssetPanel::refresh);
        connect(m_assetManager, &AssetManager::assetChanged,
                this, &AssetPanel::refresh);
    }
}

inline void AssetPanel::setDocumentContent(const QString &content)
{
    m_documentContent = content;
}

inline void AssetPanel::refresh()
{
    if (!m_assetManager) return;
    
    // 扫描文档
    if (!m_documentContent.isEmpty()) {
        m_assetManager->scanDocument(m_documentContent);
    }
    
    populateAssetList();
    updateStatusBar();
}

inline void AssetPanel::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(4, 4, 4, 4);
    m_mainLayout->setSpacing(4);
    
    setupToolbar();
    setupAssetList();
    setupContextMenu();
    
    // 状态栏
    QHBoxLayout *statusLayout = new QHBoxLayout();
    m_statusLabel = new QLabel(this);
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    m_progressBar->setFixedHeight(16);
    
    statusLayout->addWidget(m_statusLabel, 1);
    statusLayout->addWidget(m_progressBar);
    m_mainLayout->addLayout(statusLayout);
}

inline void AssetPanel::setupToolbar()
{
    m_toolbarLayout = new QHBoxLayout();
    m_toolbarLayout->setSpacing(2);
    
    // 截图按钮
    m_screenshotBtn = new QToolButton(this);
    m_screenshotBtn->setIcon(QIcon("fa-camera.fontawesome"));
    m_screenshotBtn->setToolTip(tr("截图 (Ctrl+Shift+S)"));
    m_screenshotBtn->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    connect(m_screenshotBtn, &QToolButton::clicked, this, &AssetPanel::onScreenshotClicked);
    m_toolbarLayout->addWidget(m_screenshotBtn);
    
    // 添加图片
    m_addBtn = new QToolButton(this);
    m_addBtn->setIcon(QIcon("fa-plus.fontawesome"));
    m_addBtn->setToolTip(tr("添加图片"));
    connect(m_addBtn, &QToolButton::clicked, this, &AssetPanel::onAddImageClicked);
    m_toolbarLayout->addWidget(m_addBtn);

    // Tesseract 数据路径配置
    m_tessdataBtn = new QToolButton(this);
    m_tessdataBtn->setIcon(QIcon("fa-folder-open-o.fontawesome"));
    m_tessdataBtn->setToolTip(tr("配置 Tesseract tessdata 路径"));
    connect(m_tessdataBtn, &QToolButton::clicked, this, &AssetPanel::configureTessdataPath);
    m_toolbarLayout->addWidget(m_tessdataBtn);

    m_tessdataPathEdit = new QLineEdit(this);
    m_tessdataPathEdit->setReadOnly(true);
    m_tessdataPathEdit->setPlaceholderText(tr("tessdata 路径"));
    m_tessdataPathEdit->setMaximumWidth(240);
    m_toolbarLayout->addWidget(m_tessdataPathEdit);

    m_ocrStatusLabel = new QLabel(this);
    m_ocrStatusLabel->setMinimumWidth(96);
    m_toolbarLayout->addWidget(m_ocrStatusLabel);
    
    m_toolbarLayout->addStretch();
    
    // 搜索框
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("搜索..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setMaximumWidth(150);
    connect(m_searchEdit, &QLineEdit::textChanged, 
            this, &AssetPanel::onSearchTextChanged);
    m_toolbarLayout->addWidget(m_searchEdit);
    
    // 筛选
    m_filterCombo = new QComboBox(this);
    m_filterCombo->addItem(tr("全部"), -1);
    m_filterCombo->addItem(tr("已使用"), static_cast<int>(AssetManager::AssetStatus::Valid));
    m_filterCombo->addItem(tr("未使用"), static_cast<int>(AssetManager::AssetStatus::Orphaned));
    m_filterCombo->addItem(tr("缺失"), static_cast<int>(AssetManager::AssetStatus::Missing));
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AssetPanel::onFilterChanged);
    m_toolbarLayout->addWidget(m_filterCombo);
    
    // 清理按钮
    m_cleanupBtn = new QToolButton(this);
    m_cleanupBtn->setIcon(QIcon("fa-trash.fontawesome"));
    m_cleanupBtn->setToolTip(tr("清理未使用的资产"));
    connect(m_cleanupBtn, &QToolButton::clicked, this, &AssetPanel::onCleanupClicked);
    m_toolbarLayout->addWidget(m_cleanupBtn);
    
    // 刷新按钮
    m_refreshBtn = new QToolButton(this);
    m_refreshBtn->setIcon(QIcon("fa-refresh.fontawesome"));
    m_refreshBtn->setToolTip(tr("刷新"));
    connect(m_refreshBtn, &QToolButton::clicked, this, &AssetPanel::onRefreshClicked);
    m_toolbarLayout->addWidget(m_refreshBtn);
    
    m_mainLayout->addLayout(m_toolbarLayout);
}

inline void AssetPanel::setupAssetList()
{
    m_assetList = new QListWidget(this);
    m_assetList->setViewMode(QListView::ListMode);
    m_assetList->setIconSize(QSize(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
    m_assetList->setSpacing(2);
    m_assetList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_assetList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_assetList->setItemDelegate(new AssetItemDelegate(this));
    
    connect(m_assetList, &QListWidget::itemDoubleClicked,
            this, &AssetPanel::onItemDoubleClicked);
    connect(m_assetList, &QListWidget::customContextMenuRequested,
            this, &AssetPanel::onItemContextMenu);
    connect(m_assetList, &QListWidget::itemSelectionChanged,
            this, &AssetPanel::onSelectionChanged);
    
    m_mainLayout->addWidget(m_assetList, 1);
}

inline void AssetPanel::setupContextMenu()
{
    m_contextMenu = new QMenu(this);
    
    m_copyPathAction = m_contextMenu->addAction(tr("复制路径"));
    connect(m_copyPathAction, &QAction::triggered, this, &AssetPanel::copyAssetPath);
    
    m_copyMarkdownAction = m_contextMenu->addAction(tr("复制 Markdown"));
    m_copyMarkdownAction->setShortcut(QKeySequence::Copy);
    connect(m_copyMarkdownAction, &QAction::triggered, this, &AssetPanel::copyAssetMarkdown);
    
    m_contextMenu->addSeparator();
    
    m_ocrAction = m_contextMenu->addAction(tr("OCR 识别文字"));
    connect(m_ocrAction, &QAction::triggered, this, &AssetPanel::performOcr);
    
    m_compressAction = m_contextMenu->addAction(tr("压缩图片"));
    connect(m_compressAction, &QAction::triggered, this, &AssetPanel::compressAsset);
    
    m_contextMenu->addSeparator();
    
    m_renameAction = m_contextMenu->addAction(tr("重命名"));
    connect(m_renameAction, &QAction::triggered, this, &AssetPanel::renameAsset);
    
    m_openFolderAction = m_contextMenu->addAction(tr("在文件管理器中显示"));
    connect(m_openFolderAction, &QAction::triggered, this, &AssetPanel::openInExplorer);
    
    m_contextMenu->addSeparator();
    
    m_deleteAction = m_contextMenu->addAction(tr("删除"));
    m_deleteAction->setShortcut(QKeySequence::Delete);
    connect(m_deleteAction, &QAction::triggered, this, &AssetPanel::deleteSelectedAssets);
}

inline void AssetPanel::populateAssetList()
{
    m_assetList->clear();
    
    if (!m_assetManager) return;
    
    QString searchText = m_searchEdit->text().toLower();
    int filterStatus = m_filterCombo->currentData().toInt();
    
    for (const AssetManager::Asset &asset : m_assetManager->allAssets()) {
        // 搜索过滤
        if (!searchText.isEmpty() && 
            !asset.name.toLower().contains(searchText)) {
            continue;
        }
        
        // 状态过滤
        if (filterStatus >= 0 && 
            static_cast<int>(asset.status) != filterStatus) {
            continue;
        }
        
        QListWidgetItem *item = createAssetItem(asset);
        m_assetList->addItem(item);
    }
}

inline void AssetPanel::updateStatusBar()
{
    if (!m_assetManager) {
        m_statusLabel->setText(tr("未设置资产目录"));
        return;
    }
    
    AssetManager::Statistics stats = m_assetManager->getStatistics();
    
    m_statusLabel->setText(tr("%1 个文件 | %2 | %3 个未使用 (%4)")
        .arg(stats.totalCount)
        .arg(formatFileSize(stats.totalSize))
        .arg(stats.orphanedCount)
        .arg(formatFileSize(stats.orphanedSize)));
}

inline QListWidgetItem* AssetPanel::createAssetItem(const AssetManager::Asset &asset)
{
    QListWidgetItem *item = new QListWidgetItem();
    
    // 图标
    item->setIcon(createThumbnail(asset.absolutePath, asset.type));
    
    // 文本
    QString text = asset.name;
    QString statusIcon = getStatusIcon(asset.status);
    
    item->setText(statusIcon + " " + text);
    
    // 工具提示
    QString tooltip = QString(
        "<b>%1</b><br>"
        "大小: %2<br>"
        "引用次数: %3<br>"
        "状态: %4"
    ).arg(asset.name)
     .arg(formatFileSize(asset.size))
     .arg(asset.referenceCount)
     .arg(asset.status == AssetManager::AssetStatus::Valid ? tr("已使用") :
          asset.status == AssetManager::AssetStatus::Orphaned ? tr("未使用") :
          tr("缺失"));
    
    if (asset.type == AssetManager::AssetType::Image && asset.dimensions.isValid()) {
        tooltip += QString("<br>尺寸: %1×%2")
            .arg(asset.dimensions.width())
            .arg(asset.dimensions.height());
    }
    
    item->setToolTip(tooltip);
    
    // 存储资产路径
    item->setData(Qt::UserRole, asset.path);
    item->setData(Qt::UserRole + 1, static_cast<int>(asset.status));
    
    // 根据状态设置颜色
    if (asset.status == AssetManager::AssetStatus::Orphaned) {
        item->setForeground(QColor("#ff9800"));
    } else if (asset.status == AssetManager::AssetStatus::Missing) {
        item->setForeground(QColor("#f44336"));
    }
    
    return item;
}

inline QIcon AssetPanel::createThumbnail(const QString &path, AssetManager::AssetType type)
{
    if (type == AssetManager::AssetType::Image) {
        QPixmap pixmap(path);
        if (!pixmap.isNull()) {
            return QIcon(pixmap.scaled(THUMBNAIL_SIZE, THUMBNAIL_SIZE, 
                                       Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
    
    // 默认图标
    switch (type) {
        case AssetManager::AssetType::Image:
            return QIcon("fa-image.fontawesome");
        case AssetManager::AssetType::PDF:
            return QIcon("fa-file-pdf-o.fontawesome");
        case AssetManager::AssetType::Video:
            return QIcon("fa-file-video-o.fontawesome");
        case AssetManager::AssetType::Audio:
            return QIcon("fa-file-audio-o.fontawesome");
        default:
            return QIcon("fa-file-o.fontawesome");
    }
}

inline QString AssetPanel::formatFileSize(qint64 bytes) const
{
    if (bytes < 1024) return QString("%1 B").arg(bytes);
    if (bytes < 1024 * 1024) return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    if (bytes < 1024 * 1024 * 1024) return QString("%1 MB").arg(bytes / (1024.0 * 1024), 0, 'f', 1);
    return QString("%1 GB").arg(bytes / (1024.0 * 1024 * 1024), 0, 'f', 2);
}

inline QString AssetPanel::getStatusIcon(AssetManager::AssetStatus status) const
{
    switch (status) {
        case AssetManager::AssetStatus::Valid: return "✓";
        case AssetManager::AssetStatus::Orphaned: return "⚠";
        case AssetManager::AssetStatus::Missing: return "✗";
        default: return "?";
    }
}

inline void AssetPanel::updateOcrPathStatus(bool initialized, const QString &message)
{
    if (!m_ocrEngine) {
        return;
    }

    const QString resolvedPath = m_ocrEngine->resolvedTessdataPath();
    const QString displayPath = resolvedPath.isEmpty() ? m_ocrEngine->tessdataPath() : resolvedPath;

    if (m_tessdataPathEdit) {
        m_tessdataPathEdit->setText(displayPath);
        m_tessdataPathEdit->setToolTip(displayPath);
    }

    if (m_ocrStatusLabel) {
        if (initialized) {
            m_ocrStatusLabel->setText(tr("OCR 就绪"));
            m_ocrStatusLabel->setStyleSheet(QStringLiteral("QLabel { color: #2e7d32; }"));
        } else {
            m_ocrStatusLabel->setText(tr("OCR 未就绪"));
            m_ocrStatusLabel->setStyleSheet(QStringLiteral("QLabel { color: #c62828; }"));
        }
        m_ocrStatusLabel->setToolTip(message);
    }
}

inline void AssetPanel::onScreenshotClicked()
{
    m_screenCapture->startCapture(ScreenCapture::CaptureMode::Region);
}

inline void AssetPanel::onAddImageClicked()
{
    QStringList files = QFileDialog::getOpenFileNames(this,
        tr("选择图片"),
        QString(),
        tr("图片文件 (*.png *.jpg *.jpeg *.gif *.webp *.bmp);;所有文件 (*)"));
    
    for (const QString &file : files) {
        processDroppedFile(file);
    }
}

inline void AssetPanel::onCleanupClicked()
{
    if (!m_assetManager) return;
    
    QStringList orphaned = m_assetManager->cleanupOrphanedAssets(true);
    
    if (orphaned.isEmpty()) {
        QMessageBox::information(this, tr("清理完成"),
            tr("没有未使用的资产需要清理。"));
        return;
    }
    
    QString message = tr("发现 %1 个未使用的文件:\n\n").arg(orphaned.size());
    for (int i = 0; i < qMin(orphaned.size(), 10); ++i) {
        message += "• " + orphaned[i] + "\n";
    }
    if (orphaned.size() > 10) {
        message += tr("...(还有 %1 个文件)").arg(orphaned.size() - 10);
    }
    message += tr("\n\n是否移动到回收站？");
    
    if (QMessageBox::question(this, tr("清理未使用的资产"), message) == QMessageBox::Yes) {
        m_assetManager->cleanupOrphanedAssets(false);
        refresh();
    }
}

inline void AssetPanel::onRefreshClicked()
{
    refresh();
}

inline void AssetPanel::configureTessdataPath()
{
    if (!m_ocrEngine) {
        return;
    }

    QString startPath = m_ocrEngine->resolvedTessdataPath();
    if (startPath.isEmpty()) {
        startPath = m_ocrEngine->tessdataPath();
    }
    if (startPath.isEmpty()) {
        startPath = QDir::homePath();
    }

    const QString selectedPath = QFileDialog::getExistingDirectory(
        this,
        tr("选择 tessdata 目录或其父目录"),
        startPath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (selectedPath.isEmpty()) {
        return;
    }

    m_ocrEngine->setTessdataPath(selectedPath);
    const bool initialized = m_ocrEngine->initialize(selectedPath);
    const QString errorMessage = m_ocrEngine->lastInitializationError();
    updateOcrPathStatus(initialized, errorMessage);

    if (initialized) {
        const QStringList languages = m_ocrEngine->availableLanguages();
        QString languagePreview;
        if (!languages.isEmpty()) {
            languagePreview = tr("可用语言: %1").arg(languages.mid(0, 6).join(QStringLiteral(", ")));
            if (languages.size() > 6) {
                languagePreview += tr(" ...");
            }
        }

        QMessageBox::information(
            this,
            tr("OCR 配置已更新"),
            tr("tessdata 路径已生效。\n%1").arg(
                languagePreview.isEmpty() ? tr("已完成路径校验。") : languagePreview));
    } else {
        QMessageBox::warning(
            this,
            tr("OCR 配置无效"),
            tr("所选路径未通过校验：\n%1")
                .arg(errorMessage.isEmpty() ? tr("未知错误") : errorMessage));
    }
}

inline void AssetPanel::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

inline void AssetPanel::dropEvent(QDropEvent *event)
{
    for (const QUrl &url : event->mimeData()->urls()) {
        if (url.isLocalFile()) {
            processDroppedFile(url.toLocalFile());
        }
    }
}

inline void AssetPanel::processDroppedFile(const QString &filePath)
{
    if (!m_assetManager) return;
    
    if (!AssetManager::isSupportedAsset(filePath)) {
        return;
    }
    
    // 智能命名
    SmartNamer::NamingResult naming = m_smartNamer->generateName(filePath);
    
    // 压缩（如果是图片）
    QString destPath;
    if (AssetManager::isImageFile(filePath)) {
        ImageCompressor::CompressionOptions opts = ImageCompressor::optimizeForWeb();
        QString tempPath = QDir::temp().filePath(naming.suggestedName);
        
        ImageCompressor::CompressionResult result = m_compressor->compress(filePath, tempPath, opts);
        
        if (result.success) {
            m_assetManager->addAsset(result.outputPath, &destPath);
            QFile::remove(result.outputPath);
        }
    } else {
        m_assetManager->addAsset(filePath, &destPath);
    }
    
    refresh();
}

inline void AssetPanel::onItemDoubleClicked(QListWidgetItem *item)
{
    QString path = item->data(Qt::UserRole).toString();
    QString markdown = QString("![%1](%2)")
        .arg(QFileInfo(path).completeBaseName())
        .arg(path);
    emit insertAssetRequested(markdown);
}

inline void AssetPanel::onItemContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_assetList->itemAt(pos);
    if (item) {
        m_contextMenu->exec(m_assetList->mapToGlobal(pos));
    }
}

inline void AssetPanel::onSearchTextChanged(const QString &text)
{
    Q_UNUSED(text)
    populateAssetList();
}

inline void AssetPanel::onFilterChanged(int index)
{
    Q_UNUSED(index)
    populateAssetList();
}

inline void AssetPanel::onSelectionChanged()
{
    bool hasSelection = !m_assetList->selectedItems().isEmpty();
    m_deleteAction->setEnabled(hasSelection);
    m_copyPathAction->setEnabled(hasSelection);
    m_copyMarkdownAction->setEnabled(hasSelection);
}

inline void AssetPanel::copyAssetPath()
{
    QListWidgetItem *item = m_assetList->currentItem();
    if (item) {
        QString path = item->data(Qt::UserRole).toString();
        QApplication::clipboard()->setText(path);
    }
}

inline void AssetPanel::copyAssetMarkdown()
{
    QListWidgetItem *item = m_assetList->currentItem();
    if (item) {
        QString path = item->data(Qt::UserRole).toString();
        QString name = QFileInfo(path).completeBaseName();
        QString markdown = QString("![%1](%2)").arg(name).arg(path);
        QApplication::clipboard()->setText(markdown);
    }
}

inline void AssetPanel::deleteSelectedAssets()
{
    QList<QListWidgetItem*> items = m_assetList->selectedItems();
    if (items.isEmpty()) return;
    
    if (QMessageBox::question(this, tr("删除资产"),
        tr("确定要删除选中的 %1 个文件吗？").arg(items.size())) != QMessageBox::Yes) {
        return;
    }
    
    for (QListWidgetItem *item : items) {
        QString path = item->data(Qt::UserRole).toString();
        m_assetManager->removeAsset(path, true);
    }
    
    refresh();
}

inline void AssetPanel::renameAsset()
{
    QListWidgetItem *item = m_assetList->currentItem();
    if (!item || !m_assetManager) return;
    
    QString oldPath = item->data(Qt::UserRole).toString();
    QString oldName = QFileInfo(oldPath).fileName();
    
    bool ok;
    QString newName = QInputDialog::getText(this, tr("重命名"),
        tr("新文件名:"), QLineEdit::Normal, oldName, &ok);
    
    if (ok && !newName.isEmpty() && newName != oldName) {
        QString newPath = QFileInfo(oldPath).absolutePath() + "/" + newName;
        m_assetManager->renameAsset(oldPath, 
            m_assetManager->makeRelativePath(newPath));
        refresh();
    }
}

inline void AssetPanel::openInExplorer()
{
    QListWidgetItem *item = m_assetList->currentItem();
    if (!item || !m_assetManager) return;
    
    QString path = m_assetManager->resolveAssetPath(item->data(Qt::UserRole).toString());
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
}

inline void AssetPanel::performOcr()
{
    QListWidgetItem *item = m_assetList->currentItem();
    if (!item || !m_ocrEngine) return;
    
    QString path = m_assetManager->resolveAssetPath(item->data(Qt::UserRole).toString());
    
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    
    connect(m_ocrEngine, &OcrEngine::recognitionProgress,
            m_progressBar, &QProgressBar::setValue);
    
    m_ocrEngine->recognizeAsync(path);
    
    connect(m_ocrEngine, &OcrEngine::recognitionCompleted,
            this, [this](const OcrEngine::OcrResult &result) {
                m_progressBar->setVisible(false);
                
                if (result.success) {
                    QString markdown = OcrToMarkdown::convertSmart(result);
                    emit ocrCompleted(markdown);
                    
                    // 复制到剪贴板
                    QApplication::clipboard()->setText(markdown);
                    
                    QMessageBox::information(this, tr("OCR 完成"),
                        tr("识别结果已复制到剪贴板。\n"
                           "置信度: %1%").arg(result.averageConfidence * 100, 0, 'f', 1));
                } else {
                    QMessageBox::warning(this, tr("OCR 失败"),
                        result.errorMessage);
                }
            });
}

inline void AssetPanel::compressAsset()
{
    QListWidgetItem *item = m_assetList->currentItem();
    if (!item || !m_assetManager) return;
    
    QString path = m_assetManager->resolveAssetPath(item->data(Qt::UserRole).toString());
    
    if (!AssetManager::isImageFile(path)) {
        QMessageBox::information(this, tr("压缩"),
            tr("只能压缩图片文件。"));
        return;
    }
    
    ImageCompressor::CompressionOptions opts = ImageCompressor::optimizeForWeb();
    
    QString outputPath = path;
    QFileInfo info(path);
    if (info.suffix().toLower() != "webp") {
        outputPath = info.absolutePath() + "/" + info.completeBaseName() + ".webp";
    }
    
    ImageCompressor::CompressionResult result = m_compressor->compress(path, outputPath, opts);
    
    if (result.success) {
        QMessageBox::information(this, tr("压缩完成"),
            tr("压缩比: %1%\n原始大小: %2\n压缩后: %3")
                .arg(result.compressionRatio * 100, 0, 'f', 1)
                .arg(formatFileSize(result.originalSize))
                .arg(formatFileSize(result.compressedSize)));
        
        refresh();
    } else {
        QMessageBox::warning(this, tr("压缩失败"), result.errorMessage);
    }
}


// ==================== AssetItemDelegate 实现 ====================

inline AssetItemDelegate::AssetItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

inline void AssetItemDelegate::paint(QPainter *painter, 
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const
{
    painter->save();
    
    // 背景
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    } else if (option.state & QStyle::State_MouseOver) {
        painter->fillRect(option.rect, option.palette.light());
    }
    
    QRect rect = option.rect.adjusted(PADDING, PADDING, -PADDING, -PADDING);
    
    // 缩略图
    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    QRect iconRect(rect.left(), rect.top(), THUMBNAIL_SIZE, THUMBNAIL_SIZE);
    icon.paint(painter, iconRect);
    
    // 文本区域
    QRect textRect = rect;
    textRect.setLeft(iconRect.right() + PADDING);
    
    // 文件名
    QString text = index.data(Qt::DisplayRole).toString();
    QFont font = painter->font();
    font.setBold(true);
    painter->setFont(font);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop, text);
    
    painter->restore();
}

inline QSize AssetItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    return QSize(200, ITEM_HEIGHT);
}

#endif // ASSETPANEL_H

