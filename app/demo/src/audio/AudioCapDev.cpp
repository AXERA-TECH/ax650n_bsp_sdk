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
#include "AudioCapDev.hpp"
#include "AppLogApi.h"
#include "ElapsedTimer.hpp"
#include "GlobalDef.h"
#include "ax_aenc_api.h"

#define ACDEV "ACDEV"

using namespace std;

namespace {
static AX_BOOL IsUpTalkVqeEnabled(const AX_AP_UPTALKVQE_ATTR_T &stVqeAttr) {
    return (AX_BOOL) ((stVqeAttr.stAecCfg.enAecMode != AX_AEC_MODE_DISABLE) ||
                        (stVqeAttr.stNsCfg.bNsEnable != AX_FALSE) ||
                        (stVqeAttr.stAgcCfg.bAgcEnable != AX_FALSE));
}
}

CAudioCapDev::CAudioCapDev(const AUDIO_CAP_DEV_ATTR_T& stAttr)
: m_stCapDevAttr(stAttr) {
    m_stResampleAttr.bEnable = AX_FALSE;
    m_stResampleAttr.eSampleRate = stAttr.eSampleRate;
    m_eAencSampleRate = stAttr.eSampleRate;
}

CAudioCapDev::~CAudioCapDev() {

}

AX_BOOL CAudioCapDev::Init() {
    if (AX_INVALID_POOLID == m_nACapDevPoolId) {
        AX_POOL_CONFIG_T stPoolConfig;

        stPoolConfig.MetaSize = AUDIO_CAP_DEV_META_SIZE;
        stPoolConfig.BlkSize = (0 == m_stCapDevAttr.nBlkSize) ? AUDIO_CAP_DEV_DEFAULT_BLK_SIZE : m_stCapDevAttr.nBlkSize;
        stPoolConfig.BlkCnt = (0 == m_stCapDevAttr.nBlkCnt) ? AUDIO_CAP_DEV_DEFAULT_BLK_CNT : m_stCapDevAttr.nBlkCnt;
        stPoolConfig.IsMergeMode = AX_FALSE;
        stPoolConfig.CacheMode = POOL_CACHE_MODE_NONCACHE;
        strcpy((char *)stPoolConfig.PartitionName, "anonymous");
        strcpy((char *)stPoolConfig.PoolName, "AUDIO_CAP");

        // STEP 1: creat pool
        m_nACapDevPoolId = AX_POOL_CreatePool(&stPoolConfig);

        if (AX_INVALID_POOLID == m_nACapDevPoolId) {
            LOG_MM_E(ACDEV, "AX_POOL_CreatePool failed[size: %d, cnt: %d]", stPoolConfig.BlkSize, stPoolConfig.BlkCnt);
            return AX_FALSE;
        }
    }

    // check resample
    AX_AP_UPTALKVQE_ATTR_T &stVqeAttr = m_stCapDevAttr.stVqeAttr;
    if (IsUpTalkVqeEnabled(stVqeAttr)) {
        // need resample
        if (stVqeAttr.s32SampleRate != (AX_S32)m_stCapDevAttr.eSampleRate) {
            m_stResampleAttr.bEnable = AX_TRUE;
            m_stResampleAttr.eSampleRate = (AX_AUDIO_SAMPLE_RATE_E)stVqeAttr.s32SampleRate;
        }
    }

    if (m_stResampleAttr.bEnable) {
        m_eAencSampleRate = m_stResampleAttr.eSampleRate;
    }

    return AX_TRUE;
}

AX_BOOL CAudioCapDev::DeInit() {
    if (m_bStart) {
        Stop();
    }

    // STEP1: destroy pool
    if (AX_INVALID_POOLID != m_nACapDevPoolId) {
        AX_POOL_DestroyPool(m_nACapDevPoolId);
        m_nACapDevPoolId = AX_INVALID_POOLID;
    }

    return AX_TRUE;
}

AX_BOOL CAudioCapDev::Start() {
    if (!m_bStart) {
        if (!Create()) {
            return AX_FALSE;
        }

        StartWorkThread();

        m_bStart = AX_TRUE;
    }

    return AX_TRUE;
}

AX_BOOL CAudioCapDev::Stop() {
    if (m_bStart) {
        m_bStart = AX_FALSE;

        StopWorkThread();

        Destroy();
    }

    return AX_TRUE;
}

AX_BOOL CAudioCapDev::Restart(const AUDIO_CAP_DEV_ATTR_T &stAttr) {
    // TODO:
    if (stAttr.nBlkSize != m_stCapDevAttr.nBlkSize
        || stAttr.nBlkCnt != m_stCapDevAttr.nBlkCnt) {
        Stop();

        DeInit();

        m_stCapDevAttr = stAttr;

        Init();
    }
    else {
        Stop();

        m_stCapDevAttr = stAttr;
    }

    return Start();
}

AX_BOOL CAudioCapDev::Create() {
    AX_S32 nRet = 0;

    AX_U32 &nCardId = m_stCapDevAttr.nCardId;
    AX_U32 &nDeviceId = m_stCapDevAttr.nDeviceId;
    AX_AP_UPTALKVQE_ATTR_T &stVqeAttr = m_stCapDevAttr.stVqeAttr;

    AX_AI_ATTR_T stAttr;
    memset(&stAttr, 0x00, sizeof(stAttr));
    stAttr.enBitwidth = m_stCapDevAttr.eBitWidth;
    stAttr.enLinkMode = m_stCapDevAttr.bLink ? AX_LINK_MODE : AX_UNLINK_MODE;
    stAttr.enSamplerate = m_stCapDevAttr.eSampleRate;
    stAttr.enLayoutMode = m_stCapDevAttr.eLayoutMode;
    stAttr.U32Depth = (0 == m_stCapDevAttr.nDepth) ? AUDIO_CAP_DEV_DEFAULT_DEPTH : m_stCapDevAttr.nDepth;
    stAttr.u32PeriodSize = m_stCapDevAttr.nPeriodSize;
    stAttr.u32PeriodCount = 4;

    // STEP 1: set AI device attribute
    nRet = AX_AI_SetPubAttr(nCardId, nDeviceId, &stAttr);

    if (0 != nRet) {
        LOG_MM_E(ACDEV, "AX_AI_SetPubAttr[%d][%d] failed ret=%08X", nCardId, nDeviceId, nRet);
        goto EXIT1;
    }

    // STEP 2: attach pool
    nRet = AX_AI_AttachPool(nCardId, nDeviceId, m_nACapDevPoolId);

    if (0 != nRet) {
        LOG_MM_E(ACDEV, "AX_AI_AttachPool[%d][%d] poolId[%x] failed ret=%08X", nCardId, nDeviceId, m_nACapDevPoolId, nRet);
        goto EXIT1;
    }

    // STEP 3: set 3A attribute
    if (IsUpTalkVqeEnabled(stVqeAttr)) {
        // TODO: only support AX_AGC_MODE_FIXED_DIGITAL
        if (stVqeAttr.stAgcCfg.bAgcEnable) {
            stVqeAttr.stAgcCfg.enAgcMode = AX_AGC_MODE_FIXED_DIGITAL;
        }

        nRet = AX_AI_SetUpTalkVqeAttr(nCardId, nDeviceId, &stVqeAttr);

        if (0 != nRet) {
            LOG_MM_E(ACDEV, "AX_AI_SetUpTalkVqeAttr[%d][%d] failed ret=%08X", nCardId, nDeviceId, nRet);
            goto EXIT2;
        }
    }

    // STEP 4: enable device
    nRet = AX_AI_EnableDev(nCardId, nDeviceId);

    if (0 != nRet) {
        LOG_MM_E(ACDEV, "AX_AI_EnableDev[%d][%d] failed ret=%08X", nCardId, nDeviceId, nRet);
        goto EXIT2;
    }

    // STEP 5: enable resample
    if (m_stResampleAttr.bEnable) {
        nRet = AX_AI_EnableResample(nCardId, nDeviceId, m_stResampleAttr.eSampleRate);

        if (0 != nRet) {
            LOG_MM_E(ACDEV, "AX_AI_EnableResample[%d][%d] failed ret=%08X", nCardId, nDeviceId, nRet);
            goto EXIT3;
        }
    }

    return AX_TRUE;

EXIT3:
    if (m_stResampleAttr.bEnable) {
        AX_AI_DisableResample(nCardId, nDeviceId);
    }

EXIT2:
    AX_AI_DetachPool(nCardId, nDeviceId);

EXIT1:
    return AX_FALSE;
}

AX_BOOL CAudioCapDev::Destroy() {
    AX_U32 &nCardId = m_stCapDevAttr.nCardId;
    AX_U32 &nDeviceId = m_stCapDevAttr.nDeviceId;

    // STEP1: disbale dev
    AX_AI_DisableDev(nCardId, nDeviceId);

    // STEP2: detatch pool
    AX_AI_DetachPool(nCardId, nDeviceId);

    return AX_TRUE;
}

AX_VOID CAudioCapDev::StartWorkThread() {
    if (!GetDevAttr()->bLink) {
        m_bGetThreadRunning = AX_TRUE;
        m_hGetThread = std::thread(&CAudioCapDev::FrameGetThreadFunc, this, this);
    }
}

AX_VOID CAudioCapDev::StopWorkThread() {
    m_bGetThreadRunning = AX_FALSE;

    if (m_hGetThread.joinable()) {
        m_hGetThread.join();
    }
}

AX_VOID CAudioCapDev::FrameGetThreadFunc(CAudioCapDev* pCaller) {
    CAudioCapDev* pThis = (CAudioCapDev*)pCaller;

    AX_U32 nCardId = pThis->GetDevAttr()->nCardId;
    AX_U32 nDeviceId = pThis->GetDevAttr()->nDeviceId;

    LOG_MM_I(ACDEV, "[%d][%d] +++", nCardId, nDeviceId);

    prctl(PR_SET_NAME, "APP_ACAP_Get");

    AX_S32 nRet = AX_SUCCESS;

    while (pThis->m_bGetThreadRunning) {
        AUDIO_CAP_FRAME_T stFrame;

        nRet = AX_AI_GetFrame(nCardId, nDeviceId, &stFrame.stAFrame, -1);

        if (0 != nRet) {
            if (AX_ERR_AENC_BUF_EMPTY == nRet) {
                continue;
            }
            LOG_MM_E(ACDEV, "AX_AI_GetFrame[%d][%d] failed, ret=%08X", nCardId, nDeviceId, nRet);
            break;
        }

        if (0 == stFrame.stAFrame.u32Len) {
            AX_AI_ReleaseFrame(nCardId, nDeviceId, &stFrame.stAFrame);

            continue;
        }

        stFrame.nChnCnt = m_stCapDevAttr.nChnCnt;
        stFrame.eType = PT_LPCM;
        stFrame.eBitWidth = m_stCapDevAttr.eBitWidth;
        stFrame.eSampleRate = m_stCapDevAttr.eSampleRate;

        NotifyAll(nDeviceId, &stFrame);

        AX_AI_ReleaseFrame(nCardId, nDeviceId, &stFrame.stAFrame);
    }

    LOG_MM_I(ACDEV, "[%d][%d] ---", nCardId, nDeviceId);
}

AX_VOID CAudioCapDev::RegObserver(IObserver* pObserver) {
    if (nullptr != pObserver) {
        OBS_TRANS_ATTR_T tTransAttr;
        tTransAttr.nChannel = m_stCapDevAttr.nCardId;
        tTransAttr.nPayloadType = m_stCapDevAttr.nDeviceId;

        if (pObserver->OnRegisterObserver(E_OBS_TARGET_TYPE_AIRAW, m_stCapDevAttr.nCardId, m_stCapDevAttr.nDeviceId, &tTransAttr)) {
            m_vecObserver.emplace_back(pObserver);
        }
    }
}

AX_VOID CAudioCapDev::UnregObserver(IObserver* pObserver) {
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

AX_VOID CAudioCapDev::NotifyAll(AX_U32 nChannel, AX_VOID* pStream) {
    if (nullptr == pStream) {
        return;
    }

    for (vector<IObserver*>::iterator it = m_vecObserver.begin(); it != m_vecObserver.end(); it++) {
        (*it)->OnRecvData(E_OBS_TARGET_TYPE_AIRAW, 0, nChannel, pStream);
    }
}

AX_BOOL CAudioCapDev::GetVolume(AX_F64 &fVolume) {
    if (!m_bStart) {
        return AX_TRUE;
    }

    AX_U32 &nCardId = m_stCapDevAttr.nCardId;
    AX_U32 &nDeviceId = m_stCapDevAttr.nDeviceId;

    AX_S32 nRet = AX_AI_GetVqeVolume(nCardId, nDeviceId, &fVolume);

    if (0 != nRet) {
        LOG_MM_E(ACDEV, "AX_AO_GetVqeVolume[%d][%d] failed ret=%08X", nCardId, nDeviceId, nRet);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CAudioCapDev::SetVolume(const AX_F64 &fVolume) {
    if (!m_bStart) {
        return AX_TRUE;
    }

    AX_U32 &nCardId = m_stCapDevAttr.nCardId;
    AX_U32 &nDeviceId = m_stCapDevAttr.nDeviceId;

    AX_S32 nRet = AX_AI_SetVqeVolume(nCardId, nDeviceId, fVolume);

    if (0 != nRet) {
        LOG_MM_E(ACDEV, "AX_AI_SetVqeVolume[%d][%d] failed ret=%08X", nCardId, nDeviceId, nRet);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CAudioCapDev::GetAttr(AUDIO_CAP_DEV_ATTR_T &stAttr) {
    stAttr = m_stCapDevAttr;

    return AX_TRUE;
}

AX_BOOL CAudioCapDev::SetAttr(const AUDIO_CAP_DEV_ATTR_T &stAttr) {
    if (m_bStart) {
        if (stAttr.bLink != m_stCapDevAttr.bLink
            || stAttr.nCardId != m_stCapDevAttr.nCardId
            || stAttr.nDeviceId != m_stCapDevAttr.nDeviceId
            || stAttr.nBlkSize != m_stCapDevAttr.nBlkSize
            || stAttr.nBlkCnt != m_stCapDevAttr.nBlkCnt
            || stAttr.nDepth != m_stCapDevAttr.nDepth
            || stAttr.nChnCnt != m_stCapDevAttr.nChnCnt
            || stAttr.nPeriodSize != m_stCapDevAttr.nPeriodSize
            || stAttr.eBitWidth != m_stCapDevAttr.eBitWidth
            || stAttr.eSampleRate != m_stCapDevAttr.eSampleRate
            || stAttr.eLayoutMode != m_stCapDevAttr.eLayoutMode) {
            return Restart(stAttr);
        }
        else {
            AX_AP_UPTALKVQE_ATTR_T stVqeAttr = stAttr.stVqeAttr;

            if (stVqeAttr.stAgcCfg.bAgcEnable) {
                stVqeAttr.stAgcCfg.enAgcMode = AX_AGC_MODE_FIXED_DIGITAL;
            }

            if (memcmp(&m_stCapDevAttr.stVqeAttr, &stVqeAttr, sizeof(stVqeAttr)) != 0) {
                return Restart(stAttr);
            }
        }

        m_stCapDevAttr = stAttr;

        return AX_TRUE;
    }

    if (stAttr.nBlkSize != m_stCapDevAttr.nBlkSize
        || stAttr.nBlkCnt != m_stCapDevAttr.nBlkCnt) {
        DeInit();

        m_stCapDevAttr = stAttr;

        Init();
    }
    else {
        m_stCapDevAttr = stAttr;
    }

    return AX_TRUE;
}

AX_BOOL CAudioCapDev::SetResample(const AX_AUDIO_SAMPLE_RATE_E &eSampleRate) {
    if (!m_bStart) {
        return AX_FALSE;
    }

    // need enable resample
    if (eSampleRate != m_stCapDevAttr.eSampleRate
        && !m_stResampleAttr.bEnable) {
        m_stResampleAttr.bEnable = AX_TRUE;
        m_stResampleAttr.eSampleRate = eSampleRate;
        m_eAencSampleRate = eSampleRate;

        return Restart(m_stCapDevAttr);
    }
    // need disable resample
    else if (eSampleRate == m_stCapDevAttr.eSampleRate
            && m_stResampleAttr.bEnable) {
        m_stResampleAttr.bEnable = AX_FALSE;
        m_stResampleAttr.eSampleRate = eSampleRate;
        m_eAencSampleRate = eSampleRate;

        return Restart(m_stCapDevAttr);
    }

    return AX_TRUE;
}
