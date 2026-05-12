// 文件说明：test\integration\epubexportertest.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
/*
 * Integration tests for EPUB exporter metadata and compatibility output.
 */
#include "epubexportertest.h"

#include <QtTest>

#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryDir>

#include <publishing/epubexporter.h>

namespace {

QString readZipEntry(const QString &zipPath, const QString &entryPath, QString *error = nullptr)
{
    QProcess process;
    process.start(QStringLiteral("unzip"),
                  QStringList() << QStringLiteral("-p") << zipPath << entryPath);

    if (!process.waitForFinished(15000)) {
        if (error) {
            *error = process.errorString();
        }
        return QString();
    }

    if (process.exitCode() != 0) {
        if (error) {
            *error = QString::fromUtf8(process.readAllStandardError()).trimmed();
        }
        return QString();
    }

    return QString::fromUtf8(process.readAllStandardOutput());
}

} // namespace

// 函数说明：根据当前数据生成 EpubExporterTest 需要的输出结果。
void EpubExporterTest::generatesUuidWhenIdentifierIsMissing()
{
    EpubExporter exporter;
    EpubExporter::BookMetadata metadata;
    metadata.title = QStringLiteral("UUID Test");
    metadata.author = QStringLiteral("Tester");
    metadata.identifier.clear();

    exporter.setMetadata(metadata);

    const QString identifier = exporter.metadata().identifier;
    QVERIFY2(identifier.startsWith(QStringLiteral("urn:uuid:")),
             qPrintable(QStringLiteral("Expected urn:uuid identifier, got: %1").arg(identifier)));

    const QRegularExpression uuidPattern(
        QStringLiteral("^urn:uuid:[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"));
    QVERIFY2(uuidPattern.match(identifier).hasMatch(),
             qPrintable(QStringLiteral("Identifier is not a valid UUID URN: %1").arg(identifier)));
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
void EpubExporterTest::exportsCompatibilityCssAndIdentifier()
{
    if (QStandardPaths::findExecutable(QStringLiteral("zip")).isEmpty() ||
        QStandardPaths::findExecutable(QStringLiteral("unzip")).isEmpty()) {
        QSKIP("zip/unzip is required for EPUB integration test.");
    }

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString outputPath = tempDir.filePath(QStringLiteral("compatibility.epub"));

    EpubExporter exporter;
    const QString markdown = QStringLiteral("# 第一章\n\n兼容性测试内容。");
    QVERIFY2(exporter.exportFromMarkdown(markdown, outputPath),
             qPrintable(exporter.lastError()));
    QVERIFY(QFileInfo::exists(outputPath));

    QString readError;
    const QString opf = readZipEntry(outputPath, QStringLiteral("OEBPS/content.opf"), &readError);
    QVERIFY2(!opf.isEmpty(), qPrintable(QStringLiteral("Failed to read content.opf: %1").arg(readError)));

    QVERIFY(opf.contains(QStringLiteral("unique-identifier=\"BookId\"")));
    QVERIFY(opf.contains(QStringLiteral("urn:uuid:")));

    const QString stylesheet = readZipEntry(outputPath, QStringLiteral("OEBPS/styles/stylesheet.css"), &readError);
    QVERIFY2(!stylesheet.isEmpty(), qPrintable(QStringLiteral("Failed to read stylesheet.css: %1").arg(readError)));
    QVERIFY(stylesheet.contains(QStringLiteral("CuteMarkEd EPUB compatibility layer")));
    QVERIFY(stylesheet.contains(QStringLiteral("-epub-hyphens: auto;")));
    QVERIFY(stylesheet.contains(QStringLiteral("break-before: page;")));
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
void EpubExporterTest::exportsModernCssWhenSelected()
{
    if (QStandardPaths::findExecutable(QStringLiteral("zip")).isEmpty() ||
        QStandardPaths::findExecutable(QStringLiteral("unzip")).isEmpty()) {
        QSKIP("zip/unzip is required for EPUB integration test.");
    }

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString outputPath = tempDir.filePath(QStringLiteral("modern.epub"));

    EpubExporter exporter;
    EpubExporter::ExportOptions options = exporter.options();
    options.cssProfile = EpubExporter::CssProfile::Modern;
    exporter.setOptions(options);

    const QString markdown = QStringLiteral("# Modern\n\n样式测试。");
    QVERIFY2(exporter.exportFromMarkdown(markdown, outputPath),
             qPrintable(exporter.lastError()));

    QString readError;
    const QString stylesheet = readZipEntry(outputPath, QStringLiteral("OEBPS/styles/stylesheet.css"), &readError);
    QVERIFY2(!stylesheet.isEmpty(), qPrintable(QStringLiteral("Failed to read stylesheet.css: %1").arg(readError)));
    QVERIFY(stylesheet.contains(QStringLiteral("CuteMarkEd EPUB modern layer")));
    QVERIFY(!stylesheet.contains(QStringLiteral("CuteMarkEd EPUB compatibility layer")));
    QVERIFY(!stylesheet.contains(QStringLiteral("-epub-hyphens: auto;")));
}

