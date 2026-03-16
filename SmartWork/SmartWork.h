#pragma once

#include <QtWidgets/QWidget>
#include "ui_SmartWork.h"

QT_BEGIN_NAMESPACE
namespace Ui { class SmartWorkClass; };
QT_END_NAMESPACE

class SmartWork : public QWidget
{
    Q_OBJECT

public:
    SmartWork(QWidget *parent = nullptr);
    ~SmartWork();

private:
    Ui::SmartWorkClass *ui;
};

