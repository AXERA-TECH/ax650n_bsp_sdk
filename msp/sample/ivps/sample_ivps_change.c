/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include "sample_ivps_main.h"

SAMPLE_IVPS_CHANGE_T gSampleChange = {
    .nRepeatNum = -1,
    .nFilterIdx = 0x11,
    .tMaxValue = {
        .nDstPicStride = 4096,
        .nDstPicWidth = 4096,
        .nDstPicHeight = 4096,
    },
    .tMinValue = {
        .nDstPicStride = 256,
        .nDstPicWidth = 256,
        .nDstPicHeight = 256,
    },
    .nWidthStep = 8,
    .nHeightStep = 8,
    .nCropX0Step = 0,
    .nCropY0Step = 0,
    .nCropWStep = 0,
    .nCropHStep = 0,
};

static AX_S32 IVPS_FilterAttrParaUpdate(SAMPLE_IVPS_GRP_T *pGrp)
{
    AX_IVPS_CHN_ATTR_T ChnAttr = {0};
    int pipe_idx = (gSampleChange.nFilterIdx & 0xf0) >> 4;
    int filter_idx = (gSampleChange.nFilterIdx & 0xf);
    printf("pipe_idx:%d filter_idx:%d\n", pipe_idx, filter_idx);

    for (pipe_idx = 1; pipe_idx < pGrp->tPipelineAttr.nOutChnNum + 1; pipe_idx++) {
	/* Only change filter1, filter0 don't change. */
        for (filter_idx = 1; filter_idx < 2; filter_idx++) {
            if (pGrp->tPipelineAttr.tFilter[pipe_idx][filter_idx].bEngage) {
                pGrp->tPipelineAttr.tFilter[pipe_idx][filter_idx].nDstPicWidth = (pGrp->tPipelineAttr.tFilter[pipe_idx][filter_idx].nDstPicWidth + gSampleChange.nWidthStep) % gSampleChange.tMaxValue.nDstPicWidth;
                pGrp->tPipelineAttr.tFilter[pipe_idx][filter_idx].nDstPicHeight = (pGrp->tPipelineAttr.tFilter[pipe_idx][filter_idx].nDstPicHeight +gSampleChange.nHeightStep) % gSampleChange.tMaxValue.nDstPicHeight;
                if (pGrp->tPipelineAttr.tFilter[pipe_idx][filter_idx].nDstPicWidth < gSampleChange.tMinValue.nDstPicWidth) {
                    pGrp->tPipelineAttr.tFilter[pipe_idx][filter_idx].nDstPicWidth = gSampleChange.tMinValue.nDstPicWidth;
                }
                if (pGrp->tPipelineAttr.tFilter[pipe_idx][filter_idx].nDstPicHeight < gSampleChange.tMinValue.nDstPicHeight) {
                    pGrp->tPipelineAttr.tFilter[pipe_idx][filter_idx].nDstPicHeight = gSampleChange.tMinValue.nDstPicHeight;
                }

                if (pGrp->tPipelineAttr.tFilter[pipe_idx][filter_idx].eEngine == AX_IVPS_ENGINE_GDC) {
                    pGrp->tPipelineAttr.tFilter[pipe_idx][filter_idx].nDstPicWidth = ALIGN_UP(pGrp->tPipelineAttr.tFilter[pipe_idx][filter_idx].nDstPicWidth, 32);
                    pGrp->tPipelineAttr.tFilter[pipe_idx][filter_idx].nDstPicStride = ALIGN_UP(pGrp->tPipelineAttr.tFilter[pipe_idx][filter_idx].nDstPicWidth, 128);
                } else {
                    pGrp->tPipelineAttr.tFilter[pipe_idx][filter_idx].nDstPicStride = ALIGN_UP(pGrp->tPipelineAttr.tFilter[pipe_idx][filter_idx].nDstPicWidth, 16);
                }

                AX_IVPS_GetChnAttr(pGrp->nIvpsGrp, pipe_idx - 1, filter_idx, &ChnAttr);
                ChnAttr.nDstPicHeight = pGrp->tPipelineAttr.tFilter[pipe_idx][filter_idx].nDstPicHeight;
                ChnAttr.nDstPicWidth = pGrp->tPipelineAttr.tFilter[pipe_idx][filter_idx].nDstPicWidth;
                ChnAttr.nDstPicStride = pGrp->tPipelineAttr.tFilter[pipe_idx][filter_idx].nDstPicStride;
                AX_IVPS_SetChnAttr(pGrp->nIvpsGrp, pipe_idx - 1, filter_idx, &ChnAttr);
	    }

        }
    }

    return IVPS_SUCC;
}

static AX_S32 IVPS_FilterAttrChange(AX_S32 nIvpsGrp, AX_S32 nIvpsChn, SAMPLE_IVPS_GRP_T *pGrp)
{
    int ret;

    IVPS_FilterAttrParaUpdate(pGrp);
    sleep(1); /* usleep(100000); 100ms */
    return IVPS_SUCC;
}

/*
 * IVPS_FilterAttrChangeThread()
 * Dynamic switch channel test.
 */
static AX_VOID *IVPS_FilterAttrChangeThread(AX_VOID *pArg)
{
    SAMPLE_IVPS_GRP_T *pGrp = (SAMPLE_IVPS_GRP_T *)pArg;
    AX_S32 nRepeatNum = gSampleChange.nRepeatNum;

    printf("AttrChange nRepeatNum:%d\n", nRepeatNum);

    sleep(1);
    AX_IVPS_GetPipelineAttr(pGrp->nIvpsGrp, &pGrp->tPipelineAttr);

    while (!ThreadLoopStateGet() && (nRepeatNum == -1 || nRepeatNum-- > 0))
    {

        IVPS_FilterAttrChange(pGrp->nIvpsGrp, 0, pGrp);
    }

    return (AX_VOID *)0;
}

/*
 * IVPS_FilterAttrThreadStart()
 * This function will start a thread and change the IVPS channel attributes frequently.
 */
AX_S32 IVPS_FilterAttrThreadStart(SAMPLE_IVPS_GRP_T *pGrp)
{

    if (0 != pthread_create(&pGrp->change_tid, NULL, IVPS_FilterAttrChangeThread, (AX_VOID *)pGrp))
    {
        return -1;
    }
    return 0;
}

AX_S32 IVPS_FilterAttrThreadStop(SAMPLE_IVPS_GRP_T *pGrp)
{
    if (pGrp->change_tid)
    {
        pthread_join(pGrp->change_tid, NULL);
    }
    return 0;
}
