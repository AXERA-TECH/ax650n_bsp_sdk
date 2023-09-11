/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include "sample_dynamic_jpeg.h"

#include "sample_case.h"
#include "sample_roicfg.h"

static AX_BOOL gLoopExit = AX_FALSE;
static SAMPLE_VENC_SENDFRAME_PARA_T gstFrmParam[MAX_VENC_CHN_NUM];
static SAMPLE_VENC_GETSTREAM_PARA_T gstStrmParam[MAX_VENC_CHN_NUM];

static void SigHandler(AX_S32 signo)
{
    SAMPLE_LOG("catch signal(%d).\n", signo);
    gLoopExit = AX_TRUE;
}

AX_S32 UTestDynamicJpeg(SAMPLE_VENC_CMD_PARA_T *pCml)
{
    AX_S32 s32Ret;
    AX_U32 chnNum;
    AX_S32 chnIdx;
    chnNum = (pCml->defaultUt == VENC_TEST_ALL_CASE) ? 1 : pCml->chnNum;
    AX_PAYLOAD_TYPE_E enType;
    SAMPLE_VENC_RC_E enRcMode = pCml->rcMode;
    AX_U64 gVencCaseLoopExit = 0;

    signal(SIGINT, SigHandler);

    for (chnIdx = 0; chnIdx < chnNum; chnIdx++) {
        memset(&gstFrmParam[chnIdx], 0, sizeof(SAMPLE_VENC_SENDFRAME_PARA_T));
        memset(&gstStrmParam[chnIdx], 0, sizeof(SAMPLE_VENC_GETSTREAM_PARA_T));

        enType = PT_JPEG;

        if (PT_BUTT == enType) {
            SAMPLE_LOG_ERR("chn-%d: Invalid codec type!\n", chnIdx);
            s32Ret = -1;
            goto FREE;
        }

        s32Ret = COMMON_VENC_Start(chnIdx, enType, enRcMode, pCml);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_LOG_ERR("chn-%d: COMMON_VENC_Start failed.\n", chnIdx);
            s32Ret = -1;
            goto FREE;
        }

        SampleSendFrameInit(chnIdx, enType, &gstFrmParam[chnIdx], pCml);

        s32Ret = SAMPLE_VENC_StartSendFrame(&gstFrmParam[chnIdx]);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_LOG_ERR("chn-%d: COMMON_VENC_StartSendFrame err.\n", chnIdx);
            s32Ret = -1;
            goto FREE;
        }

        sleep(1);

        SampleGetStreamInit(chnIdx, enType, &gstStrmParam[chnIdx], pCml);

        s32Ret = SAMPLE_VENC_StartGetStream(&gstStrmParam[chnIdx]);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_LOG_ERR("chn-%d: COMMON_VENC_StartGetStream err.\n", chnIdx);
            s32Ret = -1;
            goto FREE;
        }

        COMMON_VENC_AdjustLoopExit(&gVencCaseLoopExit, chnIdx);
        sleep(1);
    }

    while ((!gLoopExit) && (gVencCaseLoopExit != pCml->vencLoopExit))
        sleep(1);

FREE:

    for (chnIdx = 0; chnIdx < chnNum; chnIdx++) {
        COMMON_VENC_StopSendFrame(&gstFrmParam[chnIdx]);
        COMMON_VENC_SendGetDelay(chnIdx, gLoopExit, &gstFrmParam[chnIdx], &gstStrmParam[chnIdx]);
        COMMON_VENC_Stop(chnIdx);
        COMMON_VENC_StopGetStream(&gstStrmParam[chnIdx]);
    }

    pCml->vencLoopExit = 0;

    return s32Ret;
}

static void SampleJpegRoiInit(SAMPLE_ROI_CFG_T *pstArg, SAMPLE_VENC_SENDFRAME_PARA_T *pCml)
{
    pstArg->qFactor = pCml->qFactor;
    pstArg->roiEnable = pCml->roiEnable;
    pstArg->roimapFile = pCml->jencRoiMap;
    pstArg->qRoiFactor = pCml->qRoiFactor;
}

static void SampleJpegParamDebug(AX_S32 VencChn, AX_VENC_JPEG_PARAM_T *pStJpegParam)
{
    if (NULL == pStJpegParam) {
        SAMPLE_LOG_ERR("NULL pointer\n");
        return;
    }

    SAMPLE_LOG_DEBUG("VencChn %d u32Qfactor %d\n", VencChn, pStJpegParam->u32Qfactor);
    SAMPLE_LOG_DEBUG("VencChn %d bEnableRoi %d bSaveNonRoiQt %d u32RoiQfactor %d\n", VencChn, pStJpegParam->bEnableRoi,
                     pStJpegParam->bSaveNonRoiQt, pStJpegParam->u32RoiQfactor);

    return;
}

AX_S32 SampleJpegParam(AX_S32 VencChn, AX_VOID *handle)
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_VENC_JPEG_PARAM_T stJpegParam;
    SAMPLE_ROI_CFG_T roiCfg;
    SAMPLE_VENC_SENDFRAME_PARA_T *pCml = (SAMPLE_VENC_SENDFRAME_PARA_T *)handle;

    memset(&stJpegParam, 0, sizeof(stJpegParam));
    if (pCml->enType == PT_H264 || pCml->enType == PT_H265)
        return AX_SUCCESS;

    s32Ret = AX_VENC_GetJpegParam(VencChn, &stJpegParam);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("AX_VENC_GetJpegParam:%d failed!\n", VencChn);
        return -1;
    }

    SampleJpegParamDebug(VencChn, &stJpegParam);

    stJpegParam.u32Qfactor = pCml->qFactor;

    memcpy(stJpegParam.u8YQt, QTableLuminance, sizeof(QTableLuminance));
    memcpy(stJpegParam.u8CbCrQt, QTableChrominance, sizeof(QTableChrominance));

    if (pCml->roiEnable) {
        memset(&roiCfg, 0x0, sizeof(SAMPLE_ROI_CFG_T));
        SampleJpegRoiInit(&roiCfg, pCml);
        s32Ret = SampleJpegRoiCfg(&roiCfg, &stJpegParam);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_LOG_ERR("SampleJpegRoiCfg err.\n");
            return -1;
        }
    }

    s32Ret = AX_VENC_SetJpegParam(VencChn, &stJpegParam);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("AX_VENC_SetJpegParam:%d failed!\n", VencChn);
        return -1;
    }

    SampleJpegParamDebug(VencChn, &stJpegParam);

    return AX_SUCCESS;
}