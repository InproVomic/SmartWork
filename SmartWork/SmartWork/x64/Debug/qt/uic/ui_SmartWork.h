/********************************************************************************
** Form generated from reading UI file 'SmartWork.ui'
**
** Created by: Qt User Interface Compiler version 5.12.12
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SMARTWORK_H
#define UI_SMARTWORK_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SmartWorkClass
{
public:

    void setupUi(QWidget *SmartWorkClass)
    {
        if (SmartWorkClass->objectName().isEmpty())
            SmartWorkClass->setObjectName(QString::fromUtf8("SmartWorkClass"));
        SmartWorkClass->resize(600, 400);

        retranslateUi(SmartWorkClass);

        QMetaObject::connectSlotsByName(SmartWorkClass);
    } // setupUi

    void retranslateUi(QWidget *SmartWorkClass)
    {
        SmartWorkClass->setWindowTitle(QApplication::translate("SmartWorkClass", "SmartWork", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SmartWorkClass: public Ui_SmartWorkClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SMARTWORK_H
