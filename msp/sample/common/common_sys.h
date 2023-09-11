/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __COMMON_SYS_H__
#define __COMMON_SYS_H__

#include "ax_base_type.h"
#include "ax_global_type.h"
#include "ax_pool_type.h"

#define MAX_POOLS 5

#ifndef COMM_SYS_PRT
#define COMM_SYS_PRT(fmt...)   \
do {\
    printf("[COMM_SYS][%30s][%5d] ", __FUNCTION__, __LINE__);\
    printf(fmt);\
}while(0)
#endif

typedef struct {
    AX_U32          nWidth;
    AX_U32          nHeight;
    AX_U32          nWidthStride;
    AX_IMG_FORMAT_E nFmt;
    AX_U32          nBlkCnt;
} COMMON_SYS_POOL_CFG_T;

typedef struct {
    AX_U8 nCamCnt;
    AX_U32 nPoolCfgCnt;
    COMMON_SYS_POOL_CFG_T* pPoolCfg;
} COMMON_SYS_ARGS_T;

AX_S32 COMMON_SYS_CalcPool(COMMON_SYS_POOL_CFG_T *pPoolCfg, AX_U32 nCommPoolCnt, AX_POOL_FLOORPLAN_T *pPoolFloorPlan);
AX_S32 COMMON_SYS_Init(COMMON_SYS_ARGS_T *pCommonArgs);
AX_S32 COMMON_SYS_DeInit();
AX_S32 COMMON_SYS_GetApdPlateId(AX_CHAR *apd_plate_id);

#endif //__COMMON_SYS_H__

