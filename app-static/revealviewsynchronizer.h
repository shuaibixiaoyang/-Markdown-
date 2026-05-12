// 文件说明：app-static\revealviewsynchronizer.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef REVEALVIEWSYNCHRONIZER_H
#define REVEALVIEWSYNCHRONIZER_H

#include "viewsynchronizer.h"

#include <QPair>

class SlideLineMapping;


class RevealViewSynchronizer : public ViewSynchronizer
{
    Q_OBJECT
    Q_PROPERTY(int horizontalSlide READ horizontalSlide NOTIFY gotoSlideRequested)
    Q_PROPERTY(int verticalSlide READ verticalSlide NOTIFY gotoSlideRequested)

public:
    RevealViewSynchronizer(QWebEngineView *webView, QPlainTextEdit *editor);
    ~RevealViewSynchronizer();

    int horizontalSlide() const;
    int verticalSlide() const;

signals:
    void gotoSlideRequested(int horizontal, int vertical);

public slots:
    void slideChanged(int horizontal, int vertical);

private slots:
    void registerEvents();
    void restoreSlidePosition();
    void cursorPositionChanged();
    void textChanged();

private:
    void gotoLine(int lineNumber);
    void gotoSlide(QPair<int, int> slide);

private:
    QPair<int, int> currentSlide;
    SlideLineMapping *slideLineMapping;
};

#endif // REVEALVIEWSYNCHRONIZER_H


