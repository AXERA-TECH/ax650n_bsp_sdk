/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "SettingPageDevList.h"
#include "ui_SettingPageDevList.h"
#include "global/UiGlobalDef.h"


#include "AXNVRFramework.h"
#include "AXNVRFrameworkDefine.h"
#include "AppLogApi.h"

#define TAG "UI_DEVSET"

SettingPageDevList::SettingPageDevList(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingPageDevList)
{
    ui->setupUi(this);

    this->setStyleSheet(CSS_WIDGET_1);
    ui->pushButtonAdd->setStyleSheet(CSS_PUSHBUTTON);
    ui->pushButtonDel->setStyleSheet(CSS_PUSHBUTTON);
    ui->pushButtonEdit->setStyleSheet(CSS_PUSHBUTTON);
    ui->pushButtonImport->setStyleSheet(CSS_PUSHBUTTON);
    ui->pushButtonExport->setStyleSheet(CSS_PUSHBUTTON);

    ui->tableDevList->setAlternatingRowColors(true);
    ui->tableDevList->setStyleSheet(CSS_TABLE);
    ui->tableDevList->horizontalHeader()->setStyleSheet(CSS_TABLE_HEAD);

    // ToDo:
    ui->pushButtonImport->setVisible(false);
    ui->pushButtonExport->setVisible(false);

    ui->tableDevList->setColumnWidth(m_nColChn, 50);
    ui->tableDevList->setColumnWidth(m_nColAli, 100);
    ui->tableDevList->setColumnWidth(m_nColTyp, 100);
    ui->tableDevList->setColumnWidth(m_nColPre, 100);
    ui->tableDevList->setColumnWidth(m_nColPat, 100);
    ui->tableDevList->setColumnWidth(m_nColMaU, 300);
    ui->tableDevList->setColumnWidth(m_nColMaR, 80);
    ui->tableDevList->setColumnWidth(m_nColS1U, 300);
    ui->tableDevList->setColumnWidth(m_nColS1R, 80);

    ui->tableDevList->setSelectionBehavior(QTableWidget::SelectRows);
    ui->tableDevList->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableDevList->setRowCount(1);

    connect(ui->pushButtonAdd, &QPushButton::clicked, this, &SettingPageDevList::OnAddDevInfo);
    connect(ui->pushButtonEdit, &QPushButton::clicked, this, &SettingPageDevList::OnEditDevInfo);
    connect(ui->pushButtonDel, &QPushButton::clicked, this, &SettingPageDevList::OnDeleteDevInfo);
    connect(ui->tableDevList, &QTableWidget::itemClicked, this, [=](){ m_nCurTableRow = ui->tableDevList->currentRow();});
    connect(&m_watcher, &QFutureWatcher<AX_NVR_CHNMGR_RES_TYPE>::finished, this, &SettingPageDevList::OnFinished);
}

SettingPageDevList::~SettingPageDevList() {
    delete ui;
}

void SettingPageDevList::showEvent(QShowEvent *event) {

    ui->tableDevList->setRowCount(0);
    m_nCurTableRow = -1;
    m_vecDevInfo.clear();
    CAXNVRPreviewCtrl *pPreview = CAXNVRFramework::GetInstance()->PreviewCtrl();
    if (pPreview) {
        m_vecDevInfo = pPreview->GetDevices();
        for (auto device : m_vecDevInfo) {
            int nRowIndex = ui->tableDevList->rowCount();
            ui->tableDevList->insertRow(nRowIndex);
            ui->tableDevList->setItem(nRowIndex, m_nColChn, new QTableWidgetItem(QString::number(device.nChannelId + 1)));
            ui->tableDevList->setItem(nRowIndex, m_nColAli, new QTableWidgetItem(QString::fromUtf8((char*)device.szAlias)));
            ui->tableDevList->setItem(nRowIndex, m_nColTyp, new QTableWidgetItem((AX_NVR_CHN_TYPE::COMMON == device.enType) ? "COMMON" : "FISHEYE"));
            ui->tableDevList->setItem(nRowIndex, m_nColPre, new QTableWidgetItem((AX_NVR_CHN_IDX_TYPE::MAIN == device.enPreviewIndex) ? "MAIN" : "SUB1"));
            ui->tableDevList->setItem(nRowIndex, m_nColPat, new QTableWidgetItem((AX_NVR_CHN_IDX_TYPE::MAIN == device.enPatrolIndex) ? "MAIN" : "SUB1"));
            ui->tableDevList->setItem(nRowIndex, m_nColMaU, new QTableWidgetItem(QString::fromUtf8((char*)device.stChnMain.szRtspUrl)));
            ui->tableDevList->setItem(nRowIndex, m_nColMaR, new QTableWidgetItem((1 == device.stChnMain.bRecord) ? "Enable" : "Disable"));
            ui->tableDevList->setItem(nRowIndex, m_nColS1U, new QTableWidgetItem(QString::fromUtf8((char*)device.stChnSub1.szRtspUrl)));
            ui->tableDevList->setItem(nRowIndex, m_nColS1R, new QTableWidgetItem((1 == device.stChnSub1.bRecord) ? "Enable" : "Disable"));
        }
    }
    return QWidget::showEvent(event);
}

void SettingPageDevList::OnFinished() {

    AX_NVR_CHNMGR_RES_T result = m_watcher.result();
    AX_NVR_CHNMGR_RES_TYPE enResultType = result.enResult;
    if (enResultType == AX_NVR_CHNMGR_RES_TYPE::DEL_OK) {
        int nRowIndex = result.nTableIndex;
        // remove device item in table widget
        ui->tableDevList->removeRow(nRowIndex);
        // reset current row index
        m_nCurTableRow = ui->tableDevList->currentRow();
        // remove device item in device info list
        m_vecDevInfo.erase(m_vecDevInfo.begin() + nRowIndex);
    }
    else if (enResultType == AX_NVR_CHNMGR_RES_TYPE::UPD_OK) {
        AX_NVR_DEV_INFO_T& stDevInfo = result.stDeviceInfo;
        int nRowIndex = result.nTableIndex;
        ui->tableDevList->setItem(nRowIndex, m_nColAli, new QTableWidgetItem(QString::fromUtf8((char*)stDevInfo.szAlias)));
        ui->tableDevList->setItem(nRowIndex, m_nColTyp, new QTableWidgetItem((AX_NVR_CHN_TYPE::COMMON == stDevInfo.enType) ? "COMMON" : "FISHEYE"));
        ui->tableDevList->setItem(nRowIndex, m_nColPre, new QTableWidgetItem((AX_NVR_CHN_IDX_TYPE::MAIN == stDevInfo.enPreviewIndex) ? "MAIN" : "SUB1"));
        ui->tableDevList->setItem(nRowIndex, m_nColPat, new QTableWidgetItem((AX_NVR_CHN_IDX_TYPE::MAIN == stDevInfo.enPatrolIndex) ? "MAIN" : "SUB1"));
        ui->tableDevList->setItem(nRowIndex, m_nColMaU, new QTableWidgetItem(QString::fromUtf8((char*)stDevInfo.stChnMain.szRtspUrl)));
        ui->tableDevList->setItem(nRowIndex, m_nColMaR, new QTableWidgetItem((1 == stDevInfo.stChnMain.bRecord) ? "Enable" : "Disable"));
        ui->tableDevList->setItem(nRowIndex, m_nColS1U, new QTableWidgetItem(QString::fromUtf8((char*)stDevInfo.stChnSub1.szRtspUrl)));
        ui->tableDevList->setItem(nRowIndex, m_nColS1R, new QTableWidgetItem((1 == stDevInfo.stChnSub1.bRecord) ? "Enable" : "Disable"));
        m_vecDevInfo[nRowIndex] = stDevInfo;
    }
    else if (enResultType == AX_NVR_CHNMGR_RES_TYPE::ADD_OK) {
        AX_NVR_DEV_INFO_T& stDevInfo = result.stDeviceInfo;
        int nRowIndex = ui->tableDevList->rowCount();
        ui->tableDevList->insertRow(nRowIndex);
        ui->tableDevList->setItem(nRowIndex, m_nColChn, new QTableWidgetItem(QString::number(stDevInfo.nChannelId + 1)));
        ui->tableDevList->setItem(nRowIndex, m_nColAli, new QTableWidgetItem(QString::fromUtf8((char*)stDevInfo.szAlias)));
        ui->tableDevList->setItem(nRowIndex, m_nColTyp, new QTableWidgetItem((AX_NVR_CHN_TYPE::COMMON == stDevInfo.enType) ? "COMMON" : "FISHEYE"));
        ui->tableDevList->setItem(nRowIndex, m_nColPre, new QTableWidgetItem((AX_NVR_CHN_IDX_TYPE::MAIN == stDevInfo.enPreviewIndex) ? "MAIN" : "SUB1"));
        ui->tableDevList->setItem(nRowIndex, m_nColPat, new QTableWidgetItem((AX_NVR_CHN_IDX_TYPE::MAIN == stDevInfo.enPatrolIndex) ? "MAIN" : "SUB1"));
        ui->tableDevList->setItem(nRowIndex, m_nColMaU, new QTableWidgetItem(QString::fromUtf8((char*)stDevInfo.stChnMain.szRtspUrl)));
        ui->tableDevList->setItem(nRowIndex, m_nColMaR, new QTableWidgetItem((1 == stDevInfo.stChnMain.bRecord) ? "Enable" : "Disable"));
        ui->tableDevList->setItem(nRowIndex, m_nColS1U, new QTableWidgetItem(QString::fromUtf8((char*)stDevInfo.stChnSub1.szRtspUrl)));
        ui->tableDevList->setItem(nRowIndex, m_nColS1R, new QTableWidgetItem((1 == stDevInfo.stChnSub1.bRecord) ? "Enable" : "Disable"));
        m_vecDevInfo.push_back(stDevInfo);
    }
}

void SettingPageDevList::OnAddDevInfo() {
    do {
        if (!m_watcher.isFinished()) {
            LOG_M_D(TAG, "[%s][0x%x] Still Active.", __func__, QThread::currentThread());
            break;
        }

        DlgDevInfo dlg;
        if (QDialog::Accepted == dlg.exec()) {
            AX_NVR_CHNMGR_RES_T stResult;
            stResult.stDeviceInfo = dlg.GetDeviceInfo();

            QFutureInterface<AX_NVR_CHNMGR_RES_T> interface;
            interface.reportStarted();
            m_watcher.setFuture(interface.future());
            std::thread thread([this, interface, stResult]() mutable {

                stResult.enResult = AX_NVR_CHNMGR_RES_TYPE::ADD_ERR;
                CAXNVRPreviewCtrl *pPreview = CAXNVRFramework::GetInstance()->PreviewCtrl();
                if (pPreview) {
                    if (pPreview->InsertDevice(stResult.stDeviceInfo)) {
                        stResult.enResult = AX_NVR_CHNMGR_RES_TYPE::ADD_OK;
                    }
                }

                interface.reportResult(stResult);
                interface.reportFinished();
            });

            // detach thred, Use Future to ensure exit
            thread.detach();
        }
    } while(0);
}

void SettingPageDevList::OnEditDevInfo()
{
    do {
        if (m_nCurTableRow < 0) break;

        if (!m_watcher.isFinished()) {
            LOG_M_D(TAG, "[%s][0x%x] Still Active.", __func__, QThread::currentThread());
            break;
        }

        DlgDevInfo dlg(true); // modify mode, set true
        AX_NVR_DEV_INFO_T& stDevInfo = m_vecDevInfo[m_nCurTableRow];
        dlg.SetDevInfo(stDevInfo);
        if (QDialog::Accepted == dlg.exec()) {
            AX_NVR_CHNMGR_RES_T stResult;
            stResult.stDeviceInfo = dlg.GetDeviceInfo();
            stResult.nTableIndex = m_nCurTableRow;

            QFutureInterface<AX_NVR_CHNMGR_RES_T> interface;
            interface.reportStarted();
            m_watcher.setFuture(interface.future());

            std::thread thread([this, interface, stResult]() mutable {

                stResult.enResult = AX_NVR_CHNMGR_RES_TYPE::UPD_ERR;
                CAXNVRPreviewCtrl *pPreview = CAXNVRFramework::GetInstance()->PreviewCtrl();
                if (pPreview) {
                    if (!pPreview->UpdateDevice(stResult.stDeviceInfo)) {
                        stResult.enResult = AX_NVR_CHNMGR_RES_TYPE::UPD_OK;
                    }
                }
                interface.reportResult(stResult);
                interface.reportFinished();
            });

            // detach thred, Use Future to ensure exit
            thread.detach();
        }

    } while(0);
}

void SettingPageDevList::OnDeleteDevInfo()
{
    do {
        if (m_nCurTableRow < 0) break;

        if (!m_watcher.isFinished()) {
            LOG_M_D(TAG, "[%s][0x%x] Still Active.", __func__, QThread::currentThread());
            break;
        }

        AX_NVR_CHNMGR_RES_T stResult;
        stResult.nTableIndex = m_nCurTableRow;

        vector<AX_NVR_DEV_ID> vecDeviceIds;
        vecDeviceIds.emplace_back(m_vecDevInfo[m_nCurTableRow].nChannelId);

        QFutureInterface<AX_NVR_CHNMGR_RES_T> interface;
        interface.reportStarted();
        m_watcher.setFuture(interface.future());

        std::thread thread([this, interface, stResult, vecDeviceIds]() mutable {

            stResult.enResult = AX_NVR_CHNMGR_RES_TYPE::DEL_ERR;
            CAXNVRPreviewCtrl *pPreview = CAXNVRFramework::GetInstance()->PreviewCtrl();
            if (pPreview) {
                if (!pPreview->DeleteDevice(vecDeviceIds)) {
                    stResult.enResult = AX_NVR_CHNMGR_RES_TYPE::DEL_OK;
                }
            }
            interface.reportResult(stResult);
            interface.reportFinished();
        });

        // detach thred, Use Future to ensure exit
        thread.detach();

    } while(0);
}