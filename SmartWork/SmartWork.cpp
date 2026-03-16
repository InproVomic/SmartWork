#include "SmartWork.h"

SmartWork::SmartWork(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SmartWorkClass())
{
    ui->setupUi(this);
}

SmartWork::~SmartWork()
{
    delete ui;
}

