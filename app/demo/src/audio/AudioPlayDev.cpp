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
#include "AudioPlayDev.hpp"
#include "AppLogApi.h"
#include "ElapsedTimer.hpp"
#include "GlobalDef.h"

#define APDEV "APDEV"

using namespace std;

namespace {
static AX_BOOL IsDnVqeEnabled(const AX_AP_DNVQE_ATTR_T &stVqeAttr) {
    return (AX_BOOL)((stVqeAttr.stNsCfg.bNsEnable != AX_FALSE) ||
                    (stVqeAttr.stAgcCfg.bAgcEnable != AX_FALSE));
}
}

CAudioPlayDev::CAudioPlayDev(const AUDIO_PLAY_DEV_ATTR_T &stAttr)
: m_stPlayDevAttr(stAttr) {
    m_stResampleAttr.bEnable = AX_FALSE;
    m_stResampleAttr.eSampleRate = stAttr.eSampleRate;
    m_nPeriodSize = stAttr.nPeriodSize;
}

CAudioPlayDev::~CAudioPlayDev() {

}

AX_BOOL CAudioPlayDev::Init() {
    return AX_TRUE;
}

AX_BOOL CAudioPlayDev::DeInit() {
    if (m_bStart) {
        Stop();
    }

    return AX_TRUE;
}

AX_BOOL CAudioPlayDev::Start() {
    if (!m_bStart) {
        if (!Create()) {
            return AX_FALSE;
        }

        m_bStart = AX_TRUE;
    }

    return AX_TRUE;
}

AX_BOOL CAudioPlayDev::Stop() {
    if (m_bStart) {
        Destroy();

        m_bStart = AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CAudioPlayDev::Restart(const AUDIO_PLAY_DEV_ATTR_T &stAttr) {
    Stop();

    m_stPlayDevAttr = stAttr;

    return Start();
}

AX_BOOL CAudioPlayDev::Create() {
    AX_S32 nRet = 0;

    AX_U32 &nCardId = m_stPlayDevAttr.nCardId;
    AX_U32 &nDeviceId = m_stPlayDevAttr.nDeviceId;
    AX_AP_DNVQE_ATTR_T &stVqeAttr = m_stPlayDevAttr.stVqeAttr;

    // check vqe
    if (IsDnVqeEnabled(stVqeAttr)) {
        m_nPeriodSize = stVqeAttr.u32FrameSamples;
    }

    AX_AO_ATTR_T stAttr;
    memset(&stAttr, 0x00, sizeof(stAttr));
    stAttr.enSamplerate = m_stPlayDevAttr.eSampleRate;
    stAttr.enBitwidth = m_stPlayDevAttr.eBitWidth;
    // stAttr.enSoundmode = AX_AUDIO_SOUND_MODE_STEREO; // fixed STEREO for ALSA
    // stAttr.u32ChnCnt = 2; // fixed 2 for ALSA
    stAttr.u32PeriodSize = m_nPeriodSize;
    stAttr.u32PeriodCount = 4;
    stAttr.U32Depth = (0 == m_stPlayDevAttr.nDepth) ? AUDIO_PLAY_DEV_DEFAULT_DEPTH : m_stPlayDevAttr.nDepth;
    stAttr.enLinkMode = m_stPlayDevAttr.bLink ? AX_LINK_MODE : AX_UNLINK_MODE;
    stAttr.bInsertSilence = AX_FALSE;

    // STEP 1: set AO device attribute
    nRet = AX_AO_SetPubAttr(nCardId, nDeviceId, &stAttr);

    if (0 != nRet) {
        LOG_MM_E(APDEV, "AX_AO_SetPubAttr[%d][%d] failed ret=%08X", nCardId, nDeviceId, nRet);
        goto EXIT1;
    }

    // STEP 2: set 3A attribute
    if (IsDnVqeEnabled(stVqeAttr)) {
        // TODO: only support AX_AGC_MODE_FIXED_DIGITAL
        if (stVqeAttr.stAgcCfg.bAgcEnable) {
            stVqeAttr.stAgcCfg.enAgcMode = AX_AGC_MODE_FIXED_DIGITAL;
        }

        nRet = AX_AO_SetDnVqeAttr(nCardId, nDeviceId, &stVqeAttr);

        if (0 != nRet) {
            LOG_MM_E(APDEV, "AX_AO_SetDnVqeAttr[%d][%d] failed ret=%08X", nCardId, nDeviceId, nRet);
            goto EXIT1;
        }
    }

    // STEP 3: enable device
    nRet = AX_AO_EnableDev(nCardId, nDeviceId);

    if (0 != nRet) {
        LOG_MM_E(APDEV, "AX_AO_EnableDev[%d][%d] failed ret=%08X", nCardId, nDeviceId, nRet);
        goto EXIT1;
    }

    // STEP 4: enable resample
    if (m_stResampleAttr.bEnable) {
        nRet = AX_AO_EnableResample(nCardId, nDeviceId, m_stResampleAttr.eSampleRate);

        if (0 != nRet) {
            LOG_MM_E(APDEV, "AX_AO_EnableResample[%d][%d] failed ret=%08X", nCardId, nDeviceId, nRet);
            goto EXIT2;
        }
    }

    return AX_TRUE;

EXIT2:
    if (m_stResampleAttr.bEnable) {
        AX_AO_DisableResample(nCardId, nDeviceId);
    }

EXIT1:
    return AX_FALSE;
}

AX_BOOL CAudioPlayDev::Destroy() {
    AX_U32 &nCardId = m_stPlayDevAttr.nCardId;
    AX_U32 &nDeviceId = m_stPlayDevAttr.nDeviceId;

    // STEP1: disbale dev
    AX_AO_DisableDev(nCardId, nDeviceId);

    return AX_TRUE;
}

AX_BOOL CAudioPlayDev::GetVolume(AX_F64 &fVolume) {
    if (!m_bStart) {
        return AX_TRUE;
    }

    AX_U32 &nCardId = m_stPlayDevAttr.nCardId;
    AX_U32 &nDeviceId = m_stPlayDevAttr.nDeviceId;

    AX_S32 nRet = AX_AO_GetVqeVolume(nCardId, nDeviceId, &fVolume);

    if (0 != nRet) {
        LOG_MM_E(APDEV, "AX_AO_GetVqeVolume[%d][%d] failed ret=%08X", nCardId, nDeviceId, nRet);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CAudioPlayDev::SetVolume(const AX_F64 &fVolume) {
    if (!m_bStart) {
        return AX_TRUE;
    }

    AX_U32 &nCardId = m_stPlayDevAttr.nCardId;
    AX_U32 &nDeviceId = m_stPlayDevAttr.nDeviceId;

    AX_S32 nRet = AX_AO_SetVqeVolume(nCardId, nDeviceId, fVolume);

    if (0 != nRet) {
        LOG_MM_E(APDEV, "AX_AO_SetVqeVolume[%d][%d] failed ret=%08X", nCardId, nDeviceId, nRet);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CAudioPlayDev::QueryDevStat(AUDIO_PLAY_DEV_STATUS_E &eStatus) {
    if (!m_bStart) {
        eStatus = AUDIO_PLAY_DEV_STATUS_DISABLE;
    }
    else {
        AX_U32 &nCardId = m_stPlayDevAttr.nCardId;
        AX_U32 &nDeviceId = m_stPlayDevAttr.nDeviceId;
        AX_AO_DEV_STATE_T stStatus;
        memset(&stStatus, 0x00, sizeof(stStatus));

        AX_S32 nRet = AX_AO_QueryDevStat(nCardId, nDeviceId, &stStatus);

        if (0 != nRet) {
            LOG_MM_E(APDEV, "AX_AO_QueryDevStat[%d][%d] failed ret=%08X", nCardId, nDeviceId, nRet);
            return AX_FALSE;
        }

        if (0 == stStatus.u32DevBusyNum) {
            eStatus = AUDIO_PLAY_DEV_STATUS_IDLE;
        }
        else {
            eStatus = AUDIO_PLAY_DEV_STATUS_BUSY;
        }
    }

    return AX_TRUE;
}

AX_BOOL CAudioPlayDev::GetAttr(AUDIO_PLAY_DEV_ATTR_T &stAttr) {
    stAttr = m_stPlayDevAttr;

    return AX_TRUE;
}

AX_BOOL CAudioPlayDev::SetAttr(const AUDIO_PLAY_DEV_ATTR_T &stAttr) {
    if (m_bStart) {
        if (stAttr.bLink != m_stPlayDevAttr.bLink
            || stAttr.nCardId != m_stPlayDevAttr.nCardId
            || stAttr.nDeviceId != m_stPlayDevAttr.nDeviceId
            || stAttr.nDepth != m_stPlayDevAttr.nDepth
            || stAttr.nChnCnt != m_stPlayDevAttr.nChnCnt
            || stAttr.nPeriodSize != m_stPlayDevAttr.nPeriodSize
            || stAttr.eBitWidth != m_stPlayDevAttr.eBitWidth
            || stAttr.eSampleRate != m_stPlayDevAttr.eSampleRate
            || stAttr.eSoundMode != m_stPlayDevAttr.eSoundMode) {
            return Restart(stAttr);
        }
        else {
            AX_AP_DNVQE_ATTR_T stVqeAttr = stAttr.stVqeAttr;

            if (stVqeAttr.stAgcCfg.bAgcEnable) {
                stVqeAttr.stAgcCfg.enAgcMode = AX_AGC_MODE_FIXED_DIGITAL;
            }

            if (memcmp(&m_stPlayDevAttr.stVqeAttr, &stVqeAttr, sizeof(stVqeAttr)) != 0) {
                return Restart(stAttr);
            }
        }

        m_stPlayDevAttr = stAttr;

        return AX_TRUE;
    }

    m_stPlayDevAttr = stAttr;

    return AX_TRUE;
}

AX_BOOL CAudioPlayDev::SetResample(const AX_AUDIO_SAMPLE_RATE_E &eSampleRate) {
    if (!m_bStart) {
        return AX_FALSE;
    }

    // need enable resample
    if (eSampleRate != m_stPlayDevAttr.eSampleRate
        && !m_stResampleAttr.bEnable) {
        m_stResampleAttr.bEnable = AX_TRUE;
        m_stResampleAttr.eSampleRate = eSampleRate;

        return Restart(m_stPlayDevAttr);
    }
    // need disable resample
    else if (eSampleRate == m_stPlayDevAttr.eSampleRate
            && m_stResampleAttr.bEnable) {
        m_stResampleAttr.bEnable = AX_FALSE;
        m_stResampleAttr.eSampleRate = eSampleRate;

        return Restart(m_stPlayDevAttr);
    }

    return AX_TRUE;
}
