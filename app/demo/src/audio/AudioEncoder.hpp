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
#include <thread>
#include "AXStage.hpp"
#include "IObserver.h"
#include "ax_codec_comm.h"
#include "ax_comm_aio.h"
#include "ax_aenc_api.h"

#define AENC_DEFAULT_IN_DEPTH      (8)
#define AENC_DEFAULT_OUT_DEPTH     (8)
#define AENC_CONFIG_BUF_SIZE       (64)
#define AENC_DEF_PTNUM_PER_FRM     (1024)
#define AENC_AAC_LC_PTNUM_PER_FRM  (1024)
#define AENC_AAC_PTNUM_PER_FRM     (480)
#define AENC_G726_PTNUM_PER_FRM    (480)

typedef struct aencDEF_ENCODER_ATTR_T {
    AX_U32 nBitRate;
    AX_AUDIO_BIT_WIDTH_E eBitWidth;
    AX_AUDIO_SAMPLE_RATE_E eSampleRate;
} AENC_DEF_ENCODER_ATTR_T, *AENC_DEF_ENCODER_ATTR_PTR;

typedef struct aencAAC_ENCODER_ATTR_T {
    AX_AAC_TYPE_E eAacType;
    AX_AAC_TRANS_TYPE_E eTransType;
    // AX_AAC_CHANNEL_MODE_E eChnMode;
    // AX_U32 nGranuleLength;    /* 1024: AAC-LC default
    //                            512: Length in LD/ELD configuration.
    //                            480: Length in LD/ELD configuration. (default for LD/ELD)
    //                            256: Length for ELD reduced delay mode (x2).
    //                            240: Length for ELD reduced delay mode (x2).
    //                            128: Length for ELD reduced delay mode (x4).
    //                            120: Length for ELD reduced delay mode (x4). */
    // AX_U8 arrConfigBuf[AENC_AAC_CONFIG_BUF_SIZE]; /* Configuration buffer in binary format as an
    //                                                            AudioSpecificConfig or StreamMuxConfig according to the
    //                                                            selected transport type(AX_AUDIO_AAC_TRANS_TYPE_RAW). */
} AENC_AAC_ENCODER_ATTR_T, *AENC_AAC_ENCODER_ATTR_PTR;

typedef struct aencENCODER_ATTR_T {
    union {
        AENC_DEF_ENCODER_ATTR_T stDefEncoder;
        AENC_AAC_ENCODER_ATTR_T stAacEncoder;
    };
} AENC_ENCODER_ATTR_T, *AENC_ENCODER_ATTR_PTR;

typedef struct aencConfig {
    AX_BOOL bLink;
    AX_U8 nChannel;
    AX_U32 nInDepth;
    AX_U32 nOutDepth;
    AX_U32 nChnCnt;
    AX_U32 nBitRate;
    AX_PAYLOAD_TYPE_E eType;
    AX_AUDIO_BIT_WIDTH_E eBitWidth;
    AX_AUDIO_SAMPLE_RATE_E eSampleRate;
    AX_AUDIO_SOUND_MODE_E eSoundMode;
    AENC_ENCODER_ATTR_T stEncoderAttr;

    aencConfig (AX_PAYLOAD_TYPE_E eAencType = PT_G711A) {
        bLink = AX_TRUE;
        nChannel = 0;
        nInDepth = AENC_DEFAULT_IN_DEPTH;
        nOutDepth = AENC_DEFAULT_OUT_DEPTH;
        nChnCnt = 2;
        eType = eAencType;
        eBitWidth = AX_AUDIO_BIT_WIDTH_16;
        eSampleRate = AX_AUDIO_SAMPLE_RATE_16000;
        nBitRate = 48000;

        if (PT_AAC == eType) {
            eSoundMode = AX_AUDIO_SOUND_MODE_STEREO;
            stEncoderAttr = {
                .stAacEncoder = {
                    .eAacType = AX_AAC_TYPE_AAC_LC,
                    .eTransType = AX_AAC_TRANS_TYPE_ADTS
                }
            };
        }
        else {
            eSoundMode = AX_AUDIO_SOUND_MODE_MONO;
            stEncoderAttr = {
                .stDefEncoder = {
                    .nBitRate = nBitRate,
                    .eBitWidth = eBitWidth,
                    .eSampleRate = eSampleRate
                }
            };
        }
    }
} AENC_CONFIG_T, *AENC_CONFIG_PTR;

typedef struct aenc_STREAM_T {
    AX_U32 nBitRate;
    AX_PAYLOAD_TYPE_E eType;
    AX_AUDIO_BIT_WIDTH_E eBitWidth;
    AX_AUDIO_SAMPLE_RATE_E eSampleRate;
    AX_AUDIO_SOUND_MODE_E eSoundMode;
    AX_AUDIO_STREAM_T stPack;
} AENC_STREAM_T, *AENC_STREAM_PTR;

class CAudioEncoder : public CAXStage {
public:
    CAudioEncoder(AX_U32 nChannel, const AENC_CONFIG_T& stConfig);
    virtual ~CAudioEncoder();

    virtual AX_BOOL Init() override;
    virtual AX_BOOL DeInit() override;
    virtual AX_BOOL Start(STAGE_START_PARAM_PTR pStartParams) override;
    virtual AX_BOOL Stop(AX_VOID) override;

    virtual AX_BOOL GetAttr(AENC_CONFIG_T &stAttr);
    virtual AX_BOOL SetAttr(const AENC_CONFIG_T &stAttr);
    virtual AX_BOOL GetAacEncoderConfigBuf(const AX_U8 **ppConfBuf, AX_U32 *pDataSize);

    virtual AX_VOID RegObserver(IObserver* pObserver);
    virtual AX_VOID UnregObserver(IObserver* pObserver);

    const AENC_CONFIG_PTR GetChnCfg() {
        return &m_stAencConfig;
    }

    AX_U32 GetChannel() {
        return m_nChannel;
    }

protected:
    virtual AX_BOOL ProcessFrame(CAXFrame* pFrame) override;
    AX_VOID NotifyAll(AX_U32 nChannel, AX_VOID* pstStream);

    AX_VOID FrameGetThreadFunc(CAudioEncoder* pCaller);
    AX_VOID StartWorkThread();
    AX_VOID StopWorkThread();

private:
    AX_BOOL Restart(const AENC_CONFIG_T &stAttr);
    AX_BOOL Create();
    AX_BOOL Destroy();

private:
    AX_U32 m_nChannel{0};
    AENC_CONFIG_T m_stAencConfig;

    AX_BOOL m_bStart{AX_FALSE};

    std::thread m_hGetThread;
    AX_BOOL m_bGetThreadRunning{AX_FALSE};

    std::vector<IObserver*> m_vecObserver;

    AX_U8 m_arrConfigBuf[AENC_CONFIG_BUF_SIZE];
    AX_U32 m_nConfigBufSize{0};
};
