/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __SAMPLE_ROICFG_H__
#define __SAMPLE_ROICFG_H__

#include <string.h>
#include <stdio.h>
#include "ax_sys_api.h"
#include "ax_venc_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DCTSIZE2 (64)
static const AX_U8 std_luminance_quant_tbl[DCTSIZE2] = {
    16,  11,  10,  16,  24,  40,  51,  61,
    12,  12,  14,  19,  26,  58,  60,  55,
    14,  13,  16,  24,  40,  57,  69,  56,
    14,  17,  22,  29,  51,  87,  80,  62,
    18,  22,  37,  56,  68, 109, 103,  77,
    24,  35,  55,  64,  81, 104, 113,  92,
    49,  64,  78,  87, 103, 121, 120, 101,
    72,  92,  95,  98, 112, 100, 103,  99
};
static const AX_U8 std_chrominance_quant_tbl[DCTSIZE2] = {
    17,  18,  24,  47,  99,  99,  99,  99,
    18,  21,  26,  66,  99,  99,  99,  99,
    24,  26,  56,  99,  99,  99,  99,  99,
    47,  66,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99
};

typedef struct axSAMPLE_ROI_CFG_T {
    AX_U32 qFactor;
    AX_BOOL roiEnable;
    AX_CHAR *roimapFile;
    AX_U32 qRoiFactor;
} SAMPLE_ROI_CFG_T;

AX_S32 SampleJpegRoiCfg(SAMPLE_ROI_CFG_T *pCmdl, AX_VENC_JPEG_PARAM_T *pStJpegParam);
#ifdef __cplusplus
}
#endif
#endif