/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "FileStreamer.hpp"
#include <time.h>
#include <chrono>
#include <random>
#include <thread>
#include "AppLogApi.h"
#include "ax_sys_api.h"

#define DEMUX "DEMUX"

AX_VOID CFileStreamer::DemuxThread(AX_VOID* pArg) {
    AX_S32 ret;
    const AX_S32 nCookie = m_stInfo.nCookie;
    AX_U64 nPTS;
    AX_U32 nPTSIntv = 1000000 / ((m_nForceFps > 0) ? m_nForceFps : m_stInfo.nFps);

    std::default_random_engine e(time(0));
    std::uniform_int_distribution<unsigned> u(0, m_nMaxSendNaluIntervalMilliseconds);

    while (m_DemuxThread.IsRunning()) {
        ret = av_read_frame(m_pAvFmtCtx, m_pAvPkt);
        if (ret < 0) {
            if (AVERROR_EOF == ret) {
                LOG_M_I(DEMUX, "reach eof of stream %d ", nCookie);
                if (m_bLoop) {
                    /* AVSEEK_FLAG_BACKWARD may fail (example: zhuheqiao.mp4), use AVSEEK_FLAG_ANY, but not guarantee seek to I frame */
                    av_bsf_flush(m_pAvBSFCtx);
                    ret = av_seek_frame(m_pAvFmtCtx, m_nVideoIndex, 0, AVSEEK_FLAG_ANY /* AVSEEK_FLAG_BACKWARD */);
                    if (ret < 0) {
                        LOG_M_W(DEMUX, "retry to seek stream %d to begin", nCookie);
                        ret = avformat_seek_file(m_pAvFmtCtx, m_nVideoIndex, INT64_MIN, 0, INT64_MAX, AVSEEK_FLAG_BYTE);
                        if (ret < 0) {
                            LOG_M_E(DEMUX, "fail to seek stream %d to begin, error: %d", nCookie, ret);
                            break;
                        }
                    }
                    continue;
                } else {
                    break;
                }
            } else {
                LOG_M_E(DEMUX, "av_read_frame(stream %d) fail, error: %d", nCookie, ret);
                break;
            }

        } else {
            if (m_pAvPkt->stream_index == m_nVideoIndex) {
                ret = av_bsf_send_packet(m_pAvBSFCtx, m_pAvPkt);
                if (ret < 0) {
                    av_packet_unref(m_pAvPkt);
                    LOG_M_E(DEMUX, "av_bsf_send_packet(stream %d) fail, error: %d", nCookie, ret);
                    break;
                }

                while (ret >= 0) {
                    ret = av_bsf_receive_packet(m_pAvBSFCtx, m_pAvPkt);
                    if (ret < 0) {
                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                            break;
                        }

                        av_packet_unref(m_pAvPkt);
                        LOG_M_E(DEMUX, "av_bsf_receive_packet(stream %d) fail, error: %d", nCookie, ret);

                        UpdateStatus(AX_FALSE);
                        return;
                    }

                    ++m_stStat.nCount;

                    if (1 == m_stStat.nCount || 0 == m_stInfo.nFps) {
                        /* 1st frame or unknown fps */
                        AX_SYS_GetCurPTS(&nPTS);
                    } else {
                        nPTS += nPTSIntv;
                    }

                    // LOG_M_C(DEMUX, "pts = %lld", nPTS);

                    for (auto&& m : m_lstObs) {
                        if (!m->OnRecvVideoData(nCookie, m_pAvPkt->data, m_pAvPkt->size, nPTS) && m_bSyncObs) {
                            break;
                        }
                    }

                    if (m_nMaxSendNaluIntervalMilliseconds > 0) {
                        AX_U32 ms = u(e);
                        if (ms > 10) {
                        //  LOG_M_C(DEMUX, "sleep for %d ms", ms);
                            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
                        }
                    }
                }
            }

            av_packet_unref(m_pAvPkt);
        }
    }

    UpdateStatus(AX_FALSE);
    LOG_M_C(DEMUX, "stop stream %d ---", nCookie);
}

AX_BOOL CFileStreamer::Init(const STREAMER_ATTR_T& stAttr) {
    LOG_M_D(DEMUX, "%s: stream %d +++", __func__, stAttr.nCookie);

    m_nForceFps = stAttr.nForceFps;
    m_nMaxSendNaluIntervalMilliseconds = stAttr.nMaxSendNaluIntervalMilliseconds;
    // #ifdef __SLT__
    //     m_bLoop = AX_FALSE;
    // #else
    m_bLoop = stAttr.bLoop;
    m_bSyncObs = stAttr.bSyncObs;
    // #endif

    m_stInfo.strPath = stAttr.strPath;
    m_stInfo.eStreamType = STREAM_TYPE_E::FILE;
    m_stInfo.nCookie = stAttr.nCookie;
    m_stInfo.nWidth = stAttr.nMaxWidth;
    m_stInfo.nHeight = stAttr.nMaxHeight;
    m_stInfo.nFps = 30; /* default fps */

    m_stStat.bStarted = AX_FALSE;
    m_stStat.nCount = 0;

    const AX_S32 nCookie = m_stInfo.nCookie;
    AVCodecID eCodecID{AV_CODEC_ID_H264};

    AX_S32 ret = 0;
    m_pAvFmtCtx = avformat_alloc_context();
    if (!m_pAvFmtCtx) {
        LOG_M_E(DEMUX, "avformat_alloc_context(stream %d) failed!", nCookie);
        return AX_FALSE;
    }

    ret = avformat_open_input(&m_pAvFmtCtx, m_stInfo.strPath.c_str(), nullptr, nullptr);
    if (ret < 0) {
        AX_CHAR szError[64] = {0};
        av_strerror(ret, szError, 64);
        LOG_M_E(DEMUX, "open %s fail, error: %d, %s", m_stInfo.strPath.c_str(), ret, szError);
        goto __FAIL__;
    }

    ret = avformat_find_stream_info(m_pAvFmtCtx, nullptr);
    if (ret < 0) {
        LOG_M_E(DEMUX, "avformat_find_stream_info(stream %d) fail, error = %d", nCookie, ret);
        goto __FAIL__;
    }

    for (AX_U32 i = 0; i < m_pAvFmtCtx->nb_streams; i++) {
        if (AVMEDIA_TYPE_VIDEO == m_pAvFmtCtx->streams[i]->codecpar->codec_type) {
            m_nVideoIndex = i;
            break;
        }
    }

    if (-1 == m_nVideoIndex) {
        LOG_M_E(DEMUX, "%s has no video stream %d!", m_stInfo.strPath.c_str(), nCookie);
        goto __FAIL__;
    } else {
        AVStream* pAvs = m_pAvFmtCtx->streams[m_nVideoIndex];
        eCodecID = pAvs->codecpar->codec_id;
        switch (eCodecID) {
            case AV_CODEC_ID_H264:
                m_stInfo.eVideoType = PT_H264;
                break;
            case AV_CODEC_ID_HEVC:
                m_stInfo.eVideoType = PT_H265;
                break;
            default:
                LOG_M_E(DEMUX, "Current Only support H264 or HEVC stream %d!", nCookie);
                goto __FAIL__;
        }

        m_stInfo.nWidth = pAvs->codecpar->width;
        m_stInfo.nHeight = pAvs->codecpar->height;

        if (pAvs->avg_frame_rate.den == 0 || (pAvs->avg_frame_rate.num == 0 && pAvs->avg_frame_rate.den == 1)) {
            m_stInfo.nFps = (AX_U32)(round(av_q2d(pAvs->r_frame_rate)));
        } else {
            m_stInfo.nFps = (AX_U32)(round(av_q2d(pAvs->avg_frame_rate)));
        }

        if (m_stInfo.nFps > 60) {
            m_stInfo.nFps = 60;
        }

        if (0 == m_stInfo.nFps) {
            m_stInfo.nFps = 30;
            LOG_M_W(DEMUX, "stream %d fps is 0, set to %d fps", nCookie, m_stInfo.nFps);
        }

        LOG_M_I(DEMUX, "stream %d: vcodec %d, %dx%d, fps %d", nCookie, m_stInfo.eVideoType, m_stInfo.nWidth, m_stInfo.nHeight,
                m_stInfo.nFps);
    }

    m_pAvPkt = av_packet_alloc();
    if (!m_pAvPkt) {
        LOG_M_E(DEMUX, "Create packet(stream %d) fail!", nCookie);
        goto __FAIL__;
    }

    if ((AV_CODEC_ID_H264 == eCodecID) || (AV_CODEC_ID_HEVC == eCodecID)) {
        const AVBitStreamFilter* m_pBSFilter = av_bsf_get_by_name((AV_CODEC_ID_H264 == eCodecID) ? "h264_mp4toannexb" : "hevc_mp4toannexb");
        if (!m_pBSFilter) {
            LOG_M_E(DEMUX, "av_bsf_get_by_name(stream %d) fail!", nCookie);
            goto __FAIL__;
        }

        ret = av_bsf_alloc(m_pBSFilter, &m_pAvBSFCtx);
        if (ret < 0) {
            LOG_M_E(DEMUX, "av_bsf_alloc(stream %d) fail, error:%d", nCookie, ret);
            goto __FAIL__;
        }

        ret = avcodec_parameters_copy(m_pAvBSFCtx->par_in, m_pAvFmtCtx->streams[m_nVideoIndex]->codecpar);
        if (ret < 0) {
            LOG_M_E(DEMUX, "avcodec_parameters_copy(stream %d) fail, error:%d", nCookie, ret);
            goto __FAIL__;
        } else {
            m_pAvBSFCtx->time_base_in = m_pAvFmtCtx->streams[m_nVideoIndex]->time_base;
        }

        ret = av_bsf_init(m_pAvBSFCtx);
        if (ret < 0) {
            LOG_M_E(DEMUX, "av_bsf_init(stream %d) fail, error:%d", nCookie, ret);
            goto __FAIL__;
        }
    }

    LOG_M_D(DEMUX, "%s: stream %d ---", __func__, nCookie);
    return AX_TRUE;

__FAIL__:
    DeInit();
    return AX_FALSE;
}

AX_BOOL CFileStreamer::DeInit(AX_VOID) {
    LOG_M_D(DEMUX, "%s: stream %d +++", __func__, m_stInfo.nCookie);

    if (m_DemuxThread.IsRunning()) {
        LOG_M_E(DEMUX, "%s: demux thread is still running", __func__);
        return AX_FALSE;
    }

    if (m_pAvPkt) {
        av_packet_free(&m_pAvPkt);
        m_pAvPkt = nullptr;
    }

    if (m_pAvBSFCtx) {
        av_bsf_free(&m_pAvBSFCtx);
        m_pAvBSFCtx = nullptr;
    }

    if (m_pAvFmtCtx) {
        avformat_close_input(&m_pAvFmtCtx);
        /*  avformat_close_input will free ctx
            http://ffmpeg.org/doxygen/trunk/demux_8c_source.html
        */
        // avformat_free_context(m_pAvFmtCtx);
        m_pAvFmtCtx = nullptr;
    }

    LOG_M_D(DEMUX, "%s: stream %d ---", __func__, m_stInfo.nCookie);
    return AX_TRUE;
}

AX_BOOL CFileStreamer::Start(AX_VOID) {
    LOG_M_D(DEMUX, "%s: stream %d +++", __func__, m_stInfo.nCookie);

    AX_CHAR szName[32];
    sprintf(szName, "AppDemux%d", m_stInfo.nCookie);
    if (!m_DemuxThread.Start([this](AX_VOID* pArg) -> AX_VOID { DemuxThread(pArg); }, nullptr, szName)) {
        LOG_M_E(DEMUX, "%s: create demux thread of stream %d fail", __func__, m_stInfo.nCookie);
        return AX_FALSE;
    }

    UpdateStatus(AX_TRUE);
    LOG_M_D(DEMUX, "%s: stream %d ---", __func__, m_stInfo.nCookie);
    return AX_TRUE;
}

AX_BOOL CFileStreamer::Stop(AX_VOID) {
    LOG_M_C(DEMUX, "stop stream %d +++", m_stInfo.nCookie);

    m_DemuxThread.Stop();
    m_DemuxThread.Join();

    // LOG_M_I(DEMUX, "stream %d has sent total %lld frames", m_stInfo.nCookie, m_stStat.nCount);
    return AX_TRUE;
}
