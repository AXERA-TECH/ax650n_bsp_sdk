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
extern AX_RTSP_HANDLE g_pRtspHandle;

static SAMPLE_VENC_GETSTREAM_PARA_T gstStrmParam[MAX_VENC_CHN_NUM];
AX_VENC_MOD_ATTR_T stModAttr = {
    .enVencType = AX_VENC_MULTI_ENCODER,
    .stModThdAttr.u32TotalThreadNum = 1,
    .stModThdAttr.bExplicitSched = AX_FALSE,
};

SAMPLE_DELTA_PTS_INFO_T gSampleDeltaPts[64];

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

    pstPara->gopLen = 30;
    pstPara->virILen = pstPara->gopLen / 2;
    pstPara->bitRate = 2000;
    pstPara->qpMin = 22;
    pstPara->qpMax = 51;
    pstPara->qpMinI = 10;
    pstPara->qpMaxI = 51;
    pstPara->bLinkMode = AX_TRUE;
    pstPara->maxIprop = 60;
    pstPara->minIprop = 10;
    pstPara->inFifoDep = 4;
    pstPara->outFifoDep = 4;
}

static AX_VOID SAMPLE_GetStreamInit(VENC_CHN VeChn, AX_PAYLOAD_TYPE_E enType, SAMPLE_VENC_GETSTREAM_PARA_T *pstArg)
{
    pstArg->bGetStrmStart = AX_TRUE;
    pstArg->enType = enType;
    pstArg->syncType = 100; /* 100ms */
    pstArg->VeChn = VeChn;
}

static AX_S32 SAMPLE_VencStreamSave(SAMPLE_VENC_GETSTREAM_PARA_T *pstArg, AX_U64 totalStream, FILE *pFile,
                                    AX_VENC_STREAM_T *pstStream)
{
    if ((NULL == pstArg) || (NULL == pstStream)) {
        ALOGE("pstStream or pstArg is NULL!");
        return -1;
    }
    if (NULL == pFile) {
        ALOGE("chn-%d: pFile is NULL!", pstArg->VeChn);
        return -2;
    }
    printf("Venc Stream Save, SeqNum:%lld gopMode:%d temporalID:%d\n", pstStream->stPack.u64SeqNum, pstArg->gopMode,
           pstArg->temporalID);
    if (pstArg->gopMode != AX_VENC_GOPMODE_SVC_T) {
        fwrite(pstStream->stPack.pu8Addr, 1, pstStream->stPack.u32Len, pFile);
    } else {
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
extern SAMPLE_PARAM_T gSampleParam;

static AX_VOID SAMPLE_DeltaPtsStatistic(AX_S32 Chn, AX_VENC_STREAM_T *pstStream)
{
    if (gSampleParam.statDeltaPtsFrmNum < SAMPLE_MIN_DELTAPTS_NUM)
        return;

    static bool bPrint = AX_TRUE;

    SAMPLE_DELTA_PTS_INFO_T *pstDeltaPts = &gSampleDeltaPts[Chn];
    AX_U64 currPts = 0, inPts = 0, deltaPts = 0, seqNum = 0, avgDeltaPts = 0;
    AX_SYS_GetCurPTS(&currPts);

    inPts = pstStream->stPack.u64PTS;
    seqNum = pstStream->stPack.u64SeqNum;
    deltaPts = currPts - inPts;

    if (pstDeltaPts->totalGetStrmNum >= gSampleParam.statDeltaPtsFrmNum) {
        if (!bPrint)
            return;

        avgDeltaPts = pstDeltaPts->totalDeltaPts - pstDeltaPts->maxDeltaPts - pstDeltaPts->minDeltaPts;
        ALOGW("chn-%d: totalNum=%llu, minDeltaPts=%llu, maxDeltaPts=%llu, avgDeltaPts=%llu(us).\n", Chn,
              pstDeltaPts->totalGetStrmNum, pstDeltaPts->minDeltaPts, pstDeltaPts->maxDeltaPts,
              avgDeltaPts / (pstDeltaPts->totalGetStrmNum - 2)); /* not include min,max delta pts */

        bPrint = AX_FALSE;

        return;
    }

    if (inPts > 0 && deltaPts > 0) {
        if (deltaPts > pstDeltaPts->maxDeltaPts)
            pstDeltaPts->maxDeltaPts = deltaPts;

        if (0 == pstDeltaPts->totalGetStrmNum)
            pstDeltaPts->minDeltaPts = deltaPts;

        if (deltaPts < pstDeltaPts->minDeltaPts)
            pstDeltaPts->minDeltaPts = deltaPts;

        pstDeltaPts->totalDeltaPts += deltaPts;
        pstDeltaPts->totalGetStrmNum++;

    } else {
        ALOGE("chn-%d: invalid inPts=%llu, seqNum=%llu, currPts=%llu.\n", Chn, inPts, seqNum, currPts);
        return;
    }

    return;
}

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
    const char *pPath = gSampleGrp.pFilePath;
    AX_CHAR esName[50];

    memset(&stStream, 0, sizeof(stStream));
    memset(esName, 0, 50);

    if (gSampleGrp.nFrameDump == 1) {
        if (pPath == NULL) {
            ALOGE("File save path is NULL!");
            return NULL;
        }

        if (PT_H264 == enType)
            sprintf(esName, "%s/ivps_venc_chn%d_ut%d.264", pPath, VeChn, testId);
        else if (PT_H265 == enType)
            sprintf(esName, "%s/ivps_venc_chn%d_ut%d.265", pPath, VeChn, testId);
        else if (PT_MJPEG == enType)
            sprintf(esName, "%s/ivps_venc_chn%d_ut%d.mjpg", pPath, VeChn, testId);
        else if (PT_JPEG == enType)
            sprintf(esName, "%s/ivps_venc_chn%d_ut%d.jpg", pPath, VeChn, testId);

        pStrm = fopen(esName, "wb");
        if (NULL == pStrm) {
            ALOGE("chn-%d: Open output file error!", VeChn);
            return NULL;
        }
    }
    while (pstArg->bGetStrmStart && !ThreadLoopStateGet()) {
        s32Ret = AX_VENC_GetStream(VeChn, &stStream, -1); /* 100ms */
        if (0 != s32Ret) {
            ALOGE("CHN[%d] AX_VENC_GetStream failed! continue GetStream", VeChn);
            continue;
        }

        SAMPLE_DeltaPtsStatistic(VeChn, &stStream);

        if (gSampleGrp.nFrameDump == 1) {
            s32Ret = SAMPLE_VencStreamSave(pstArg, totalGetStream, pStrm, &stStream);
            if (0 != s32Ret)
                ALOGE("CHN[%d] SAMPLE_VencStreamSave error", VeChn);
        }
        totalGetStream++;

        /* Send to RTSP */
        AX_BOOL bIFrame = (AX_VENC_INTRA_FRAME == stStream.stPack.enCodingType) ? AX_TRUE : AX_FALSE;
        AX_Rtsp_SendNalu(gSampleGrp.pRtspObj, VeChn, stStream.stPack.pu8Addr, stStream.stPack.u32Len,
                             stStream.stPack.u64PTS, bIFrame);

        ALOGI("CHN[%d] Get stream success, addr=%p, len=%u, codeType=%d. seqNum %lld pts %lld\n", VeChn,
              stStream.stPack.pu8Addr, stStream.stPack.u32Len, stStream.stPack.enCodingType, stStream.stPack.u64SeqNum,
              stStream.stPack.u64PTS);

        s32Ret = AX_VENC_ReleaseStream(VeChn, &stStream);
        if (0 != s32Ret) {
            ALOGE("CHN[%d] AX_VENC_ReleaseStream failed!", VeChn);
            goto EXIT;
        }
    }
EXIT:
    if (pStrm != NULL) {
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

static AX_S32 SAMPLE_VencInit(AX_S32 nChnIdx, AX_U16 width, AX_U16 height, AX_MOD_ID_E enModId)
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_PAYLOAD_TYPE_E enType = (AX_ID_VENC == enModId ? PT_H264 : PT_JPEG);
    SAMPLE_VENC_RC_E enRcMode = 0;
    SAMPLE_VENC_CMD_PARA_T stCmdPara;

    memset(&stCmdPara, 0x0, sizeof(SAMPLE_VENC_CMD_PARA_T));
    SAMPLE_SetDefaultVencParams(&stCmdPara, width, height);
    enRcMode = stCmdPara.rcMode;

    s32Ret = COMMON_VENC_Start(nChnIdx, enType, enRcMode, &stCmdPara);
    if (AX_SUCCESS != s32Ret) {
        ALOGE("chn-%d: COMMON_VENC_Start failed", nChnIdx);
        s32Ret = -1;
        goto FREE;
    }

    SAMPLE_GetStreamInit(nChnIdx, enType, &gstStrmParam[nChnIdx]);
    s32Ret = SAMPLE_VencStartGetStream(&gstStrmParam[nChnIdx]);
    if (AX_SUCCESS != s32Ret) {
        ALOGE("chn-%d: COMMON_VENC_StartGetStream err", nChnIdx);
        s32Ret = -1;
        goto FREE;
    }

    return s32Ret;

FREE:
    COMMON_VENC_StopGetStream(&gstStrmParam[nChnIdx]);
    COMMON_VENC_Stop(nChnIdx);

    return s32Ret;
}

static AX_S32 SAMPLE_VencDeinit(AX_S32 nChnIdx)
{
    AX_S32 s32Ret = AX_SUCCESS;

    COMMON_VENC_StopGetStream(&gstStrmParam[nChnIdx]);
    COMMON_VENC_Stop(nChnIdx);

    return s32Ret;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Sample for IVPS -> VENC.
  @param    pGrp       IVPS group information.
  @param    bVencMode     True: the output is video frame. false the output is JPEG.
  @return    0 if Ok, anything else otherwise.
 */
/*--------------------------------------------------------------------------*/
AX_S32 SAMPLE_Ivps2VencInit(SAMPLE_GRP_T *pGrp, AX_BOOL bVencMode)
{
    AX_S32 ret, chn_idx;
    AX_U32 nWidth = 0, nHeight = 0;
    AX_MOD_INFO_T tSrcMod = {0}, tDstMod = {0};
    AX_MOD_ID_E dstModId = AX_ID_BUTT;

    ret = AX_VENC_Init(&stModAttr);
    if (AX_SUCCESS != ret) {
        ALOGE("AX_VENC_Init error");
    }

    for (chn_idx = 0; chn_idx < pGrp->tPipelineAttr.nOutChnNum; chn_idx++) {
        if (pGrp->tPipelineAttr.tFilter[chn_idx + 1][1].bEngage) {
            nWidth = pGrp->tPipelineAttr.tFilter[chn_idx + 1][1].nDstPicWidth;
            nHeight = pGrp->tPipelineAttr.tFilter[chn_idx + 1][1].nDstPicHeight;
        } else if (pGrp->tPipelineAttr.tFilter[chn_idx + 1][0].bEngage) {
            nWidth = pGrp->tPipelineAttr.tFilter[chn_idx + 1][0].nDstPicWidth;
            nHeight = pGrp->tPipelineAttr.tFilter[chn_idx + 1][0].nDstPicHeight;
        } else {
            ALOGE("IVPS module is Bypass!");
            return -1;
        }
        printf("VENC nWidth:%d nHeight:%d\n", nWidth, nHeight);
        dstModId = bVencMode ? AX_ID_VENC : AX_ID_JENC;
        tSrcMod.enModId = AX_ID_IVPS;
        tSrcMod.s32GrpId = pGrp->nIvpsGrp;
        tSrcMod.s32ChnId = chn_idx;

        tDstMod.enModId = dstModId;
        tDstMod.s32GrpId = 0;
        tDstMod.s32ChnId = chn_idx; /* the chn id of VENC is fix to 0 */
        ret = AX_SYS_Link(&tSrcMod, &tDstMod);
        if (0 != ret) {
            ALOGE("AX_SYS_Link err. ret:%d. src: modId %d grpId %d chnId %d. dst: modId %d grpId %d chnId %d", ret,
                tSrcMod.enModId, tSrcMod.s32GrpId, tSrcMod.s32ChnId, tDstMod.enModId, tDstMod.s32GrpId, tDstMod.s32ChnId);
            return ret;
        }

        ret = SAMPLE_VencInit(tDstMod.s32ChnId, nWidth, nHeight, dstModId);
        if (0 != ret) {
            ALOGE("SAMPLE_VencInit failed, ret:0x%x", ret);
            return ret;
        }
    }
    return 0;
}

AX_S32 SAMPLE_Ivps2VencDeinit(SAMPLE_GRP_T *pGrp, AX_BOOL bVencMode)
{
    AX_S32 chn_idx;
    AX_MOD_INFO_T tSrcMod = {0}, tDstMod = {0};
    AX_MOD_ID_E dstModId = AX_ID_BUTT;

    for (chn_idx = 0; chn_idx < pGrp->tPipelineAttr.nOutChnNum; chn_idx++) {
        dstModId = bVencMode ? AX_ID_VENC : AX_ID_JENC;
        tSrcMod.enModId = AX_ID_IVPS;
        tSrcMod.s32GrpId = pGrp->nIvpsGrp;
        tSrcMod.s32ChnId = chn_idx;
        tDstMod.enModId = dstModId;
        tDstMod.s32GrpId = 0;
        tDstMod.s32ChnId = chn_idx; /* the chn id of VENC is fix to 0 */

        SAMPLE_VencDeinit(tDstMod.s32ChnId);
        AX_SYS_UnLink(&tSrcMod, &tDstMod);
    }

    AX_S32 s32Ret = AX_VENC_Deinit();
    if (AX_SUCCESS != s32Ret) {
        ALOGE("AX_VENC_Deinit failed! Error Code:0x%X", s32Ret);
        return -1;
    }
    return 0;
}