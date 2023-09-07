/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "AudioPlay.hpp"
#include "AppLogApi.h"
#include "ElapsedTimer.hpp"
#include "GlobalDef.h"
#include "AplayObserver.hpp"

#define APLAY "APLAY"

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

CAudioPlay::CAudioPlay() {

}

CAudioPlay::~CAudioPlay() {

}

AX_BOOL CAudioPlay::Init(const AUDIO_PLAY_DEV_ATTR_PTR pstPlayDevAttr, AX_U32 nPipeNum, const ADEC_CONFIG_PTR pstPipeAttr) {
    AX_BOOL bRet = AX_FALSE;

    if (!pstPlayDevAttr
        || (0 == nPipeNum)
        || !pstPipeAttr) {
        LOG_MM_E(APLAY, "Invalid input(dev:%p, pipe:%p, pipe num:%d)", pstPlayDevAttr, pstPipeAttr, nPipeNum);
        return bRet;
    }

    m_bLink = pstPlayDevAttr->bLink;
    m_nCardId = pstPlayDevAttr->nCardId;
    m_nDeviceId = pstPlayDevAttr->nDeviceId;
    m_nPipeNum = nPipeNum;

    // STEP1: set link
    if (nPipeNum > 1
        || pstPipeAttr[0].bLink != m_bLink) {
        m_bLink = AX_FALSE;
    }

    if (m_bLink) {
        for (AX_U32 i = 0; i < m_nPipeNum; i ++) {
            AX_MOD_INFO_T stAdecMod = {AX_ID_ADEC, 0, (AX_S32)pstPipeAttr[i].nChannel};
            AX_MOD_INFO_T stAoMod = {AX_ID_AO, (AX_S32)m_nCardId, (AX_S32)m_nDeviceId};
            m_nPipeList[i] = (AX_S32)pstPipeAttr[i].nChannel;

            AX_S32 nRet = AX_SYS_Link(&stAdecMod, &stAoMod);
            if (0 != nRet) {
                LOG_MM_E(APLAY, "AX_SYS_Link ADEC[%d]<->AO[%d][%d] failed ret=%08X", i, m_nCardId, m_nDeviceId, nRet);
                return bRet;
            }
        }
    }

    // STEP2: construct audio play device
    {
        AUDIO_PLAY_DEV_ATTR_T stPlayDevAttr = pstPlayDevAttr[0];
        stPlayDevAttr.bLink = m_bLink;

        m_pAudioDev = new CAudioPlayDev(stPlayDevAttr);

        if (!m_pAudioDev) {
            return bRet;
        }

        bRet = m_pAudioDev->Init();

        if (!bRet) {
            goto EXIT;
        }
    }

    // STEP3: construct audio play pipe
    {
        for (AX_U32 i = 0; i < m_nPipeNum; i ++) {
            ADEC_CONFIG_T stPipeAttr = pstPipeAttr[i];
            stPipeAttr.bLink = m_bLink;

            CAudioDecoder *pAudioDecoder = new CAudioDecoder(stPipeAttr.nChannel, stPipeAttr);

            if (!pAudioDecoder) {
                goto EXIT;
            }

            bRet = pAudioDecoder->Init();

            if (!bRet) {
                goto EXIT;
            }

            // register observer
            if (m_pAudioDev) {
                m_vecAPlayObs.emplace_back(CObserverMaker::CreateObserver<CAPlayObserver>(m_pAudioDev));
                pAudioDecoder->RegObserver(m_vecAPlayObs[m_vecAPlayObs.size() - 1].get());
            }

            m_vecAudioDecoderInst.emplace_back(pAudioDecoder);

            // initial audio file play
            CAudioFilePlay *pAudioFilePlay = new CAudioFilePlay(m_pAudioDev, pAudioDecoder, i);

            if (!pAudioFilePlay) {
                goto EXIT;
            }

            bRet = pAudioFilePlay->Init();

            if (!bRet) {
                goto EXIT;
            }

            m_vecAudioFilePlayInst.emplace_back(pAudioFilePlay);
        }
    }

EXIT:
    if (!bRet) {
        DeInit();
    }

    return AX_TRUE;
}

AX_BOOL CAudioPlay::DeInit() {
    // STEP 1: deinit audio file play
    for (auto& pInstance : m_vecAudioFilePlayInst) {
        pInstance->DeInit();
        delete pInstance;
    }
    m_vecAudioFilePlayInst.clear();

    // STEP 1: deinit adec
    for (auto& pInstance : m_vecAudioDecoderInst) {
        pInstance->DeInit();
        delete pInstance;
    }
    m_vecAudioDecoderInst.clear();

    // STEP 2: deinit dev
    if (m_pAudioDev) {
        m_pAudioDev->DeInit();
        delete m_pAudioDev;
        m_pAudioDev = nullptr;
    }

    if (m_bLink) {
        for (AX_U32 i = 0; i < m_nPipeNum; i ++) {
            AX_MOD_INFO_T stAdecMod = {AX_ID_ADEC, 0, (AX_S32)m_nPipeList[i]};
            AX_MOD_INFO_T stAoMod = {AX_ID_AO, (AX_S32)m_nCardId, (AX_S32)m_nDeviceId};

            AX_S32 nRet = AX_SYS_UnLink(&stAdecMod, &stAoMod);
            if (0 != nRet) {
                LOG_MM_E(APLAY, "AX_SYS_UnLink ADEC[%d]<->AO[%d][%d] failed ret=%08X", i, m_nCardId, m_nDeviceId, nRet);
            }
        }
    }

    return AX_TRUE;
}

AX_BOOL CAudioPlay::Start() {
    AX_BOOL bRet = AX_FALSE;

    // STEP1: start audio play device
    {
        if (m_pAudioDev) {
            bRet = m_pAudioDev->Start();

            if (!bRet) {
                goto EXIT;
            }
        }
    }

    // STEP2: start audio play pipe
    {
        for (auto& pInstance : m_vecAudioDecoderInst) {
            bRet = pInstance->Start();

            if (!bRet) {
                goto EXIT;
            }
        }
    }

#if 0 // will start audio file play when need
    // STEP3: start audio file play
    {
        for (auto& pInstance : m_vecAudioFilePlayInst) {
            bRet = pInstance->Start();

            if (!bRet) {
                goto EXIT;
            }
        }
    }
#endif

EXIT:
    if (!bRet) {
        Stop();
    }

    return bRet;
}

AX_BOOL CAudioPlay::Stop() {
    // STEP 1: stop audio play
    for (auto& pInstance : m_vecAudioFilePlayInst) {
        pInstance->Stop();
    }

    // STEP 2: stop adec
    for (auto& pInstance : m_vecAudioDecoderInst) {
        pInstance->Stop();
    }

    // STEP 3: deinit dev
    if (m_pAudioDev) {
        m_pAudioDev->Stop();
    }

    return AX_TRUE;
}

AX_BOOL CAudioPlay::Play(AX_PAYLOAD_TYPE_E eType,
                            const AX_U8 *pData,
                            AX_U32 nDataSize,
                            AX_U64 u64SeqNum,
                            AX_U32 nChannel/* = 0*/) {
    AX_BOOL bRet = AX_FALSE;

    if (!IsAudioSupport(eType)) {
        LOG_MM_E(APLAY, "unsupport audio type(%d)", eType);
        return bRet;
    }

    CAudioDecoder *pAudioDecoder = GetAudioDecoder(nChannel);

    if (!pAudioDecoder) {
        LOG_MM_E(APLAY, "invalid channel: %d", nChannel);
        return bRet;
    }

    {
        ADEC_CONFIG_T stAttr;
        pAudioDecoder->GetAttr(stAttr);

        // check type
        if (eType != stAttr.eType) {
            LOG_MM_E(APLAY, "dismatch payload type(init(%d), play(%d))", stAttr.eType, eType);
            return AX_FALSE;
        }

        // resample
        if (m_pAudioDev) {
            m_pAudioDev->SetResample(stAttr.eSampleRate);
        }
    }

    // play
    {
        AX_AUDIO_STREAM_T stStream;
        stStream.pStream = (AX_U8 *)pData;
        stStream.u64PhyAddr = 0;
        stStream.u32Len = nDataSize;
        stStream.u32Seq = (AX_U32)u64SeqNum;
        stStream.bEof = AX_FALSE;

        bRet = pAudioDecoder->Play(eType, &stStream);
    }

    return bRet;
}

AX_BOOL CAudioPlay::PlayFile(AX_PAYLOAD_TYPE_E eType,
                                const std::string &strFileName,
                                AX_S32 nLoop,
                                AUDIO_FILE_PLAY_CALLBACK callback,
                                AX_VOID *pUserData,
                                AX_U32 nChannel/* = AUDIO_FILE_PLAY_DEFAULT_CHANNEL*/) {
    CAudioFilePlay *pAudioFilePlay = GetAudioFilePlay(nChannel);

    if (!pAudioFilePlay) {
        LOG_MM_E(APLAY, "invalid channel: %d", nChannel);
        return AX_FALSE;
    }

    // start audio file play when need
    if (!pAudioFilePlay->Start()) {
        return AX_FALSE;
    }

    return pAudioFilePlay->Play(eType,
                                strFileName,
                                nLoop,
                                callback,
                                pUserData);
}

AX_BOOL CAudioPlay::StopPlayFile(AX_U32 nChannel/* = AUDIO_FILE_PLAY_DEFAULT_CHANNEL*/) {
    CAudioFilePlay *pAudioFilePlay = GetAudioFilePlay(nChannel);

    if (!pAudioFilePlay) {
        LOG_MM_E(APLAY, "invalid channel: %d", nChannel);
        return AX_FALSE;
    }

    return pAudioFilePlay->StopPlay();
}

AX_BOOL CAudioPlay::GetVolume(AX_F64 &fVolume) {
    if (m_pAudioDev) {
        AX_BOOL bRet = m_pAudioDev->GetVolume(fVolume);

        if (fVolume > AUDIO_PLAY_VOLUME_MAX) {
            fVolume = AUDIO_PLAY_VOLUME_MAX;
        }

        return bRet;
    }

    return AX_FALSE;
}

AX_BOOL CAudioPlay::SetVolume(const AX_F64 &fVolume) {
    AX_F64 fVolumeSet = fVolume;
    if (fVolumeSet > AUDIO_PLAY_VOLUME_MAX) {
        fVolumeSet = AUDIO_PLAY_VOLUME_MAX;
    }

    if (m_pAudioDev) {
        return m_pAudioDev->SetVolume(fVolumeSet);
    }

    return AX_FALSE;
}

AX_BOOL CAudioPlay::GetDevAttr(AUDIO_PLAY_DEV_ATTR_T &stAttr) {
    if (m_pAudioDev) {
        return m_pAudioDev->GetAttr(stAttr);
    }

    return AX_FALSE;
}

AX_BOOL CAudioPlay::SetDevAttr(const AUDIO_PLAY_DEV_ATTR_T &stAttr) {
    if (m_pAudioDev) {
        return m_pAudioDev->SetAttr(stAttr);
    }

    return AX_FALSE;
}

AX_BOOL CAudioPlay::GetAdecAttr(AX_U32 nChannel, ADEC_CONFIG_T &stAttr) {
    CAudioDecoder *pAudioDecoder = GetAudioDecoder(nChannel);

    if (!pAudioDecoder) {
        LOG_MM_E(APLAY, "invalid channel: %d", nChannel);
        return AX_FALSE;
    }

    return pAudioDecoder->GetAttr(stAttr);
}

AX_BOOL CAudioPlay::SetAdecAttr(AX_U32 nChannel, const ADEC_CONFIG_T &stAttr) {
    CAudioDecoder *pAudioDecoder = GetAudioDecoder(nChannel);

    if (!pAudioDecoder) {
        LOG_MM_E(APLAY, "invalid channel: %d", nChannel);
        return AX_FALSE;
    }

    return pAudioDecoder->SetAttr(stAttr);
}

CAudioDecoder* CAudioPlay::GetAudioDecoder(AX_U32 nChannel) {
    for (auto pInst : m_vecAudioDecoderInst) {
        if (nChannel == pInst->GetChannel()) {
            return pInst;
        }
    }

    return nullptr;
}

CAudioFilePlay* CAudioPlay::GetAudioFilePlay(AX_U32 nChannel) {
    for (AX_U32 i = 0; i < m_vecAudioDecoderInst.size(); i ++) {
        if (nChannel == m_vecAudioDecoderInst[i]->GetChannel()) {
            return m_vecAudioFilePlayInst[i];
        }
    }

    return nullptr;
}
