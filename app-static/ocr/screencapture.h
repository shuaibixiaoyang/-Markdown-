// 文件说明：app-static\ocr\screencapture.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef SCREENCAPTURE_H
#define SCREENCAPTURE_H

#include <QObject>
#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QRubberBand>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QScreen>
#include <QGuiApplication>
#include <QTimer>
#include <QClipboard>
#include <QDateTime>

/**
 * @brief 截图工具
 * 
 * 功能：
 * - 全屏/区域截图
 * - 延时截图
 * - 窗口截图
 * - 截图编辑（标注、马赛克）
 * - 自动复制到剪贴板
 */
class ScreenCapture : public QWidget
{
    Q_OBJECT

public:
    // 截图模式
    enum class CaptureMode {
        FullScreen,     // 全屏
        Region,         // 区域选择
        Window,         // 窗口
        Delayed         // 延时截图
    };
    Q_ENUM(CaptureMode)

    // 截图结果
    struct CaptureResult {
        QPixmap pixmap;
        QRect region;
        QDateTime timestamp;
        QString suggestedName;
        bool success;
    };

    explicit ScreenCapture(QWidget *parent = nullptr);
    ~ScreenCapture();

    // 开始截图
    void startCapture(CaptureMode mode = CaptureMode::Region);
    
    // 延时截图
    void startDelayedCapture(int delaySeconds, CaptureMode mode = CaptureMode::Region);

    // 直接截取全屏
    static QPixmap captureFullScreen();
    
    // 截取指定区域
    static QPixmap captureRegion(const QRect &region);
    
    // 截取窗口
    static QPixmap captureWindow(WId windowId);

    // 获取最后截图结果
    CaptureResult lastResult() const { return m_lastResult; }

signals:
    void captureCompleted(const CaptureResult &result);
    void captureCanceled();
    void captureStarted();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onDelayTimeout();

private:
    void showCaptureOverlay();
    void hideCaptureOverlay();
    void finishCapture();
    void updateMagnifier(const QPoint &pos);
    QRect normalizedRect(const QPoint &p1, const QPoint &p2) const;

    QPixmap m_screenPixmap;
    QPoint m_startPoint;
    QPoint m_endPoint;
    bool m_isSelecting;
    CaptureMode m_mode;
    CaptureResult m_lastResult;
    QTimer *m_delayTimer;
    int m_delayRemaining;
    
    // 放大镜
    bool m_showMagnifier;
    QPoint m_magnifierPos;
    static const int MAGNIFIER_SIZE = 120;
    static const int MAGNIFIER_ZOOM = 2;
};


/**
 * @brief 截图编辑器
 * 
 * 功能：
 * - 矩形/椭圆标注
 * - 箭头
 * - 文字
 * - 马赛克/模糊
 * - 画笔
 * - 裁剪
 */
class CaptureEditor : public QWidget
{
    Q_OBJECT

public:
    enum class Tool {
        None,
        Rectangle,
        Ellipse,
        Arrow,
        Text,
        Pen,
        Mosaic,
        Blur,
        Crop
    };
    Q_ENUM(Tool)

    struct Annotation {
        Tool tool;
        QRect rect;
        QColor color;
        int lineWidth;
        QString text;
        QVector<QPoint> points;  // for pen
    };

    explicit CaptureEditor(const QPixmap &pixmap, QWidget *parent = nullptr);

    QPixmap editedPixmap() const;
    
    void setCurrentTool(Tool tool);
    void setCurrentColor(const QColor &color);
    void setLineWidth(int width);
    void undo();
    void redo();

signals:
    void editingFinished(const QPixmap &result);
    void editingCanceled();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void applyMosaic(QPainter &painter, const QRect &rect);
    void applyBlur(QPainter &painter, const QRect &rect);
    void drawArrow(QPainter &painter, const QPoint &start, const QPoint &end);
    void renderAnnotations(QPainter &painter);

    QPixmap m_originalPixmap;
    QPixmap m_currentPixmap;
    Tool m_currentTool;
    QColor m_currentColor;
    int m_lineWidth;
    
    QVector<Annotation> m_annotations;
    QVector<Annotation> m_redoStack;
    Annotation m_currentAnnotation;
    bool m_isDrawing;
};


// ==================== ScreenCapture 实现 ====================

inline ScreenCapture::ScreenCapture(QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
    , m_isSelecting(false)
    , m_mode(CaptureMode::Region)
    , m_delayTimer(new QTimer(this))
    , m_delayRemaining(0)
    , m_showMagnifier(true)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    
    connect(m_delayTimer, &QTimer::timeout, this, &ScreenCapture::onDelayTimeout);
}

inline ScreenCapture::~ScreenCapture()
{
}

inline void ScreenCapture::startCapture(CaptureMode mode)
{
    m_mode = mode;
    
    if (mode == CaptureMode::FullScreen) {
        m_lastResult.pixmap = captureFullScreen();
        m_lastResult.region = m_lastResult.pixmap.rect();
        m_lastResult.timestamp = QDateTime::currentDateTime();
        m_lastResult.success = !m_lastResult.pixmap.isNull();
        emit captureCompleted(m_lastResult);
        return;
    }
    
    showCaptureOverlay();
}

inline void ScreenCapture::startDelayedCapture(int delaySeconds, CaptureMode mode)
{
    m_mode = mode;
    m_delayRemaining = delaySeconds;
    m_delayTimer->start(1000);
}

inline QPixmap ScreenCapture::captureFullScreen()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        return screen->grabWindow(0);
    }
    return QPixmap();
}

inline QPixmap ScreenCapture::captureRegion(const QRect &region)
{
    QPixmap full = captureFullScreen();
    if (!full.isNull() && region.isValid()) {
        return full.copy(region);
    }
    return full;
}

inline QPixmap ScreenCapture::captureWindow(WId windowId)
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        return screen->grabWindow(windowId);
    }
    return QPixmap();
}

inline void ScreenCapture::showCaptureOverlay()
{
    // 截取当前屏幕作为背景
    m_screenPixmap = captureFullScreen();
    
    // 全屏显示
    QScreen *screen = QGuiApplication::primaryScreen();
    setGeometry(screen->geometry());
    
    m_isSelecting = false;
    m_startPoint = QPoint();
    m_endPoint = QPoint();
    
    show();
    raise();
    activateWindow();
    
    emit captureStarted();
}

inline void ScreenCapture::hideCaptureOverlay()
{
    hide();
}

inline void ScreenCapture::finishCapture()
{
    QRect selectedRect = normalizedRect(m_startPoint, m_endPoint);
    
    if (selectedRect.width() > 5 && selectedRect.height() > 5) {
        m_lastResult.pixmap = m_screenPixmap.copy(selectedRect);
        m_lastResult.region = selectedRect;
        m_lastResult.timestamp = QDateTime::currentDateTime();
        m_lastResult.success = true;
        
        // 复制到剪贴板
        QGuiApplication::clipboard()->setPixmap(m_lastResult.pixmap);
        
        emit captureCompleted(m_lastResult);
    } else {
        emit captureCanceled();
    }
    
    hideCaptureOverlay();
}

inline void ScreenCapture::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    
    // 绘制截屏背景
    painter.drawPixmap(0, 0, m_screenPixmap);
    
    // 绘制半透明遮罩
    painter.fillRect(rect(), QColor(0, 0, 0, 100));
    
    // 绘制选中区域（清除遮罩）
    if (m_isSelecting || (!m_startPoint.isNull() && !m_endPoint.isNull())) {
        QRect selectedRect = normalizedRect(m_startPoint, m_endPoint);
        
        // 显示原始图像
        painter.drawPixmap(selectedRect, m_screenPixmap, selectedRect);
        
        // 绘制选中区域边框
        painter.setPen(QPen(QColor(0, 174, 255), 2));
        painter.drawRect(selectedRect);
        
        // 绘制尺寸信息
        QString sizeText = QString("%1 x %2").arg(selectedRect.width()).arg(selectedRect.height());
        QRect textRect = selectedRect;
        textRect.setHeight(20);
        textRect.moveTop(selectedRect.top() - 25);
        
        painter.setPen(Qt::white);
        painter.fillRect(textRect.adjusted(-5, 0, 5, 0), QColor(0, 0, 0, 180));
        painter.drawText(textRect, Qt::AlignCenter, sizeText);
        
        // 绘制控制点
        int pointSize = 8;
        QColor pointColor(0, 174, 255);
        painter.setBrush(pointColor);
        painter.setPen(Qt::white);
        
        // 四角和四边中点
        QVector<QPoint> controlPoints = {
            selectedRect.topLeft(),
            selectedRect.topRight(),
            selectedRect.bottomLeft(),
            selectedRect.bottomRight(),
            QPoint(selectedRect.center().x(), selectedRect.top()),
            QPoint(selectedRect.center().x(), selectedRect.bottom()),
            QPoint(selectedRect.left(), selectedRect.center().y()),
            QPoint(selectedRect.right(), selectedRect.center().y())
        };
        
        for (const QPoint &p : controlPoints) {
            painter.drawRect(p.x() - pointSize/2, p.y() - pointSize/2, pointSize, pointSize);
        }
    }
    
    // 绘制放大镜
    if (m_showMagnifier && m_isSelecting) {
        updateMagnifier(m_endPoint);
        
        QRect magRect(m_magnifierPos.x() + 20, m_magnifierPos.y() + 20, 
                      MAGNIFIER_SIZE, MAGNIFIER_SIZE);
        
        // 确保放大镜在屏幕内
        if (magRect.right() > width()) {
            magRect.moveLeft(m_magnifierPos.x() - MAGNIFIER_SIZE - 20);
        }
        if (magRect.bottom() > height()) {
            magRect.moveTop(m_magnifierPos.y() - MAGNIFIER_SIZE - 20);
        }
        
        // 放大镜背景
        painter.fillRect(magRect, Qt::white);
        painter.setPen(QPen(Qt::gray, 1));
        painter.drawRect(magRect);
        
        // 放大的图像
        int srcSize = MAGNIFIER_SIZE / MAGNIFIER_ZOOM;
        QRect srcRect(m_magnifierPos.x() - srcSize/2, m_magnifierPos.y() - srcSize/2,
                      srcSize, srcSize);
        painter.drawPixmap(magRect, m_screenPixmap, srcRect);
        
        // 十字线
        painter.setPen(QPen(QColor(0, 174, 255), 1));
        painter.drawLine(magRect.left(), magRect.center().y(),
                        magRect.right(), magRect.center().y());
        painter.drawLine(magRect.center().x(), magRect.top(),
                        magRect.center().x(), magRect.bottom());
        
        // 颜色信息
        QColor pixelColor = m_screenPixmap.toImage().pixelColor(m_magnifierPos);
        QString colorInfo = QString("RGB: %1, %2, %3")
            .arg(pixelColor.red()).arg(pixelColor.green()).arg(pixelColor.blue());
        
        QRect colorRect = magRect;
        colorRect.setTop(magRect.bottom() + 2);
        colorRect.setHeight(18);
        painter.fillRect(colorRect, QColor(0, 0, 0, 180));
        painter.setPen(Qt::white);
        painter.drawText(colorRect, Qt::AlignCenter, colorInfo);
    }
    
    // 提示信息
    painter.setPen(Qt::white);
    painter.drawText(10, 30, tr("按住鼠标拖动选择区域，ESC 取消，Enter 确认"));
}

inline void ScreenCapture::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isSelecting = true;
        m_startPoint = event->pos();
        m_endPoint = event->pos();
        update();
    }
}

inline void ScreenCapture::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isSelecting) {
        m_endPoint = event->pos();
        m_magnifierPos = event->pos();
        update();
    }
}

inline void ScreenCapture::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_isSelecting) {
        m_isSelecting = false;
        m_endPoint = event->pos();
        update();
    }
}

inline void ScreenCapture::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit captureCanceled();
        hideCaptureOverlay();
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        finishCapture();
    }
}

inline void ScreenCapture::onDelayTimeout()
{
    m_delayRemaining--;
    
    if (m_delayRemaining <= 0) {
        m_delayTimer->stop();
        
        if (m_mode == CaptureMode::FullScreen) {
            m_lastResult.pixmap = captureFullScreen();
            m_lastResult.region = m_lastResult.pixmap.rect();
            m_lastResult.timestamp = QDateTime::currentDateTime();
            m_lastResult.success = !m_lastResult.pixmap.isNull();
            emit captureCompleted(m_lastResult);
        } else {
            showCaptureOverlay();
        }
    }
}

inline void ScreenCapture::updateMagnifier(const QPoint &pos)
{
    m_magnifierPos = pos;
}

inline QRect ScreenCapture::normalizedRect(const QPoint &p1, const QPoint &p2) const
{
    return QRect(qMin(p1.x(), p2.x()), qMin(p1.y(), p2.y()),
                 qAbs(p2.x() - p1.x()), qAbs(p2.y() - p1.y()));
}

#endif // SCREENCAPTURE_H

