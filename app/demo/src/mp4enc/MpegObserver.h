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
#include "Mpeg4Encoder.h"

#define MPEG4_OBS "MPEG4_OBS"

class CMPEG4Observer : public IObserver {
public:
    // static std::unique_ptr<IObserver> NewInstance(CMPEG4Encoder* pSink) {
    //     return std::unique_ptr<IObserver>(new CMPEG4Observer(pSink));
    // }

    CMPEG4Observer(CMPEG4Encoder* pSink) : m_pSink(pSink){};
    virtual ~CMPEG4Observer(AX_VOID) = default;
public:
    AX_BOOL IsSame(AX_U32 nGrp, AX_U32 nChn) const {
        return (m_nGroup == nGrp && m_nChannel == nChn) ? AX_TRUE : AX_FALSE;
    }
public:
    AX_BOOL OnRecvData(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChannel, AX_VOID* pData) override {
        if (!m_pSink) {
            return AX_TRUE;
        }

        if (E_OBS_TARGET_TYPE_VENC == eTarget) {
            if (nullptr == pData) {
                LOG_M_E(MPEG4_OBS, "chn[%d] pData nullptr", nChannel);
                return AX_FALSE;
            }

            AX_VENC_PACK_T* pVencPack = &((AX_VENC_STREAM_T*)pData)->stPack;
            if (PT_H264 == pVencPack->enType ||  PT_H265 == pVencPack->enType) {
                if (nullptr == pVencPack->pu8Addr || 0 == pVencPack->u32Len) {
                LOG_M_E(MPEG4_OBS, "Invalid NALU data(chn=%d, buff=0x%08X, len=%d).", nChannel, pVencPack->pu8Addr, pVencPack->u32Len);
                return AX_FALSE;
                }

                AX_BOOL bIFrame = (AX_VENC_INTRA_FRAME == pVencPack->enCodingType) ? AX_TRUE : AX_FALSE;
                m_pSink->SendRawFrame(nChannel, pVencPack->pu8Addr, pVencPack->u32Len, pVencPack->u64PTS, bIFrame);
            }

        }
        else if (E_OBS_TARGET_TYPE_AENC == eTarget) {
            AX_U32 nBitRate = ((AENC_STREAM_T*)pData)->nBitRate;
            AX_PAYLOAD_TYPE_E eType = ((AENC_STREAM_T*)pData)->eType;
            AX_AUDIO_STREAM_T* pAencPack = &((AENC_STREAM_T*)pData)->stPack;
            if (nullptr == pAencPack->pStream || 0 == pAencPack->u32Len) {
                LOG_M_E(MPEG4_OBS, "Invalid Aenc data(chn=%d, buff=0x%08X, len=%d, type=%d, bitrate=%d).", nChannel, pAencPack->pStream, pAencPack->u32Len, eType, nBitRate);
                return AX_FALSE;
            }
            return m_pSink->SendAudioFrame(m_nChannel, pAencPack->pStream, pAencPack->u32Len, pAencPack->u64TimeStamp);
        }
        return AX_TRUE;
    }
    AX_BOOL OnRegisterObserver(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChannel, OBS_TRANS_ATTR_PTR pParams) override {
        if (E_OBS_TARGET_TYPE_AENC == eTarget) {
            // nothing to do
        }
        else if (E_OBS_TARGET_TYPE_VENC == eTarget) {
            m_nGroup = nGrp;
            m_nChannel = nChannel;
        }
        return AX_TRUE;
    }

private:
    CMPEG4Encoder* m_pSink{nullptr};
    AX_U32 m_nGroup{0};
    AX_U32 m_nChannel{0};
};
