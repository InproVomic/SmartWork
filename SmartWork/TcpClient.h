#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QAbstractSocket>
#include <QJsonDocument>
#include <QJsonObject>

class QTcpSocket;

class TcpClient : public QObject
{
    Q_OBJECT

public:
    explicit TcpClient(QObject *parent = nullptr);
    ~TcpClient();

    void connectToServer(const QString& host, quint16 port);
    void disconnectFromServer();
    void sendMessage(const QString& message);
    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void responseReceived(const QString& content, bool success);
    void errorOccurred(const QString& error);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket* m_socket;
    bool m_isConnected;
    quint32 m_blockSize;
};

#endif // TCPCLIENT_H