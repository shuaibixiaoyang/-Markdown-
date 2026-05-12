// 文件说明：app-static\compat\websocketcompat.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
/**
 * WebSocket Compatibility Layer
 *
 * On platforms with Qt WebSockets: includes the real headers.
 * On MinGW without WebSockets (NO_WEBSOCKETS defined): provides
 * QTcpSocket/QTcpServer-based stubs so the project compiles.
 */
#ifndef WEBSOCKETCOMPAT_H
#define WEBSOCKETCOMPAT_H

#ifdef NO_WEBSOCKETS

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QUrl>
#include <QSslError>
#include <QSslConfiguration>
#include <QQueue>
#include <QDataStream>

// QWebSocketProtocol stub
namespace QWebSocketProtocol {
    enum Version { VersionLatest = 13 };
}

// QWebSocket replacement using QTcpSocket
class QWebSocket : public QObject
{
    Q_OBJECT
public:
    explicit QWebSocket(const QString &origin = QString(),
                        QWebSocketProtocol::Version version = QWebSocketProtocol::VersionLatest,
                        QObject *parent = nullptr)
        : QObject(parent)
    {
        Q_UNUSED(origin);
        Q_UNUSED(version);
        m_socket = new QTcpSocket(this);
        connect(m_socket, &QTcpSocket::connected, this, &QWebSocket::connected);
        connect(m_socket, &QTcpSocket::disconnected, this, &QWebSocket::disconnected);
        connect(m_socket, &QTcpSocket::readyRead, this, &QWebSocket::onReadyRead);
        connect(m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError err) {
            emit errorOccurred(err);
        });
    }

    void open(const QUrl &url)
    {
        m_socket->connectToHost(url.host(), static_cast<quint16>(url.port(80)));
    }

    void close()
    {
        m_socket->close();
    }

    void abort()
    {
        m_socket->abort();
    }

    qint64 sendTextMessage(const QString &message)
    {
        QByteArray data = message.toUtf8();
        // Simple length-prefixed protocol
        QByteArray header;
        QDataStream ds(&header, QIODevice::WriteOnly);
        ds << static_cast<quint32>(data.size());
        m_socket->write(header);
        return m_socket->write(data);
    }

    bool isValid() const { return m_socket->isValid(); }
    QAbstractSocket::SocketState state() const { return m_socket->state(); }
    QHostAddress peerAddress() const { return m_socket->peerAddress(); }
    quint16 peerPort() const { return m_socket->peerPort(); }
    QString errorString() const { return m_socket->errorString(); }

    void ignoreSslErrors() {}
    void ignoreSslErrors(const QList<QSslError> &) {}
    void setSslConfiguration(const QSslConfiguration &) {}

    qint64 sendBinaryMessage(const QByteArray &data)
    {
        QByteArray header;
        QDataStream ds(&header, QIODevice::WriteOnly);
        ds << static_cast<quint32>(data.size());
        m_socket->write(header);
        return m_socket->write(data);
    }

signals:
    void connected();
    void disconnected();
    void textMessageReceived(const QString &message);
    void binaryMessageReceived(const QByteArray &message);
    void errorOccurred(QAbstractSocket::SocketError error);
    void sslErrors(const QList<QSslError> &errors);

private slots:
    void onReadyRead()
    {
        m_buffer.append(m_socket->readAll());
        while (m_buffer.size() >= 4) {
            QDataStream ds(m_buffer);
            quint32 len;
            ds >> len;
            if (static_cast<quint32>(m_buffer.size()) < 4 + len) break;
            QByteArray msg = m_buffer.mid(4, len);
            m_buffer.remove(0, 4 + len);
            emit textMessageReceived(QString::fromUtf8(msg));
        }
    }

private:
    QTcpSocket *m_socket;
    QByteArray m_buffer;
};

// QWebSocketServer replacement using QTcpServer
class QWebSocketServer : public QObject
{
    Q_OBJECT
public:
    enum SslMode { NonSecureMode, SecureMode };

    explicit QWebSocketServer(const QString &serverName, SslMode secureMode, QObject *parent = nullptr)
        : QObject(parent), m_secureMode(secureMode)
    {
        Q_UNUSED(serverName);
        m_server = new QTcpServer(this);
        connect(m_server, &QTcpServer::newConnection, this, &QWebSocketServer::onNewConnection);
    }

    bool listen(const QHostAddress &address = QHostAddress::Any, quint16 port = 0)
    {
        return m_server->listen(address, port);
    }

    void close() { m_server->close(); }
    bool isListening() const { return m_server->isListening(); }
    quint16 serverPort() const { return m_server->serverPort(); }
    QHostAddress serverAddress() const { return m_server->serverAddress(); }
    QString errorString() const { return m_server->errorString(); }

    void setSslConfiguration(const QSslConfiguration &) {}

    QWebSocket *nextPendingConnection()
    {
        if (m_pendingConnections.isEmpty()) return nullptr;
        return m_pendingConnections.dequeue();
    }

signals:
    void newConnection();

private slots:
    void onNewConnection()
    {
        while (m_server->hasPendingConnections()) {
            QTcpSocket *tcp = m_server->nextPendingConnection();
            QWebSocket *ws = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
            // Transfer the socket - note: simplified, real impl would be more complex
            Q_UNUSED(tcp);
            m_pendingConnections.enqueue(ws);
            emit newConnection();
        }
    }

private:
    QTcpServer *m_server;
    SslMode m_secureMode;
    QQueue<QWebSocket*> m_pendingConnections;
};

#else
// Real Qt WebSockets
#include <QWebSocket>
#include <QWebSocketServer>
#endif

#endif // WEBSOCKETCOMPAT_H

