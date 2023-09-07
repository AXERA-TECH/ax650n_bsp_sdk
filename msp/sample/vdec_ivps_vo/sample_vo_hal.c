/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "ax_sys_api.h"
#include "ax_base_type.h"
#include "ax_vo_api.h"

#include "common_vo.h"
#include "common_vo_pattern.h"
#include "sample_vo_hal.h"

#ifndef SAMPLE_PRT
#define SAMPLE_PRT(fmt,...)   \
{ \
    printf("[SAMPLE-VO][%s-%d] "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
}
#endif

AX_S32 g_vo_debug;

void *VoGetFrameThread(void *arg)
{
    return NULL;
}

static int SAMPLE_VO_POOL_DESTROY(AX_U32 u32PoolID)
{
    return AX_POOL_DestroyPool(u32PoolID);
}

static AX_S32 SAMPLE_VO_CREATE_POOL(AX_U32 u32BlkCnt, AX_U64 u64BlkSize, AX_U64 u64MetaSize, AX_U32 *pPoolID)
{
    AX_POOL_CONFIG_T stPoolCfg = {0};

    stPoolCfg.MetaSize = u64MetaSize;
    stPoolCfg.BlkCnt = u32BlkCnt;
    stPoolCfg.BlkSize = u64BlkSize;
    stPoolCfg.CacheMode = POOL_CACHE_MODE_NONCACHE;
    strcpy((char *)stPoolCfg.PartitionName, "anonymous");

    *pPoolID = AX_POOL_CreatePool(&stPoolCfg);
    if (*pPoolID == AX_INVALID_POOLID) {
        SAMPLE_PRT("AX_POOL_CreatePool failed, u32BlkCnt = %d, u64BlkSize = 0x%llx, u64MetaSize = 0x%llx\n", u32BlkCnt,
                   u64BlkSize, u64MetaSize);
        return -1;
    }

    SAMPLE_PRT("u32BlkCnt = %d, u64BlkSize = 0x%llx, pPoolID = %d\n", u32BlkCnt, u64BlkSize, *pPoolID);

    return 0;
}

typedef struct axSAMPLE_VO_THREAD_PARAM {
    AX_U32 u32ThreadForceStop;
    pthread_t ThreadID;

    SAMPLE_VO_CONFIG_S *pstVoConf;
} SAMPLE_VO_THREAD_PARAM_S;

static SAMPLE_VO_THREAD_PARAM_S gSampleVoCtrlParam;

static AX_VOID *SAMPLE_VO_CTRL_THREAD(AX_VOID *pData)
{
    AX_CHAR key, key0, key1;
    AX_BOOL bQuit = AX_FALSE;
    AX_S32 s32Ret;
    AX_U32 u32LayerID, u32ChnID;
    SAMPLE_VO_THREAD_PARAM_S *pstSampleVoCtrlParam = (SAMPLE_VO_THREAD_PARAM_S *)pData;
    SAMPLE_VO_CONFIG_S *pstVoConf = pstSampleVoCtrlParam->pstVoConf;
    AX_IVPS_CROP_INFO_T CropInfo = {0};
    CropInfo.bEnable = AX_TRUE;
    CropInfo.eCoordMode = AX_COORD_ABS;
    CropInfo.tCropRect.nX = 100;
    CropInfo.tCropRect.nY = 100;
    CropInfo.tCropRect.nW = 600;
    CropInfo.tCropRect.nH = 800;

    u32LayerID = pstVoConf->stVoLayer[0].u32VoLayer;
    u32ChnID = 0;

    SAMPLE_PRT("layer%d-chn%d ctrl thread enter\n", u32LayerID, u32ChnID);

    while (pstSampleVoCtrlParam->u32ThreadForceStop && !bQuit) {
        SAMPLE_PRT("please enter[pause(p), step(s), refresh(f), resume(r), exit(q)]:\n");
        key0 = getchar();
        key1 = getchar();
        key = (key0 == '\n') ? key1 : key0;

        switch (key) {
        case 'd':
        case 'D':
            s32Ret = AX_VO_DpmsOff(0);
            SAMPLE_PRT("disable vo0 %s\n", s32Ret ? "failed" : "success");
            break;
        case 'e':
        case 'E':
            s32Ret = AX_VO_DpmsOn(0);
            SAMPLE_PRT("enable vo0 %s\n", s32Ret ? "failed" : "success");
            break;
        case 'p':
        case 'P':
            s32Ret = AX_VO_PauseChn(u32LayerID, u32ChnID);
            SAMPLE_PRT("pause layer%d-chn%d %s\n", u32LayerID, u32ChnID, s32Ret ? "failed" : "success");
            break;
        case 's':
        case 'S':
            s32Ret = AX_VO_StepChn(u32LayerID, u32ChnID);
            SAMPLE_PRT("step layer%d-chn%d %s\n", u32LayerID, u32ChnID, s32Ret ? "failed" : "success");
            break;
        case 'r':
        case 'R':
            s32Ret = AX_VO_ResumeChn(u32LayerID, u32ChnID);
            SAMPLE_PRT("resume layer%d-chn%d %s\n", u32LayerID, u32ChnID, s32Ret ? "failed" : "success");
            break;
        case 'f':
        case 'F':
            s32Ret = AX_IVPS_SetGrpCrop(u32LayerID, &CropInfo);
            s32Ret = AX_VO_RefreshChn(u32LayerID, u32ChnID);
            SAMPLE_PRT("refresh layer%d-chn%d %s\n", u32LayerID, u32ChnID, s32Ret ? "failed" : "success");
            break;
        case 'q':
        case 'Q':
            bQuit = AX_TRUE;
            break;
        }
    }

    SAMPLE_PRT("exit\n");

    return NULL;
}

AX_S32 VoInit(SAMPLE_VO_CONFIG_S *pstVoConf)
{
    AX_S32 i, j, s32Ret = 0;
    AX_U32 u32LayerWidth = 0;
    AX_U32 u32LayerHeight = 0;
    AX_U64 u64BlkSize = 0, u64BlkNr;
    SAMPLE_VO_DEV_CONFIG_S *pstVoDevConf;
    SAMPLE_VO_LAYER_CONFIG_S *pstVoLayerConf;
    AX_VO_VIDEO_LAYER_ATTR_T *pstVoLayerAttr;

    s32Ret = AX_VO_Init();
    if (s32Ret) {
        SAMPLE_PRT("AX_VO_Init failed, s32Ret = 0x%x\n", s32Ret);
        return s32Ret;
    }

    pstVoConf->u32LayerNr = pstVoConf->u32VDevNr;
    if (pstVoConf->u32BindMode) {
        pstVoConf->u32LayerNr = 1;
    }

    for (i = 0; i < pstVoConf->u32LayerNr; i++) {
        pstVoDevConf = &pstVoConf->stVoDev[i];
        pstVoLayerConf = &pstVoConf->stVoLayer[i];
        pstVoLayerAttr = &pstVoLayerConf->stVoLayerAttr;

        if (pstVoConf->u32BindMode) {
            for (j = 0; j < pstVoConf->u32VDevNr; j++) {
                pstVoLayerConf->bindVoDev[j] = pstVoConf->stVoDev[j].u32VoDev;
            }
        } else {
            pstVoLayerConf->bindVoDev[0] = pstVoDevConf->u32VoDev;
        }

        u32LayerWidth = pstVoLayerAttr->stImageSize.u32Width;
        u32LayerHeight = pstVoLayerAttr->stImageSize.u32Height;

        u64BlkSize = (AX_U64)u32LayerWidth * u32LayerHeight * 3 / 2;
        u64BlkNr = 5;
        s32Ret = SAMPLE_VO_CREATE_POOL(u64BlkNr, u64BlkSize, 512, &pstVoLayerConf->u32LayerPoolId);
        if (s32Ret) {
            SAMPLE_PRT("SAMPLE_VO_CREATE_POOL failed, i:%d, s32Ret:0x%x\n", i, s32Ret);
            goto exit;
        }

        pstVoLayerAttr->u32PoolId = pstVoLayerConf->u32LayerPoolId;

        SAMPLE_PRT("u32LayerPoolId = %d\n", pstVoLayerConf->u32LayerPoolId);
    }

    s32Ret = SAMPLE_COMM_VO_StartVO(pstVoConf);
    if (s32Ret) {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartVO failed, s32Ret:0x%x\n", s32Ret);
    }

    if (!s32Ret && g_vo_debug) {
        gSampleVoCtrlParam.u32ThreadForceStop = 1;
        gSampleVoCtrlParam.pstVoConf = pstVoConf;
        pthread_create(&gSampleVoCtrlParam.ThreadID, NULL, SAMPLE_VO_CTRL_THREAD, &gSampleVoCtrlParam);
    }

exit:
    if (s32Ret) {
        for (i = 0; i < pstVoConf->u32LayerNr; i++) {
            pstVoLayerConf = &pstVoConf->stVoLayer[i];
            SAMPLE_VO_POOL_DESTROY(pstVoLayerConf->u32LayerPoolId);
        }

        AX_VO_Deinit();
    }

    SAMPLE_PRT("done, s32Ret = 0x%x\n", s32Ret);

    return s32Ret;
}

AX_VOID VoDeInit(SAMPLE_VO_CONFIG_S *pstVoConf)
{
    AX_S32 i;
    SAMPLE_VO_LAYER_CONFIG_S *pstVoLayerConf;

    if (gSampleVoCtrlParam.ThreadID) {
        gSampleVoCtrlParam.u32ThreadForceStop = 0;
        pthread_join(gSampleVoCtrlParam.ThreadID, NULL);
    }

    SAMPLE_COMM_VO_StopVO(pstVoConf);

    for (i = 0; i < pstVoConf->u32LayerNr; i++) {
        pstVoLayerConf = &pstVoConf->stVoLayer[i];
        SAMPLE_VO_POOL_DESTROY(pstVoLayerConf->u32LayerPoolId);
    }

    AX_VO_Deinit();

    SAMPLE_PRT("done\n");
}
