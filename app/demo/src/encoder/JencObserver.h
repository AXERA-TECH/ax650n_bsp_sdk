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
#include "JpegEncoder.h"
#include "IObserver.h"

class CJencObserver : public IObserver {
public:
    CJencObserver(CJpegEncoder* pSink) : m_pSink(pSink){};
    virtual ~CJencObserver(AX_VOID) = default;

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

        JPEG_CONFIG_T* tJencConfig = m_pSink->GetChnCfg();
        tJencConfig->nWidth     = pParams->nWidth;
        tJencConfig->nHeight    = pParams->nHeight;
        tJencConfig->bLink      = AX_FALSE;
        tJencConfig->bFBC       = pParams->bEnableFBC;

        m_nTargetPipeChannel = eTarget << 24 | nGrp << 16 | nChn;

        return AX_TRUE;
    }

private:
    CJpegEncoder* m_pSink{nullptr};
    AX_U32 m_nTargetPipeChannel{0};
};