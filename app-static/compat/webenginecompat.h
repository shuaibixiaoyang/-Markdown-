// 文件说明：app-static\compat\webenginecompat.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
/**
 * WebEngine Compatibility Layer
 *
 * On platforms with Qt WebEngine (macOS, MSVC): includes the real headers.
 * On MinGW (NO_WEBENGINE defined): provides QTextBrowser-based replacements
 * so the project compiles and runs with basic HTML preview support.
 */
#ifndef WEBENGINECOMPAT_H
#define WEBENGINECOMPAT_H

#ifdef NO_WEBENGINE

#include <QTextBrowser>
#include <QAction>
#include <QVariant>
#include <QUrl>
#include <QPageLayout>
#include <functional>

// Stub for QWebEnginePage
class QWebEnginePage : public QObject
{
    Q_OBJECT
public:
    enum WebAction { Copy, Paste, Cut, Undo, Redo, SelectAll, NoWebAction = -1 };

    explicit QWebEnginePage(QObject *parent = nullptr) : QObject(parent) {}

    void runJavaScript(const QString &, const std::function<void(const QVariant &)> &callback = nullptr)
    {
        // No JS support in fallback mode
        if (callback) callback(QVariant());
    }

    void setWebChannel(QObject *) {}

    QAction *action(WebAction) const { return nullptr; }

    // printToPdf stubs (no-op in fallback mode)
    void printToPdf(const std::function<void(const QByteArray &)> &callback,
                    const QPageLayout & = QPageLayout())
    {
        if (callback) callback(QByteArray());
    }
    void printToPdf(const QString &, const QPageLayout & = QPageLayout()) {}
};

// Stub for QWebEngineSettings
class QWebEngineSettings
{
public:
    enum WebAttribute {
        JavascriptEnabled, LocalContentCanAccessRemoteUrls,
        LocalContentCanAccessFileUrls, PluginsEnabled,
        ShowScrollBars
    };
    void setAttribute(WebAttribute, bool) {}
    static QWebEngineSettings *defaultSettings() {
        static QWebEngineSettings s;
        return &s;
    }
};

// QWebEngineView replacement based on QTextBrowser
class QWebEngineView : public QTextBrowser
{
    Q_OBJECT
public:
    explicit QWebEngineView(QWidget *parent = nullptr)
        : QTextBrowser(parent), m_page(new QWebEnginePage(this)), m_zoomFactor(1.0)
    {
        setOpenExternalLinks(true);
        setReadOnly(true);
    }

    QWebEnginePage *page() const { return m_page; }

    QAction *pageAction(QWebEnginePage::WebAction action) const
    {
        Q_UNUSED(action);
        // Return a dummy action for Copy etc.
        if (!m_copyAction) {
            auto *self = const_cast<QWebEngineView*>(this);
            self->m_copyAction = new QAction(tr("Copy"), self);
            connect(self->m_copyAction, &QAction::triggered, self, &QTextBrowser::copy);
        }
        return m_copyAction;
    }

    void setHtml(const QString &html, const QUrl &baseUrl = QUrl())
    {
        QTextBrowser::setHtml(html);
        Q_UNUSED(baseUrl);
        emit loadFinished(true);
    }

    void setUrl(const QUrl &url) { setSource(url); }
    QUrl url() const { return source(); }

    qreal zoomFactor() const { return m_zoomFactor; }
    void setZoomFactor(qreal factor)
    {
        m_zoomFactor = factor;
        QFont f = font();
        f.setPointSizeF(10.0 * factor);
        setFont(f);
    }

    QWebEngineSettings *settings() const {
        return QWebEngineSettings::defaultSettings();
    }

signals:
    void loadFinished(bool ok);
    void titleChanged(const QString &title);

private:
    QWebEnginePage *m_page;
    qreal m_zoomFactor;
    QAction *m_copyAction = nullptr;
};

#else
// Real Qt WebEngine
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#endif

// Alias used by mainwindow.ui (avoids uic generating Qt module includes)
using CompatWebView = QWebEngineView;

#endif // WEBENGINECOMPAT_H

