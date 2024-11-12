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
#include "AudioCapDev.hpp"
#include "AudioEncoder.hpp"

#define AUDIO_CAP_VOLUME_DEFAULT   (1.0)
#define AUDIO_CAP_VOLUME_MAX       (10.0)

typedef enum axAUDIO_CAP_NODE_E {
    AX_AUDIO_CAP_NODE_RAW,
    AX_AUDIO_CAP_NODE_AENC,
    AX_AUDIO_CAP_NODE_BUTT
} AX_AUDIO_CAP_NODE_E;

class CAudioCap {
public:
    CAudioCap();
    virtual ~CAudioCap();

    AX_BOOL Init(const AUDIO_CAP_DEV_ATTR_PTR pstCapDevAttr, AX_U32 nPipeNum, const AENC_CONFIG_PTR pstPipeAttr);
    AX_BOOL DeInit();

    AX_BOOL Start();
    AX_BOOL Stop();

    AX_BOOL GetVolume(AX_F64 &fVolume);
    AX_BOOL SetVolume(const AX_F64 &fVolume);

    AX_BOOL GetDevAttr(AUDIO_CAP_DEV_ATTR_T &stAttr);
    AX_BOOL SetDevAttr(const AUDIO_CAP_DEV_ATTR_T &stAttr);
    AX_BOOL GetAencAttr(AX_U32 nChannel, AENC_CONFIG_T &stAttr);
    AX_BOOL SetAencAttr(AX_U32 nChannel, const AENC_CONFIG_T &stAttr);

    AX_BOOL GetAacEncoderConfigBuf(AX_U32 nChannel, const AX_U8 **ppConfBuf, AX_U32 *pDataSize);

    AX_VOID RegObserver(AX_AUDIO_CAP_NODE_E eNode, AX_U32 nChannel, IObserver* pObserver);
    AX_VOID UnregObserver(AX_AUDIO_CAP_NODE_E eNode, AX_U32 nChannel, IObserver* pObserver);

private:
    CAudioEncoder* GetAudioEncoder(AX_U32 nChannel);

protected:
    std::vector<std::unique_ptr<IObserver>> m_vecACapObs;

private:
    AX_BOOL m_bLink{AX_TRUE};
    AX_U32 m_nCardId{0};
    AX_U32 m_nDeviceId{0};
    AX_U32 m_nPipeNum{0};
    AX_S32 m_nPipeList[AX_AENC_MAX_CHN_NUM]{0};
    CAudioCapDev *m_pAudioDev{nullptr};
    std::vector<CAudioEncoder*> m_vecAudioEncoderInst;
};
