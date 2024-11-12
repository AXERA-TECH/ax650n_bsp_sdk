#include "SettingPageRoundPatrol.h"
#include "ui_SettingPageRoundPatrol.h"

SettingPageRoundPatrol::SettingPageRoundPatrol(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingPageRoundPatrol)
{
    ui->setupUi(this);
}

SettingPageRoundPatrol::~SettingPageRoundPatrol()
{
    delete ui;
}
