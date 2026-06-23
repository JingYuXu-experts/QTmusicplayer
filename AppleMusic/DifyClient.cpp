#include "DifyClient.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QSet>
#include <QUrl>
#include <QUuid>

namespace {
void collectTextValues(const QJsonValue &value, QStringList &values, QSet<QString> &seen)
{
    if (value.isString()) {
        const QString text = value.toString().trimmed();
        if (!text.isEmpty() && !seen.contains(text)) {
            seen.insert(text);
            values.append(text);
        }
        return;
    }

    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        for (const QJsonValue &item : array) {
            collectTextValues(item, values, seen);
        }
        return;
    }

    if (value.isObject()) {
        const QJsonObject object = value.toObject();
        for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
            collectTextValues(it.value(), values, seen);
        }
    }
}
}

DifyClient::DifyClient(QObject *parent)
    : QObject(parent),
      m_network(new QNetworkAccessManager(this)),
      m_userId(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    loadConfiguration();
}

void DifyClient::sendPrompt(const QString &prompt)
{
    const QString trimmedPrompt = prompt.trimmed();
    if (trimmedPrompt.isEmpty() || m_busy) {
        return;
    }

    appendMessage(QStringLiteral("user"), trimmedPrompt);

    if (!m_configured) {
        const QString error = QStringLiteral("Dify 配置不可用，请检查 config/dify.ini。");
        appendMessage(QStringLiteral("assistant"), error, true);
        setStatusText(error);
        return;
    }

    QString endpoint = m_apiBaseUrl;
    while (endpoint.endsWith('/')) {
        endpoint.chop(1);
    }
    endpoint += QStringLiteral("/workflows/run");

    QNetworkRequest request{QUrl(endpoint)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", QByteArray("Bearer ") + m_apiKey.toUtf8());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(m_timeoutMs);

    QJsonObject inputs;
    inputs.insert(m_inputKey, trimmedPrompt);

    QJsonObject body;
    body.insert(QStringLiteral("inputs"), inputs);
    body.insert(QStringLiteral("response_mode"), QStringLiteral("blocking"));
    body.insert(QStringLiteral("user"), m_userId);

    setBusy(true);
    setStatusText(QStringLiteral("AI 正在思考…"));

    QNetworkReply *reply = m_network->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    m_activeReply = reply;

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply != m_activeReply) {
            reply->deleteLater();
            return;
        }

        handleReply(reply);
        m_activeReply = nullptr;
        setBusy(false);
        reply->deleteLater();
    });
}

void DifyClient::clearHistory()
{
    if (m_busy || m_messages.isEmpty()) {
        return;
    }

    m_messages.clear();
    emit messagesChanged();
    setStatusText(m_configured ? QStringLiteral("AI 助手已就绪")
                               : QStringLiteral("Dify 配置不可用"));
}

void DifyClient::loadConfiguration()
{
    const QString configPath = locateConfigurationFile();
    if (configPath.isEmpty()) {
        m_configured = false;
        setStatusText(QStringLiteral("未找到 config/dify.ini"));
        emit configuredChanged();
        return;
    }

    QSettings settings(configPath, QSettings::IniFormat);
    m_apiBaseUrl = qEnvironmentVariable("DIFY_API_BASE_URL");
    if (m_apiBaseUrl.trimmed().isEmpty()) {
        m_apiBaseUrl = settings.value(QStringLiteral("Dify/apiBaseUrl")).toString().trimmed();
    }

    m_apiKey = qEnvironmentVariable("DIFY_API_KEY");
    if (m_apiKey.trimmed().isEmpty()) {
        m_apiKey = settings.value(QStringLiteral("Dify/apiKey")).toString().trimmed();
    }

    m_inputKey = settings.value(QStringLiteral("Dify/inputKey"), QStringLiteral("clear_query"))
                     .toString()
                     .trimmed();
    if (m_inputKey.isEmpty()) {
        m_inputKey = QStringLiteral("clear_query");
    }

    m_timeoutMs = settings.value(QStringLiteral("Dify/timeoutMs"), 95000).toInt();
    m_timeoutMs = qBound(1000, m_timeoutMs, 100000);

    const QUrl apiUrl(m_apiBaseUrl);
    m_configured = apiUrl.isValid()
                   && (apiUrl.scheme() == QStringLiteral("https")
                       || apiUrl.scheme() == QStringLiteral("http"))
                   && !apiUrl.host().isEmpty()
                   && !m_apiKey.isEmpty();

    setStatusText(m_configured ? QStringLiteral("AI 助手已就绪")
                               : QStringLiteral("Dify 配置内容无效"));
    emit configuredChanged();
}

QString DifyClient::locateConfigurationFile() const
{
    QStringList candidates;
    const QString explicitPath = qEnvironmentVariable("DIFY_CONFIG_FILE");
    if (!explicitPath.trimmed().isEmpty()) {
        candidates.append(explicitPath);
    }

    const QString appDir = QCoreApplication::applicationDirPath();
    const QString currentDir = QDir::currentPath();
    candidates.append({
        QDir(appDir).absoluteFilePath(QStringLiteral("config/dify.ini")),
        QDir(appDir).absoluteFilePath(QStringLiteral("dify.ini")),
        QDir(appDir).absoluteFilePath(QStringLiteral("../config/dify.ini")),
        QDir(appDir).absoluteFilePath(QStringLiteral("../../config/dify.ini")),
        QDir(appDir).absoluteFilePath(QStringLiteral("../../../config/dify.ini")),
        QDir(currentDir).absoluteFilePath(QStringLiteral("config/dify.ini")),
        QDir(currentDir).absoluteFilePath(QStringLiteral("AppleMusic/config/dify.ini"))
    });

    QSet<QString> checked;
    for (const QString &candidate : candidates) {
        const QString cleanPath = QDir::cleanPath(QFileInfo(candidate).absoluteFilePath());
        if (checked.contains(cleanPath)) {
            continue;
        }
        checked.insert(cleanPath);

        const QFileInfo info(cleanPath);
        if (info.exists() && info.isFile() && info.isReadable()) {
            return cleanPath;
        }
    }
    return {};
}

void DifyClient::appendMessage(const QString &role, const QString &content, bool isError)
{
    QVariantMap message;
    message.insert(QStringLiteral("role"), role);
    message.insert(QStringLiteral("content"), content);
    message.insert(QStringLiteral("isError"), isError);
    m_messages.append(message);
    emit messagesChanged();
}

void DifyClient::handleReply(QNetworkReply *reply)
{
    const QByteArray payload = reply->readAll();
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    QString errorMessage;
    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() == QNetworkReply::TimeoutError) {
            errorMessage = QStringLiteral("Dify 请求超时，请稍后重试。");
        } else if (statusCode == 401 || statusCode == 403) {
            errorMessage = QStringLiteral("Dify 认证失败，请检查 API Key。");
        } else if (statusCode == 429) {
            errorMessage = QStringLiteral("Dify 请求过于频繁或额度不足，请稍后重试。");
        } else if (statusCode >= 500) {
            errorMessage = QStringLiteral("Dify 服务暂时不可用，请稍后重试。");
        } else {
            const QString apiMessage = responseErrorMessage(payload);
            errorMessage = apiMessage.isEmpty()
                               ? QStringLiteral("网络请求失败：%1").arg(reply->errorString())
                               : QStringLiteral("Dify 请求失败：%1").arg(apiMessage);
        }
    } else if (statusCode < 200 || statusCode >= 300) {
        const QString apiMessage = responseErrorMessage(payload);
        errorMessage = apiMessage.isEmpty()
                           ? QStringLiteral("Dify 返回 HTTP %1。").arg(statusCode)
                           : QStringLiteral("Dify 请求失败：%1").arg(apiMessage);
    }

    if (!errorMessage.isEmpty()) {
        appendMessage(QStringLiteral("assistant"), errorMessage, true);
        setStatusText(errorMessage);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        errorMessage = QStringLiteral("Dify 返回了无法解析的数据。");
    } else {
        const QJsonObject root = document.object();
        const QJsonObject data = root.value(QStringLiteral("data")).toObject();
        const QString workflowStatus = data.value(QStringLiteral("status")).toString();
        const QString workflowError = data.value(QStringLiteral("error")).toString().trimmed();

        if (!workflowError.isEmpty()
            || (!workflowStatus.isEmpty() && workflowStatus != QStringLiteral("succeeded"))) {
            errorMessage = workflowError.isEmpty()
                               ? QStringLiteral("Dify 工作流执行失败：%1").arg(workflowStatus)
                               : QStringLiteral("Dify 工作流执行失败：%1").arg(workflowError);
        } else {
            const QString answer = outputText(data.value(QStringLiteral("outputs")));
            if (answer.isEmpty()) {
                errorMessage = QStringLiteral("Dify 工作流已完成，但没有返回可显示的文本。");
            } else {
                appendMessage(QStringLiteral("assistant"), answer);
                setStatusText(QStringLiteral("AI 助手已就绪"));
                return;
            }
        }
    }

    appendMessage(QStringLiteral("assistant"), errorMessage, true);
    setStatusText(errorMessage);
}

void DifyClient::setBusy(bool busy)
{
    if (m_busy == busy) {
        return;
    }
    m_busy = busy;
    emit busyChanged();
}

void DifyClient::setStatusText(const QString &statusText)
{
    if (m_statusText == statusText) {
        return;
    }
    m_statusText = statusText;
    emit statusTextChanged();
}

QString DifyClient::outputText(const QJsonValue &value) const
{
    QStringList values;
    QSet<QString> seen;
    collectTextValues(value, values, seen);
    return values.join(QStringLiteral("\n\n")).trimmed();
}

QString DifyClient::responseErrorMessage(const QByteArray &payload) const
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return {};
    }

    const QJsonObject root = document.object();
    const QString message = root.value(QStringLiteral("message")).toString().trimmed();
    if (!message.isEmpty()) {
        return message;
    }

    const QString error = root.value(QStringLiteral("error")).toString().trimmed();
    if (!error.isEmpty()) {
        return error;
    }

    return root.value(QStringLiteral("data")).toObject()
        .value(QStringLiteral("error")).toString().trimmed();
}
