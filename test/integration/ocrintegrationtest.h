// 文件说明：test\integration\ocrintegrationtest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
/*
 * Integration tests for OCR path resolution and PdfOcr render guardrails.
 */
#ifndef OCRINTEGRATIONTEST_H
#define OCRINTEGRATIONTEST_H

#include <QObject>

class OcrIntegrationTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void pdfRenderClampsHugePageResolution();
    void initializeReportsMissingLanguageFiles();
    void explicitPathOverridesEnvAndPersistedPath();
    void envPathOverridesPersistedPathWhenNoExplicitPath();

private:
    QString m_previousOrganization;
    QString m_previousApplication;
};

#endif // OCRINTEGRATIONTEST_H

