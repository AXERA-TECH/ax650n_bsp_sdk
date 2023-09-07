/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include <string.h>
#include <sys/prctl.h>
#include "AudioDecoder.hpp"
#include "AppLogApi.h"
#include "ElapsedTimer.hpp"
#include "GlobalDef.h"

#ifdef SLT
#include "PrintHelper.h"
#endif

#define ADEC "ADEC"

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

CAudioDecoder::CAudioDecoder(AX_U32 nChannel, const ADEC_CONFIG_T& tConfig)
: m_nChannel(nChannel)
, m_stAdecConfig(tConfig) {
}

CAudioDecoder::~CAudioDecoder() {
}

AX_BOOL CAudioDecoder::Init() {
    // STEP 1: parameter check
    if (!IsAudioSupport(m_stAdecConfig.eType)) {
        LOG_MM_E(ADEC, "unsupport audio type(%d)", m_stAdecConfig.eType);
        return AX_FALSE;
    }

    // STEP 2: create pool
    if (AX_INVALID_POOLID == m_nAPlayPipePoolId) {
        AX_POOL_CONFIG_T stPoolConfig;
        stPoolConfig.MetaSize = ADEC_META_SIZE;
        stPoolConfig.BlkSize = (0 == m_stAdecConfig.nBlkSize) ? ADEC_DEFAULT_BLK_SIZE : m_stAdecConfig.nBlkSize;
        stPoolConfig.BlkCnt = (0 == m_stAdecConfig.nBlkCnt) ? ADEC_DEFAULT_BLK_CNT : m_stAdecConfig.nBlkCnt;
        stPoolConfig.IsMergeMode = AX_FALSE;
        stPoolConfig.CacheMode = POOL_CACHE_MODE_NONCACHE;
        strcpy((char *)stPoolConfig.PartitionName, "anonymous");
        snprintf((char *)stPoolConfig.PoolName, AX_MAX_POOL_NAME_LEN, "AUDIO_PLAY_%d", m_stAdecConfig.nChannel);

        m_nAPlayPipePoolId = AX_POOL_CreatePool(&stPoolConfig);

        if (AX_INVALID_POOLID == m_nAPlayPipePoolId) {
            LOG_MM_E(ADEC, "AX_POOL_CreatePool failed[size: %d, cnt: %d]", stPoolConfig.BlkSize, stPoolConfig.BlkCnt);
            return AX_FALSE;
        }
    }

    return AX_TRUE;
}

AX_BOOL CAudioDecoder::DeInit() {
    if (m_bStart) {
        Stop();
    }

    if (AX_INVALID_POOLID != m_nAPlayPipePoolId) {
        AX_POOL_DestroyPool(m_nAPlayPipePoolId);
        m_nAPlayPipePoolId = AX_INVALID_POOLID;
    }

    return AX_TRUE;
}

AX_BOOL CAudioDecoder::Start() {
    if (!m_bStart) {
        if (!Create()) {
            return AX_FALSE;
        }

        StartWorkThread();

        m_bStart = AX_TRUE;
    }

    return AX_TRUE;
}

AX_BOOL CAudioDecoder::Stop() {
    if (m_bStart) {
        StopWorkThread();

        Destroy();

        m_bStart = AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CAudioDecoder::Restart(const ADEC_CONFIG_T &stAttr) {
    // TODO:
    if (stAttr.nBlkSize != m_stAdecConfig.nBlkSize
        || stAttr.nBlkCnt != m_stAdecConfig.nBlkCnt) {
        Stop();

        DeInit();

        m_stAdecConfig = stAttr;

        Init();
    }
    else {
        Stop();

        m_stAdecConfig = stAttr;
    }

    return Start();
}

AX_BOOL CAudioDecoder::Create() {
    AX_S32 nRet = 0;

    // STEP 1: create adec channel
    {
        AX_ADEC_CHN_ATTR_T stChnAttr;

        // aac decoder
        AX_ADEC_AAC_DECODER_ATTR_T stAacDecoderAttr;
        // G726 decoder
        AX_ADEC_G726_DECODER_ATTR_T stG726DecoderAttr;
        // Opus decoder
        AX_ADEC_OPUS_DECODER_ATTR_T stOpusDecoderAttr;

        memset(&stChnAttr, 0x00, sizeof(stChnAttr));
        stChnAttr.enType = m_stAdecConfig.eType;
        stChnAttr.u32BufSize = (0 == m_stAdecConfig.nInDepth) ? ADEC_DEFAULT_IN_DEPTH : m_stAdecConfig.nInDepth;
        stChnAttr.enMode = AX_ADEC_MODE_PACK;
        stChnAttr.pValue = nullptr;
        stChnAttr.enLinkMode = m_stAdecConfig.bLink ? AX_LINK_MODE : AX_UNLINK_MODE;

        if (PT_AAC == m_stAdecConfig.eType) {
            AX_U8 *pConf[] = { m_stAdecConfig.stDecoderAttr.stAacDecoder.ConfigBuf };
            stAacDecoderAttr = {
                    .enTransType = m_stAdecConfig.stDecoderAttr.stAacDecoder.eTransType,
                    .u8Conf = pConf,
                    .u32ConfLen = m_stAdecConfig.stDecoderAttr.stAacDecoder.nConfLen
                };

            stChnAttr.pValue = &stAacDecoderAttr;
        }
        else if (PT_G726 == m_stAdecConfig.eType) {
            stG726DecoderAttr = {
                    .u32BitRate = m_stAdecConfig.stDecoderAttr.stDefDecoder.nBitRate
                };

            stChnAttr.pValue = &stG726DecoderAttr;
        }
        else if (PT_OPUS == m_stAdecConfig.eType) {
            stOpusDecoderAttr = {
                    .u32SamplingRate = m_stAdecConfig.eSampleRate,
                    .s32Channels = (AX_AUDIO_SOUND_MODE_MONO == m_stAdecConfig.eSoundMode) ? 1 : 2
                };

            stChnAttr.pValue = &stOpusDecoderAttr;
        }
        else {
            stChnAttr.pValue = nullptr;
        }

        nRet = AX_ADEC_CreateChn(m_nChannel, &stChnAttr);
        if (0 != nRet) {
            LOG_MM_E(ADEC, "AX_ADEC_CreateChn[%d] failed ret=%08X", m_nChannel, nRet);
            goto EXIT1;
        }
    }

    // STEP 2: attach pool
    nRet = AX_ADEC_AttachPool(m_nChannel, m_nAPlayPipePoolId);
    if (0 != nRet) {
        LOG_MM_E(ADEC, "AX_ADEC_AttachPool[%d] failed ret=%08X", m_nChannel, nRet);
        goto EXIT2;
    }

    return AX_TRUE;

EXIT2:
    AX_ADEC_DestroyChn(m_nChannel);

EXIT1:
    return AX_FALSE;
}

AX_BOOL CAudioDecoder::Destroy() {
    AX_S32 nRet = 0;

    // STEP 1: detach pool
    nRet = AX_ADEC_DetachPool(m_nChannel);
    if (0 != nRet) {
        LOG_MM_E(ADEC, "AX_ADEC_AttachPool[%d] failed ret=%08X", m_nChannel, nRet);
        goto EXIT;
    }

    // STEP 2: destroy channel
    nRet = AX_ADEC_DestroyChn(m_nChannel);
    if (0 != nRet) {
        LOG_MM_E(ADEC, "AX_ADEC_DestroyChn[%d] failed ret=%08X", m_nChannel, nRet);
        return AX_FALSE;
    }

    return AX_TRUE;

EXIT:
    return AX_FALSE;
}

AX_VOID CAudioDecoder::StartWorkThread() {
    if (!m_stAdecConfig.bLink
        && !m_bGetThreadRunning) {
        m_bGetThreadRunning = AX_TRUE;
        m_hGetThread = std::thread(&CAudioDecoder::FrameGetThreadFunc, this, this);
    }
}

AX_VOID CAudioDecoder::StopWorkThread() {
    m_bGetThreadRunning = AX_FALSE;

    // TODO: send one eof frame
    AX_AUDIO_STREAM_T stStream;
    memset(&stStream, 0x00, sizeof(stStream));
    stStream.bEof = AX_TRUE;
    AX_ADEC_SendStream(m_nChannel, &stStream, AX_TRUE);

    if (m_hGetThread.joinable()) {
        m_hGetThread.join();
    }
}

AX_VOID CAudioDecoder::FrameGetThreadFunc(CAudioDecoder* pCaller) {
    LOG_MM_I(ADEC, "[%d] +++", GetChannel());

    CAudioDecoder* pThis = (CAudioDecoder*)pCaller;

    AX_U32 nChannel = pThis->GetChannel();

    AX_CHAR szName[50] = {0};
    sprintf(szName, "ADEC_Get_%d", nChannel);
    prctl(PR_SET_NAME, szName);

    AX_S32 nRet = AX_SUCCESS;
    AX_AUDIO_FRAME_T stAFrame;

    while (pThis->m_bGetThreadRunning) {
        nRet = AX_ADEC_GetFrame(nChannel, &stAFrame, AX_TRUE);

        if (0 != nRet) {
            if (AX_ERR_ADEC_END_OF_STREAM == nRet) {
                pThis->m_bGetThreadRunning = AX_FALSE;
                break;
            }
            else if (AX_ERR_ADEC_BUF_EMPTY == nRet) {
                continue;
            }
            LOG_MM_E(ADEC, "AX_ADEC_GetFrame[%d] failed, ret=%08X", nChannel, nRet);
            break;
        }

        if (0 == stAFrame.u32Len) {
            AX_ADEC_ReleaseFrame(nChannel, &stAFrame);
            continue;
        }

        NotifyAll(nChannel, &stAFrame);

        AX_ADEC_ReleaseFrame(nChannel, &stAFrame);
    }

    LOG_MM_I(ADEC, "[%d] +++", GetChannel());
}

AX_BOOL CAudioDecoder::GetAttr(ADEC_CONFIG_T &stAttr) {
    stAttr = m_stAdecConfig;

    return AX_TRUE;
}

AX_BOOL CAudioDecoder::SetAttr(const ADEC_CONFIG_T &stAttr) {
    if (m_bStart) {
        if (stAttr.bLink != m_stAdecConfig.bLink
            || stAttr.nChannel != m_stAdecConfig.nChannel
            || stAttr.nBlkSize != m_stAdecConfig.nBlkSize
            || stAttr.nBlkCnt != m_stAdecConfig.nBlkCnt
            || stAttr.nInDepth != m_stAdecConfig.nInDepth
            || stAttr.nOutDepth != m_stAdecConfig.nOutDepth
            || stAttr.eType != m_stAdecConfig.eType
            || stAttr.eBitWidth != m_stAdecConfig.eBitWidth
            || stAttr.eSampleRate != m_stAdecConfig.eSampleRate
            || stAttr.eSoundMode != m_stAdecConfig.eSoundMode) {
            return Restart(stAttr);
        }
        else {
            if (PT_AAC == stAttr.eType
                && stAttr.stDecoderAttr.stAacDecoder.eTransType != m_stAdecConfig.stDecoderAttr.stAacDecoder.eTransType) {
                return Restart(stAttr);
            }
        }

        m_stAdecConfig = stAttr;

        return AX_TRUE;
    }

    if (stAttr.nBlkSize != m_stAdecConfig.nBlkSize
        || stAttr.nBlkCnt != m_stAdecConfig.nBlkCnt) {
        DeInit();

        m_stAdecConfig = stAttr;

        Init();
    }
    else {
        m_stAdecConfig = stAttr;
    }

    return AX_TRUE;
}

AX_BOOL CAudioDecoder::Play(AX_PAYLOAD_TYPE_E eType, const AX_AUDIO_STREAM_T *pstStream, AX_S32 nTimeout/* = -1 */) {
    if (!pstStream || !m_bStart) {
        return AX_FALSE;
    }

    if (m_stAdecConfig.eType != eType) {
        LOG_MM_E(ADEC, "[%d] dismatch payload type(init(%d), play(%d)", m_nChannel, m_stAdecConfig.eType, eType);
        return AX_FALSE;
    }

    AX_S32 nRet = 0;

    if (nTimeout < 0) {
        nRet = AX_ADEC_SendStream(m_nChannel, pstStream, AX_TRUE);
    }
    else {
        auto startTime = std::chrono::steady_clock::now();

        do {
            nRet = AX_ADEC_SendStream(m_nChannel, pstStream, AX_FALSE);

            if (0 == nRet) {
                break;
            }

            auto endTime = std::chrono::steady_clock::now();

            AX_S32 nElapsed = (AX_S32)(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());

            if (nElapsed >= nTimeout) {
                break;
            }

            CElapsedTimer::GetInstance()->mSleep(1);
        } while (1);
    }

    return AX_TRUE;
}

AX_VOID CAudioDecoder::RegObserver(IObserver* pObserver) {
    if (nullptr != pObserver) {
        OBS_TRANS_ATTR_T tTransAttr;
        tTransAttr.nChannel = m_nChannel;

        tTransAttr.nPayloadType = m_stAdecConfig.eType;

        if (pObserver->OnRegisterObserver(E_OBS_TARGET_TYPE_APLAY, 0, m_nChannel, &tTransAttr)) {
            m_vecObserver.emplace_back(pObserver);
        }
    }
}

AX_VOID CAudioDecoder::UnregObserver(IObserver* pObserver) {
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

AX_VOID CAudioDecoder::NotifyAll(AX_U32 nChannel, AX_VOID* pstFrame) {
    if (nullptr == pstFrame) {
        return;
    }

    for (vector<IObserver*>::iterator it = m_vecObserver.begin(); it != m_vecObserver.end(); it++) {
        (*it)->OnRecvData(E_OBS_TARGET_TYPE_APLAY, 0, nChannel, pstFrame);
    }
}
