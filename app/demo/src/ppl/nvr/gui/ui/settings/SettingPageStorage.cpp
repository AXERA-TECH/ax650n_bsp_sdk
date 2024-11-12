#include "SettingPageStorage.h"
#include "ui_SettingPageStorage.h"

SettingPageStorage::SettingPageStorage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingPageStorage)
{
    ui->setupUi(this);
}

SettingPageStorage::~SettingPageStorage()
{
    delete ui;
}

int SettingPageStorage::OnLoad()
{
    return 0;
}

int SettingPageStorage::OnSave()
{
    return 0;
}
