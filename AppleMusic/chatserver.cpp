#include "ChatServer.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QDebug>

ChatServer::ChatServer(QObject *parent) : QTcpServer(parent)
{
    // 启动机器人定时器
    m_botTimer = new QTimer(this);
    connect(m_botTimer, &QTimer::timeout, this, &ChatServer::broadcastBotMessage);
}

void ChatServer::startServer()
{
    // 监听所有网卡的 12345 端口
    if (this->listen(QHostAddress::Any, 12345)) {
        qDebug() << "Server started on port 12345";
        m_botTimer->start(5000); // 5秒说一次话
    } else {
        qDebug() << "Server failed to start:" << this->errorString();
    }
}

void ChatServer::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket *socket = new QTcpSocket(this);
    if (socket->setSocketDescriptor(socketDescriptor)) {
        connect(socket, &QTcpSocket::readyRead, this, &ChatServer::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &ChatServer::onDisconnected);
        m_clients.append(socket);
        qDebug() << "Client connected. Total:" << m_clients.size();

        // 发送欢迎消息
        QJsonObject welcome;
        welcome["type"] = "text";
        welcome["sender"] = "System";
        welcome["content"] = "已连接到 Music 官方服务器";
        welcome["isSelf"] = false;

        socket->write(QJsonDocument(welcome).toJson(QJsonDocument::Compact) + "\n");
    } else {
        delete socket;
    }
}

void ChatServer::onReadyRead()
{
    QTcpSocket *senderSocket = qobject_cast<QTcpSocket*>(sender());
    if (!senderSocket) return;

    // 读取所有数据 (按行读取，防止粘包)
    while (senderSocket->canReadLine()) {
        QByteArray data = senderSocket->readLine();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            // 广播给所有人（除了发送者自己，因为发送者界面已经显示了）
            // 注意：实际项目中通常也回显给发送者，但这里为了配合你的UI逻辑，我们在客户端自己处理自己发的消息
            // 这里我们把消息转发给【其他】客户端
            broadcast(obj, senderSocket);
        }
    }
}

void ChatServer::onDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        m_clients.removeAll(socket);
        socket->deleteLater();
        qDebug() << "Client disconnected. Remaining:" << m_clients.size();
    }
}

void ChatServer::broadcast(const QJsonObject &message, QTcpSocket *exclude)
{
    QByteArray data = QJsonDocument(message).toJson(QJsonDocument::Compact) + "\n";
    for (QTcpSocket *client : m_clients) {
        if (client != exclude) {
            client->write(data);
            client->flush();
        }
    }
}

// --- 机器人逻辑移到服务器 ---
void ChatServer::broadcastBotMessage()
{
    int role = QRandomGenerator::global()->bounded(2);
    QString name = (role == 0) ? "Jay Chou" : "Taylor";

    QStringList jayTexts = { "哎哟，不错哦。", "想喝奶茶了...", "新歌编曲还可以吧？", "演唱会见！" };
    QStringList taylorTexts = { "Hi guys! 💖", "Writing songs ✍️", "Midnights ✨", "Love you all!" };

    QString content = (role == 0)
                          ? jayTexts[QRandomGenerator::global()->bounded(jayTexts.size())]
                          : taylorTexts[QRandomGenerator::global()->bounded(taylorTexts.size())];

    QJsonObject msg;
    msg["type"] = "text";
    msg["sender"] = name;
    msg["content"] = content;
    msg["isSelf"] = false; // 对所有客户端来说，这都是别人发的

    broadcast(msg);
}
