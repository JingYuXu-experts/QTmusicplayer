#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QVariantList>

class ChatClient : public QObject
{
    Q_OBJECT
    // 聊天记录模型，供 QML 显示
    Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)

public:
    explicit ChatClient(QObject *parent = nullptr);

    QVariantList messages() const { return m_messages; }

    // QML 调用的函数
    Q_INVOKABLE void connectToServer();
    Q_INVOKABLE void sendMessage(const QString &userName, const QString &text);

signals:
    void messagesChanged();

private slots:
    void onReadyRead();

private:
    QTcpSocket *m_socket;
    QVariantList m_messages;
};

#endif // CHATCLIENT_H
