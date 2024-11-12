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
#include "DspStage.h"
#include "IObserver.h"
#define DSP_OBS "dsp_obs"

namespace AX_ITS {

class CDspObserver : public IObserver {
public:
    CDspObserver(CDspStage* pSink) : m_pSink(pSink){};
    virtual ~CDspObserver(AX_VOID) = default;

public:
    AX_BOOL OnRecvData(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, AX_VOID* pData) override {
        if (!m_pSink) {
            return AX_TRUE;
        }
        CAXFrame* pFrame = (CAXFrame*)pData;
        if (!pFrame) {
            return AX_TRUE;
        }
        if (m_mapChn.find(nGrp) == m_mapChn.end() || m_mapChn[nGrp] != nChn) {
            LOG_MM_D(DSP_OBS, "invalid nGrp:%d, nChn:%d", nGrp, nChn);
            return AX_TRUE;
        }
        return m_pSink->SendFrame(nGrp, pFrame);
    }

    AX_BOOL OnRegisterObserver(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, OBS_TRANS_ATTR_PTR pParams) override {
        if (!m_pSink) {
            return AX_FALSE;
        }
        DSP_ATTR_S* p_sAttr = m_pSink->GetDspAttr();
        p_sAttr->nSrcHeight = pParams->nHeight;
        p_sAttr->nSrcWidth = pParams->nWidth;
        p_sAttr->nGrp = pParams->nSnsSrc;
        p_sAttr->fSrcFramerate = pParams->fFramerate;
        m_mapChn[nGrp] = nChn;

        return AX_TRUE;
    }

private:
    CDspStage* m_pSink{nullptr};
    std::map<AX_U32, AX_U32> m_mapChn;
};

}  // namespace AX_ITS