/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "ts_main.hpp"
#include "AXNVRFrameworkDefine.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

#define TAG "TS_MAIN"


CTestMain::CTestMain() {
    m_strModuleName = TAG;
}

AX_BOOL CTestMain::Start() {
    m_bRunning = AX_TRUE;
    testOpenPatrol();
    m_bRunning = AX_FALSE;

    return AX_TRUE;
}

AX_BOOL CTestMain::EmitUIOprSignal(const TS_OPERATION_INFO_T& tOprInfo) {
    if (tOprInfo.target) {
        emit signal_ts_widget_opr(tOprInfo);
        return AX_TRUE;
    }

    return AX_FALSE;
}

void CTestMain::testVersion() {
    QLabel* pLabelVersion = (QLabel*)GetWidget("LabelVersion");
    if (nullptr == pLabelVersion) {
        LOG_M_E(TAG, "Can not find label version");
        return;
    }

    if (pLabelVersion->text().toStdString() == NVR_VERSION_STR) {
        LOG_MM_C(TAG, "Action<%s> successed." , "CheckVersion");
    } else {
        LOG_MM_E(TAG, "Action<%s> failed." , "CheckVersion");
    }
}

void CTestMain::testOpenPatrol() {
    QPushButton* pBtnPolling = (QPushButton*)GetWidget("BtnPolling");
    if (nullptr == pBtnPolling) {
        LOG_M_E(TAG, "Can not find button polling");
        return;
    }

    AX_NVR_TS_MODULE_INFO_PTR pModuleCfg = GetModuleConfig();
    AX_NVR_TS_CASE_INFO_T tCaseInfo = {"polling", TS_DEFAULT_REPEAT_COUNT, TS_DEFAULT_OPERATION_INTERFAL};
    GetCaseInfo("polling", pModuleCfg->vecCaseInfo, tCaseInfo);

    AX_S32 nRepeatCount = tCaseInfo.nRepeatCount;

    while (nRepeatCount-- > 0 && m_bRunning) {
        if (!CheckEntry()) {
            break;
        }

        ActionToggleBtnClick(pBtnPolling, tCaseInfo.nOprInterval, "polling");
    }
}

AX_BOOL CTestMain::GetCaseInfo(const string& strCaseName, vector<AX_NVR_TS_CASE_INFO_T>& vecCaseInfo, AX_NVR_TS_CASE_INFO_T& tOutInfo) {
    for (auto &m : vecCaseInfo) {
        if (m.strName == strCaseName) {
            tOutInfo = m;
            return AX_TRUE;
        }
    }

    return AX_FALSE;
}

string CTestMain::GetActionDesc(MAIN_ACTION_TYPE eType) {
    switch (eType) {
        case MAIN_ACTION_TYPE::UNKNOWN: return "Unknown";
        case MAIN_ACTION_TYPE::MAIN_SHOW: return "Show";
        case MAIN_ACTION_TYPE::MAIN_SHOW_PREVIEW: return "ShowPreview";
        case MAIN_ACTION_TYPE::MAIN_SHOW_PLAYBACK: return "ShowPlayback";
        case MAIN_ACTION_TYPE::MAIN_SHOW_SETTING: return "ShowSetting";
        default: return "Unknown";
    }

    return "Unknown";
}

void CTestMain::OnRecvResult(const AX_NVR_ACTION_RES_T& tActionResult) {
    LOG_MM_C(TAG, "Action<%s> successed.", GetActionDesc(tActionResult.enMainActionType).c_str());
}
