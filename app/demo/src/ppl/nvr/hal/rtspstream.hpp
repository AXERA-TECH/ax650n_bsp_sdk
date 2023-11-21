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
#include "AXEvent.hpp"
#include "AXLockQ.hpp"
#include "AXRTSPClient.h"
#include "AXThread.hpp"
#include "istream.hpp"
#include "rtspdamon.hpp"

class CRtspStream : public CStream0 {
public:
    CRtspStream(AX_VOID) = default;

    AX_BOOL Init(CONST STREAM_ATTR_T& stAttr) override;
    AX_BOOL DeInit(AX_VOID) override;

    AX_BOOL Start(AX_VOID) override;
    AX_BOOL Stop(AX_VOID) override;

protected:
    AX_BOOL ReConnect(AX_VOID);
    AX_VOID OnConnectStatusReport(AX_S32 nStatus);

    /* rtsp client callback */
    void OnRecvFrame(const void* session, const unsigned char* pFrame, unsigned nSize, AX_PAYLOAD_TYPE_E ePayload, STREAM_NALU_TYPE_E eNalu, struct timeval tv);
    void OnTracksInfo(const TRACKS_INFO_T& tracks);
    void OnPreparePlay(void);
    void OnCheckAlive(int resultCode, const char* resultString);

protected:
    TaskScheduler* m_scheduler = {nullptr};
    UsageEnvironment* m_env = {nullptr};
    CAXRTSPClient* m_client = {nullptr};

    CAXThread m_EventLoopThread;
    CAXThread m_DamonThread;
    CAXEvent m_InitEvent;
    CAXEvent m_PlayEvent;
    volatile char m_cExitThread = {1};
    std::unique_ptr<CRtspDamon> m_damon;
};
