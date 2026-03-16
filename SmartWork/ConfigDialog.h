#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QTextEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include "ApiManager.h"

class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigDialog(QWidget *parent = nullptr);
    ~ConfigDialog();

    ApiConfig getConfig() const;
    void setConfig(const ApiConfig& config);

    void loadFromFile(const QString& path);
    void saveToFile(const QString& path);

    QString getUserName() const;
    void setUserName(const QString& name);
    QString getBotName() const;
    void setBotName(const QString& name);
    QString getUserAvatarPath() const;
    void setUserAvatarPath(const QString& path);
    QString getBotAvatarPath() const;
    void setBotAvatarPath(const QString& path);

private slots:
    void onAccepted();
    void onTestConnection();
    void onSelectUserAvatar();
    void onSelectBotAvatar();

private:
    void setupUI();
    void applyStyles();
    QString getConfigPath() const;
    void updateAvatarPreview(QLabel* previewLabel, const QString& avatarPath, const QString& defaultLetter);

    QLineEdit* m_userNameEdit;
    QLineEdit* m_botNameEdit;
    QPushButton* m_userAvatarBtn;
    QPushButton* m_botAvatarBtn;
    QLabel* m_userAvatarPreview;
    QLabel* m_botAvatarPreview;
    QString m_userAvatarPath;
    QString m_botAvatarPath;

    QComboBox* m_providerCombo;

    // Authentication
    QLineEdit* m_apiKeyEdit;
    QLineEdit* m_baseUrlEdit;

    // Model settings
    QLineEdit* m_modelEdit;
    QComboBox* m_presetModelCombo;

    // Generation parameters
    QDoubleSpinBox* m_temperatureSpin;
    QSpinBox* m_maxTokensSpin;
    QDoubleSpinBox* m_topPSpin;

    // System prompt
    QTextEdit* m_systemPromptEdit;

    // Buttons
    QPushButton* m_testButton;
    QPushButton* m_saveButton;
    QPushButton* m_loadButton;
};
