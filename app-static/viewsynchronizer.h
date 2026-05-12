// 文件说明：app-static\viewsynchronizer.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef VIEWSYNCHRONIZER_H
#define VIEWSYNCHRONIZER_H

#include <QObject>

class QWebEngineView;
class QPlainTextEdit;


class ViewSynchronizer : public QObject
{
    Q_OBJECT

public:
    ViewSynchronizer(QWebEngineView *webView, QPlainTextEdit *editor, QObject *parent = nullptr);
    virtual ~ViewSynchronizer() {}

protected:
    QWebEngineView *m_webView;
    QPlainTextEdit *m_editor;
};

#endif // VIEWSYNCHRONIZER_H

