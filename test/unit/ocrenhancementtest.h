// 文件说明：test\unit\ocrenhancementtest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
/*
 * OCR enhancement tests for Qt6 implementation.
 */
#ifndef OCRENHANCEMENTTEST_H
#define OCRENHANCEMENTTEST_H

#include <QObject>

class OcrEnhancementTest : public QObject
{
    Q_OBJECT

private slots:
    void preprocessImageHonorsExistingDpi();
    void preprocessImageClampsLargeInputScale();
    void tableDetectorDetectsStructuredGrid();
};

#endif // OCRENHANCEMENTTEST_H

