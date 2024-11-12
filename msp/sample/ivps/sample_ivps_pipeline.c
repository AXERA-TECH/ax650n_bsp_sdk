/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "sample_ivps_main.h"

#define DEFAULT_DELIM "@"
#define EPOLL_MAXUSERS 128

AX_IVPS_TDP_CFG_T gSampleTdpCfg0 = {
    .bMirror = AX_FALSE,
    .bFlip = AX_FALSE,
    .eRotation = AX_IVPS_ROTATION_0,
};

AX_IVPS_TDP_CFG_T gSampleTdpCfg1 = {
    .bMirror = AX_FALSE,
    .bFlip = AX_FALSE,
    .eRotation = AX_IVPS_ROTATION_0,
};

AX_IVPS_TDP_CFG_T gSampleTdpCfg2 = {
    .bMirror = AX_FALSE,
    .bFlip = AX_TRUE,
    .eRotation = AX_IVPS_ROTATION_0,
};

AX_IVPS_GDC_CFG_T gSampleGdcCfg = {
    .eRotation = AX_IVPS_ROTATION_0,
    /*
    .eDewarpType = AX_IVPS_DEWARP_LDC,
    .tLdcAttr = {
          .nDistortionRatio = 5000,
          .nXRatio = 50,
          .nYRatio = 50,
    },
    */
    /*
    .eDewarpType = AX_IVPS_DEWARP_LDC_V2,
    .tLdcV2Attr = {
        .nXCenter = 192000/2,
        .nYCenter = 108000/2,
        .nXFocus = 191931,
        .nYFocus = 192664,
        .nDistortionCoeff = {-462135, 320390, -2206, -170, -147752},
    },
    */
   /*
    .eDewarpType = AX_IVPS_DEWARP_LDC_V2,
    .tLdcV2Attr = {
        .nXCenter = 96718,
        .nYCenter = 53018,
        .nXFocus = 116042,
        .nYFocus = 115973,
        .nDistortionCoeff = {-169404, 108643, -236, -139, -26403, 196578},
    },
    */
};

SAMPLE_IVPS_GRP_T gSampleIvpsGrp = {
    .nIvpsGrp = 1,

    .tGrpAttr = {
        .ePipeline = AX_IVPS_PIPELINE_DEFAULT,
    },
    .tPipelineAttr = {

        .nOutChnNum = 4,
        .nOutFifoDepth = {2, 2, 2, 2},
        .tFilter = {
            {
                /* group filter0 */
                {
                    .bEngage = AX_TRUE,
                    .eEngine = AX_IVPS_ENGINE_TDP,
                    .bCrop = AX_FALSE,
                    .tCropRect = {
                        .nX = 0,
                        .nY = 0,
                        .nW = 0,
                        .nH = 0,
                    },
                    .nDstPicWidth = 3840,
                    .nDstPicHeight = 2160,
                    .nDstPicStride = 3840,
                    .eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR,
                },
                /* group filter0 */
                {
                    .bEngage = AX_TRUE,
                    .eEngine = AX_IVPS_ENGINE_TDP,
                    .bCrop = AX_FALSE,
                    .tCropRect = {
                        .nX = 0,
                        .nY = 0,
                        .nW = 0,
                        .nH = 0,
                    },
                    .nDstPicWidth = 1920,
                    .nDstPicHeight = 1080,
                    .nDstPicStride = 1920,
                    .eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR,
                },
            },

            {
                /* channel0 filter0 */
                {
                    .bEngage = AX_TRUE,
                    .eEngine = AX_IVPS_ENGINE_VPP,
                    .bCrop = AX_FALSE,
                    .tCompressInfo = {
                        .enCompressMode = AX_COMPRESS_MODE_NONE,
                        .u32CompressLevel = 4,
                    },
                    .tCropRect = {
                        .nX = 0,
                        .nY = 0,
                        .nW = 0,
                        .nH = 0,
                    },
                    .nDstPicWidth = 1920,
                    .nDstPicHeight = 1080,
                    .nDstPicStride = 1920,
                    .eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR,
                    /*.tAspectRatio = {.eMode = IVPS_ASPECT_RATIO_MANUAL, .tRect = {64, 20, 500, 500}},*/
                    .tAspectRatio = {.eMode = AX_IVPS_ASPECT_RATIO_STRETCH, .eAligns = {AX_IVPS_ASPECT_RATIO_HORIZONTAL_RIGHT, AX_IVPS_ASPECT_RATIO_VERTICAL_TOP}},
                },
                /* channel0 filter1 */
                {
                    .bEngage = AX_TRUE,
                    .eEngine = AX_IVPS_ENGINE_VPP,
                    .bCrop = AX_FALSE,
                    .tCompressInfo = {
                        .enCompressMode = AX_COMPRESS_MODE_NONE,
                        .u32CompressLevel = 4,
                    },
                    .tCropRect = {
                        .nX = 0,
                        .nY = 0,
                        .nW = 0,
                        .nH = 0,
                    },
                    .nDstPicWidth = 1920,
                    .nDstPicHeight = 1080,
                    .nDstPicStride = 1920,
                    .eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR,
                    /*.tAspectRatio = {.eMode = IVPS_ASPECT_RATIO_MANUAL, .tRect = {64, 20, 500, 500}},*/
                    .tAspectRatio = {.eMode = AX_IVPS_ASPECT_RATIO_STRETCH, .eAligns = {AX_IVPS_ASPECT_RATIO_HORIZONTAL_RIGHT, AX_IVPS_ASPECT_RATIO_VERTICAL_TOP}},
                },
            },
            {
                /* channel1 filter0 */
                {
                    .bEngage = AX_TRUE,
                    .eEngine = AX_IVPS_ENGINE_GDC,
                    .bCrop = AX_FALSE,
                    .tCropRect = {
                        .nX = 0,
                        .nY = 0,
                        .nW = 0,
                        .nH = 0,
                    },
                    .nDstPicWidth = 3840,
                    .nDstPicHeight = 2160,
                    .nDstPicStride = 3840,
                    .eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR,
                },
                /* channel1 filter1 */
                {
                    .bEngage = AX_TRUE,
                    .eEngine = AX_IVPS_ENGINE_GDC,
                    .bCrop = AX_FALSE,
                    .tCompressInfo = {
                        .enCompressMode = AX_COMPRESS_MODE_NONE,
                        .u32CompressLevel = 4,
                    },
                    .tCropRect = {
                        .nX = 0,
                        .nY = 0,
                        .nW = 0,
                        .nH = 0,
                    },
                    .nDstPicWidth = 3840,
                    .nDstPicHeight = 2160,
                    .nDstPicStride = 3840,
                    .eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR,
                    /*.tAspectRatio = {.eMode = IVPS_ASPECT_RATIO_MANUAL, .tRect = {64, 20, 500, 500}},*/
                    .tAspectRatio = {.eMode = AX_IVPS_ASPECT_RATIO_STRETCH, .eAligns = {AX_IVPS_ASPECT_RATIO_HORIZONTAL_RIGHT, AX_IVPS_ASPECT_RATIO_VERTICAL_TOP}},
                },
            },
            {
                /* channel2 filter0 */
                {
                    .bEngage = AX_TRUE,
                    .eEngine = AX_IVPS_ENGINE_VGP,
                    .bCrop = AX_FALSE,
                    .tCropRect = {
                        .nX = 0,
                        .nY = 0,
                        .nW = 0,
                        .nH = 0,
                    },
                    .nDstPicWidth = 2048,
                    .nDstPicHeight = 2048,
                    .nDstPicStride = 2048,
                    .eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR,
                },
                /* channel2 filter1 */
                {
                    .bEngage = AX_TRUE,
                    .eEngine = AX_IVPS_ENGINE_VGP,
                    .bCrop = AX_FALSE,
                    .tCompressInfo = {
                        .enCompressMode = AX_COMPRESS_MODE_NONE,
                        .u32CompressLevel = 4,
                    },
                    .tCropRect = {
                        .nX = 0,
                        .nY = 0,
                        .nW = 0,
                        .nH = 0,
                    },
                    .nDstPicWidth = 1024,
                    .nDstPicHeight = 1024,
                    .nDstPicStride = 1024,
                    .eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR,
                },
            },
            {
                /* channel3 filter0 */
                {
                    .bEngage = AX_TRUE,
                    .eEngine = AX_IVPS_ENGINE_TDP,
                    .bCrop = AX_FALSE,
                    .tCropRect = {
                        .nX = 0,
                        .nY = 0,
                        .nW = 0,
                        .nH = 0,
                    },
                    .nDstPicWidth = 1280,
                    .nDstPicHeight = 768,
                    .nDstPicStride = 1280,
                    .eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR,
                },
                /* channel3 filter1 */
                {
                    .bEngage = AX_TRUE,
                    .eEngine = AX_IVPS_ENGINE_TDP,
                    .bCrop = AX_FALSE,
                    .tCompressInfo = {
                        .enCompressMode = AX_COMPRESS_MODE_NONE,
                        .u32CompressLevel = 4,
                    },
                    .tCropRect = {
                        .nX = 0,
                        .nY = 0,
                        .nW = 0,
                        .nH = 0,
                    },
                    .nDstPicWidth = 1920,
                    .nDstPicHeight = 1080,
                    .nDstPicStride = 1920,
                    .eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR,
                    /*.tAspectRatio = {.eMode = IVPS_ASPECT_RATIO_MANUAL, .tRect = {64, 20, 500, 500}},*/
                    .tAspectRatio = {.eMode = AX_IVPS_ASPECT_RATIO_STRETCH, .eAligns = {AX_IVPS_ASPECT_RATIO_HORIZONTAL_RIGHT, AX_IVPS_ASPECT_RATIO_VERTICAL_TOP}},
                },
            },
        },
    },
    .nIvpsRepeatNum = 3,
};

SAMPLE_IVPS_GRP_T gSampleIvpsGrpExt = {
    .nIvpsGrp = 0,

    .tGrpAttr = {
        .ePipeline = AX_IVPS_PIPELINE_DEFAULT,
    },
    .tPipelineAttr = {
        .nOutChnNum = 1,
        .nOutFifoDepth = {2, 2, 2},
        .tFilter = {
            {
                /* group filter0 */
                {
                    .bEngage = AX_FALSE,
                    .eEngine = AX_IVPS_ENGINE_TDP,
                    .nDstPicWidth = 1024,
                    .nDstPicHeight = 1024,
                    .nDstPicStride = 1024,
                    .eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR,
                },
            },
            {
                /* channel 0 filter0 */
                {
                    .bEngage = AX_TRUE,
                    .eEngine = AX_IVPS_ENGINE_TDP,
                    .nDstPicWidth = 1024,
                    .nDstPicHeight = 1024,
                    .nDstPicStride = 1024,

                    .eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR,
                },
            },
        },
    },
};

static AX_VOID *IVPS_GetFrameThread(AX_VOID *pArg)
{

    AX_S32 i, ret = 0;
    AX_S32 events_num = 0;
    SAMPLE_IVPS_GRP_T *pThis = (SAMPLE_IVPS_GRP_T *)pArg;

    ALOGI("--- IVPS Grp: %d, Chn num: %d", pThis->nIvpsGrp, pThis->tPipelineAttr.nOutChnNum);

    for (i = 0; i < pThis->tPipelineAttr.nOutChnNum; i++)
    {
        pThis->arrOutChns[i].bEmpty = AX_TRUE;
    }

    AX_VIDEO_FRAME_T tDstFrame[pThis->tPipelineAttr.nOutChnNum];
    while (!ThreadLoopStateGet())
    {
        for (i = 0; i < pThis->tPipelineAttr.nOutChnNum; i++)
        {
            if (!pThis->arrOutChns[i].bEmpty)
            {
                break;
            }
        }
        if (i == pThis->tPipelineAttr.nOutChnNum)
        {
            ALOGI("Enter DevPolWait");
            events_num = DevPolWait(pThis->nEfd, pThis->tPipelineAttr.nOutChnNum, 3000);
            if (!events_num)
            {
                break;
            }
            for (i = 0; i < pThis->tPipelineAttr.nOutChnNum; i++)
            {
                pThis->arrOutChns[i].bEmpty = AX_FALSE;
            }
        }

        for (i = 0; i < pThis->tPipelineAttr.nOutChnNum; i++)
        {
            if (!pThis->arrOutChns[i].bEmpty)
            {
                ret = AX_IVPS_GetChnFrame(pThis->nIvpsGrp, i, &tDstFrame[i], 0);

                if (ret == AX_ERR_IVPS_BUF_EMPTY)
                {
                    /* reach EOF */
                    ALOGW("warning! CHN[%d] is empty ret:%x", i, ret);
                    pThis->arrOutChns[i].bEmpty = AX_TRUE;
                    continue;
                }

                printf("AX_IVPS_GetChnFrame(%lld): Chn:%d, (%d x %d) Stride:%d, Phy:%llx, UserData:%llx, PTS:%llx, BlockId:%x\n",
                       tDstFrame[i].u64SeqNum, i, tDstFrame[i].u32Width, tDstFrame[i].u32Height, tDstFrame[i].u32PicStride[0],
                       tDstFrame[i].u64PhyAddr[0], tDstFrame[i].u64UserData, tDstFrame[i].u64PTS, tDstFrame[i].u32BlkId[0]);
                if (pThis->bSaveFile) {
                        SaveFile(&tDstFrame[i], pThis->nIvpsGrp, i, pThis->pFilePath, "PIPELINE");
                }

                ret = AX_IVPS_ReleaseChnFrame(pThis->nIvpsGrp, i, &tDstFrame[i]);
                if (ret)
                {
                    ALOGE("AX_IVPS_ReleaseFrame fail, ret=0x%x", ret);
                    return (AX_VOID *)-1;
                }
            }
        }
        ALOGI("AX_IVPS_GetFrame");
    }

    ALOGI("IVPS END");
    return (AX_VOID *)0;
}

static AX_VOID *IVPS_SendFrameThread(AX_VOID *pArg)
{
    AX_S32 ret = 0, i = 0;
    AX_S32 nStreamIdx = 0;

    SAMPLE_IVPS_GRP_T *pThis = (SAMPLE_IVPS_GRP_T *)pArg;

    ALOGI("+++ IVPS Grp: %d", pThis->nIvpsGrp);

    while (!ThreadLoopStateGet() && (pThis->nIvpsRepeatNum || pThis->nIvpsStreamNum > 0))
    {
        ALOGI("nRepeatNum:%d", pThis->nIvpsRepeatNum);
        if (pThis->nIvpsRepeatNum > 0)
            pThis->nIvpsRepeatNum--;
        if (pThis->nIvpsStreamNum > 0)
        {
            FrameBufGet(nStreamIdx, &pThis->tFrameInput, pThis->pFileName);
        }
        pThis->tFrameInput.u64SeqNum++;
        pThis->tFrameInput.u64PTS = GetTickCount();
        printf("AX_IVPS_SendFrame[%lld]: nOutChnNum:%d PTS:%lld UserData: %llx +++\n",
               pThis->tFrameInput.u64SeqNum, pThis->tPipelineAttr.nOutChnNum,
               pThis->tFrameInput.u64PTS, pThis->tFrameInput.u64UserData);

        ret = AX_IVPS_SendFrame(pThis->nIvpsGrp, &pThis->tFrameInput, -1);

        if (pThis->nIvpsStreamNum > 0)
        {
            ret = AX_POOL_ReleaseBlock(pThis->tFrameInput.u32BlkId[0]);
            if (ret)
            {
                ALOGE("AX_POOL_ReleaseBlock(Grp: %d) failed, ret=0x%x.", pThis->nIvpsGrp, ret);
                return (AX_VOID *)-1;
            }
            pThis->nIvpsStreamNum--;
            nStreamIdx++;
        }
        usleep(100000);
        ALOGI("AX_IVPS_SendFrame(Chn: %d) ---, ret: 0x%x", pThis->arrOutChns[i].nIvpsChn, ret);
        if (IVPS_SUCC != ret)
        {
            ALOGE("AX_IVPS_SendFrame(Chn %d) failed, ret=0x%x.", pThis->arrOutChns[i].nIvpsChn, ret);
            continue;
        }
    }
    if (pThis->nIvpsStreamNum == 0) {
        ret = AX_POOL_ReleaseBlock(pThis->tFrameInput.u32BlkId[0]);
        if (ret)
        {
            ALOGE("IVPS Release BlkId fail, ret=0x%x", ret);
        }
    }

    return (AX_VOID *)0;
}

AX_S32 IVPS_StartGrp(SAMPLE_IVPS_GRP_T *p)
{
    int ret = 0;
    AX_S32 nFd;
    AX_IVPS_POOL_ATTR_T PoolAttr = {0};

    p->nEfd = DevEfdCreate(EPOLL_MAXUSERS);

    switch (p->tPipelineAttr.tFilter[1][0].eEngine)
    {
    case AX_IVPS_ENGINE_TDP:
        memcpy(&p->tPipelineAttr.tFilter[1][0].tTdpCfg, &gSampleTdpCfg1, sizeof(AX_IVPS_TDP_CFG_T));
        break;
    case AX_IVPS_ENGINE_GDC:
        memcpy(&p->tPipelineAttr.tFilter[1][0].tGdcCfg, &gSampleGdcCfg, sizeof(AX_IVPS_GDC_CFG_T));
        break;
    default:
        break;
    }

    switch (p->tPipelineAttr.tFilter[2][0].eEngine)
    {
    case AX_IVPS_ENGINE_TDP:
        memcpy(&p->tPipelineAttr.tFilter[2][0].tTdpCfg, &gSampleTdpCfg1, sizeof(AX_IVPS_TDP_CFG_T));
        break;
    case AX_IVPS_ENGINE_GDC:
        memcpy(&p->tPipelineAttr.tFilter[2][0].tGdcCfg, &gSampleGdcCfg, sizeof(AX_IVPS_GDC_CFG_T));
        break;
    default:
        break;
    }

    memcpy(&p->tPipelineAttr.tFilter[3][0].tTdpCfg, &gSampleTdpCfg2, sizeof(AX_IVPS_TDP_CFG_T));

    ret = AX_IVPS_CreateGrp(p->nIvpsGrp, &p->tGrpAttr);
    if (IVPS_SUCC != ret)
    {
        ALOGE("AX_IVPS_CreateGrp(Grp: %d) failed, ret=0x%x.", p->nIvpsGrp, ret);
        return -1;
    }
    ret = AX_IVPS_SetPipelineAttr(p->nIvpsGrp, &p->tPipelineAttr);
    if (IVPS_SUCC != ret)
    {
        ALOGE("AX_IVPS_SetPipelineAttr(Grp: %d) failed, ret=0x%x.", p->nIvpsGrp, ret);
        return -1;
    }

    /* set pool type */
    if (POOL_SOURCE_PRIVATE == p->ePoolSrc) {
        PoolAttr.ePoolSrc = POOL_SOURCE_PRIVATE;
        PoolAttr.nFrmBufNum = 10;
    } else if (POOL_SOURCE_USER == p->ePoolSrc) {
        PoolAttr.ePoolSrc = POOL_SOURCE_USER;
        PoolAttr.PoolId = p->user_pool_id;
    } else {
        PoolAttr.ePoolSrc = POOL_SOURCE_COMMON;
    }
    printf("PoolAttr.ePoolSrc :%d\n", PoolAttr.ePoolSrc);
    ret = AX_IVPS_SetGrpPoolAttr(p->nIvpsGrp, &PoolAttr);
    if (IVPS_SUCC != ret)
    {
        ALOGE("AX_IVPS_SetGrpPoolAttr(Grp: %d) failed, ret=0x%x.", p->nIvpsGrp, ret);
        return -1;
    }
    for (IVPS_CHN chn = 0; chn < p->tPipelineAttr.nOutChnNum; chn++)
    {
        ret = AX_IVPS_SetChnPoolAttr(p->nIvpsGrp, chn, &PoolAttr);
        if (IVPS_SUCC != ret)
        {
            ALOGE("AX_IVPS_SetChnPoolAttr(Grp: %d) failed, ret=0x%x.", p->nIvpsGrp, ret);
            return -1;
        }
    }

    printf("nOutChnNum :%d\n", p->tPipelineAttr.nOutChnNum);
    for (IVPS_CHN chn = 0; chn < p->tPipelineAttr.nOutChnNum; chn++)
    {
        printf("chn id :%d", chn);

        ret = AX_IVPS_EnableChn(p->nIvpsGrp, chn);
        if (IVPS_SUCC != ret)
        {
            ALOGE("AX_IVPS_EnableChn(Chn: %d) failed, ret=0x%x.", chn, ret);
            return -1;
        }

        nFd = AX_IVPS_GetChnFd(p->nIvpsGrp, chn);
        p->arrOutChns[chn].nFd = nFd;
        DevFdListen(p->nEfd, nFd);
    }

    ret = AX_IVPS_StartGrp(p->nIvpsGrp);
    if (IVPS_SUCC != ret)
    {
        ALOGE("AX_IVPS_StartGrp(Grp: %d) failed, ret=0x%x.", p->nIvpsGrp, ret);
        return -1;
    }

    return 0;
}

AX_S32 IVPS_StopGrp(const SAMPLE_IVPS_GRP_T *p)
{
    AX_S32 ret = IVPS_SUCC;

    ret = AX_IVPS_StopGrp(p->nIvpsGrp);
    if (IVPS_SUCC != ret)
    {
        ALOGE("AX_IVPS_StopGrp(Grp: %d) failed(this grp is not started) ret=0x%x.", p->nIvpsGrp, ret);
        return -1;
    }

    for (IVPS_CHN chn = 0; chn < p->tPipelineAttr.nOutChnNum; ++chn)
    {
        ret = AX_IVPS_DisableChn(p->nIvpsGrp, chn);
        if (IVPS_SUCC != ret)
        {
            ALOGE("AX_IVPS_DestoryChn(Chn: %d) failed, ret=0x%x.", chn, ret);
            return -1;
        }
    }

    ret = AX_IVPS_DestoryGrp(p->nIvpsGrp);
    if (IVPS_SUCC != ret)
    {
        ALOGE("AX_IVPS_DestoryGrp(Grp: %d) failed, ret=0x%x.", p->nIvpsGrp, ret);
        return -1;
    }

    /* AX_IVPS_CloseAllFd(); */
    DevEfdRelease(p->nEfd);
    return 0;
}

AX_S32 IVPS_StartGrpV2(IVPS_CHN IvpsChn)
{
    SAMPLE_IVPS_GRP_T *p = &gSampleIvpsGrpExt;
    int ret = 0;

    p->nEfd = DevEfdCreate(1);

    ALOGI("chn id :%d\n", IvpsChn);
    ret = AX_IVPS_CreateGrp(p->nIvpsGrp, &p->tGrpAttr);
    if (IVPS_SUCC != ret)
    {
        ALOGE("AX_IVPS_CreateGrp(Grp: %d) failed, ret=0x%x.", p->nIvpsGrp, ret);
        return -1;
    }

    ret = AX_IVPS_EnableChn(p->nIvpsGrp, IvpsChn);
    if (IVPS_SUCC != ret)
    {
        ALOGE("AX_IVPS_CreateChn(Chn: %d) failed, ret=0x%x.", IvpsChn, ret);
        return -1;
    }

    ret = AX_IVPS_StartGrp(p->nIvpsGrp);

    if (IVPS_SUCC != ret)
    {
        ALOGE("AX_IVPS_StartChn(Chn: %d) failed, ret=0x%x.", IvpsChn, ret);
        return -1;
    }

    p->arrOutChns[0].nFd = AX_IVPS_GetChnFd(p->nIvpsGrp, IvpsChn);
    DevFdListen(p->nEfd, p->arrOutChns[0].nFd);

    return 0;
}

AX_S32 IVPS_StopGrpV2(IVPS_CHN IvpsChn)
{
    AX_S32 ret = IVPS_SUCC;

    const SAMPLE_IVPS_GRP_T *p = &gSampleIvpsGrpExt;

    ret = AX_IVPS_StopGrp(p->nIvpsGrp);
    if (IVPS_SUCC != ret)
    {
        ALOGE("AX_IVPS_StopChn(Chn: %d) failed(this chn is not started) ret=0x%x.", IvpsChn, ret);
        return -1;
    }

    ret = AX_IVPS_DisableChn(p->nIvpsGrp, IvpsChn);
    if (IVPS_SUCC != ret)
    {
        ALOGE("AX_IVPS_DestoryChn(Chn: %d) failed, ret=0x%x.", IvpsChn, ret);
        return -1;
    }

    ret = AX_IVPS_DestoryGrp(p->nIvpsGrp);
    if (IVPS_SUCC != ret)
    {
        ALOGE("AX_IVPS_DestoryGrp(Grp: %d) failed, ret=0x%x.", p->nIvpsGrp, ret);
        return -1;
    }

    DevFdRelease(p->arrOutChns[0].nFd);

    DevEfdRelease(p->nEfd);

    return 0;
}

/*
 * IVPS_ThreadStart()
 * IVPS frame send and get can be done separately in different threads.
 */
AX_S32 IVPS_ThreadStart(AX_VOID *src, AX_VOID *dst)
{
    SAMPLE_IVPS_GRP_T *pGrp = NULL;
    if (dst)
    {
        pGrp = (SAMPLE_IVPS_GRP_T *)dst;
        if (0 != pthread_create(&(pGrp->get_frame_tid), NULL, IVPS_GetFrameThread, dst))
        {
            pGrp->get_frame_tid = 0;
            return -1;
        }
    }

    if (src)
    {
        pGrp = (SAMPLE_IVPS_GRP_T *)src;
        if (0 != pthread_create(&(pGrp->send_frame_tid), NULL, IVPS_SendFrameThread, src))
        {
            pGrp->send_frame_tid = 0;
            return -1;
        }
    }
    return 0;
}

AX_S32 IVPS_ThreadStop(SAMPLE_IVPS_GRP_T *pGrp)
{
    if (pGrp->send_frame_tid)
    {
        pthread_join(pGrp->send_frame_tid, NULL);
    }
    if (pGrp->get_frame_tid)
    {
        pthread_join(pGrp->get_frame_tid, NULL);
    }
    return 0;
}
