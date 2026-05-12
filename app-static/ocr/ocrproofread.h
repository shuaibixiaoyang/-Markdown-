// 文件说明：app-static\ocr\ocrproofread.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef OCRPROOFREAD_H
#define OCRPROOFREAD_H

#include <QDialog>
#include <QWidget>
#include <QSplitter>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QRubberBand>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QClipboard>
#include <QApplication>
#include <QFileDialog>

#include "ocrengine.h"

/**
 * @brief OCR 结果校对界面
 * 
 * 功能：
 * - 图像与文字并排显示
 * - 区域高亮（点击文字定位到图像区域）
 * - 手动编辑和修正
 * - 重新识别选定区域
 * - 置信度标记（低置信度文字高亮）
 * - 导出为多种格式
 */
class OcrProofreadWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OcrProofreadWidget(QWidget *parent = nullptr);
    ~OcrProofreadWidget();

    // 设置 OCR 结果
    void setOcrResult(const OcrEngine::OcrResult &result, const QImage &sourceImage);
    
    // 获取编辑后的文本
    QString editedText() const;
    
    // 设置 OCR 引擎（用于重新识别）
    void setOcrEngine(OcrEngine *engine);

signals:
    void textChanged();
    void accepted();
    void rejected();

public slots:
    void copyToClipboard();
    void selectAll();
    void highlightLowConfidence(bool enable);
    void rerecognizeSelection();

private slots:
    void onTextCursorChanged();
    void onRegionClicked(const QRect &region);
    void onZoomChanged(int value);
    void syncScrollBars();

private:
    void setupUI();
    void populateTextEdit();
    void highlightRegion(const QRect &region);
    void clearHighlight();

    OcrEngine::OcrResult m_result;
    QImage m_sourceImage;
    OcrEngine *m_ocrEngine;
    
    // UI 组件
    QSplitter *m_splitter;
    
    // 图像视图
    QGraphicsView *m_imageView;
    QGraphicsScene *m_scene;
    QGraphicsPixmapItem *m_pixmapItem;
    QGraphicsRectItem *m_highlightRect;
    
    // 文本编辑
    QPlainTextEdit *m_textEdit;
    
    // 工具栏
    QSpinBox *m_zoomSpin;
    QCheckBox *m_highlightCheck;
    QPushButton *m_rerecognizeBtn;
    QPushButton *m_copyBtn;
    
    // 区域映射
    QMap<int, OcrEngine::TextRegion> m_lineToRegion;
    
    float m_confidenceThreshold;
};


/**
 * @brief 图像视图（支持区域点击）
 */
class OcrImageView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit OcrImageView(QWidget *parent = nullptr);
    
    void setImage(const QImage &image);
    void setRegions(const QVector<OcrEngine::TextRegion> &regions);
    void highlightRegion(const QRect &rect);
    void clearHighlight();
    void setZoom(double factor);

signals:
    void regionClicked(const QRect &region);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QGraphicsScene *m_scene;
    QGraphicsPixmapItem *m_pixmapItem;
    QGraphicsRectItem *m_highlightRect;
    QVector<OcrEngine::TextRegion> m_regions;
    double m_zoomFactor;
};


/**
 * @brief OCR 校对对话框
 */
class OcrProofreadDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OcrProofreadDialog(const OcrEngine::OcrResult &result,
                                 const QImage &sourceImage,
                                 QWidget *parent = nullptr);

    QString editedText() const;
    
    void setOcrEngine(OcrEngine *engine);

private slots:
    void onAccept();
    void onCopy();
    void onSaveAs();

private:
    void setupUI();

    OcrProofreadWidget *m_proofreadWidget;
    QPushButton *m_copyBtn;
    QPushButton *m_saveAsBtn;
    QDialogButtonBox *m_buttonBox;
};


// ==================== OcrProofreadWidget 实现 ====================

inline OcrProofreadWidget::OcrProofreadWidget(QWidget *parent)
    : QWidget(parent)
    , m_ocrEngine(nullptr)
    , m_highlightRect(nullptr)
    , m_confidenceThreshold(0.7f)
{
    setupUI();
}

inline OcrProofreadWidget::~OcrProofreadWidget()
{
}

inline void OcrProofreadWidget::setOcrResult(const OcrEngine::OcrResult &result,
                                              const QImage &sourceImage)
{
    m_result = result;
    m_sourceImage = sourceImage;
    
    // 设置图像
    m_scene->clear();
    m_pixmapItem = m_scene->addPixmap(QPixmap::fromImage(sourceImage));
    m_highlightRect = nullptr;
    
    m_imageView->setSceneRect(m_pixmapItem->boundingRect());
    m_imageView->fitInView(m_pixmapItem, Qt::KeepAspectRatio);
    
    // 填充文本
    populateTextEdit();
}

inline QString OcrProofreadWidget::editedText() const
{
    return m_textEdit->toPlainText();
}

inline void OcrProofreadWidget::setOcrEngine(OcrEngine *engine)
{
    m_ocrEngine = engine;
    m_rerecognizeBtn->setEnabled(engine != nullptr && engine->isInitialized());
}

inline void OcrProofreadWidget::copyToClipboard()
{
    QApplication::clipboard()->setText(m_textEdit->toPlainText());
}

inline void OcrProofreadWidget::selectAll()
{
    m_textEdit->selectAll();
}

inline void OcrProofreadWidget::highlightLowConfidence(bool enable)
{
    if (!enable) {
        // 清除格式
        QString text = m_textEdit->toPlainText();
        m_textEdit->setPlainText(text);
        return;
    }
    
    // 高亮低置信度文字
    QTextCharFormat lowConfFormat;
    lowConfFormat.setBackground(QColor(255, 200, 200));  // 淡红色
    lowConfFormat.setToolTip(tr("低置信度"));
    
    QTextCursor cursor(m_textEdit->document());
    
    for (const OcrEngine::TextRegion &region : m_result.regions) {
        if (region.confidence < m_confidenceThreshold) {
            // 查找文本位置（简化处理）
            cursor = m_textEdit->document()->find(region.text, cursor);
            if (!cursor.isNull()) {
                cursor.mergeCharFormat(lowConfFormat);
            }
        }
    }
}

inline void OcrProofreadWidget::rerecognizeSelection()
{
    if (!m_ocrEngine || !m_ocrEngine->isInitialized()) return;
    
    QTextCursor cursor = m_textEdit->textCursor();
    if (!cursor.hasSelection()) return;
    
    // 获取选中区域对应的图像区域
    // 简化实现：重新识别整个图像
    OcrEngine::OcrResult newResult = m_ocrEngine->recognize(m_sourceImage);
    
    if (newResult.success) {
        // 替换选中的文本
        cursor.insertText(newResult.fullText);
    }
}

inline void OcrProofreadWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // 工具栏
    QHBoxLayout *toolbarLayout = new QHBoxLayout();
    
    toolbarLayout->addWidget(new QLabel(tr("缩放:"), this));
    m_zoomSpin = new QSpinBox(this);
    m_zoomSpin->setRange(10, 500);
    m_zoomSpin->setValue(100);
    m_zoomSpin->setSuffix("%");
    connect(m_zoomSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &OcrProofreadWidget::onZoomChanged);
    toolbarLayout->addWidget(m_zoomSpin);
    
    m_highlightCheck = new QCheckBox(tr("高亮低置信度"), this);
    connect(m_highlightCheck, &QCheckBox::toggled,
            this, &OcrProofreadWidget::highlightLowConfidence);
    toolbarLayout->addWidget(m_highlightCheck);
    
    toolbarLayout->addStretch();
    
    m_rerecognizeBtn = new QPushButton(tr("重新识别选中区域"), this);
    m_rerecognizeBtn->setEnabled(false);
    connect(m_rerecognizeBtn, &QPushButton::clicked,
            this, &OcrProofreadWidget::rerecognizeSelection);
    toolbarLayout->addWidget(m_rerecognizeBtn);
    
    m_copyBtn = new QPushButton(tr("复制全部"), this);
    connect(m_copyBtn, &QPushButton::clicked,
            this, &OcrProofreadWidget::copyToClipboard);
    toolbarLayout->addWidget(m_copyBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // 分割器
    m_splitter = new QSplitter(Qt::Horizontal, this);
    
    // 图像视图
    m_scene = new QGraphicsScene(this);
    m_imageView = new QGraphicsView(m_scene, this);
    m_imageView->setRenderHint(QPainter::Antialiasing);
    m_imageView->setRenderHint(QPainter::SmoothPixmapTransform);
    m_imageView->setDragMode(QGraphicsView::ScrollHandDrag);
    m_splitter->addWidget(m_imageView);
    
    // 文本编辑器
    m_textEdit = new QPlainTextEdit(this);
    m_textEdit->setFont(QFont("Consolas", 11));
    m_textEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    connect(m_textEdit, &QPlainTextEdit::cursorPositionChanged,
            this, &OcrProofreadWidget::onTextCursorChanged);
    connect(m_textEdit, &QPlainTextEdit::textChanged,
            this, &OcrProofreadWidget::textChanged);
    m_splitter->addWidget(m_textEdit);
    
    m_splitter->setSizes({400, 400});
    mainLayout->addWidget(m_splitter, 1);
}

inline void OcrProofreadWidget::populateTextEdit()
{
    m_textEdit->setPlainText(m_result.fullText);
    
    // 建立行号到区域的映射
    m_lineToRegion.clear();
    int currentLine = 0;
    
    for (const OcrEngine::TextRegion &region : m_result.regions) {
        m_lineToRegion[currentLine] = region;
        currentLine += region.text.count('\n') + 1;
    }
}

inline void OcrProofreadWidget::highlightRegion(const QRect &region)
{
    clearHighlight();
    
    if (!region.isValid()) return;
    
    QPen pen(Qt::red, 2);
    QBrush brush(QColor(255, 0, 0, 50));
    
    m_highlightRect = m_scene->addRect(region, pen, brush);
    m_imageView->centerOn(region.center());
}

inline void OcrProofreadWidget::clearHighlight()
{
    if (m_highlightRect) {
        m_scene->removeItem(m_highlightRect);
        delete m_highlightRect;
        m_highlightRect = nullptr;
    }
}

inline void OcrProofreadWidget::onTextCursorChanged()
{
    int blockNumber = m_textEdit->textCursor().blockNumber();
    
    // 查找对应的区域
    for (auto it = m_lineToRegion.begin(); it != m_lineToRegion.end(); ++it) {
        if (it.key() <= blockNumber) {
            highlightRegion(it.value().boundingBox);
            return;
        }
    }
    
    clearHighlight();
}

inline void OcrProofreadWidget::onRegionClicked(const QRect &region)
{
    // 查找对应的文本行
    for (auto it = m_lineToRegion.begin(); it != m_lineToRegion.end(); ++it) {
        if (it.value().boundingBox == region) {
            QTextCursor cursor = m_textEdit->textCursor();
            cursor.movePosition(QTextCursor::Start);
            cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, it.key());
            m_textEdit->setTextCursor(cursor);
            m_textEdit->centerCursor();
            break;
        }
    }
}

inline void OcrProofreadWidget::onZoomChanged(int value)
{
    double factor = value / 100.0;
    m_imageView->resetTransform();
    m_imageView->scale(factor, factor);
}

inline void OcrProofreadWidget::syncScrollBars()
{
    // 同步滚动位置
}


// ==================== OcrImageView 实现 ====================

inline OcrImageView::OcrImageView(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
    , m_pixmapItem(nullptr)
    , m_highlightRect(nullptr)
    , m_zoomFactor(1.0)
{
    setScene(m_scene);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::ScrollHandDrag);
}

inline void OcrImageView::setImage(const QImage &image)
{
    m_scene->clear();
    m_pixmapItem = m_scene->addPixmap(QPixmap::fromImage(image));
    m_highlightRect = nullptr;
    setSceneRect(m_pixmapItem->boundingRect());
    fitInView(m_pixmapItem, Qt::KeepAspectRatio);
}

inline void OcrImageView::setRegions(const QVector<OcrEngine::TextRegion> &regions)
{
    m_regions = regions;
}

inline void OcrImageView::highlightRegion(const QRect &rect)
{
    clearHighlight();
    
    if (rect.isValid()) {
        QPen pen(Qt::red, 2);
        QBrush brush(QColor(255, 0, 0, 50));
        m_highlightRect = m_scene->addRect(rect, pen, brush);
    }
}

inline void OcrImageView::clearHighlight()
{
    if (m_highlightRect) {
        m_scene->removeItem(m_highlightRect);
        delete m_highlightRect;
        m_highlightRect = nullptr;
    }
}

inline void OcrImageView::setZoom(double factor)
{
    m_zoomFactor = factor;
    resetTransform();
    scale(factor, factor);
}

inline void OcrImageView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());
        
        for (const OcrEngine::TextRegion &region : m_regions) {
            if (region.boundingBox.contains(scenePos.toPoint())) {
                emit regionClicked(region.boundingBox);
                break;
            }
        }
    }
    
    QGraphicsView::mousePressEvent(event);
}

inline void OcrImageView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        double factor = event->angleDelta().y() > 0 ? 1.15 : 0.85;
        m_zoomFactor *= factor;
        scale(factor, factor);
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}


// ==================== OcrProofreadDialog 实现 ====================

inline OcrProofreadDialog::OcrProofreadDialog(const OcrEngine::OcrResult &result,
                                               const QImage &sourceImage,
                                               QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("OCR 结果校对"));
    setMinimumSize(900, 600);
    setupUI();
    
    m_proofreadWidget->setOcrResult(result, sourceImage);
}

inline QString OcrProofreadDialog::editedText() const
{
    return m_proofreadWidget->editedText();
}

inline void OcrProofreadDialog::setOcrEngine(OcrEngine *engine)
{
    m_proofreadWidget->setOcrEngine(engine);
}

inline void OcrProofreadDialog::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    m_proofreadWidget = new OcrProofreadWidget(this);
    layout->addWidget(m_proofreadWidget, 1);
    
    // 按钮栏
    QHBoxLayout *btnLayout = new QHBoxLayout();
    
    m_copyBtn = new QPushButton(tr("复制到剪贴板"), this);
    connect(m_copyBtn, &QPushButton::clicked, this, &OcrProofreadDialog::onCopy);
    btnLayout->addWidget(m_copyBtn);
    
    m_saveAsBtn = new QPushButton(tr("另存为..."), this);
    connect(m_saveAsBtn, &QPushButton::clicked, this, &OcrProofreadDialog::onSaveAs);
    btnLayout->addWidget(m_saveAsBtn);
    
    btnLayout->addStretch();
    
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &OcrProofreadDialog::onAccept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    btnLayout->addWidget(m_buttonBox);
    
    layout->addLayout(btnLayout);
}

inline void OcrProofreadDialog::onAccept()
{
    accept();
}

inline void OcrProofreadDialog::onCopy()
{
    m_proofreadWidget->copyToClipboard();
}

inline void OcrProofreadDialog::onSaveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("保存 OCR 结果"),
        QString(), tr("Markdown (*.md);;文本文件 (*.txt);;所有文件 (*)"));
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out.setEncoding(QStringConverter::Utf8);
            out << m_proofreadWidget->editedText();
        }
    }
}

#endif // OCRPROOFREAD_H

