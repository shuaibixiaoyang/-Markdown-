// 文件说明：test\integration\latexexportertest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
/*
 * Copyright 2026
 */
#ifndef LATEXEXPORTERTEST_H
#define LATEXEXPORTERTEST_H

#include <QObject>

class LaTeXExporterTest : public QObject
{
    Q_OBJECT

private slots:
    void preservesInlineAndDisplayMath();
    void ignoresEscapedDollarAndCodeSpanForMath();
    void usesListingsInAutoModeByDefault();
    void usesMintedInAutoModeWhenEnvironmentEnabled();
};

#endif // LATEXEXPORTERTEST_H

