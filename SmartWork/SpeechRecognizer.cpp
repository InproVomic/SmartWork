#include "SpeechRecognizer.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

const QString SpeechRecognizer::BAIDU_TOKEN_URL = QLatin1String("https://aip.baidubce.com/oauth/2.0/token");
const QString SpeechRecognizer::BAIDU_ASR_URL = QLatin1String("https://vop.baidu.com/server_api");

SpeechRecognizer::SpeechRecognizer(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_isProcessing(false)
    , m_isTokenRequesting(false)
    , m_pendingAudioSize(0)
{
    m_recordingDirectory = getRecordingsDirectory();
    loadConfig();
}

SpeechRecognizer::~SpeechRecognizer()
{
    saveConfig();
}

QString SpeechRecognizer::getRecordingsDirectory()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString recordingsDir = appDir + QLatin1String("/Recordings");
    
    if (!QDir(recordingsDir).exists()) {
        QString altDir = QLatin1String("D:/SmartWork/1/SmartWork/x64/Debug/Recordings");
        if (QDir(altDir).exists()) {
            return QDir::cleanPath(altDir);
        }
    }
    
    return QDir::cleanPath(recordingsDir);
}

void SpeechRecognizer::setApiKey(const QString& apiKey, const QString& secretKey)
{
    m_config.apiKey = apiKey;
    m_config.secretKey = secretKey;
    m_config.accessToken.clear();
    m_config.tokenExpireTime = 0;
    saveConfig();
}

void SpeechRecognizer::setRecordingDirectory(const QString& directory)
{
    m_recordingDirectory = directory;
}

bool SpeechRecognizer::loadConfig()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString configPath = configDir + QLatin1String("/baidu_speech_config.json");
    
    QFile file(configPath);
    if (!file.exists()) {
        return false;
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return false;
    }
    
    QJsonObject root = doc.object();
    m_config.apiKey = root[QLatin1String("apiKey")].toString();
    m_config.secretKey = root[QLatin1String("secretKey")].toString();
    m_config.accessToken = root[QLatin1String("accessToken")].toString();
    m_config.tokenExpireTime = root[QLatin1String("tokenExpireTime")].toVariant().toLongLong();
    
    return true;
}

bool SpeechRecognizer::saveConfig()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(configDir);
    
    QString configPath = configDir + QLatin1String("/baidu_speech_config.json");
    
    QJsonObject root;
    root[QLatin1String("apiKey")] = m_config.apiKey;
    root[QLatin1String("secretKey")] = m_config.secretKey;
    root[QLatin1String("accessToken")] = m_config.accessToken;
    root[QLatin1String("tokenExpireTime")] = QJsonValue::fromVariant(m_config.tokenExpireTime);
    
    QJsonDocument doc(root);
    
    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(doc.toJson());
    file.close();
    return true;
}

bool SpeechRecognizer::isTokenValid() const
{
    if (m_config.accessToken.isEmpty()) {
        return false;
    }
    
    qint64 currentTime = QDateTime::currentSecsSinceEpoch();
    return currentTime < (m_config.tokenExpireTime - 60);
}

void SpeechRecognizer::requestAccessToken()
{
    if (m_config.apiKey.isEmpty() || m_config.secretKey.isEmpty()) {
        emit recognitionError(QString::fromUtf8("API密钥未配置"));
        return;
    }
    
    m_isTokenRequesting = true;
    
    QUrlQuery query;
    query.addQueryItem(QLatin1String("grant_type"), QLatin1String("client_credentials"));
    query.addQueryItem(QLatin1String("client_id"), m_config.apiKey);
    query.addQueryItem(QLatin1String("client_secret"), m_config.secretKey);
    
    QUrl url(BAIDU_TOKEN_URL);
    url.setQuery(query);
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/x-www-form-urlencoded"));
    
    QNetworkReply* reply = m_networkManager->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onTokenReplyFinished(reply);
    });
}

void SpeechRecognizer::onTokenReplyFinished(QNetworkReply* reply)
{
    reply->deleteLater();
    m_isTokenRequesting = false;
    
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = QString::fromUtf8("获取Access Token失败: ") + reply->errorString();
        emit recognitionError(errorMsg);
        emit tokenRefreshed(false);
        return;
    }
    
    QByteArray data = reply->readAll();
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        emit recognitionError(QString::fromUtf8("Token响应解析失败"));
        emit tokenRefreshed(false);
        return;
    }
    
    QJsonObject root = doc.object();
    
    if (root.contains(QLatin1String("error"))) {
        QString errorDesc = root[QLatin1String("error_description")].toString();
        emit recognitionError(QString::fromUtf8("Token错误: ") + errorDesc);
        emit tokenRefreshed(false);
        return;
    }
    
    m_config.accessToken = root[QLatin1String("access_token")].toString();
    int expiresIn = root[QLatin1String("expires_in")].toInt(86400);
    m_config.tokenExpireTime = QDateTime::currentSecsSinceEpoch() + expiresIn;
    
    saveConfig();
    emit tokenRefreshed(true);
    
    if (m_isProcessing && !m_pendingAudioData.isEmpty()) {
        sendRecognitionRequest(m_pendingAudioData, m_pendingAudioSize);
        m_pendingAudioData.clear();
        m_pendingAudioSize = 0;
    }
}

QByteArray SpeechRecognizer::readPcmFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.exists()) {
        qWarning() << "PCM file not found:" << filePath;
        return QByteArray();
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open PCM file:" << filePath;
        return QByteArray();
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    return data;
}

QByteArray SpeechRecognizer::base64Encode(const QByteArray& data)
{
    return data.toBase64();
}

void SpeechRecognizer::sendRecognitionRequest(const QByteArray& audioData, int dataSize)
{
    if (m_config.accessToken.isEmpty()) {
        emit recognitionError(QString::fromUtf8("Access Token无效"));
        m_isProcessing = false;
        return;
    }
    
    QString speechBase64 = QString::fromLatin1(base64Encode(audioData));
    
    QJsonObject requestBody;
    requestBody[QLatin1String("format")] = QLatin1String("pcm");
    requestBody[QLatin1String("rate")] = SAMPLE_RATE;
    requestBody[QLatin1String("channel")] = CHANNEL;
    requestBody[QLatin1String("speech")] = speechBase64;
    requestBody[QLatin1String("len")] = dataSize;
    requestBody[QLatin1String("token")] = m_config.accessToken;
    requestBody[QLatin1String("dev_pid")] = DEV_PID;
    requestBody[QLatin1String("cuid")] = QLatin1String("SmartWork_Client");
    
    QJsonDocument doc(requestBody);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    QUrl asrUrl(BAIDU_ASR_URL);
    QNetworkRequest request(asrUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/json"));
    
    QNetworkReply* reply = m_networkManager->post(request, jsonData);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onRecognitionReplyFinished(reply);
    });
}

SpeechRecognitionResult SpeechRecognizer::parseRecognitionResponse(const QByteArray& data)
{
    SpeechRecognitionResult result;
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        result.error = QString::fromUtf8("响应解析失败: ") + parseError.errorString();
        return result;
    }
    
    QJsonObject root = doc.object();
    
    int errNo = root[QLatin1String("err_no")].toInt();
    QString errMsg = root[QLatin1String("err_msg")].toString();
    
    if (errNo != 0) {
        result.errorCode = errNo;
        result.error = QString::fromUtf8("识别错误(%1): %2").arg(errNo).arg(errMsg);
        return result;
    }
    
    QJsonArray resultArray = root[QLatin1String("result")].toArray();
    if (resultArray.isEmpty()) {
        result.error = QString::fromUtf8("未识别到语音内容");
        return result;
    }
    
    QStringList texts;
    for (const QJsonValue& val : resultArray) {
        texts.append(val.toString());
    }
    
    result.success = true;
    result.text = texts.join(QString());
    
    return result;
}

QString SpeechRecognizer::findLatestPcmFile(const QString& directory)
{
    QDir dir(directory);
    if (!dir.exists()) {
        return QString();
    }
    
    QStringList filters;
    filters << QLatin1String("*.pcm");
    
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Time | QDir::Reversed);
    
    if (files.isEmpty()) {
        return QString();
    }
    
    return files.last().absoluteFilePath();
}

void SpeechRecognizer::recognizeFile(const QString& filePath)
{
    if (m_isProcessing) {
        emit recognitionError(QString::fromUtf8("正在处理中，请稍候"));
        return;
    }
    
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        emit recognitionError(QString::fromUtf8("文件不存在: ") + filePath);
        return;
    }
    
    if (!fileInfo.suffix().toLower().contains(QLatin1String("pcm"))) {
        emit recognitionError(QString::fromUtf8("仅支持PCM格式音频文件"));
        return;
    }
    
    m_isProcessing = true;
    emit recognitionStarted();
    
    QByteArray audioData = readPcmFile(filePath);
    if (audioData.isEmpty()) {
        m_isProcessing = false;
        emit recognitionError(QString::fromUtf8("无法读取音频文件"));
        return;
    }
    
    int dataSize = audioData.size();
    
    if (!isTokenValid()) {
        m_pendingAudioData = audioData;
        m_pendingAudioSize = dataSize;
        requestAccessToken();
        return;
    }
    
    sendRecognitionRequest(audioData, dataSize);
}

void SpeechRecognizer::recognizeLatestRecording()
{
    QString latestFile = findLatestPcmFile(m_recordingDirectory);
    
    if (latestFile.isEmpty()) {
        emit recognitionError(QString::fromUtf8("录音目录中没有找到PCM文件"));
        return;
    }
    
    recognizeFile(latestFile);
}

void SpeechRecognizer::recognizeFromDirectory(const QString& directory)
{
    m_recordingDirectory = directory;
    recognizeLatestRecording();
}

void SpeechRecognizer::onRecognitionReplyFinished(QNetworkReply* reply)
{
    reply->deleteLater();
    m_isProcessing = false;
    
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = QString::fromUtf8("网络请求失败: ") + reply->errorString();
        m_lastResult = SpeechRecognitionResult(false, QString(), errorMsg);
        emit recognitionError(errorMsg);
        emit recognitionFinished(m_lastResult);
        return;
    }
    
    QByteArray data = reply->readAll();
    m_lastResult = parseRecognitionResponse(data);
    
    if (m_lastResult.success) {
        qDebug() << "Recognition successful:" << m_lastResult.text;
    } else {
        qDebug() << "Recognition failed:" << m_lastResult.error;
    }
    
    emit recognitionFinished(m_lastResult);
}
