// 文件说明：app-static\collaboration\remotecursoroverlay.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef REMOTECURSOROVERLAY_H
#define REMOTECURSOROVERLAY_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QMap>
#include <QTimer>
#include <QColor>
#include <QPainter>

namespace Collaboration {

/**
 * @brief Represents a remote user's cursor
 */
struct RemoteCursorInfo {
    QString userId;
    QString userName;
    int position;
    int selectionStart;
    int selectionEnd;
    QColor color;
    bool visible;
    qint64 lastUpdate;
};

/**
 * @brief Overlay widget to display remote users' cursors and selections
 *
 * This widget is placed over the text editor and draws the cursors
 * and selections of other collaborators in real-time.
 */
class RemoteCursorOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit RemoteCursorOverlay(QPlainTextEdit *editor, QWidget *parent = nullptr);
    ~RemoteCursorOverlay() = default;

    // Cursor management
    void updateCursor(const QString &userId, const QString &userName,
                      int position, const QColor &color);
    void updateSelection(const QString &userId, int start, int end);
    void removeCursor(const QString &userId);
    void clearAllCursors();

    // Visibility
    void setShowCursors(bool show);
    void setShowSelections(bool show);
    void setShowLabels(bool show);

    bool showCursors() const { return m_showCursors; }
    bool showSelections() const { return m_showSelections; }
    bool showLabels() const { return m_showLabels; }

public slots:
    void updateOverlay();

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void drawCursor(QPainter &painter, const RemoteCursorInfo &cursor);
    void drawSelection(QPainter &painter, const RemoteCursorInfo &cursor);
    void drawLabel(QPainter &painter, const RemoteCursorInfo &cursor, const QRect &cursorRect);

    QRect getCursorRect(int position) const;
    QVector<QRect> getSelectionRects(int start, int end) const;

    QPlainTextEdit *m_editor;
    QMap<QString, RemoteCursorInfo> m_cursors;
    QTimer *m_blinkTimer;

    bool m_showCursors;
    bool m_showSelections;
    bool m_showLabels;
    bool m_cursorVisible;

    static const int CursorWidth = 2;
    static const int LabelHeight = 18;
    static const int LabelPadding = 4;
    static const int CursorTimeout = 10000; // 10 seconds
};

} // namespace Collaboration

#endif // REMOTECURSOROVERLAY_H

