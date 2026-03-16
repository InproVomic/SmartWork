#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include "ApiManager.h"
#include "ConfigDialog.h"
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QString>
#include <vector>
#include <QProcess>
#include <QPixmap>

class TcpClient;
class PythonProcessManager;

struct Message {
    QString content;
    bool isUser;
    QString timestamp;

    Message(const QString& c, bool u, const QString& t)
        : content(c), isUser(u), timestamp(t) {}
};

class ChatClient : public QWidget
{
    Q_OBJECT

public:
    explicit ChatClient(QWidget *parent = nullptr);
    ~ChatClient();

    void setApiConfig(const ApiConfig& config);
    ApiConfig getApiConfig() const;
    void initializePythonProcess();

signals:
    void messageSent(const QString& message);

private slots:
    void onSendClicked();
    void onReturnPressed();
    void onConfigClicked();
    void onExportClicked();
    void onApiResponse(const ApiResponse& response);
    void onRequestStarted();
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpResponseReceived(const QString& content, bool success);
    void onTcpError(const QString& error);
    void onPythonProcessStarted();
    void onPythonProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onPythonProcessError(const QString& error);
    void onReconnectTimer();
    void onScrollChanged(int value);

private:
    void setupUI();
    void applyStyles();
    void addMessage(const QString& message, bool isUser);
    QString getCurrentTime() const;
    void loadConfig();
    void saveConfig();
    QJsonArray getHistoryForApi() const;
    void tryConnectToPython();
    QString getAvatarLetter(const QString& name);
    QString getAvatarHtml(bool isUser, int size);
    void loadAvatarConfig();
    void saveAvatarConfig();
    QString getAvatarConfigPath() const;
    void refreshMessages();
    QPixmap createDefaultAvatar(const QString& name, const QColor& color);

    ApiManager* m_apiManager;
    ConfigDialog* m_configDialog;
    QLineEdit* m_inputEdit;
    QPushButton* m_sendButton;
    QTextEdit* m_chatDisplay;
    QPushButton* m_configButton;
    QPushButton* m_exportButton;
    QLabel* m_statusLabel;
    std::vector<Message> m_messages;
    QString m_configPath;
    TcpClient* m_tcpClient;
    bool m_useTcpMode;
    PythonProcessManager* m_pythonProcess;
    QTimer* m_reconnectTimer;
    int m_reconnectAttempts;
    bool m_isRequestInProgress;
    
    QString m_userName;
    QString m_botName;
    QString m_userAvatarPath;
    QString m_botAvatarPath;
    QPixmap m_userAvatar;
    QPixmap m_botAvatar;
};

#endif // CHATCLIENT_H
