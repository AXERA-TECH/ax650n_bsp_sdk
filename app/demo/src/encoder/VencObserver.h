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
#include "AXFrame.hpp"
#include "IObserver.h"
#include "VideoEncoder.h"

class CVencObserver : public IObserver {
public:
    CVencObserver(CVideoEncoder* pSink) : m_pSink(pSink) {
    }
    virtual ~CVencObserver(AX_VOID) = default;
    AX_S32 GetChannel() {
        return m_pSink->GetChannel();
    }

public:
    AX_BOOL OnRecvData(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, AX_VOID* pData) override {
        if (!m_pSink) {
            return AX_TRUE;
        }
        CAXFrame* pFrame = (CAXFrame*)pData;
        if (!pFrame) {
            return AX_TRUE;
        }

        if (m_nTargetPipeChannel != (eTarget << 24 | nGrp << 16 | nChn)) {
            return AX_TRUE;
        }

        return m_pSink->EnqueueFrame(pFrame);
    }

    AX_BOOL OnRegisterObserver(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, OBS_TRANS_ATTR_PTR pParams) override {
        if (nullptr == pParams) {
            return AX_FALSE;
        }

        if (E_OBS_TARGET_TYPE_IVPS == eTarget || E_OBS_TARGET_TYPE_VIN == eTarget || E_OBS_TARGET_TYPE_COLLECT == eTarget) {
            VIDEO_CONFIG_T* tVideoConfig = m_pSink->GetChnCfg();
            tVideoConfig->fFramerate = pParams->fFramerate;
            tVideoConfig->nWidth = pParams->nWidth;
            tVideoConfig->nHeight = pParams->nHeight;
            tVideoConfig->bFBC = pParams->bEnableFBC;
            tVideoConfig->bLink = AX_FALSE;

            m_nTargetPipeChannel = eTarget << 24 | nGrp << 16 | nChn;
        }

        return AX_TRUE;
    }

private:
    CVideoEncoder* m_pSink{nullptr};
    AX_U32 m_nTargetPipeChannel{0};
};