#include "AvatarConfigDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QPainter>
#include <QFont>
#include <QRadialGradient>

AvatarConfigDialog::AvatarConfigDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QString::fromUtf8("头像和昵称设置"));
    resize(450, 400);

    setupUI();
    applyStyles();
}

AvatarConfigDialog::~AvatarConfigDialog()
{
}

void AvatarConfigDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    auto* userGroup = new QGroupBox(QString::fromUtf8("用户设置"), this);
    auto* userLayout = new QVBoxLayout();
    userLayout->setSpacing(10);

    auto* userNameLayout = new QHBoxLayout();
    userNameLayout->addWidget(new QLabel(QString::fromUtf8("昵称:"), this));
    m_userNameEdit = new QLineEdit(this);
    m_userNameEdit->setText(QString::fromUtf8("用户"));
    m_userNameEdit->setPlaceholderText(QString::fromUtf8("输入您的昵称"));
    userNameLayout->addWidget(m_userNameEdit, 1);
    userLayout->addLayout(userNameLayout);

    auto* userAvatarLayout = new QHBoxLayout();
    m_userAvatarLabel = new QLabel(this);
    m_userAvatarLabel->setFixedSize(64, 64);
    m_userAvatarLabel->setScaledContents(true);
    m_userAvatarLabel->setStyleSheet(QLatin1String("border: 2px solid #e0e0e0; border-radius: 32px;"));
    userAvatarLayout->addWidget(m_userAvatarLabel);

    auto* userBtnLayout = new QVBoxLayout();
    m_selectUserAvatarBtn = new QPushButton(QString::fromUtf8("选择头像"), this);
    m_resetUserAvatarBtn = new QPushButton(QString::fromUtf8("重置"), this);
    m_resetUserAvatarBtn->setObjectName(QLatin1String("secondary"));
    userBtnLayout->addWidget(m_selectUserAvatarBtn);
    userBtnLayout->addWidget(m_resetUserAvatarBtn);
    userAvatarLayout->addLayout(userBtnLayout);
    userAvatarLayout->addStretch(1);
    userLayout->addLayout(userAvatarLayout);

    userGroup->setLayout(userLayout);

    auto* botGroup = new QGroupBox(QString::fromUtf8("机器人设置"), this);
    auto* botLayout = new QVBoxLayout();
    botLayout->setSpacing(10);

    auto* botNameLayout = new QHBoxLayout();
    botNameLayout->addWidget(new QLabel(QString::fromUtf8("昵称:"), this));
    m_botNameEdit = new QLineEdit(this);
    m_botNameEdit->setText(QString::fromUtf8("助手"));
    m_botNameEdit->setPlaceholderText(QString::fromUtf8("输入机器人昵称"));
    botNameLayout->addWidget(m_botNameEdit, 1);
    botLayout->addLayout(botNameLayout);

    auto* botAvatarLayout = new QHBoxLayout();
    m_botAvatarLabel = new QLabel(this);
    m_botAvatarLabel->setFixedSize(64, 64);
    m_botAvatarLabel->setScaledContents(true);
    m_botAvatarLabel->setStyleSheet(QLatin1String("border: 2px solid #e0e0e0; border-radius: 32px;"));
    botAvatarLayout->addWidget(m_botAvatarLabel);

    auto* botBtnLayout = new QVBoxLayout();
    m_selectBotAvatarBtn = new QPushButton(QString::fromUtf8("选择头像"), this);
    m_resetBotAvatarBtn = new QPushButton(QString::fromUtf8("重置"), this);
    m_resetBotAvatarBtn->setObjectName(QLatin1String("secondary"));
    botBtnLayout->addWidget(m_selectBotAvatarBtn);
    botBtnLayout->addWidget(m_resetBotAvatarBtn);
    botAvatarLayout->addLayout(botBtnLayout);
    botAvatarLayout->addStretch(1);
    botLayout->addLayout(botAvatarLayout);

    botGroup->setLayout(botLayout);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    auto* okButton = new QPushButton(QString::fromUtf8("确定"), this);
    auto* cancelButton = new QPushButton(QString::fromUtf8("取消"), this);
    cancelButton->setObjectName(QLatin1String("secondary"));
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addWidget(userGroup);
    mainLayout->addWidget(botGroup);
    mainLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_selectUserAvatarBtn, &QPushButton::clicked, this, &AvatarConfigDialog::onSelectUserAvatar);
    connect(m_selectBotAvatarBtn, &QPushButton::clicked, this, &AvatarConfigDialog::onSelectBotAvatar);
    connect(m_resetUserAvatarBtn, &QPushButton::clicked, this, &AvatarConfigDialog::onResetUserAvatar);
    connect(m_resetBotAvatarBtn, &QPushButton::clicked, this, &AvatarConfigDialog::onResetBotAvatar);
    connect(m_userNameEdit, &QLineEdit::textChanged, this, [this]() {
        if (m_userAvatarPath.isEmpty()) {
            updateUserAvatarPreview();
        }
    });
    connect(m_botNameEdit, &QLineEdit::textChanged, this, [this]() {
        if (m_botAvatarPath.isEmpty()) {
            updateBotAvatarPreview();
        }
    });

    updateUserAvatarPreview();
    updateBotAvatarPreview();
}

void AvatarConfigDialog::applyStyles()
{
    setStyleSheet(QLatin1String(
        "QDialog {"
        "    font-family: \"Microsoft YaHei UI\", \"Segoe UI\", sans-serif;"
        "    background-color: #f8f9fa;"
        "}"
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
        "QLineEdit {"
        "    padding: 8px 12px;"
        "    border: 2px solid #e0e0e0;"
        "    border-radius: 6px;"
        "    background-color: white;"
        "    font-size: 13px;"
        "}"
        "QLineEdit:focus {"
        "    border-color: #007AFF;"
        "}"
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #007AFF, stop:1 #0051D5);"
        "    color: white;"
        "    border: none;"
        "    border-radius: 6px;"
        "    padding: 8px 16px;"
        "    font-size: 13px;"
        "    font-weight: 600;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0066FF, stop:1 #0044BB);"
        "}"
        "QPushButton#secondary {"
        "    background-color: #f0f0f0;"
        "    color: #333;"
        "}"
        "QPushButton#secondary:hover {"
        "    background-color: #e0e0e0;"
        "}"
        "QLabel {"
        "    color: #333;"
        "}"
    ));
}

void AvatarConfigDialog::onSelectUserAvatar()
{
    QString path = QFileDialog::getOpenFileName(
        this,
        QString::fromUtf8("选择用户头像"),
        QString(),
        QString::fromUtf8("图片文件 (*.png *.jpg *.jpeg *.bmp *.gif);;所有文件 (*.*)")
    );

    if (!path.isEmpty()) {
        m_userAvatarPath = path;
        updateUserAvatarPreview();
    }
}

void AvatarConfigDialog::onSelectBotAvatar()
{
    QString path = QFileDialog::getOpenFileName(
        this,
        QString::fromUtf8("选择机器人头像"),
        QString(),
        QString::fromUtf8("图片文件 (*.png *.jpg *.jpeg *.bmp *.gif);;所有文件 (*.*)")
    );

    if (!path.isEmpty()) {
        m_botAvatarPath = path;
        updateBotAvatarPreview();
    }
}

void AvatarConfigDialog::onResetUserAvatar()
{
    m_userAvatarPath.clear();
    updateUserAvatarPreview();
}

void AvatarConfigDialog::onResetBotAvatar()
{
    m_botAvatarPath.clear();
    updateBotAvatarPreview();
}

void AvatarConfigDialog::updateUserAvatarPreview()
{
    if (m_userAvatarPath.isEmpty()) {
        m_userAvatar = createDefaultAvatar(m_userNameEdit->text(), QColor(102, 126, 234));
    } else {
        QPixmap pix(m_userAvatarPath);
        if (!pix.isNull()) {
            m_userAvatar = pix.scaled(64, 64, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        } else {
            m_userAvatar = createDefaultAvatar(m_userNameEdit->text(), QColor(102, 126, 234));
        }
    }
    m_userAvatarLabel->setPixmap(m_userAvatar);
}

void AvatarConfigDialog::updateBotAvatarPreview()
{
    if (m_botAvatarPath.isEmpty()) {
        m_botAvatar = createDefaultAvatar(m_botNameEdit->text(), QColor(17, 153, 142));
    } else {
        QPixmap pix(m_botAvatarPath);
        if (!pix.isNull()) {
            m_botAvatar = pix.scaled(64, 64, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        } else {
            m_botAvatar = createDefaultAvatar(m_botNameEdit->text(), QColor(17, 153, 142));
        }
    }
    m_botAvatarLabel->setPixmap(m_botAvatar);
}

QPixmap AvatarConfigDialog::createDefaultAvatar(const QString& name, const QColor& color)
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

QString AvatarConfigDialog::getUserName() const
{
    return m_userNameEdit->text();
}

QString AvatarConfigDialog::getBotName() const
{
    return m_botNameEdit->text();
}

QString AvatarConfigDialog::getUserAvatarPath() const
{
    return m_userAvatarPath;
}

QString AvatarConfigDialog::getBotAvatarPath() const
{
    return m_botAvatarPath;
}

QPixmap AvatarConfigDialog::getUserAvatar() const
{
    return m_userAvatar;
}

QPixmap AvatarConfigDialog::getBotAvatar() const
{
    return m_botAvatar;
}

void AvatarConfigDialog::setUserName(const QString& name)
{
    m_userNameEdit->setText(name);
    updateUserAvatarPreview();
}

void AvatarConfigDialog::setBotName(const QString& name)
{
    m_botNameEdit->setText(name);
    updateBotAvatarPreview();
}

void AvatarConfigDialog::setUserAvatarPath(const QString& path)
{
    m_userAvatarPath = path;
    updateUserAvatarPreview();
}

void AvatarConfigDialog::setBotAvatarPath(const QString& path)
{
    m_botAvatarPath = path;
    updateBotAvatarPreview();
}

void AvatarConfigDialog::loadFromFile(const QString& path)
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
        updateUserAvatarPreview();
        updateBotAvatarPreview();
    }

    file.close();
}

void AvatarConfigDialog::saveToFile(const QString& path)
{
    QJsonObject root;
    root[QLatin1String("userName")] = m_userNameEdit->text();
    root[QLatin1String("botName")] = m_botNameEdit->text();
    root[QLatin1String("userAvatarPath")] = m_userAvatarPath;
    root[QLatin1String("botAvatarPath")] = m_botAvatarPath;

    QJsonDocument doc(root);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}
