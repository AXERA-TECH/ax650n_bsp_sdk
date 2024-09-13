/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "PrintHelper.hpp"
#include "ElapsedTimer.hpp"
#include "OptionHelper.h"

#define PRINT_HELPER "PRINT"


AX_VOID CPrintHelper::PrintThreadFunc(CPrintHelper* pCaller) {
    prctl(PR_SET_NAME, "FRTDemo_PRINT");

    CPrintHelper* pThis = (CPrintHelper*)pCaller;
    pThis->m_nTickStart = CElapsedTimer::GetInstance()->GetTickCount();

    while (pThis->m_bPrintThreadWorking) {
        if (!pThis->m_bEnableStart) {
            pThis->m_nTickStart = CElapsedTimer::GetInstance()->GetTickCount();
            CElapsedTimer::GetInstance()->mSleep(100);
            continue;
        }

        pThis->m_nTickEnd = CElapsedTimer::GetInstance()->GetTickCount();
        if ((pThis->m_nTickEnd - pThis->m_nTickStart) >= PRINT_INTERVAL * 1000) {
            m_tPcieRecv.Print();
            m_tPcieSend.Print();

            pThis->m_nTickStart = pThis->m_nTickEnd;
        }

        CElapsedTimer::GetInstance()->mSleep(1000);
    }
}

AX_VOID CPrintHelper::Start() {
    LOG_M_I(PRINT_HELPER, "+++");

    m_bPrintThreadWorking = AX_TRUE;
    m_hPrintThread = std::thread(&CPrintHelper::PrintThreadFunc, this, this);

    LOG_M_I(PRINT_HELPER, "---");
}

AX_VOID CPrintHelper::Stop() {
    LOG_M_I(PRINT_HELPER, "+++");

    if (m_bPrintThreadWorking) {
        m_bPrintThreadWorking = AX_FALSE;
    }

    if (m_hPrintThread.joinable()) {
        m_hPrintThread.join();
    }

    m_bEnableStart = AX_FALSE;
    m_nTickStart = 0;
    m_nTickEnd = 0;
    m_tPcieSend.ClearAll();
    m_tPcieRecv.ClearAll();

    LOG_M_I(PRINT_HELPER, "---");
}

AX_VOID CPrintHelper::Add(PRINT_HELPER_MOD_E eModType, AX_U32 nChn) {
    if (!m_hPrintThread.joinable()) {
        return;
    }

    m_bEnableStart = AX_TRUE;
    switch (eModType) {
        case E_PH_MOD_PCIE_RECV:
            m_tPcieRecv.Add(nChn);
            break;
        case E_PH_MOD_PCIE_SEND:
            m_tPcieSend.Add(nChn);
            break;
        case E_PH_MOD_VDEC:
            break;
        default:
            break;
    }
}
