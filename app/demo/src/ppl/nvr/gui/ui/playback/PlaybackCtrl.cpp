#include "PlaybackCtrl.h"
#include "ui_PlaybackCtrl.h"
#include "AppLogApi.h"
#include "AXNVRFramework.h"
#include <unistd.h>
#include <QMetaType>
#include <QDebug>

#define TAG "PLAYBACK"


PlaybackCtrl::PlaybackCtrl(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PlaybackCtrl)
{

    qRegisterMetaType<PLAYBACK_ACTION_TYPE>("PLAYBACK_ACTION_TYPE");

    ui->setupUi(this);
    this->setStyleSheet(CSS_PUSHBUTTON);

    connect(ui->pushButtonPlay, SIGNAL(clicked()), this, SLOT(OnCtrlBtnClicked()));
    connect(ui->pushButtonPause, SIGNAL(clicked()), this, SLOT(OnCtrlBtnClicked()));
    connect(ui->pushButtonStop, SIGNAL(clicked()), this, SLOT(OnCtrlBtnClicked()));
    connect(ui->pushButtonReverse, SIGNAL(clicked()), this, SLOT(OnCtrlBtnClicked()));

    connect(ui->pushButtonPreviousFrame, SIGNAL(clicked()), this, SLOT(OnCtrlBtnClicked()));
    connect(ui->pushButtonNextFrame, SIGNAL(clicked()), this, SLOT(OnCtrlBtnClicked()));

    connect(ui->pushButtonSlow, SIGNAL(clicked()), this, SLOT(OnCtrlBtnClicked()));
    connect(ui->pushButtonFast, SIGNAL(clicked()), this, SLOT(OnCtrlBtnClicked()));

    connect(ui->pushButtonPrevious, SIGNAL(clicked()), this, SLOT(OnCtrlBtnClicked()));
    connect(ui->pushButtonNext, SIGNAL(clicked()), this, SLOT(OnCtrlBtnClicked()));
}

PlaybackCtrl::~PlaybackCtrl(void)
{
    delete ui;
}

void PlaybackCtrl::OnCtrlBtnClicked(void) {
    QPushButton *pBtn = (QPushButton*)sender();

    PLAYBACK_ACTION_TYPE enActionType = PLAYBACK_ACTION_TYPE::INIT;

    if (pBtn == ui->pushButtonPlay)         enActionType = PLAYBACK_ACTION_TYPE::PLAY;
    if (pBtn == ui->pushButtonPause)        enActionType = PLAYBACK_ACTION_TYPE::PAUSE;
    if (pBtn == ui->pushButtonStop)         enActionType = PLAYBACK_ACTION_TYPE::STOP;
    if (pBtn == ui->pushButtonReverse)      enActionType = PLAYBACK_ACTION_TYPE::REVERSE;

    if (pBtn == ui->pushButtonPreviousFrame)enActionType = PLAYBACK_ACTION_TYPE::PREV_FRAME;
    if (pBtn == ui->pushButtonNextFrame)    enActionType = PLAYBACK_ACTION_TYPE::NEXT_FRAME;

    if (pBtn == ui->pushButtonSlow)         enActionType = PLAYBACK_ACTION_TYPE::SLOW_SPEED;
    if (pBtn == ui->pushButtonFast)         enActionType = PLAYBACK_ACTION_TYPE::FAST_SPEED;

    if (pBtn == ui->pushButtonPrevious)     enActionType = PLAYBACK_ACTION_TYPE::PREV;
    if (pBtn == ui->pushButtonNext)         enActionType = PLAYBACK_ACTION_TYPE::NEXT;

    emit signal_playback_action(enActionType);
}

void PlaybackCtrl::EnableSingleLabelCtrl(bool bEnable) {

    ui->pushButtonPause->setEnabled(bEnable);
    ui->pushButtonReverse->setEnabled(bEnable);
    ui->pushButtonPreviousFrame->setEnabled(bEnable);
    ui->pushButtonNextFrame->setEnabled(bEnable);
    ui->pushButtonSlow->setEnabled(bEnable);
    ui->pushButtonFast->setEnabled(bEnable);
}