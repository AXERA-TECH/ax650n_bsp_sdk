/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#pragma once

#include "ISensor.hpp"
#include "ax_isp_3a_api.h"

class CIspAlgoWrapper final {
public:
    CIspAlgoWrapper(AX_VOID) {
        /* by default, AE and AWB is actived with Axera algorithm */
        m_tAlg.bActiveAe = AX_TRUE;
        m_tAlg.bUserAe = AX_FALSE;
        m_tAlg.tAeFns.pfnAe_Init = AX_ISP_ALG_AeInit;
        m_tAlg.tAeFns.pfnAe_Exit = AX_ISP_ALG_AeDeInit;
        m_tAlg.tAeFns.pfnAe_Run = AX_ISP_ALG_AeRun;
        m_tAlg.tAeFns.pfnAe_Ctrl = AX_ISP_ALG_AeCtrl;

        m_tAlg.bActiveAwb = AX_TRUE;
        m_tAlg.bUserAwb = AX_FALSE;
        m_tAlg.tAwbFns.pfnAwb_Init = AX_ISP_ALG_AwbInit;
        m_tAlg.tAwbFns.pfnAwb_Exit = AX_ISP_ALG_AwbDeInit;
        m_tAlg.tAwbFns.pfnAwb_Run = AX_ISP_ALG_AwbRun;
    }
    ~CIspAlgoWrapper(AX_VOID) = default;

    AX_VOID SetupAlgoInfo(const ISP_ALGO_INFO_T &tAlg) {
        m_tAlg = tAlg;
    };

    AX_BOOL RegisterAlgoToSensor(AX_SENSOR_REGISTER_FUNC_T *pSnsObj, AX_U8 nPipe);
    AX_BOOL UnRegisterAlgoFromSensor(AX_U8 nPipe);

private:
    ISP_ALGO_INFO_T m_tAlg;
};