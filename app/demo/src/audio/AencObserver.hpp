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
#include "AudioEncoder.hpp"

class CAencObserver : public IObserver {
public:
    CAencObserver(CAudioEncoder* pSink) : m_pSink(pSink){};
    virtual ~CAencObserver(AX_VOID) = default;
    AX_S32 GetChannel() {
        return m_pSink->GetChannel();
    }

public:
    AX_BOOL OnRecvData(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, AX_VOID* pData) override {
        if (!m_pSink) {
            return AX_TRUE;
        }

        CAXFrame* pstFrame = (CAXFrame*)pData;
        if (!pstFrame) {
            return AX_TRUE;
        }

        if (m_nTargetPipeChannel != (eTarget << 24 | nGrp << 16 | nChn)) {
            return AX_TRUE;
        }

        return m_pSink->EnqueueFrame(pstFrame);
    }

    AX_BOOL OnRegisterObserver(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, OBS_TRANS_ATTR_PTR pParams) override {
        if (nullptr == pParams) {
            return AX_FALSE;
        }

        if (E_OBS_TARGET_TYPE_AIRAW == eTarget) {
            m_nTargetPipeChannel = eTarget << 24 | nGrp << 16 | nChn;
        }

        return AX_TRUE;
    }

private:
    CAudioEncoder* m_pSink{nullptr};
    AX_U32 m_nTargetPipeChannel{0};
};
