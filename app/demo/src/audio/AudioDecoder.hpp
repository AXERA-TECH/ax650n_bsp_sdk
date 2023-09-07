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
#include "IObserver.h"
#include "ax_codec_comm.h"
#include "ax_comm_aio.h"
#include "ax_adec_api.h"

#define ADEC_META_SIZE          (8 * 1024)
#define ADEC_AAC_BLK_SIZE       (32 * 1024)
#define ADEC_OPUS_BLK_SIZE      (38400)
#define ADEC_G711_BLK_SIZE      (8 * 1024)
#define ADEC_DEFAULT_BLK_SIZE   (8 * 1024)
#define ADEC_DEFAULT_BLK_CNT    (64)
#define ADEC_DEFAULT_IN_DEPTH   (8)
#define ADEC_DEFAULT_OUT_DEPTH  (8)

#define ADEC_CONFIG_BUF_SIZE       (64)

typedef struct adecDEF_DECODER_ATTR_T {
    AX_U32 nBitRate;
    AX_AUDIO_BIT_WIDTH_E eBitWidth;
    AX_AUDIO_SAMPLE_RATE_E eSampleRate;
} ADEC_DEF_DECODER_ATTR_T, *ADEC_DEF_DECODER_ATTR_PTR;

typedef struct adecAAC_DECODER_ATTR_T {
    AX_AAC_TRANS_TYPE_E eTransType;
    AX_U8 ConfigBuf[ADEC_CONFIG_BUF_SIZE];
    AX_U32 nConfLen;
} ADEC_AAC_DECODER_ATTR_T, *ADEC_AAC_DECODER_ATTR_PTR;

typedef struct adecDECODER_ATTR_T {
    union {
        ADEC_DEF_DECODER_ATTR_T stDefDecoder;
        ADEC_AAC_DECODER_ATTR_T stAacDecoder;
    };
} ADEC_DECODER_ATTR_T, *ADEC_DECODER_ATTR_PTR;

typedef struct adecConfig {
    AX_BOOL bLink;
    AX_U32 nChannel;
    AX_U32 nBlkSize;
    AX_U32 nBlkCnt;
    AX_U32 nInDepth;
    AX_U32 nOutDepth;
    AX_PAYLOAD_TYPE_E eType;
    AX_AUDIO_BIT_WIDTH_E eBitWidth;
    AX_AUDIO_SAMPLE_RATE_E eSampleRate;
    AX_AUDIO_SOUND_MODE_E eSoundMode;
    ADEC_DECODER_ATTR_T stDecoderAttr;

    adecConfig (AX_PAYLOAD_TYPE_E eAdecType = PT_G711A) {
        bLink = AX_TRUE;
        nChannel = 0;
        nInDepth = ADEC_DEFAULT_IN_DEPTH;
        nOutDepth = ADEC_DEFAULT_OUT_DEPTH;
        eType = eAdecType;
        eBitWidth = AX_AUDIO_BIT_WIDTH_16;
        eSampleRate = AX_AUDIO_SAMPLE_RATE_16000;

        if (PT_AAC == eType) {
            nBlkSize = ADEC_AAC_BLK_SIZE;
            nBlkCnt = ADEC_DEFAULT_BLK_CNT;
            eSoundMode = AX_AUDIO_SOUND_MODE_STEREO;
            stDecoderAttr = {
                .stAacDecoder = {
                    .eTransType = AX_AAC_TRANS_TYPE_ADTS,
                    .nConfLen = 0
                }
            };
        }
        else {
            nBlkSize = ADEC_DEFAULT_BLK_SIZE;
            nBlkCnt = ADEC_DEFAULT_BLK_CNT;
            eSoundMode = AX_AUDIO_SOUND_MODE_MONO;
            stDecoderAttr = {
                .stDefDecoder = {
                    .nBitRate = 32000,
                    .eBitWidth = eBitWidth,
                    .eSampleRate = eSampleRate
                }
            };
        }
    }
} ADEC_CONFIG_T, *ADEC_CONFIG_PTR;

class CAudioDecoder {
public:
    CAudioDecoder(AX_U32 nChannel, const ADEC_CONFIG_T& tConfig);
    virtual ~CAudioDecoder();

    virtual AX_BOOL Init();
    virtual AX_BOOL DeInit();
    virtual AX_BOOL Start();
    virtual AX_BOOL Stop();

    virtual AX_BOOL GetAttr(ADEC_CONFIG_T &stAttr);
    virtual AX_BOOL SetAttr(const ADEC_CONFIG_T &stAttr);

    virtual AX_BOOL Play(AX_PAYLOAD_TYPE_E eType, const AX_AUDIO_STREAM_T *pstStream, AX_S32 nTimeout = -1);

    virtual AX_VOID RegObserver(IObserver* pObserver);
    virtual AX_VOID UnregObserver(IObserver* pObserver);

    const ADEC_CONFIG_T* GetChnCfg() {
        return &m_stAdecConfig;
    }

    AX_U32 GetChannel() {
        return m_nChannel;
    }

protected:
    AX_VOID FrameGetThreadFunc(CAudioDecoder* pCaller);
    AX_VOID StartWorkThread();
    AX_VOID StopWorkThread();

    AX_VOID NotifyAll(AX_U32 nChannel, AX_VOID* pstFrame);

private:
    AX_BOOL Restart(const ADEC_CONFIG_T &stAttr);
    AX_BOOL Create();
    AX_BOOL Destroy();

private:
    AX_U32 m_nChannel{0};
    AX_POOL m_nAPlayPipePoolId{AX_INVALID_POOLID};

    ADEC_CONFIG_T m_stAdecConfig;

    AX_BOOL m_bStart{AX_FALSE};

    std::thread m_hGetThread;
    AX_BOOL m_bGetThreadRunning{AX_FALSE};

    std::vector<IObserver*> m_vecObserver;
};
