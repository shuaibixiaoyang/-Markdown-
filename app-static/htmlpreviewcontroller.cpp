// 文件说明：app-static\htmlpreviewcontroller.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "htmlpreviewcontroller.h"

#include <QAction>
#include <QStandardPaths>
#include "compat/webenginecompat.h"

static const qreal ZOOM_CHANGE_VALUE = 0.1;

// 函数说明：构造 HtmlPreviewController 对象，初始化本模块需要的状态、界面和资源。
HtmlPreviewController::HtmlPreviewController(QWebEngineView *view, QObject *parent) :
    QObject(parent),
    view(view),
    zoomInAction(0),
    zoomOutAction(0),
    zoomResetAction(0)
{
    createActions();
    registerActionsWithView();

    // use registered actions as custom context menu
    view->setContextMenuPolicy(Qt::ActionsContextMenu);
}

// 函数说明：创建 HtmlPreviewController 需要的对象、记录或输出内容。
void HtmlPreviewController::createActions()
{
    zoomInAction = createAction(tr("Zoom &In"), QKeySequence(Qt::CTRL | Qt::Key_Plus));
    connect(zoomInAction, &QAction::triggered,
            this, &HtmlPreviewController::zoomInView);

    zoomOutAction = createAction(tr("Zoom &Out"), QKeySequence(Qt::CTRL | Qt::Key_Minus));
    connect(zoomOutAction, &QAction::triggered,
            this, &HtmlPreviewController::zoomOutView);

    zoomResetAction = createAction(tr("Reset &Zoom"), QKeySequence(Qt::CTRL | Qt::Key_0));
    connect(zoomResetAction, &QAction::triggered,
            this, &HtmlPreviewController::resetZoomOfView);
}

QAction *HtmlPreviewController::createAction(const QString &text, const QKeySequence &shortcut)
{
    QAction *action = new QAction(text, this);
    action->setShortcut(shortcut);
    return action;
}

// 函数说明：实现 HtmlPreviewController::registerActionsWithView 的核心逻辑，供当前模块调用。
void HtmlPreviewController::registerActionsWithView()
{
    view->addAction(view->pageAction(QWebEnginePage::Copy));
    view->addAction(zoomInAction);
    view->addAction(zoomOutAction);
    view->addAction(zoomResetAction);
}

// 函数说明：实现 HtmlPreviewController::zoomInView 的核心逻辑，供当前模块调用。
void HtmlPreviewController::zoomInView()
{
    view->setZoomFactor(view->zoomFactor() + ZOOM_CHANGE_VALUE);
}

// 函数说明：实现 HtmlPreviewController::zoomOutView 的核心逻辑，供当前模块调用。
void HtmlPreviewController::zoomOutView()
{
    view->setZoomFactor(view->zoomFactor() - ZOOM_CHANGE_VALUE);
}

// 函数说明：实现 HtmlPreviewController::resetZoomOfView 的核心逻辑，供当前模块调用。
void HtmlPreviewController::resetZoomOfView()
{
    view->setZoomFactor(1.0);
}


