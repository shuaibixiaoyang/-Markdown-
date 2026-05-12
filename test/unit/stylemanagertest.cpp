// 文件说明：test\unit\stylemanagertest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "stylemanagertest.h"

#include <QtTest>

#include <themes/stylemanager.h>

static const Theme defaultTheme("Default", "Default", "Default", "Default");
static const Theme githubTheme("Github", "Github", "Github", "Github");
static const Theme solarizedLightTheme("Solarized Light", "Solarized Light", "Solarized Light", "Solarized Light");
static const Theme solarizedDarkTheme("Solarized Dark", "Solarized Dark", "Solarized Dark", "Solarized Dark");
static const Theme clearnessTheme("Clearness", "Clearness", "Clearness", "Clearness");
static const Theme clearnessDarkTheme("Clearness Dark", "Clearness Dark", "Clearness Dark", "Clearness Dark");
static const Theme bywordDarkTheme("Byword Dark", "Byword Dark", "Byword Dark", "Byword Dark");

// 函数说明：实现 StyleManagerTest::returnsPathForMarkdownHighlighting 的核心逻辑，供当前模块调用。
void StyleManagerTest::returnsPathForMarkdownHighlighting()
{
    StyleManager styleManager;

    QCOMPARE(styleManager.markdownHighlightingPath(defaultTheme), QLatin1String("default"));
    QCOMPARE(styleManager.markdownHighlightingPath(solarizedLightTheme), QLatin1String("solarized-light"));
    QCOMPARE(styleManager.markdownHighlightingPath(solarizedDarkTheme), QLatin1String("solarized-dark"));
    QCOMPARE(styleManager.markdownHighlightingPath(clearnessDarkTheme), QLatin1String("clearness-dark"));
    QCOMPARE(styleManager.markdownHighlightingPath(bywordDarkTheme), QLatin1String("byword-dark"));
}

// 函数说明：实现 StyleManagerTest::returnsPathForCodeHighlighting 的核心逻辑，供当前模块调用。
void StyleManagerTest::returnsPathForCodeHighlighting()
{
    StyleManager styleManager;

    QCOMPARE(styleManager.codeHighlightingPath(defaultTheme), QLatin1String("default"));
    QCOMPARE(styleManager.codeHighlightingPath(githubTheme), QLatin1String("github"));
    QCOMPARE(styleManager.codeHighlightingPath(solarizedLightTheme), QLatin1String("solarized_light"));
    QCOMPARE(styleManager.codeHighlightingPath(solarizedDarkTheme), QLatin1String("solarized_dark"));
}

// 函数说明：实现 StyleManagerTest::returnsPathForPreviewStylesheet 的核心逻辑，供当前模块调用。
void StyleManagerTest::returnsPathForPreviewStylesheet()
{
    StyleManager styleManager;

    QCOMPARE(styleManager.previewStylesheetPath(defaultTheme), QLatin1String("qrc:/css/markdown.css"));
    QCOMPARE(styleManager.previewStylesheetPath(githubTheme), QLatin1String("qrc:/css/github.css"));
    QCOMPARE(styleManager.previewStylesheetPath(solarizedLightTheme), QLatin1String("qrc:/css/solarized-light.css"));
    QCOMPARE(styleManager.previewStylesheetPath(solarizedDarkTheme), QLatin1String("qrc:/css/solarized-dark.css"));
    QCOMPARE(styleManager.previewStylesheetPath(clearnessTheme), QLatin1String("qrc:/css/clearness.css"));
    QCOMPARE(styleManager.previewStylesheetPath(clearnessDarkTheme), QLatin1String("qrc:/css/clearness-dark.css"));
    QCOMPARE(styleManager.previewStylesheetPath(bywordDarkTheme), QLatin1String("qrc:/css/byword-dark.css"));
}

// 函数说明：实现 StyleManagerTest::returnsPathForCustomPreviewStylesheet 的核心逻辑，供当前模块调用。
void StyleManagerTest::returnsPathForCustomPreviewStylesheet()
{
    QString expectedPath = "file:///C:/User/Test/custom.css";
    Theme customTheme("Custom", "Default", "Default", "Custom");
    StyleManager styleManager;
    
    styleManager.insertCustomPreviewStylesheet("Custom", expectedPath);

    QCOMPARE(styleManager.previewStylesheetPath(customTheme), expectedPath);
}

// 函数说明：实现 StyleManagerTest::customPreviewStylesheetOverwritesBuiltin 的核心逻辑，供当前模块调用。
void StyleManagerTest::customPreviewStylesheetOverwritesBuiltin()
{
    QString expectedPath = "file:///C:/User/Test/custom.css";
    Theme customTheme("Custom", "Default", "Default", "Github");
    StyleManager styleManager;

    styleManager.insertCustomPreviewStylesheet("Github", expectedPath);

    QCOMPARE(styleManager.previewStylesheetPath(customTheme), expectedPath);
}

