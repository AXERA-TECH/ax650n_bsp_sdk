/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef __SAMPLE_VDEC_H__
#define __SAMPLE_VDEC_H__

#include "ax_vdec_api.h"
#include "def.h"

typedef struct {
    /* input stream information */
    AX_U32 input_width;
    AX_U32 input_heigth;
    AX_U32 fps;
    AX_PAYLOAD_TYPE_E payload;
    /*
        if no B frame and decode and display order are sample, set AX_VDEC_OUTPUT_ORDER_DEC to save memory.
        Otherwise set to AX_VDEC_OUTPUT_ORDER_DISP.
    */
    AX_VDEC_OUTPUT_ORDER_E order;
    /*
        if src stream is from file, recommend to set AX_VDEC_DISPLAY_MODE_PLAYBACK
        if src stream is from rtsp, recommend to set AX_VDEC_DISPLAY_MODE_PREVIEW
        refer to VDEC API doc to get more information about playback and preview mode
    */
    AX_VDEC_DISPLAY_MODE_E display_mode;

    /*
        refer to AX_VDEC_CHN_ATTR_T.u32FrameBufCnt of VDEC API doc or AX AE engineer to get more information.
    */
    AX_U32 blk_num;

    /* output */
    AX_U32 output_width;
    AX_U32 output_height;

} vdec_attr_t;

/**
 * @brief VDEC support 3 channels which called PP:
 *          PP0: same width and height with stream, cannot support scaler
 *          PP1: support scaler down, max output is: 4096×4096
 *          PP2: support scaler down, max output is: 1920×1080
 *        sample_vdec_create_grp API just enable PP1 to hit scaler down target.
 *
 * @param attr - refer to VDEC API doc to add more parameters, sample just list the basic parameters.
 * @return AX_S32 - VDEC group id, range: [0 - AX_VDEC_MAX_GRP_NUM]
 */
AX_S32  sample_vdec_create_grp(const vdec_attr_t *attr);
AX_BOOL sample_vdec_destory_grp(AX_S32 vdGrp);

/**
 * @brief Start and stop VDEC
 *
 * @param vdGrp
 * @param timeout - unit: ms
 *                         < 0: wait all streams are decoded and then stop
 *                           0: stop immediatelly
 *                         if rtsp, recommend to 0.
 * @return AX_BOOL
 */
AX_BOOL sample_vdec_start_grp(AX_S32 vdGrp);
AX_BOOL sample_vdec_stop_grp(AX_S32 vdGrp, AX_S32 timeout);

/**
 * @brief Send annexb stream by frame
 *        AX_VDEC_INPUT_MODE_FRAME mode is highly recommeneded and make sure combine SPS,PPS,VPS and IDR to one frame to send
 *        if rtsp, client received frame by protocol
 *        if file, application should identify the start and end of one frame from nalu stream.
 *
 * @param vdGrp
 * @param data - annexb stream frame
 * @param len  - annexb stream frame length in bytes
 * @param pts  - PTS is by pass to next moudle, SDK will not change PTS
 * @return AX_BOOL
 */
AX_BOOL sample_vdec_send_stream(AX_S32 vdGrp, const AX_U8 *data, AX_U32 len, AX_U64 pts);

#endif /* __SAMPLE_VDEC_H__ */
