/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "rtspstream.hpp"
#include <exception>
#include <functional>
#include "AppLogApi.h"
#include "SpsParser.hpp"
#include "h264.hpp"
#include "hevc.hpp"
#if defined(__RTSP_PTS__)
#include "ax_sys_api.h"
#endif

#define TAG "RTSP"

AX_BOOL CRtspStream::Init(CONST STREAM_ATTR_T &stAttr) {
    m_stStat.Reset();

    m_InitEvent.ResetEvent();
    m_PlayEvent.ResetEvent();

    RtspClientCallback cb;
    cb.OnRecvFrame = std::bind(&CRtspStream::OnRecvFrame, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                               std::placeholders::_4, std::placeholders::_5, std::placeholders::_6);
    cb.OnTracksInfo = std::bind(&CRtspStream::OnTracksInfo, this, std::placeholders::_1);
    cb.OnPreparePlay = std::bind(&CRtspStream::OnPreparePlay, this);
    cb.OnCheckAlive = std::bind(&CRtspStream::OnCheckAlive, this, std::placeholders::_1, std::placeholders::_2);

    try {
        m_scheduler = BasicTaskScheduler::createNew();
        m_env = BasicUsageEnvironment::createNew(*m_scheduler);
        m_client = CAXRTSPClient::createNew(*m_env, stAttr.strURL.c_str(), cb, stAttr.nMaxBufSize, stAttr.nDebugLevel, "RTSPClient", 0);
        m_client->scs.streamTransportMode = (RTP_OVER_TCP == stAttr.enTransportMode) ? 1 : 0; /* 0: UDP 1: TCP */
        m_client->scs.keepAliveInterval = stAttr.nKeepAliveInterval;
        m_client->Start();

        if (!m_EventLoopThread.Start(
                [this](AX_VOID *) -> AX_VOID {
                    m_cExitThread = 0;
                    m_env->taskScheduler().doEventLoop(&m_cExitThread);
                },
                this, "RtspEvent")) {
            LOG_M_E(TAG, "%s: start %s event loop fail", __func__, stAttr.strURL.c_str());

            /* client will be deleted by continueAfterDESCRIBE() -> shutdownStream() */
            m_client->Stop();
            m_client = nullptr;

            m_env->reclaim();
            m_env = nullptr;

            delete m_scheduler;
            m_scheduler = nullptr;

            return AX_FALSE;
        }

        m_stAttr = stAttr;
        m_stInfo.strURL = stAttr.strURL;

        if (!m_damon) {
            RTSP_DAMON_ATTR_T stDamon;
            stDamon.strUrl = stAttr.strURL;
            stDamon.nKeepAliveInterval = stAttr.nKeepAliveInterval + 1; /* margin to damon wait_for to avoid missing condition */
            stDamon.nReconnectThreshold = stAttr.nReconnectThreshold;
            stDamon.reconnect = std::bind(&CRtspStream::ReConnect, this);
            stDamon.statusReport = std::bind(&CRtspStream::OnConnectStatusReport, this, std::placeholders::_1);
            m_damon.reset(CRtspDamon::CreateInstance(stDamon));
        }

        /* wait SDP is received to parse stream info in 5 second */
        if (!m_InitEvent.WaitEvent(10000)) {
            LOG_M_E(TAG, "recv sdp from %s timeout", stAttr.strURL.c_str());
            m_EventLoopThread.Stop();
            m_cExitThread = 1;
            m_EventLoopThread.Join();

            /* client will be deleted by continueAfterDESCRIBE() -> shutdownStream() */
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

    } catch (std::bad_alloc &e) {
        LOG_M_E(TAG, "%s: setup %s fail", __func__, stAttr.strURL.c_str());
        DeInit();
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CRtspStream::DeInit(AX_VOID) {
    // if (m_EventLoopThread.IsRunning()) {
    //     LOG_M_E(TAG, "%s: %s is still running", __func__, m_stInfo.strURL.c_str());
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

    return AX_TRUE;
}

AX_BOOL CRtspStream::Start(AX_VOID) {
    LOG_M_D(TAG, "%s: %s +++", __func__, m_stAttr.strURL.c_str());

    if (nullptr == m_scheduler || nullptr == m_env || nullptr == m_damon) {
        LOG_MM_E(TAG, "RTSP stream start failed, please do initialization first.");
        return AX_FALSE;
    }

    m_stStat.nState = 1;
    for (auto &&m : m_stStat.nCount) {
        m = 0;
    }

    m_PlayEvent.SetEvent();

    if (!m_client->CheckPlayed(10000)) {
        LOG_M_E(TAG, "%s: %s play fail", __func__, m_stAttr.strURL.c_str());

        Stop();
        return AX_FALSE;
    }

    if (m_damon) {
        m_damon->Start();
    }

    LOG_M_D(TAG, "%s: %s ---", __func__, m_stAttr.strURL.c_str());
    return AX_TRUE;
}

AX_BOOL CRtspStream::Stop(AX_VOID) {
    LOG_M_D(TAG, "%s: %s +++", __func__, m_stAttr.strURL.c_str());

    if (m_damon) {
        m_damon->Stop();
    }

    m_EventLoopThread.Stop();
    /* if Init ok, but start is not invoked, then we need wakeup play event */
    m_PlayEvent.SetEvent();
    m_cExitThread = 1;
    m_EventLoopThread.Join();

    if (m_client) {
        m_client->Stop();
        m_client = nullptr;
    }

    m_stStat.nState = 0;

    LOG_M_D(TAG, "%s: %s ---", __func__, m_stAttr.strURL.c_str());
    return AX_TRUE;
}

void CRtspStream::OnRecvFrame(const void *session, const unsigned char *pFrame, unsigned nSize, AX_PAYLOAD_TYPE_E ePayload,
                              STREAM_NALU_TYPE_E eNalu, struct timeval /* tv */) {
    auto it = m_stInfo.tracks.find((AX_VOID *)session);
    if (it == m_stInfo.tracks.end()) {
        return;
    }

    if (PT_H264 == ePayload || PT_H265 == ePayload) {
        ++m_stStat.nCount[0];

        STREAM_FRAME_T stFrame;
        stFrame.enPayload = ePayload;
        stFrame.nPrivData = 0;
        stFrame.frame.stVideo.stInfo = it->second.info.stVideo;
        stFrame.frame.stVideo.enNalu = eNalu;
        stFrame.frame.stVideo.pData = const_cast<AX_U8 *>(pFrame);
        stFrame.frame.stVideo.nLen = nSize;
        stFrame.frame.stVideo.nPTS = 0; /* let VO to make PTS for rtsp preview */
        stFrame.frame.stVideo.bSkipDisplay = AX_FALSE;

        std::lock_guard<std::mutex> lck(m_mtxObs);
        for (auto &&m : m_lstObs) {
            m->OnRecvStreamData(stFrame);
        }
    }
}

void CRtspStream::OnTracksInfo(const TRACKS_INFO_T &tracks) {
    if (0 == tracks.tracks.size()) {
        LOG_M_E(TAG, "%s: no tracks", m_stInfo.strURL.c_str());
        return;
    }

    m_stInfo.tracks.clear();

    for (auto &&kv : tracks.tracks) {
        switch (kv.second.enPayload) {
            case PT_H264:
            case PT_H265: {
                STREAM_TRACK_INFO_T track;
                track.enPayload = kv.second.enPayload;
                track.info.stVideo.enPayload = track.enPayload;

                SPS_INFO_T sps;
                memset(&sps, 0, sizeof(sps));
                (PT_H264 == track.enPayload) ? h264_parse_sps(kv.second.video.sps[0], kv.second.video.len[0], &sps)
                                             : hevc_parse_sps(kv.second.video.sps[0], kv.second.video.len[0], &sps);

                track.info.stVideo.nProfile = sps.profile_idc;
                track.info.stVideo.nLevel = sps.level_idc;
                track.info.stVideo.nWidth = sps.width;
                track.info.stVideo.nHeight = sps.height;
                track.info.stVideo.nFps = sps.fps;
                track.info.stVideo.nNumRefs = sps.num_ref_frames;

                m_stInfo.tracks[kv.first] = track;
                m_stInfo.stVideo = track.info.stVideo;

                LOG_M_N(TAG, "%s: %s payload %d, profile %d level %d, num_ref_frames %, %dx%d %dfps", tracks.url, kv.second.control,
                        kv.second.rtpPayload, sps.profile_idc, sps.level_idc, sps.num_ref_frames, sps.width, sps.height, sps.fps);
            } break;

            case PT_G711A:
            case PT_G711U:
            case PT_AAC: {
                // m_stInfo.tracks[kv.first] = track;
            } break;

            default:
                LOG_M_N(TAG, "%s: %s payload %d", tracks.url, kv.second.control, kv.second.rtpPayload);
                break;
        }
    }

    {
        std::lock_guard<std::mutex> lck(m_mtxObs);
        for (auto &&m : m_lstObs) {
            m->OnRecvStreamInfo(m_stInfo);
        }
    }

    m_InitEvent.SetEvent();
}

void CRtspStream::OnPreparePlay(void) {
    m_PlayEvent.WaitEvent();
}

void CRtspStream::OnCheckAlive(int resultCode, const char *resultString) {
    if (0 == resultCode) {
        // LOG_M_C(TAG, "%s: resultCode = %d", __func__, resultCode);
        if (m_damon) {
            m_damon->KeepAlive();
        }
    } else {
        if (resultString) {
            LOG_M_E(TAG, "%s: resultCode = %d, %s", __func__, resultCode, resultString);
        } else {
            LOG_M_E(TAG, "%s: resultCode = %d", __func__, resultCode);
        }
    }
}

AX_BOOL CRtspStream::ReConnect(AX_VOID) {
    if (m_env && m_client) {
        m_env->taskScheduler().unscheduleDelayedTask(m_client->scs.streamTimerTask);
    }

    m_EventLoopThread.Stop();
    m_cExitThread = 1;
    m_EventLoopThread.Join();

    if (m_client) {
        m_client->Stop();
        m_client = nullptr;
    }

    m_stStat.nState = 0;

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
    m_stStat.nState = 1;

    return AX_TRUE;
}

AX_VOID CRtspStream::OnConnectStatusReport(AX_S32 nStatus) {
    std::lock_guard<std::mutex> lck(m_mtxObs);
    for (auto &&m : m_lstObs) {
        m->OnNotifyConnStatus(m_stAttr.strURL.c_str(), (CONNECT_STATUS_E)nStatus);
    }
}
