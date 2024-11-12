#include "PreviewTopToolbar.h"
#include "ui_PreviewTopToolbar.h"
#include "AppLogApi.h"
#include "../utils/NoSignals.h"
#include <unistd.h>

#define TAG "PREVIEW"


PreviewTopToolbar::PreviewTopToolbar(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PreviewTopToolbar)
{
    ui->setupUi(this);
    ui->pushButton_polling->setVisible(false);
    ui->pushButton_6x6->setVisible(false);

    ui->pushButton_1x1->setCheckable(true);
    ui->pushButton_2x2->setCheckable(true);
    ui->pushButton_1x8->setCheckable(true);
    ui->pushButton_4x4->setCheckable(true);
    ui->pushButton_6x6->setCheckable(true);
    ui->pushButton_8x8->setCheckable(true);
    ui->pushButton_1x1->setStyleSheet(CSS_PUSHBUTTON);
    ui->pushButton_2x2->setStyleSheet(CSS_PUSHBUTTON);
    ui->pushButton_1x8->setStyleSheet(CSS_PUSHBUTTON);
    ui->pushButton_4x4->setStyleSheet(CSS_PUSHBUTTON);
    ui->pushButton_6x6->setStyleSheet(CSS_PUSHBUTTON);
    ui->pushButton_8x8->setStyleSheet(CSS_PUSHBUTTON);

    ui->pushButton_previous->setStyleSheet(CSS_PUSHBUTTON);
    ui->pushButton_next->setStyleSheet(CSS_PUSHBUTTON);
    ui->pushButton_main_sub1->setStyleSheet(CSS_PUSHBUTTON);
    ui->pushButton_pip->setStyleSheet(CSS_PUSHBUTTON);

    NoSignals::setChecked(ui->pushButton_2x2, true);

    connect(ui->pushButton_1x1, &QPushButton::clicked, this, &PreviewTopToolbar::OnChangeSplit);
    connect(ui->pushButton_2x2, &QPushButton::clicked, this, &PreviewTopToolbar::OnChangeSplit);
    connect(ui->pushButton_1x8, &QPushButton::clicked, this, &PreviewTopToolbar::OnChangeSplit);
    connect(ui->pushButton_4x4, &QPushButton::clicked, this, &PreviewTopToolbar::OnChangeSplit);
    // connect(ui->pushButton_6x6, &QPushButton::clicked, this, &PreviewTopToolbar::OnChangeSplit);
    connect(ui->pushButton_8x8, &QPushButton::clicked, this, &PreviewTopToolbar::OnChangeSplit);

    connect(ui->pushButton_main_sub1, &QPushButton::clicked, this, &PreviewTopToolbar::OnChangeMainSub1);

    connect(ui->pushButton_previous,&QPushButton::clicked, this, &PreviewTopToolbar::OnChangePrevNext);
    connect(ui->pushButton_next, &QPushButton::clicked, this, &PreviewTopToolbar::OnChangePrevNext);
    // connect(ui->pushButton_polling, &QPushButton::clicked, this, &PreviewTopToolbar::OnStartPolling);
    connect(ui->pushButton_pip, &QPushButton::clicked, this, &PreviewTopToolbar::OnEnablePip);

}

PreviewTopToolbar::~PreviewTopToolbar()
{
    delete ui;
}

// void PreviewTopToolbar::OnStartPolling() {
//     QPushButton *pBtn = (QPushButton*)sender();
//     if (pBtn == ui->pushButton_polling) {
//         emit signal_enable_polling(ui->pushButton_polling->isChecked());
//     }
// }

void PreviewTopToolbar::OnEnablePip() {
    QPushButton *pBtn = (QPushButton*)sender();
    if (pBtn == ui->pushButton_pip) {
        emit signal_enable_pip(ui->pushButton_pip->isChecked());
    }
}

void PreviewTopToolbar::OnChangePrevNext() {
    QPushButton *pBtn = (QPushButton*)sender();
    PREV_NEXT_TYPE type = PREV_NEXT_TYPE::NEXT;
    if (pBtn == ui->pushButton_previous) {
        type = PREV_NEXT_TYPE::PREV;
    }
    else if (pBtn == ui->pushButton_next) {
        type = PREV_NEXT_TYPE::NEXT;
        if (!ui->pushButton_previous->isEnabled()) {
            ui->pushButton_previous->setEnabled(true);
        }
    }
    emit signal_change_prev_next(type);
}

void PreviewTopToolbar::OnChangeSplit() {
    QPushButton *pBtn = (QPushButton*)sender();
    SPLIT_TYPE type = SPLIT_TYPE::MAX;
    bool isChecked = pBtn->isChecked();
    if (pBtn == ui->pushButton_1x1) {
        type = SPLIT_TYPE::ONE;
        NoSignals::setChecked(ui->pushButton_1x1, true);
        NoSignals::setChecked(ui->pushButton_2x2, false);
        NoSignals::setChecked(ui->pushButton_1x8, false);
        NoSignals::setChecked(ui->pushButton_4x4, false);
        NoSignals::setChecked(ui->pushButton_6x6, false);
        NoSignals::setChecked(ui->pushButton_8x8, false);
        ui->pushButton_previous->setEnabled(false);
        ui->pushButton_next->setEnabled(true);
    }
    else if (pBtn == ui->pushButton_2x2) {
        type = SPLIT_TYPE::FOUR;
        NoSignals::setChecked(ui->pushButton_1x1, false);
        NoSignals::setChecked(ui->pushButton_2x2, true);
        NoSignals::setChecked(ui->pushButton_1x8, false);
        NoSignals::setChecked(ui->pushButton_4x4, false);
        NoSignals::setChecked(ui->pushButton_6x6, false);
        NoSignals::setChecked(ui->pushButton_8x8, false);
        ui->pushButton_previous->setEnabled(false);
        ui->pushButton_next->setEnabled(true);
    }
    else if (pBtn == ui->pushButton_1x8) {
        type = SPLIT_TYPE::EIGHT;
        NoSignals::setChecked(ui->pushButton_1x1, false);
        NoSignals::setChecked(ui->pushButton_2x2, false);
        NoSignals::setChecked(ui->pushButton_1x8, true);
        NoSignals::setChecked(ui->pushButton_4x4, false);
        NoSignals::setChecked(ui->pushButton_6x6, false);
        NoSignals::setChecked(ui->pushButton_8x8, false);
        ui->pushButton_previous->setEnabled(false);
        ui->pushButton_next->setEnabled(true);
    }
    else if (pBtn == ui->pushButton_4x4) {
        type = SPLIT_TYPE::SIXTEEN;
        NoSignals::setChecked(ui->pushButton_1x1, false);
        NoSignals::setChecked(ui->pushButton_2x2, false);
        NoSignals::setChecked(ui->pushButton_1x8, false);
        NoSignals::setChecked(ui->pushButton_4x4, true);
        NoSignals::setChecked(ui->pushButton_6x6, false);
        NoSignals::setChecked(ui->pushButton_8x8, false);
        ui->pushButton_previous->setEnabled(false);
        ui->pushButton_next->setEnabled(true);
    }
    else if (pBtn == ui->pushButton_6x6) {
        type = SPLIT_TYPE::THIRTYSIX;
        NoSignals::setChecked(ui->pushButton_1x1, false);
        NoSignals::setChecked(ui->pushButton_2x2, false);
        NoSignals::setChecked(ui->pushButton_1x8, false);
        NoSignals::setChecked(ui->pushButton_4x4, false);
        NoSignals::setChecked(ui->pushButton_6x6, true);
        NoSignals::setChecked(ui->pushButton_8x8, false);
        ui->pushButton_previous->setEnabled(false);
        ui->pushButton_next->setEnabled(true);
    }
    else if (pBtn == ui->pushButton_8x8) {
        type = SPLIT_TYPE::SIXTYFOUR;
        NoSignals::setChecked(ui->pushButton_1x1, false);
        NoSignals::setChecked(ui->pushButton_2x2, false);
        NoSignals::setChecked(ui->pushButton_1x8, false);
        NoSignals::setChecked(ui->pushButton_4x4, false);
        NoSignals::setChecked(ui->pushButton_6x6, false);
        NoSignals::setChecked(ui->pushButton_8x8, true);
        ui->pushButton_previous->setEnabled(false);
        ui->pushButton_next->setEnabled(false);
    }

    if (isChecked){
        emit signal_change_split(type);
    }
}

void PreviewTopToolbar::OnChangeMainSub1() {
    emit signal_change_mainsub1();
}

void PreviewTopToolbar::hideEvent(QHideEvent *event) {
    ui->pushButton_pip->setChecked(false);
    return QWidget::hideEvent(event);
}

void PreviewTopToolbar::CheckPip(bool bChecked) {
    ui->pushButton_pip->setChecked(bChecked);
}

void PreviewTopToolbar::SetCurSplit(SPLIT_TYPE type) {
    switch (type)
    {
    case SPLIT_TYPE::ONE:
        ui->pushButton_1x1->clicked();
        break;
    case SPLIT_TYPE::FOUR:
        ui->pushButton_2x2->clicked();
        break;
    case SPLIT_TYPE::EIGHT:
        ui->pushButton_1x8->clicked();
        break;
    case SPLIT_TYPE::SIXTEEN:
        ui->pushButton_4x4->clicked();
        break;
    case SPLIT_TYPE::THIRTYSIX:
        ui->pushButton_6x6->clicked();
        break;
    case SPLIT_TYPE::SIXTYFOUR:
        ui->pushButton_8x8->clicked();
        break;
    default:
        break;
    }
}

void PreviewTopToolbar::SetPrevEnabled(bool bEnabled)
{
    ui->pushButton_previous->setEnabled(bEnabled);
}

void PreviewTopToolbar::SetNextEnabled(bool bEnabled)
{
    ui->pushButton_next->setEnabled(bEnabled);
}

void PreviewTopToolbar::EnableWidgets(bool bEnabled) {
    ui->pushButton_1x1->setEnabled(bEnabled);
    ui->pushButton_2x2->setEnabled(bEnabled);
    ui->pushButton_1x8->setEnabled(bEnabled);
    ui->pushButton_4x4->setEnabled(bEnabled);
    ui->pushButton_6x6->setEnabled(bEnabled);
    ui->pushButton_8x8->setEnabled(bEnabled);
    ui->pushButton_previous->setEnabled(bEnabled);
    ui->pushButton_next->setEnabled(bEnabled);
    ui->pushButton_main_sub1->setEnabled(bEnabled);
    ui->pushButton_pip->setEnabled(bEnabled);
}