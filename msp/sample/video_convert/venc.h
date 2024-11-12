/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef __SAMPLE_VENC_H__
#define __SAMPLE_VENC_H__

#include "ax_venc_api.h"
#include "def.h"

typedef struct {
    AX_PAYLOAD_TYPE_E payload;
    AX_U32 width;
    AX_U32 height;
    AX_U32 fps;
    AX_VENC_PROFILE_E profile;
    AX_VENC_LEVEL_E level;
    AX_VENC_TIER_E tile;

    /* RC */
    AX_VENC_RC_ATTR_T rc;
} venc_attr_t;

typedef AX_VOID (*on_recv_stream_callback)(AX_S32 veChn, AX_VENC_STREAM_T *stream, AX_VOID *sinker);
typedef struct {
    on_recv_stream_callback on_recv_stream;
    AX_VOID *sinker;
} venc_sink_t;

AX_S32  sample_venc_create_chn(const venc_attr_t *attr);
AX_BOOL sample_venc_destory_chn(AX_S32 veChn);

AX_BOOL sample_venc_register_sink(AX_S32 veChn, venc_sink_t sink);

/**
 * @brief Start and stop encoder
 *
 * @param veChn
 * @param timeout - unit: ms
 *                         < 0: wait all streams are finished to encode and then stop
 *                           0: stop immediatelly
 *                         if rtsp, recommend to 0.
 * @return AX_BOOL
 */
AX_BOOL sample_venc_start_chn(AX_S32 veChn);
AX_BOOL sample_venc_stop_chn(AX_S32 veChn, AX_S32 timeout);

#endif /* __SAMPLE_VENC_H__ */