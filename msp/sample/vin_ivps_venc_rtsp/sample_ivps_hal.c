/**********************************************************************************
 *
 * Copyright (c) 2019-2020 Beijing AXera Technology Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Beijing AXera Technology Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Beijing AXera Technology Co., Ltd.
 *
 **********************************************************************************/
#include "sample_utils.h"

AX_IVPS_TDP_CFG_T gSampleTdpCfg = {
    .bMirror = AX_FALSE,
    .bFlip = AX_FALSE,
    .eRotation = AX_IVPS_ROTATION_0,
};

AX_IVPS_GDC_CFG_T gSampleGdcCfg = {
    .eRotation = AX_IVPS_ROTATION_0,
};

SAMPLE_GRP_T gSampleGrp = {
    .nIvpsGrp = 1,
    .pFilePath = ".",
    .tGrpAttr = {
        .ePipeline = AX_IVPS_PIPELINE_DEFAULT,
    },
    .tPipelineAttr = {

        .nOutChnNum = 3,
        .nOutFifoDepth = {2, 2, 2, 2},
        .tFilter = {
            {},
            {
                /* channel 0 filter0 */
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
            {
                /* channel 1 filter0 */
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
                    .nDstPicWidth = 2688,
                    .nDstPicHeight = 1520,
                    .nDstPicStride = 2688,
                    .eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR,
                },
            },
            {
                /* channel 2 filter0 */
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
                    .nDstPicWidth = 720,
                    .nDstPicHeight = 576,
                    .nDstPicStride = 720,
                    .eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR,
                },
            },
        },
    },
};

static AX_VOID *IVPS_SendFrameThread(AX_VOID *pArg)
{
    AX_S32 ret = 0, i = 0;
    AX_S32 nStreamIdx = 0;

    SAMPLE_GRP_T *pThis = (SAMPLE_GRP_T *)pArg;

    ALOGI("+++ IVPS Grp: %d", pThis->nIvpsGrp);

    while (!ThreadLoopStateGet())
    {
        pThis->tFrameInput.u64SeqNum++;
        pThis->tFrameInput.u64PTS = GetTickCount();
        printf("AX_IVPS_SendFrame(nChnNum: %d) seq num:%lld PTS:%lld userdata: %llx +++\n",
               pThis->tPipelineAttr.nOutChnNum, pThis->tFrameInput.u64SeqNum,
               pThis->tFrameInput.u64PTS, pThis->tFrameInput.u64UserData);

        ret = AX_IVPS_SendFrame(pThis->nIvpsGrp, &pThis->tFrameInput, -1);

        usleep(40000);
        ALOGI("AX_IVPS_SendFrame(Chn: %d) ---, ret: 0x%x", pThis->arrOutChns[i].nIvpsChn, ret);
        if (IVPS_SUCC != ret)
        {
            ALOGE("AX_IVPS_SendFrame(Chn %d) failed, ret=0x%x.", pThis->arrOutChns[i].nIvpsChn, ret);
            continue;
        }
    }
    return (AX_VOID *)0;
}

/*
 * SAMPLE_IVPS_ThreadStart()
 * IVPS frame send and get can be done separately in different threads.
 */
AX_S32 SAMPLE_IVPS_ThreadStart(AX_VOID *src)
{
    pthread_t tid = 0;
    if (src)
    {
        if (0 != pthread_create(&tid, NULL, IVPS_SendFrameThread, src))
        {
            return -1;
        }
        pthread_detach(tid);
    }
    return 0;
}

AX_S32 SAMPLE_IVPS_StartGrp(SAMPLE_GRP_T *p)
{
    int ret = 0;

    if (p->tPipelineAttr.tFilter[1][0].eEngine == AX_IVPS_ENGINE_TDP)
        memcpy(&p->tPipelineAttr.tFilter[1][0].tTdpCfg, &gSampleTdpCfg, sizeof(AX_IVPS_TDP_CFG_T));
    else
        memcpy(&p->tPipelineAttr.tFilter[1][0].tGdcCfg, &gSampleGdcCfg, sizeof(AX_IVPS_GDC_CFG_T));

    if (p->tPipelineAttr.tFilter[2][0].eEngine == AX_IVPS_ENGINE_TDP)
        memcpy(&p->tPipelineAttr.tFilter[2][0].tTdpCfg, &gSampleTdpCfg, sizeof(AX_IVPS_TDP_CFG_T));
    else
        memcpy(&p->tPipelineAttr.tFilter[2][0].tGdcCfg, &gSampleGdcCfg, sizeof(AX_IVPS_GDC_CFG_T));

    if (p->tPipelineAttr.tFilter[3][0].eEngine == AX_IVPS_ENGINE_TDP)
        memcpy(&p->tPipelineAttr.tFilter[3][0].tTdpCfg, &gSampleTdpCfg, sizeof(AX_IVPS_TDP_CFG_T));
    else
        memcpy(&p->tPipelineAttr.tFilter[3][0].tGdcCfg, &gSampleGdcCfg, sizeof(AX_IVPS_GDC_CFG_T));

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

    printf("nOutChnNum :%d\n", p->tPipelineAttr.nOutChnNum);
    for (IVPS_CHN chn = 0; chn < p->tPipelineAttr.nOutChnNum; chn++)
    {
        ret = AX_IVPS_EnableChn(p->nIvpsGrp, chn);
        if (IVPS_SUCC != ret)
        {
            ALOGE("AX_IVPS_EnableChn(Chn: %d) failed, ret=0x%x.", chn, ret);
            return -1;
        }
    }

    ret = AX_IVPS_StartGrp(p->nIvpsGrp);
    if (IVPS_SUCC != ret)
    {
        ALOGE("AX_IVPS_StartGrp(Grp: %d) failed, ret=0x%x.", p->nIvpsGrp, ret);
        return -1;
    }

    return 0;
}

AX_S32 SAMPLE_IVPS_StopGrp(const SAMPLE_GRP_T *p)
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

    return 0;
}