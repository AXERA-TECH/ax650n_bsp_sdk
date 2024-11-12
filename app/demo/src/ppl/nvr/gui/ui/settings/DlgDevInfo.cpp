/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "DlgDevInfo.h"
#include "ui_DlgDevInfo.h"
#include "AXNVRFramework.h"
#include "global/UiGlobalDef.h"

#include <QDebug>


DlgDevInfo::DlgDevInfo(bool bEdit, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DlgDevInfo)
    , m_bEdit(bEdit) {
    ui->setupUi(this);
    this->setStyleSheet(CSS_WIDGET_POPUP);
}

DlgDevInfo::~DlgDevInfo() {
    delete ui;
}

void DlgDevInfo::SetDevInfo(const AX_NVR_DEV_INFO_T &stDevInfo) {
    m_stDevInfo = stDevInfo;
}

void DlgDevInfo::showEvent(QShowEvent *evt) {
    // channel id
    if (m_bEdit) {
        ui->comboBoxChannels->addItem(QString::number(m_stDevInfo.nChannelId + 1));
    }
    else {
        CAXNVRPreviewCtrl *pPreview = CAXNVRFramework::GetInstance()->PreviewCtrl();
        if (pPreview) {
            vector<AX_NVR_DEV_ID> vecFreeIDs = pPreview->GetFreeDevices();
            for (auto id : vecFreeIDs) {
                ui->comboBoxChannels->addItem(QString::number(id + 1));
            }
        }
    }

    ui->comboBoxChannels->setCurrentIndex(0);
    // alias
    ui->lineEditAlias->setText(QString::fromUtf8((char*)m_stDevInfo.szAlias));
    // type
    ui->comboBoxType->setCurrentIndex((int)m_stDevInfo.enType);
    // main or sub1
    int nIndex = 0;
    if (AX_NVR_CHN_IDX_TYPE::MAIN == m_stDevInfo.enPreviewIndex) {
        nIndex = 0;
    } else if (AX_NVR_CHN_IDX_TYPE::SUB1 == m_stDevInfo.enPreviewIndex) {
        nIndex = 1;
    }
    ui->comboBoxPreviewIndex->setCurrentIndex(nIndex);

    nIndex = 0;
    if (AX_NVR_CHN_IDX_TYPE::MAIN == m_stDevInfo.enPatrolIndex) {
        nIndex = 0;
    } else if (AX_NVR_CHN_IDX_TYPE::SUB1 == m_stDevInfo.enPatrolIndex) {
        nIndex = 1;
    }
    ui->comboBoxPatrolIndex->setCurrentIndex(nIndex);

    // main rtsp
    ui->lineEditRtspUrlMain->setText(QString::fromUtf8((char*)m_stDevInfo.stChnMain.szRtspUrl));
    // ui->checkBoxRecordMain
    // sub1 rtsp
    ui->lineEditRtspUrlSub1->setText(QString::fromUtf8((char*)m_stDevInfo.stChnSub1.szRtspUrl));
    // ui->checkBoxSub1Record

    QDialog::showEvent(evt);
}

void DlgDevInfo::accept()
{
    // channel id
    m_stDevInfo.nChannelId = ui->comboBoxChannels->currentText().toInt() - 1;
    // alias
    QByteArray _ba = ui->lineEditAlias->text().toUtf8();
    strncpy((char*)m_stDevInfo.szAlias, _ba.data(), 64 - 1);
    m_stDevInfo.szAlias[63] = '\0';
    // type
    m_stDevInfo.enType = (AX_NVR_CHN_TYPE)ui->comboBoxType->currentIndex();

    // main or sub1
    int nIndex = ui->comboBoxPreviewIndex->currentIndex();
    AX_NVR_CHN_IDX_TYPE enIndex = AX_NVR_CHN_IDX_TYPE::MAIN;
    if (0 == nIndex) {
        enIndex = AX_NVR_CHN_IDX_TYPE::MAIN;
    } else if (1 == nIndex) {
        enIndex = AX_NVR_CHN_IDX_TYPE::SUB1;
    }
    m_stDevInfo.enPreviewIndex = enIndex;

    nIndex = ui->comboBoxPatrolIndex->currentIndex();
    enIndex = AX_NVR_CHN_IDX_TYPE::MAIN;
    if (0 == nIndex) {
        enIndex = AX_NVR_CHN_IDX_TYPE::MAIN;
    } else if (1 == nIndex) {
        enIndex = AX_NVR_CHN_IDX_TYPE::SUB1;
    }
    m_stDevInfo.enPatrolIndex = enIndex;

    // primary rtsp
    _ba = ui->lineEditRtspUrlMain->text().toUtf8();
    strncpy((char*)m_stDevInfo.stChnMain.szRtspUrl, _ba.data(), 128 - 1);
    m_stDevInfo.stChnMain.szRtspUrl[127] = '\0';
    // m_stDevInfo.stChnMain.bRecord

    // secondary rtsp
    _ba = ui->lineEditRtspUrlSub1->text().toUtf8();
    strncpy((char*)m_stDevInfo.stChnSub1.szRtspUrl, _ba.data(), 128 - 1);
    m_stDevInfo.stChnSub1.szRtspUrl[127] = '\0';

    done(QDialog::Accepted);
}
