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
#include <queue>
#include "condition_variable.hpp"
#include "AXStage.hpp"
#include "IObserver.h"
#include "ax_codec_comm.h"
#include "AudioPlayDev.hpp"
#include "AudioDecoder.hpp"
#include "AudioFilePlay.hpp"

#define AUDIO_PLAY_VOLUME_DEFAULT   (1.0)
#define AUDIO_PLAY_VOLUME_MAX       (10.0)

#define AUDIO_FILE_PLAY_DEFAULT_CHANNEL (1)

class CAudioPlay {
public:
    CAudioPlay();
    virtual ~CAudioPlay();

    AX_BOOL Init(const AUDIO_PLAY_DEV_ATTR_PTR pstPlayDevAttr, AX_U32 nPipeNum, const ADEC_CONFIG_PTR pstPipeAttr);
    AX_BOOL DeInit();

    AX_BOOL Start();
    AX_BOOL Stop();

    AX_BOOL Play(AX_PAYLOAD_TYPE_E eType,
                    const AX_U8 *pData,
                    AX_U32 nDataSize,
                    AX_U64 u64SeqNum,
                    AX_U32 nChannel = 0);
    AX_BOOL PlayFile(AX_PAYLOAD_TYPE_E eType,
                        const std::string &strFileName,
                        AX_S32 nLoop,
                        AUDIO_FILE_PLAY_CALLBACK callback,
                        AX_VOID *pUserData,
                        AX_U32 nChannel = AUDIO_FILE_PLAY_DEFAULT_CHANNEL);
    AX_BOOL StopPlayFile(AX_U32 nChannel = AUDIO_FILE_PLAY_DEFAULT_CHANNEL);

    AX_BOOL GetVolume(AX_F64 &fVolume);
    AX_BOOL SetVolume(const AX_F64 &fVolume);

    AX_BOOL GetDevAttr(AUDIO_PLAY_DEV_ATTR_T &stAttr);
    AX_BOOL SetDevAttr(const AUDIO_PLAY_DEV_ATTR_T &stAttr);
    AX_BOOL GetAdecAttr(AX_U32 nChannel, ADEC_CONFIG_T &stAttr);
    AX_BOOL SetAdecAttr(AX_U32 nChannel, const ADEC_CONFIG_T &stAttr);

private:
    CAudioDecoder* GetAudioDecoder(AX_U32 nChannel);
    CAudioFilePlay* GetAudioFilePlay(AX_U32 nChannel);

protected:
    std::vector<std::unique_ptr<IObserver>> m_vecAPlayObs;

private:
    AX_BOOL m_bLink{AX_TRUE};
    AX_U32 m_nCardId{0};
    AX_U32 m_nDeviceId{0};
    AX_U32 m_nPipeNum{0};
    AX_S32 m_nPipeList[AX_ADEC_MAX_CHN_NUM]{0};
    CAudioPlayDev *m_pAudioDev{nullptr};
    std::vector<CAudioDecoder*> m_vecAudioDecoderInst;
    std::vector<CAudioFilePlay*> m_vecAudioFilePlayInst;
};
