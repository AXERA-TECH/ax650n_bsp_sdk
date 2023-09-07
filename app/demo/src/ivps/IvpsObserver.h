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

#include "IObserver.h"
#include "IVPSGrpStage.h"
#define IVPS_OBS "IVPS_OBS"

class CIvpsObserver : public IObserver {
public:
    CIvpsObserver(CIVPSGrpStage* pSink) : m_pSink(pSink){};
    virtual ~CIvpsObserver(AX_VOID) = default;

public:
    AX_BOOL OnRecvData(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, AX_VOID* pData) override {
        if (!m_pSink) {
            return AX_TRUE;
        }

        if (m_nTargetPipeChannel != (eTarget << 24 | nGrp << 16 | nChn)) {
            return AX_TRUE;
        }

        CAXFrame* pFrame = (CAXFrame*)pData;
        if (E_OBS_TARGET_TYPE_COLLECT == eTarget || E_OBS_TARGET_TYPE_VIN == eTarget) {
            return m_pSink->RecvFrame(pFrame);
        } else {
            LOG_M_E(IVPS_OBS, "Recieve frame from unreconigzed module: %d", eTarget);
        }

        return AX_TRUE;
    }

    /* Usually used to fill attributes from source modules in unlink mode */
    AX_BOOL OnRegisterObserver(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, OBS_TRANS_ATTR_PTR pParams) override {
        if (nullptr == pParams) {
            return AX_FALSE;
        }

        if (E_OBS_TARGET_TYPE_VIN == eTarget || E_OBS_TARGET_TYPE_COLLECT == eTarget) {
            IVPS_GROUP_CFG_T* pGrpConfig = m_pSink->GetGrpCfg();
            AX_U8 nOutChannels = pGrpConfig->nGrpChnNum;

            pGrpConfig->nSnsSrc = pParams->nSnsSrc;
            pGrpConfig->arrGrpResolution[0] = pParams->nWidth;
            pGrpConfig->arrGrpResolution[1] = pParams->nHeight;
            pGrpConfig->arrGrpFramerate[0] = pParams->fFramerate;
            pGrpConfig->arrGrpFramerate[1] = pParams->fFramerate;
            for (AX_U8 i = 0; i < nOutChannels; i++) {
                if (-1 == pGrpConfig->arrChnResolution[i][0]) {
                    pGrpConfig->arrChnResolution[i][0] = pParams->nWidth;
                }

                if (-1 == pGrpConfig->arrChnResolution[i][1]) {
                    pGrpConfig->arrChnResolution[i][1] = pParams->nHeight;
                }

                if (-1 == pGrpConfig->arrChnFramerate[i][0]) {
                    pGrpConfig->arrChnFramerate[i][0] = pParams->fFramerate;
                }

                if (-1 == pGrpConfig->arrChnFramerate[i][1]) {
                    pGrpConfig->arrChnFramerate[i][1] = pParams->fFramerate;
                }
            }
        }

        m_nTargetPipeChannel = eTarget << 24 | nGrp << 16 | nChn;

        return AX_TRUE;
    }

private:
    CIVPSGrpStage* m_pSink{nullptr};
    AX_U32 m_nTargetPipeChannel{0};
};
