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
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include "AXThread.hpp"

typedef struct RTSP_DAMON_ATTR_S {
    std::string strUrl;         /* rtsp url, rtsp://192.168.2.10:8554/stream1 */
    AX_S32 nKeepAliveInterval;  /* heart beat interval time in seconds */
    AX_S32 nReconnectThreshold; /* if the count of heart beat absent from server is >= nReconnThreshold, trigger to reconnect */
    std::function<AX_BOOL(AX_VOID)> reconnect;   /* reconnect to rtsp server */
    std::function<AX_VOID(AX_S32)> statusReport; /* 0: disconnect, 1: reconnect, 2: connected */

    RTSP_DAMON_ATTR_S(AX_VOID) {
        nKeepAliveInterval = 10;
        nReconnectThreshold = 3;
        reconnect = nullptr;
        statusReport = nullptr;
    }
} RTSP_DAMON_ATTR_T;

class CRtspDamon {
public:
    static CRtspDamon* CreateInstance(const RTSP_DAMON_ATTR_T& stAttr);

    AX_BOOL Start(AX_VOID);
    AX_BOOL Stop(AX_VOID);

    AX_VOID KeepAlive(AX_VOID);

protected:
    CRtspDamon(const RTSP_DAMON_ATTR_T& stAttr);

    AX_VOID DamonThread(AX_VOID* pArg);
    AX_VOID WaitConnected(AX_VOID);
    AX_BOOL IsAlive(AX_S32 nTimeOut);
    AX_VOID ReportConnectStatus(AX_S32 nStatus);
    AX_VOID DiscConnect(AX_VOID);

private:
    RTSP_DAMON_ATTR_T m_stAttr;
    std::condition_variable m_cv;
    AX_BOOL m_bAlive = {AX_FALSE};
    std::mutex m_mtx;
    CAXThread m_thread;
    std::atomic<AX_BOOL> m_bStarted = {AX_FALSE};
};

inline AX_VOID CRtspDamon::ReportConnectStatus(AX_S32 nStatus) {
    if (m_stAttr.statusReport) {
        m_stAttr.statusReport(nStatus);
    }
}