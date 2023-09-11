/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include "sample_pool.h"

#include <string.h>

#include "ax_sys_api.h"
#include "sample_venc_log.h"

AX_S32 SampleMemInit(SAMPLE_VENC_CMD_PARA_T *pCml)
{
    AX_S32 s32Ret = -1;
    AX_POOL_CONFIG_T stPoolConfig;
    pCml->BlkSize = pCml->frameSize;

    s32Ret = AX_SYS_Init();
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("AX_SYS_Init failed! Error Code:0x%X\n", s32Ret);
        return -1;
    }

    if (pCml->qpMapQpType || pCml->qpMapBlkType)
        pCml->BlkSize += pCml->qpMapSize;

    /* use user pool */
    memset(&stPoolConfig, 0, sizeof(AX_POOL_CONFIG_T));
    stPoolConfig.MetaSize = 512;
    stPoolConfig.BlkCnt = pCml->vbCnt;
    stPoolConfig.BlkSize = pCml->BlkSize;
    stPoolConfig.CacheMode = POOL_CACHE_MODE_NONCACHE;
    memset(stPoolConfig.PartitionName, 0, sizeof(stPoolConfig.PartitionName));
    strcpy((AX_CHAR *)stPoolConfig.PartitionName, "anonymous");

    pCml->poolId = AX_POOL_CreatePool(&stPoolConfig);
    if (AX_INVALID_POOLID == pCml->poolId) {
        SAMPLE_LOG_ERR("Create pool err.\n");
        goto FREE_SYS;
    }

    return 0;

FREE_SYS:
    s32Ret = AX_SYS_Deinit();
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("AX_SYS_Deinit failed! Error Code:0x%X\n", s32Ret);
        s32Ret = -1;
    }

    return s32Ret;
}

AX_S32 SampleMemDeinit(SAMPLE_VENC_CMD_PARA_T *pCml)
{
    AX_S32 s32Ret = -1;

    if (AX_INVALID_POOLID != pCml->poolId) {
        s32Ret = AX_POOL_DestroyPool(pCml->poolId);
        if (s32Ret) {
            SAMPLE_LOG("fail to destroy pool(%d), ret=0x%lx.\n", pCml->poolId, s32Ret);
            return -1;
        }
    }

    if (pCml->bDynRes && (AX_INVALID_POOLID != pCml->poolIdDynRes)) {
        s32Ret = AX_POOL_DestroyPool(pCml->poolIdDynRes);
        if (s32Ret) {
            SAMPLE_LOG_ERR("Pool destroy error, ret=0x%lx.\n", s32Ret);
            return -1;
        }
    }

    s32Ret = AX_SYS_Deinit();
    if (s32Ret) {
        SAMPLE_LOG_ERR("AX_SYS_Deinit failed! Error Code:0x%X\n", s32Ret);
        return -1;
    }

    return 0;
}