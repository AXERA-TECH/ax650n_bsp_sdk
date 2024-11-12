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
#include "AudioPlayDev.hpp"

class CAPlayObserver : public IObserver {
public:
    CAPlayObserver(CAudioPlayDev* pSink) : m_pSink(pSink){};
    virtual ~CAPlayObserver(AX_VOID) = default;

public:
    AX_BOOL OnRecvData(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, AX_VOID* pData) override {
        if (!m_pSink) {
            return AX_TRUE;
        }

        AX_AUDIO_FRAME_T *pstFrame = (AX_AUDIO_FRAME_T *)pData;
        if (!pstFrame) {
            return AX_TRUE;
        }

        if (m_nTargetPipeChannel != (eTarget << 24 | nGrp << 16 | nChn)) {
            return AX_TRUE;
        }

        AX_U32 &nCardId = m_pSink->GetDevAttr()->nCardId;
        AX_U32 &nDeviceId = m_pSink->GetDevAttr()->nDeviceId;

        AX_S32 nRet = AX_AO_SendFrame((AX_S32)nCardId, (AX_S32)nDeviceId, pstFrame, -1);

        return ((0 == nRet) ? AX_TRUE : AX_FALSE);
    }

    AX_BOOL OnRegisterObserver(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, OBS_TRANS_ATTR_PTR pParams) override {
        if (nullptr == pParams) {
            return AX_FALSE;
        }

        if (E_OBS_TARGET_TYPE_APLAY == eTarget) {
            m_nTargetPipeChannel = eTarget << 24 | nGrp << 16 | nChn;
        }

        return AX_TRUE;
    }

private:
    CAudioPlayDev* m_pSink{nullptr};
    AX_U32 m_nTargetPipeChannel{0};
};
