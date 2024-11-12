/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "stream.h"
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "log.h"

#define TAG "STREAM"

#define AVERRMSG(err, msg)                  \
    ({                                      \
        av_strerror(err, msg, sizeof(msg)); \
        msg;                                \
    })

typedef struct {
    stream_attr_t attr;
    AVFormatContext *avfmt;
    AVBSFContext *avbsf;
    pthread_t demux_tid;
    volatile AX_BOOL stop_demux;
    struct {
        AX_S32 id;
        stream_info_t info;
    } video;
    stream_sink_t sink;
} STREAM_OBJ_T;

static AX_VOID *demux_thread(AX_VOID *arg);

AX_HANDLE sample_stream_create(const stream_attr_t *attr) {
    ENTER_FUNC();

    STREAM_OBJ_T *obj = CALLOC(1, sizeof(STREAM_OBJ_T));
    if (!obj) {
        LOG_M_E(TAG, "create stream obj fail, url: %s", attr->url);
        return AX_INVALID_HANDLE;
    }

    obj->attr = *attr;
    obj->video.id = -1;
    obj->stop_demux = AX_TRUE;

    AX_S32 ret;
    AX_CHAR err_msg[64] = {0};

    do {
        obj->avfmt = avformat_alloc_context();
        if (!obj->avfmt) {
            LOG_M_E(TAG, "%s: avformat_alloc_context() fail", attr->url);
            break;
        }

        ret = avformat_open_input(&obj->avfmt, attr->url, NULL, NULL);
        if (ret < 0) {
            LOG_M_E(TAG, "open url: %s fail, %s", attr->url, AVERRMSG(ret, err_msg));
            avformat_free_context(obj->avfmt);
            obj->avfmt = AX_NULL;
            break;
        }

        /* find video track id */
        ret = avformat_find_stream_info(obj->avfmt, NULL);
        if (ret < 0) {
            LOG_M_E(TAG, "avformat_find_stream_info(url: %s) fail, %s", attr->url, AVERRMSG(ret, err_msg));
            break;
        }

        for (AX_S32 i = 0; i < obj->avfmt->nb_streams; ++i) {
            if (AVMEDIA_TYPE_VIDEO == obj->avfmt->streams[i]->codecpar->codec_type) {
                obj->video.id = i;
                break;
            }
        }

        if (-1 == obj->video.id) {
            LOG_M_E(TAG, "%s: has no video stream", attr->url);
            break;
        }

        AVStream *avs = obj->avfmt->streams[obj->video.id];
        switch (avs->codecpar->codec_id) {
            case AV_CODEC_ID_H264:
                obj->video.info.payload = PT_H264;
                break;
            case AV_CODEC_ID_HEVC:
                obj->video.info.payload = PT_H265;
                break;
            default:
                LOG_M_E(TAG, "unsupport video codec of url: %s", attr->url);
                break;
        }

        strcpy(obj->video.info.url, attr->url);
        obj->video.info.width = avs->codecpar->width;
        obj->video.info.height = avs->codecpar->height;

        if (avs->avg_frame_rate.den == 0 || (avs->avg_frame_rate.num == 0 && avs->avg_frame_rate.den == 1)) {
            obj->video.info.fps = (AX_U32)(round(av_q2d(avs->r_frame_rate)));
        } else {
            obj->video.info.fps = (AX_U32)(round(av_q2d(avs->avg_frame_rate)));
        }
        if (0 == obj->video.info.fps) {
            obj->video.info.fps = 30;
            LOG_M_W(TAG, "fps is 0, set to %d fps of url: %s", obj->video.info.fps, attr->url);
        }

        /* just example: assume mp4 here */
        const AVBitStreamFilter *bsf = av_bsf_get_by_name((PT_H264 == obj->video.info.payload) ? "h264_mp4toannexb" : "hevc_mp4toannexb");
        if (!bsf) {
            LOG_M_E(TAG, "%s: av_bsf_get_by_name() fail", attr->url);
            break;
        }

        ret = av_bsf_alloc(bsf, &obj->avbsf);
        if (ret < 0) {
            LOG_M_E(TAG, "%s: av_bsf_alloc() fail, %s", attr->url, AVERRMSG(ret, err_msg));
            break;
        }

        ret = avcodec_parameters_copy(obj->avbsf->par_in, obj->avfmt->streams[obj->video.id]->codecpar);
        if (ret < 0) {
            LOG_M_E(TAG, "%s: avcodec_parameters_copy() fail, %s", attr->url, AVERRMSG(ret, err_msg));
            break;
        }

        obj->avbsf->time_base_in = obj->avfmt->streams[obj->video.id]->time_base;

        ret = av_bsf_init(obj->avbsf);
        if (ret < 0) {
            LOG_M_E(TAG, "%s: av_bsf_init() fail, %s", attr->url, AVERRMSG(ret, err_msg));
            break;
        }

        /* probe to create decoder */
        if (obj->attr.event.probe) {
            obj->attr.event.probe(&obj->video.info, obj->attr.event.obj);
        }

        LEAVE_FUNC();
        return (AX_HANDLE)obj;

    } while (0);

    sample_stream_destory(obj);
    return AX_INVALID_HANDLE;
}

AX_BOOL sample_stream_destory(AX_HANDLE handle) {
    ENTER_FUNC();

    STREAM_OBJ_T *obj = (STREAM_OBJ_T *)handle;
    if (!obj) {
        LOG_M_E(TAG, "handle is nil");
        return AX_FALSE;
    }

    if (obj->avbsf) {
        av_bsf_free(&obj->avbsf);
        obj->avbsf = AX_NULL;
    }

    if (obj->avfmt) {
        avformat_close_input(&obj->avfmt);
        /*  avformat_close_input will FREE ctx
            http://ffmpeg.org/doxygen/trunk/demux_8c_source.html
        */
        // avformat_free_context(m_pAvFmtCtx);
        obj->avfmt = AX_NULL;
    }

    FREE(obj);

    LEAVE_FUNC();
    return AX_TRUE;
}

AX_BOOL sample_stream_register_sink(AX_HANDLE handle, stream_sink_t sink) {
    ENTER_FUNC();

    STREAM_OBJ_T *obj = (STREAM_OBJ_T *)handle;
    if (!obj) {
        LOG_M_E(TAG, "handle is nil");
        return AX_FALSE;
    }

    obj->sink = sink;

    LEAVE_FUNC();
    return AX_TRUE;
}

AX_BOOL sample_stream_start(AX_HANDLE handle) {
    ENTER_FUNC();

    STREAM_OBJ_T *obj = (STREAM_OBJ_T *)handle;
    if (!obj) {
        LOG_M_E(TAG, "handle is nil");
        return AX_FALSE;
    }

    if (!obj->stop_demux) {
        LOG_M_W(TAG, "%s: recv thread is running", obj->attr.url);
        return AX_TRUE;
    }

    if (0 != pthread_create(&obj->demux_tid, AX_NULL, demux_thread, obj)) {
        LOG_M_E(TAG, "%s: create recv thread fail", obj->attr.url);
        return AX_FALSE;
    }

    LEAVE_FUNC();
    return AX_TRUE;
}

AX_BOOL sample_stream_stop(AX_HANDLE handle) {
    ENTER_FUNC();

    STREAM_OBJ_T *obj = (STREAM_OBJ_T *)handle;
    if (!obj) {
        LOG_M_E(TAG, "handle is nil");
        return AX_FALSE;
    }

    obj->stop_demux = AX_TRUE;

    if (obj->demux_tid) {
        pthread_join(obj->demux_tid, AX_NULL);
        obj->demux_tid = 0;
    }

    LEAVE_FUNC();
    return AX_TRUE;
}

static AX_VOID *demux_thread(AX_VOID *arg) {
    ENTER_FUNC();

    STREAM_OBJ_T *obj = (STREAM_OBJ_T *)arg;
    obj->stop_demux = AX_FALSE;

    AX_S32 ret;
    AX_CHAR err_msg[64] = {0};
    AX_U32 frame_num = 0;
    AX_U64 pts = 0; /* us */
    const AX_U32 interval = (1000000 / obj->video.info.fps);

    AVPacket *avpkt = av_packet_alloc();
    if (!avpkt) {
        obj->stop_demux = AX_TRUE;
        LOG_M_E(TAG, "%s: av_packet_alloc() fail!", obj->attr.url);
        return (AX_VOID *)0;
    }

    while (!obj->stop_demux) {
        ret = av_read_frame(obj->avfmt, avpkt);
        if (ret < 0) {
            if (AVERROR_EOF == ret) {
                LOG_M_I(TAG, "%s: reach eof", obj->attr.url);
                /* EOF */
                if (obj->sink.on_send_stream) {
                    obj->sink.on_send_stream(AX_NULL, 0, 0, obj->sink.sinker);
                }

                if (obj->attr.event.eof) {
                    obj->attr.event.eof(obj->attr.url, frame_num, obj->attr.event.obj);
                }
                break;
            } else {
                LOG_M_E(TAG, "%s: av_read_frame()fail, %s", obj->attr.url, AVERRMSG(ret, err_msg));
                break;
            }

        } else {
            if (avpkt->stream_index == obj->video.id) {
                ret = av_bsf_send_packet(obj->avbsf, avpkt);
                if (ret < 0) {
                    av_packet_unref(avpkt);
                    LOG_M_E(TAG, "%s: av_bsf_send_packet() fail, %s", obj->attr.url, AVERRMSG(ret, err_msg));
                    break;
                }

                while (ret >= 0) {
                    ret = av_bsf_receive_packet(obj->avbsf, avpkt);
                    if (ret < 0) {
                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                            break;
                        }

                        LOG_M_E(TAG, "%s: av_bsf_receive_packet() fail, %s", obj->attr.url, AVERRMSG(ret, err_msg));

                        av_packet_unref(avpkt);
                        av_packet_free(&avpkt);
                        obj->stop_demux = AX_TRUE;

                        return (AX_VOID *)0;
                    }

                    ++frame_num;

                    /* just example */
                    pts += interval;

                    if (obj->sink.on_send_stream) {
                        obj->sink.on_send_stream(avpkt->data, avpkt->size, pts, obj->sink.sinker);
                    }
                }
            }

            av_packet_unref(avpkt);
        }
    }

    av_packet_free(&avpkt);

    LEAVE_FUNC();
    return (AX_VOID *)0;
}