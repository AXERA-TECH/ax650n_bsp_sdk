/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#pragma once
#include "ax_base_type.h"

typedef struct {
    AX_U32 profile_idc;
    AX_U32 level_idc;
    AX_U32 width;
    AX_U32 height;
    AX_U32 fps;
    AX_U32 num_ref_frames;
} SPS_INFO_T;

/**
 * @brief parse SPS and return width, height and fps
 *
 * @param data: start without start code, such as:
 *
 *               1920x1080 30fps
 *               AX_U8 sps264[ ] = {
 *                  0x27,
 *                  0x4D, 0x60, 0x34, 0x89, 0x8D, 0x50, 0x3C, 0x01, 0x13, 0xF2, 0xC2, 0x00, 0x00, 0x03, 0x00, 0x02,
 *                  0x00, 0x00, 0x03, 0x00, 0x78, 0x1E, 0x2C, 0x4D, 0x40
 *               };
 *
 *               1920x1080 25fps
 *               AX_U8 sps265[ ] = {
 *                  0x42, 0x01,
 *                  0x01, 0x01, 0x40, 0x00, 0x00, 0x03, 0x00, 0x80, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x78,
 *                  0xa0, 0x03, 0xc0, 0x80, 0x11, 0x07, 0xcb, 0x90, 0x62, 0xee, 0x46, 0xc0, 0x52, 0x2f, 0xc8, 0x46,
 *                  0xfd, 0x37, 0xb9, 0x78, 0xf5, 0xb9, 0x8a, 0xd6, 0x44, 0xe9, 0x97, 0x1d, 0xc5, 0xf6, 0x02, 0x80,
 *                  0x50, 0x00, 0x00, 0x03, 0x00, 0x10, 0x00, 0x00, 0x03, 0x01, 0x96, 0x01, 0x5e, 0xf7, 0xe0, 0x00,
 *                  0x6b, 0xd9, 0x00, 0x01, 0xae, 0xaa, 0x20
 *               };
 *
 * @param size: bytes of data
 * @param info: return parsed information.
 * @return success return AX_TRUE, otherwise return AX_FALSE
 */
AX_BOOL h264_parse_sps(const AX_U8 *data, AX_U32 size, SPS_INFO_T *info);
AX_BOOL hevc_parse_sps(const AX_U8 *data, AX_U32 size, SPS_INFO_T *info);
