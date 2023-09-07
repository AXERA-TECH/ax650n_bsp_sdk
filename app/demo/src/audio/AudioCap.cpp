/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "AudioCap.hpp"
#include "AppLogApi.h"
#include "ElapsedTimer.hpp"
#include "GlobalDef.h"
#include "AencObserver.hpp"

#define ACAP "ACAP"

using namespace std;

CAudioCap::CAudioCap() {

}

CAudioCap::~CAudioCap() {

}

AX_BOOL CAudioCap::Init(const AUDIO_CAP_DEV_ATTR_PTR pstCapDevAttr, AX_U32 nPipeNum, const AENC_CONFIG_PTR pstPipeAttr) {
    AX_BOOL bRet = AX_FALSE;

    if (!pstCapDevAttr
        || (0 == nPipeNum)
        || !pstPipeAttr) {
        LOG_MM_E(ACAP, "Invalid input(dev:%p, pipe:%p, pipe num:%d)", pstCapDevAttr, pstPipeAttr, nPipeNum);
        return bRet;
    }

    m_bLink = pstCapDevAttr->bLink;
    m_nCardId = pstCapDevAttr->nCardId;
    m_nDeviceId = pstCapDevAttr->nDeviceId;
    m_nPipeNum = nPipeNum;

    // STEP1: set link
    for (AX_U32 i = 0; i < m_nPipeNum; i ++) {
        if (pstPipeAttr[i].bLink != m_bLink) {
            m_bLink = AX_FALSE;
            break;
        }
    }

    if (m_bLink) {
        for (AX_U32 i = 0; i < m_nPipeNum; i ++) {
            AX_MOD_INFO_T stAiMod = {AX_ID_AI, (AX_S32)m_nCardId, (AX_S32)m_nDeviceId};
            AX_MOD_INFO_T stAencMod = {AX_ID_AENC, 0, (AX_S32)pstPipeAttr[i].nChannel};
            m_nPipeList[i] = (AX_S32)pstPipeAttr[i].nChannel;

            AX_S32 nRet = AX_SYS_Link(&stAiMod, &stAencMod);
            if (0 != nRet) {
                LOG_MM_E(ACAP, "AX_SYS_Link AI[%d][%d]<->AENC[%d] failed ret=%08X", m_nCardId, m_nDeviceId, i, nRet);
                return bRet;
            }
        }
    }

    // STEP2: construct audio capture device
    {
        AUDIO_CAP_DEV_ATTR_T stCapDevAttr = pstCapDevAttr[0];
        stCapDevAttr.bLink = m_bLink;

        m_pAudioDev = new CAudioCapDev(stCapDevAttr);

        if (!m_pAudioDev) {
            return bRet;
        }

        bRet = m_pAudioDev->Init();

        if (!bRet) {
            goto EXIT;
        }
    }

    // STEP3: construct audio capture pipe
    {
        AX_U32 nChnCnt = 2;

        for (AX_U32 i = 0; i < m_nPipeNum; i ++) {
            if (PT_G711A == pstPipeAttr[i].eType
                || PT_G711U == pstPipeAttr[i].eType
                || PT_G726 == pstPipeAttr[i].eType
                || PT_LPCM == pstPipeAttr[i].eType //TODO:
                || AX_AUDIO_SOUND_MODE_MONO == pstPipeAttr[i].eSoundMode) {
                nChnCnt = 1;
                break;
            }
        }

        for (AX_U32 i = 0; i < m_nPipeNum; i ++) {
            AENC_CONFIG_T stPipeAttr = pstPipeAttr[i];
            stPipeAttr.bLink = m_bLink;

            if (1 == nChnCnt) {
                stPipeAttr.nChnCnt = nChnCnt;
                stPipeAttr.eSoundMode = AX_AUDIO_SOUND_MODE_MONO;
            }

            // get encoder samplerate from dev
            stPipeAttr.eSampleRate = m_pAudioDev->GetEncoderSampleRate();

            CAudioEncoder *pAudioEncoder = new CAudioEncoder(stPipeAttr.nChannel, stPipeAttr);

            if (!pAudioEncoder) {
                goto EXIT;
            }

            bRet = pAudioEncoder->Init();

            if (!bRet) {
                goto EXIT;
            }

            // register observer
            if (m_pAudioDev) {
                m_vecACapObs.emplace_back(CObserverMaker::CreateObserver<CAencObserver>(pAudioEncoder));
                m_pAudioDev->RegObserver(m_vecACapObs[m_vecACapObs.size() - 1].get());
            }

            m_vecAudioEncoderInst.emplace_back(pAudioEncoder);
        }
    }

EXIT:
    if (!bRet) {
        DeInit();
    }

    return bRet;
}

AX_BOOL CAudioCap::DeInit() {
    LOG_MM_I(ACAP, "+++");

    // STEP 1: deinit aenc
    for (auto& pInstance : m_vecAudioEncoderInst) {
        pInstance->DeInit();
        delete pInstance;
    }
    m_vecAudioEncoderInst.clear();

    // STEP 2: deinit dev
    if (m_pAudioDev) {
        m_pAudioDev->DeInit();
        delete m_pAudioDev;
        m_pAudioDev = nullptr;
    }

    if (m_bLink) {
        for (AX_U32 i = 0; i < m_nPipeNum; i ++) {
            AX_MOD_INFO_T stAiMod = {AX_ID_AI, (AX_S32)m_nCardId, (AX_S32)m_nDeviceId};
            AX_MOD_INFO_T stAencMod = {AX_ID_AENC, 0, (AX_S32)m_nPipeList[i]};

            AX_S32 nRet = AX_SYS_UnLink(&stAiMod, &stAencMod);
            if (0 != nRet) {
                LOG_MM_E(ACAP, "AX_SYS_UnLink AI[%d][%d]<->AENC[%d] failed ret=%08X", m_nCardId, m_nDeviceId, i, nRet);
            }
        }
    }

    LOG_MM_I(ACAP, "---");

    return AX_TRUE;
}

AX_BOOL CAudioCap::Start() {
    AX_BOOL bRet = AX_FALSE;

    // start audio capture device
    {
        if (m_pAudioDev) {
            bRet = m_pAudioDev->Start();

            if (!bRet) {
                goto EXIT;
            }
        }
    }

    // start audio capture pipe
    {
        for (auto& pInstance : m_vecAudioEncoderInst) {
            STAGE_START_PARAM_T stStartParam = {.bStartProcessingThread = AX_TRUE};

            bRet = pInstance->Start(&stStartParam);
        }
    }

EXIT:
    if (!bRet) {
        Stop();
    }

    return bRet;
}

AX_BOOL CAudioCap::Stop() {
    // STEP 1: stop aenc
    for (auto& pInstance : m_vecAudioEncoderInst) {
        pInstance->Stop();
    }

    // STEP 2: deinit dev
    if (m_pAudioDev) {
        m_pAudioDev->Stop();
    }

    return AX_TRUE;
}

AX_VOID CAudioCap::RegObserver(AX_AUDIO_CAP_NODE_E eNode, AX_U32 nChannel, IObserver* pObserver) {
    if (nullptr != pObserver) {
        switch (eNode) {
        case AX_AUDIO_CAP_NODE_RAW:
            {
                if (m_pAudioDev) {
                    m_pAudioDev->RegObserver(pObserver);
                }
            }
            break;

        case AX_AUDIO_CAP_NODE_AENC:
            {
                CAudioEncoder *pAudioEncoder = GetAudioEncoder(nChannel);

                if (pAudioEncoder) {
                    pAudioEncoder->RegObserver(pObserver);
                }
            }
            break;

        default:
            break;
        }
    }
}

AX_VOID CAudioCap::UnregObserver(AX_AUDIO_CAP_NODE_E eNode, AX_U32 nChannel, IObserver* pObserver) {
    if (nullptr == pObserver) {
        return;
    }

    switch (eNode) {
    case AX_AUDIO_CAP_NODE_RAW:
        {
            if (m_pAudioDev) {
                m_pAudioDev->RegObserver(pObserver);
            }
        }
        break;
    
    case AX_AUDIO_CAP_NODE_AENC:
        {
            CAudioEncoder *pAudioEncoder = GetAudioEncoder(nChannel);

            if (pAudioEncoder) {
                pAudioEncoder->UnregObserver(pObserver);
            }
        }
        break;
    
    default:
        break;
    }
}

AX_BOOL CAudioCap::GetVolume(AX_F64 &fVolume) {
    if (m_pAudioDev) {
        AX_BOOL bRet = m_pAudioDev->GetVolume(fVolume);

        if (fVolume > AUDIO_CAP_VOLUME_MAX) {
            fVolume = AUDIO_CAP_VOLUME_MAX;
        }

        return bRet;
    }

    return AX_FALSE;
}

AX_BOOL CAudioCap::SetVolume(const AX_F64 &fVolume) {
    AX_F64 fVolumeSet = fVolume;
    if (fVolumeSet > AUDIO_CAP_VOLUME_MAX) {
        fVolumeSet = AUDIO_CAP_VOLUME_MAX;
    }

    if (m_pAudioDev) {
        return m_pAudioDev->SetVolume(fVolumeSet);
    }

    return AX_FALSE;
}

AX_BOOL CAudioCap::GetDevAttr(AUDIO_CAP_DEV_ATTR_T &stAttr) {
    if (m_pAudioDev) {
        return m_pAudioDev->GetAttr(stAttr);
    }

    return AX_FALSE;
}

AX_BOOL CAudioCap::SetDevAttr(const AUDIO_CAP_DEV_ATTR_T &stAttr) {
    if (m_pAudioDev) {
        return m_pAudioDev->SetAttr(stAttr);
    }

    return AX_FALSE;
}

AX_BOOL CAudioCap::GetAencAttr(AX_U32 nChannel, AENC_CONFIG_T &stAttr) {
    CAudioEncoder *pAudioEncoder = GetAudioEncoder(nChannel);

    if (!pAudioEncoder) {
        LOG_MM_E(ACAP, "invalid channel: %d", nChannel);
        return AX_FALSE;
    }

    return pAudioEncoder->GetAttr(stAttr);
}

AX_BOOL CAudioCap::SetAencAttr(AX_U32 nChannel, const AENC_CONFIG_T &stAttr) {
    CAudioEncoder *pAudioEncoder = GetAudioEncoder(nChannel);

    if (!pAudioEncoder) {
        LOG_MM_E(ACAP, "invalid channel: %d", nChannel);
        return AX_FALSE;
    }

    return pAudioEncoder->SetAttr(stAttr);
}

AX_BOOL CAudioCap::GetAacEncoderConfigBuf(AX_U32 nChannel, const AX_U8 **ppConfBuf, AX_U32 *pDataSize) {
    CAudioEncoder *pAudioEncoder = GetAudioEncoder(nChannel);

    if (!pAudioEncoder) {
        LOG_MM_E(ACAP, "invalid channel: %d", nChannel);
        return AX_FALSE;
    }

    return pAudioEncoder->GetAacEncoderConfigBuf(ppConfBuf, pDataSize);
}

CAudioEncoder* CAudioCap::GetAudioEncoder(AX_U32 nChannel) {
    for (auto pInst : m_vecAudioEncoderInst) {
        if (nChannel == pInst->GetChannel()) {
            return pInst;
        }
    }

    return nullptr;
}
