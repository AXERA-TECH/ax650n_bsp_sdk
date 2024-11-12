/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "AiSwitchSimulator.hpp"
#include "AppLog.hpp"
#include "PcieAdapter.hpp"
#include "ElapsedTimer.hpp"
#include "AiSwitchConfig.hpp"

#define AI_SWITCH "AI_SWITCH"

using namespace std;
using namespace aicard_mst;

AX_VOID CAiSwitchSimulator::BindTransfer(CTransferHelper* pInstance) {
    m_pTransferHelper = pInstance;
}

AX_BOOL CAiSwitchSimulator::Init(std::string strPath) {
    return AX_TRUE;
}

AX_BOOL CAiSwitchSimulator::DeInit(AX_VOID) {
    return AX_TRUE;
}

AX_BOOL CAiSwitchSimulator::Start(AX_VOID) {
    if (0 == CAiSwitchConfig::GetInstance()->GetAttrCount()) {
        LOG_MM_E(AI_SWITCH, "AI switch attr not configured.");
        return AX_FALSE;
    }

    if (!m_threadWork.Start([this](AX_VOID* pArg) -> AX_VOID { WorkThread(pArg); }, nullptr, "AiSwitch")) {
        LOG_MM_E(AI_SWITCH, "Create ai switch thread fail.");
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CAiSwitchSimulator::Stop(AX_VOID) {
    if (0 == CAiSwitchConfig::GetInstance()->GetAttrCount()) {
        LOG_MM_E(AI_SWITCH, "AI switch attr not configured.");
        return AX_FALSE;
    }

    m_threadWork.Stop();
    m_threadWork.Join();

    return AX_TRUE;
}

AX_VOID CAiSwitchSimulator::WorkThread(AX_VOID* pArg) {
    LOG_MM_I(AI_SWITCH, "+++");

    while (m_threadWork.IsRunning()) {
        AX_U32 nIntervalSec = CAiSwitchConfig::GetInstance()->GetInterval();
        while (nIntervalSec-- > 0) {
            CElapsedTimer::GetInstance()->mSleep(1000);
            if (!m_threadWork.IsRunning()) {
                break;
            }
        }

        if (m_pTransferHelper) {
            AI_CARD_AI_SWITCH_ATTR_T tAiAttr;
            if (CAiSwitchConfig::GetInstance()->GetNextAttr(tAiAttr)) {
                m_pTransferHelper->SendAiAttr(tAiAttr);
            } else {
                LOG_M_E(AI_SWITCH, "Get ai switch attr failed.");
            }
        }
    }

    LOG_MM_I(AI_SWITCH, "---");
}
