#include "SettingsTopToolbar.h"
#include "ui_SettingsTopToolbar.h"

SettingsTopToolbar::SettingsTopToolbar(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingsTopToolbar)
{
    ui->setupUi(this);
}

SettingsTopToolbar::~SettingsTopToolbar()
{
    delete ui;
}

