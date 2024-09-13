/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "PrintHelper.h"
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
            m_stVencStatisticsInfo.Print();
            m_stMJencStatisticsInfo.Print();
            m_stJencStatisticsInfo.Print();
            pThis->m_nTickStart = pThis->m_nTickEnd;
        }

        CElapsedTimer::GetInstance()->mSleep(1000);
    }

    m_stVencStatisticsInfo.FinalPrint();
    m_stMJencStatisticsInfo.FinalPrint();
    m_stJencStatisticsInfo.FinalPrint();
}

AX_VOID CPrintHelper::Start() {
    LOG_M(PRINT_HELPER, "+++");

#ifdef SLT
    m_nSLTResult = -1;
    m_bSLTThreadWorking = AX_TRUE;
    m_hPrintThread = std::thread(&CPrintHelper::SLTThreadFunc, this, this);
#else
    m_bPrintThreadWorking = AX_TRUE;
    m_hPrintThread = std::thread(&CPrintHelper::PrintThreadFunc, this, this);
#endif

    LOG_M(PRINT_HELPER, "---");
}

AX_VOID CPrintHelper::Stop() {
    LOG_M(PRINT_HELPER, "+++");

    if (m_bPrintThreadWorking) {
        m_bPrintThreadWorking = AX_FALSE;
    }

    if (m_bSLTThreadWorking) {
        m_bSLTThreadWorking = AX_FALSE;
    }

    if (m_hPrintThread.joinable()) {
        m_hPrintThread.join();
    }

    m_bEnableStart = AX_FALSE;
    m_nTickStart = 0;
    m_nTickEnd = 0;
    m_stVencStatisticsInfo.ClearAll();
    m_stJencStatisticsInfo.ClearAll();
    m_stMJencStatisticsInfo.ClearAll();

    LOG_M(PRINT_HELPER, "---");
}

AX_VOID CPrintHelper::Add(PRINT_HELPER_MOD_E eModType, AX_U32 nChn) {
    if (!m_hPrintThread.joinable()) {
        return;
    }

#ifdef SLT
    if (!m_bEnableStart) {
        LOG_M(PRINT_HELPER, "<----- SLT fps statistic begin ----->");
    }
#endif

    m_bEnableStart = AX_TRUE;
    switch (eModType) {
        case E_PH_MOD_VENC:
            m_stVencStatisticsInfo.Add(nChn);
            break;
        case E_PH_MOD_JENC:
            m_stJencStatisticsInfo.Add(nChn);
            break;
        case E_PH_MOD_MJENC:
            m_stMJencStatisticsInfo.Add(nChn);
            break;
        default:
            break;
    }
}

AX_VOID CPrintHelper::Reset(PRINT_HELPER_MOD_E eModType, AX_U32 nChn) {
    if (!m_hPrintThread.joinable()) {
        return;
    }

#ifdef SLT
    if (!m_bEnableStart) {
        LOG_M(PRINT_HELPER, "<----- SLT fps statistic begin ----->");
    }
#endif

    switch (eModType) {
        case E_PH_MOD_VENC:
            m_stVencStatisticsInfo.Reset(nChn);
            break;
        case E_PH_MOD_JENC:
            m_stJencStatisticsInfo.Reset(nChn);
            break;
        case E_PH_MOD_MJENC:
            m_stMJencStatisticsInfo.Reset(nChn);
            break;
        default:
            break;
    }
}

AX_S32 CPrintHelper::GetVencFramerate(AX_U32 nChn) {
    AX_U32 nTickEnd = CElapsedTimer::GetInstance()->GetTickCount();
    return m_stVencStatisticsInfo.nVencPeriodFrames[nChn] * 1000 / (nTickEnd - m_nTickStart);
}

AX_VOID CPrintHelper::SetSLTTargetFPS(PRINT_HELPER_MOD_E eModType, AX_U32 nChn, AX_U32 nTargetFPS) {
    switch (eModType) {
        case E_PH_MOD_VENC:
            m_stVencStatisticsInfo.SetTargetFPS(nChn, nTargetFPS);
            break;
        default:
            break;
    }
}

AX_S32 CPrintHelper::GetSLTResult() const {
    return m_nSLTResult;
}

AX_VOID CPrintHelper::SetSLTResult(AX_S32 nRet) {
    m_nSLTResult = nRet;
}

AX_VOID CPrintHelper::SLTThreadFunc(CPrintHelper* pCaller) {
    m_bSLTThreadWorking = AX_TRUE;
    AX_U64 nThreadTickStart = CElapsedTimer::GetInstance()->GetTickCount();
    AX_U64 nTickStart = 0;
    AX_U64 nTickEnd = 0;
    LOG_M_C(PRINT_HELPER, "Target period: %d seconds", COptionHelper::GetInstance()->GetSLTRunTime());
    LOG_M_C(PRINT_HELPER, "Check frequency: %d seconds", COptionHelper::GetInstance()->GetSLTFpsCheckFreq());
    LOG_M_C(PRINT_HELPER, "Target fps: %d", m_stVencStatisticsInfo.nVencTargetFPS[0]);
    LOG_M_C(PRINT_HELPER, "Max fps diff: %d", COptionHelper::GetInstance()->GetSLTFpsDiff());

    while (m_bSLTThreadWorking) {
        if (!m_bEnableStart) {
            nTickStart = CElapsedTimer::GetInstance()->GetTickCount();
            if (nTickStart - nThreadTickStart >= COptionHelper::GetInstance()->GetSLTRunTime() * 1000) {
                LOG_M_E(PRINT_HELPER, "No data received for whole slt statistics period. Quit SLT.");
                SetSLTResult(1);
                break;
            }
            CElapsedTimer::GetInstance()->mSleep(1);
            continue;
        }

        nTickEnd = CElapsedTimer::GetInstance()->GetTickCount();
        if (nTickEnd - nThreadTickStart >= COptionHelper::GetInstance()->GetSLTRunTime() * 1000) {
            SetSLTResult(0);
            m_bSLTThreadWorking = AX_FALSE;
            break;
        }

        if (nTickEnd - nTickStart >= COptionHelper::GetInstance()->GetSLTFpsCheckFreq() * 1000) {
            AX_BOOL bResult = m_stVencStatisticsInfo.SLT_CheckResult();
            if (!bResult) {
                SetSLTResult(1);
                m_bSLTThreadWorking = AX_FALSE;
                break;
            }

            nTickStart = nTickEnd;
        }

        CElapsedTimer::GetInstance()->uSleep(100);
    }

    LOG_M(PRINT_HELPER, "<----- SLT fps statistic end ----->");
}
