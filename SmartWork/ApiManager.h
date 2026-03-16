#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <memory>

struct ApiResponse {
    bool success;
    QString content;
    QString error;
    int statusCode;

    ApiResponse() : success(false), statusCode(0) {}
    ApiResponse(bool s, const QString& c, const QString& e = QString(), int code = 0)
        : success(s), content(c), error(e), statusCode(code) {}
};

struct ApiConfig {
    QString apiKey;
    QString baseUrl;
    QString model;
    QString systemPrompt;
    double temperature;
    int maxTokens;
    int topP;

    ApiConfig()
        : apiKey(QString())
        , baseUrl(QLatin1String("https://api.deepseek.com/v1"))
        , model(QLatin1String("deepseek-chat"))
        , systemPrompt(QString::fromUtf8("\u4f60\u662f\u4e00\u4e2a\u6709\u5e2e\u52a9\u7684AI\u52a9\u624b\u3002"))
        , temperature(0.7)
        , maxTokens(4096)
        , topP(1)
    {}
};

class ApiManager : public QObject
{
    Q_OBJECT

public:
    explicit ApiManager(QObject *parent = nullptr);
    ~ApiManager();

    // Configuration
    void setConfig(const ApiConfig& config);
    ApiConfig getConfig() const;

    // API calls
    void sendMessage(const QString& message, const QJsonArray& history = QJsonArray());
    void cancelRequest();

    // For future API integration
    void setApiProvider(const QString& provider); // "deepseek", "openai", etc.

signals:
    void responseReceived(const ApiResponse& response);
    void requestProgress(qint64 bytesSent, qint64 bytesTotal);
    void requestStarted();

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    QString buildRequestBody(const QString& message, const QJsonArray& history) const;
    ApiResponse parseResponse(const QByteArray& data) const;

    QNetworkAccessManager* m_networkManager;
    ApiConfig m_config;
    QString m_provider;
};
