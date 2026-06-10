#include "ChatManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>

ChatManager::ChatManager(QObject *parent) : QObject(parent)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::readyRead, this, &ChatManager::onReadyRead);
    connect(m_socket, &QTcpSocket::connected, this, &ChatManager::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &ChatManager::onDisconnected);
}

void ChatManager::connectToServer()
{
    m_statusText = "正在连接服务器...";
    emit statusTextChanged();

    // 连接本地服务器
    m_socket->abort();
    m_socket->connectToHost("127.0.0.1", 12345);
}

void ChatManager::sendMessage(const QString &user, const QString &content)
{
    // 1. 先在本地显示（让自己看到自己发的消息）
    QVariantMap msgMap;
    msgMap["type"] = "text";
    msgMap["sender"] = user;
    msgMap["content"] = content;
    msgMap["isSelf"] = true;
    m_messages.append(msgMap);
    emit messagesChanged();

    // 2. 发送给服务器 (JSON格式)
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        QJsonObject json;
        json["type"] = "text";
        json["sender"] = user;
        json["content"] = content;
        // 注意：发给服务器时不需要 isSelf，服务器会转发给别人
        m_socket->write(QJsonDocument(json).toJson(QJsonDocument::Compact) + "\n");
    }
}

void ChatManager::shareMusic(const QString &user, const QString &title, const QString &artist, const QString &cover)
{
    // 1. 本地显示
    QVariantMap msgMap;
    msgMap["type"] = "music";
    msgMap["sender"] = user;
    msgMap["title"] = title;
    msgMap["artist"] = artist;
    msgMap["cover"] = cover;
    msgMap["isSelf"] = true;
    m_messages.append(msgMap);
    emit messagesChanged();

    // 2. 发送给服务器
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        QJsonObject json;
        json["type"] = "music";
        json["sender"] = user;
        json["title"] = title;
        json["artist"] = artist;
        json["cover"] = cover;
        m_socket->write(QJsonDocument(json).toJson(QJsonDocument::Compact) + "\n");
    }
}

void ChatManager::onReadyRead()
{
    // 接收服务器发来的消息 (Jay, Taylor, 或者 System)
    while (m_socket->canReadLine()) {
        QByteArray data = m_socket->readLine();
        QJsonDocument doc = QJsonDocument::fromJson(data);

        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            QVariantMap msgMap = obj.toVariantMap();

            // 标记为别人发的
            msgMap["isSelf"] = false;

            m_messages.append(msgMap);
            emit messagesChanged();
        }
    }
}

void ChatManager::onConnected()
{
    m_statusText = "🟢 已连接到服务器 | 3位好友在线";
    emit statusTextChanged();
}

void ChatManager::onDisconnected()
{
    m_statusText = "🔴 与服务器断开连接";
    emit statusTextChanged();
}
