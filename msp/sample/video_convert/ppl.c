/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ppl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "event.h"
#include "log.h"
#include "stream.h"
#include "utils.h"
#include "vdec.h"
#include "venc.h"

#define TAG "PPL"

typedef struct {
    AX_BOOL is_rtsp;
    AX_U32 width;
    AX_U32 height;
    AX_PAYLOAD_TYPE_E payload;
    AX_VDEC_OUTPUT_ORDER_E order;

    AX_HANDLE stream;
    AX_HANDLE stream_event;
    AX_S32 vdGrp;
    AX_S32 veChn;
    FILE *fp; /* file to save encoded stream */
} ppl_obj_t;

AX_BOOL sample_sys_init(AX_VOID) {
    ENTER_FUNC();

    AX_S32 ret;

    ret = AX_SYS_Init();
    if (0 != ret) {
        LOG_M_E(TAG, "AX_SYS_Init() fail, ret = 0x%x", ret);
        return AX_FALSE;
    }

    (AX_VOID) AX_POOL_Exit();

    AX_VDEC_MOD_ATTR_T vdec_attr;
    memset(&vdec_attr, 0, sizeof(vdec_attr));
    vdec_attr.u32MaxGroupCount = AX_VDEC_MAX_GRP_NUM; /* can be configured by max. decoded grp num */
    vdec_attr.enDecModule = AX_ENABLE_ONLY_VDEC;
    ret = AX_VDEC_Init(&vdec_attr);
    if (0 != ret) {
        LOG_M_E(TAG, "AX_VDEC_Init() fail, ret = 0x%x", ret);
        return AX_FALSE;
    }

    AX_VENC_MOD_ATTR_T venc_attr;
    memset(&venc_attr, 0, sizeof(venc_attr));
    venc_attr.enVencType = AX_VENC_VIDEO_ENCODER;
    venc_attr.stModThdAttr.u32TotalThreadNum = 8;
    ret = AX_VENC_Init(&venc_attr);
    if (0 != ret) {
        LOG_M_E(TAG, "AX_VENC_Init() fail, ret = 0x%x", ret);
        return AX_FALSE;
    }

    LEAVE_FUNC();
    return AX_TRUE;
}

AX_BOOL sample_sys_deinit(AX_VOID) {
    ENTER_FUNC();

    AX_S32 ret;
    ret = AX_VENC_Deinit();
    if (0 != ret) {
        LOG_M_E(TAG, "AX_VENC_Deinit() fail, ret = 0x%x", ret);
        return AX_FALSE;
    }

    ret = AX_VDEC_Deinit();
    if (0 != ret) {
        LOG_M_E(TAG, "AX_VDEC_Deinit() fail, ret = 0x%x", ret);
        return AX_FALSE;
    }

    ret = AX_POOL_Exit();
    if (0 != ret) {
        LOG_M_E(TAG, "AX_POOL_Exit() fail, ret = 0x%x", ret);
        return AX_FALSE;
    }

    ret = AX_SYS_Deinit();
    if (0 != ret) {
        LOG_M_E(TAG, "AX_SYS_Deinit() fail, ret = 0x%x", ret);
        return AX_FALSE;
    }

    LEAVE_FUNC();
    return AX_TRUE;
}

static AX_VOID on_recv_encoded_stream(AX_S32 veChn, AX_VENC_STREAM_T *stream, AX_VOID *sinker) {
    // printf("veChn %d seq %lld, len %d\n", veChn, stream->stPack.u64SeqNum, stream->stPack.u32Len);
    ppl_obj_t *ppl = (ppl_obj_t *)sinker;
    if (ppl->fp) {
        fwrite(stream->stPack.pu8Addr, 1, stream->stPack.u32Len, ppl->fp);
    }
}

static AX_BOOL source_is_rtsp(const AX_CHAR *fpath) {
    const AX_CHAR *prefix = "rtsp://";
    return (0 == strncasecmp(fpath, prefix, strlen(prefix))) ? AX_TRUE : AX_FALSE;
}

static AX_BOOL create_codec_objs(AX_VOID *obj, const stream_info_t *info) {
    ppl_obj_t *ppl = (ppl_obj_t *)obj;

    /**
     * step1: create vdec object
     *    as for sample, only support local IP stream file, so set to playback mode and AX_VDEC_OUTPUT_ORDER_DEC.
     *    application can change by source type (rtsp or file)
     *      rtsp: AX_VDEC_DISPLAY_MODE_PREVIEW
     *      file: AX_VDEC_DISPLAY_MODE_PLAYBACK
     *
     *    IPB: AX_VDEC_OUTPUT_ORDER_DISP
     */
    vdec_attr_t vdec_attr;
    memset(&vdec_attr, 0, sizeof(vdec_attr));
    vdec_attr.input_width = info->width;
    vdec_attr.input_heigth = info->height;
    vdec_attr.fps = info->fps;
    vdec_attr.payload = info->payload;
    vdec_attr.order = ppl->order;
    vdec_attr.display_mode = ppl->is_rtsp ? AX_VDEC_DISPLAY_MODE_PREVIEW : AX_VDEC_DISPLAY_MODE_PLAYBACK;

    /* just for sample, set fixed 8 blk_num, application can set a proper value to decrease memory usage */
    vdec_attr.blk_num = 8;
    vdec_attr.output_width = ppl->width;
    vdec_attr.output_height = ppl->height;
    ppl->vdGrp = sample_vdec_create_grp(&vdec_attr);
    if (INVALID_VDEC_GRP == ppl->vdGrp) {
        return AX_FALSE;
    }

    /**
     * step2: create venc object
     *    as for sample, RC is set to fixed value and bitrate is 4Kbps.
     *    recommend application to change to proper parameters.
     */
    venc_attr_t venc_attr;
    memset(&venc_attr, 0, sizeof(venc_attr));
    venc_attr.payload = ppl->payload;
    venc_attr.width = ppl->width;
    venc_attr.height = ppl->height;
    venc_attr.fps = info->fps;
    if (PT_H264 == ppl->payload) {
        venc_attr.profile = AX_VENC_H264_MAIN_PROFILE;
        venc_attr.level = AX_VENC_H264_LEVEL_5_2;

        venc_attr.rc.s32FirstFrameStartQp = 10;
        venc_attr.rc.enRcMode = AX_VENC_RC_MODE_H264CBR;
        venc_attr.rc.stH264Cbr.u32Gop = info->fps;
        venc_attr.rc.stH264Cbr.u32BitRate = 4096;
        venc_attr.rc.stH264Cbr.u32MinQp = 10;
        venc_attr.rc.stH264Cbr.u32MaxQp = 51;
        venc_attr.rc.stH264Cbr.u32MinIQp = 10;
        venc_attr.rc.stH264Cbr.u32MaxIQp = 51;
        venc_attr.rc.stH264Cbr.u32MinIprop = 10;
        venc_attr.rc.stH264Cbr.u32MaxIprop = 40;
        venc_attr.rc.stH264Cbr.s32IntraQpDelta = -1;

    } else {
        venc_attr.profile = AX_VENC_HEVC_MAIN_PROFILE;
        venc_attr.level = AX_VENC_HEVC_LEVEL_5_1;
        venc_attr.tile = AX_VENC_HEVC_MAIN_TIER;

        venc_attr.rc.s32FirstFrameStartQp = 10;
        venc_attr.rc.enRcMode = AX_VENC_RC_MODE_H265CBR;
        venc_attr.rc.stH265Cbr.u32Gop = info->fps;
        venc_attr.rc.stH265Cbr.u32BitRate = 4096;
        venc_attr.rc.stH265Cbr.u32MinQp = 10;
        venc_attr.rc.stH265Cbr.u32MaxQp = 51;
        venc_attr.rc.stH265Cbr.u32MinIQp = 10;
        venc_attr.rc.stH265Cbr.u32MaxIQp = 51;
        venc_attr.rc.stH265Cbr.u32MinIprop = 30;
        venc_attr.rc.stH265Cbr.u32MaxIprop = 40;
        venc_attr.rc.stH265Cbr.s32IntraQpDelta = -1;
    }
    ppl->veChn = sample_venc_create_chn(&venc_attr);
    if (INVALID_VENC_CHN == ppl->veChn) {
        sample_vdec_destory_grp(ppl->vdGrp);
        return AX_FALSE;
    }

    /* register encoded stream handler */
    venc_sink_t vencSink = {
        .on_recv_stream = on_recv_encoded_stream,
        .sinker = ppl,
    };

    sample_venc_register_sink(ppl->veChn, vencSink);

    /* open file to save encoded stream */
    AX_CHAR fpath[MAX_PATH];
    sprintf(fpath, "/opt/data/output%d_%dx%d.%s", ppl->vdGrp, ppl->width, ppl->height, (PT_H265 == ppl->payload) ? "265" : "264");
    ppl->fp = fopen(fpath, "wb");
    if (!ppl->fp) {
        sample_vdec_destory_grp(ppl->vdGrp);
        sample_venc_destory_chn(ppl->veChn);
        LOG_M_E(TAG, "open %s to save encoded stream fail", fpath);
        return AX_FALSE;
    }

    /* wakeup to start transfering */
    sample_set_event(ppl->stream_event);
    return AX_TRUE;
}

static AX_VOID on_stream_probe_event(const stream_info_t *info, AX_VOID *obj) {
    LOG_M_C(TAG, "%s: probe %dx%d, codec %d, fps %d", info->url, info->width, info->height, info->payload, info->fps);
    create_codec_objs(obj, info);
}

static AX_VOID on_stream_eof_event(const AX_CHAR *url, AX_U32 frame_num, AX_VOID *obj) {
    LOG_M_C(TAG, "%s: eof, total frame num %d", url, frame_num);

    ppl_obj_t *ppl = (ppl_obj_t *)obj;
    sample_set_event(ppl->stream_event);
}

static AX_VOID on_send_stream(const AX_U8 *data, AX_U32 len, AX_U64 pts, AX_VOID *sinker) {
    ppl_obj_t *ppl = (ppl_obj_t *)sinker;
    sample_vdec_send_stream(ppl->vdGrp, data, len, pts);
}

static AX_BOOL sample_link_codec(ppl_obj_t *ppl) {
    AX_MOD_INFO_T src = {AX_ID_VDEC, ppl->vdGrp, VDEC_PP1};
    AX_MOD_INFO_T dst = {AX_ID_VENC, 0, ppl->veChn};
    AX_S32 ret = AX_SYS_Link(&src, &dst);
    if (0 != ret) {
        LOG_M_E(TAG, "AX_SYS_Link(VDEC-%d-%d -> VENC-%d-%d) fail, ret = 0x%x", ppl->vdGrp, VDEC_PP1, 0, ppl->veChn, ret);
        return AX_FALSE;
    }

    return AX_TRUE;
}

static AX_BOOL sample_unlink_codec(ppl_obj_t *ppl) {
    AX_MOD_INFO_T src = {AX_ID_VDEC, ppl->vdGrp, VDEC_PP1};
    AX_MOD_INFO_T dst = {AX_ID_VENC, 0, ppl->veChn};
    AX_S32 ret = AX_SYS_UnLink(&src, &dst);
    if (0 != ret) {
        LOG_M_E(TAG, "AX_SYS_UnLink(VDEC-%d-%d -> VENC-%d-%d) fail, ret = 0x%x", ppl->vdGrp, VDEC_PP1, 0, ppl->veChn, ret);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_HANDLE sample_create_ppl(const AX_CHAR *fpath, AX_PAYLOAD_TYPE_E payload, AX_U32 width, AX_U32 height, AX_S32 order) {
    ppl_obj_t *ppl = CALLOC(1, sizeof(ppl_obj_t));
    if (!ppl) {
        LOG_M_E(TAG, "create ppl object fail, url %s", fpath);
        return AX_INVALID_HANDLE;
    }

    ppl->stream_event = sample_create_event("stream");
    if (AX_INVALID_HANDLE == ppl->stream_event) {
        FREE(ppl);
        return AX_INVALID_HANDLE;
    }

    ppl->payload = payload;
    ppl->width = width;
    ppl->height = height;
    ppl->vdGrp = -1;
    ppl->veChn = -1;
    ppl->is_rtsp = source_is_rtsp(fpath) ? AX_TRUE : AX_FALSE;
    ppl->order = (order == AX_VDEC_OUTPUT_ORDER_DISP) ? AX_VDEC_OUTPUT_ORDER_DISP : AX_VDEC_OUTPUT_ORDER_DEC;

    stream_attr_t stream_attr;
    memset(&stream_attr, 0, sizeof(stream_attr));
    strcpy(stream_attr.url, fpath);
    stream_attr.event.probe = on_stream_probe_event;
    stream_attr.event.eof = on_stream_eof_event;
    stream_attr.event.obj = (AX_VOID *)ppl;
    ppl->stream = sample_stream_create(&stream_attr);
    if (AX_INVALID_HANDLE == ppl->stream) {
        FREE(ppl);
        return AX_INVALID_HANDLE;
    }

    LEAVE_FUNC();
    return (AX_HANDLE)ppl;
}

AX_BOOL sample_destory_ppl(AX_HANDLE handle) {
    ENTER_FUNC();

    ppl_obj_t *ppl = (ppl_obj_t *)handle;
    if (!ppl) {
        LOG_M_E(TAG, "obj is nil");
        return AX_FALSE;
    }

    if (INVALID_VENC_CHN != ppl->veChn) {
        sample_venc_destory_chn(ppl->veChn);
        ppl->veChn = INVALID_VENC_CHN;
    }

    if (INVALID_VDEC_GRP != ppl->vdGrp) {
        sample_vdec_destory_grp(ppl->vdGrp);
        ppl->vdGrp = INVALID_VDEC_GRP;
    }

    if (AX_INVALID_HANDLE != ppl->stream) {
        sample_stream_destory(ppl->stream);
        ppl->stream = AX_INVALID_HANDLE;
    }

    if (ppl->stream_event) {
        sample_destory_event(ppl->stream_event);
        ppl->stream_event = AX_INVALID_HANDLE;
    }

    FREE(ppl);

    LEAVE_FUNC();
    return AX_TRUE;
}

AX_BOOL sample_start_ppl(AX_HANDLE handle) {
    ENTER_FUNC();

    ppl_obj_t *ppl = (ppl_obj_t *)handle;
    if (!ppl) {
        LOG_M_E(TAG, "obj is nil");
        return AX_FALSE;
    }

    /* wait stream is probed and codec objects are created */
    sample_wait_event(ppl->stream_event, -1);
    sample_reset_event(ppl->stream_event);

    if (!sample_link_codec(ppl)) {
        return AX_FALSE;
    }

    if (!sample_venc_start_chn(ppl->veChn)) {
        sample_unlink_codec(ppl);
        return AX_FALSE;
    }

    if (!sample_vdec_start_grp(ppl->vdGrp)) {
        sample_unlink_codec(ppl);
        sample_venc_stop_chn(ppl->veChn, 0);
        return AX_FALSE;
    }

    stream_sink_t streamSink = {
        .on_send_stream = on_send_stream,
        .sinker = (AX_VOID *)ppl,
    };

    sample_stream_register_sink(ppl->stream, streamSink);

    if (!sample_stream_start(ppl->stream)) {
        sample_unlink_codec(ppl);
        sample_venc_stop_chn(ppl->veChn, 0);
        sample_vdec_stop_grp(ppl->vdGrp, 0);
        return AX_FALSE;
    }

    LEAVE_FUNC();
    return AX_TRUE;
}

AX_BOOL sample_wait_ppl_eof(AX_HANDLE handle, AX_S32 timeout) {
    ppl_obj_t *ppl = (ppl_obj_t *)handle;
    if (!ppl) {
        LOG_M_E(TAG, "obj is nil");
        return AX_FALSE;
    }

    if (ppl->is_rtsp) {
        msleep(timeout);
        return AX_FALSE;
    } else {
        return sample_wait_event(ppl->stream_event, timeout);
    }
}

AX_BOOL sample_stop_ppl(AX_HANDLE handle) {
    ENTER_FUNC();

    ppl_obj_t *ppl = (ppl_obj_t *)handle;
    if (!ppl) {
        LOG_M_E(TAG, "obj is nil");
        return AX_FALSE;
    }

    /**
     * if rtsp, stop immediately, otherwise wait all frames are finished.
     */
    if (!ppl->is_rtsp) {
        /* Step1: stop stream */
        sample_stream_stop(ppl->stream);

        /* Step2: stop decoder */
        sample_vdec_stop_grp(ppl->vdGrp, -1);

        /* Step3: stop encoder */
        sample_venc_stop_chn(ppl->veChn, -1);

        /* Step4: unlink src and dst */
        sample_unlink_codec(ppl);
    } else {
        /* Step1: unlink src and dst */
        sample_unlink_codec(ppl);

        /* Step2: stop stream */
        sample_stream_stop(ppl->stream);

        /* Step3: stop decoder */
        sample_vdec_stop_grp(ppl->vdGrp, 0);

        /* Step4: stop encoder */
        sample_venc_stop_chn(ppl->veChn, 0);
    }

    if (ppl->fp) {
        fclose(ppl->fp);
        ppl->fp = AX_NULL;
    }

    LEAVE_FUNC();
    return AX_TRUE;
}
