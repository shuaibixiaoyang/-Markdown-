// 文件说明：test\unit\stylemanagertest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef STYLEMANAGERTEST_H
#define STYLEMANAGERTEST_H

#include <QObject>

class StyleManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void returnsPathForMarkdownHighlighting();
    void returnsPathForCodeHighlighting();
    void returnsPathForPreviewStylesheet();
    void returnsPathForCustomPreviewStylesheet();
    void customPreviewStylesheetOverwritesBuiltin();
};

#endif // STYLEMANAGERTEST_H



