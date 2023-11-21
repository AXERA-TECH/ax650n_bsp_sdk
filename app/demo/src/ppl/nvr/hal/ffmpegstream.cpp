/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "ffmpegstream.hpp"
#include "AppLogApi.h"
#include "ax_sys_api.h"
#include "fpsctrl.hpp"

#define TAG "FFMPEG"

#define AVERRMSG(err, msg)                  \
    ({                                      \
        av_strerror(err, msg, sizeof(msg)); \
        msg;                                \
    })

AX_VOID CFFMpegStream::DemuxThread(AX_VOID* /* pArg */) {
    LOG_MM_I(TAG, "%s: +++", m_url);

    AX_S32 ret;
    AX_CHAR errMsg[64] = {0};

    CFpsCtrl fpsCtrl(m_stInfo.stVideo.nFps);
    AX_U32 nFpsMargin = 1000000 / m_stInfo.stVideo.nFps / 3;

    STREAM_FRAME_T nalu;
    nalu.enPayload = m_stInfo.stVideo.enPayload;
    nalu.nPrivData = 0;
    nalu.frame.stVideo.stInfo = m_stInfo.stVideo;
    nalu.frame.stVideo.nPTS = 0;
    nalu.frame.stVideo.bSkipDisplay = AX_FALSE;

    while (m_DemuxThread.IsRunning()) {
        ret = av_read_frame(m_pAvFmtCtx, m_pAvPkt);

        if (ret < 0) {
            if (AVERROR_EOF == ret) {
                LOG_M_I(TAG, "%s: reach eof", m_url);
                if (m_bLoop) {
                    /* AVSEEK_FLAG_BACKWARD may fail (example: zhuheqiao.mp4), use AVSEEK_FLAG_ANY, but not guarantee seek to I frame */
                    av_bsf_flush(m_pAvBSFCtx);
                    ret = av_seek_frame(m_pAvFmtCtx, m_nVideoTrack, 0, AVSEEK_FLAG_ANY /* AVSEEK_FLAG_BACKWARD */);
                    if (ret < 0) {
                        LOG_M_W(TAG, "retry to seek %s to begin", m_url);
                        ret = avformat_seek_file(m_pAvFmtCtx, m_nVideoTrack, INT64_MIN, 0, INT64_MAX, AVSEEK_FLAG_BYTE);
                        if (ret < 0) {
                            LOG_M_E(TAG, "fail to seek %s to begin, %s", m_url, AVERRMSG(ret, errMsg));
                            break;
                        }
                    }

                    fpsCtrl.Reset();
                    continue;
                } else {
                    break;
                }
            } else {
                LOG_M_E(TAG, "%s: av_read_frame()fail, %s", m_url, AVERRMSG(ret, errMsg));
                break;
            }

        } else {
            if (m_pAvPkt->stream_index == m_nVideoTrack) {
                ret = av_bsf_send_packet(m_pAvBSFCtx, m_pAvPkt);
                if (ret < 0) {
                    av_packet_unref(m_pAvPkt);
                    LOG_M_E(TAG, "%s: av_bsf_send_packet() fail, %s", m_url, AVERRMSG(ret, errMsg));
                    break;
                }

                while (ret >= 0) {
                    ret = av_bsf_receive_packet(m_pAvBSFCtx, m_pAvPkt);
                    if (ret < 0) {
                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                            break;
                        }

                        av_packet_unref(m_pAvPkt);
                        LOG_M_E(TAG, "%s: av_bsf_receive_packet() fail, %s", m_url, AVERRMSG(ret, errMsg));

                        return;
                    }

                    ++m_stStat.nCount[0];

                    /* fixme:
                        ffmpeg is just for debug, so simply to set all nalus to IDR frame
                    */
                    nalu.frame.stVideo.enNalu = NALU_TYPE_IDR;
                    nalu.frame.stVideo.pData = m_pAvPkt->data;
                    nalu.frame.stVideo.nLen = m_pAvPkt->size;

                    if (1 == m_stStat.nCount[0] || 0 == m_stInfo.stVideo.nFps) {
                        /* 1st frame or unknown fps */
                        AX_SYS_GetCurPTS(&nalu.frame.stVideo.nPTS);
                    } else {
                        nalu.frame.stVideo.nPTS += (1000000 / m_stInfo.stVideo.nFps);
                    }

                    if (m_stAttr.nFps >= 0) {
                        fpsCtrl.Control(m_stStat.nCount[0], nFpsMargin);
                    }

                    std::lock_guard<std::mutex> lck(m_mtxObs);
                    for (auto&& m : m_lstObs) {
                        m->OnRecvStreamData(nalu);
                    }
                }
            }

            av_packet_unref(m_pAvPkt);
        }
    }

    LOG_MM_I(TAG, "%s: ---", m_url);
}

AX_BOOL CFFMpegStream::Init(const STREAM_ATTR_T& stAttr) {
    m_stAttr = stAttr;
    m_stInfo.strURL = stAttr.strURL;
    m_url = m_stInfo.strURL.c_str();

    LOG_MM_I(TAG, "%s: +++", m_url);

    AX_CHAR errMsg[64] = {0};
    AX_S32 ret = 0;
    m_pAvFmtCtx = avformat_alloc_context();
    if (!m_pAvFmtCtx) {
        LOG_M_E(TAG, "%s: avformat_alloc_context() failed!", m_url);
        return AX_FALSE;
    }

    ret = avformat_open_input(&m_pAvFmtCtx, m_url, NULL, NULL);
    if (ret < 0) {
        LOG_M_E(TAG, "%s: open fail, %s", m_url, AVERRMSG(ret, errMsg));
        goto __FAIL__;
    }

    ret = avformat_find_stream_info(m_pAvFmtCtx, NULL);
    if (ret < 0) {
        LOG_M_E(TAG, "%s: avformat_find_stream_info() fail, %s", m_url, AVERRMSG(ret, errMsg));
        goto __FAIL__;
    }

    for (AX_U32 i = 0; i < m_pAvFmtCtx->nb_streams; i++) {
        if (AVMEDIA_TYPE_VIDEO == m_pAvFmtCtx->streams[i]->codecpar->codec_type) {
            m_nVideoTrack = i;
            break;
        }
    }

    if (-1 == m_nVideoTrack) {
        LOG_M_E(TAG, "%s: has no video stream", m_url);
        goto __FAIL__;
    } else {
        AVStream* avs = m_pAvFmtCtx->streams[m_nVideoTrack];
        AVCodecID codec = avs->codecpar->codec_id;
        STREAM_TRACK_INFO_T video;
        switch (codec) {
            case AV_CODEC_ID_H264:
                video.enPayload = PT_H264;
                break;
            case AV_CODEC_ID_HEVC:
                video.enPayload = PT_H265;
                break;
            default:
                LOG_M_E(TAG, "%s: unsupport video track!", m_url);
                goto __FAIL__;
        }

        video.info.stVideo.enPayload = video.enPayload;
        video.info.stVideo.nWidth = avs->codecpar->width;
        video.info.stVideo.nHeight = avs->codecpar->height;

        if (avs->avg_frame_rate.den == 0 || (avs->avg_frame_rate.num == 0 && avs->avg_frame_rate.den == 1)) {
            video.info.stVideo.nFps = (AX_U32)(round(av_q2d(avs->r_frame_rate)));
        } else {
            video.info.stVideo.nFps = (AX_U32)(round(av_q2d(avs->avg_frame_rate)));
        }

        if (video.info.stVideo.nFps > 60) {
            video.info.stVideo.nFps = 60;
        }

        if (0 == video.info.stVideo.nFps) {
            video.info.stVideo.nFps = 30;
            LOG_M_W(TAG, "%s: fps is 0, set to %d fps", m_url, video.info.stVideo.nFps);
        }

        if (m_stAttr.nFps > 0) {
            /* force fps */
            video.info.stVideo.nFps = m_stAttr.nFps;
        }

        LOG_MM_D(TAG, "%s: codec %d, %dx%d, fps %d", m_url, video.enPayload, video.info.stVideo.nWidth, video.info.stVideo.nHeight,
                 video.info.stVideo.nFps);

        m_stInfo.tracks[this] = video;
        m_stInfo.stVideo = video.info.stVideo;

        std::lock_guard<std::mutex> lck(m_mtxObs);
        for (auto&& m : m_lstObs) {
            m->OnRecvStreamInfo(m_stInfo);
        }
    }

    m_pAvPkt = av_packet_alloc();
    if (!m_pAvPkt) {
        LOG_M_E(TAG, "%s: av_packet_alloc() fail!", m_url);
        goto __FAIL__;
    }

    if (PT_H264 == m_stInfo.stVideo.enPayload || PT_H265 == m_stInfo.stVideo.enPayload) {
        CONST AVBitStreamFilter* bsf =
            av_bsf_get_by_name((PT_H264 == m_stInfo.stVideo.enPayload) ? "h264_mp4toannexb" : "hevc_mp4toannexb");
        if (!bsf) {
            LOG_M_E(TAG, "%s: av_bsf_get_by_name() fail!", m_url);
            goto __FAIL__;
        }

        ret = av_bsf_alloc(bsf, &m_pAvBSFCtx);
        if (ret < 0) {
            LOG_M_E(TAG, "%s: av_bsf_alloc() fail, %s", m_url, AVERRMSG(ret, errMsg));
            goto __FAIL__;
        }

        ret = avcodec_parameters_copy(m_pAvBSFCtx->par_in, m_pAvFmtCtx->streams[m_nVideoTrack]->codecpar);
        if (ret < 0) {
            LOG_M_E(TAG, "%s: avcodec_parameters_copy() fail, %s", m_url, AVERRMSG(ret, errMsg));
            goto __FAIL__;
        } else {
            m_pAvBSFCtx->time_base_in = m_pAvFmtCtx->streams[m_nVideoTrack]->time_base;
        }

        ret = av_bsf_init(m_pAvBSFCtx);
        if (ret < 0) {
            LOG_M_E(TAG, "%s: av_bsf_init() fail, %s", m_url, AVERRMSG(ret, errMsg));
            goto __FAIL__;
        }
    }

    LOG_MM_I(TAG, "%s: ---", m_url);
    return AX_TRUE;

__FAIL__:

    DeInit();
    return AX_FALSE;
}

AX_BOOL CFFMpegStream::DeInit(AX_VOID) {
#ifdef __DEBUG_STOP__
    LOG_M_C(TAG, "%s: %s +++", m_url, __func__);
#else
    LOG_M_I(TAG, "%s: %s +++", m_url, __func__);
#endif

    if (m_DemuxThread.IsRunning()) {
        LOG_M_W(TAG, "%s: demux thread is running, stop first", m_url);
        Stop();
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

#ifdef __DEBUG_STOP__
    LOG_M_C(TAG, "%s: %s ---", m_url, __func__);
#else
    LOG_M_I(TAG, "%s: %s ---", m_url, __func__);
#endif
    return AX_TRUE;
}

AX_BOOL CFFMpegStream::Start(AX_VOID) {
    LOG_M_I(TAG, "%s: %s +++", m_url, __func__);

    m_stStat.nState = 1;
    for (auto&& m : m_stStat.nCount) {
        m = 0;
    }

    if (!m_DemuxThread.Start([this](AX_VOID* pArg) -> AX_VOID { DemuxThread(pArg); }, nullptr, "AppDemux")) {
        LOG_M_E(TAG, "%s: create demux thread fail", m_url);
        return AX_FALSE;
    }

    LOG_M_I(TAG, "%s: %s ---", m_url, __func__);
    return AX_TRUE;
}

AX_BOOL CFFMpegStream::Stop(AX_VOID) {
#ifdef __DEBUG_STOP__
    LOG_M_C(TAG, "%s: %s +++", m_url, __func__);
#else
    LOG_M_I(TAG, "%s: %s +++", m_url, __func__);
#endif

    m_DemuxThread.Stop();
    m_DemuxThread.Join();

    m_stStat.nState = 0;

#ifdef __DEBUG_STOP__
    LOG_M_C(TAG, "%s: %s ---", m_url, __func__);
#else
    LOG_M_I(TAG, "%s: %s ---", m_url, __func__);
#endif
    return AX_TRUE;
}
