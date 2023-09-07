/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "IspAlgoWrapper.hpp"
#include <AppLogApi.h>

#define ALGO_WRAPPER "ALGO_WRAPPER"

AX_BOOL CIspAlgoWrapper::RegisterAlgoToSensor(AX_SENSOR_REGISTER_FUNC_T *pSnsObj, AX_U8 nPipe) {
    AX_S32 ret;
    if (!pSnsObj) {
        LOG_M_E(ALGO_WRAPPER, "%s: pipe %d sns instance is nil", __func__, nPipe);
        return AX_FALSE;
    }

    if (m_tAlg.bActiveAe) {
        if (!m_tAlg.tAeFns.pfnAe_Init || !m_tAlg.tAeFns.pfnAe_Run || !m_tAlg.tAeFns.pfnAe_Exit || !m_tAlg.tAeFns.pfnAe_Ctrl) {
            LOG_M_E(ALGO_WRAPPER, "Invalid AE algorithm function");
            return AX_FALSE;
        }

        if (!m_tAlg.bUserAe) {
            /* Register the sensor driven interface TO the AE library */
            ret = AX_ISP_ALG_AeRegisterSensor(nPipe, pSnsObj);
            if (0 != ret) {
                LOG_M_E(ALGO_WRAPPER, "AX_ISP_ALG_AeRegisterSensor(pipe: %d) fail, ret = 0x%08X", nPipe, ret);
                return AX_FALSE;
            }
        }

        ret = AX_ISP_RegisterAeLibCallback(nPipe, &m_tAlg.tAeFns);
        if (0 != ret) {
            LOG_M_E(ALGO_WRAPPER, "AX_ISP_RegisterAeLibCallback(pipe: %d) fail, ret = 0x%08X", nPipe, ret);
            return AX_FALSE;
        }
    }

    if (m_tAlg.bActiveAwb) {
        if (!m_tAlg.tAwbFns.pfnAwb_Init || !m_tAlg.tAwbFns.pfnAwb_Run || !m_tAlg.tAwbFns.pfnAwb_Exit) {
            LOG_M_E(ALGO_WRAPPER, "Invalid AWB algorithm function");
            return AX_FALSE;
        }

        ret = AX_ISP_RegisterAwbLibCallback(nPipe, &m_tAlg.tAwbFns);
        if (0 != ret) {
            LOG_M_E(ALGO_WRAPPER, "AX_ISP_RegisterAwbLibCallback(pipe: %d) fail, ret = 0x%08X", nPipe, ret);
            return AX_FALSE;
        }
    }

    if (m_tAlg.bActiveLsc && m_tAlg.bUserLsc) {
        if (!m_tAlg.bUserLsc) {
            LOG_M_W(ALGO_WRAPPER, "Axera LSC is no need to register");
        } else {
            if (!m_tAlg.tLscFns.pfnLsc_Init || !m_tAlg.tLscFns.pfnLsc_Run || !m_tAlg.tLscFns.pfnLsc_Exit) {
                LOG_M_E(ALGO_WRAPPER, "Invalid LSC algorithm function");
                return AX_FALSE;
            }
            ret = AX_ISP_RegisterLscLibCallback(nPipe, &m_tAlg.tLscFns);
            if (0 != ret) {
                LOG_M_E(ALGO_WRAPPER, "AX_ISP_RegisterLscLibCallback(pipe: %d) fail, ret = 0x%08X", nPipe, ret);
                return AX_FALSE;
            }
        }
    }

    return AX_TRUE;
}

AX_BOOL CIspAlgoWrapper::UnRegisterAlgoFromSensor(AX_U8 nPipe) {
    AX_S32 ret;
    if (m_tAlg.bActiveAe) {
        if (!m_tAlg.bUserAe) {
            ret = AX_ISP_ALG_AeUnRegisterSensor(nPipe);
            if (0 != ret) {
                LOG_M_E(ALGO_WRAPPER, "AX_ISP_ALG_AeUnRegisterSensor(pipe: %d) fail, ret = 0x%08X", nPipe, ret);
                return AX_FALSE;
            }
        }

        ret = AX_ISP_UnRegisterAeLibCallback(nPipe);
        if (0 != ret) {
            LOG_M_E(ALGO_WRAPPER, "AX_ISP_UnRegisterAeLibCallback(pipe: %d) fail, ret = 0x%08X", nPipe, ret);
            return AX_FALSE;
        }
    }

    if (m_tAlg.bActiveAwb) {
        ret = AX_ISP_UnRegisterAwbLibCallback(nPipe);
        if (0 != ret) {
            LOG_M_E(ALGO_WRAPPER, "AX_ISP_UnRegisterAwbLibCallback(pipe: %d) fail, ret = 0x%08X", nPipe, ret);
            return AX_FALSE;
        }
    }

    if (m_tAlg.bActiveLsc && m_tAlg.bUserLsc) {
        ret = AX_ISP_UnRegisterLscLibCallback(nPipe);
        if (0 != ret) {
            LOG_M_E(ALGO_WRAPPER, "AX_ISP_UnRegisterLscLibCallback(pipe: %d) fail, ret = 0x%08X", nPipe, ret);
            return AX_FALSE;
        }
    }

    return AX_TRUE;
}
