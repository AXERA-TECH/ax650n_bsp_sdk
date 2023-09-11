/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include "ax_sys_api.h"
#include "ax_venc_api.h"
#include "common_venc.h"
#include "sample_utils.h"

AX_S32 gVencLoopExit;

static SAMPLE_VENC_GETSTREAM_PARA_T gstStrmParam[MAX_VENC_CHN_NUM];

AX_VENC_MOD_ATTR_T stModAttr = {
    .enVencType = AX_VENC_MULTI_ENCODER,
    .stModThdAttr.u32TotalThreadNum = 9,
    .stModThdAttr.bExplicitSched = AX_FALSE,
};

static AX_VOID SAMPLE_SetDefaultVencParams(SAMPLE_VENC_CMD_PARA_T *pstPara, AX_U16 width, AX_U16 height)
{
    pstPara->output = "stream.h264";

    pstPara->picW = width;
    pstPara->picH = height;
    pstPara->maxPicW = width;
    pstPara->maxPicH = height;
    pstPara->rcMode = SAMPLE_RC_CBR;
    pstPara->chnNum = 1;
    pstPara->srcFrameRate = 30;
    pstPara->dstFrameRate = 30;
    pstPara->maxIprop = 10;
    pstPara->minIprop = 10;

    pstPara->gopLen = 30;
    pstPara->virILen = pstPara->gopLen / 2;
    pstPara->bitRate = 2000;
    pstPara->qpMin = 22;
    pstPara->qpMax = 51;
    pstPara->qpMinI = 10;
    pstPara->qpMaxI = 51;
    pstPara->bLinkMode = AX_TRUE;
    pstPara->inFifoDep = 4;
    pstPara->outFifoDep = 4;
}

static AX_VOID SAMPLE_GetStreamInit(VENC_CHN VeChn, AX_PAYLOAD_TYPE_E enType, AX_BOOL bSaveStrm, SAMPLE_VENC_GETSTREAM_PARA_T *pstArg)
{
    pstArg->bGetStrmStart = AX_TRUE;
    pstArg->enType = enType;
    pstArg->syncType = 100; /* 100ms */
    pstArg->VeChn = VeChn;
    pstArg->bSaveStrm = bSaveStrm;
}

static AX_S32 SAMPLE_VencStreamSave(SAMPLE_VENC_GETSTREAM_PARA_T *pstArg, AX_U64 totalStream, FILE *pFile,
                                    AX_VENC_STREAM_T *pstStream)
{
    if ((NULL == pstArg) || (NULL == pstStream))
    {
        ALOGE("pstStream or pstArg is NULL!");
        return -1;
    }
    if (NULL == pFile)
    {
        ALOGE("chn-%d: pFile is NULL!", pstArg->VeChn);
        return -2;
    }
    printf("Venc Stream Save, SeqNum:%lld gopMode:%d temporalID:%d\n", pstStream->stPack.u64SeqNum, pstArg->gopMode, pstArg->temporalID);
    if (pstArg->gopMode != AX_VENC_GOPMODE_SVC_T)
    {
        fwrite(pstStream->stPack.pu8Addr, 1, pstStream->stPack.u32Len, pFile);
    }
    else
    {
        if (((0 == pstArg->temporalID) && (0 == pstStream->stPack.u32TemporalID)) ||
            ((1 == pstArg->temporalID) &&
             ((0 == pstStream->stPack.u32TemporalID) || (1 == pstStream->stPack.u32TemporalID))) ||
            ((2 == pstArg->temporalID) &&
             ((0 == pstStream->stPack.u32TemporalID) || (1 == pstStream->stPack.u32TemporalID) ||
              (2 == pstStream->stPack.u32TemporalID))))
            fwrite(pstStream->stPack.pu8Addr, 1, pstStream->stPack.u32Len, pFile);
    }
    fflush(pFile);
    return 0;
}


extern void *AXRtspServerObj;

static AX_VOID *SAMPLE_VencGetStreamProc(AX_VOID *arg)
{
    AX_S32 s32Ret = -1;
    AX_VENC_STREAM_T stStream;
    FILE *pStrm = NULL;
    AX_U64 totalGetStream = 0;
    SAMPLE_VENC_GETSTREAM_PARA_T *pstArg = (SAMPLE_VENC_GETSTREAM_PARA_T *)arg;
    VENC_CHN VeChn = pstArg->VeChn;
    AX_S32 testId = pstArg->testId;
    AX_PAYLOAD_TYPE_E enType = pstArg->enType;
    AX_CHAR esName[50];

    memset(&stStream, 0, sizeof(stStream));
    memset(esName, 0, 50);

    if (pstArg->bSaveStrm)
    {
        if (PT_H264 == enType)
            sprintf(esName, "ivps_venc_chn%d_ut%d.264", VeChn, testId);
        else if (PT_H265 == enType)
            sprintf(esName, "ivps_venc_chn%d_ut%d.265", VeChn, testId);
        else if (PT_MJPEG == enType)
            sprintf(esName, "ivps_venc_chn%d_ut%d.mjpg", VeChn, testId);
        else if (PT_JPEG == enType)
            sprintf(esName, "ivps_venc_chn%d_ut%d.jpg", VeChn, testId);

        pStrm = fopen(esName, "wb");
        if (NULL == pStrm)
        {
            ALOGE("chn-%d: Open output file error!", VeChn);
            return NULL;
        }
    }

    while (pstArg->bGetStrmStart)
    {
        s32Ret = AX_VENC_GetStream(VeChn, &stStream, 500); /* 100ms */
        if (AX_SUCCESS != s32Ret)
        {
            if (AX_ERR_VENC_QUEUE_EMPTY != s32Ret) {
                ALOGE("CHN[%d] AX_VENC_GetStream failed! s32Ret:0x%x", VeChn, s32Ret);
            }
            continue;
        }

        if (pstArg->bSaveStrm)
        {
            s32Ret = SAMPLE_VencStreamSave(pstArg, totalGetStream, pStrm, &stStream);
            if (0 != s32Ret)
                ALOGE("CHN[%d] SAMPLE_VencStreamSave error", VeChn);
        }

        totalGetStream++;
        gstStrmParam[VeChn].totalGetStream++;

        ALOGI("CHN[%d] Get stream success, addr=%p, len=%u, codeType=%d. seqNum %lld pts %lld\n", VeChn,
              stStream.stPack.pu8Addr, stStream.stPack.u32Len, stStream.stPack.enCodingType,
              stStream.stPack.u64SeqNum, stStream.stPack.u64PTS);

        s32Ret = AX_VENC_ReleaseStream(VeChn, &stStream);
        if (AX_SUCCESS != s32Ret)
        {
            ALOGE("CHN[%d] AX_VENC_ReleaseStream failed!", VeChn);
            goto EXIT;
        }
    }
EXIT:
    if (pStrm != NULL)
    {
        fclose(pStrm);
        pStrm = NULL;
    }

    ALOGI("CHN[%d] Total get %llu encoded frames. getStream Exit!", VeChn, totalGetStream);
    return (void *)(intptr_t)s32Ret;
}

static AX_S32 SAMPLE_VencStartGetStream(SAMPLE_VENC_GETSTREAM_PARA_T *pstArg)
{
    AX_S32 s32Ret;

    s32Ret = pthread_create(&pstArg->getStrmPid, 0, SAMPLE_VencGetStreamProc, (AX_VOID *)pstArg);

    return s32Ret;
}

AX_S32 SAMPLE_VencInit(AX_S32 chnNum, AX_U16 width, AX_U16 height, AX_MOD_ID_E enModId, AX_BOOL bSaveStrm)
{
    int i = 0;
    AX_S32 s32Ret = AX_SUCCESS;
    AX_PAYLOAD_TYPE_E enType = (AX_ID_VENC == enModId ? PT_H264 : PT_JPEG);
    SAMPLE_VENC_RC_E enRcMode = 0;
    SAMPLE_VENC_CMD_PARA_T stCmdPara;

    memset(&stCmdPara, 0x0, sizeof(SAMPLE_VENC_CMD_PARA_T));
    SAMPLE_SetDefaultVencParams(&stCmdPara, width, height);
    enRcMode = stCmdPara.rcMode;

    s32Ret = AX_VENC_Init(&stModAttr);
    if (AX_SUCCESS != s32Ret)
    {
        ALOGE("AX_VENC_Init error");
    }

    for(i = 0; i < chnNum; i++)
    {
        s32Ret = COMMON_VENC_Start(i, enType, enRcMode, &stCmdPara);
        if (AX_SUCCESS != s32Ret)
        {
            ALOGE("chn-%d: COMMON_VENC_Start failed", i);
            s32Ret = -1;
            goto FREE;
        }

        SAMPLE_GetStreamInit(i, enType, bSaveStrm, &gstStrmParam[i]);
        s32Ret = SAMPLE_VencStartGetStream(&gstStrmParam[i]);
        if (AX_SUCCESS != s32Ret)
        {
            ALOGE("chn-%d: COMMON_VENC_StartGetStream err", i);
            s32Ret = -1;
            goto FREE;
        }
    }

    return s32Ret;

FREE:
    for (i = 0; i < chnNum; i++) {
        COMMON_VENC_StopGetStream(&gstStrmParam[i]);
        COMMON_VENC_Stop(i);
    }

    return s32Ret;
}

AX_S32 SAMPLE_VencDeinit(AX_S32 chnNum)
{
    AX_S32 s32Ret = AX_SUCCESS;
    int i = 0;

    for (i = 0; i < chnNum; i++) {
        COMMON_VENC_StopGetStream(&gstStrmParam[i]);
        COMMON_VENC_Stop(i);
        printf("chn-%d: total get %llu streams.\n", i, gstStrmParam[i].totalGetStream);
    }

    s32Ret = AX_VENC_Deinit();
    if (AX_SUCCESS != s32Ret)
    {
        ALOGE("AX_VENC_Deinit failed! Error Code:0x%X", s32Ret);
        return -1;
    }

    return s32Ret;
}