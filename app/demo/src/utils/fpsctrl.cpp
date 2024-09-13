/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "fpsctrl.hpp"
#include <chrono>
#include <thread>
#include "AppLogApi.h"
#include "ax_sys_api.h"

#define TAG "FPSCTRL"

CFpsCtrl::CFpsCtrl(AX_U32 nFps) {
    if (0 == nFps) {
        nFps = 30;
    }

    m_nIntervl = 1000000 / nFps;
}

AX_VOID CFpsCtrl::Control(AX_U64 nSeqNum, AX_U32 nMargin) {
    AX_SYS_GetCurPTS(&m_nCurrPTS);

    if (0 == m_nNextPTS) {
        m_nNextPTS = m_nCurrPTS + m_nIntervl;
        // LOG_M_D(TAG, "[1][%lld] 1st PTS = %lld", nSeqNum, m_nCurrPTS);
    } else {
        if (m_nNextPTS > m_nCurrPTS) {
            AX_U32 nSleep = (AX_U32)(m_nNextPTS - m_nCurrPTS);
            if (nSleep > nMargin) {
                nSleep -= nMargin;
            }

            // LOG_M_D(TAG, "[<][%lld] curr PTS %lld < next PTS %lld, sleep %d us", nSeqNum, m_nCurrPTS, m_nNextPTS, nSleep);
            std::this_thread::sleep_for(std::chrono::microseconds(nSleep));
        } else {
            LOG_M_W(TAG, "[>][%lld] curr PTS %lld > next PTS %lld, sleep 0 us", nSeqNum, m_nCurrPTS, m_nNextPTS);
        }

        m_nNextPTS += m_nIntervl;
    }
}