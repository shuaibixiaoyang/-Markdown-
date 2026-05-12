// 文件说明：app-static\htmlpreviewcontroller.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef HTMLPREVIEWCONTROLLER_H
#define HTMLPREVIEWCONTROLLER_H

#include <QObject>

class QAction;
class QWebEngineView;

class HtmlPreviewController : public QObject
{
    Q_OBJECT
public:
    explicit HtmlPreviewController(QWebEngineView *view, QObject *parent = nullptr);

public slots:
    void zoomInView();
    void zoomOutView();
    void resetZoomOfView();

private:
    void createActions();
    QAction *createAction(const QString &text, const QKeySequence &shortcut);
    void registerActionsWithView();

private:
    QWebEngineView *view;
    QAction *zoomInAction;
    QAction *zoomOutAction;
    QAction *zoomResetAction;
};

#endif // HTMLPREVIEWCONTROLLER_H


