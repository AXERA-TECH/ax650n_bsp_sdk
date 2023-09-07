/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_RTSP_WRAPPER_H__
#define __AX_RTSP_WRAPPER_H__

#include "ax_base_type.h"
#include "ax_global_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_RTSP_MAX_CHANNEL_NUM (8)

typedef enum axRTSP_MEDIA_TYPE_E {
    AX_RTSP_MEDIA_VIDEO,
    AX_RTSP_MEDIA_AUDIO,
    AX_RTSP_MEDIA_BUTT
} AX_RTSP_MEDIA_TYPE_E;

typedef struct axRTSP_ATTR {
    AX_U32 nChannel;
    struct rstp_video_attr {
        AX_BOOL bEnable;
        AX_PAYLOAD_TYPE_E ePt;
    } stVideoAttr;
    struct rstp_audio_attr {
        AX_BOOL bEnable;
        AX_PAYLOAD_TYPE_E ePt;
        AX_U32 nSampleRate;
        AX_S32 nAOT; // audio object type
        AX_U8 nChnCnt;
    } stAudioAttr;
} AX_RTSP_ATTR_T;

typedef AX_VOID *AX_RTSP_HANDLE;

AX_S32 AX_Rtsp_Init(AX_RTSP_HANDLE *pHandle, const AX_RTSP_ATTR_T *stAttr, AX_S32 nNum, AX_U16 uBasePort);
AX_S32 AX_Rtsp_Start(AX_RTSP_HANDLE pHandle);
AX_S32 AX_Rtsp_Stop(AX_RTSP_HANDLE pHandle);
AX_S32 AX_Rtsp_Deinit(AX_RTSP_HANDLE pHandle);
AX_S32 AX_Rtsp_SendNalu(AX_RTSP_HANDLE pHandle, AX_U8 nChn, const AX_U8* pBuf, AX_U32 nLen, AX_U64 nPts, AX_BOOL bIFrame);
AX_S32 AX_Rtsp_SendAudio(AX_RTSP_HANDLE pHandle, AX_U32 nChn, AX_U8* pBuf, AX_U32 nLen, AX_U64 nPts);

#ifdef __cplusplus
}
#endif

#endif  // __AX_RTSP_WRAPPER_H__
