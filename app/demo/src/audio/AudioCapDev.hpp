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
#include "AXStage.hpp"
#include "IObserver.h"
#include "ax_codec_comm.h"
#include "ax_comm_aio.h"
#include "ax_ai_api.h"

#define AUDIO_CAP_DEV_META_SIZE         (8 * 1024)
#define AUDIO_CAP_DEV_DEFAULT_BLK_SIZE  (8 * 1024)
#define AUDIO_CAP_DEV_DEFAULT_BLK_CNT   (64)
#define AUDIO_CAP_DEV_DEFAULT_DEPTH     (32)

typedef struct audio_CAP_FRAME_T {
    AX_U32 nChnCnt;
    AX_PAYLOAD_TYPE_E eType;
    AX_AUDIO_BIT_WIDTH_E eBitWidth;
    AX_AUDIO_SAMPLE_RATE_E eSampleRate;
    AX_AUDIO_FRAME_T stAFrame;
} AUDIO_CAP_FRAME_T, *AUDIO_CAP_FRAME_PTR;

typedef struct audioCAP_DEV_RESAMPLE_ATTR_T {
    AX_BOOL bEnable;
    AX_AUDIO_SAMPLE_RATE_E eSampleRate;
} AUDIO_CAP_DEV_RESAMPLE_ATTR_T, *AUDIO_CAP_DEV_RESAMPLE_ATTR_PTR;

typedef struct audioCAP_DEV_ATTR_T {
    AX_BOOL bLink;
    AX_U32 nCardId;
    AX_U32 nDeviceId;
    AX_U32 nBlkSize;
    AX_U32 nBlkCnt;
    AX_U32 nDepth;
    AX_U32 nChnCnt;
    AX_U32 nPeriodSize;
    AX_AUDIO_BIT_WIDTH_E eBitWidth;
    AX_AUDIO_SAMPLE_RATE_E eSampleRate;
    AX_AI_LAYOUT_MODE_E eLayoutMode;
    AX_AP_UPTALKVQE_ATTR_T stVqeAttr;

    audioCAP_DEV_ATTR_T () {
        bLink = AX_TRUE;
        nCardId = 0;
        nDeviceId = 2; // (/dev/snd:/pcmC0_D0/1 is HDMI device, only can play)
        nBlkSize = AUDIO_CAP_DEV_DEFAULT_BLK_SIZE;
        nBlkCnt = AUDIO_CAP_DEV_DEFAULT_BLK_CNT;
        nChnCnt = 2;
        nDepth = AUDIO_CAP_DEV_DEFAULT_DEPTH;
        nPeriodSize = 160;
        eBitWidth = AX_AUDIO_BIT_WIDTH_16;
        eSampleRate = AX_AUDIO_SAMPLE_RATE_16000;
        eLayoutMode = AX_AI_MIC_REF;
        stVqeAttr = {
            .s32SampleRate = eSampleRate,
            .u32FrameSamples = nPeriodSize,
            .stAecCfg = {
                .enAecMode = AX_AEC_MODE_FLOAT,
                .stAecFloatCfg = {
                    .enSuppressionLevel = AX_SUPPRESSION_LEVEL_HIGH
                }
            },
            .stNsCfg = {
                .bNsEnable = AX_TRUE,
                .enAggressivenessLevel = AX_AGGRESSIVENESS_LEVEL_HIGH
            },
            .stAgcCfg = {
                .bAgcEnable = AX_TRUE,
                .enAgcMode = AX_AGC_MODE_FIXED_DIGITAL,
                .s16TargetLevel = -3,
                .s16Gain = 9
            }
        };
    }
} AUDIO_CAP_DEV_ATTR_T, *AUDIO_CAP_DEV_ATTR_PTR;

class CAudioCapDev {
public:
    CAudioCapDev(const AUDIO_CAP_DEV_ATTR_T& stAttr);
    virtual ~CAudioCapDev();

    virtual AX_BOOL Init();
    virtual AX_BOOL DeInit();
    virtual AX_BOOL Start();
    virtual AX_BOOL Stop();

    virtual AX_BOOL GetVolume(AX_F64 &fVolume);
    virtual AX_BOOL SetVolume(const AX_F64 &fVolume);

    virtual AX_BOOL GetAttr(AUDIO_CAP_DEV_ATTR_T &stAttr);
    virtual AX_BOOL SetAttr(const AUDIO_CAP_DEV_ATTR_T &stAttr);

    virtual AX_BOOL SetResample(const AX_AUDIO_SAMPLE_RATE_E &eSampleRate);

    AX_VOID RegObserver(IObserver* pObserver);
    AX_VOID UnregObserver(IObserver* pObserver);

    const AUDIO_CAP_DEV_ATTR_PTR GetDevAttr() {
        return &m_stCapDevAttr;
    }

    AX_AUDIO_SAMPLE_RATE_E GetEncoderSampleRate(AX_VOID) {
        return m_eAencSampleRate;
    }

protected:
    AX_VOID NotifyAll(AX_U32 nChannel, AX_VOID* pStream);

    AX_VOID FrameGetThreadFunc(CAudioCapDev* pCaller);
    AX_VOID StartWorkThread();
    AX_VOID StopWorkThread();

private:
    AX_BOOL Restart(const AUDIO_CAP_DEV_ATTR_T &stAttr);
    AX_BOOL Create();
    AX_BOOL Destroy();

private:
    AX_POOL m_nACapDevPoolId{AX_INVALID_POOLID};
    AUDIO_CAP_DEV_ATTR_T m_stCapDevAttr;
    AX_AUDIO_SAMPLE_RATE_E m_eAencSampleRate{AX_AUDIO_SAMPLE_RATE_16000};

    AX_BOOL m_bStart{AX_FALSE};

    std::thread m_hGetThread;
    AX_BOOL m_bGetThreadRunning{AX_FALSE};

    std::vector<IObserver*> m_vecObserver;

    AUDIO_CAP_DEV_RESAMPLE_ATTR_T m_stResampleAttr;
};
