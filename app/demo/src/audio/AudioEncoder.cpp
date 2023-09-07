/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "AudioEncoder.hpp"
#include "AppLogApi.h"
#include "ElapsedTimer.hpp"
#include "GlobalDef.h"

#ifdef SLT
#include "PrintHelper.h"
#endif

#define AENC "AENC"

using namespace std;

namespace {
static AX_BOOL IsAudioSupport(AX_PAYLOAD_TYPE_E eType) {
    AX_BOOL bRet = (AX_BOOL)((PT_G711A == eType)
                            || (PT_G711U == eType)
                            || (PT_LPCM == eType)
                            || (PT_G726 == eType)
                            //|| (PT_OPUS == eType)
                            || (PT_AAC == eType));

    return bRet;
}
}

CAudioEncoder::CAudioEncoder(AX_U32 nChannel, const AENC_CONFIG_T& stConfig)
: CAXStage((string)AENC + (char)('0' + nChannel))
, m_nChannel(nChannel)
, m_stAencConfig(stConfig) {
}

CAudioEncoder::~CAudioEncoder() {
}

AX_BOOL CAudioEncoder::Init() {
    // parameter check
    if (!IsAudioSupport(m_stAencConfig.eType)) {
        LOG_MM_E(AENC, "unsupport audio type(%d)", m_stAencConfig.eType);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CAudioEncoder::DeInit() {
    if (m_bStart) {
        Stop();
    }

    return AX_TRUE;
}

AX_BOOL CAudioEncoder::Create() {
    AX_S32 nRet = 0;

    {
        AX_AENC_CHN_ATTR_T stChnAttr;

        // aac encoder
        AX_AENC_AAC_ENCODER_ATTR_T stAacEncoderAttr;
        // G726 encoder
        AX_AENC_G726_ENCODER_ATTR_T stG726EncoderAttr;
        // Opus encoder
        AX_AENC_OPUS_ENCODER_ATTR_T stOpusEncoderAttr;

        memset(&stChnAttr, 0x00, sizeof(stChnAttr));
        stChnAttr.enType = m_stAencConfig.eType;
        stChnAttr.u32BufSize = (0 == m_stAencConfig.nOutDepth) ? AENC_DEFAULT_OUT_DEPTH : m_stAencConfig.nOutDepth;
        stChnAttr.pValue = nullptr;
        stChnAttr.enLinkMode = m_stAencConfig.bLink ? AX_LINK_MODE : AX_UNLINK_MODE;

        if (PT_AAC == m_stAencConfig.eType) {
            stAacEncoderAttr = {
                    .enAacType = m_stAencConfig.stEncoderAttr.stAacEncoder.eAacType,
                    .enTransType = m_stAencConfig.stEncoderAttr.stAacEncoder.eTransType,
                    .enChnMode = (1 == m_stAencConfig.nChnCnt) ? AX_AAC_CHANNEL_MODE_1 : AX_AAC_CHANNEL_MODE_2,
                    .u32GranuleLength = (AX_U32)((AX_AAC_TYPE_AAC_LC == m_stAencConfig.stEncoderAttr.stAacEncoder.eAacType) ? 1024 : 480),
                    .u32SampleRate = m_stAencConfig.eSampleRate,
                    .u32BitRate = m_stAencConfig.nBitRate
                };

            if (AX_AAC_TYPE_AAC_LC == m_stAencConfig.stEncoderAttr.stAacEncoder.eAacType) {
                stChnAttr.u32PtNumPerFrm = AENC_AAC_LC_PTNUM_PER_FRM;
            }
            else {
                stChnAttr.u32PtNumPerFrm = AENC_AAC_PTNUM_PER_FRM;
            }
            stChnAttr.pValue = &stAacEncoderAttr;
        }
        else if (PT_G726 == m_stAencConfig.eType) {
            stG726EncoderAttr = {
                    .u32BitRate = m_stAencConfig.nBitRate //32000
                };

            stChnAttr.u32PtNumPerFrm = AENC_G726_PTNUM_PER_FRM;
            stChnAttr.pValue = &stG726EncoderAttr;
        }
        else if (PT_OPUS == m_stAencConfig.eType) {
            stOpusEncoderAttr = {
                    .enApplication = AX_OPUS_APPLICATION_RESTRICTED_LOWDELAY,
                    .u32SamplingRate = m_stAencConfig.eSampleRate,
                    .s32Channels = (AX_S32)m_stAencConfig.nChnCnt,
                    .s32BitrateBps = 32000,
                    .f32FramesizeInMs = 10.0
                };

            stChnAttr.u32PtNumPerFrm = (AX_U32)((AX_F32)m_stAencConfig.eSampleRate * (stOpusEncoderAttr.f32FramesizeInMs / 1000.0));
            stChnAttr.pValue = &stOpusEncoderAttr;
        }
        else {
            stChnAttr.u32PtNumPerFrm = AENC_DEF_PTNUM_PER_FRM;
            stChnAttr.pValue = NULL;
        }

        // create aenc channel
        nRet = AX_AENC_CreateChn(m_nChannel, &stChnAttr);
        if (0 != nRet) {
            LOG_MM_E(AENC, "AX_AENC_CreateChn[%d] failed ret=%08X", m_nChannel, nRet);
            return AX_FALSE;
        }

        if (PT_AAC == m_stAencConfig.eType) {
            m_nConfigBufSize = AX_MIN(sizeof(stAacEncoderAttr.u8ConfBuf), AENC_CONFIG_BUF_SIZE);
            memcpy(m_arrConfigBuf, stAacEncoderAttr.u8ConfBuf, m_nConfigBufSize);
        }
    }

    return AX_TRUE;
}

AX_BOOL CAudioEncoder::Destroy() {
    AX_S32 nRet = AX_AENC_DestroyChn(m_nChannel);

    if (0 != nRet) {
        LOG_MM_E(AENC, "AX_AENC_DestroyChn[%d] failed ret=%08X", m_nChannel, nRet);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CAudioEncoder::Start(STAGE_START_PARAM_PTR pStartParams) {
    if (!m_bStart) {
        if (!Create()) {
            return AX_FALSE;
        }

        StartWorkThread();

        m_bStart = AX_TRUE;
    }

    return CAXStage::Start(pStartParams);
}

AX_BOOL CAudioEncoder::Stop() {
    if (m_bStart) {
        StopWorkThread();

        CAXStage::Stop();

        Destroy();

        m_bStart = AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CAudioEncoder::Restart(const AENC_CONFIG_T &stAttr) {
    Stop();

    m_stAencConfig = stAttr;

    STAGE_START_PARAM_T stStartParam = {.bStartProcessingThread = AX_TRUE};

    return Start(&stStartParam);
}

AX_VOID CAudioEncoder::StartWorkThread() {
    if (!m_bGetThreadRunning) {
        m_bGetThreadRunning = AX_TRUE;
        m_hGetThread = std::thread(&CAudioEncoder::FrameGetThreadFunc, this, this);
    }
}

AX_VOID CAudioEncoder::StopWorkThread() {
    m_bGetThreadRunning = AX_FALSE;

    if (m_hGetThread.joinable()) {
        m_hGetThread.join();
    }
}

AX_VOID CAudioEncoder::FrameGetThreadFunc(CAudioEncoder* pCaller) {
    LOG_MM_I(AENC, "[%d] +++", GetChannel());

    CAudioEncoder* pThis = (CAudioEncoder*)pCaller;

    AX_U32 nChannel = pThis->GetChannel();

    AX_CHAR szName[50] = {0};
    sprintf(szName, "AENC_Get_%d", nChannel);
    prctl(PR_SET_NAME, szName);

    AX_S32 nRet = AX_SUCCESS;
    AENC_STREAM_T stStream;

    while (pThis->m_bGetThreadRunning) {
        nRet = AX_AENC_GetStream(nChannel, &stStream.stPack, -1);

        if (0 != nRet) {
            if (AX_ERR_AENC_BUF_EMPTY == nRet) {
                continue;
            }
            LOG_MM_E(AENC, "AX_AENC_GetStream[%d] failed, ret=%08X", nChannel, nRet);
            break;
        }

        if (0 == stStream.stPack.u32Len) {
            AX_AENC_ReleaseStream(nChannel, &stStream.stPack);
            continue;
        }

        const AENC_CONFIG_PTR pstConfig = GetChnCfg();
        stStream.nBitRate = pstConfig->nBitRate;
        stStream.eType = pstConfig->eType;
        stStream.eBitWidth = pstConfig->eBitWidth;
        stStream.eSampleRate = pstConfig->eSampleRate;
        stStream.eSoundMode = pstConfig->eSoundMode;
        NotifyAll(nChannel, &stStream);

        AX_AENC_ReleaseStream(nChannel, &stStream.stPack);
    }

    LOG_MM_I(AENC, "[%d] ---", GetChannel());
}

AX_BOOL CAudioEncoder::GetAttr(AENC_CONFIG_T &stAttr) {
    stAttr = m_stAencConfig;

    return AX_TRUE;
}

AX_BOOL CAudioEncoder::SetAttr(const AENC_CONFIG_T &stAttr) {
    // TODO
    if (m_bStart) {
        if (stAttr.bLink != m_stAencConfig.bLink
            || stAttr.nChannel != m_stAencConfig.nChannel
            || stAttr.nInDepth != m_stAencConfig.nInDepth
            || stAttr.nOutDepth != m_stAencConfig.nOutDepth
            || stAttr.nChnCnt != m_stAencConfig.nChnCnt
            || stAttr.nBitRate != m_stAencConfig.nBitRate
            || stAttr.eType != m_stAencConfig.eType
            || stAttr.eBitWidth != m_stAencConfig.eBitWidth
            || stAttr.eSampleRate != m_stAencConfig.eSampleRate
            || stAttr.eSoundMode != m_stAencConfig.eSoundMode) {
            return Restart(stAttr);
        }
        else {
            if (PT_AAC == stAttr.eType
                && (stAttr.stEncoderAttr.stAacEncoder.eAacType != m_stAencConfig.stEncoderAttr.stAacEncoder.eAacType
                    || stAttr.stEncoderAttr.stAacEncoder.eTransType != m_stAencConfig.stEncoderAttr.stAacEncoder.eTransType)) {
                return Restart(stAttr);
            }
        }
    }

    m_stAencConfig = stAttr;

    return AX_TRUE;
}

AX_BOOL CAudioEncoder::GetAacEncoderConfigBuf(const AX_U8 **ppConfBuf, AX_U32 *pDataSize) {
    if (!ppConfBuf || !pDataSize) {
        LOG_MM_E(AENC, "[%d] Invalid config buffer(%p, %p)", ppConfBuf, pDataSize);
        return AX_FALSE;
    }

    *ppConfBuf = (const AX_U8 *)m_arrConfigBuf;
    *pDataSize = m_nConfigBufSize;

    return AX_TRUE;
}

AX_VOID CAudioEncoder::RegObserver(IObserver* pObserver) {
    if (nullptr != pObserver) {
        OBS_TRANS_ATTR_T tTransAttr;
        tTransAttr.nChannel = m_nChannel;

        tTransAttr.nPayloadType = m_stAencConfig.eType;

        if (pObserver->OnRegisterObserver(E_OBS_TARGET_TYPE_AENC, 0, m_nChannel, &tTransAttr)) {
            m_vecObserver.emplace_back(pObserver);
        }
    }
}

AX_VOID CAudioEncoder::UnregObserver(IObserver* pObserver) {
    if (nullptr == pObserver) {
        return;
    }

    for (vector<IObserver*>::iterator it = m_vecObserver.begin(); it != m_vecObserver.end(); it++) {
        if (*it == pObserver) {
            m_vecObserver.erase(it);
            break;
        }
    }
}

AX_VOID CAudioEncoder::NotifyAll(AX_U32 nChannel, AX_VOID* pstStream) {
    if (nullptr == pstStream) {
        return;
    }

    for (vector<IObserver*>::iterator it = m_vecObserver.begin(); it != m_vecObserver.end(); it++) {
        (*it)->OnRecvData(E_OBS_TARGET_TYPE_AENC, 0, nChannel, pstStream);
    }
}

AX_BOOL CAudioEncoder::ProcessFrame(CAXFrame* pFrame) {
    AX_S32 nRet = 0;

    nRet = AX_AENC_SendFrame(m_nChannel, &pFrame->stFrame.stAFrame.stAFrame);

    if (0 != nRet) {
        LOG_M_E(AENC, "[%d] AX_AENC_SendFrame failed, ret=0x%x", m_nChannel, nRet);
        return AX_FALSE;
    }
    else {
        LOG_M_D(AENC, "[%d] AX_AENC_SendFrame, seq=%ld", m_nChannel, pFrame->stFrame.stAFrame.stAFrame.u32Seq);
    }

    return AX_TRUE;
}
