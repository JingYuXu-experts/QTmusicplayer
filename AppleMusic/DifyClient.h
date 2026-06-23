#ifndef DIFYCLIENT_H
#define DIFYCLIENT_H

#include <QObject>
#include <QVariantList>

class QJsonValue;
class QNetworkAccessManager;
class QNetworkReply;

class DifyClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool configured READ configured NOTIFY configuredChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    explicit DifyClient(QObject *parent = nullptr);

    QVariantList messages() const { return m_messages; }
    bool busy() const { return m_busy; }
    bool configured() const { return m_configured; }
    QString statusText() const { return m_statusText; }

    Q_INVOKABLE void sendPrompt(const QString &prompt);
    Q_INVOKABLE void clearHistory();

signals:
    void messagesChanged();
    void busyChanged();
    void configuredChanged();
    void statusTextChanged();

private:
    void loadConfiguration();
    QString locateConfigurationFile() const;
    void appendMessage(const QString &role, const QString &content, bool isError = false);
    void handleReply(QNetworkReply *reply);
    void setBusy(bool busy);
    void setStatusText(const QString &statusText);
    QString endpointPath() const;
    QString outputText(const QJsonValue &value) const;
    QString chatAnswerText(const QJsonObject &root) const;
    QString responseErrorMessage(const QByteArray &payload) const;

    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_activeReply = nullptr;
    QVariantList m_messages;
    QString m_apiBaseUrl;
    QString m_apiKey;
    QString m_appMode = QStringLiteral("chat");
    QString m_inputKey = QStringLiteral("user_query");
    QString m_contextKey = QStringLiteral("player_context");
    QString m_conversationId;
    QString m_userId;
    QString m_statusText;
    int m_timeoutMs = 95000;
    bool m_busy = false;
    bool m_configured = false;
};

#endif // DIFYCLIENT_H
