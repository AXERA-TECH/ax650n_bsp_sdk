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

#include <thread>
#include <mutex>
#include <condition_variable>
#include "AXLiveServerMediaSession.h"
#include "AXSingleton.h"
#include "BasicUsageEnvironment.hh"
#include "IModule.h"
#include "liveMedia.hh"

#define MAX_RTSP_CHANNEL_NUM (20)

typedef enum axRTSP_SESS_MEDIA_TYPE_E {
    RTSP_SESS_MEDIA_VIDEO,
    RTSP_SESS_MEDIA_AUDIO,
    RTSP_SESS_MEDIA_BUTT
} RTSP_SESS_MEDIA_TYPE_E;

typedef struct axRTSP_SESS_ATTR {
    AX_U32 nChannel;
    struct rstp_video_sess_attr {
        AX_BOOL bEnable;
        AX_PAYLOAD_TYPE_E ePt;
        AX_U32 nMaxFrmSize;
        AX_U32 nBitRate;  // kpbs
    } stVideoAttr;
    struct rstp_audio_sess_attr {
        AX_BOOL bEnable;
        AX_PAYLOAD_TYPE_E ePt;
        AX_U32 nMaxFrmSize;
        AX_U32 nBitRate;  // kpbs
        AX_U32 nSampleRate;
        AX_S32 nAOT; // audio object type
        AX_U8 nChnCnt;
    } stAudioAttr;

    axRTSP_SESS_ATTR() {
        memset(this, 0x00, sizeof(axRTSP_SESS_ATTR));
    }
} RTSP_SESS_ATTR_T;

using namespace std;
class CAXRtspServer : public CAXSingleton<CAXRtspServer>, public IModule {
    friend class CAXSingleton<CAXRtspServer>;

public:
    virtual AX_BOOL Init() override;
    virtual AX_BOOL DeInit() override;
    virtual AX_BOOL Start() override;
    virtual AX_BOOL Stop() override;

    AX_BOOL SendNalu(AX_U32 nChannel, AX_VOID* pData, AX_U32 nLen, AX_U64 nPTS, AX_BOOL bIFrame = AX_FALSE);
    AX_BOOL SendAudio(AX_U32 nChannel, AX_VOID* pData, AX_U32 nLen, AX_U64 nPTS);
    AX_BOOL AddSessionAttr(AX_U32 nChannel, const RTSP_SESS_ATTR_T &stSessAttr);
    AX_BOOL UpdateSessionAttr(AX_U32 nChannel, const RTSP_SESS_ATTR_T &stSessAttr);
    AX_VOID RestartSessions(AX_VOID);

private:
    CAXRtspServer(AX_VOID) = default;
    virtual ~CAXRtspServer(AX_VOID) = default;

    AX_VOID RtspServerThreadFunc();
    AX_VOID StopRtspServerThread();
    AX_VOID DoRestartSessions(AX_VOID);

private:
    vector<RTSP_SESS_ATTR_T> m_vecMediaSessionAttr;
    /* Valid media session during progress may be not continously, based on VENC's channel id */
    AXLiveServerMediaSession* m_pMediaSession[MAX_RTSP_CHANNEL_NUM][RTSP_SESS_MEDIA_BUTT]{nullptr};
    UsageEnvironment* m_pUEnv{nullptr};
    RTSPServer* m_rtspServer{nullptr};
    AX_CHAR m_chStopEventLoop{0};

    thread* m_pServerThread{nullptr};
    AX_BOOL m_bServerThreadWorking{AX_FALSE};

    std::mutex m_mtxSessions;
    std::condition_variable m_cvSessions;
    AX_BOOL m_bNeedRestartSessions{AX_FALSE};
};
