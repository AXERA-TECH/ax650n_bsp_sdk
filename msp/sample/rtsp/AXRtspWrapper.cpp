/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "AXRtspWrapper.h"
#include "AXRtspServer.h"

AX_S32 AX_Rtsp_Init(AX_RTSP_HANDLE *pHandle, const AX_RTSP_ATTR_T *pstAttr, AX_S32 nNum, AX_U16 uBasePort)
{
    AX_BOOL bRet = AX_FALSE;

    AXRtspServer *rtspServer = new AXRtspServer();

    if (rtspServer) {
        RTSP_ATTR_T stAttr[MAX_RTSP_CHANNEL_NUM];

        if (nNum > MAX_RTSP_CHANNEL_NUM) {
            nNum = MAX_RTSP_CHANNEL_NUM;
        }

        for (AX_S32 i = 0; i < nNum; i ++) {
            // channel
            stAttr[i].nChannel = pstAttr[i].nChannel;

            // video
            stAttr[i].stVideoAttr.bEnable = pstAttr[i].stVideoAttr.bEnable;
            stAttr[i].stVideoAttr.ePt = pstAttr[i].stVideoAttr.ePt;

            // audio
            stAttr[i].stAudioAttr.bEnable = pstAttr[i].stAudioAttr.bEnable;
            stAttr[i].stAudioAttr.ePt = pstAttr[i].stAudioAttr.ePt;
            stAttr[i].stAudioAttr.nSampleRate = pstAttr[i].stAudioAttr.nSampleRate;
            stAttr[i].stAudioAttr.nAOT = pstAttr[i].stAudioAttr.nAOT;
            stAttr[i].stAudioAttr.nChnCnt = pstAttr[i].stAudioAttr.nChnCnt;
        }

        bRet = rtspServer->Init(&stAttr[0], nNum, uBasePort);
    }

    if (pHandle) {
        *pHandle = (AX_RTSP_HANDLE)rtspServer;
    }

    return bRet ? 0 : -1;
}

AX_S32 AX_Rtsp_Start(AX_RTSP_HANDLE pHandle)
{
    AX_BOOL bRet = AX_FALSE;

    AXRtspServer *rtspServer = (AXRtspServer *)pHandle;

    if (rtspServer) {
        bRet = rtspServer->Start();
    }

    return bRet ? 0 : -1;
}

AX_S32 AX_Rtsp_Stop(AX_RTSP_HANDLE pHandle)
{
    AX_BOOL bRet = AX_FALSE;

    AXRtspServer *rtspServer = (AXRtspServer *)pHandle;

    if (rtspServer) {
        rtspServer->Stop();

        bRet = AX_TRUE;
    }

    return bRet ? 0 : -1;
}

AX_S32 AX_Rtsp_Deinit(AX_RTSP_HANDLE pHandle)
{
    AX_BOOL bRet = AX_FALSE;

    AXRtspServer *rtspServer = (AXRtspServer *)pHandle;

    if (rtspServer) {
        delete rtspServer;

        bRet = AX_TRUE;
    }

    return bRet ? 0 : -1;
}


AX_S32 AX_Rtsp_SendNalu(AX_RTSP_HANDLE pHandle, AX_U8 nChn, const AX_U8* pBuf, AX_U32 nLen, AX_U64 nPts, AX_BOOL bIFrame)
{
    AX_BOOL bRet = AX_FALSE;

    AXRtspServer *rtspServer = (AXRtspServer *)pHandle;

    if (rtspServer) {
        rtspServer->SendNalu(nChn, pBuf, nLen, nPts, bIFrame);

        bRet = AX_TRUE;
    }

    return bRet ? 0 : -1;
}

AX_S32 AX_Rtsp_SendAudio(AX_RTSP_HANDLE pHandle, AX_U32 nChn, const AX_U8* pBuf, AX_U32 nLen, AX_U64 nPts)
{
    AX_BOOL bRet = AX_FALSE;

    AXRtspServer *rtspServer = (AXRtspServer *)pHandle;

    if (rtspServer) {
        rtspServer->SendAudio(nChn, pBuf, nLen, nPts);

        bRet = AX_TRUE;
    }

    return bRet ? 0 : -1;
}
