// 文件说明：test\integration\ocrintegrationtest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
/*
 * Integration tests for OCR path resolution and PdfOcr render guardrails.
 */
#include "ocrintegrationtest.h"

#include <QtTest>

#include <QCoreApplication>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QSettings>
#include <QTemporaryDir>

#include <ocr/ocrengine.h>
#include <ocr/pdfocr.h>

namespace {

bool createHugeSinglePagePdf(const QString &path)
{
    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QSizeF(2400.0, 2400.0), QPageSize::Point));
    writer.setPageMargins(QMarginsF(0.0, 0.0, 0.0, 0.0), QPageLayout::Point);
    writer.setResolution(72);

    QPainter painter(&writer);
    if (!painter.isActive()) {
        return false;
    }

    painter.fillRect(QRectF(0, 0, 2400, 2400), Qt::white);
    painter.setPen(QPen(Qt::black, 8));
    painter.drawRect(QRectF(120, 120, 2160, 2160));
    painter.drawText(QPointF(220, 400), QStringLiteral("Huge PDF page for render clamp test"));
    painter.end();
    return true;
}

void clearOcrSettings()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("ocr"));
    settings.remove(QString());
    settings.endGroup();
    settings.sync();
}

QString makeTessdataDir(QTemporaryDir &dir, const QString &name)
{
    const QString path = dir.filePath(name + QStringLiteral("/tessdata"));
    if (!QDir().mkpath(path)) {
        return QString();
    }
    return QDir(path).absolutePath();
}

} // namespace

// 函数说明：实现 OcrIntegrationTest::init 的核心逻辑，供当前模块调用。
void OcrIntegrationTest::init()
{
    m_previousOrganization = QCoreApplication::organizationName();
    m_previousApplication = QCoreApplication::applicationName();

    QCoreApplication::setOrganizationName(QStringLiteral("CuteMarkEd-Integration"));
    QCoreApplication::setApplicationName(QStringLiteral("OcrIntegrationTestApp"));
    clearOcrSettings();
    qunsetenv("TESSDATA_PREFIX");
}

// 函数说明：实现 OcrIntegrationTest::cleanup 的核心逻辑，供当前模块调用。
void OcrIntegrationTest::cleanup()
{
    clearOcrSettings();
    qunsetenv("TESSDATA_PREFIX");
    QCoreApplication::setOrganizationName(m_previousOrganization);
    QCoreApplication::setApplicationName(m_previousApplication);
}

// 函数说明：实现 OcrIntegrationTest::pdfRenderClampsHugePageResolution 的核心逻辑，供当前模块调用。
void OcrIntegrationTest::pdfRenderClampsHugePageResolution()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString pdfPath = dir.filePath(QStringLiteral("huge-page.pdf"));
    QVERIFY(createHugeSinglePagePdf(pdfPath));

    PdfOcr pdfOcr;
    const QImage image = pdfOcr.renderPage(pdfPath, 0, 600);
    QVERIFY(!image.isNull());

    const qint64 pixels = static_cast<qint64>(image.width()) * static_cast<qint64>(image.height());
    const int maxSide = qMax(image.width(), image.height());

    QVERIFY2(pixels <= 25000000LL, qPrintable(QStringLiteral("pixels=%1").arg(pixels)));
    QVERIFY2(maxSide <= 6000, qPrintable(QStringLiteral("maxSide=%1").arg(maxSide)));
}

// 函数说明：执行 OcrIntegrationTest 的启动初始化流程，把延后加载的功能接入主界面。
void OcrIntegrationTest::initializeReportsMissingLanguageFiles()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString tessdataPath = makeTessdataDir(dir, QStringLiteral("missing_lang"));
    QVERIFY(!tessdataPath.isEmpty());

    OcrEngine engine;
    QVERIFY(engine.setLanguages(QStringList() << QStringLiteral("eng") << QStringLiteral("chi_sim")));
    QVERIFY(!engine.initialize(tessdataPath));

    const QString error = engine.lastInitializationError();
    QVERIFY(error.contains(QStringLiteral("缺少语言包")));
    QVERIFY(error.contains(QStringLiteral("eng")));
    QVERIFY(error.contains(QStringLiteral("chi_sim")));
}

// 函数说明：实现 OcrIntegrationTest::explicitPathOverridesEnvAndPersistedPath 的核心逻辑，供当前模块调用。
void OcrIntegrationTest::explicitPathOverridesEnvAndPersistedPath()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString explicitPath = makeTessdataDir(dir, QStringLiteral("explicit"));
    const QString envPath = makeTessdataDir(dir, QStringLiteral("env"));
    const QString persistedPath = makeTessdataDir(dir, QStringLiteral("persisted"));
    QVERIFY(!explicitPath.isEmpty());
    QVERIFY(!envPath.isEmpty());
    QVERIFY(!persistedPath.isEmpty());

    OcrEngine persistedEngine;
    persistedEngine.setTessdataPath(persistedPath);
    qputenv("TESSDATA_PREFIX", envPath.toLocal8Bit());

    OcrEngine engine;
    QVERIFY(!engine.initialize(explicitPath));

    const QString firstLine = engine.lastInitializationError().section('\n', 0, 0);
    QVERIFY2(firstLine.contains(QDir(explicitPath).absolutePath()), qPrintable(firstLine));
}

// 函数说明：实现 OcrIntegrationTest::envPathOverridesPersistedPathWhenNoExplicitPath 的核心逻辑，供当前模块调用。
void OcrIntegrationTest::envPathOverridesPersistedPathWhenNoExplicitPath()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString envPath = makeTessdataDir(dir, QStringLiteral("env_only"));
    const QString persistedPath = makeTessdataDir(dir, QStringLiteral("persisted_only"));
    QVERIFY(!envPath.isEmpty());
    QVERIFY(!persistedPath.isEmpty());

    OcrEngine persistedEngine;
    persistedEngine.setTessdataPath(persistedPath);
    qputenv("TESSDATA_PREFIX", envPath.toLocal8Bit());

    OcrEngine engine;
    QVERIFY(!engine.initialize());

    const QString firstLine = engine.lastInitializationError().section('\n', 0, 0);
    QVERIFY2(firstLine.contains(QDir(envPath).absolutePath()), qPrintable(firstLine));
}

