#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QPixmap>

class AvatarConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AvatarConfigDialog(QWidget *parent = nullptr);
    ~AvatarConfigDialog();

    QString getUserName() const;
    QString getBotName() const;
    QString getUserAvatarPath() const;
    QString getBotAvatarPath() const;
    QPixmap getUserAvatar() const;
    QPixmap getBotAvatar() const;

    void setUserName(const QString& name);
    void setBotName(const QString& name);
    void setUserAvatarPath(const QString& path);
    void setBotAvatarPath(const QString& path);

    void loadFromFile(const QString& path);
    void saveToFile(const QString& path);

private slots:
    void onSelectUserAvatar();
    void onSelectBotAvatar();
    void onResetUserAvatar();
    void onResetBotAvatar();

private:
    void setupUI();
    void applyStyles();
    void updateUserAvatarPreview();
    void updateBotAvatarPreview();
    QPixmap createDefaultAvatar(const QString& name, const QColor& color);
    QString getDefaultAvatarPath(bool isUser);

    QLineEdit* m_userNameEdit;
    QLineEdit* m_botNameEdit;
    QLabel* m_userAvatarLabel;
    QLabel* m_botAvatarLabel;
    QPushButton* m_selectUserAvatarBtn;
    QPushButton* m_selectBotAvatarBtn;
    QPushButton* m_resetUserAvatarBtn;
    QPushButton* m_resetBotAvatarBtn;

    QString m_userAvatarPath;
    QString m_botAvatarPath;
    QPixmap m_userAvatar;
    QPixmap m_botAvatar;
};
