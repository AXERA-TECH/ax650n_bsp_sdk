#include "SettingPageSystem.h"
#include "ui_SettingPageSystem.h"
#include "global/UiGlobalDef.h"


SettingPageSystem::SettingPageSystem(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingPageSystem)
{
    ui->setupUi(this);
    this->setStyleSheet(CSS_WIDGET);
    ui->tabWidget->setStyleSheet(CSS_TABWIDGET);
}

SettingPageSystem::~SettingPageSystem()
{
    delete ui;
}

int SettingPageSystem::OnLoad()
{
    return 0;
}

int SettingPageSystem::OnSave()
{
    return 0;
}
