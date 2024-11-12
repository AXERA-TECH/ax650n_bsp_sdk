/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _SAMPLE_UTILS_H_
#define _SAMPLE_UTILS_H_

#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>

#include "AXRtspWrapper.h"
#include "ax_ivps_api.h"
#include "ax_sys_api.h"
#include "sample_ivps_hal.h"
#include "sample_venc_hal.h"
#include "sample_vin_hal.h"
#include "sample_engine.h"

/* #define __SAMPLE_LOG_EN__ */

#ifdef __SAMPLE_LOG_EN__
#define ALOGD(fmt, ...) \
    printf("\033[1;30;37mDEBUG  :[%s:%d] " fmt "\033[0m\n", __func__, __LINE__, ##__VA_ARGS__)  // white
#define ALOGI(fmt, ...) \
    printf("\033[1;30;32mINFO   :[%s:%d] " fmt "\033[0m\n", __func__, __LINE__, ##__VA_ARGS__)  // green
#else
#define ALOGD(fmt, ...) \
    do {                \
    } while (0)
#define ALOGI(fmt, ...) \
    do {                \
    } while (0)
#endif
#define ALOGW(fmt, ...) \
    printf("\033[1;30;33mWARN   :[%s:%d] " fmt "\033[0m\n", __func__, __LINE__, ##__VA_ARGS__)  // yellow
#define ALOGE(fmt, ...) \
    printf("\033[1;30;31mERROR  :[%s:%d] " fmt "\033[0m\n", __func__, __LINE__, ##__VA_ARGS__)  // red
#define ALOGN(fmt, ...) \
    printf("\033[1;30;37mINFO   :[%s:%d] " fmt "\033[0m\n", __func__, __LINE__, ##__VA_ARGS__)  // white

#define SAMPLE_PHY_MEM_ALIGN_SIZE (16)

#define ARRAY_SIZE(array)  sizeof(array) / sizeof(array[0])
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))

#define SAMPLE_MIN_DELTAPTS_NUM (30)

typedef struct
{
    AX_U16 nW;
    AX_U16 nH;
    AX_U32 nStride;
    AX_IMG_FORMAT_E eFormat;
    AX_U32 nPhyAddr;
    AX_VOID *pVirAddr;
    AX_POOL PoolId;
    AX_BLK BlkId;
} SAMPLE_IMAGE_T;

typedef struct
{
    char *pImgFile;
    SAMPLE_IMAGE_T tImage;
    AX_IVPS_RECT_T tRect;

    AX_U32 nAlpha;
    AX_U32 nChn;
    AX_U32 nColor; /* for rectangle */
    AX_U32 nLineW; /* for rectangle */
    AX_IVPS_MOSAIC_BLK_SIZE_E eBlkSize;
} SAMPLE_IMAGE_INFO_T;

typedef struct
{
    AX_U32 nSize;
    AX_U32 nCnt;
} SAMPLE_BLK_T;

typedef enum
{
    SAMPLE_CASE_NONE = -1,
    SAMPLE_CASE_OS08A20_VIN_IVPS_ENGINE_VENC_RTSP_E = 0,
    SAMPLE_CASE_BUTT
} SAMPLE_CASE_E;

typedef struct
{
    SAMPLE_CASE_E eSampleCase;
    COMMON_VIN_MODE_E eSysMode;
    AX_SNS_HDR_MODE_E eHdrMode;
    AX_BOOL bAiispEnable;
    AX_S32 nVencDump;
    AX_CHAR *pFrameInfo;
    AX_U32 statDeltaPtsFrmNum;
    AX_BOOL bVencSelect;
} SAMPLE_PARAM_T;

AX_U64 GetTickCount(AX_VOID);
AX_S32 SAMPLE_PoolFloorInit(SAMPLE_BLK_T *pBlkInfo, AX_U32 nNum);
AX_VOID ThreadLoopStateSet(AX_BOOL bValue);
AX_BOOL ThreadLoopStateGet(AX_VOID);
AX_U32 CalcImgSize(AX_U32 nStride, AX_U32 nW, AX_U32 nH, AX_IMG_FORMAT_E eType, AX_U32 nAlign);
char *FrameInfoGet(char *optArg, AX_VIDEO_FRAME_T *ptFrame);
AX_S32 FrameBufGet(AX_S32 nFrameIdx, AX_VIDEO_FRAME_T *ptImage, char *pImgFile);
#endif
