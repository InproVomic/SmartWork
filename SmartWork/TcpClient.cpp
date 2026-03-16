#include "TcpClient.h"
#include <QTcpSocket>
#include <QDebug>

TcpClient::TcpClient(QObject *parent)
    : QObject(parent)
    , m_socket(nullptr)
    , m_isConnected(false)
    , m_blockSize(0)
{
    m_socket = new QTcpSocket(this);
    
    connect(m_socket, &QTcpSocket::connected, this, &TcpClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &TcpClient::onError);
}

TcpClient::~TcpClient()
{
    disconnectFromServer();
}

void TcpClient::connectToServer(const QString& host, quint16 port)
{
    if (m_isConnected) {
        disconnectFromServer();
    }
    
    m_blockSize = 0;
    m_socket->connectToHost(host, port);
}

void TcpClient::disconnectFromServer()
{
    if (m_socket) {
        m_socket->disconnectFromHost();
    }
    m_isConnected = false;
    m_blockSize = 0;
}

void TcpClient::sendMessage(const QString& message)
{
    if (!m_isConnected) {
        emit errorOccurred("Not connected to server");
        return;
    }
    
    QByteArray data = message.toUtf8();
    quint32 length = static_cast<quint32>(data.size());
    
    QByteArray packet;
    packet.reserve(4 + data.size());
    
    packet.append(static_cast<char>((length >> 24) & 0xFF));
    packet.append(static_cast<char>((length >> 16) & 0xFF));
    packet.append(static_cast<char>((length >> 8) & 0xFF));
    packet.append(static_cast<char>(length & 0xFF));
    packet.append(data);
    
    m_socket->write(packet);
    m_socket->flush();
    
    qDebug() << "Sent message, length:" << length;
}

bool TcpClient::isConnected() const
{
    return m_isConnected;
}

void TcpClient::onConnected()
{
    m_isConnected = true;
    m_blockSize = 0;
    qDebug() << "TCP connected";
    emit connected();
}

void TcpClient::onDisconnected()
{
    m_isConnected = false;
    m_blockSize = 0;
    qDebug() << "TCP disconnected";
    emit disconnected();
}

void TcpClient::onReadyRead()
{
    qDebug() << "ReadyRead, bytes available:" << m_socket->bytesAvailable();
    
    while (m_socket->bytesAvailable() > 0) {
        if (m_blockSize == 0) {
            if (m_socket->bytesAvailable() < 4) {
                return;
            }
            
            QByteArray lengthData = m_socket->read(4);
            m_blockSize = (static_cast<quint32>(static_cast<unsigned char>(lengthData[0])) << 24) |
                         (static_cast<quint32>(static_cast<unsigned char>(lengthData[1])) << 16) |
                         (static_cast<quint32>(static_cast<unsigned char>(lengthData[2])) << 8) |
                         static_cast<quint32>(static_cast<unsigned char>(lengthData[3]));
            
            qDebug() << "Received packet length:" << m_blockSize;
        }
        
        if (m_socket->bytesAvailable() < m_blockSize) {
            qDebug() << "Waiting for more data, need:" << m_blockSize << "have:" << m_socket->bytesAvailable();
            return;
        }
        
        QByteArray data = m_socket->read(m_blockSize);
        QString response = QString::fromUtf8(data);
        
        qDebug() << "Received response:" << response.left(100);
        
        m_blockSize = 0;
        
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8(), &parseError);
        
        if (parseError.error != QJsonParseError::NoError) {
            emit responseReceived(response, false);
            return;
        }
        
        QJsonObject obj = doc.object();
        bool success = obj.value(QLatin1String("success")).toBool(false);
        
        if (success) {
            QString content = obj.value(QLatin1String("content")).toString();
            qDebug() << "=== Emitting responseReceived signal ===";
            qDebug() << "Content length:" << content.length();
            qDebug() << "Content preview:" << content.left(100);
            emit responseReceived(content, true);
        } else {
            QString error = obj.value(QLatin1String("error")).toString();
            emit responseReceived(error, false);
        }
    }
}

void TcpClient::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    qDebug() << "Socket error:" << m_socket->errorString();
    emit errorOccurred(m_socket->errorString());
}