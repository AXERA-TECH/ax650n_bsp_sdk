/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#ifndef SETTINGPAGEDEVLIST_H
#define SETTINGPAGEDEVLIST_H
#include <QFutureWatcher>
#include <QFutureInterface>
#include <QThread>
#include <QWidget>
#include <vector>

#include "SettingPageBase.h"
#include "DlgDevInfo.h"


QT_BEGIN_NAMESPACE
namespace Ui { class SettingPageDevList; }
QT_END_NAMESPACE

enum class AX_NVR_CHNMGR_TABLE_COL {
     CHANNEL    = 0,
     ALIAS,
     TYPE,
     PREVIEW,
     PATROL,
     MAINURL,
     MAINRECORD,
     SUB1URL,
     SUB1RECORD,
};

enum class AX_NVR_CHNMGR_RES_TYPE {
     UNKONW     = 0x00,
     ADD_ERR    = 0x10,
     DEL_ERR    = 0x20,
     UPD_ERR    = 0x30,
     ADD_OK     = 0x11,
     DEL_OK     = 0x21,
     UPD_OK     = 0x31,
};

typedef struct _AX_NVR_CHNMGR_RES_T {
    AX_NVR_CHNMGR_RES_TYPE enResult;
    int nTableIndex;
    AX_NVR_DEV_INFO_T stDeviceInfo;

    _AX_NVR_CHNMGR_RES_T() {
        enResult = AX_NVR_CHNMGR_RES_TYPE::UNKONW;
        nTableIndex = -1;
        //memset(&stDeviceInfo, 0x0, sizeof(AX_NVR_DEV_INFO_T));
    }
} AX_NVR_CHNMGR_RES_T;

class SettingPageDevList : public QWidget, public SettingPageBase
{
    Q_OBJECT

public:
    SettingPageDevList(QWidget *parent = nullptr);
    ~SettingPageDevList();

protected:
    virtual void showEvent(QShowEvent *event) override;

    int m_nCurTableRow = -1;
    std::vector<AX_NVR_DEV_INFO_T> m_vecDevInfo;
    QFutureWatcher<AX_NVR_CHNMGR_RES_T> m_watcher;

private slots:
    void OnAddDevInfo();
    void OnEditDevInfo();
    void OnDeleteDevInfo();
    void OnFinished();

public:
    virtual int OnLoad() override {return 0;};
    virtual int OnSave() override {return 0;};

private:
    Ui::SettingPageDevList *ui;
    int m_nColChn = (int)AX_NVR_CHNMGR_TABLE_COL::CHANNEL;
    int m_nColAli = (int)AX_NVR_CHNMGR_TABLE_COL::ALIAS;
    int m_nColTyp = (int)AX_NVR_CHNMGR_TABLE_COL::TYPE;
    int m_nColPre = (int)AX_NVR_CHNMGR_TABLE_COL::PREVIEW;
    int m_nColPat = (int)AX_NVR_CHNMGR_TABLE_COL::PATROL;
    int m_nColMaU = (int)AX_NVR_CHNMGR_TABLE_COL::MAINURL;
    int m_nColMaR = (int)AX_NVR_CHNMGR_TABLE_COL::MAINRECORD;
    int m_nColS1U = (int)AX_NVR_CHNMGR_TABLE_COL::SUB1URL;
    int m_nColS1R = (int)AX_NVR_CHNMGR_TABLE_COL::SUB1RECORD;
};
#endif // SETTINGPAGEDEVLIST_H