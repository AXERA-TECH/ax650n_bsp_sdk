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

#include "AXRTPDefine.h"

typedef AX_VOID (*videoFrameOut)(AX_VOID *clientData, AX_U8 *outBuf, AX_S32 outLen, AX_S32 chn);
typedef AX_VOID (*audioFrameOut)(AX_VOID *clientData, AX_U8 *outBuf, AX_S32 outLen, AX_S32 chn);

typedef struct _AX_RTP_DEC_T
{
    AX_VOID *clientData;

#ifdef TEST_LATENCY
    AX_U64 sequence;
    AX_U64 pts;
#endif
    AX_U64 u64SeqNum;

    AX_S32 nChannel;
    AX_PAYLOAD_TYPE_E enAVMediaType;

    AX_U8 *outBuf;
    AX_S32 outBufLen;
    videoFrameOut cbVideoFrameOut;
    audioFrameOut cbAudioFrameOut;
}AX_RTP_DEC_T;

AX_S32 AX_RTPDEC_Init(AX_RTP_DEC_T *pstRtpDec, AX_VOID* data, AX_S32 chn, AX_PAYLOAD_TYPE_E enMediaType, videoFrameOut cbVideoFrameOut, audioFrameOut cbAudioFrameOut);
AX_S32 AX_RTPDEC_DeInit(AX_RTP_DEC_T *pstRtpDec);
AX_S32 AX_RTPDEC_Proc(AX_RTP_DEC_T *pstRtpDec, AX_U8 *inBuf, AX_S32 inLen);
