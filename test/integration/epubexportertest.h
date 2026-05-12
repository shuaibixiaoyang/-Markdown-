// 文件说明：test\integration\epubexportertest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
/*
 * Integration tests for EPUB exporter metadata and compatibility output.
 */
#ifndef EPUBEXPORTERTEST_H
#define EPUBEXPORTERTEST_H

#include <QObject>

class EpubExporterTest : public QObject
{
    Q_OBJECT

private slots:
    void generatesUuidWhenIdentifierIsMissing();
    void exportsCompatibilityCssAndIdentifier();
    void exportsModernCssWhenSelected();
};

#endif // EPUBEXPORTERTEST_H

