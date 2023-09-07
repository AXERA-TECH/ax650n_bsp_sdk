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

#include <vector>
#include "ax_codec_comm.h"
#include "ax_comm_aio.h"
#include "ax_ao_api.h"

#define AUDIO_PLAY_DEV_DEFAULT_DEPTH    (32)

typedef enum audioPLAY_DEV_STATUS_E {
    AUDIO_PLAY_DEV_STATUS_DISABLE,
    AUDIO_PLAY_DEV_STATUS_IDLE,
    AUDIO_PLAY_DEV_STATUS_BUSY,
    AUDIO_PLAY_DEV_STATUS_BUTT
} AUDIO_PLAY_DEV_STATUS_E;

typedef struct audioPLAY_DEV_RESAMPLE_ATTR_T {
    AX_BOOL bEnable;
    AX_AUDIO_SAMPLE_RATE_E eSampleRate;
} AUDIO_PLAY_DEV_RESAMPLE_ATTR_T, *AUDIO_PLAY_DEV_RESAMPLE_ATTR_PTR;

typedef struct audioPLAY_DEV_ATTR_T {
    AX_BOOL bLink;
    AX_U32 nCardId;
    AX_U32 nDeviceId;
    AX_U32 nDepth;
    AX_U32 nChnCnt;
    AX_U32 nPeriodSize;
    AX_AUDIO_BIT_WIDTH_E eBitWidth;
    AX_AUDIO_SAMPLE_RATE_E eSampleRate;
    AX_AUDIO_SOUND_MODE_E eSoundMode;
    AX_AP_DNVQE_ATTR_T stVqeAttr;

    audioPLAY_DEV_ATTR_T () {
        bLink = AX_TRUE;
        nCardId = 0;
        nDeviceId = 3; // 0/1: HDMI device, only for play
        nChnCnt = 2;
        nDepth = AUDIO_PLAY_DEV_DEFAULT_DEPTH;
        nPeriodSize = 160;
        eBitWidth = AX_AUDIO_BIT_WIDTH_16;
        eSampleRate = AX_AUDIO_SAMPLE_RATE_16000;
        eSoundMode = AX_AUDIO_SOUND_MODE_MONO;
        stVqeAttr = {
            .s32SampleRate = eSampleRate,
            .u32FrameSamples = nPeriodSize,
            .stNsCfg = {
                .bNsEnable = AX_FALSE,
                .enAggressivenessLevel = AX_AGGRESSIVENESS_LEVEL_HIGH
            },
            .stAgcCfg = {
                .bAgcEnable = AX_FALSE,
                .enAgcMode = AX_AGC_MODE_FIXED_DIGITAL,
                .s16TargetLevel = -3,
                .s16Gain = 9
            }
        };
    }
} AUDIO_PLAY_DEV_ATTR_T, *AUDIO_PLAY_DEV_ATTR_PTR;

class CAudioPlayDev {
public:
    CAudioPlayDev(const AUDIO_PLAY_DEV_ATTR_T &stAttr);
    virtual ~CAudioPlayDev();

    virtual AX_BOOL Init();
    virtual AX_BOOL DeInit();
    virtual AX_BOOL Start();
    virtual AX_BOOL Stop();

    virtual AX_BOOL GetVolume(AX_F64 &fVolume);
    virtual AX_BOOL SetVolume(const AX_F64 &fVolume);
    virtual AX_BOOL QueryDevStat(AUDIO_PLAY_DEV_STATUS_E &eStatus);

    virtual AX_BOOL GetAttr(AUDIO_PLAY_DEV_ATTR_T &stAttr);
    virtual AX_BOOL SetAttr(const AUDIO_PLAY_DEV_ATTR_T &stAttr);

    virtual AX_BOOL SetResample(const AX_AUDIO_SAMPLE_RATE_E &eSampleRate);

    const AUDIO_PLAY_DEV_ATTR_PTR GetDevAttr() {
        return &m_stPlayDevAttr;
    }

private:
    AX_BOOL Restart(const AUDIO_PLAY_DEV_ATTR_T &stAttr);
    AX_BOOL Create();
    AX_BOOL Destroy();

private:
    AUDIO_PLAY_DEV_ATTR_T m_stPlayDevAttr;
    AX_U32 m_nPeriodSize{160};

    AX_BOOL m_bStart{AX_FALSE};

    AUDIO_PLAY_DEV_RESAMPLE_ATTR_T m_stResampleAttr;
};
