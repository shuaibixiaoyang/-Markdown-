// 文件说明：app-static\viewsynchronizer.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "viewsynchronizer.h"

#include "compat/webenginecompat.h"

// 函数说明：构造 ViewSynchronizer 对象，初始化本模块需要的状态、界面和资源。
ViewSynchronizer::ViewSynchronizer(QWebEngineView *webView, QPlainTextEdit *editor, QObject *parent) :
    QObject(parent),
    m_webView(webView),
    m_editor(editor)
{
}

