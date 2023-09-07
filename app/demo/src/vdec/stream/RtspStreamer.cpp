/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "RtspStreamer.hpp"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <chrono>
#include "AppLogApi.h"
#include "SpsParser.hpp"

using namespace std;
using namespace std::placeholders;
#define CLIENT "CLIENT"

AX_VOID CRtspStreamer::EventLoopThread(AX_VOID *pArg) {
    m_cExitThread = 0;
    m_env->taskScheduler().doEventLoop(&m_cExitThread);
    UpdateStatus(AX_FALSE);
}

AX_VOID CRtspStreamer::DispatchThread(AX_VOID *pArg) {
    AX_BOOL bCaching = {AX_TRUE};
    const AX_S32 nCookie = m_stInfo.nCookie;
    LOG_M_C(CLIENT, "waiting client %3d caching frames for %d frames", nCookie, m_nCachedCount);
    while (m_DispatchThread.IsRunning()) {
        if (bCaching) {
            if (m_cacheQ.GetCount() >= m_nCachedCount) {
                LOG_M_C(CLIENT, "client %d has cached over %d frames, start to decode", nCookie, m_nCachedCount);
                bCaching = AX_FALSE;
            } else {
                this_thread::sleep_for(chrono::milliseconds(1));
                continue;
            }
        }

        STREAM_DATA_T data;
        if (m_cacheQ.Pop(data, -1)) {
            for (auto &&m : m_lstObs) {
                m->OnRecvVideoData(nCookie, data.pData, data.nSize, 0);
            }

            free(data.pData);
        }
    }
}

void CRtspStreamer::OnRecvFrame(const void *session, const unsigned char *pFrame, unsigned nSize, AX_PAYLOAD_TYPE_E, STREAM_NALU_TYPE_E, struct timeval) {
    if (m_stInfo.session != session) {
        /* ignore other tracks except for video */
        return;
    }

    ++m_stStat.nCount;

    if (0 == m_nCachedCount) {
        for (auto &&m : m_lstObs) {
            m->OnRecvVideoData(m_stInfo.nCookie, pFrame, nSize, 0);
        }
    } else {
        STREAM_DATA_T data;
        data.nSize = nSize;
        data.pData = (AX_U8 *)malloc(nSize); /* fixme: how to avoid malloc ? */
        if (data.pData) {
            memcpy(data.pData, pFrame, nSize);
            if (!m_cacheQ.Push(data)) {
                free(data.pData);
                // LOG_M_E(CLIENT, "client %d data q is full", m_stInfo.nCookie);
            }
        } else {
            LOG_M_E(CLIENT, "client %d malloc %d fail, %s", m_stInfo.nCookie, nSize, strerror(errno));
        }
    }
}

void CRtspStreamer::OnTracksInfo(const TRACKS_INFO_T &tracks) {
    if (0 == tracks.tracks.size()) {
        LOG_M_E(CLIENT, "%s: no tracks", m_stInfo.strPath.c_str());
        return;
    }

    for (auto &&kv : tracks.tracks) {
        const TRACK_INFO_T &track = kv.second;
        switch (track.enPayload) {
            case PT_H264:
            case PT_H265: {
                m_stInfo.session = kv.first;
                m_stInfo.eVideoType = track.enPayload;

                SPS_INFO_T sps;
                memset(&sps, 0, sizeof(sps));
                (PT_H264 == track.enPayload) ? h264_parse_sps(track.video.sps[0], track.video.len[0], &sps)
                                             : hevc_parse_sps(track.video.sps[0], track.video.len[0], &sps);

                if (sps.width > 0 && sps.height > 0) {
                    m_stInfo.nWidth = sps.width;
                    m_stInfo.nHeight = sps.height;
                }

                if (sps.fps > 0) {
                    m_stInfo.nFps = sps.fps;
                }

                LOG_M_C(CLIENT, "%s: %s payload %d, %dx%d %dfps", tracks.url, track.control, track.rtpPayload, sps.width, sps.height,
                        sps.fps);
            } break;

            default:
                LOG_M_C(CLIENT, "%s: %s payload %d", tracks.url, track.control, track.rtpPayload);
                break;
        }
    }

    m_InitEvent.SetEvent();
}

void CRtspStreamer::OnPreparePlay(void) {
    // LOG_M_W(CLIENT, "%s client %d WaitEvent +++", __func__, m_stInfo.nCookie);
    m_nCachedCount = (AX_U32)((m_nCacheTime / 1000.0) * m_stInfo.nFps);
    if (m_nCachedCount > 0) {
        /* capacity must be set, otherwise if vdec hung, always malloc and push will cause oom */
        m_cacheQ.SetCapacity(m_nCachedCount * 2);

        AX_CHAR szName[32];
        sprintf(szName, "AppRtspcRecv%d", m_stInfo.nCookie);
        if (!m_DispatchThread.Start([this](AX_VOID *pArg) -> AX_VOID { DispatchThread(pArg); }, this, szName)) {
            LOG_M_E(CLIENT, "%s: start rtsp recv thread of client %d fail", __func__, m_stInfo.nCookie);
        }
    }

    m_PlayEvent.WaitEvent();
    // LOG_M_W(CLIENT, "%s client %d WaitEvent ---", __func__, m_stInfo.nCookie);
}

AX_BOOL CRtspStreamer::Init(const STREAMER_ATTR_T &stAttr) {
    LOG_M_D(CLIENT, "%s: client %d +++", __func__, stAttr.nCookie);

    m_stAttr = stAttr;
    m_nCacheTime = stAttr.nCacheTime;
    m_nCachedCount = 0;

    m_stInfo.strPath = stAttr.strPath;
    m_stInfo.eStreamType = STREAM_TYPE_E::RTSP;
    m_stInfo.nCookie = stAttr.nCookie;
    m_stInfo.nWidth = stAttr.nMaxWidth;
    m_stInfo.nHeight = stAttr.nMaxHeight;
    m_stInfo.nFps = 25; /* default fps */

    m_stStat.bStarted = AX_FALSE;
    m_stStat.nCount = 0;

    AX_S32 nVerbosityLevel = 0;
    AX_CHAR *pEnv = getenv("RTSP_CLIENT_LOG");
    if (pEnv) {
        nVerbosityLevel = atoi(pEnv);
    }

    m_InitEvent.ResetEvent();
    m_PlayEvent.ResetEvent();

    RtspClientCallback cb;
    cb.OnRecvFrame = bind(&CRtspStreamer::OnRecvFrame, this, _1, _2, _3, _4, _5, _6);
    cb.OnTracksInfo = bind(&CRtspStreamer::OnTracksInfo, this, _1);
    cb.OnPreparePlay = bind(&CRtspStreamer::OnPreparePlay, this);
    cb.OnCheckAlive = std::bind(&CRtspStreamer::OnCheckAlive, this, std::placeholders::_1, std::placeholders::_2);

    try {
        m_scheduler = BasicTaskScheduler::createNew();
        m_env = BasicUsageEnvironment::createNew(*m_scheduler);

        // Begin by creating a "RTSPClient" object.  Note that there is a separate "RTSPClient" object for each stream that we wish
        // to receive (even if more than stream uses the same "rtsp://" URL).
        m_client = CAXRTSPClient::createNew(*m_env, m_stInfo.strPath.c_str(), cb, m_stInfo.nWidth * m_stInfo.nHeight, nVerbosityLevel,
                                            "AxRtspClient", 0);
        m_client->scs.keepAliveInterval = 10;

        char *env = getenv("RTP_TRANSPORT_MODE");
        if (env) {
            m_client->scs.streamTransportMode = ((1 != atoi(env)) ? 0 : 1);
            LOG_M_I(CLIENT, "client %d RTP transport mode: %d", m_stInfo.nCookie, m_client->scs.streamTransportMode);
        }

        m_client->Start();

        AX_CHAR szName[32];
        sprintf(szName, "AppRtspClient%d", m_stInfo.nCookie);
        if (!m_EventThread.Start([this](AX_VOID *pArg) -> AX_VOID { EventLoopThread(pArg); }, this, szName)) {
            m_client->Stop();
            m_client = nullptr;

            m_env->reclaim();
            m_env = nullptr;

            delete m_scheduler;
            m_scheduler = nullptr;

            LOG_M_E(CLIENT, "%s: start rtsp client thread of client %d fail", __func__, m_stInfo.nCookie);
            return AX_FALSE;
        }

        if (!m_damon) {
            RTSP_DAMON_ATTR_T stDamon;
            stDamon.strUrl = stAttr.strPath;
            stDamon.nKeepAliveInterval = m_client->scs.keepAliveInterval + 1; /* margin to damon wait_for to avoid missing condition */
            stDamon.nReconnectThreshold = 3;
            stDamon.reconnect = std::bind(&CRtspStreamer::ReConnect, this);
            m_damon.reset(CRtspDamon::CreateInstance(stDamon));
        }

        /* wait SDP is received to parse stream info in 5 seconds */
        if (!m_InitEvent.WaitEvent(5000)) {
            m_EventThread.Stop();
            m_cExitThread = 1;
            m_EventThread.Join();

            m_client->Stop();
            m_client = nullptr;

            if (m_env) {
                m_env->reclaim();
                m_env = nullptr;
            }

            if (m_scheduler) {
                delete m_scheduler;
                m_scheduler = nullptr;
            }

            return AX_FALSE;
        }

    } catch (bad_alloc &e) {
        LOG_M_E(CLIENT, "%s: create client %d fail", __func__, m_stInfo.nCookie);
        DeInit();
        return AX_FALSE;
    }

    LOG_M_D(CLIENT, "%s: client %d ---", __func__, m_stInfo.nCookie);
    return AX_TRUE;
}

AX_BOOL CRtspStreamer::DeInit(AX_VOID) {
    LOG_M_D(CLIENT, "%s: client %d +++", __func__, m_stInfo.nCookie);

    // if (m_EventThread.IsRunning()) {
    //     LOG_M_E(CLIENT, "%s: client %d is still running", __func__, m_stInfo.nCookie);
    //     return AX_FALSE;
    // }

    Stop();

    if (m_env) {
        m_env->reclaim();
        m_env = nullptr;
    }

    if (m_scheduler) {
        delete m_scheduler;
        m_scheduler = nullptr;
    }

    LOG_M_D(CLIENT, "%s: stream %d ---", __func__, m_stInfo.nCookie);
    return AX_TRUE;
}

AX_BOOL CRtspStreamer::Start(AX_VOID) {
    LOG_M_D(CLIENT, "%s: client %d +++", __func__, m_stInfo.nCookie);

    if (m_damon) {
        m_damon->Start();
    }

    m_PlayEvent.SetEvent();
    UpdateStatus(AX_TRUE);

    LOG_M_D(CLIENT, "%s: client %d ---", __func__, m_stInfo.nCookie);
    return AX_TRUE;
}

AX_BOOL CRtspStreamer::Stop(AX_VOID) {
    LOG_M_D(CLIENT, "%s: client %d +++", __func__, m_stInfo.nCookie);

    if (m_damon) {
        m_damon->Stop();
    }

    m_EventThread.Stop();
    /* if Init ok, but start is not invoked, then we need wakeup play event */
    m_PlayEvent.SetEvent();
    m_cExitThread = 1;
    m_EventThread.Join();

    if (m_client) {
        m_client->Stop();
        m_client = nullptr;
    }

    m_DispatchThread.Stop();
    m_cacheQ.Wakeup();
    m_DispatchThread.Join();

    ClearCacheQ();

    // LOG_M_I(CLIENT, "client %d has sent total %lld frames", m_stInfo.nCookie, m_tStatus.nCount);
    LOG_M_D(CLIENT, "%s: client %d ---", __func__, m_stInfo.nCookie);
    return AX_TRUE;
}

AX_VOID CRtspStreamer::ClearCacheQ(AX_VOID) {
    const AX_U32 nCount = m_cacheQ.GetCount();
    if (nCount > 0) {
        STREAM_DATA_T data;
        for (AX_U32 i = 0; i < nCount; ++i) {
            if (m_cacheQ.Pop(data, 0)) {
                free(data.pData);
            }
        }
    }
}

void CRtspStreamer::OnCheckAlive(int resultCode, const char *resultString) {
    if (0 == resultCode) {
        // LOG_M_C(TAG, "%s: resultCode = %d", __func__, resultCode);
        if (m_damon) {
            m_damon->KeepAlive();
        }
    } else {
        if (resultString) {
            LOG_M_E(CLIENT, "%s: resultCode = %d, %s", __func__, resultCode, resultString);
        } else {
            LOG_M_E(CLIENT, "%s: resultCode = %d", __func__, resultCode);
        }
    }
}

AX_BOOL CRtspStreamer::ReConnect(AX_VOID) {
    if (m_env && m_client) {
        m_env->taskScheduler().unscheduleDelayedTask(m_client->scs.streamTimerTask);
    }

    m_EventThread.Stop();
    m_cExitThread = 1;
    m_EventThread.Join();

    if (m_client) {
        m_client->Stop();
        m_client = nullptr;
    }

    m_DispatchThread.Stop();
    m_cacheQ.Wakeup();
    m_DispatchThread.Join();
    ClearCacheQ();

    m_stStat.bStarted = AX_FALSE;

    if (m_env) {
        m_env->reclaim();
        m_env = nullptr;
    }

    if (m_scheduler) {
        delete m_scheduler;
        m_scheduler = nullptr;
    }

    if (!Init(m_stAttr)) {
        return AX_FALSE;
    }

    m_PlayEvent.SetEvent();
    m_stStat.bStarted = AX_TRUE;

    return AX_TRUE;
}
