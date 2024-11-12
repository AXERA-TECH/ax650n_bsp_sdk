/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "vdec.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "ax_buffer_tool.h"
#include "log.h"
#include "utils.h"

#define TAG "VDEC"

enum {
    stateUnknown = 0,
    stateInited = 1,
    stateStarted = 2,
};

typedef struct {
    pthread_mutex_t lock;
    AX_S32 vdGrp;
    AX_U32 state;
    vdec_attr_t attr;
    AX_S32 send_timeout;
} vdec_obj_t;

static vdec_obj_t *g_objs[AX_VDEC_MAX_GRP_NUM] = {AX_NULL};
static pthread_mutex_t g_objsLock = PTHREAD_MUTEX_INITIALIZER;

#define GET_VDEC_OBJ(vdGrp)                                             \
    ({                                                                  \
        if (vdGrp < 0 || vdGrp >= AX_VDEC_MAX_GRP_NUM) {                \
            LOG_M_E(TAG, "invalid vdGrp %d", vdGrp);                    \
            return AX_FALSE;                                            \
        }                                                               \
                                                                        \
        vdec_obj_t *obj = g_objs[vdGrp];                                \
        if (!obj || !obj->state) {                                      \
            LOG_M_E(TAG, "vdGrp %d is not inited, obj %p", vdGrp, obj); \
            return AX_FALSE;                                            \
        }                                                               \
                                                                        \
        obj;                                                            \
    })

AX_S32 sample_vdec_create_grp(const vdec_attr_t *attr) {
    ENTER_FUNC();

    vdec_obj_t *obj = CALLOC(1, sizeof(vdec_obj_t));
    if (!obj) {
        LOG_M_E(TAG, "create vdec obj fail");
        return INVALID_VDEC_GRP;
    }

    obj->attr = *attr;
    obj->vdGrp = INVALID_VDEC_GRP;
    obj->state = stateUnknown;

    pthread_mutex_init(&obj->lock, AX_NULL);

    /* find an unused vdGrp */
    pthread_mutex_lock(&g_objsLock);
    for (AX_S32 i = 0; i < AX_VDEC_MAX_GRP_NUM; ++i) {
        if (!g_objs[i]) {
            /* mark vdGrp busy */
            obj->vdGrp = i;
            g_objs[i] = obj;
            break;
        }
    }
    pthread_mutex_unlock(&g_objsLock);

    if (INVALID_VDEC_GRP == obj->vdGrp) {
        LOG_M_E(TAG, "all vdGrps are used");
        pthread_mutex_destroy(&obj->lock);
        FREE(obj);
        return INVALID_VDEC_GRP;
    }

    AX_S32 ret;

    enum {
        stateGrpCreated = (1 << 0),
        stateChnEnabled = (1 << 1),
    };

    AX_U32 state = 0;
    do {
        /* Step1: Create and configure VDEC group */
        AX_VDEC_GRP_ATTR_T grp_attr;
        memset(&grp_attr, 0, sizeof(grp_attr));
        grp_attr.enCodecType = attr->payload;
        grp_attr.enInputMode = AX_VDEC_INPUT_MODE_FRAME;
        grp_attr.u32MaxPicWidth = ALIGN_UP(attr->input_width, 16);   /* H264 MB 16x16 */
        grp_attr.u32MaxPicHeight = ALIGN_UP(attr->input_heigth, 16); /* H264 MB 16x16 */

        /* input stream buffer size：make sure VDEC can hold one nalu frame at least */
        grp_attr.u32StreamBufSize = grp_attr.u32MaxPicWidth * grp_attr.u32MaxPicHeight * 2;

        /* private pool is highly recommended */
        grp_attr.bSdkAutoFramePool = AX_TRUE;
        ret = AX_VDEC_CreateGrp(obj->vdGrp, &grp_attr);
        if (0 != ret) {
            LOG_M_E(TAG, "AX_VDEC_CreateGrp(vdGrp %d) fail, ret = 0x%x", obj->vdGrp, ret);
            break;
        } else {
            state |= stateGrpCreated;
        }

        AX_VDEC_GRP_PARAM_T grp_param;
        memset(&grp_param, 0, sizeof(grp_param));
        /* if no B frame, can replaced by VIDEO_DEC_MODE_IP */
        grp_param.stVdecVideoParam.enVdecMode = VIDEO_DEC_MODE_IPB;
        grp_param.stVdecVideoParam.enOutputOrder = attr->order;
        ret = AX_VDEC_SetGrpParam(obj->vdGrp, &grp_param);
        if (0 != ret) {
            LOG_M_E(TAG, "AX_VDEC_SetGrpParam(vdGrp %d) fail, ret = 0x%x", obj->vdGrp, ret);
            break;
        }

        ret = AX_VDEC_SetDisplayMode(obj->vdGrp, attr->display_mode);
        if (0 != ret) {
            LOG_M_E(TAG, "AX_VDEC_SetGrpParam(vdGrp %d) fail, ret = 0x%x", obj->vdGrp, ret);
            break;
        }

        /* playback recommend to -1, preview mode can be adjust, unit is ms. */
        obj->send_timeout = (AX_VDEC_DISPLAY_MODE_PLAYBACK == attr->display_mode) ? -1 : 1000;

        /*
            Step2: configure and enable channel PP
                PP0: same width and height with stream, cannot support scaler
                PP1: support scaler down, max output is: 4096×4096
                PP2: support scaler down, max output is: 1920×1080

                Here we only use PP1 one channel.
        */
        for (AX_VDEC_CHN vdChn = 0; vdChn < AX_VDEC_MAX_CHN_NUM; ++vdChn) {
            if (VDEC_PP1 == vdChn) {
                AX_VDEC_CHN_ATTR_T chn_attr;
                memset(&chn_attr, 0, sizeof(chn_attr));
                chn_attr.u32PicWidth = attr->output_width;
                chn_attr.u32PicHeight = attr->output_height;
                /* VDEC stride is 256 whatever FBC or no FBC */
                chn_attr.u32FrameStride = ALIGN_UP(chn_attr.u32PicWidth, VDEC_STRIDE_ALIGN);

                /* if linked mode (eg: VDEC link VENC), set depth to 0, otherwise should configure depth > 0 */
                chn_attr.u32OutputFifoDepth = 0;

                chn_attr.enOutputMode = AX_VDEC_OUTPUT_SCALE;
                chn_attr.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;

                /*
                    active lossy 4 mode to save DDR bitwidth and memory
                    AX_COMPRESS_MODE_LOSSLESS is not supported by VDEC
                */
                chn_attr.stCompressInfo.enCompressMode = AX_COMPRESS_MODE_LOSSY;
                chn_attr.stCompressInfo.u32CompressLevel = 4;

                chn_attr.u32FrameBufCnt = attr->blk_num;
                chn_attr.u32FrameBufSize = AX_VDEC_GetPicBufferSize(chn_attr.u32PicWidth, chn_attr.u32PicHeight, chn_attr.enImgFormat,
                                                                    &chn_attr.stCompressInfo, grp_attr.enCodecType);

                LOG_M_I(TAG, "vdGrp %d vdChn %d: %dx%d stride %d, pix %d, FBC %d(lv %d), blkCnt %d blkSize %d, depth %d", obj->vdGrp, vdChn,
                        chn_attr.u32PicWidth, chn_attr.u32PicHeight, chn_attr.u32FrameStride, chn_attr.enImgFormat,
                        chn_attr.stCompressInfo.enCompressMode, chn_attr.stCompressInfo.u32CompressLevel, chn_attr.u32FrameBufCnt,
                        chn_attr.u32FrameBufSize, chn_attr.u32OutputFifoDepth);

                ret = AX_VDEC_SetChnAttr(obj->vdGrp, vdChn, &chn_attr);
                if (0 != ret) {
                    LOG_M_E(TAG, "AX_VDEC_SetChnAttr(vdGrp %d, vdChn %d) fail, ret = 0x%x", obj->vdGrp, vdChn, ret);
                    break;
                }

                ret = AX_VDEC_EnableChn(obj->vdGrp, vdChn);
                if (0 != ret) {
                    LOG_M_E(TAG, "AX_VDEC_EnableChn(vdGrp %d, vdChn %d) fail, ret = 0x%x", obj->vdGrp, vdChn, ret);
                    break;
                }

                state |= stateChnEnabled;

            } else {
                ret = AX_VDEC_DisableChn(obj->vdGrp, vdChn);
                if (0 != ret) {
                    LOG_M_E(TAG, "AX_VDEC_DisableChn(vdGrp %d, vdChn %d) fail, ret = 0x%x", obj->vdGrp, vdChn, ret);
                    break;
                }
            }
        }

        pthread_mutex_lock(&obj->lock);
        obj->state = stateInited;
        pthread_mutex_unlock(&obj->lock);

        LEAVE_FUNC();
        return obj->vdGrp;

    } while (0);

    /* failure, destory resources */
    if (stateChnEnabled == (state & stateChnEnabled)) {
        for (AX_VDEC_CHN vdChn = 0; vdChn < AX_VDEC_MAX_CHN_NUM; ++vdChn) {
            AX_VDEC_DisableChn(obj->vdGrp, vdChn);
        }
    }

    if (stateGrpCreated == (state & stateGrpCreated)) {
        AX_VDEC_DestroyGrp(obj->vdGrp);
    }

    pthread_mutex_lock(&g_objsLock);
    g_objs[obj->vdGrp] = AX_NULL;
    pthread_mutex_unlock(&g_objsLock);

    pthread_mutex_destroy(&obj->lock);
    FREE(obj);

    return INVALID_VDEC_GRP;
}

AX_BOOL sample_vdec_destory_grp(AX_S32 vdGrp) {
    ENTER_FUNC();

    vdec_obj_t *obj = GET_VDEC_OBJ(vdGrp);

    AX_S32 ret;
    for (AX_VDEC_CHN vdChn = 0; vdChn < AX_VDEC_MAX_CHN_NUM; ++vdChn) {
        ret = AX_VDEC_DisableChn(vdGrp, vdChn);
        if (0 != ret) {
            LOG_M_E(TAG, "AX_VDEC_DisableChn(vdGrp %d, vdChn %d) fail, ret = 0x%x", vdGrp, vdChn, ret);
            return AX_FALSE;
        }
    }

    ret = AX_VDEC_DestroyGrp(vdGrp);
    if (0 != ret) {
        LOG_M_E(TAG, "AX_VDEC_DestroyGrp(vdGrp %d) fail, ret = 0x%x", vdGrp, ret);
        return AX_FALSE;
    }

    pthread_mutex_lock(&g_objsLock);
    g_objs[vdGrp] = AX_NULL;
    pthread_mutex_unlock(&g_objsLock);

    pthread_mutex_destroy(&obj->lock);
    FREE(obj);

    LEAVE_FUNC();
    return AX_TRUE;
}

AX_BOOL sample_vdec_start_grp(AX_S32 vdGrp) {
    ENTER_FUNC();

    vdec_obj_t *obj = GET_VDEC_OBJ(vdGrp);
    if (stateStarted == obj->state) {
        LOG_M_W(TAG, "vdGrp %d is started", vdGrp);
        return AX_TRUE;
    }

    AX_VDEC_RECV_PIC_PARAM_T param;
    memset(&param, 0, sizeof(param));
    param.s32RecvPicNum = -1;
    AX_S32 ret = AX_VDEC_StartRecvStream(vdGrp, &param);
    if (0 != ret) {
        LOG_M_E(TAG, "AX_VDEC_StartRecvStream(vdGrp %d) fail, ret = 0x%x", vdGrp);
        return AX_FALSE;
    }

    pthread_mutex_lock(&obj->lock);
    obj->state = stateStarted;
    pthread_mutex_unlock(&obj->lock);

    LEAVE_FUNC();
    return AX_TRUE;
}

AX_BOOL sample_vdec_stop_grp(AX_S32 vdGrp, AX_S32 timeout) {
    ENTER_FUNC();

    vdec_obj_t *obj = GET_VDEC_OBJ(vdGrp);
    if (stateStarted != obj->state) {
        LOG_M_W(TAG, "vdGrp %d is not started", vdGrp);
        return AX_TRUE;
    }

    AX_S32 ret;

    /* step1: stop recv stream at first */
    ret = AX_VDEC_StopRecvStream(vdGrp);
    if (0 != ret) {
        LOG_M_E(TAG, "AX_VDEC_StopRecvStream(vdGrp %d) fail, ret = 0x%x", vdGrp, ret);
        return AX_FALSE;
    }

    if (0 != timeout) {
        /* step2: make sure all frames are decoded done if needed */
        AX_U64 nTick = get_tick_count();
        do {
            AX_VDEC_GRP_STATUS_T status;
            ret = AX_VDEC_QueryStatus(vdGrp, &status);
            if (0 == ret) {
                if (0 == (status.u32LeftStreamFrames + status.u32LeftPics[0] + status.u32LeftPics[1] + status.u32LeftPics[2])) {
                    break;
                } else {
                    LOG_M_I(TAG, "vdGrp %d left stream %d, left pic %d - %d - %d", vdGrp, status.u32LeftStreamFrames, status.u32LeftPics[0],
                            status.u32LeftPics[1], status.u32LeftPics[2]);
                }
            } else {
                LOG_M_E(TAG, "AX_VDEC_QueryStatus(vdGrp %d) fail, ret = 0x%x", vdGrp, ret);
            }

            msleep(10);

        } while (timeout < 0 || (get_tick_count() - nTick < timeout));
    }

    /* step3: reset grp, retry to make sure reset success */
    for (AX_U32 i = 0; i < 5; ++i) {
        ret = AX_VDEC_ResetGrp(vdGrp);
        if (0 == ret) {
            break;
        }

        if (AX_ERR_VDEC_BUSY == ret) {
            LOG_M_W(TAG, "vdGrp %d is busy, try again", vdGrp);
        } else {
            LOG_M_E(TAG, "AX_VDEC_ResetGrp(vdGrp %d) fail, ret = 0x%x, try again", vdGrp, ret);
        }

        msleep(10);
    }

    if (0 != ret) {
        LOG_M_E(TAG, "AX_VDEC_ResetGrp(vdGrp %d) failed, ret = 0x%x", vdGrp, ret);
        return AX_FALSE;
    }

    pthread_mutex_lock(&obj->lock);
    obj->state = stateInited;
    pthread_mutex_unlock(&obj->lock);

    LEAVE_FUNC();
    return AX_TRUE;
}

AX_BOOL sample_vdec_send_stream(AX_S32 vdGrp, const AX_U8 *data, AX_U32 len, AX_U64 pts) {
    vdec_obj_t *obj = GET_VDEC_OBJ(vdGrp);
    if (stateStarted != obj->state) {
        LOG_M_W(TAG, "vdGrp %d is not started", vdGrp);
        return AX_TRUE;
    }

    AX_VDEC_STREAM_T stream;
    memset(&stream, 0, sizeof(stream));
    /* as we uses AX_VDEC_INPUT_MODE_FRAME, bEndofFame always AX_TRUE */
    stream.bEndOfFrame = AX_TRUE;
    if (data && len > 0) {
        stream.u64PTS = pts;
        stream.pu8Addr = (AX_U8 *)data;
        stream.u32StreamPackLen = len;
    } else {
        /* end of all stream */
        stream.bEndOfStream = AX_TRUE;
    }

    AX_S32 ret = AX_VDEC_SendStream(vdGrp, &stream, obj->send_timeout);
    if (0 != ret) {
        if ((AX_ERR_VDEC_BUF_FULL == ret) || (AX_ERR_VDEC_QUEUE_FULL == ret)) {
            LOG_M_E(TAG, "vdGrp %d input buffer is full, abandon %d bytes, pts %lld", vdGrp, len, pts);
        } else if (AX_ERR_VDEC_NOT_PERM == ret) {
            LOG_M_E(TAG, "AX_VDEC_SendStream(vdGrp %d, len %d, pts %lld, timeout %d) not permitted", vdGrp, len, pts, obj->send_timeout);
        } else {
            LOG_M_E(TAG, "AX_VDEC_SendStream(vdGrp %d, len %d, pts %lld, timeout %d) fail, ret = 0x%x", vdGrp, len, pts, obj->send_timeout,
                    ret);
        }

        return AX_FALSE;
    }

    return AX_TRUE;
}