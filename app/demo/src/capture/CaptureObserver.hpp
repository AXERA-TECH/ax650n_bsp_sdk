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
#include "Capture.hpp"
#include "IObserver.h"

class CAXFrame;

class CCaptureObserver : public IObserver {
public:
    CCaptureObserver(CCapture* pSink) : m_pSink(pSink){};
    ~CCaptureObserver(AX_VOID) = default;

public:
    AX_BOOL OnRecvData(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, AX_VOID* pData) override {
        CAXFrame* pFrame = static_cast<CAXFrame*>(pData);
        return m_pSink->SendFrame(m_nSnsSrc, pFrame);
    }

    AX_BOOL OnRegisterObserver(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, OBS_TRANS_ATTR_PTR pParams) override {
        m_nSnsSrc = pParams->nSnsSrc;
        return AX_TRUE;
    }

private:
    CCapture* m_pSink{nullptr};
     AX_S8 m_nSnsSrc{0};
};
