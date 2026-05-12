// 文件说明：test\unit\ocrenhancementtest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "ocrenhancementtest.h"

#include <QtTest>
#include <QImage>
#include <QPainter>

#include <ocr/ocrengine.h>

namespace {

constexpr qreal kMetersPerInch = 39.37007874015748;

QImage createTableGridImage()
{
    QImage image(420, 300, QImage::Format_RGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QPen(Qt::black, 3));

    const QVector<int> verticals = {20, 140, 260, 380};
    for (const int x : verticals) {
        painter.drawLine(x, 20, x, 260);
    }

    const QVector<int> horizontals = {20, 100, 180, 260};
    for (const int y : horizontals) {
        painter.drawLine(20, y, 380, y);
    }

    painter.end();
    return image;
}

} // namespace

// 函数说明：实现 OcrEnhancementTest::preprocessImageHonorsExistingDpi 的核心逻辑，供当前模块调用。
void OcrEnhancementTest::preprocessImageHonorsExistingDpi()
{
    QImage image(400, 300, QImage::Format_RGB32);
    image.fill(Qt::white);
    image.setDotsPerMeterX(qRound(300.0 * kMetersPerInch));
    image.setDotsPerMeterY(qRound(300.0 * kMetersPerInch));

    const QImage processed = OcrEngine::preprocessImage(image, 300);
    QCOMPARE(processed.width(), image.width());
    QCOMPARE(processed.height(), image.height());
    QCOMPARE(processed.format(), QImage::Format_Grayscale8);
}

// 函数说明：实现 OcrEnhancementTest::preprocessImageClampsLargeInputScale 的核心逻辑，供当前模块调用。
void OcrEnhancementTest::preprocessImageClampsLargeInputScale()
{
    QImage image(5000, 4000, QImage::Format_RGB32);
    image.fill(Qt::white);

    const QImage processed = OcrEngine::preprocessImage(image, 600);
    const qint64 maxPixels = 25000000LL;
    const qint64 processedPixels =
        static_cast<qint64>(processed.width()) * static_cast<qint64>(processed.height());

    QVERIFY(processedPixels <= maxPixels);
    QVERIFY(processed.width() <= 6000);
    QVERIFY(processed.height() <= 6000);
}

// 函数说明：实现 OcrEnhancementTest::tableDetectorDetectsStructuredGrid 的核心逻辑，供当前模块调用。
void OcrEnhancementTest::tableDetectorDetectsStructuredGrid()
{
    const QImage image = createTableGridImage();
    const QVector<OcrEngine::Table> tables = TableDetector::detectTables(image);

    QVERIFY(!tables.isEmpty());
    const OcrEngine::Table table = tables.first();

    QCOMPARE(table.rows, 3);
    QCOMPARE(table.columns, 3);
    QCOMPARE(table.cells.size(), 9);
    QVERIFY(table.boundingBox.isValid());
}

