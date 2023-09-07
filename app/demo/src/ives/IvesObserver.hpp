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
#include <vector>
#include "AXFrame.hpp"
#include "IObserver.h"
#include "IVESStage.h"

#define IVES_OBS "ives_obs"

typedef struct _IVES_GRP_CHN_MAP_T {
    AX_U32 nGrp{0};
    AX_U32 nChn{0};
    AX_U32 nSnsSrc{0};
} IVES_GRP_CHN_MAP_T;
class CIvesObserver : public IObserver {
public:
    CIvesObserver(CIVESStage* pSink) : m_pSink(pSink){};
    virtual ~CIvesObserver(AX_VOID) = default;

public:
    AX_BOOL OnRecvData(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, AX_VOID* pData) override {
        if (!m_pSink) {
            return AX_TRUE;
        }

        CAXFrame* pFrame = (CAXFrame*)pData;
        if (!pFrame) {
            return AX_TRUE;
        }
        if (m_mapChn.find(nGrp) == m_mapChn.end() || m_mapChn[nGrp].nChn != nChn) {
            return AX_TRUE;
        }
        m_pSink->SendFrame(m_mapChn[nGrp].nSnsSrc, pFrame);
        return AX_TRUE;
    }

    AX_BOOL OnRegisterObserver(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, OBS_TRANS_ATTR_PTR pParams) override {
        IVES_ATTR_T* pAttr = m_pSink->GetIVESCfg();
        pAttr->nWidth = pParams->nWidth;
        pAttr->nHeight = pParams->nHeight;
        pAttr->fSrcFramerate = pParams->fSrcFramerate;
        pAttr->nGrpCount++;
        pAttr->nSnsSrc = pParams->nSnsSrc;

        IVES_GRP_CHN_MAP_T stGCMap;
        stGCMap.nChn = nChn;
        stGCMap.nSnsSrc = pParams->nSnsSrc;
        m_mapChn[nGrp] = stGCMap;
        return AX_TRUE;
    }

private:
    CIVESStage* m_pSink{nullptr};
    std::map<AX_U32, IVES_GRP_CHN_MAP_T> m_mapChn;
};
