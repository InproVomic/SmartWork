#include "ApiManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QTimer>

ApiManager::ApiManager(QObject *parent)
    : QObject(parent)
    , m_provider(QLatin1String("deepseek"))
{
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &ApiManager::onReplyFinished);
}

ApiManager::~ApiManager()
{
    cancelRequest();
}

void ApiManager::setConfig(const ApiConfig& config)
{
    m_config = config;
}

ApiConfig ApiManager::getConfig() const
{
    return m_config;
}

void ApiManager::setApiProvider(const QString& provider)
{
    m_provider = provider.toLower();

    // Set default base URL based on provider
    if (m_provider == QLatin1String("openai")) {
        m_config.baseUrl = QLatin1String("https://api.openai.com/v1");
        if (m_config.model.isEmpty() || m_config.model.startsWith(QLatin1String("deepseek"))) {
            m_config.model = QLatin1String("gpt-3.5-turbo");
        }
    } else if (m_provider == QLatin1String("deepseek")) {
        m_config.baseUrl = QLatin1String("https://api.deepseek.com/v1");
        if (m_config.model.isEmpty() || m_config.model.startsWith(QLatin1String("gpt"))) {
            m_config.model = QLatin1String("deepseek-chat");
        }
    }
}

void ApiManager::sendMessage(const QString& message, const QJsonArray& history)
{
    if (m_config.apiKey.isEmpty()) {
        QString error = QString::fromUtf8("API Key \u672a\u914d\u7f6e");
        emit responseReceived(ApiResponse(false, QString(), error));
        return;
    }

    emit requestStarted();

    QString url = m_config.baseUrl + QLatin1String("/chat/completions");

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/json"));
    QString auth = QLatin1String("Bearer ") + m_config.apiKey;
    request.setRawHeader("Authorization", auth.toUtf8());

    QString body = buildRequestBody(message, history);

    QNetworkReply* reply = m_networkManager->post(request, body.toUtf8());

    connect(reply, &QNetworkReply::uploadProgress,
            this, &ApiManager::requestProgress);
    // Error handling is done in onReplyFinished
}

void ApiManager::cancelRequest()
{
    // Find and abort any active replies
    QList<QNetworkReply*> replies = m_networkManager->findChildren<QNetworkReply*>();
    for (QNetworkReply* reply : replies) {
        if (reply->isRunning()) {
            reply->abort();
        }
    }
}

void ApiManager::onReplyFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = reply->errorString();
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        emit responseReceived(ApiResponse(false, QString(), errorMsg, statusCode));
        return;
    }

    QByteArray data = reply->readAll();
    ApiResponse response = parseResponse(data);
    response.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    emit responseReceived(response);
}

QString ApiManager::buildRequestBody(const QString& message, const QJsonArray& history) const
{
    QJsonObject root;

    // Add messages
    QJsonArray messages;

    // Add system prompt
    if (!m_config.systemPrompt.isEmpty()) {
        QJsonObject systemMsg;
        systemMsg[QLatin1String("role")] = QLatin1String("system");
        systemMsg[QLatin1String("content")] = m_config.systemPrompt;
        messages.append(systemMsg);
    }

    // Add history
    messages.append(history);

    // Add current message
    QJsonObject userMsg;
    userMsg[QLatin1String("role")] = QLatin1String("user");
    userMsg[QLatin1String("content")] = message;
    messages.append(userMsg);

    root[QLatin1String("messages")] = messages;

    // Add model
    root[QLatin1String("model")] = m_config.model;

    // Add parameters
    root[QLatin1String("temperature")] = m_config.temperature;
    root[QLatin1String("max_tokens")] = m_config.maxTokens;
    root[QLatin1String("top_p")] = m_config.topP / 100.0;

    // Stream
    root[QLatin1String("stream")] = false;

    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Compact);
}

ApiResponse ApiManager::parseResponse(const QByteArray& data) const
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        QString error = QString::fromUtf8("JSON \u89e3\u6790\u9519\u8bef: ") + parseError.errorString();
        return ApiResponse(false, QString(), error);
    }

    QJsonObject root = doc.object();

    // Check for error
    if (root.contains(QLatin1String("error"))) {
        QJsonObject errorObj = root[QLatin1String("error")].toObject();
        QString errorMsg = errorObj[QLatin1String("message")].toString();
        if (errorMsg.isEmpty()) {
            errorMsg = errorObj[QLatin1String("type")].toString();
        }
        return ApiResponse(false, QString(), errorMsg);
    }

    // Extract content from choices
    QJsonArray choices = root[QLatin1String("choices")].toArray();
    if (choices.isEmpty()) {
        QString error = QString::fromUtf8("\u54cd\u5e94\u4e2d\u6ca1\u6709 choices");
        return ApiResponse(false, QString(), error);
    }

    QJsonObject choice = choices.first().toObject();
    QJsonObject messageObj = choice[QLatin1String("message")].toObject();
    QString content = messageObj[QLatin1String("content")].toString();

    return ApiResponse(true, content);
}
