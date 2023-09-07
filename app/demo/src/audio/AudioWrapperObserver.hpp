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
#include "AudioWrapper.hpp"
#include "AudioWrapperUtils.hpp"

class CAudioWrapperObserver : public IObserver {
public:
    CAudioWrapperObserver(AX_VOID* pSink, AX_VOID *pUserData) : m_pSink(pSink), m_pUserData(pUserData){};
    virtual ~CAudioWrapperObserver(AX_VOID) = default;

public:
    AX_BOOL OnRecvData(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, AX_VOID* pData) override {
        if (!m_pSink) {
            return AX_TRUE;
        }

        if (m_nTargetPipeChannel != (eTarget << 24 | nGrp << 16 | nChn)) {
            return AX_TRUE;
        }

        if (E_OBS_TARGET_TYPE_AIRAW == eTarget) {
            AX_APP_AUDIO_FRAME_CALLBACK callback = (AX_APP_AUDIO_FRAME_CALLBACK)m_pSink;
            AX_APP_AUDIO_FRAME_T stFrame;
            AUDIO_CAP_FRAME_PTR pstFrame = (AUDIO_CAP_FRAME_PTR)pData;

            stFrame.eType = pstFrame->eType;
            stFrame.eBitWidth = _APP_BIT_WIDTH(pstFrame->eBitWidth);
            stFrame.eSampleRate = _APP_SAMPLE_RATE(pstFrame->eSampleRate);
            stFrame.nChnCnt = pstFrame->nChnCnt;
            stFrame.pData = (AX_U8 *)pstFrame->stAFrame.u64VirAddr;
            stFrame.nDataSize = (AX_U32)pstFrame->stAFrame.u32Len;
            stFrame.pUserData = m_pUserData;
            stFrame.pPrivateData = nullptr;

            callback(&stFrame);
        }
        else if (E_OBS_TARGET_TYPE_AENC == eTarget) {
            AX_APP_AUDIO_PKT_CALLBACK callback = (AX_APP_AUDIO_PKT_CALLBACK)m_pSink;
            AX_APP_AUDIO_PKT_T stPkt;
            AENC_STREAM_PTR pstStream = (AENC_STREAM_PTR)pData;

            memset(&stPkt, 0x00, sizeof(stPkt));
            stPkt.nBitRate = pstStream->nBitRate;
            stPkt.eType = pstStream->eType;
            stPkt.eBitWidth = _APP_BIT_WIDTH(pstStream->eBitWidth);
            stPkt.eSampleRate = _APP_SAMPLE_RATE(pstStream->eSampleRate);
            stPkt.eSoundMode = _APP_SOUND_MODE(pstStream->eSoundMode);
            stPkt.pData = pstStream->stPack.pStream;
            stPkt.nDataSize = pstStream->stPack.u32Len;
            stPkt.u64Pts = pstStream->stPack.u64TimeStamp;
            stPkt.pUserData = m_pUserData;
            stPkt.pPrivateData = nullptr;

            callback((AX_APP_AUDIO_CHAN_E)nChn, &stPkt);
        }

        return AX_TRUE;
    }

    AX_BOOL OnRegisterObserver(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, OBS_TRANS_ATTR_PTR pParams) override {
        if (nullptr == pParams) {
            return AX_FALSE;
        }

        if (E_OBS_TARGET_TYPE_AIRAW == eTarget) {
            m_nTargetPipeChannel = eTarget << 24 | nGrp << 16 | nChn;
        }
        else if (E_OBS_TARGET_TYPE_AENC == eTarget) {
            m_nTargetPipeChannel = eTarget << 24 | nGrp << 16 | nChn;
        }

        return AX_TRUE;
    }

private:
    AX_VOID* m_pSink{nullptr};
    AX_VOID* m_pUserData{nullptr};
    AX_U32 m_nTargetPipeChannel{0};
};

