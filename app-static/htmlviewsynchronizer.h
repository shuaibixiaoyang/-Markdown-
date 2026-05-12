// 文件说明：app-static\htmlviewsynchronizer.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef HTMLVIEWSYNCHRONIZER_H
#define HTMLVIEWSYNCHRONIZER_H

#include "viewsynchronizer.h"


class HtmlViewSynchronizer : public ViewSynchronizer
{
    Q_OBJECT

public:
    HtmlViewSynchronizer(QWebEngineView *webView, QPlainTextEdit *editor);
    ~HtmlViewSynchronizer();

public slots:
    void webViewScrolled();
    void rememberScrollBarPos();

private slots:
    void scrollValueChanged(int value);
    void htmlContentSizeChanged();

private:
    int scrollBarPos;
};

#endif // HTMLVIEWSYNCHRONIZER_H


