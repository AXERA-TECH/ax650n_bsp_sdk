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
#include "AXRtspServer.h"
#include "IObserver.h"
#include "OptionHelper.h"
#include "AudioOptionHelper.h"

#define RTSP_OBS "RTSP_OBS"

class CAXRtspObserver : public IObserver {
public:
    CAXRtspObserver(CAXRtspServer* pSink) : m_pSink(pSink){};
    virtual ~CAXRtspObserver(AX_VOID) = default;
public:
    AX_BOOL IsSame(AX_U32 nGrp, AX_U32 nChn) const {
        return (m_nGroup == nGrp && m_nChannel == nChn) ? AX_TRUE : AX_FALSE;
    }

    AX_VOID UpdateVideoPayloadType(AX_BOOL bEnable, AX_PAYLOAD_TYPE_E ePayloadType) {
        m_stSessAttr.stVideoAttr.bEnable = bEnable;
        m_stSessAttr.stVideoAttr.ePt = ePayloadType;
        m_pSink->UpdateSessionAttr(m_nChannel, m_stSessAttr);
    }

public:
    AX_BOOL OnRecvData(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, AX_VOID* pData) override {
        if (!m_pSink) {
            return AX_TRUE;
        }

        if (E_OBS_TARGET_TYPE_VENC == eTarget) {
            if (nChn >= MAX_RTSP_CHANNEL_NUM || nullptr == pData) {
                LOG_M_E(RTSP_OBS, "Invalid NALU data(chn=%d, pData=0x%08X).", nChn, pData);
                return AX_FALSE;
            }

            AX_VENC_PACK_T* pVencPack = &((AX_VENC_STREAM_T*)pData)->stPack;

            if (PT_H264 == pVencPack->enType ||  PT_H265 == pVencPack->enType) {
                if (nullptr == pVencPack->pu8Addr || 0 == pVencPack->u32Len) {
                    LOG_M_E(RTSP_OBS, "Invalid NALU data(chn=%d, buff=0x%08X, len=%d).", nChn, pVencPack->pu8Addr, pVencPack->u32Len);
                    return AX_FALSE;
                }

                AX_BOOL bIFrame = (AX_VENC_INTRA_FRAME == pVencPack->enCodingType) ? AX_TRUE : AX_FALSE;
                return m_pSink->SendNalu(m_nChannel, pVencPack->pu8Addr, pVencPack->u32Len, pVencPack->u64PTS, bIFrame);
            }
        }
        else if (E_OBS_TARGET_TYPE_AENC == eTarget) {
            AX_U32 nBitRate = ((AENC_STREAM_T*)pData)->nBitRate;
            AX_PAYLOAD_TYPE_E eType = ((AENC_STREAM_T*)pData)->eType;
            AX_AUDIO_STREAM_T* pAencPack = &((AENC_STREAM_T*)pData)->stPack;
            if (nullptr == pAencPack->pStream || 0 == pAencPack->u32Len) {
                LOG_M_E(RTSP_OBS, "Invalid Aenc data(chn=%d, buff=0x%08X, len=%d, type=%d, bitrate=%d).", nChn, pAencPack->pStream, pAencPack->u32Len, eType, nBitRate);
                return AX_FALSE;
            }
            return m_pSink->SendAudio(m_nChannel, pAencPack->pStream, pAencPack->u32Len, pAencPack->u64TimeStamp);
        }

        return AX_FALSE;
    }

    AX_BOOL OnRegisterObserver(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, OBS_TRANS_ATTR_PTR pParams) override {
        if (nullptr == pParams || (E_OBS_TARGET_TYPE_VENC != eTarget && E_OBS_TARGET_TYPE_AENC != eTarget)) {
            return AX_FALSE;
        }

        if (E_OBS_TARGET_TYPE_AENC == eTarget) {
            AX_APP_AUDIO_CHAN_E nAudioRtspChn = APP_AUDIO_RTSP_CHANNEL();
            AX_APP_AUDIO_ENCODER_ATTR_T stAttr;
            if (0 == AX_APP_Audio_GetEncoderAttr(nAudioRtspChn, &stAttr)) {
                m_stSessAttr.stAudioAttr.bEnable = AX_TRUE;
                m_stSessAttr.stAudioAttr.ePt = (AX_PAYLOAD_TYPE_E)stAttr.eType;
                m_stSessAttr.stAudioAttr.nSampleRate = (AX_U32)stAttr.eSampleRate;
                m_stSessAttr.stAudioAttr.nBitRate = (AX_U32)(stAttr.nBitRate / 1024); // kpbs
                m_stSessAttr.stAudioAttr.nChnCnt = (AX_APP_AUDIO_SOUND_MODE_MONO == stAttr.eSoundMode) ? 1 : 2;
                m_stSessAttr.stAudioAttr.nAOT = (AX_S32)stAttr.nAOT;
                m_stSessAttr.stAudioAttr.nMaxFrmSize = COptionHelper::GetInstance()->GetAencOutFrmSize();
            }
            else {
                m_stSessAttr.stAudioAttr.bEnable = AX_FALSE;
            }
        }
        else if (E_OBS_TARGET_TYPE_VENC == eTarget) {
            m_nGroup = nGrp;
            m_nChannel = nChn;

            m_stSessAttr.nChannel = nChn;
            m_stSessAttr.stVideoAttr.bEnable = AX_TRUE;
            m_stSessAttr.stVideoAttr.ePt = (AX_PAYLOAD_TYPE_E)pParams->nPayloadType;
            m_stSessAttr.stVideoAttr.nMaxFrmSize = COptionHelper::GetInstance()->GetRTSPMaxFrmSize();
            m_stSessAttr.stVideoAttr.nBitRate = (AX_U32)pParams->nBitRate;

            return m_pSink->AddSessionAttr(pParams->nChannel, m_stSessAttr);
        }

        return AX_TRUE;
    }

private:
    CAXRtspServer* m_pSink{nullptr};
    AX_U32 m_nGroup{0};
    AX_U32 m_nChannel{0};
    RTSP_SESS_ATTR_T m_stSessAttr;
};
