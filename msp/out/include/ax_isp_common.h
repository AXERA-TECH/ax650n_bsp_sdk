/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_ISP_COMMON_H__
#define __AX_ISP_COMMON_H__

#include "ax_base_type.h"
#include "ax_global_type.h"


#define AX_VIN_MAX_DEV_NUM              (8)
#define AX_VIN_MAX_PIPE_NUM             (16)
#define AX_VIN_MAX_CHN_NUM              (3)
#define AX_VIN_MAX_STITCHGRP_NUM        (4)
#define AX_VIN_STITCH_MAX_PIPE_NUM      (4)

#define AX_HDR_CHN_NUM                  (4)
#define AX_ISP_BAYER_CHN_NUM            (4)
#define AX_VIN_SYNC_CODE_NUM            (4)
#define AX_VIN_LVDS_LANE_NUM            (16)

typedef enum {
    AX_SNS_MODE_NONE   = 0,
    AX_SNS_LINEAR_MODE = 1,
    AX_SNS_HDR_2X_MODE = 2,
    AX_SNS_HDR_3X_MODE = 3,
    AX_SNS_HDR_4X_MODE = 4,
    AX_SNS_HDR_MODE_MAX
} AX_SNS_HDR_MODE_E;

typedef enum {
    AX_SNS_HDR_OUTPUT_MODE_FRAME_BASED = 0,
    AX_SNS_HDR_OUTPUT_MODE_DOL = 1,
    AX_SNS_HDR_OUTPUT_MODE_MAX
} AX_SNS_HDR_OUTPUT_MODE_E;

typedef enum {
    AX_SNS_HDR_FRAME_L      = 0,
    AX_SNS_HDR_FRAME_M      = 1,
    AX_SNS_HDR_FRAME_S      = 2,
    AX_SNS_HDR_FRAME_VS     = 3,
    AX_SNS_HDR_FRAME_MAX
} AX_SNS_HDR_FRAME_E;

typedef enum {
    AX_RT_RAW8                          = 8,        // raw8, 8-bit per pixel
    AX_RT_RAW10                         = 10,       // raw10, 10-bit per pixel
    AX_RT_RAW12                         = 12,       // raw12, 12-bit per pixel
    AX_RT_RAW14                         = 14,       // raw14, 14-bit per pixel
    AX_RT_RAW16                         = 16,       // raw16, 16-bit per pixel
    AX_RT_YUV422                        = 20,       // yuv422
    AX_RT_YUV420                        = 21,       // yuv420
} AX_RAW_TYPE_E;

typedef enum {
    AX_BP_RGGB                          = 0,        // R Gr Gb B bayer pattern
    AX_BP_GRBG                          = 1,        // Gr R B Gb bayer pattern
    AX_BP_GBRG                          = 2,        // Gb B R Gr byaer pattern
    AX_BP_BGGR                          = 3,        // B Gb Gr R byaer pattern
    AX_BP_MONO                          = 4,        // MONO, Gray, IR, etc
    AX_BP_RGBIR4x4                      = 16,       //< RGBIR 4x4
    AX_BP_MAX
} AX_BAYER_PATTERN_E;

typedef enum {
    AX_SNS_NORMAL                       = 0,
    AX_SNS_DOL_HDR                      = 1,
    AX_SNS_BME_HDR                      = 2,
    AX_SNS_QUAD_BAYER_NO_HDR            = 3,
    AX_SNS_QUAD_BAYER_2_HDR_MODE0       = 4,
    AX_SNS_QUAD_BAYER_2_HDR_MODE1       = 5,
    AX_SNS_QUAD_BAYER_2_HDR_MODE2       = 6,
    AX_SNS_QUAD_BAYER_3_HDR_MODE3       = 7,
    AX_SNS_OUTPUT_MODE_MAX
} AX_SNS_OUTPUT_MODE_E;

typedef enum {
    AX_PRIVATE_DATA_MODE_BOTTOM           = 0,
    AX_PRIVATE_DATA_MODE_TOP              = 1,
    AX_PRIVATE_DATA_MODE_MAX
} AX_PRIVATE_DATA_MODE_E;

typedef struct _AX_WIN_AREA_T_ {
    AX_U32                              nStartX;
    AX_U32                              nStartY;
    AX_U32                              nWidth;
    AX_U32                              nHeight;
} AX_WIN_AREA_T;

typedef struct _AX_FRAME_EXP_INFO_T_ {
    AX_U32      nPipeId;
    AX_F32      fExpTime;
    AX_F32      fAGain;
    AX_F32      fDGain;
    AX_F32      fIspGain;
    AX_U32      nHcgLcgMode;        /* 0:HCG 1:LCG 2: Not Support*/
} AX_FRAME_EXP_INFO_T;

typedef struct _AX_ISP_FRAME_T_ {
    AX_RAW_TYPE_E       eRawType;
    AX_SNS_HDR_MODE_E   eHdrMode;
    AX_SNS_HDR_FRAME_E  nHdrFrame;
    AX_BAYER_PATTERN_E  eBayerPattern;
    AX_FRAME_EXP_INFO_T tExpInfo;
} AX_ISP_FRAME_T;


#endif // __AX_ISP_CONNON_H__

