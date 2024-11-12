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
#include <string.h>
#include <fstream>
#include <memory>
#include <string>
#include "IObserver.h"
#include "Mpeg4Encoder.h"
#include "VideoEncoder.h"

typedef struct BOX_RECORDER_ATTR_S {
    VENC_CHN veChn;
    AX_U32 nFifoDepth[2];
    AX_U32 nW;
    AX_U32 nH;
    RC_INFO_T stRC;
    AX_PAYLOAD_TYPE_E ePayloadType;
    AX_U32 nFps;
    AX_U32 nGop;
    AX_S32 nBitRate;
    std::string strRecordPath;
    AX_U64 nMaxRecordSize; /* MB */
    AX_BOOL bMuxer;
    AX_BOOL bLinkMode;

    BOX_RECORDER_ATTR_S(AX_VOID) {
        veChn = 0;
        nFifoDepth[0] = 4;
        nFifoDepth[1] = 8;
        nW = 0;
        nH = 0;
        ePayloadType = PT_H264;
        nFps = 60;
        nGop = nFps;
        nBitRate = 8192;
        nMaxRecordSize = 0;
        bMuxer = AX_TRUE;
        bLinkMode = AX_TRUE;
    }
} BOX_RECORDER_ATTR_T;

/**
 * @brief
 *
 */
class CBoxRecorder : public IObserver {
public:
    CBoxRecorder(AX_VOID) = default;

    AX_BOOL Init(const BOX_RECORDER_ATTR_T& stAttr);
    AX_BOOL DeInit(AX_VOID);
    AX_BOOL Start(AX_VOID);
    AX_BOOL Stop(AX_VOID);

    AX_BOOL SendFrame(CAXFrame* pFrame);

    AX_BOOL OnRecvData(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, AX_VOID* pData) override;
    AX_BOOL OnRegisterObserver(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, OBS_TRANS_ATTR_PTR pParams) override;

protected:
    AX_BOOL CreateEncoderInstance(AX_VOID);
    AX_BOOL CreateMuxerInstance(AX_VOID);

    AX_BOOL CreateRawFile(AX_VOID);

private:
    BOX_RECORDER_ATTR_T m_stAttr;
    std::unique_ptr<CVideoEncoder> m_pEncoder;
    std::ofstream m_ofs;
    AX_U64 m_nFileSize{0};
    std::unique_ptr<CMPEG4Encoder> m_pMuxer;
};