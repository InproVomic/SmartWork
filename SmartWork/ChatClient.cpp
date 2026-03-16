#include "ChatClient.h"
#include "ConfigDialog.h"
#include "TcpClient.h"
#include "PythonProcessManager.h"
#include "LAppDefine.hpp"
#include <QDateTime>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QScrollBar>
#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QPixmap>
#include <QIcon>
#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QPainter>
#include <QBuffer>

namespace {
    const QString TCP_HOST = QLatin1String("127.0.0.1");
    const quint16 TCP_PORT = 18790;
    const int MAX_RECONNECT_ATTEMPTS = 30;
    const int RECONNECT_INTERVAL_MS = 500;
}

ChatClient::ChatClient(QWidget *parent)
    : QWidget(parent)
    , m_isRequestInProgress(false)
    , m_configPath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1String("/chat_config.json"))
    , m_tcpClient(nullptr)
    , m_useTcpMode(true)
    , m_pythonProcess(nullptr)
    , m_reconnectTimer(nullptr)
    , m_reconnectAttempts(0)
    , m_userName(QString::fromUtf8("用户"))
    , m_botName(QString::fromUtf8("助手"))
{
    setWindowTitle(QString::fromUtf8("SmartWork Chat"));
    resize(550, 750);

    m_apiManager = new ApiManager(this);
    connect(m_apiManager, &ApiManager::responseReceived, this, &ChatClient::onApiResponse);
    connect(m_apiManager, &ApiManager::requestStarted, this, &ChatClient::onRequestStarted);

    m_configDialog = new ConfigDialog(this);
    connect(m_configDialog, &QDialog::accepted, [this]() {
        m_apiManager->setConfig(m_configDialog->getConfig());
        m_userName = m_configDialog->getUserName();
        m_botName = m_configDialog->getBotName();
        m_userAvatarPath = m_configDialog->getUserAvatarPath();
        m_botAvatarPath = m_configDialog->getBotAvatarPath();
        if (!m_userAvatarPath.isEmpty()) {
            m_userAvatar.load(m_userAvatarPath);
        }
        if (!m_botAvatarPath.isEmpty()) {
            m_botAvatar.load(m_botAvatarPath);
        }
        saveAvatarConfig();
        m_statusLabel->setText(QString::fromUtf8("配置已更新"));
    });

    m_tcpClient = new TcpClient(this);
    connect(m_tcpClient, &TcpClient::connected, this, &ChatClient::onTcpConnected);
    connect(m_tcpClient, &TcpClient::disconnected, this, &ChatClient::onTcpDisconnected);
    connect(m_tcpClient, &TcpClient::responseReceived, this, &ChatClient::onTcpResponseReceived);
    connect(m_tcpClient, &TcpClient::errorOccurred, this, &ChatClient::onTcpError);

    m_pythonProcess = new PythonProcessManager(this);
    connect(m_pythonProcess, &PythonProcessManager::processStarted, this, &ChatClient::onPythonProcessStarted);
    connect(m_pythonProcess, &PythonProcessManager::processFinished, this, &ChatClient::onPythonProcessFinished);
    connect(m_pythonProcess, &PythonProcessManager::processError, this, &ChatClient::onPythonProcessError);

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &ChatClient::onReconnectTimer);

    setupUI();
    applyStyles();

    loadConfig();
    loadAvatarConfig();
}

ChatClient::~ChatClient()
{
    if (m_pythonProcess) {
        m_pythonProcess->stopProcess();
    }
    saveConfig();
}

void ChatClient::initializePythonProcess()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString logPath = appDir + "/debug_log.txt";
    
    QFile logFile(logPath);
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qDebug() << "Failed to open log file:" << logPath;
    }
    QTextStream logStream(&logFile);
    
    qDebug() << "Application dir:" << appDir;
    logStream << "Application dir: " << appDir << "\n";
    logStream << "Log file path: " << logPath << "\n";
    
    QStringList possiblePaths;
    
    possiblePaths << appDir + QLatin1String("/../ConnectOpenclaw/openclaw_client_working.py");
    possiblePaths << appDir + QLatin1String("/../../ConnectOpenclaw/openclaw_client_working.py");
    possiblePaths << QLatin1String("D:/SmartWork/1/SmartWork/ConnectOpenclaw/openclaw_client_working.py");
    
    QString scriptPath;
    for (const QString& path : possiblePaths) {
        QString normalizedPath = QDir::cleanPath(path);
        qDebug() << "Checking path:" << normalizedPath;
        logStream << "Checking path: " << normalizedPath << "\n";
        if (QFile::exists(normalizedPath)) {
            scriptPath = normalizedPath;
            logStream << "Found script at: " << normalizedPath << "\n";
            break;
        }
    }
    
    if (scriptPath.isEmpty()) {
        QString errorMsg = QString::fromUtf8("Python脚本不存在");
        qDebug() << errorMsg;
        logStream << errorMsg << "\n";
        logStream.flush();
        m_statusLabel->setText(errorMsg);
        return;
    }
    
    qDebug() << "Python script path:" << scriptPath;
    logStream << "Python script path: " << scriptPath << "\n";

    QString pythonPath = QLatin1String("C:/Users/cbb123/AppData/Local/Programs/Python/Python310/python.exe");
    
    if (!QFile::exists(pythonPath)) {
        logStream << "Hardcoded Python path not found, searching PATH...\n";
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QString pathEnv = env.value(QLatin1String("PATH"));

        QStringList paths = pathEnv.split(QLatin1Char(';'));
        for (const QString& path : paths) {
            QString pythonExe = path.trimmed() + QLatin1String("/python.exe");
            if (QFile::exists(pythonExe)) {
                pythonPath = pythonExe;
                qDebug() << "Found Python:" << pythonPath;
                logStream << "Found Python: " << pythonPath << "\n";
                break;
            }
        }
    } else {
        qDebug() << "Using hardcoded Python path:" << pythonPath;
        logStream << "Using hardcoded Python path: " << pythonPath << "\n";
    }

    if (pythonPath.isEmpty()) {
        pythonPath = QLatin1String("python");
        qDebug() << "Using default python command";
        logStream << "Using default python command\n";
    }

    m_statusLabel->setText(QString::fromUtf8("正在启动Python服务..."));
    logStream << "Starting Python process...\n";
    logStream.flush();

    if (!m_pythonProcess->startProcess(pythonPath, scriptPath)) {
        m_statusLabel->setText(QString::fromUtf8("启动Python服务失败"));
        logStream << "Failed to start Python process\n";
    }
    
    logStream.flush();
    logFile.close();
}

void ChatClient::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(10);

    auto* headerWidget = new QWidget(this);
    auto* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(10);

    QString configIconPath = QLatin1String(LAppDefine::ResourcesPath) + QLatin1String(LAppDefine::GearImageName);
    QPixmap configPixmap(configIconPath);
    QIcon configIcon(configPixmap);

    m_configButton = new QPushButton(this);
    if (!configPixmap.isNull()) {
        m_configButton->setIcon(configIcon);
        m_configButton->setIconSize(QSize(24, 24));
        m_configButton->setText(QLatin1String(""));
    } else {
        m_configButton->setText(QChar(0x2699));
    }
    m_configButton->setObjectName(QLatin1String("configButton"));
    m_configButton->setToolTip(QString::fromUtf8("设置"));
    m_configButton->setFixedSize(40, 40);

    auto* titleLabel = new QLabel(QString::fromUtf8("SmartWork Chat"), this);
    titleLabel->setStyleSheet(QLatin1String("color: #333; font-size: 16px; font-weight: bold;"));

    m_statusLabel = new QLabel(QString::fromUtf8("就绪"), this);
    m_statusLabel->setStyleSheet(QLatin1String("color: #666; font-size: 12px;"));

    m_exportButton = new QPushButton(this);
    m_exportButton->setText(QChar(0x25A3));
    m_exportButton->setObjectName(QLatin1String("exportButton"));
    m_exportButton->setToolTip(QString::fromUtf8("导出对话"));
    m_exportButton->setFixedSize(40, 40);

    headerLayout->addWidget(m_configButton);
    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(m_statusLabel, 1);
    headerLayout->addWidget(m_exportButton);

    m_chatDisplay = new QTextEdit(this);
    m_chatDisplay->setReadOnly(true);
    m_chatDisplay->setObjectName(QLatin1String("chatDisplay"));

    auto* inputContainer = new QWidget(this);
    auto* inputLayout = new QHBoxLayout(inputContainer);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(10);

    m_inputEdit = new QLineEdit(this);
    m_inputEdit->setPlaceholderText(QString::fromUtf8("输入消息..."));
    m_inputEdit->setObjectName(QLatin1String("inputEdit"));

    m_sendButton = new QPushButton(QString::fromUtf8("发送"), this);
    m_sendButton->setObjectName(QLatin1String("sendButton"));
    m_sendButton->setFixedSize(80, 45);

    inputLayout->addWidget(m_inputEdit);
    inputLayout->addWidget(m_sendButton);

    mainLayout->addWidget(headerWidget);
    mainLayout->addWidget(m_chatDisplay, 1);
    mainLayout->addWidget(inputContainer);

    connect(m_sendButton, &QPushButton::clicked, this, &ChatClient::onSendClicked);
    connect(m_inputEdit, &QLineEdit::returnPressed, this, &ChatClient::onReturnPressed);
    connect(m_configButton, &QPushButton::clicked, this, &ChatClient::onConfigClicked);
    connect(m_exportButton, &QPushButton::clicked, this, &ChatClient::onExportClicked);
    connect(m_chatDisplay->verticalScrollBar(), &QScrollBar::valueChanged, this, &ChatClient::onScrollChanged);

    QString welcome = QString::fromUtf8("欢迎使用 SmartWork Chat！\n\n正在连接Python服务...");
    addMessage(welcome, false);
}

void ChatClient::applyStyles()
{
    setStyleSheet(QLatin1String(
        "QWidget {"
        "    font-family: \"Microsoft YaHei UI\", \"Segoe UI\", sans-serif;"
        "}"
        "QTextEdit#chatDisplay {"
        "    border: none;"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f8f9fa, stop:1 #e9ecef);"
        "    border-radius: 12px;"
        "    padding: 15px;"
        "}"
        "QLineEdit#inputEdit {"
        "    padding: 12px 18px;"
        "    border: 2px solid #e0e0e0;"
        "    border-radius: 24px;"
        "    background-color: white;"
        "    font-size: 14px;"
        "    color: #333;"
        "}"
        "QLineEdit#inputEdit:focus {"
        "    border-color: #007AFF;"
        "}"
        "QPushButton#sendButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #007AFF, stop:1 #0051D5);"
        "    color: white;"
        "    border: none;"
        "    border-radius: 22px;"
        "    font-size: 15px;"
        "    font-weight: 600;"
        "}"
        "QPushButton#sendButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0066FF, stop:1 #0044BB);"
        "}"
        "QPushButton#sendButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0044BB, stop:1 #003399);"
        "}"
        "QPushButton#sendButton:disabled {"
        "    background-color: #c0c0c0;"
        "    color: #808080;"
        "}"
        "QPushButton#configButton, QPushButton#exportButton, QPushButton#avatarConfigButton {"
        "    background-color: transparent;"
        "    color: #555;"
        "    border: 2px solid #e0e0e0;"
        "    border-radius: 20px;"
        "    font-size: 20px;"
        "}"
        "QPushButton#configButton:hover, QPushButton#exportButton:hover, QPushButton#avatarConfigButton:hover {"
        "    background-color: #f0f0f0;"
        "    border-color: #007AFF;"
        "    color: #007AFF;"
        "}"
        "QPushButton#configButton:pressed, QPushButton#exportButton:pressed, QPushButton#avatarConfigButton:pressed {"
        "    background-color: #e0e0e0;"
        "}"
    ));
}

void ChatClient::addMessage(const QString& message, bool isUser)
{
    qDebug() << "=== addMessage called ===";
    qDebug() << "isUser:" << isUser;
    qDebug() << "Message length:" << message.length();
    qDebug() << "Message preview:" << message.left(100);
    
    m_messages.emplace_back(message, isUser, getCurrentTime());

    QString html;
    QString displayName = isUser ? m_userName : m_botName;
    QString escapedName = displayName.toHtmlEscaped();
    QString escapedMessage = message.toHtmlEscaped().replace(QLatin1Char('\n'), QLatin1String("<br>"));
    QString avatarHtml = getAvatarHtml(isUser, 45);
    QString timeStr = getCurrentTime();
    
    if (isUser) {
        html += QString::fromLatin1(
            "<table width='100%' cellpadding='0' cellspacing='0' style='margin: 12px 0;'>"
            "<tr><td align='right' valign='bottom' style='padding-right: 8px;'>"
            "<table cellpadding='0' cellspacing='0'>"
            "<tr><td align='right' style='padding-bottom: 6px;'>"
            "<div style='background-color: #1E88E5; color: #000000; "
            "border-radius: 18px 18px 4px 18px; padding: 12px 16px; font-size: 14px; line-height: 1.6; "
            "max-width: 420px; min-width: 20px; word-wrap: break-word; display: inline-block; text-align: left; "
            "box-shadow: 0 3px 12px rgba(30, 136, 229, 0.35); margin: 2px;'>%3</div>"
            "</td></tr>"
            "<tr><td align='right' style='padding-right: 4px;'>"
            "<span style='font-size: 11px; color: #999; font-family: Arial, sans-serif;'>%1</span>"
            "</td></tr>"
            "</table>"
            "</td>"
            "<td width='70' align='center' valign='top' style='padding-left: 12px;'>"
            "<table cellpadding='0' cellspacing='0'>"
            "<tr><td align='center'>%4</td></tr>"
            "<tr><td align='center' style='padding-top: 4px;'>"
            "<span style='font-size: 11px; color: #666; font-family: Arial, sans-serif;'>%2</span>"
            "</td></tr>"
            "</table>"
            "</td></tr>"
            "</table>")
            .arg(timeStr)
            .arg(escapedName)
            .arg(escapedMessage)
            .arg(avatarHtml);
    } else {
        html += QString::fromLatin1(
            "<table width='100%' cellpadding='0' cellspacing='0' style='margin: 12px 0;'>"
            "<tr>"
            "<td width='70' align='center' valign='top' style='padding-right: 12px;'>"
            "<table cellpadding='0' cellspacing='0'>"
            "<tr><td align='center'>%1</td></tr>"
            "<tr><td align='center' style='padding-top: 4px;'>"
            "<span style='font-size: 11px; color: #666; font-family: Arial, sans-serif;'>%2</span>"
            "</td></tr>"
            "</table>"
            "</td>"
            "<td align='left' valign='bottom' style='padding-left: 8px;'>"
            "<table cellpadding='0' cellspacing='0'>"
            "<tr><td align='left' style='padding-bottom: 6px;'>"
            "<div style='background-color: #4CAF50; color: #000000; border-radius: 18px 18px 18px 4px; "
            "padding: 12px 16px; font-size: 14px; line-height: 1.6; max-width: 420px; min-width: 20px; "
            "word-wrap: break-word; display: inline-block; text-align: left; "
            "box-shadow: 0 3px 12px rgba(76, 175, 80, 0.35); margin: 2px;'>%4</div>"
            "</td></tr>"
            "<tr><td align='left' style='padding-left: 4px;'>"
            "<span style='font-size: 11px; color: #999; font-family: Arial, sans-serif;'>%3</span>"
            "</td></tr>"
            "</table>"
            "</td></tr>"
            "</table>")
            .arg(avatarHtml)
            .arg(escapedName)
            .arg(timeStr)
            .arg(escapedMessage);
    }

    m_chatDisplay->append(html);

    m_chatDisplay->verticalScrollBar()->setValue(
        m_chatDisplay->verticalScrollBar()->maximum()
    );
}

QString ChatClient::getAvatarLetter(const QString& name)
{
    if (name.isEmpty()) {
        return QString::fromLatin1("?");
    }
    return QString(name.at(0)).toUpper();
}

QString ChatClient::getAvatarHtml(bool isUser, int size)
{
    QString avatarLetter = getAvatarLetter(isUser ? m_userName : m_botName);
    QString base64Image;
    
    QPixmap avatar = isUser ? m_userAvatar : m_botAvatar;
    if (!avatar.isNull()) {
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        buffer.open(QIODevice::WriteOnly);
        avatar.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation).save(&buffer, "PNG");
        buffer.close();
        base64Image = QString::fromLatin1("data:image/png;base64,") + byteArray.toBase64();
    }
    
    QString gradientStart = isUser ? QLatin1String("#667eea") : QLatin1String("#11998e");
    QString gradientEnd = isUser ? QLatin1String("#764ba2") : QLatin1String("#38ef7d");
    
    if (!base64Image.isEmpty()) {
        return QString::fromLatin1(
            "<div style='width: %1px; height: %1px; border-radius: %2px; "
            "background: linear-gradient(135deg, %3 0%, %4 100%); "
            "display: flex; align-items: center; justify-content: center; "
            "overflow: hidden; box-shadow: 0 2px 8px rgba(0, 0, 0, 0.15);'>"
            "<img src='%5' style='width: 100%; height: 100%; object-fit: cover;'/>"
            "</div>")
            .arg(size)
            .arg(size / 2)
            .arg(gradientStart)
            .arg(gradientEnd)
            .arg(base64Image);
    }
    
    return QString::fromLatin1(
        "<div style='width: %1px; height: %1px; border-radius: %2px; "
        "background: linear-gradient(135deg, %3 0%, %4 100%); "
        "display: flex; align-items: center; justify-content: center; "
        "color: white; font-weight: bold; font-size: %5px; "
        "box-shadow: 0 2px 8px rgba(0, 0, 0, 0.15);'>"
        "%6</div>")
        .arg(size)
        .arg(size / 2)
        .arg(gradientStart)
        .arg(gradientEnd)
        .arg(size * 0.4)
        .arg(avatarLetter);
}

void ChatClient::onScrollChanged(int value)
{
    Q_UNUSED(value);
}

void ChatClient::loadAvatarConfig()
{
    QString path = getAvatarConfigPath();
    QFile file(path);
    if (file.exists()) {
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            QJsonObject root = doc.object();
            m_userName = root[QLatin1String("userName")].toString(QString::fromUtf8("用户"));
            m_botName = root[QLatin1String("botName")].toString(QString::fromUtf8("助手"));
            m_userAvatarPath = root[QLatin1String("userAvatarPath")].toString();
            m_botAvatarPath = root[QLatin1String("botAvatarPath")].toString();
            file.close();
            
            if (!m_userAvatarPath.isEmpty() && QFile::exists(m_userAvatarPath)) {
                m_userAvatar.load(m_userAvatarPath);
            }
            if (!m_botAvatarPath.isEmpty() && QFile::exists(m_botAvatarPath)) {
                m_botAvatar.load(m_botAvatarPath);
            }
        }
    }
    
    if (m_userAvatar.isNull()) {
        m_userAvatar = createDefaultAvatar(m_userName, QColor(102, 126, 234));
    }
    if (m_botAvatar.isNull()) {
        m_botAvatar = createDefaultAvatar(m_botName, QColor(17, 153, 142));
    }
}

void ChatClient::saveAvatarConfig()
{
    QString path = getAvatarConfigPath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    
    QJsonObject root;
    root[QLatin1String("userName")] = m_userName;
    root[QLatin1String("botName")] = m_botName;
    root[QLatin1String("userAvatarPath")] = m_userAvatarPath;
    root[QLatin1String("botAvatarPath")] = m_botAvatarPath;
    
    QJsonDocument doc(root);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

QString ChatClient::getAvatarConfigPath() const
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return configDir + QLatin1String("/avatar_config.json");
}

void ChatClient::refreshMessages()
{
    m_chatDisplay->clear();
    for (const auto& msg : m_messages) {
        QString html;
        QString displayName = msg.isUser ? m_userName : m_botName;
        QString escapedName = displayName.toHtmlEscaped();
        QString escapedMessage = msg.content.toHtmlEscaped().replace(QLatin1Char('\n'), QLatin1String("<br>"));
        QString avatarHtml = getAvatarHtml(msg.isUser, 45);
        QString timeStr = msg.timestamp;
        
        if (msg.isUser) {
            html += QString::fromLatin1(
                "<div style='display: flex; justify-content: flex-end; margin: 8px 0; padding: 5px;'>"
                "<div style='flex: 1; max-width: 65%; display: flex; flex-direction: column; align-items: flex-end; margin-right: 12px;'>"
                "<div style='display: flex; align-items: center; margin-bottom: 4px;'>"
                "<span style='font-size: 11px; color: #999; margin-right: 8px;'>%1</span>"
                "<span style='font-size: 13px; color: #333; font-weight: 600;'>%2</span>"
                "</div>"
                "<div style='background-color: #007AFF; color: white; border-radius: 16px 16px 4px 16px; "
                "padding: 12px 16px; font-size: 14px; line-height: 1.5; word-wrap: break-word; "
                "max-width: 100%; box-shadow: 0 2px 8px rgba(0, 122, 255, 0.25); "
                "border: 1px solid rgba(255, 255, 255, 0.2);'>"
                "%3</div>"
                "</div>"
                "%4"
                "</div>")
                .arg(timeStr)
                .arg(escapedName)
                .arg(escapedMessage)
                .arg(avatarHtml);
        } else {
            html += QString::fromLatin1(
                "<div style='display: flex; justify-content: flex-start; margin: 8px 0; padding: 5px;'>"
                "%1"
                "<div style='flex: 1; max-width: 65%; display: flex; flex-direction: column; align-items: flex-start; margin-left: 12px;'>"
                "<div style='display: flex; align-items: center; margin-bottom: 4px;'>"
                "<span style='font-size: 13px; color: #333; font-weight: 600;'>%2</span>"
                "<span style='font-size: 11px; color: #999; margin-left: 8px;'>%3</span>"
                "</div>"
                "<div style='background-color: #ffffff; color: #333; border-radius: 16px 16px 16px 4px; "
                "padding: 12px 16px; font-size: 14px; line-height: 1.5; word-wrap: break-word; "
                "max-width: 100%; box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08); "
                "border: 1px solid #e8e8e8;'>"
                "%4</div>"
                "</div>"
                "</div>")
                .arg(avatarHtml)
                .arg(escapedName)
                .arg(timeStr)
                .arg(escapedMessage);
        }
        m_chatDisplay->append(html);
    }
    
    m_chatDisplay->verticalScrollBar()->setValue(
        m_chatDisplay->verticalScrollBar()->maximum()
    );
}

QPixmap ChatClient::createDefaultAvatar(const QString& name, const QColor& color)
{
    QPixmap pix(64, 64);
    pix.fill(Qt::transparent);

    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);

    QRadialGradient gradient(32, 32, 32);
    gradient.setColorAt(0, color.lighter(120));
    gradient.setColorAt(1, color);
    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, 64, 64);

    QString letter = name.isEmpty() ? QString::fromLatin1("?") : QString(name.at(0)).toUpper();
    QFont font = painter.font();
    font.setPointSize(28);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(pix.rect(), Qt::AlignCenter, letter);

    return pix;
}

void ChatClient::onSendClicked()
{
    QString message = m_inputEdit->text().trimmed();

    if (message.isEmpty()) {
        return;
    }

    if (m_useTcpMode) {
        if (!m_tcpClient->isConnected()) {
            QMessageBox::warning(this, QString::fromUtf8("未连接"), QString::fromUtf8("Python服务未连接，请稍候..."));
            return;
        }

        addMessage(message, true);
        m_inputEdit->clear();

        m_isRequestInProgress = true;
        m_sendButton->setEnabled(false);
        m_statusLabel->setText(QString::fromUtf8("正在思考..."));

        m_tcpClient->sendMessage(message);
        emit messageSent(message);
    } else {
        if (m_apiManager->getConfig().apiKey.isEmpty()) {
            QMessageBox::warning(this, QString::fromUtf8("未配置"), QString::fromUtf8("请先配置API Key"));
            onConfigClicked();
            return;
        }

        addMessage(message, true);
        m_inputEdit->clear();

        m_apiManager->sendMessage(message, getHistoryForApi());

        emit messageSent(message);
    }
}

void ChatClient::onReturnPressed()
{
    onSendClicked();
}

void ChatClient::onConfigClicked()
{
    m_configDialog->setConfig(m_apiManager->getConfig());
    m_configDialog->setUserName(m_userName);
    m_configDialog->setBotName(m_botName);
    m_configDialog->setUserAvatarPath(m_userAvatarPath);
    m_configDialog->setBotAvatarPath(m_botAvatarPath);
    m_configDialog->exec();
}

void ChatClient::onExportClicked()
{
    if (m_messages.empty()) {
        QMessageBox::information(this, QString::fromUtf8("导出"), QString::fromUtf8("没有对话记录可导出"));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this,
        QString::fromUtf8("导出对话"),
        QLatin1String("chat_") + QDateTime::currentDateTime().toString(QLatin1String("yyyyMMdd_hhmmss")) + QLatin1String(".json"),
        QString::fromUtf8("JSON Files (*.json);;Text Files (*.txt);;Markdown Files (*.md)")
    );

    if (fileName.isEmpty()) {
        return;
    }

    if (fileName.endsWith(QLatin1String(".json"))) {
        QJsonArray messages;
        for (const auto& msg : m_messages) {
            QJsonObject obj;
            obj[QLatin1String("content")] = msg.content;
            obj[QLatin1String("isUser")] = msg.isUser;
            obj[QLatin1String("timestamp")] = msg.timestamp;
            messages.append(obj);
        }

        QJsonDocument doc(messages);
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(doc.toJson());
            file.close();
            m_statusLabel->setText(QString::fromUtf8("已导出JSON: ") + QFileInfo(fileName).fileName());
        }
    } else if (fileName.endsWith(QLatin1String(".md"))) {
        QString markdown = QString::fromUtf8("# SmartWork Chat 对话记录\n\n");
        markdown += QString::fromUtf8("导出时间: ") + QDateTime::currentDateTime().toString(QLatin1String("yyyy-MM-dd hh:mm:ss")) + QLatin1String("\n\n---\n\n");

        for (const auto& msg : m_messages) {
            QString role = msg.isUser ? QString::fromUtf8("用户") : QString::fromUtf8("AI");
            markdown += QString::fromUtf8("### [") + msg.timestamp + QString::fromUtf8("] ") + role + QLatin1String("\n\n");
            markdown += msg.content + QLatin1String("\n\n---\n\n");
        }

        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(markdown.toUtf8());
            file.close();
            m_statusLabel->setText(QString::fromUtf8("已导出Markdown: ") + QFileInfo(fileName).fileName());
        }
    } else {
        QString text = QString::fromUtf8("SmartWork Chat 对话记录\n");
        text += QString::fromUtf8("导出时间: ") + QDateTime::currentDateTime().toString(QLatin1String("yyyy-MM-dd hh:mm:ss")) + QLatin1String("\n");
        text += QString(QChar('=')).repeated(50) + QLatin1String("\n\n");

        for (const auto& msg : m_messages) {
            QString role = msg.isUser ? QString::fromUtf8("用户") : QString::fromUtf8("AI");
            text += QString::fromUtf8("[") + msg.timestamp + QString::fromUtf8("] ") + role + QLatin1String(":\n");
            text += msg.content + QLatin1String("\n");
            text += QString(QChar('-')).repeated(30) + QLatin1String("\n\n");
        }

        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(text.toUtf8());
            file.close();
            m_statusLabel->setText(QString::fromUtf8("已导出TXT: ") + QFileInfo(fileName).fileName());
        }
    }
}

void ChatClient::onRequestStarted()
{
    m_isRequestInProgress = true;
    m_sendButton->setEnabled(false);
    m_statusLabel->setText(QString::fromUtf8("正在思考..."));
}

void ChatClient::onApiResponse(const ApiResponse& response)
{
    m_isRequestInProgress = false;
    m_sendButton->setEnabled(true);

    if (response.success) {
        addMessage(response.content, false);
        m_statusLabel->setText(QString::fromUtf8("就绪"));
    } else {
        QString errorMsg = response.error;
        if (response.statusCode > 0) {
            errorMsg = QString::fromLatin1("HTTP %1: %2").arg(response.statusCode).arg(errorMsg);
        }
        QMessageBox::critical(this, QString::fromUtf8("API 错误"), errorMsg);
        m_statusLabel->setText(QString::fromUtf8("错误: ") + errorMsg.left(30) + QLatin1String("..."));
    }
}

void ChatClient::onTcpConnected()
{
    qDebug() << "TCP connected successfully";
    m_statusLabel->setText(QString::fromUtf8("已连接Python服务"));
    m_reconnectAttempts = 0;

    if (m_messages.size() == 1) {
        m_messages.clear();
        m_chatDisplay->clear();
        QString welcome = QString::fromUtf8("已连接Python服务，可以开始对话！");
        addMessage(welcome, false);
    }
}

void ChatClient::onTcpDisconnected()
{
    qDebug() << "TCP disconnected";
    m_statusLabel->setText(QString::fromUtf8("Python服务已断开"));

    if (m_isRequestInProgress) {
        m_isRequestInProgress = false;
        m_sendButton->setEnabled(true);
    }
}

void ChatClient::onTcpResponseReceived(const QString& content, bool success)
{
    qDebug() << "=== onTcpResponseReceived called ===";
    qDebug() << "Content length:" << content.length();
    qDebug() << "Content preview:" << content.left(100);
    qDebug() << "m_isRequestInProgress:" << m_isRequestInProgress;
    
    m_isRequestInProgress = false;
    m_sendButton->setEnabled(true);

    if (success) {
        addMessage(content, false);
        m_statusLabel->setText(QString::fromUtf8("就绪"));
    } else {
        QMessageBox::critical(this, QString::fromUtf8("错误"), content);
        m_statusLabel->setText(QString::fromUtf8("错误: ") + content.left(30) + QLatin1String("..."));
    }
}

void ChatClient::onTcpError(const QString& error)
{
    qDebug() << "TCP error:" << error;
    m_statusLabel->setText(QString::fromUtf8("TCP错误: ") + error.left(20) + QLatin1String("..."));
}

void ChatClient::onPythonProcessStarted()
{
    qDebug() << "Python process started, waiting for TCP service...";
    m_statusLabel->setText(QString::fromUtf8("Python服务已启动，等待连接..."));
    
    m_reconnectAttempts = 0;
    m_reconnectTimer->start(1000);
}

void ChatClient::onPythonProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "Python process finished, exit code:" << exitCode;
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);
    m_statusLabel->setText(QString::fromUtf8("Python服务已停止"));
    m_tcpClient->disconnectFromServer();
    m_reconnectTimer->stop();
}

void ChatClient::onPythonProcessError(const QString& error)
{
    qDebug() << "Python process error:" << error;
    m_statusLabel->setText(QString::fromUtf8("Python进程错误: ") + error);
}

void ChatClient::onReconnectTimer()
{
    if (m_tcpClient->isConnected()) {
        qDebug() << "Already connected";
        return;
    }

    if (m_reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
        m_reconnectAttempts++;
        qDebug() << "Attempting to connect (" << m_reconnectAttempts << "/" << MAX_RECONNECT_ATTEMPTS << ")";
        m_statusLabel->setText(QString::fromUtf8("正在连接Python服务 (%1/%2)...").arg(m_reconnectAttempts).arg(MAX_RECONNECT_ATTEMPTS));
        m_tcpClient->connectToServer(TCP_HOST, TCP_PORT);
        
        m_reconnectTimer->start(RECONNECT_INTERVAL_MS);
    } else {
        qDebug() << "Max reconnect attempts reached";
        m_statusLabel->setText(QString::fromUtf8("无法连接Python服务，请检查Python环境"));
    }
}

void ChatClient::tryConnectToPython()
{
    m_reconnectAttempts = 0;
    m_tcpClient->connectToServer(TCP_HOST, TCP_PORT);
}

QString ChatClient::getCurrentTime() const
{
    return QDateTime::currentDateTime().toString(QLatin1String("hh:mm:ss"));
}

void ChatClient::setApiConfig(const ApiConfig& config)
{
    m_apiManager->setConfig(config);
}

ApiConfig ChatClient::getApiConfig() const
{
    return m_apiManager->getConfig();
}

void ChatClient::loadConfig()
{
    QFile file(m_configPath);
    if (file.exists()) {
        m_configDialog->loadFromFile(m_configPath);
        m_apiManager->setConfig(m_configDialog->getConfig());
    }
}

void ChatClient::saveConfig()
{
    QDir().mkpath(QFileInfo(m_configPath).absolutePath());
    m_configDialog->setConfig(m_apiManager->getConfig());
    m_configDialog->saveToFile(m_configPath);
}

QJsonArray ChatClient::getHistoryForApi() const
{
    QJsonArray history;

    for (size_t i = 1; i < m_messages.size(); ++i) {
        const auto& msg = m_messages[i];

        QJsonObject msgObj;
        msgObj[QLatin1String("role")] = msg.isUser ? QLatin1String("user") : QLatin1String("assistant");
        msgObj[QLatin1String("content")] = msg.content;

        history.append(msgObj);
    }

    return history;
}
