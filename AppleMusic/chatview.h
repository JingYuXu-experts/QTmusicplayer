#ifndef CHATMANAGER_H
#define CHATMANAGER_H

#include <QObject>
#include <QVariantList>
#include <QDateTime>

class ChatManager : public QObject
{
    Q_OBJECT
    // 消息列表
    Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)
    // 好友列表 (带状态)
    Q_PROPERTY(QVariantList friends READ friends NOTIFY friendsChanged)

public:
    explicit ChatManager(QObject *parent = nullptr);

    QVariantList messages() const { return m_messages; }
    QVariantList friends() const { return m_friends; }

    // 发送消息
    Q_INVOKABLE void sendMessage(const QString &content);

signals:
    void messagesChanged();
    void friendsChanged();

private:
    QVariantList m_messages;
    QVariantList m_friends;
};

#endif // CHATMANAGER_H
