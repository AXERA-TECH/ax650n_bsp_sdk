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
#include <map>
#include "AXFrame.hpp"
#include "Detector.hpp"
#include "IObserver.h"
#define DETECTOR_OBS "detector_obs"

class CDetectObserver : public IObserver {
public:
    CDetectObserver(CDetector* pSink) : m_pSink(pSink){};
    virtual ~CDetectObserver(AX_VOID) = default;

public:
    AX_BOOL OnRecvData(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, AX_VOID* pData) override {
        if (!m_pSink) {
            return AX_TRUE;
        }
        CAXFrame* pFrame = (CAXFrame*)pData;
        if (!pFrame) {
            return AX_TRUE;
        }
        if (m_mapSnsMatch.find(nGrp) == m_mapSnsMatch.end()) {
            LOG_MM_D(DETECTOR_OBS, "invalid nGrp:%d", nGrp);
            pFrame->FreeMem();
            return AX_TRUE;
        }
        return m_pSink->SendFrame(m_mapSnsMatch[nGrp], pFrame);
    }

    AX_BOOL OnRegisterObserver(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, OBS_TRANS_ATTR_PTR pParams) override {
        DETECTOR_ATTR_T* pConfig = m_pSink->GetDetectorCfg();
        if (pParams->bEnableFBC) {
            LOG_M_E(DETECTOR_OBS, "[%d][%d] AI detector module does not support FBC input frames.", nGrp, nChn);
            return AX_FALSE;
        }

        if (pParams->nWidth * pParams->nHeight > pConfig->nWidth * pConfig->nHeight) {
            pConfig->nWidth = pParams->nWidth;
            pConfig->nHeight = pParams->nHeight;
        }

        m_mapSnsMatch[nGrp] = pParams->nSnsSrc;

        return AX_TRUE;
    }

private:
    CDetector* m_pSink{nullptr};
    std::map<AX_U32, AX_U32> m_mapSnsMatch;
};
