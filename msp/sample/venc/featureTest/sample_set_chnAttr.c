/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "sample_case.h"

static SAMPLE_VENC_SENDFRAME_PARA_T gstFrmParam[MAX_VENC_CHN_NUM];
static SAMPLE_VENC_GETSTREAM_PARA_T gstStrmParam[MAX_VENC_CHN_NUM];
static AX_BOOL gLoopExit = AX_FALSE;

static void SigHandler(AX_S32 signo)
{
    SAMPLE_LOG("catch signal(%d).\n", signo);
    gLoopExit = AX_TRUE;
}

static void SampleChnAttrDebug(AX_VENC_CHN_ATTR_T *pStChnAttr)
{
    if (NULL == pStChnAttr) {
        SAMPLE_LOG_ERR("NULL pointer\n");
        return;
    }

    SAMPLE_LOG_DEBUG("type %d cropX %d cropY %d cropW %d cropH %d \n", pStChnAttr->stVencAttr.enType,
                     pStChnAttr->stVencAttr.stCropCfg.stRect.s32X, pStChnAttr->stVencAttr.stCropCfg.stRect.s32Y,
                     pStChnAttr->stVencAttr.stCropCfg.stRect.u32Width,
                     pStChnAttr->stVencAttr.stCropCfg.stRect.u32Height);

    return;
}

AX_S32 SampleChnAttr(AX_S32 VencChn, SAMPLE_VENC_CMD_PARA_T *pCml)
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_VENC_CHN_ATTR_T stChnAttr;

    memset(&stChnAttr, 0, sizeof(AX_VENC_CHN_ATTR_T));
    s32Ret = AX_VENC_GetChnAttr(VencChn, &stChnAttr);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("AX_VENC_GetChnAttr:%d failed! ret=0x%x\n", VencChn, s32Ret);
        return -1;
    }

    SampleChnAttrDebug(&stChnAttr);

    if (pCml->bCrop) {
        stChnAttr.stVencAttr.stCropCfg.stRect.s32X = pCml->cropH;
        stChnAttr.stVencAttr.stCropCfg.stRect.s32Y = pCml->cropH;
        stChnAttr.stVencAttr.stCropCfg.stRect.u32Width = pCml->cropX;
        stChnAttr.stVencAttr.stCropCfg.stRect.u32Height = pCml->cropY;
    } else {
        SAMPLE_LOG("Disable crop!\n");
        return 0;
    }

    s32Ret = AX_VENC_SetChnAttr(VencChn, &stChnAttr);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("AX_VENC_SetChnAttr:%d failed! ret=0x%x\n", VencChn, s32Ret);
        return -1;
    }

    SampleChnAttrDebug(&stChnAttr);

    return AX_SUCCESS;
}

AX_S32 UTestSetChnAttr(SAMPLE_VENC_CMD_PARA_T *pCml)
{
    AX_S32 s32Ret;
    AX_U32 chnNum = pCml->chnNum;
    AX_S32 chnIdx;
    AX_PAYLOAD_TYPE_E enType;
    SAMPLE_VENC_RC_E enRcMode = pCml->rcMode;
    AX_U64 gVencCaseLoopExit = 0;

    signal(SIGINT, SigHandler);

    for (chnIdx = 0; chnIdx < chnNum; chnIdx++) {
        memset(&gstFrmParam[chnIdx], 0, sizeof(SAMPLE_VENC_SENDFRAME_PARA_T));
        memset(&gstStrmParam[chnIdx], 0, sizeof(SAMPLE_VENC_GETSTREAM_PARA_T));

        if (pCml->bChnCustom)
            enType = SampleGetCodecType(pCml->codecType);
        else
            enType = SampleGetCodecType(chnIdx % SAMPLE_ALL_CODEC_TYPE);

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

        s32Ret = SampleChnAttr(chnIdx, pCml);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_LOG_ERR("chn-%d: SampleJpegParam failed.\n", chnIdx);
            goto FREE;
        }

        SampleSendFrameInit(chnIdx, enType, &gstFrmParam[chnIdx], pCml);

        s32Ret = SAMPLE_VENC_StartSendFrame(&gstFrmParam[chnIdx]);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_LOG_ERR("chn-%d: SAMPLE_VENC_StartSendFrame err.\n", chnIdx);
            s32Ret = -1;
            goto FREE;
        }

        sleep(1);

        SampleGetStreamInit(chnIdx, enType, &gstStrmParam[chnIdx], pCml);

        s32Ret = SAMPLE_VENC_StartGetStream(&gstStrmParam[chnIdx]);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_LOG_ERR("chn-%d: SAMPLE_VENC_StartGetStream err.\n", chnIdx);
            s32Ret = -1;
            goto FREE;
        }

        COMMON_VENC_AdjustLoopExit(&gVencCaseLoopExit, chnIdx);
        sleep(1);
    }

    while ((!gLoopExit) && (gVencCaseLoopExit != pCml->vencLoopExit))
        usleep(100);

FREE:

    for (chnIdx = 0; chnIdx < chnNum; chnIdx++) {
        COMMON_VENC_StopSendFrame(&gstFrmParam[chnIdx]);
        COMMON_VENC_SendGetDelay(chnIdx, gLoopExit, &gstFrmParam[chnIdx], &gstStrmParam[chnIdx]);
        COMMON_VENC_Stop(chnIdx);
        COMMON_VENC_StopGetStream(&gstStrmParam[chnIdx]);
    }

    pCml->vencLoopExit = 0;

    return 0;
}