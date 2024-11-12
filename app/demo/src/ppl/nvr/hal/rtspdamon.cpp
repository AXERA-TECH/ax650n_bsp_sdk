/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "rtspdamon.hpp"
#include <chrono>
#include <exception>
#include <iostream>
#include "AppLogApi.h"

extern int ping4(const char* ip, int timeout);
#define TAG "DAMON"

AX_VOID CRtspDamon::DamonThread(AX_VOID* /* pArg */) {
    LOG_M_I(TAG, "%s: %s +++", __func__, m_stAttr.strUrl.c_str());

    AX_S32 nLossCnt = {-1};
    AX_BOOL bNetDown = AX_FALSE;

    while (1) {
        if (!m_thread.IsRunning()) {
            break;
        }

        if (nLossCnt < 0) {
            WaitConnected();
            nLossCnt = 0;
            continue;
        }

        if (IsAlive(m_stAttr.nKeepAliveInterval)) {
            nLossCnt = 0;
        } else {
            ++nLossCnt;
            LOG_MM_W(TAG, "[%d] %d heart beats are lossed from %s", m_stAttr.nCookie, nLossCnt, m_stAttr.strUrl.c_str());
        }

        if (!m_thread.IsRunning()) {
            break;
        }

        if (nLossCnt >= m_stAttr.nReconnectThreshold) {
            DiscConnect();
            nLossCnt = -1;

            ReportConnectStatus(1);
            LOG_MM_W(TAG, "[%d] disconnect with %s, checking network ...", m_stAttr.nCookie, m_stAttr.strUrl.c_str());
            while (m_thread.IsRunning()) {
                if (0 == ping4(m_stAttr.strUrl.c_str(), 2)) {
                    bNetDown = AX_FALSE;
                    break;
                } else {
                    if (!bNetDown) {
                        LOG_MM_W(TAG, "[%d] network to %s is down", m_stAttr.nCookie, m_stAttr.strUrl.c_str());
                        bNetDown = AX_TRUE;
                    }
                }

                /* if network is down, delay to wait network stable */
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            AX_BOOL bConn = {AX_FALSE};
            AX_S32 i = {0};
            while (m_thread.IsRunning()) {
                LOG_MM_W(TAG, "[%d] network is ok, reconnecting %d to %s ...", m_stAttr.nCookie, ++i, m_stAttr.strUrl.c_str());
                if (m_stAttr.reconnect()) {
                    bConn = AX_TRUE;
                    break;
                } else {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            };

            if (!bConn) {
                ReportConnectStatus(0);
                LOG_MM_E(TAG, "[%d] reconnect to %s fail", m_stAttr.nCookie, m_stAttr.strUrl.c_str());
                break;
            }
        }
    }

    LOG_M_I(TAG, "%s: %s ---", __func__, m_stAttr.strUrl.c_str());
}

CRtspDamon* CRtspDamon::CreateInstance(const RTSP_DAMON_ATTR_T& stAttr) {
    if (stAttr.strUrl.empty()) {
        LOG_M_E(TAG, "invalid parameter: url");
        return nullptr;
    }

    if (stAttr.nKeepAliveInterval <= 0) {
        LOG_M_E(TAG, "invalid parameter: heart beat interval time %d", stAttr.nKeepAliveInterval);
        return nullptr;
    }

    if (stAttr.nReconnectThreshold <= 0) {
        LOG_M_E(TAG, "invalid parameter: reconnect threshold %d", stAttr.nReconnectThreshold);
        return nullptr;
    }

    if (!stAttr.reconnect) {
        LOG_M_E(TAG, "invalid parameter: nil reconnect function");
        return nullptr;
    }

    return new (std::nothrow) CRtspDamon(stAttr);
}

CRtspDamon::CRtspDamon(const RTSP_DAMON_ATTR_T& stAttr) : m_stAttr(stAttr) {
}

AX_BOOL CRtspDamon::Start(AX_VOID) {
    if (m_bStarted) {
        LOG_M_W(TAG, "damon thread of %s is already started", m_stAttr.strUrl.c_str());
        return AX_FALSE;
    }

    LOG_M_I(TAG, "%s: %s +++", __func__, m_stAttr.strUrl.c_str());

    char szName[32];
    sprintf(szName, "rtspDamon%d", m_stAttr.nCookie);
    if (!m_thread.Start([this](AX_VOID* pArg) -> AX_VOID { DamonThread(pArg); }, this, szName)) {
        LOG_MM_E(TAG, "[%d] start damon thread of %s fail", m_stAttr.nCookie, m_stAttr.strUrl.c_str());
        return AX_FALSE;
    }

    m_bStarted = AX_TRUE;

    LOG_M_I(TAG, "%s: %s ---", __func__, m_stAttr.strUrl.c_str());
    return AX_TRUE;
}

AX_BOOL CRtspDamon::Stop(AX_VOID) {
    if (!m_bStarted) {
        return AX_TRUE;
    }

    LOG_M_I(TAG, "%s: %s +++", __func__, m_stAttr.strUrl.c_str());

    m_thread.Stop();
    m_cv.notify_one();
    m_thread.Join();

    m_bStarted = AX_FALSE;

    LOG_M_I(TAG, "%s: %s ---", __func__, m_stAttr.strUrl.c_str());
    return AX_TRUE;
}

AX_VOID CRtspDamon::KeepAlive(AX_VOID) {
    if (!m_bStarted) {
        return;
    }

    {
        std::lock_guard<std::mutex> lck(m_mtx);
        if (!m_bAlive) {
            ReportConnectStatus(2);
        }

        m_bAlive = AX_TRUE;
    }

    m_cv.notify_one();
}

AX_VOID CRtspDamon::DiscConnect(AX_VOID) {
    std::lock_guard<std::mutex> lck(m_mtx);
    m_bAlive = AX_FALSE;
}

AX_VOID CRtspDamon::WaitConnected(AX_VOID) {
    std::unique_lock<std::mutex> lck(m_mtx);
    while (!m_bAlive && m_thread.IsRunning()) {
        m_cv.wait(lck);
    }
}

AX_BOOL CRtspDamon::IsAlive(AX_S32 nTimeOut) {
    std::unique_lock<std::mutex> lck(m_mtx);
    return (std::cv_status::timeout == m_cv.wait_for(lck, std::chrono::seconds(nTimeOut))) ? AX_FALSE : AX_TRUE;
}
