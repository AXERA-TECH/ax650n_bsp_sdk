#include "PlaybackTopToolbar.h"
#include "ui_PlaybackTopToolbar.h"

PlaybackTopToolbar::PlaybackTopToolbar(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PlaybackTopToolbar)
{
    ui->setupUi(this);
    connect(ui->pushButton_main_sub1, &QPushButton::clicked, this, &PlaybackTopToolbar::OnChangeMainSub1);
}

PlaybackTopToolbar::~PlaybackTopToolbar()
{
    delete ui;
}

void PlaybackTopToolbar::OnChangeMainSub1() {
    emit signal_change_mainsub1();
}