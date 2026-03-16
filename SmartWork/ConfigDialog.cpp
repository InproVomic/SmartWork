#include "ConfigDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPixmap>

// Default DeepSeek API Key
static const QLatin1String DEFAULT_DEEPSEEK_API_KEY("sk-64dc23582916442386a2db13b867dafa");

ConfigDialog::ConfigDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QString::fromUtf8("配置 API"));
    resize(520, 620);

    setupUI();
    applyStyles();

    // Load saved settings
    loadFromFile(getConfigPath());

    // Set default API key if not configured
    if (m_apiKeyEdit->text().isEmpty()) {
        m_apiKeyEdit->setText(DEFAULT_DEEPSEEK_API_KEY);
    }
}

ConfigDialog::~ConfigDialog()
{
}

void ConfigDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(12);

    auto* profileGroup = new QGroupBox(QString::fromUtf8("个人设置"), this);
    auto* profileLayout = new QGridLayout();
    profileLayout->setSpacing(10);

    m_userNameEdit = new QLineEdit(this);
    m_userNameEdit->setText(QString::fromUtf8("用户"));
    m_userNameEdit->setPlaceholderText(QString::fromUtf8("输入您的昵称"));
    
    m_userAvatarPreview = new QLabel(this);
    m_userAvatarPreview->setFixedSize(50, 50);
    m_userAvatarPreview->setAlignment(Qt::AlignCenter);
    m_userAvatarPreview->setStyleSheet(QLatin1String(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #667eea, stop:1 #764ba2); "
        "border-radius: 25px; color: white; font-weight: bold; font-size: 20px;"));
    m_userAvatarPreview->setText(QString::fromUtf8("用"));
    
    m_userAvatarBtn = new QPushButton(QString::fromUtf8("选择头像"), this);
    m_userAvatarBtn->setObjectName(QLatin1String("secondary"));

    profileLayout->addWidget(new QLabel(QString::fromUtf8("用户昵称:")), 0, 0);
    profileLayout->addWidget(m_userNameEdit, 0, 1);
    profileLayout->addWidget(new QLabel(QString::fromUtf8("用户头像:")), 1, 0);
    profileLayout->addWidget(m_userAvatarPreview, 1, 1);
    profileLayout->addWidget(m_userAvatarBtn, 1, 2);

    m_botNameEdit = new QLineEdit(this);
    m_botNameEdit->setText(QString::fromUtf8("助手"));
    m_botNameEdit->setPlaceholderText(QString::fromUtf8("输入机器人昵称"));
    
    m_botAvatarPreview = new QLabel(this);
    m_botAvatarPreview->setFixedSize(50, 50);
    m_botAvatarPreview->setAlignment(Qt::AlignCenter);
    m_botAvatarPreview->setStyleSheet(QLatin1String(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #11998e, stop:1 #38ef7d); "
        "border-radius: 25px; color: white; font-weight: bold; font-size: 20px;"));
    m_botAvatarPreview->setText(QString::fromUtf8("助"));
    
    m_botAvatarBtn = new QPushButton(QString::fromUtf8("选择头像"), this);
    m_botAvatarBtn->setObjectName(QLatin1String("secondary"));

    profileLayout->addWidget(new QLabel(QString::fromUtf8("机器人昵称:")), 2, 0);
    profileLayout->addWidget(m_botNameEdit, 2, 1);
    profileLayout->addWidget(new QLabel(QString::fromUtf8("机器人头像:")), 3, 0);
    profileLayout->addWidget(m_botAvatarPreview, 3, 1);
    profileLayout->addWidget(m_botAvatarBtn, 3, 2);
    
    profileGroup->setLayout(profileLayout);

    auto* providerGroup = new QGroupBox(QString::fromUtf8("API 提供商"), this);
    auto* providerLayout = new QFormLayout();

    m_providerCombo = new QComboBox(this);
    m_providerCombo->addItem(QLatin1String("DeepSeek"), QLatin1String("deepseek"));
    m_providerCombo->addItem(QLatin1String("OpenAI"), QLatin1String("openai"));
    m_providerCombo->addItem(QString::fromUtf8("自定义"), QLatin1String("custom"));

    providerLayout->addRow(QString::fromUtf8("提供商:"), m_providerCombo);
    providerGroup->setLayout(providerLayout);

    // Authentication
    auto* authGroup = new QGroupBox(QString::fromUtf8("认证"), this);
    auto* authLayout = new QFormLayout();

    m_apiKeyEdit = new QLineEdit(this);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText(QLatin1String("sk-..."));

    m_baseUrlEdit = new QLineEdit(this);
    m_baseUrlEdit->setText(QLatin1String("https://api.deepseek.com/v1"));

    authLayout->addRow(QLatin1String("API Key:"), m_apiKeyEdit);
    authLayout->addRow(QString::fromUtf8("Base URL:"), m_baseUrlEdit);
    authGroup->setLayout(authLayout);

    // Model Settings
    auto* modelGroup = new QGroupBox(QString::fromUtf8("模型设置"), this);
    auto* modelLayout = new QFormLayout();

    m_presetModelCombo = new QComboBox(this);
    m_presetModelCombo->addItem(QLatin1String("deepseek-chat"), QLatin1String("deepseek-chat"));
    m_presetModelCombo->addItem(QLatin1String("deepseek-coder"), QLatin1String("deepseek-coder"));
    m_presetModelCombo->addItem(QLatin1String("gpt-3.5-turbo"), QLatin1String("gpt-3.5-turbo"));
    m_presetModelCombo->addItem(QLatin1String("gpt-4"), QLatin1String("gpt-4"));
    m_presetModelCombo->addItem(QLatin1String("gpt-4-turbo"), QLatin1String("gpt-4-turbo"));
    m_presetModelCombo->addItem(QString::fromUtf8("自定义"), QString());

    m_modelEdit = new QLineEdit(this);
    m_modelEdit->setText(QLatin1String("deepseek-chat"));

    modelLayout->addRow(QString::fromUtf8("预设模型:"), m_presetModelCombo);
    modelLayout->addRow(QString::fromUtf8("模型名称:"), m_modelEdit);
    modelGroup->setLayout(modelLayout);

    // Generation Parameters
    auto* paramGroup = new QGroupBox(QString::fromUtf8("生成参数"), this);
    auto* paramLayout = new QFormLayout();

    m_temperatureSpin = new QDoubleSpinBox(this);
    m_temperatureSpin->setRange(0.0, 2.0);
    m_temperatureSpin->setSingleStep(0.1);
    m_temperatureSpin->setValue(0.7);

    m_maxTokensSpin = new QSpinBox(this);
    m_maxTokensSpin->setRange(1, 128000);
    m_maxTokensSpin->setValue(4096);

    m_topPSpin = new QDoubleSpinBox(this);
    m_topPSpin->setRange(0.0, 1.0);
    m_topPSpin->setSingleStep(0.05);
    m_topPSpin->setValue(1.0);

    paramLayout->addRow(QString::fromUtf8("温度:"), m_temperatureSpin);
    paramLayout->addRow(QString::fromUtf8("最大令牌:"), m_maxTokensSpin);
    paramLayout->addRow(QLatin1String("Top P:"), m_topPSpin);
    paramGroup->setLayout(paramLayout);

    // System Prompt
    auto* promptGroup = new QGroupBox(QString::fromUtf8("系统提示"), this);
    auto* promptLayout = new QVBoxLayout();

    m_systemPromptEdit = new QTextEdit(this);
    m_systemPromptEdit->setPlainText(QString::fromUtf8("你是一个有帮助的AI助手。"));
    m_systemPromptEdit->setMaximumHeight(100);

    promptLayout->addWidget(m_systemPromptEdit);
    promptGroup->setLayout(promptLayout);

    // Buttons
    auto* buttonLayout = new QHBoxLayout();

    m_testButton = new QPushButton(QString::fromUtf8("测试连接"), this);
    m_saveButton = new QPushButton(QString::fromUtf8("保存配置"), this);
    m_loadButton = new QPushButton(QString::fromUtf8("加载配置"), this);

    auto* okButton = new QPushButton(QString::fromUtf8("确定"), this);
    auto* cancelButton = new QPushButton(QString::fromUtf8("取消"), this);

    buttonLayout->addWidget(m_testButton);
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_loadButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    // Add all to main layout
    mainLayout->addWidget(profileGroup);
    mainLayout->addWidget(providerGroup);
    mainLayout->addWidget(authGroup);
    mainLayout->addWidget(modelGroup);
    mainLayout->addWidget(paramGroup);
    mainLayout->addWidget(promptGroup);
    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_testButton, &QPushButton::clicked, this, &ConfigDialog::onTestConnection);
    connect(m_userAvatarBtn, &QPushButton::clicked, this, &ConfigDialog::onSelectUserAvatar);
    connect(m_botAvatarBtn, &QPushButton::clicked, this, &ConfigDialog::onSelectBotAvatar);
    connect(m_saveButton, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getSaveFileName(this, QString::fromUtf8("保存配置"), QString(), QLatin1String("JSON Files (*.json)"));
        if (!path.isEmpty()) {
            saveToFile(path);
            QMessageBox::information(this, QString::fromUtf8("成功"), QString::fromUtf8("配置已保存"));
        }
    });
    connect(m_loadButton, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, QString::fromUtf8("加载配置"), QString(), QLatin1String("JSON Files (*.json)"));
        if (!path.isEmpty()) {
            loadFromFile(path);
            QMessageBox::information(this, QString::fromUtf8("成功"), QString::fromUtf8("配置已加载"));
        }
    });

    // Provider combo change
    connect(m_providerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
        QString provider = m_providerCombo->currentData().toString();
        if (provider == QLatin1String("deepseek")) {
            m_baseUrlEdit->setText(QLatin1String("https://api.deepseek.com/v1"));
            m_modelEdit->setText(QLatin1String("deepseek-chat"));
        } else if (provider == QLatin1String("openai")) {
            m_baseUrlEdit->setText(QLatin1String("https://api.openai.com/v1"));
            m_modelEdit->setText(QLatin1String("gpt-3.5-turbo"));
        }
    });

    // Preset model combo change
    connect(m_presetModelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
        QString model = m_presetModelCombo->currentData().toString();
        if (!model.isEmpty()) {
            m_modelEdit->setText(model);
        }
    });
}

void ConfigDialog::applyStyles()
{
    setStyleSheet(QLatin1String(
        /* Dialog base */
        "QDialog {"
        "    font-family: \"Microsoft YaHei UI\", \"Segoe UI\", sans-serif;"
        "    background-color: #f8f9fa;"
        "}"
        /* Group Box */
        "QGroupBox {"
        "    font-weight: 600;"
        "    border: 2px solid #e0e0e0;"
        "    border-radius: 8px;"
        "    margin-top: 12px;"
        "    padding-top: 15px;"
        "    background-color: white;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 15px;"
        "    padding: 0 8px;"
        "    color: #333;"
        "}"
        /* Input fields */
        "QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {"
        "    padding: 8px 12px;"
        "    border: 2px solid #e0e0e0;"
        "    border-radius: 6px;"
        "    background-color: white;"
        "    font-size: 13px;"
        "    color: #333;"
        "}"
        "QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus {"
        "    border-color: #007AFF;"
        "}"
        "QComboBox::drop-down {"
        "    border: none;"
        "    width: 20px;"
        "}"
        "QComboBox::down-arrow {"
        "    image: none;"
        "    border-left: 5px solid transparent;"
        "    border-right: 5px solid transparent;"
        "    border-top: 5px solid #666;"
        "    width: 0;"
        "    height: 0;"
        "}"
        /* Text Edit */
        "QTextEdit {"
        "    border: 2px solid #e0e0e0;"
        "    border-radius: 6px;"
        "    background-color: white;"
        "    padding: 8px;"
        "    font-size: 13px;"
        "    color: #333;"
        "}"
        "QTextEdit:focus {"
        "    border-color: #007AFF;"
        "}"
        /* Primary Buttons */
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #007AFF, stop:1 #0051D5);"
        "    color: white;"
        "    border: none;"
        "    border-radius: 6px;"
        "    padding: 10px 18px;"
        "    font-size: 13px;"
        "    font-weight: 600;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0066FF, stop:1 #0044BB);"
        "}"
        "QPushButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0044BB, stop:1 #003399);"
        "}"
        /* Secondary Buttons */
        "QPushButton#secondary {"
        "    background-color: #f0f0f0;"
        "    color: #333;"
        "    font-weight: 500;"
        "}"
        "QPushButton#secondary:hover {"
        "    background-color: #e0e0e0;"
        "}"
        "QPushButton#secondary:pressed {"
        "    background-color: #d0d0d0;"
        "}"
    ));

    m_saveButton->setObjectName(QLatin1String("secondary"));
    m_loadButton->setObjectName(QLatin1String("secondary"));
    m_testButton->setObjectName(QLatin1String("secondary"));
}

ApiConfig ConfigDialog::getConfig() const
{
    ApiConfig config;
    config.apiKey = m_apiKeyEdit->text();
    config.baseUrl = m_baseUrlEdit->text();
    config.model = m_modelEdit->text();
    config.systemPrompt = m_systemPromptEdit->toPlainText();
    config.temperature = m_temperatureSpin->value();
    config.maxTokens = m_maxTokensSpin->value();
    config.topP = static_cast<int>(m_topPSpin->value() * 100);
    return config;
}

void ConfigDialog::setConfig(const ApiConfig& config)
{
    m_apiKeyEdit->setText(config.apiKey);
    m_baseUrlEdit->setText(config.baseUrl);
    m_modelEdit->setText(config.model);
    m_systemPromptEdit->setPlainText(config.systemPrompt);
    m_temperatureSpin->setValue(config.temperature);
    m_maxTokensSpin->setValue(config.maxTokens);
    m_topPSpin->setValue(config.topP / 100.0);
}

void ConfigDialog::loadFromFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);

    if (parseError.error == QJsonParseError::NoError) {
        QJsonObject root = doc.object();

        m_userNameEdit->setText(root[QLatin1String("userName")].toString(QString::fromUtf8("用户")));
        m_botNameEdit->setText(root[QLatin1String("botName")].toString(QString::fromUtf8("助手")));
        m_userAvatarPath = root[QLatin1String("userAvatarPath")].toString();
        m_botAvatarPath = root[QLatin1String("botAvatarPath")].toString();
        
        updateAvatarPreview(m_userAvatarPreview, m_userAvatarPath, m_userNameEdit->text());
        updateAvatarPreview(m_botAvatarPreview, m_botAvatarPath, m_botNameEdit->text());

        ApiConfig config;
        config.apiKey = root[QLatin1String("apiKey")].toString();
        config.baseUrl = root[QLatin1String("baseUrl")].toString();
        config.model = root[QLatin1String("model")].toString();
        config.systemPrompt = root[QLatin1String("systemPrompt")].toString();
        config.temperature = root[QLatin1String("temperature")].toDouble(0.7);
        config.maxTokens = root[QLatin1String("maxTokens")].toInt(4096);
        config.topP = root[QLatin1String("topP")].toInt(100);

        setConfig(config);
    }

    file.close();
}

void ConfigDialog::saveToFile(const QString& path)
{
    QJsonObject root;
    ApiConfig config = getConfig();

    root[QLatin1String("userName")] = m_userNameEdit->text();
    root[QLatin1String("botName")] = m_botNameEdit->text();
    root[QLatin1String("userAvatarPath")] = m_userAvatarPath;
    root[QLatin1String("botAvatarPath")] = m_botAvatarPath;

    root[QLatin1String("apiKey")] = config.apiKey;
    root[QLatin1String("baseUrl")] = config.baseUrl;
    root[QLatin1String("model")] = config.model;
    root[QLatin1String("systemPrompt")] = config.systemPrompt;
    root[QLatin1String("temperature")] = config.temperature;
    root[QLatin1String("maxTokens")] = config.maxTokens;
    root[QLatin1String("topP")] = config.topP;

    QJsonDocument doc(root);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

QString ConfigDialog::getConfigPath() const
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(configDir);
    return configDir + QLatin1String("/chat_config.json");
}

void ConfigDialog::onAccepted()
{
    // Auto-save config
    saveToFile(getConfigPath());
    QDialog::accept();
}

void ConfigDialog::onTestConnection()
{
    ApiConfig config = getConfig();

    if (config.apiKey.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("错误"), QString::fromUtf8("请先输入 API Key"));
        return;
    }

    // TODO: Implement actual test connection
    QMessageBox::information(this, QString::fromUtf8("测试"), QString::fromUtf8("测试功能将在与ApiManager集成后实现"));
}

void ConfigDialog::onSelectUserAvatar()
{
    QString path = QFileDialog::getOpenFileName(this, QString::fromUtf8("选择用户头像"),
        QString(), QString::fromUtf8("图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)"));
    if (!path.isEmpty()) {
        m_userAvatarPath = path;
        updateAvatarPreview(m_userAvatarPreview, path, m_userNameEdit->text());
    }
}

void ConfigDialog::onSelectBotAvatar()
{
    QString path = QFileDialog::getOpenFileName(this, QString::fromUtf8("选择机器人头像"),
        QString(), QString::fromUtf8("图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)"));
    if (!path.isEmpty()) {
        m_botAvatarPath = path;
        updateAvatarPreview(m_botAvatarPreview, path, m_botNameEdit->text());
    }
}

void ConfigDialog::updateAvatarPreview(QLabel* previewLabel, const QString& avatarPath, const QString& defaultLetter)
{
    if (!avatarPath.isEmpty() && QFile::exists(avatarPath)) {
        QPixmap pixmap(avatarPath);
        if (!pixmap.isNull()) {
            QPixmap scaled = pixmap.scaled(50, 50, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            previewLabel->setPixmap(scaled);
            previewLabel->setText(QString());
            return;
        }
    }
    
    QString letter = defaultLetter.isEmpty() ? QString::fromUtf8("?") : QString(defaultLetter.at(0)).toUpper();
    previewLabel->clear();
    previewLabel->setText(letter);
}

QString ConfigDialog::getUserName() const
{
    return m_userNameEdit->text();
}

void ConfigDialog::setUserName(const QString& name)
{
    m_userNameEdit->setText(name);
    updateAvatarPreview(m_userAvatarPreview, m_userAvatarPath, name);
}

QString ConfigDialog::getBotName() const
{
    return m_botNameEdit->text();
}

void ConfigDialog::setBotName(const QString& name)
{
    m_botNameEdit->setText(name);
    updateAvatarPreview(m_botAvatarPreview, m_botAvatarPath, name);
}

QString ConfigDialog::getUserAvatarPath() const
{
    return m_userAvatarPath;
}

void ConfigDialog::setUserAvatarPath(const QString& path)
{
    m_userAvatarPath = path;
    updateAvatarPreview(m_userAvatarPreview, path, m_userNameEdit->text());
}

QString ConfigDialog::getBotAvatarPath() const
{
    return m_botAvatarPath;
}

void ConfigDialog::setBotAvatarPath(const QString& path)
{
    m_botAvatarPath = path;
    updateAvatarPreview(m_botAvatarPreview, path, m_botNameEdit->text());
}
