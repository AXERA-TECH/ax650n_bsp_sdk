/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __SAMPLE_CASE_H__
#define __SAMPLE_CASE_H__

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ax_sys_api.h"
#include "ax_venc_api.h"
#include "common_venc.h"
#include "sample_cmd_params.h"
#include "sample_global.h"
#include "sample_venc_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ENCODE_ONCE_NUM (2)

#define MAX_WIDTH_DEFAULT     (16384)
#define MAX_HEIGHT_DEFAULT    (16384)
#define JPEG_ENCODE_ONCE_NAME "JENC_ONCE"
#define DCTSIZE2              (64)

typedef struct axSAMPLE_VENC_ENCODEONCE_PARA_T
{
    AX_BOOL bEncOnceFrmStart;
    VENC_CHN VeChn;
    AX_BOOL bLoopEncode;
    AX_U32 encFrmNum;
    const AX_CHAR *fileInput;
    AX_S32 syncType;
    AX_IMG_FORMAT_E eFmt;
    AX_U32 width;
    AX_U32 height;
    pthread_t encodeOnceFrmPid;
    AX_U32 strideY;
    AX_U32 strideU;
    AX_U32 strideV;
    AX_S32 frameSize;
    AX_S32 strmBufSize;
    AX_U32 qFactor;
    AX_BOOL bCrop;
    AX_U32 cropX; /* horizontal cropping offset */
    AX_U32 cropY; /* vertical cropping offset */
    AX_U32 cropW; /* width of encoded image */
    AX_U32 cropH; /* height of encoded image */
    AX_BOOL roiEnable;
    AX_CHAR *roimapFile;
    AX_U32 qRoiFactor;
    AX_S32 testId;
    AX_BOOL bSaveStrm;
    SAMPLE_VENC_FBC_INFO_T stFbcInfo;
} SAMPLE_VENC_ENCODEONCEFRAME_PARA_T;

static const AX_U8 QTableLuminance[DCTSIZE2] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

static const AX_U8 QTableChrominance[DCTSIZE2] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

typedef struct axSAMPLE_VENC_SETJPEG_PARA_T
{
    AX_U32 qFactor;
    AX_BOOL roiEnable;
    AX_CHAR *roimapFile;
    AX_U32 qRoiFactor;
} SAMPLE_VENC_SETJPEG_PARA_T;

#ifdef __cplusplus
}
#endif

#endif