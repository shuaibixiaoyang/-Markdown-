// 文件说明：app-static\compat\pdfcompat.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
/**
 * PDF Widgets Compatibility Layer
 *
 * On platforms with Qt PDF module: includes the real headers.
 * On MinGW without pdfwidgets (NO_PDFWIDGETS defined): provides stubs
 * so the project compiles.
 */
#ifndef PDFCOMPAT_H
#define PDFCOMPAT_H

#ifdef NO_PDFWIDGETS

#include <QObject>
#include <QImage>
#include <QString>
#include <QSizeF>

// QPdfDocumentRenderOptions stub
class QPdfDocumentRenderOptions
{
public:
    void setScaledSize(const QSize &) {}
};

// QPdfDocument stub
class QPdfDocument : public QObject
{
    Q_OBJECT
public:
    enum class Status { Null, Loading, Ready, Unloading, Error };
    enum class Error { None, Unknown, DataNotYetAvailable, FileNotFound, InvalidFileFormat, IncorrectPassword, UnsupportedSecurityScheme };

    explicit QPdfDocument(QObject *parent = nullptr) : QObject(parent), m_status(Status::Null) {}

    Error load(const QString &) { m_status = Status::Error; return Error::Unknown; }
    void close() { m_status = Status::Null; }

    Status status() const { return m_status; }
    int pageCount() const { return 0; }
    QSizeF pagePointSize(int) const { return QSizeF(); }

    QImage render(int, QSize, QPdfDocumentRenderOptions = QPdfDocumentRenderOptions())
    {
        return QImage();
    }

signals:
    void statusChanged(Status status);

private:
    Status m_status;
};

#else
// Real Qt PDF
#include <QPdfDocument>
#include <QPdfDocumentRenderOptions>
#endif

#endif // PDFCOMPAT_H

