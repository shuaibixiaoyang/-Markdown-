// 文件说明：app-static\ocr\captureeditor.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef CAPTUREEDITOR_H
#define CAPTUREEDITOR_H

#include <QWidget>
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QToolBar>
#include <QToolButton>
#include <QColorDialog>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QInputDialog>
#include <QFontDialog>
#include <QUndoStack>
#include <QUndoCommand>
#include <QGraphicsBlurEffect>
#include <QScrollArea>

/**
 * @brief 截图编辑器 - 完整实现
 * 
 * 功能：
 * - 矩形/椭圆/箭头标注
 * - 文字添加
 * - 画笔工具
 * - 马赛克/模糊
 * - 裁剪
 * - 撤销/重做
 * - 缩放
 */
class CaptureEditor : public QWidget
{
    Q_OBJECT

public:
    enum class Tool {
        None,
        Select,
        Rectangle,
        Ellipse,
        Arrow,
        Line,
        Text,
        Pen,
        Highlighter,
        Mosaic,
        Blur,
        Eraser,
        Crop
    };
    Q_ENUM(Tool)

    struct Annotation {
        Tool tool;
        QRect rect;
        QColor color;
        int lineWidth;
        QString text;
        QFont font;
        QVector<QPoint> points;
        bool filled;
        int id;
    };

    explicit CaptureEditor(const QPixmap &pixmap, QWidget *parent = nullptr);
    ~CaptureEditor();

    QPixmap editedPixmap() const;
    QPixmap originalPixmap() const { return m_originalPixmap; }
    
    void setCurrentTool(Tool tool);
    Tool currentTool() const { return m_currentTool; }
    
    void setCurrentColor(const QColor &color);
    QColor currentColor() const { return m_currentColor; }
    
    void setLineWidth(int width);
    int lineWidth() const { return m_lineWidth; }
    
    void setFont(const QFont &font);
    QFont currentFont() const { return m_currentFont; }
    
    void setFilled(bool filled);
    bool isFilled() const { return m_filled; }
    
    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;
    
    void zoomIn();
    void zoomOut();
    void zoomFit();
    void setZoom(double factor);
    double zoom() const { return m_zoomFactor; }
    
    void crop(const QRect &rect);
    void resetToOriginal();
    
    void save(const QString &path);

signals:
    void editingFinished(const QPixmap &result);
    void editingCanceled();
    void toolChanged(Tool tool);
    void colorChanged(const QColor &color);
    void modified();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void renderAnnotations(QPainter &painter);
    void drawAnnotation(QPainter &painter, const Annotation &ann);
    void drawArrow(QPainter &painter, const QPoint &start, const QPoint &end, int lineWidth);
    void applyMosaic(QImage &image, const QRect &rect, int blockSize = 10);
    void applyBlur(QImage &image, const QRect &rect, int radius = 5);
    QPoint mapToImage(const QPoint &pos) const;
    QRect mapToImage(const QRect &rect) const;
    QPoint mapFromImage(const QPoint &pos) const;
    void updateCanvas();
    void addAnnotation(const Annotation &ann);
    int nextAnnotationId();

    QPixmap m_originalPixmap;
    QImage m_workingImage;
    QPixmap m_displayPixmap;
    
    Tool m_currentTool;
    QColor m_currentColor;
    int m_lineWidth;
    QFont m_currentFont;
    bool m_filled;
    double m_zoomFactor;
    QPoint m_panOffset;
    
    QVector<Annotation> m_annotations;
    Annotation m_currentAnnotation;
    bool m_isDrawing;
    QPoint m_lastPoint;
    
    QUndoStack *m_undoStack;
    int m_annotationIdCounter;
    
    // 裁剪模式
    bool m_isCropping;
    QRect m_cropRect;
};


/**
 * @brief 编辑器工具栏
 */
class EditorToolBar : public QToolBar
{
    Q_OBJECT

public:
    explicit EditorToolBar(CaptureEditor *editor, QWidget *parent = nullptr);

private slots:
    void onToolButtonClicked();
    void onColorClicked();
    void onLineWidthChanged(int value);
    void onFontClicked();

private:
    CaptureEditor *m_editor;
    QMap<QToolButton*, CaptureEditor::Tool> m_toolButtons;
    QToolButton *m_colorButton;
    QSpinBox *m_lineWidthSpin;
};


/**
 * @brief 完整的截图编辑对话框
 */
class CaptureEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CaptureEditorDialog(const QPixmap &pixmap, QWidget *parent = nullptr);
    
    QPixmap result() const;

private slots:
    void onSave();
    void onCancel();

private:
    void setupUI();

    CaptureEditor *m_editor;
    EditorToolBar *m_toolbar;
    QPushButton *m_saveBtn;
    QPushButton *m_cancelBtn;
};


// ==================== CaptureEditor 实现 ====================

inline CaptureEditor::CaptureEditor(const QPixmap &pixmap, QWidget *parent)
    : QWidget(parent)
    , m_originalPixmap(pixmap)
    , m_workingImage(pixmap.toImage())
    , m_currentTool(Tool::None)
    , m_currentColor(Qt::red)
    , m_lineWidth(3)
    , m_currentFont(QFont("Arial", 14))
    , m_filled(false)
    , m_zoomFactor(1.0)
    , m_isDrawing(false)
    , m_undoStack(new QUndoStack(this))
    , m_annotationIdCounter(0)
    , m_isCropping(false)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    updateCanvas();
}

inline CaptureEditor::~CaptureEditor()
{
}

inline QPixmap CaptureEditor::editedPixmap() const
{
    QImage result = m_workingImage.copy();
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    
    for (const Annotation &ann : m_annotations) {
        const_cast<CaptureEditor*>(this)->drawAnnotation(painter, ann);
    }
    
    return QPixmap::fromImage(result);
}

inline void CaptureEditor::setCurrentTool(Tool tool)
{
    m_currentTool = tool;
    
    switch (tool) {
        case Tool::Crop:
            setCursor(Qt::CrossCursor);
            m_isCropping = true;
            break;
        case Tool::Pen:
        case Tool::Highlighter:
        case Tool::Eraser:
            setCursor(Qt::CrossCursor);
            m_isCropping = false;
            break;
        case Tool::Text:
            setCursor(Qt::IBeamCursor);
            m_isCropping = false;
            break;
        default:
            setCursor(Qt::ArrowCursor);
            m_isCropping = false;
    }
    
    emit toolChanged(tool);
}

inline void CaptureEditor::setCurrentColor(const QColor &color)
{
    m_currentColor = color;
    emit colorChanged(color);
}

inline void CaptureEditor::setLineWidth(int width)
{
    m_lineWidth = qBound(1, width, 50);
}

inline void CaptureEditor::setFont(const QFont &font)
{
    m_currentFont = font;
}

inline void CaptureEditor::setFilled(bool filled)
{
    m_filled = filled;
}

inline void CaptureEditor::undo()
{
    m_undoStack->undo();
    update();
}

inline void CaptureEditor::redo()
{
    m_undoStack->redo();
    update();
}

inline bool CaptureEditor::canUndo() const
{
    return m_undoStack->canUndo();
}

inline bool CaptureEditor::canRedo() const
{
    return m_undoStack->canRedo();
}

inline void CaptureEditor::zoomIn()
{
    setZoom(m_zoomFactor * 1.25);
}

inline void CaptureEditor::zoomOut()
{
    setZoom(m_zoomFactor / 1.25);
}

inline void CaptureEditor::zoomFit()
{
    double wRatio = static_cast<double>(width()) / m_workingImage.width();
    double hRatio = static_cast<double>(height()) / m_workingImage.height();
    setZoom(qMin(wRatio, hRatio) * 0.95);
}

inline void CaptureEditor::setZoom(double factor)
{
    m_zoomFactor = qBound(0.1, factor, 10.0);
    updateCanvas();
    update();
}

inline void CaptureEditor::crop(const QRect &rect)
{
    if (!rect.isValid()) return;
    
    m_workingImage = m_workingImage.copy(rect);
    m_annotations.clear();
    updateCanvas();
    emit modified();
}

inline void CaptureEditor::resetToOriginal()
{
    m_workingImage = m_originalPixmap.toImage();
    m_annotations.clear();
    m_undoStack->clear();
    updateCanvas();
    update();
}

inline void CaptureEditor::save(const QString &path)
{
    editedPixmap().save(path);
}

inline void CaptureEditor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 背景
    painter.fillRect(rect(), QColor(50, 50, 50));
    
    // 图像
    QRect imageRect(m_panOffset, m_displayPixmap.size());
    painter.drawPixmap(imageRect, m_displayPixmap);
    
    // 绘制标注
    painter.save();
    painter.translate(m_panOffset);
    painter.scale(m_zoomFactor, m_zoomFactor);
    
    for (const Annotation &ann : m_annotations) {
        drawAnnotation(painter, ann);
    }
    
    // 当前绘制中的标注
    if (m_isDrawing) {
        drawAnnotation(painter, m_currentAnnotation);
    }
    
    painter.restore();
    
    // 裁剪框
    if (m_isCropping && m_cropRect.isValid()) {
        painter.setPen(QPen(Qt::white, 1, Qt::DashLine));
        QRect displayCropRect(
            m_panOffset + m_cropRect.topLeft() * m_zoomFactor,
            m_cropRect.size() * m_zoomFactor
        );
        painter.drawRect(displayCropRect);
        
        // 暗化裁剪区域外
        QRegion outside = QRegion(rect()) - QRegion(displayCropRect);
        painter.setClipRegion(outside);
        painter.fillRect(rect(), QColor(0, 0, 0, 128));
    }
}

inline void CaptureEditor::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;
    
    QPoint imgPos = mapToImage(event->pos());
    m_lastPoint = imgPos;
    m_isDrawing = true;
    
    m_currentAnnotation = Annotation();
    m_currentAnnotation.tool = m_currentTool;
    m_currentAnnotation.color = m_currentColor;
    m_currentAnnotation.lineWidth = m_lineWidth;
    m_currentAnnotation.font = m_currentFont;
    m_currentAnnotation.filled = m_filled;
    m_currentAnnotation.id = nextAnnotationId();
    
    if (m_currentTool == Tool::Crop) {
        m_cropRect.setTopLeft(imgPos);
        m_cropRect.setBottomRight(imgPos);
    } else if (m_currentTool == Tool::Text) {
        bool ok;
        QString text = QInputDialog::getText(this, tr("添加文字"), 
                                              tr("输入文字:"), QLineEdit::Normal, "", &ok);
        if (ok && !text.isEmpty()) {
            m_currentAnnotation.text = text;
            m_currentAnnotation.rect = QRect(imgPos, QSize(200, 50));
            addAnnotation(m_currentAnnotation);
        }
        m_isDrawing = false;
    } else if (m_currentTool == Tool::Pen || m_currentTool == Tool::Highlighter || 
               m_currentTool == Tool::Eraser) {
        m_currentAnnotation.points.append(imgPos);
    } else {
        m_currentAnnotation.rect.setTopLeft(imgPos);
    }
    
    update();
}

inline void CaptureEditor::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_isDrawing) return;
    
    QPoint imgPos = mapToImage(event->pos());
    
    if (m_currentTool == Tool::Crop) {
        m_cropRect.setBottomRight(imgPos);
    } else if (m_currentTool == Tool::Pen || m_currentTool == Tool::Highlighter || 
               m_currentTool == Tool::Eraser) {
        m_currentAnnotation.points.append(imgPos);
    } else {
        m_currentAnnotation.rect.setBottomRight(imgPos);
    }
    
    m_lastPoint = imgPos;
    update();
}

inline void CaptureEditor::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_isDrawing) return;
    
    m_isDrawing = false;
    
    if (m_currentTool == Tool::Crop) {
        m_cropRect = m_cropRect.normalized();
        if (m_cropRect.width() > 10 && m_cropRect.height() > 10) {
            crop(m_cropRect);
        }
        m_cropRect = QRect();
        m_isCropping = false;
        setCurrentTool(Tool::None);
    } else if (m_currentTool == Tool::Mosaic || m_currentTool == Tool::Blur) {
        QRect rect = m_currentAnnotation.rect.normalized();
        if (rect.isValid()) {
            if (m_currentTool == Tool::Mosaic) {
                applyMosaic(m_workingImage, rect);
            } else {
                applyBlur(m_workingImage, rect);
            }
            updateCanvas();
            emit modified();
        }
    } else if (m_currentTool != Tool::None && m_currentTool != Tool::Text) {
        m_currentAnnotation.rect = m_currentAnnotation.rect.normalized();
        if (m_currentAnnotation.rect.isValid() || !m_currentAnnotation.points.isEmpty()) {
            addAnnotation(m_currentAnnotation);
        }
    }
    
    update();
}

inline void CaptureEditor::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
        event->accept();
    }
}

inline void CaptureEditor::keyPressEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::Undo)) {
        undo();
    } else if (event->matches(QKeySequence::Redo)) {
        redo();
    } else if (event->key() == Qt::Key_Escape) {
        if (m_isCropping) {
            m_isCropping = false;
            m_cropRect = QRect();
            setCurrentTool(Tool::None);
            update();
        } else {
            emit editingCanceled();
        }
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emit editingFinished(editedPixmap());
    }
}

inline void CaptureEditor::resizeEvent(QResizeEvent *)
{
    // 居中图像
    m_panOffset = QPoint(
        (width() - m_displayPixmap.width()) / 2,
        (height() - m_displayPixmap.height()) / 2
    );
}

inline void CaptureEditor::drawAnnotation(QPainter &painter, const Annotation &ann)
{
    painter.save();
    
    QPen pen(ann.color, ann.lineWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    
    if (ann.filled) {
        QColor fillColor = ann.color;
        fillColor.setAlpha(50);
        painter.setBrush(fillColor);
    } else {
        painter.setBrush(Qt::NoBrush);
    }
    
    switch (ann.tool) {
        case Tool::Rectangle:
            painter.drawRect(ann.rect);
            break;
            
        case Tool::Ellipse:
            painter.drawEllipse(ann.rect);
            break;
            
        case Tool::Arrow:
        case Tool::Line:
            if (ann.tool == Tool::Arrow) {
                drawArrow(painter, ann.rect.topLeft(), ann.rect.bottomRight(), ann.lineWidth);
            } else {
                painter.drawLine(ann.rect.topLeft(), ann.rect.bottomRight());
            }
            break;
            
        case Tool::Text:
            painter.setFont(ann.font);
            painter.drawText(ann.rect, Qt::AlignLeft | Qt::TextWordWrap, ann.text);
            break;
            
        case Tool::Pen:
        case Tool::Eraser:
            if (ann.points.size() > 1) {
                if (ann.tool == Tool::Eraser) {
                    pen.setColor(Qt::white);
                    painter.setPen(pen);
                }
                for (int i = 1; i < ann.points.size(); ++i) {
                    painter.drawLine(ann.points[i-1], ann.points[i]);
                }
            }
            break;
            
        case Tool::Highlighter:
            if (ann.points.size() > 1) {
                QColor hlColor = ann.color;
                hlColor.setAlpha(80);
                QPen hlPen(hlColor, ann.lineWidth * 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                painter.setPen(hlPen);
                for (int i = 1; i < ann.points.size(); ++i) {
                    painter.drawLine(ann.points[i-1], ann.points[i]);
                }
            }
            break;
            
        default:
            break;
    }
    
    painter.restore();
}

inline void CaptureEditor::drawArrow(QPainter &painter, const QPoint &start, const QPoint &end, int lineWidth)
{
    painter.drawLine(start, end);
    
    // 箭头头部
    double angle = std::atan2(end.y() - start.y(), end.x() - start.x());
    double arrowSize = lineWidth * 4;
    
    QPointF p1(end.x() - arrowSize * std::cos(angle - M_PI / 6),
               end.y() - arrowSize * std::sin(angle - M_PI / 6));
    QPointF p2(end.x() - arrowSize * std::cos(angle + M_PI / 6),
               end.y() - arrowSize * std::sin(angle + M_PI / 6));
    
    QPolygonF arrowHead;
    arrowHead << end << p1 << p2;
    
    painter.save();
    painter.setBrush(painter.pen().color());
    painter.drawPolygon(arrowHead);
    painter.restore();
}

inline void CaptureEditor::applyMosaic(QImage &image, const QRect &rect, int blockSize)
{
    QRect r = rect.intersected(image.rect());
    
    for (int y = r.top(); y < r.bottom(); y += blockSize) {
        for (int x = r.left(); x < r.right(); x += blockSize) {
            // 计算块平均颜色
            int sumR = 0, sumG = 0, sumB = 0, count = 0;
            for (int dy = 0; dy < blockSize && y + dy < r.bottom(); ++dy) {
                for (int dx = 0; dx < blockSize && x + dx < r.right(); ++dx) {
                    QColor c = image.pixelColor(x + dx, y + dy);
                    sumR += c.red();
                    sumG += c.green();
                    sumB += c.blue();
                    count++;
                }
            }
            
            if (count > 0) {
                QColor avgColor(sumR / count, sumG / count, sumB / count);
                for (int dy = 0; dy < blockSize && y + dy < r.bottom(); ++dy) {
                    for (int dx = 0; dx < blockSize && x + dx < r.right(); ++dx) {
                        image.setPixelColor(x + dx, y + dy, avgColor);
                    }
                }
            }
        }
    }
}

inline void CaptureEditor::applyBlur(QImage &image, const QRect &rect, int radius)
{
    QRect r = rect.intersected(image.rect());
    QImage region = image.copy(r);
    
    // 简单的盒式模糊
    QImage blurred = region.copy();
    
    for (int pass = 0; pass < 3; ++pass) {
        // 水平方向
        for (int y = 0; y < region.height(); ++y) {
            for (int x = 0; x < region.width(); ++x) {
                int sumR = 0, sumG = 0, sumB = 0, count = 0;
                for (int dx = -radius; dx <= radius; ++dx) {
                    int nx = qBound(0, x + dx, region.width() - 1);
                    QColor c = blurred.pixelColor(nx, y);
                    sumR += c.red();
                    sumG += c.green();
                    sumB += c.blue();
                    count++;
                }
                region.setPixelColor(x, y, QColor(sumR / count, sumG / count, sumB / count));
            }
        }
        
        // 垂直方向
        for (int y = 0; y < region.height(); ++y) {
            for (int x = 0; x < region.width(); ++x) {
                int sumR = 0, sumG = 0, sumB = 0, count = 0;
                for (int dy = -radius; dy <= radius; ++dy) {
                    int ny = qBound(0, y + dy, region.height() - 1);
                    QColor c = region.pixelColor(x, ny);
                    sumR += c.red();
                    sumG += c.green();
                    sumB += c.blue();
                    count++;
                }
                blurred.setPixelColor(x, y, QColor(sumR / count, sumG / count, sumB / count));
            }
        }
    }
    
    // 复制回原图
    for (int y = 0; y < blurred.height(); ++y) {
        for (int x = 0; x < blurred.width(); ++x) {
            image.setPixelColor(r.left() + x, r.top() + y, blurred.pixelColor(x, y));
        }
    }
}

inline QPoint CaptureEditor::mapToImage(const QPoint &pos) const
{
    return (pos - m_panOffset) / m_zoomFactor;
}

inline QRect CaptureEditor::mapToImage(const QRect &rect) const
{
    return QRect(mapToImage(rect.topLeft()), mapToImage(rect.bottomRight()));
}

inline QPoint CaptureEditor::mapFromImage(const QPoint &pos) const
{
    return pos * m_zoomFactor + m_panOffset;
}

inline void CaptureEditor::updateCanvas()
{
    m_displayPixmap = QPixmap::fromImage(m_workingImage).scaled(
        m_workingImage.size() * m_zoomFactor,
        Qt::KeepAspectRatio, Qt::SmoothTransformation
    );
    
    m_panOffset = QPoint(
        (width() - m_displayPixmap.width()) / 2,
        (height() - m_displayPixmap.height()) / 2
    );
}

inline void CaptureEditor::addAnnotation(const Annotation &ann)
{
    m_annotations.append(ann);
    emit modified();
}

inline int CaptureEditor::nextAnnotationId()
{
    return ++m_annotationIdCounter;
}


// ==================== EditorToolBar 实现 ====================

inline EditorToolBar::EditorToolBar(CaptureEditor *editor, QWidget *parent)
    : QToolBar(parent)
    , m_editor(editor)
{
    setIconSize(QSize(24, 24));
    
    // 工具按钮
    struct ToolInfo {
        CaptureEditor::Tool tool;
        QString icon;
        QString tip;
    };
    
    QVector<ToolInfo> tools = {
        {CaptureEditor::Tool::Select, "fa-mouse-pointer", tr("选择")},
        {CaptureEditor::Tool::Rectangle, "fa-square-o", tr("矩形")},
        {CaptureEditor::Tool::Ellipse, "fa-circle-o", tr("椭圆")},
        {CaptureEditor::Tool::Arrow, "fa-long-arrow-right", tr("箭头")},
        {CaptureEditor::Tool::Line, "fa-minus", tr("直线")},
        {CaptureEditor::Tool::Text, "fa-font", tr("文字")},
        {CaptureEditor::Tool::Pen, "fa-pencil", tr("画笔")},
        {CaptureEditor::Tool::Highlighter, "fa-paint-brush", tr("荧光笔")},
        {CaptureEditor::Tool::Mosaic, "fa-th", tr("马赛克")},
        {CaptureEditor::Tool::Blur, "fa-adjust", tr("模糊")},
        {CaptureEditor::Tool::Eraser, "fa-eraser", tr("橡皮擦")},
        {CaptureEditor::Tool::Crop, "fa-crop", tr("裁剪")}
    };
    
    for (const ToolInfo &info : tools) {
        QToolButton *btn = new QToolButton(this);
        btn->setIcon(QIcon(info.icon + ".fontawesome"));
        btn->setToolTip(info.tip);
        btn->setCheckable(true);
        m_toolButtons[btn] = info.tool;
        connect(btn, &QToolButton::clicked, this, &EditorToolBar::onToolButtonClicked);
        addWidget(btn);
    }
    
    addSeparator();
    
    // 颜色按钮
    m_colorButton = new QToolButton(this);
    m_colorButton->setToolTip(tr("颜色"));
    QPixmap colorPix(24, 24);
    colorPix.fill(m_editor->currentColor());
    m_colorButton->setIcon(QIcon(colorPix));
    connect(m_colorButton, &QToolButton::clicked, this, &EditorToolBar::onColorClicked);
    addWidget(m_colorButton);
    
    // 线宽
    addWidget(new QLabel(tr("线宽:"), this));
    m_lineWidthSpin = new QSpinBox(this);
    m_lineWidthSpin->setRange(1, 20);
    m_lineWidthSpin->setValue(m_editor->lineWidth());
    connect(m_lineWidthSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &EditorToolBar::onLineWidthChanged);
    addWidget(m_lineWidthSpin);
    
    addSeparator();
    
    // 撤销/重做
    QAction *undoAction = addAction(QIcon("fa-undo.fontawesome"), tr("撤销"));
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, m_editor, &CaptureEditor::undo);
    
    QAction *redoAction = addAction(QIcon("fa-repeat.fontawesome"), tr("重做"));
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, m_editor, &CaptureEditor::redo);
    
    addSeparator();
    
    // 缩放
    QAction *zoomInAction = addAction(QIcon("fa-search-plus.fontawesome"), tr("放大"));
    connect(zoomInAction, &QAction::triggered, m_editor, &CaptureEditor::zoomIn);
    
    QAction *zoomOutAction = addAction(QIcon("fa-search-minus.fontawesome"), tr("缩小"));
    connect(zoomOutAction, &QAction::triggered, m_editor, &CaptureEditor::zoomOut);
    
    QAction *zoomFitAction = addAction(QIcon("fa-arrows-alt.fontawesome"), tr("适应窗口"));
    connect(zoomFitAction, &QAction::triggered, m_editor, &CaptureEditor::zoomFit);
}

inline void EditorToolBar::onToolButtonClicked()
{
    QToolButton *btn = qobject_cast<QToolButton*>(sender());
    if (!btn) return;
    
    // 取消其他按钮选中
    for (auto it = m_toolButtons.begin(); it != m_toolButtons.end(); ++it) {
        if (it.key() != btn) {
            it.key()->setChecked(false);
        }
    }
    
    m_editor->setCurrentTool(m_toolButtons[btn]);
}

inline void EditorToolBar::onColorClicked()
{
    QColor color = QColorDialog::getColor(m_editor->currentColor(), this, tr("选择颜色"));
    if (color.isValid()) {
        m_editor->setCurrentColor(color);
        QPixmap pix(24, 24);
        pix.fill(color);
        m_colorButton->setIcon(QIcon(pix));
    }
}

inline void EditorToolBar::onLineWidthChanged(int value)
{
    m_editor->setLineWidth(value);
}

inline void EditorToolBar::onFontClicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_editor->currentFont(), this, tr("选择字体"));
    if (ok) {
        m_editor->setFont(font);
    }
}


// ==================== CaptureEditorDialog 实现 ====================

inline CaptureEditorDialog::CaptureEditorDialog(const QPixmap &pixmap, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("编辑截图"));
    setMinimumSize(800, 600);
    setupUI();
    m_editor = new CaptureEditor(pixmap, this);
    
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(this->layout());
    layout->insertWidget(1, m_editor, 1);
    
    m_toolbar = new EditorToolBar(m_editor, this);
    layout->insertWidget(0, m_toolbar);
}

inline QPixmap CaptureEditorDialog::result() const
{
    return m_editor->editedPixmap();
}

inline void CaptureEditorDialog::onSave()
{
    accept();
}

inline void CaptureEditorDialog::onCancel()
{
    reject();
}

inline void CaptureEditorDialog::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // 底部按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    m_cancelBtn = new QPushButton(tr("取消"), this);
    connect(m_cancelBtn, &QPushButton::clicked, this, &CaptureEditorDialog::onCancel);
    btnLayout->addWidget(m_cancelBtn);
    
    m_saveBtn = new QPushButton(tr("保存"), this);
    m_saveBtn->setDefault(true);
    connect(m_saveBtn, &QPushButton::clicked, this, &CaptureEditorDialog::onSave);
    btnLayout->addWidget(m_saveBtn);
    
    layout->addLayout(btnLayout);
}

#endif // CAPTUREEDITOR_H

