#ifndef SPEECHRECOGNIZER_H
#define SPEECHRECOGNIZER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QByteArray>
#include <QFile>
#include <QJsonObject>

struct SpeechRecognitionResult {
    bool success;
    QString text;
    QString error;
    int errorCode;
    
    SpeechRecognitionResult() : success(false), errorCode(0) {}
    SpeechRecognitionResult(bool s, const QString& t, const QString& e = QString(), int code = 0)
        : success(s), text(t), error(e), errorCode(code) {}
};

struct BaiduApiConfig {
    QString apiKey;
    QString secretKey;
    QString accessToken;
    qint64 tokenExpireTime;
    
    BaiduApiConfig() : tokenExpireTime(0) {}
};

class SpeechRecognizer : public QObject
{
    Q_OBJECT

public:
    explicit SpeechRecognizer(QObject *parent = nullptr);
    ~SpeechRecognizer();
    
    void setApiKey(const QString& apiKey, const QString& secretKey);
    void setRecordingDirectory(const QString& directory);
    
    void recognizeFile(const QString& filePath);
    void recognizeLatestRecording();
    void recognizeFromDirectory(const QString& directory);
    
    bool isProcessing() const { return m_isProcessing; }
    QString lastRecognizedText() const { return m_lastResult.text; }
    
    static QString getRecordingsDirectory();

signals:
    void recognitionStarted();
    void recognitionFinished(const SpeechRecognitionResult& result);
    void recognitionError(const QString& error);
    void tokenRefreshed(bool success);

private slots:
    void onTokenReplyFinished(QNetworkReply* reply);
    void onRecognitionReplyFinished(QNetworkReply* reply);

private:
    bool loadConfig();
    bool saveConfig();
    void requestAccessToken();
    bool isTokenValid() const;
    QByteArray readPcmFile(const QString& filePath);
    QByteArray base64Encode(const QByteArray& data);
    void sendRecognitionRequest(const QByteArray& audioData, int dataSize);
    SpeechRecognitionResult parseRecognitionResponse(const QByteArray& data);
    QString findLatestPcmFile(const QString& directory);
    
    QNetworkAccessManager* m_networkManager;
    BaiduApiConfig m_config;
    QString m_recordingDirectory;
    bool m_isProcessing;
    bool m_isTokenRequesting;
    SpeechRecognitionResult m_lastResult;
    QByteArray m_pendingAudioData;
    int m_pendingAudioSize;
    
    static const QString BAIDU_TOKEN_URL;
    static const QString BAIDU_ASR_URL;
    static const int SAMPLE_RATE = 16000;
    static const int CHANNEL = 1;
    static const int DEV_PID = 1537;
};

#endif // SPEECHRECOGNIZER_H
