/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AXRTSPSERVER_H__
#define __AXRTSPSERVER_H__

#include "ax_base_type.h"
#include "ax_global_type.h"
#include "AXRtspWrapper.h"
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "AXLiveServerMediaSession.h"

#define MAX_RTSP_CHANNEL_NUM (8)

typedef enum _RTSP_MEDIA_TYPE_E {
    RTSP_MEDIA_VIDEO,
    RTSP_MEDIA_AUDIO,
    RTSP_MEDIA_BUTT
} RTSP_MEDIA_TYPE_E;

typedef struct _RTSP_ATTR {
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

    _RTSP_ATTR() {
        memset(this, 0x00, sizeof(axRTSP_ATTR));
    }
} RTSP_ATTR_T;

class AXRtspServer{
public:
    AXRtspServer(void);
    virtual ~AXRtspServer(void);

public:
    AX_BOOL Init(const RTSP_ATTR_T *pstAttr, AX_S32 nNum, AX_U16 uBasePort = 0);
    AX_BOOL Start(void);
    AX_VOID Stop(void);
    AX_VOID SendNalu(AX_U8 nChn, const AX_U8* pBuf, AX_U32 nLen, AX_U64 nPts = 0, AX_BOOL bIFrame = AX_FALSE);
	AX_VOID SendAudio(AX_U32 nChn, const AX_U8* pBuf, AX_U32 nLen, AX_U64 nPts);

public:
    RTSPServer*               m_pRtspServer;
    RTSP_ATTR_T               m_stRtspAttr[MAX_RTSP_CHANNEL_NUM];
    AX_S32                    m_nMaxNum;
    AX_U16                    m_uBasePort;

    AXLiveServerMediaSession* m_pLiveServerMediaSession[MAX_RTSP_CHANNEL_NUM][RTSP_MEDIA_BUTT];
    UsageEnvironment*         m_pUEnv;

private:
    pthread_t m_tidServer;

};

#endif /*__AXRTSPSERVER_H__*/
