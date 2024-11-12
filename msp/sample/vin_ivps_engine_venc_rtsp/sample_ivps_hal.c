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

        .nOutChnNum = 4,
        .nOutFifoDepth = {2, 2, 2, 2, 1},
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
                    .nDstPicWidth = 2688,
                    .nDstPicHeight = 1520,
                    .nDstPicStride = 2688,
                    .eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR,
                    /*.tAspectRatio = {.eMode = IVPS_ASPECT_RATIO_MANUAL, .tRect = {64, 20, 500, 500}},*/
                    .tAspectRatio = {.eMode = AX_IVPS_ASPECT_RATIO_STRETCH, .eAligns = {AX_IVPS_ASPECT_RATIO_HORIZONTAL_CENTER, AX_IVPS_ASPECT_RATIO_VERTICAL_CENTER}},
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
                    .nDstPicWidth = 1920,
                    .nDstPicHeight = 1080,
                    .nDstPicStride = 1920,
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
            {
                /* channel 3 filter0 */
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
                    .nDstPicWidth = INFER_WIDTH,
                    .nDstPicHeight = INFER_HEIGHT,
                    .nDstPicStride = INFER_WIDTH,
                    .eDstPicFormat = INFER_FORMAT,
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

void visualize_infer_result(IVPS_RGN_HANDLE handle, SAMPLE_ENGINE_Results *objects, float aspect_w, float aspect_h)
{
    int obj_num = objects->obj_count;

    AX_IVPS_RGN_DISP_GROUP_T *rgn_grp = (AX_IVPS_RGN_DISP_GROUP_T *)malloc(sizeof(AX_IVPS_RGN_DISP_GROUP_T));
    memset(rgn_grp, 0, sizeof(AX_IVPS_RGN_DISP_GROUP_T));

    rgn_grp->nNum = obj_num;
    rgn_grp->tChnAttr.nZindex = 0;
    rgn_grp->tChnAttr.bSingleCanvas = AX_TRUE;
    rgn_grp->tChnAttr.nAlpha = 255;
    rgn_grp->tChnAttr.eFormat = AX_FORMAT_RGBA8888;
    AX_IVPS_RGN_DISP_T *disp = rgn_grp->arrDisp;

    for (int i = 0; i < (obj_num < AX_IVPS_REGION_MAX_DISP_NUM ? obj_num : AX_IVPS_REGION_MAX_DISP_NUM); i++)
    {
        disp[i].bShow = AX_TRUE;
        disp[i].eType = AX_IVPS_RGN_TYPE_RECT;
        disp[i].uDisp.tPolygon.tRect.nX = (int)(objects->objs[i].x * aspect_w);
        disp[i].uDisp.tPolygon.tRect.nY = (int)(objects->objs[i].y * aspect_h);
        disp[i].uDisp.tPolygon.tRect.nW = (int)(objects->objs[i].width * aspect_w);
        disp[i].uDisp.tPolygon.tRect.nH = (int)(objects->objs[i].height * aspect_h);
        if (disp[i].uDisp.tPolygon.tRect.nW == 0 || disp[i].uDisp.tPolygon.tRect.nH == 0)
        {
            disp[i].bShow = AX_FALSE;
        }
        // printf("x y w h: %d %d %d %d\n", disp[i].uDisp.tPolygon.tRect.nX, disp[i].uDisp.tPolygon.tRect.nY, disp[i].uDisp.tPolygon.tRect.nW, disp[i].uDisp.tPolygon.tRect.nH);
        disp[i].uDisp.tPolygon.nLineWidth = 3;
        disp[i].uDisp.tPolygon.bSolid = AX_FALSE;
        // disp[i].uDisp.tPolygon.bCornerRect = AX_FALSE;
        disp[i].uDisp.tPolygon.nAlpha = 255;
        disp[i].uDisp.tPolygon.nColor = objects->objs[i].color;
    }

    int ret = AX_IVPS_RGN_Update(handle, rgn_grp);
    if (ret != 0)
    {
        printf("AX_IVPS_RGN_Update failed. ret=0x%x\n", ret);
    }

    free(rgn_grp);
}

static AX_VOID *IVPS_GetFrameThread(AX_VOID *pArg)
{
    AX_S32 ret = 0;
    SAMPLE_GRP_T *pThis = (SAMPLE_GRP_T *)pArg;
    AX_VIDEO_FRAME_T engine_input_frame;
    IVPS_GRP nGrp = pThis->nIvpsGrp;
    AX_IVPS_PIPELINE_ATTR_T stPipelineAttr;
    ret = AX_IVPS_GetPipelineAttr(nGrp, &stPipelineAttr);
    if (ret != 0)
    {
        printf("AX_IVPS_GetPipelineAttr failed. ret=0x%x\n", ret);
        return (AX_VOID *)0;
    }

    int chn_num = stPipelineAttr.nOutChnNum - 1;
    const int infer_chn = INFER_CHN;

    IVPS_RGN_HANDLE *rgn_handles = (IVPS_RGN_HANDLE *)malloc(sizeof(IVPS_RGN_HANDLE) * chn_num);
    float *aspect_w = (float *)malloc(sizeof(float) * chn_num);
    float *aspect_h = (float *)malloc(sizeof(float) * chn_num);
    for (int i = 0; i < chn_num; i++)
    {
        rgn_handles[i] = AX_IVPS_RGN_Create();
        ret = AX_IVPS_RGN_AttachToFilter(rgn_handles[i], nGrp, ((i + 1) << 4));
        if (ret != 0)
        {
            printf("AX_IVPS_RGN_AttachToFilter filter=%d failed. ret=%d\n", i + 1, ret);
            AX_IVPS_RGN_Destroy(rgn_handles[i]);
            free(rgn_handles);
            free(aspect_w);
            free(aspect_h);
            return (AX_VOID *)0;
        }

        aspect_w[i] = stPipelineAttr.tFilter[i + 1][0].nDstPicWidth * 1.0f / INFER_WIDTH;
        aspect_h[i] = stPipelineAttr.tFilter[i + 1][0].nDstPicHeight * 1.0f / INFER_HEIGHT;

        // printf("aspect ratio[%d]: %f %f\n", i, aspect_w[i], aspect_h[i]);
    }

    while (!ThreadLoopStateGet())
    {
        // Get infer_chn frame
        memset(&engine_input_frame, 0, sizeof(AX_VIDEO_FRAME_T));
        ret = AX_IVPS_GetChnFrame(nGrp, infer_chn, &engine_input_frame, 200);
        if (ret != 0)
        {
            printf("AX_IVPS_GetChnFrame(Grp:%d Chn:%d) failed, ret=0x%x.\n", nGrp, infer_chn, ret);
            continue;
        }

        // inference
        engine_input_frame.u64VirAddr[0] = (AX_U64)AX_POOL_GetBlockVirAddr(engine_input_frame.u32BlkId[0]);
        engine_input_frame.u64PhyAddr[0] = AX_POOL_Handle2PhysAddr(engine_input_frame.u32BlkId[0]);

        SAMPLE_ENGINE_Results stResults;
        stResults.obj_count = 0;
        ret = SAMPLE_ENGINE_Inference(&engine_input_frame, &stResults);
        if (ret != 0)
        {
            printf("inference failed, ret=%d.\n", ret);
        }
        // if (stResults.obj_count > 0)
        //     printf("Detected %d object(s)\n", stResults.obj_count);

        // draw frame
        for (int i = 0; i < chn_num; i++)
            visualize_infer_result(rgn_handles[i], &stResults, aspect_w[i], aspect_h[i]);

        ret = AX_IVPS_ReleaseChnFrame(nGrp, infer_chn, &engine_input_frame);
        if (ret != 0)
        {
            printf("AX_IVPS_ReleaseChnFrame(Grp:%d Chn:%d) failed, ret=0x%x.\n", nGrp, infer_chn, ret);
        }
    }

    for (int i = 0; i < chn_num; i++)
    {
        ret = AX_IVPS_RGN_DetachFromFilter(rgn_handles[i], nGrp, ((i + 1) << 4));
        if (ret != 0)
        {
            printf("AX_IVPS_RGN_DetachFromFilter failed, ret=0x%x.\n", ret);
        }

        ret = AX_IVPS_RGN_Destroy(rgn_handles[i]);
        if (ret != 0)
        {
            printf("AX_IVPS_RGN_Destroy chn:%d failed, ret=0x%x.\n", i, ret);
        }
    }

    free(rgn_handles);
    free(aspect_w);
    free(aspect_h);

    return (AX_VOID *)0;
}

/*
 * SAMPLE_IVPS_ThreadStart()
 * IVPS frame send and get can be done separately in different threads.
 */
AX_S32 SAMPLE_IVPS_ThreadStart(SAMPLE_GRP_T *src)
{
    pthread_t tid = 0;
    if (src)
    {
        if (0 != pthread_create(&tid, NULL, IVPS_GetFrameThread, (AX_VOID*)src))
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

    if (p->tPipelineAttr.tFilter[4][0].eEngine == AX_IVPS_ENGINE_TDP)
        memcpy(&p->tPipelineAttr.tFilter[4][0].tTdpCfg, &gSampleTdpCfg, sizeof(AX_IVPS_TDP_CFG_T));
    else
        memcpy(&p->tPipelineAttr.tFilter[4][0].tGdcCfg, &gSampleGdcCfg, sizeof(AX_IVPS_GDC_CFG_T));

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