// 文件说明：app-static\ocr\screenshotdialog.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef SCREENSHOTDIALOG_H
#define SCREENSHOTDIALOG_H

#include <QDialog>
#include <QPixmap>
#include <QImage>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QFileInfo>
#include <QDir>
#include <QFileDialog>

#include "imagecompressor.h"
#include "smartnamer.h"
#include "ocrengine.h"

/**
 * @brief 截图保存对话框
 * 
 * 功能：
 * - 预览截图
 * - 智能命名（OCR 关键词）
 * - 压缩选项（格式、质量）
 * - OCR 识别选项
 * - 保存位置选择
 */
class ScreenshotDialog : public QDialog
{
    Q_OBJECT

public:
    struct SaveOptions {
        QString fileName;
        QString savePath;
        ImageCompressor::Format format;
        int quality;
        bool compress;
        bool performOcr;
        bool copyToClipboard;
        bool insertMarkdown;
    };

    explicit ScreenshotDialog(const QPixmap &screenshot, QWidget *parent = nullptr);
    ~ScreenshotDialog();

    SaveOptions options() const;
    QPixmap processedImage() const;
    QString ocrResult() const { return m_ocrResult; }

    void setDefaultSavePath(const QString &path);
    void setOcrEngine(OcrEngine *engine);
    void setSmartNamer(SmartNamer *namer);

signals:
    void ocrCompleted(const QString &text);

private slots:
    void onFormatChanged(int index);
    void onQualityChanged(int value);
    void onAutoNameClicked();
    void onOcrClicked();
    void onBrowseClicked();
    void updatePreview();
    void updateSizeEstimate();

private:
    void setupUI();
    void performOcrRecognition();
    QString generateSmartName();

    QPixmap m_screenshot;
    QPixmap m_processedPixmap;
    QString m_ocrResult;
    QString m_defaultSavePath;
    
    OcrEngine *m_ocrEngine;
    SmartNamer *m_smartNamer;
    ImageCompressor m_compressor;
    
    // UI 组件
    QLabel *m_previewLabel;
    QLabel *m_originalSizeLabel;
    QLabel *m_estimatedSizeLabel;
    QLabel *m_dimensionsLabel;
    
    QLineEdit *m_nameEdit;
    QPushButton *m_autoNameBtn;
    
    QComboBox *m_formatCombo;
    QSpinBox *m_qualitySpin;
    QCheckBox *m_compressCheck;
    
    QLineEdit *m_pathEdit;
    QPushButton *m_browseBtn;
    
    QCheckBox *m_ocrCheck;
    QPushButton *m_ocrBtn;
    QLabel *m_ocrStatusLabel;
    
    QCheckBox *m_clipboardCheck;
    QCheckBox *m_insertMarkdownCheck;
    
    QDialogButtonBox *m_buttonBox;
    
    static const int PREVIEW_SIZE = 300;
};


// ==================== 实现 ====================

inline ScreenshotDialog::ScreenshotDialog(const QPixmap &screenshot, QWidget *parent)
    : QDialog(parent)
    , m_screenshot(screenshot)
    , m_processedPixmap(screenshot)
    , m_ocrEngine(nullptr)
    , m_smartNamer(nullptr)
{
    setWindowTitle(tr("保存截图"));
    setMinimumWidth(500);
    setupUI();
    updatePreview();
    updateSizeEstimate();
}

inline ScreenshotDialog::~ScreenshotDialog()
{
}

inline ScreenshotDialog::SaveOptions ScreenshotDialog::options() const
{
    SaveOptions opts;
    opts.fileName = m_nameEdit->text();
    opts.savePath = m_pathEdit->text();
    opts.format = static_cast<ImageCompressor::Format>(
        m_formatCombo->currentData().toInt());
    opts.quality = m_qualitySpin->value();
    opts.compress = m_compressCheck->isChecked();
    opts.performOcr = m_ocrCheck->isChecked();
    opts.copyToClipboard = m_clipboardCheck->isChecked();
    opts.insertMarkdown = m_insertMarkdownCheck->isChecked();
    return opts;
}

inline QPixmap ScreenshotDialog::processedImage() const
{
    return m_processedPixmap;
}

inline void ScreenshotDialog::setDefaultSavePath(const QString &path)
{
    m_defaultSavePath = path;
    m_pathEdit->setText(path);
}

inline void ScreenshotDialog::setOcrEngine(OcrEngine *engine)
{
    m_ocrEngine = engine;
    m_ocrBtn->setEnabled(engine != nullptr && engine->isInitialized());
}

inline void ScreenshotDialog::setSmartNamer(SmartNamer *namer)
{
    m_smartNamer = namer;
    m_autoNameBtn->setEnabled(namer != nullptr);
}

inline void ScreenshotDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 上部：预览和信息
    QHBoxLayout *topLayout = new QHBoxLayout();
    
    // 预览
    QGroupBox *previewGroup = new QGroupBox(tr("预览"), this);
    QVBoxLayout *previewLayout = new QVBoxLayout(previewGroup);
    m_previewLabel = new QLabel(this);
    m_previewLabel->setFixedSize(PREVIEW_SIZE, PREVIEW_SIZE);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet("background-color: #f0f0f0; border: 1px solid #ccc;");
    previewLayout->addWidget(m_previewLabel);
    
    m_dimensionsLabel = new QLabel(this);
    m_dimensionsLabel->setAlignment(Qt::AlignCenter);
    previewLayout->addWidget(m_dimensionsLabel);
    
    topLayout->addWidget(previewGroup);
    
    // 信息和选项
    QVBoxLayout *infoLayout = new QVBoxLayout();
    
    // 文件名
    QGroupBox *nameGroup = new QGroupBox(tr("文件名"), this);
    QHBoxLayout *nameLayout = new QHBoxLayout(nameGroup);
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setText(QString("screenshot_%1")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")));
    nameLayout->addWidget(m_nameEdit, 1);
    
    m_autoNameBtn = new QPushButton(tr("智能命名"), this);
    m_autoNameBtn->setToolTip(tr("基于 OCR 识别内容自动生成文件名"));
    connect(m_autoNameBtn, &QPushButton::clicked, this, &ScreenshotDialog::onAutoNameClicked);
    nameLayout->addWidget(m_autoNameBtn);
    infoLayout->addWidget(nameGroup);
    
    // 格式和质量
    QGroupBox *formatGroup = new QGroupBox(tr("格式"), this);
    QGridLayout *formatLayout = new QGridLayout(formatGroup);
    
    formatLayout->addWidget(new QLabel(tr("格式:"), this), 0, 0);
    m_formatCombo = new QComboBox(this);
    m_formatCombo->addItem("WebP (推荐)", static_cast<int>(ImageCompressor::Format::WebP));
    m_formatCombo->addItem("JPEG", static_cast<int>(ImageCompressor::Format::JPEG));
    m_formatCombo->addItem("PNG (无损)", static_cast<int>(ImageCompressor::Format::PNG));
    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScreenshotDialog::onFormatChanged);
    formatLayout->addWidget(m_formatCombo, 0, 1);
    
    formatLayout->addWidget(new QLabel(tr("质量:"), this), 1, 0);
    m_qualitySpin = new QSpinBox(this);
    m_qualitySpin->setRange(10, 100);
    m_qualitySpin->setValue(80);
    m_qualitySpin->setSuffix("%");
    connect(m_qualitySpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ScreenshotDialog::onQualityChanged);
    formatLayout->addWidget(m_qualitySpin, 1, 1);
    
    m_compressCheck = new QCheckBox(tr("启用压缩"), this);
    m_compressCheck->setChecked(true);
    connect(m_compressCheck, &QCheckBox::toggled, this, &ScreenshotDialog::updateSizeEstimate);
    formatLayout->addWidget(m_compressCheck, 2, 0, 1, 2);
    
    // 大小信息
    m_originalSizeLabel = new QLabel(this);
    formatLayout->addWidget(m_originalSizeLabel, 3, 0, 1, 2);
    
    m_estimatedSizeLabel = new QLabel(this);
    m_estimatedSizeLabel->setStyleSheet("color: green; font-weight: bold;");
    formatLayout->addWidget(m_estimatedSizeLabel, 4, 0, 1, 2);
    
    infoLayout->addWidget(formatGroup);
    
    topLayout->addLayout(infoLayout, 1);
    mainLayout->addLayout(topLayout);
    
    // 保存路径
    QGroupBox *pathGroup = new QGroupBox(tr("保存位置"), this);
    QHBoxLayout *pathLayout = new QHBoxLayout(pathGroup);
    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setText(QDir::homePath() + "/Pictures");
    pathLayout->addWidget(m_pathEdit, 1);
    
    m_browseBtn = new QPushButton(tr("浏览..."), this);
    connect(m_browseBtn, &QPushButton::clicked, this, &ScreenshotDialog::onBrowseClicked);
    pathLayout->addWidget(m_browseBtn);
    mainLayout->addWidget(pathGroup);
    
    // OCR 选项
    QGroupBox *ocrGroup = new QGroupBox(tr("OCR 识别"), this);
    QHBoxLayout *ocrLayout = new QHBoxLayout(ocrGroup);
    
    m_ocrCheck = new QCheckBox(tr("识别图片中的文字"), this);
    ocrLayout->addWidget(m_ocrCheck);
    
    m_ocrBtn = new QPushButton(tr("立即识别"), this);
    m_ocrBtn->setEnabled(false);
    connect(m_ocrBtn, &QPushButton::clicked, this, &ScreenshotDialog::onOcrClicked);
    ocrLayout->addWidget(m_ocrBtn);
    
    m_ocrStatusLabel = new QLabel(this);
    ocrLayout->addWidget(m_ocrStatusLabel, 1);
    
    mainLayout->addWidget(ocrGroup);
    
    // 其他选项
    QHBoxLayout *optionsLayout = new QHBoxLayout();
    m_clipboardCheck = new QCheckBox(tr("复制到剪贴板"), this);
    m_clipboardCheck->setChecked(true);
    optionsLayout->addWidget(m_clipboardCheck);
    
    m_insertMarkdownCheck = new QCheckBox(tr("插入 Markdown 引用"), this);
    m_insertMarkdownCheck->setChecked(true);
    optionsLayout->addWidget(m_insertMarkdownCheck);
    
    optionsLayout->addStretch();
    mainLayout->addLayout(optionsLayout);
    
    // 按钮
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(m_buttonBox);
}

inline void ScreenshotDialog::onFormatChanged(int index)
{
    Q_UNUSED(index)
    
    ImageCompressor::Format format = static_cast<ImageCompressor::Format>(
        m_formatCombo->currentData().toInt());
    
    // PNG 不需要质量设置
    m_qualitySpin->setEnabled(format != ImageCompressor::Format::PNG);
    
    updateSizeEstimate();
}

inline void ScreenshotDialog::onQualityChanged(int value)
{
    Q_UNUSED(value)
    updateSizeEstimate();
}

inline void ScreenshotDialog::onAutoNameClicked()
{
    QString smartName = generateSmartName();
    if (!smartName.isEmpty()) {
        m_nameEdit->setText(smartName);
    }
}

inline void ScreenshotDialog::onOcrClicked()
{
    performOcrRecognition();
}

inline void ScreenshotDialog::onBrowseClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("选择保存目录"),
                                                     m_pathEdit->text());
    if (!dir.isEmpty()) {
        m_pathEdit->setText(dir);
    }
}

inline void ScreenshotDialog::updatePreview()
{
    QPixmap scaled = m_screenshot.scaled(PREVIEW_SIZE, PREVIEW_SIZE,
                                         Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_previewLabel->setPixmap(scaled);
    
    m_dimensionsLabel->setText(QString("%1 × %2 像素")
        .arg(m_screenshot.width())
        .arg(m_screenshot.height()));
}

inline void ScreenshotDialog::updateSizeEstimate()
{
    qint64 originalSize = m_screenshot.toImage().sizeInBytes();
    m_originalSizeLabel->setText(tr("原始大小: %1 KB")
        .arg(originalSize / 1024.0, 0, 'f', 1));
    
    if (!m_compressCheck->isChecked()) {
        m_estimatedSizeLabel->setText(tr("压缩已禁用"));
        return;
    }
    
    ImageCompressor::CompressionOptions opts;
    opts.format = static_cast<ImageCompressor::Format>(
        m_formatCombo->currentData().toInt());
    opts.quality = m_qualitySpin->value();
    
    qint64 estimated = m_compressor.estimateCompressedSize(
        m_screenshot.toImage(), opts);
    
    double ratio = originalSize > 0 ? 
        static_cast<double>(estimated) / originalSize * 100 : 100;
    
    m_estimatedSizeLabel->setText(tr("预估大小: %1 KB (%2%)")
        .arg(estimated / 1024.0, 0, 'f', 1)
        .arg(ratio, 0, 'f', 0));
}

inline void ScreenshotDialog::performOcrRecognition()
{
    if (!m_ocrEngine || !m_ocrEngine->isInitialized()) {
        m_ocrStatusLabel->setText(tr("OCR 引擎未初始化"));
        return;
    }
    
    m_ocrBtn->setEnabled(false);
    m_ocrStatusLabel->setText(tr("正在识别..."));
    
    OcrEngine::OcrResult result = m_ocrEngine->recognize(m_screenshot.toImage());
    
    if (result.success) {
        m_ocrResult = result.fullText;
        m_ocrStatusLabel->setText(tr("识别完成 (置信度: %1%)")
            .arg(result.averageConfidence * 100, 0, 'f', 1));
        
        // 自动生成文件名
        if (m_smartNamer) {
            QStringList keywords = m_smartNamer->extractKeywords(m_ocrResult, 3);
            if (!keywords.isEmpty()) {
                QString name = keywords.join("_") + "_" + 
                    QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
                m_nameEdit->setText(SmartNamer::sanitizeFileName(name));
            }
        }
        
        emit ocrCompleted(m_ocrResult);
    } else {
        m_ocrStatusLabel->setText(tr("识别失败: %1").arg(result.errorMessage));
    }
    
    m_ocrBtn->setEnabled(true);
}

inline QString ScreenshotDialog::generateSmartName()
{
    if (m_smartNamer) {
        SmartNamer::NamingResult result = m_smartNamer->generateName(
            m_screenshot.toImage());
        return result.baseName;
    }
    
    return QString("screenshot_%1")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
}

#endif // SCREENSHOTDIALOG_H

