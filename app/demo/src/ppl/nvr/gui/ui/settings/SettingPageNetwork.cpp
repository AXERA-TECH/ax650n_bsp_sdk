#include "SettingPageNetwork.h"
#include "ui_SettingPageNetwork.h"

SettingPageNetwork::SettingPageNetwork(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingPageNetwork)
{
    ui->setupUi(this);
}

SettingPageNetwork::~SettingPageNetwork()
{
    delete ui;
}

int SettingPageNetwork::OnLoad()
{
    return 0;
}

int SettingPageNetwork::OnSave()
{
    return 0;
}
