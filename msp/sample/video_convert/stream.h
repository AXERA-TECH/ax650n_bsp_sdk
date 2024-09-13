/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef __SAMPLE_STREAM_H__
#define __SAMPLE_STREAM_H__

#include "def.h"

/**
 * @brief demux mp4 or h264/h265 annexb raw stream, send to decode by frame
 *    H264: combine sps+pps+IDR to one frame
 *    H265: combine sps+pps+vps+IDR to one frame
 */

typedef struct {
    AX_CHAR url[MAX_PATH];
    AX_U32 width;
    AX_U32 height;
    AX_U32 fps;
    AX_PAYLOAD_TYPE_E payload;
} stream_info_t;

/**
 * @brief Send annexb nalu to decoder
 * @data annexb nalu
 * @len length of annexb nalu
 * @pts PTS
 * @sinker deocder object
 */
typedef AX_VOID (*on_send_stream_callback)(const AX_U8 *data, AX_U32 len, AX_U64 pts, AX_VOID *sinker);
typedef AX_VOID (*on_stream_probe_event_callback)(const stream_info_t *info, AX_VOID *obj);
typedef AX_VOID (*on_stream_eof_event_callback)(const AX_CHAR *url, AX_U32 frame_num, AX_VOID *obj);

typedef struct {
    AX_CHAR url[MAX_PATH];
    struct {
        on_stream_probe_event_callback probe;
        on_stream_eof_event_callback eof;
        AX_VOID *obj;
    } event;
} stream_attr_t;

typedef struct {
    on_send_stream_callback on_send_stream;
    AX_VOID *sinker;
} stream_sink_t;

AX_HANDLE sample_stream_create(const stream_attr_t *attr);
AX_BOOL sample_stream_destory(AX_HANDLE handle);

AX_BOOL sample_stream_register_sink(AX_HANDLE handle, stream_sink_t sink);

AX_BOOL sample_stream_start(AX_HANDLE handle);
AX_BOOL sample_stream_stop(AX_HANDLE handle);

#endif /* __SAMPLE_STREAM_H__ */