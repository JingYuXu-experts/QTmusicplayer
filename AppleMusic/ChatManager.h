#ifndef CHATMANAGER_H
#define CHATMANAGER_H

#include <QObject>
#include <QVariantList>
#include <QTcpSocket> // 【新增】网络套接字

class ChatManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    explicit ChatManager(QObject *parent = nullptr);

    QVariantList messages() const { return m_messages; }
    QString statusText() const { return m_statusText; }

    Q_INVOKABLE void connectToServer();
    Q_INVOKABLE void sendMessage(const QString &user, const QString &content);
    Q_INVOKABLE void shareMusic(const QString &user, const QString &title, const QString &artist, const QString &cover);

signals:
    void messagesChanged();
    void statusTextChanged();

private slots:
    void onReadyRead(); // 接收服务器消息
    void onConnected();
    void onDisconnected();

private:
    QTcpSocket *m_socket; // 客户端 socket
    QVariantList m_messages;
    QString m_statusText = "离线";
};

#endif // CHATMANAGER_H
