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
#include <mutex>
#include "AXEvent.hpp"
#include "AXLockQ.hpp"
#include "AXRTSPClient.h"
#include "AXThread.hpp"
#include "IStreamHandler.hpp"
#include "rtspdamon.hpp"

typedef struct {
    AX_U8 *pData;
    AX_U32 nSize;
} STREAM_DATA_T;

class CRtspStreamer : public CStreamBaseHander {
public:
    CRtspStreamer(AX_VOID) = default;
    virtual ~CRtspStreamer(AX_VOID) = default;

    AX_BOOL Init(const STREAMER_ATTR_T &stAttr) override;
    AX_BOOL DeInit(AX_VOID) override;

    AX_BOOL Start(AX_VOID) override;
    AX_BOOL Stop(AX_VOID) override;

    AX_BOOL ReConnect(AX_VOID);

protected:
    AX_VOID EventLoopThread(AX_VOID *pArg);
    AX_VOID DispatchThread(AX_VOID *pArg);
    AX_VOID ClearCacheQ(AX_VOID);

    /* rtsp client callback */
    void OnRecvFrame(const void *session, const unsigned char *pFrame, unsigned nSize, AX_PAYLOAD_TYPE_E ePayload, STREAM_NALU_TYPE_E eNalu, struct timeval tv);
    void OnTracksInfo(const TRACKS_INFO_T &tracks);
    void OnPreparePlay(void);
    void OnCheckAlive(int resultCode, const char *resultString);

private:
    TaskScheduler *m_scheduler{nullptr};
    UsageEnvironment *m_env{nullptr};
    CAXRTSPClient *m_client{nullptr};
    CAXThread m_EventThread;
    CAXEvent m_InitEvent;
    CAXEvent m_PlayEvent;
    volatile char m_cExitThread{1};
    CAXLockQ<STREAM_DATA_T> m_cacheQ;
    CAXThread m_DispatchThread;
    AX_S32 m_nCachedCount{0};
    AX_S32 m_nCacheTime{0};
    std::unique_ptr<CRtspDamon> m_damon;
    STREAMER_ATTR_T m_stAttr;
};
