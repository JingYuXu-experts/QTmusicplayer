#include "ChatClient.h"
#include <QDateTime>

ChatClient::ChatClient(QObject *parent) : QObject(parent)
{
    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::readyRead, this, &ChatClient::onReadyRead);

    // 连接成功提示
    connect(m_socket, &QTcpSocket::connected, this, [this](){
        m_messages.append(">>> 已连接到服务器 (127.0.0.1:8888)");
        emit messagesChanged();
    });
}

void ChatClient::connectToServer()
{
    if(m_socket->state() == QAbstractSocket::UnconnectedState) {
        // 连接本地服务器
        m_socket->connectToHost("127.0.0.1", 8888);
    }
}

void ChatClient::sendMessage(const QString &userName, const QString &text)
{
    if(m_socket->state() == QAbstractSocket::ConnectedState && !text.isEmpty()) {
        // 格式: [时间] 用户名: 内容
        QString time = QDateTime::currentDateTime().toString("HH:mm");
        QString fullMsg = QString("[%1] %2: %3").arg(time, userName, text);

        m_socket->write(fullMsg.toUtf8());
    }
}

void ChatClient::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    QString msg = QString::fromUtf8(data);

    // 把收到的消息加到列表里
    m_messages.append(msg);
    emit messagesChanged();
}
