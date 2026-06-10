#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QTimer>
#include <QJsonObject>

class ChatServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit ChatServer(QObject *parent = nullptr);
    void startServer();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void onReadyRead();
    void onDisconnected();
    void broadcastBotMessage(); // 机器人的定时发言

private:
    void broadcast(const QJsonObject &message, QTcpSocket *exclude = nullptr);

    QList<QTcpSocket*> m_clients;
    QTimer *m_botTimer;
};

#endif // CHATSERVER_H
