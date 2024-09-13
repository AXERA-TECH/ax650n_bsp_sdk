/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "venc.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"
#include "utils.h"

#define TAG "VENC"

enum {
    stateUnknown = 0,
    stateInited = 1,
    stateStarted = 2,
};

typedef struct {
    pthread_mutex_t lock;
    AX_S32 veChn;
    AX_U32 state;
    venc_attr_t attr;
    venc_sink_t sink;
    pthread_t recv_tid;
    volatile AX_BOOL stop_recv;
} venc_obj_t;

static venc_obj_t *g_objs[MAX_VENC_CHN_NUM] = {AX_NULL};
static pthread_mutex_t g_objsLock = PTHREAD_MUTEX_INITIALIZER;

#define GET_VENC_OBJ(veChn)                                             \
    ({                                                                  \
        if (veChn < 0 || veChn >= MAX_VENC_CHN_NUM) {                   \
            LOG_M_E(TAG, "invalid veChn %d", veChn);                    \
            return AX_FALSE;                                            \
        }                                                               \
                                                                        \
        venc_obj_t *obj = g_objs[veChn];                                \
        if (!obj || !obj->state) {                                      \
            LOG_M_E(TAG, "veChn %d is not inited, obj %p", veChn, obj); \
            return AX_FALSE;                                            \
        }                                                               \
                                                                        \
        obj;                                                            \
    })

static AX_VOID *recv_thread(AX_VOID *arg);

AX_S32 sample_venc_create_chn(const venc_attr_t *attr) {
    ENTER_FUNC();

    venc_obj_t *obj = CALLOC(1, sizeof(venc_obj_t));
    if (!obj) {
        LOG_M_E(TAG, "create venc obj fail");
        return INVALID_VENC_CHN;
    }

    obj->attr = *attr;
    obj->veChn = INVALID_VENC_CHN;
    obj->stop_recv = AX_TRUE;
    pthread_mutex_init(&obj->lock, AX_NULL);

    /* find an unused vdChn */
    pthread_mutex_lock(&g_objsLock);
    for (AX_S32 i = 0; i < MAX_VENC_CHN_NUM; ++i) {
        if (!g_objs[i]) {
            obj->veChn = i;
            g_objs[i] = obj;
            break;
        }
    }
    pthread_mutex_unlock(&g_objsLock);

    if (INVALID_VENC_CHN == obj->veChn) {
        LOG_M_E(TAG, "all veChns are used");
        pthread_mutex_destroy(&obj->lock);
        FREE(obj);
        return INVALID_VENC_CHN;
    }

    AX_S32 ret;

    do {
        /* Step1: Create and configure encoder channel */
        AX_VENC_CHN_ATTR_T chn_attr;
        memset(&chn_attr, 0, sizeof(chn_attr));

        chn_attr.stVencAttr.enType = attr->payload;
        chn_attr.stVencAttr.u32MaxPicWidth = ALIGN_UP(attr->width, VENC_FBC_STRIDE_ALIGN_VAL);
        chn_attr.stVencAttr.u32MaxPicHeight = ALIGN_UP(attr->height, 16);
        chn_attr.stVencAttr.enMemSource = AX_MEMORY_SOURCE_CMM;
        /*
            0: means calculate u32BufSize by SDK - default  w * h * 1.5
            min u32BufSize for H264/H265 not less than w * h / 2
        */
        chn_attr.stVencAttr.u32BufSize = 0;
        chn_attr.stVencAttr.enProfile = attr->profile;
        chn_attr.stVencAttr.enLevel = attr->level;
        chn_attr.stVencAttr.enTier = attr->tile;
        chn_attr.stVencAttr.enStrmBitDepth = AX_VENC_STREAM_BIT_8;
        chn_attr.stVencAttr.u32PicWidthSrc = attr->width;
        chn_attr.stVencAttr.u32PicHeightSrc = attr->height;
        chn_attr.stVencAttr.enLinkMode = AX_VENC_LINK_MODE;
        chn_attr.stVencAttr.s32StopWaitTime = 0;
        chn_attr.stVencAttr.u8InFifoDepth = 2;
        chn_attr.stVencAttr.u8OutFifoDepth = 2;

        chn_attr.stVencAttr.flag = (1 << 1); /* stream is cached */

        /* RC */
        chn_attr.stRcAttr = attr->rc;
        chn_attr.stRcAttr.stFrameRate.fSrcFrameRate = attr->fps;
        chn_attr.stRcAttr.stFrameRate.fDstFrameRate = attr->fps;

        /* just example, fix to normalP mode, application can change the gop mode. */
        chn_attr.stGopAttr.enGopMode = AX_VENC_GOPMODE_NORMALP;
        ret = AX_VENC_CreateChn(obj->veChn, &chn_attr);
        if (0 != ret) {
            LOG_M_E(TAG, "AX_VENC_CreateChn(veChn %d) fail, ret = 0x%x", obj->veChn, ret);
            break;
        }

        pthread_mutex_lock(&obj->lock);
        obj->state = stateInited;
        pthread_mutex_unlock(&obj->lock);

        LEAVE_FUNC();
        return obj->veChn;

    } while (0);

    pthread_mutex_lock(&g_objsLock);
    g_objs[obj->veChn] = AX_NULL;
    pthread_mutex_unlock(&g_objsLock);

    pthread_mutex_destroy(&obj->lock);
    FREE(obj);

    return INVALID_VENC_CHN;
}

AX_BOOL sample_venc_destory_chn(AX_S32 veChn) {
    ENTER_FUNC();

    venc_obj_t *obj = GET_VENC_OBJ(veChn);

    AX_S32 ret;
    ret = AX_VENC_DestroyChn(veChn);
    if (0 != ret) {
        LOG_M_E(TAG, "AX_VENC_DestroyChn(veChn %d) fail, ret = 0x%x", veChn, ret);
        return AX_FALSE;
    }

    pthread_mutex_lock(&g_objsLock);
    g_objs[veChn] = AX_NULL;
    pthread_mutex_unlock(&g_objsLock);

    pthread_mutex_destroy(&obj->lock);
    FREE(obj);

    LEAVE_FUNC();
    return AX_TRUE;
}

AX_BOOL sample_venc_start_chn(AX_S32 veChn) {
    ENTER_FUNC();

    venc_obj_t *obj = GET_VENC_OBJ(veChn);
    if (stateStarted == obj->state) {
        LOG_M_W(TAG, "veChn %d is started", veChn);
        return AX_TRUE;
    }

    if (0 != pthread_create(&obj->recv_tid, AX_NULL, recv_thread, obj)) {
        LOG_M_E(TAG, "start veChn %d recv thread fail", veChn);
        return AX_FALSE;
    }

    AX_VENC_RECV_PIC_PARAM_T param;
    param.s32RecvPicNum = -1;
    AX_S32 ret = AX_VENC_StartRecvFrame(veChn, &param);
    if (0 != ret) {
        LOG_M_E(TAG, "AX_VENC_StartRecvFrame(veChn %d) failed, ret = 0x%x", veChn, ret);
        return AX_FALSE;
    }

    pthread_mutex_lock(&obj->lock);
    obj->state = stateStarted;
    pthread_mutex_unlock(&obj->lock);

    LEAVE_FUNC();
    return AX_TRUE;
}

AX_BOOL sample_venc_stop_chn(AX_S32 veChn, AX_S32 timeout) {
    ENTER_FUNC();

    venc_obj_t *obj = GET_VENC_OBJ(veChn);
    if (stateStarted != obj->state) {
        LOG_M_W(TAG, "veChn %d is not started", veChn);
        return AX_TRUE;
    }

    AX_S32 ret;

    /* Step1: stop recv frame */
    ret = AX_VENC_StopRecvFrame(veChn);
    if (AX_SUCCESS != ret) {
        LOG_M_E(TAG, "AX_VENC_StopRecvFrame(veChn %d) fail, ret = 0x%x", veChn, ret);
        return AX_FALSE;
    }

    /* Step2: wait all frames encoded done if needed */
    if (0 != timeout) {
        AX_U64 nTick = get_tick_count();
        do {
            AX_VENC_CHN_STATUS_T status;
            ret = AX_VENC_QueryStatus(veChn, &status);
            if (0 == ret) {
                if (0 == (status.u32LeftPics + status.u32LeftStreamFrames)) {
                    break;
                } else {
                    LOG_M_I(TAG, "veChn %d left pic %d, left stream %d", veChn, status.u32LeftPics, status.u32LeftStreamFrames);
                }
            } else {
                LOG_M_E(TAG, "AX_VENC_QueryStatus(veChn %d) fail, ret = 0x%x", veChn, ret);
            }

            msleep(10);

        } while (timeout < 0 || (get_tick_count() - nTick < timeout));
    }

    /* Step3: stop recv thread */
    obj->stop_recv = AX_TRUE;

    /* Step4: reset to wakeup AX_VENC_GetStream(timeout: -1) */
    ret = AX_VENC_ResetChn(veChn);
    if (0 != ret) {
        LOG_M_E(TAG, "AX_VENC_ResetChn(veChn) fail, ret = 0x%x", veChn, ret);
        return AX_FALSE;
    }

    /* Step5: wait recv thread safe quit */
    if (obj->recv_tid) {
        pthread_join(obj->recv_tid, AX_NULL);
        obj->recv_tid = 0;
    }

    pthread_mutex_lock(&obj->lock);
    obj->state = stateInited;
    pthread_mutex_unlock(&obj->lock);

    LEAVE_FUNC();
    return AX_TRUE;
}

AX_BOOL sample_venc_register_sink(AX_S32 veChn, venc_sink_t sink) {
    ENTER_FUNC();

    venc_obj_t *obj = GET_VENC_OBJ(veChn);
    obj->sink = sink;

    LEAVE_FUNC();
    return AX_TRUE;
}

static AX_VOID *recv_thread(AX_VOID *arg) {
    ENTER_FUNC();

    venc_obj_t *obj = (venc_obj_t *)arg;
    obj->stop_recv = AX_FALSE;

    const AX_S32 veChn = obj->veChn;
    AX_S32 ret;
    AX_VENC_STREAM_T stream;
    memset(&stream, 0, sizeof(AX_VENC_STREAM_T));

    while (!obj->stop_recv) {
        /* Step1:  get encoded stream */
        ret = AX_VENC_GetStream(veChn, &stream, -1);
        if (0 != ret) {
            if (AX_ERR_VENC_FLOW_END == ret) {
                LOG_M_I(TAG, "reach eof");
                obj->stop_recv = AX_TRUE;
                break;
            }

            if (AX_ERR_VENC_QUEUE_EMPTY == ret) {
                LOG_M_I(TAG, "no stream in veChn %d queue", veChn);
            } else {
                LOG_M_E(TAG, "AX_VENC_GetStream(veChn %d) fail, ret = 0x%x", veChn, ret);
            }

            continue;
        }

        /* Step2: notify stream */
        if (obj->sink.on_recv_stream) {
            obj->sink.on_recv_stream(veChn, &stream, obj->sink.sinker);
        }

        /* Step3: release stream */
        ret = AX_VENC_ReleaseStream(veChn, &stream);
        if (0 != ret) {
            LOG_M_E(TAG, "AX_VENC_ReleaseStream(veChn %d) fail, ret = 0x%x", veChn, ret);
            continue;
        }
    }

    LEAVE_FUNC();
    return (AX_VOID *)0;
}