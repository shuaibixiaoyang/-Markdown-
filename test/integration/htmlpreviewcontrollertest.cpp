// 文件说明：test\integration\htmlpreviewcontrollertest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "htmlpreviewcontrollertest.h"

#include <QtTest>
#include <QGuiApplication>
#include <QScreen>
#include "compat/webenginecompat.h"

#include "htmlpreviewcontroller.h"

// 函数说明：实现 HtmlPreviewControllerTest::initTestCase 的核心逻辑，供当前模块调用。
void HtmlPreviewControllerTest::initTestCase()
{
    const QString platform = QGuiApplication::platformName();
    if (platform == QStringLiteral("offscreen") ||
        platform == QStringLiteral("minimal") ||
        platform == QStringLiteral("minimalegl")) {
        QSKIP("QWebEngineView is not supported on headless Qt platform plugins.");
    }

    if (QGuiApplication::primaryScreen() == nullptr) {
        QSKIP("No screen available for QWebEngineView integration test.");
    }

    webView = new QWebEngineView();
    controller = new HtmlPreviewController(webView);

    webView->show();
    QVERIFY(QTest::qWaitForWindowExposed(webView));
    webView->activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(webView));
}

// 函数说明：实现 HtmlPreviewControllerTest::cleanupTestCase 的核心逻辑，供当前模块调用。
void HtmlPreviewControllerTest::cleanupTestCase()
{
    delete controller;
    delete webView;
}

// 函数说明：实现 HtmlPreviewControllerTest::increasesZoomFactorOnZoomIn 的核心逻辑，供当前模块调用。
void HtmlPreviewControllerTest::increasesZoomFactorOnZoomIn()
{
    qreal previousZoomFactor = 1.0;
    webView->setZoomFactor(previousZoomFactor);

    controller->zoomInView();

    QVERIFY(webView->zoomFactor() > previousZoomFactor);
}

// 函数说明：实现 HtmlPreviewControllerTest::decreasesZoomFactorOnZoomOut 的核心逻辑，供当前模块调用。
void HtmlPreviewControllerTest::decreasesZoomFactorOnZoomOut()
{
    qreal previousZoomFactor = 1.0;
    webView->setZoomFactor(previousZoomFactor);

    controller->zoomOutView();

    QVERIFY(webView->zoomFactor() < previousZoomFactor);
}

// 函数说明：实现 HtmlPreviewControllerTest::resetsZoomFactorOnZoomReset 的核心逻辑，供当前模块调用。
void HtmlPreviewControllerTest::resetsZoomFactorOnZoomReset()
{
    qreal previousZoomFactor = 2.0;
    webView->setZoomFactor(previousZoomFactor);

    controller->resetZoomOfView();

    QCOMPARE(webView->zoomFactor(), 1.0);
}

// 函数说明：实现 HtmlPreviewControllerTest::zoomsInOnCtrlPlusKeyPress 的核心逻辑，供当前模块调用。
void HtmlPreviewControllerTest::zoomsInOnCtrlPlusKeyPress()
{
    qreal previousZoomFactor = 1.0;
    webView->setZoomFactor(previousZoomFactor);

    QTest::keyClick(webView, Qt::Key_Plus, Qt::ControlModifier);

    QVERIFY(webView->zoomFactor() > previousZoomFactor);
}

// 函数说明：实现 HtmlPreviewControllerTest::zoomsOutOnCtrlMinusKeyPress 的核心逻辑，供当前模块调用。
void HtmlPreviewControllerTest::zoomsOutOnCtrlMinusKeyPress()
{
    qreal previousZoomFactor = 1.0;
    webView->setZoomFactor(previousZoomFactor);

    QTest::keyClick(webView, Qt::Key_Minus, Qt::ControlModifier);

    QVERIFY(webView->zoomFactor() < previousZoomFactor);
}

// 函数说明：实现 HtmlPreviewControllerTest::resetsZoomOnCtrlZeroKeyPress 的核心逻辑，供当前模块调用。
void HtmlPreviewControllerTest::resetsZoomOnCtrlZeroKeyPress()
{
    qreal previousZoomFactor = 2.0;
    webView->setZoomFactor(previousZoomFactor);

    QTest::keyClick(webView, Qt::Key_0, Qt::ControlModifier);

    QCOMPARE(webView->zoomFactor(), 1.0);
}

// 函数说明：初始化 HtmlPreviewControllerTest 的 setupsNetworkDiskCache 相关界面、动作或服务连接。
void HtmlPreviewControllerTest::setupsNetworkDiskCache()
{
    QVERIFY(webView->page() != nullptr);
}

