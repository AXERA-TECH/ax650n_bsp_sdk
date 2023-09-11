/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "ax_vo_api.h"
#include "common_vo.h"
#include "common_vo_pattern.h"

typedef struct axSAMPLE_VO_CHN_THREAD_PARAM {
    pthread_t ThreadID;

    AX_U32 u32ThreadForceStop;

    AX_U32 u32LayerID;
    AX_U32 u32ChnID;
    AX_POOL u32UserPoolId;
} SAMPLE_VO_CHN_THREAD_PARAM_S;

static SAMPLE_VO_CONFIG_S stVoConf = {
    /* for device */
    .u32VDevNr = 1,
    .stVoDev = {
        {
            .u32VoDev = SAMPLE_VO_DEV0,
            .enVoIntfType = AX_VO_INTF_HDMI,
            .enIntfSync = AX_VO_OUTPUT_1080P60,
        },
    },

    .u32LayerNr = 1,
    .stVoLayer = {
        {
            // layer0
            .bindVoDev = {SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX},
            .stVoLayerAttr = {
                .stDispRect = {0, 0, 1920, 1080},
                .stImageSize = {1920, 1080},
                .enPixFmt = AX_FORMAT_YUV420_SEMIPLANAR,
                .enSyncMode = AX_VO_LAYER_SYNC_NORMAL,
                .u32PrimaryChnId = 0,
                .u32FifoDepth = 0,
                .u32ChnNr = 2,
                .u32BkClr = 0,
                .enWBMode = AX_VO_LAYER_WB_POOL,
                .u32InplaceChnId = 0,
                .u32PoolId = ~0,
                .u32DispatchMode = AX_VO_LAYER_OUT_TO_LINK,
            },
            .enVoMode = VO_MODE_1MUX,
            .u64KeepChnPrevFrameBitmap0 = 0x1UL,
            .u64KeepChnPrevFrameBitmap1 = 0x0UL,
        },
        {
            // layer1
            .bindVoDev = {SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX},
            .stVoLayerAttr = {
                .stDispRect = {0, 0, 1920, 1080},
                .stImageSize = {1920, 1080},
                .enPixFmt = AX_FORMAT_YUV420_SEMIPLANAR,
                .enSyncMode = AX_VO_LAYER_SYNC_NORMAL,
                .u32PrimaryChnId = 0,
                .u32FifoDepth = 0,
                .u32ChnNr = 16,
                .u32BkClr = 0,
                .enWBMode = AX_VO_LAYER_WB_POOL,
                .u32InplaceChnId = 0,
                .u32PoolId = ~0,
                .u32DispatchMode = AX_VO_LAYER_OUT_TO_FIFO,
            },
            .enVoMode = VO_MODE_1MUX,
            .u64KeepChnPrevFrameBitmap0 = ~0x0UL,
            .u64KeepChnPrevFrameBitmap1 = ~0x0UL,
        },
        {
            // layer2
            .bindVoDev = {SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX, SAMPLE_VO_DEV_MAX},
        }
    },
};

AX_CHAR sync_name[AX_VO_OUTPUT_BUTT][32] = {
    [AX_VO_OUTPUT_576P50] = {"576P50"},
    [AX_VO_OUTPUT_480P60] = {"480P60"},
    [AX_VO_OUTPUT_720P50] = {"720P50"},
    [AX_VO_OUTPUT_720P60] = {"720P60"},
    [AX_VO_OUTPUT_1080P24] = {"1080P24"},
    [AX_VO_OUTPUT_1080P25] = {"1080P25"},
    [AX_VO_OUTPUT_1080P30] = {"1080P30"},
    [AX_VO_OUTPUT_1080P50] = {"1080P50"},
    [AX_VO_OUTPUT_1080P60] = {"1080P60"},
    [AX_VO_OUTPUT_640x480_60] = {"640x480_60"},
    [AX_VO_OUTPUT_800x600_60] = {"800x600_60"},
    [AX_VO_OUTPUT_1024x768_60] = {"1024x768_60"},
    [AX_VO_OUTPUT_1280x1024_60] = {"1280x1024_60"},
    [AX_VO_OUTPUT_1366x768_60] = {"1366x768_60"},
    [AX_VO_OUTPUT_1280x800_60] = {"1280x800_60"},
    [AX_VO_OUTPUT_1440x900_60] = {"1440x900_60"},
    [AX_VO_OUTPUT_1600x1200_60] = {"1600x1200_60"},
    [AX_VO_OUTPUT_1680x1050_60] = {"1680x1050_60"},
    [AX_VO_OUTPUT_1920x1200_60] = {"1920x1200_60"},
    [AX_VO_OUTPUT_2560x1600_60] = {"2560x1600_60"},
    [AX_VO_OUTPUT_3840x2160_24] = {"3840x2160_24"},
    [AX_VO_OUTPUT_3840x2160_25] = {"3840x2160_25"},
    [AX_VO_OUTPUT_3840x2160_30] = {"3840x2160_30"},
    [AX_VO_OUTPUT_3840x2160_50] = {"3840x2160_50"},
    [AX_VO_OUTPUT_3840x2160_60] = {"3840x2160_60"},
    [AX_VO_OUTPUT_4096x2160_24] = {"4096x2160_24"},
    [AX_VO_OUTPUT_4096x2160_25] = {"4096x2160_25"},
    [AX_VO_OUTPUT_4096x2160_30] = {"4096x2160_30"},
    [AX_VO_OUTPUT_4096x2160_50] = {"4096x2160_50"},
    [AX_VO_OUTPUT_4096x2160_60] = {"4096x2160_60"},
    [AX_VO_OUTPUT_720x1280_60] = {"720x1280_60"},
    [AX_VO_OUTPUT_1080x1920_60] = {"1080x1920_60"},
};

AX_VOID PrintVoReso(AX_VOID)
{
    AX_S32 i, printItems = 0;

    for (i = 0; i < AX_VO_OUTPUT_BUTT; i++) {
        if (!strlen(sync_name[i]))
            continue;

        if (!(printItems%4))
            printf("\n\t\t\t");

        printf("[%s]\t", sync_name[i]);
        printItems++;
    }
    printf("\n");
}

AX_S32 ParseVoPubAttr(AX_CHAR *pStr)
{
    SAMPLE_VO_DEV_CONFIG_S *pstVoDev = &stVoConf.stVoDev[0];
    AX_CHAR *p, *end;
    AX_S32 i;

    if (!pStr)
        return -EINVAL;

    p = pStr;

    if (strstr(p, "dpi")) {
        SAMPLE_PRT("dpi output\n");
        pstVoDev->enVoIntfType = AX_VO_INTF_DPI;
    } else if (strstr(p, "dsi")) {
        SAMPLE_PRT("dsi output\n");
        pstVoDev->enVoIntfType = AX_VO_INTF_DSI;
    } else if (strstr(p, "hdmi")) {
        SAMPLE_PRT("hdmi output\n");
        pstVoDev->enVoIntfType = AX_VO_INTF_HDMI;
    } else if (strstr(p, "bt601")) {
        SAMPLE_PRT("bt601 output\n");
        pstVoDev->enVoIntfType = AX_VO_INTF_BT601;
    } else if (strstr(p, "bt656")) {
        SAMPLE_PRT("bt656 output\n");
        pstVoDev->enVoIntfType = AX_VO_INTF_BT656;
    } else if (strstr(p, "bt1120")) {
        SAMPLE_PRT("bt1120 output\n");
        pstVoDev->enVoIntfType = AX_VO_INTF_BT1120;
    } else {
        SAMPLE_PRT("unsupported interface type, %s\n", p);
        return -EINVAL;
    }

    end = strstr(p, "@");
    if (end == NULL) {
        SAMPLE_PRT("No display resolution.\n");
        return -EINVAL;
    }
    p = end + 1;
    end = strstr(p, "@dev");
    if (end == NULL) {
        SAMPLE_PRT("No vo device.\n");
        return -EINVAL;
    }
    *end = 0;
    if (!strlen(p)) {
        SAMPLE_PRT("No display resolution.\n");
        return -EINVAL;
    }
    /* find display resolution. */
    for (i = 0; i < AX_VO_OUTPUT_BUTT; i++) {
        if (!strcmp(p, sync_name[i])) {
            pstVoDev->enIntfSync = i;
            break;
        }
    }
    if (i >= AX_VO_OUTPUT_BUTT) {
        SAMPLE_PRT("unknow display resolution.\n");
        return -EINVAL;
    }
    /* find dpu device. */
    p = end + 4;
    pstVoDev->u32VoDev = strtoul(p, &end, 10);
    if (pstVoDev->u32VoDev >= SAMPLE_VO_DEV_MAX) {
        SAMPLE_PRT("unsupported vo dev%u\n", pstVoDev->u32VoDev);
        return -EINVAL;
    }

    return 0;
}

AX_S32 SampleGetVoModeInfo(AX_MOD_INFO_T *pstModeInfo)
{
    SAMPLE_VO_LAYER_CONFIG_S *pstVoLayer = &stVoConf.stVoLayer[0];

    if (!pstVoLayer->s32InitFlag) {
        SAMPLE_PRT("Error! VO layer isn't created.\n")
        return -EINVAL;
    }

    pstModeInfo->enModId = AX_ID_VO;
    pstModeInfo->s32GrpId = pstVoLayer->u32VoLayer;
    pstModeInfo->s32ChnId = 0;

    return 0;
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

static AX_VOID *SAMPLE_VO_CHN_THREAD(AX_VOID *pData)
{
    AX_S32 s32Ret = 0;
    AX_VIDEO_FRAME_T stFrame = {0};
    AX_U32 u32FrameSize;
    AX_U32 u32LayerID, u32ChnID;
    AX_BLK BlkId;
    AX_VO_CHN_ATTR_T stChnAttr;
    AX_VOID * u64VirAddr;
    AX_U64 u64PhysAddr;
    SAMPLE_VO_CHN_THREAD_PARAM_S *pstChnThreadParam = (SAMPLE_VO_CHN_THREAD_PARAM_S *)pData;

    u32LayerID = pstChnThreadParam->u32LayerID;
    u32ChnID = pstChnThreadParam->u32ChnID;

    s32Ret = AX_VO_GetChnAttr(u32LayerID, u32ChnID, &stChnAttr);
    if (s32Ret) {
        SAMPLE_PRT("layer%d-chn%d AX_VO_GetChnAttr failed, s32Ret = 0x%x\n", u32LayerID, u32ChnID, s32Ret);
        goto exit;
    }
    SAMPLE_PRT("layer%d-chn%d u32Width = %d, u32Height = %d\n",
               u32LayerID, u32ChnID,
               stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height);

    stFrame.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
    stFrame.u32Width = stChnAttr.stRect.u32Width;
    stFrame.u32Height = stChnAttr.stRect.u32Height;
    stFrame.u32PicStride[0] = stChnAttr.stRect.u32Width;
    u32FrameSize = stFrame.u32PicStride[0] * stFrame.u32Height * 3 / 2;

    s32Ret = SAMPLE_VO_CREATE_POOL(3, u32FrameSize, 512, &pstChnThreadParam->u32UserPoolId);
    if (s32Ret) {
        SAMPLE_PRT("layer%d-chn%d SAMPLE_VO_CREATE_POOL failed, s32Ret = 0x%x\n", u32LayerID, u32ChnID, s32Ret);
        goto exit;
    }

    while (!pstChnThreadParam->u32ThreadForceStop) {
        BlkId = AX_POOL_GetBlock(pstChnThreadParam->u32UserPoolId, u32FrameSize, NULL);
        if (AX_INVALID_BLOCKID == BlkId) {
            SAMPLE_PRT("layer%d-chn%d AX_POOL_GetBlock failed\n", u32LayerID, u32ChnID);
            usleep(10000);
            continue;
        }
        stFrame.u64PhyAddr[0] = AX_POOL_Handle2PhysAddr(BlkId);
        stFrame.u64VirAddr[0] = 0;

        u64PhysAddr = AX_POOL_Handle2PhysAddr(BlkId);

        if (!u64PhysAddr) {
            SAMPLE_PRT("AX_POOL_Handle2PhysAddr failed,BlkId=0x%x\n", BlkId);
            break;
        }

        u64VirAddr = AX_SYS_Mmap(u64PhysAddr, u32FrameSize);

        if (!u64VirAddr) {
            SAMPLE_PRT("AX_SYS_Mmap failed\n");
            break;
        }

        SAMPLE_Fill_Color(stFrame.enImgFormat,
                          stFrame.u32Width,
                          stFrame.u32Height,
                          stFrame.u32PicStride[0],
                          u64VirAddr);

        s32Ret = AX_SYS_Munmap(u64VirAddr, u32FrameSize);

        if (s32Ret) {
            SAMPLE_PRT("AX_SYS_Munmap failed,s32Ret=0x%x\n", s32Ret);
            break;
        }

        stFrame.u32BlkId[0] = BlkId;
        stFrame.u32BlkId[1] = AX_INVALID_BLOCKID;
        //SAMPLE_PRT("layer%d-chn%d start send frame, BlkId = 0x%x\n", u32LayerID, u32ChnID, BlkId);
        s32Ret = AX_VO_SendFrame(u32LayerID, u32ChnID, &stFrame, 0);
        if (s32Ret) {
            SAMPLE_PRT("layer%d-chn%d AX_VO_SendFrame failed, s32Ret = 0x%x\n", u32LayerID, u32ChnID, s32Ret);
            AX_POOL_ReleaseBlock(BlkId);
            usleep(16666);
            continue;
        }

        AX_POOL_ReleaseBlock(BlkId);

        usleep(16666);

    }

exit:
    SAMPLE_PRT("layer%d-chn%d exit, s32Ret = 0x%x\n", u32LayerID, u32ChnID, s32Ret);

    return NULL;
}

SAMPLE_VO_CHN_THREAD_PARAM_S g_stChnThreadParam = {0};

AX_S32 VoInit(AX_MOD_INFO_T *pstModeInfo, AX_VO_SIZE_T *pstImgSize)
{
    AX_S32 i, s32Ret;
    AX_U64 u64BlkSize = 0;
    SAMPLE_VO_CONFIG_S *pstVoConf = &stVoConf;
    SAMPLE_VO_DEV_CONFIG_S *pstVoDevConf;
    SAMPLE_VO_LAYER_CONFIG_S *pstVoLayer;
    AX_VO_VIDEO_LAYER_ATTR_T *pstVoLayerAttr;
    SAMPLE_VO_CHN_THREAD_PARAM_S *pstChnThreadParam = &g_stChnThreadParam;

    if (pstModeInfo == NULL || pstImgSize == NULL) {
        SAMPLE_PRT("Invalid argument! pstModeInfo:0x%p, pstImgSize:0x%p\n",
                   pstModeInfo, pstImgSize);
        return -EINVAL;
    }
    s32Ret = AX_SYS_Init();
    if (s32Ret) {
        SAMPLE_PRT("AX_SYS_Init failed, s32Ret = 0x%x\n", s32Ret);
        return -1;
    }

    s32Ret = AX_VO_Init();
    if (s32Ret) {
        SAMPLE_PRT("AX_VO_Init failed, s32Ret = 0x%x\n", s32Ret);
        goto exit0;
    }

    SAMPLE_PRT("u32LayerNr = %d\n", pstVoConf->u32LayerNr);

    for (i = 0; i < pstVoConf->u32LayerNr; i++) {
        pstVoDevConf = &pstVoConf->stVoDev[i];
        pstVoLayer = &pstVoConf->stVoLayer[i];

        pstVoLayer->bindVoDev[0] = pstVoDevConf->u32VoDev;

        pstVoLayerAttr = &pstVoLayer->stVoLayerAttr;
        if (i == 0) {
            pstVoLayerAttr->stImageSize.u32Width = pstImgSize->u32Width;
            pstVoLayerAttr->stImageSize.u32Height = pstImgSize->u32Height;
        } else {
            pstVoLayer->enVoMode = VO_MODE_1MUX;
            pstVoLayerAttr->u32ChnNr = 1;
        }

        pstVoLayerAttr->stDispRect.u32Width = pstVoLayerAttr->stImageSize.u32Width;
        pstVoLayerAttr->stDispRect.u32Height = pstVoLayerAttr->stImageSize.u32Height;

        SAMPLE_PRT("layer%d u32Width = %d, u32Height = %d\n", i, pstVoLayerAttr->stImageSize.u32Width,
                   pstVoLayerAttr->stImageSize.u32Height);

        pstVoLayerAttr->u32PoolId = ~0;
        u64BlkSize = (AX_U64)ALIGN_UP(pstVoLayerAttr->stImageSize.u32Width, 16) * ALIGN_UP(pstVoLayerAttr->stImageSize.u32Height, 2) * 3 / 2;
        s32Ret = SAMPLE_VO_CREATE_POOL(3, u64BlkSize, 512, &pstVoLayerAttr->u32PoolId);
        if (s32Ret) {
            SAMPLE_PRT("SAMPLE_VO_CREATE_POOL failed, s32Ret = 0x%x\n", s32Ret);
            goto exit1;
        }

        SAMPLE_PRT("layer pool id = 0x%x\n", pstVoLayerAttr->u32PoolId);
    }

    s32Ret = SAMPLE_COMM_VO_StartVO(pstVoConf);
    if (s32Ret) {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartVO failed, s32Ret = 0x%x\n", s32Ret);
        goto exit1;
    }

    for (i = 0; i < pstVoConf->u32LayerNr; i++) {
        pstVoLayer = &pstVoConf->stVoLayer[i];
        if (i == 0) {
            pstVoLayer->stSrcMod = *pstModeInfo;

            pstVoLayer->stDstMod.enModId = AX_ID_VO;
            pstVoLayer->stDstMod.s32GrpId = 0;
            pstVoLayer->stDstMod.s32ChnId = 0;
            AX_SYS_Link(&pstVoLayer->stSrcMod, &pstVoLayer->stDstMod);
        } else {
            pstChnThreadParam->u32LayerID = pstVoLayer->u32VoLayer;
            pstChnThreadParam->u32ChnID = 0;
            pstChnThreadParam->u32ThreadForceStop = 0;
            pstChnThreadParam->u32UserPoolId = ~0;
            pthread_create(&pstChnThreadParam->ThreadID, NULL, SAMPLE_VO_CHN_THREAD, pstChnThreadParam);
        }
    }

    SAMPLE_PRT("done\n");

    return 0;

exit1:
    for (i = 0; i < pstVoConf->u32LayerNr; i++) {
        pstVoLayer = &pstVoConf->stVoLayer[i];
        pstVoLayerAttr = &pstVoLayer->stVoLayerAttr;
        if (pstVoLayerAttr->u32PoolId != ~0)
            SAMPLE_VO_POOL_DESTROY(pstVoLayerAttr->u32PoolId);
    }

    AX_VO_Deinit();

exit0:
    AX_SYS_Deinit();

    SAMPLE_PRT("done, s32Ret = 0x%x\n", s32Ret);

    return s32Ret;
}

AX_VOID VoDeInit(AX_VOID)
{
    AX_S32 i;
    SAMPLE_VO_CONFIG_S *pstVoConf = &stVoConf;
    SAMPLE_VO_LAYER_CONFIG_S *pstVoLayer;
    AX_VO_VIDEO_LAYER_ATTR_T *pstVoLayerAttr;
    SAMPLE_VO_CHN_THREAD_PARAM_S *pstChnThreadParam = &g_stChnThreadParam;

    for (i = 0; i < pstVoConf->u32LayerNr; i++) {
        pstVoLayer = &pstVoConf->stVoLayer[i];
        if (i == 0) {
            AX_SYS_UnLink(&pstVoLayer->stSrcMod, &pstVoLayer->stDstMod);
        } else {
            if (pstChnThreadParam->ThreadID) {
                pstChnThreadParam->u32ThreadForceStop = 1;
                pthread_join(pstChnThreadParam->ThreadID, NULL);
            }
        }
    }

    SAMPLE_COMM_VO_StopVO(pstVoConf);

    AX_VO_Deinit();

    for (i = 0; i < pstVoConf->u32LayerNr; i++) {
        pstVoLayer = &pstVoConf->stVoLayer[i];
        pstVoLayerAttr = &pstVoLayer->stVoLayerAttr;
        if (i != 0) {
            if (pstChnThreadParam->u32UserPoolId != ~0)
                SAMPLE_VO_POOL_DESTROY(pstChnThreadParam->u32UserPoolId);
        }

        if (pstVoLayerAttr->u32PoolId != ~0)
            SAMPLE_VO_POOL_DESTROY(pstVoLayerAttr->u32PoolId);
    }

    AX_SYS_Deinit();
}
