/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
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
    .stModThdAttr.u32TotalThreadNum = 1,
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

#define DUMP_STREAM_NAME "vo_venc_chn"
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

    if (pPath == NULL) {
        ALOGE("File save path is NULL!");
        return NULL;
    }

    if (PT_H264 == enType)
        sprintf(esName, "%s/" DUMP_STREAM_NAME "%d_ut%d.264", pPath, VeChn, testId);
    else if (PT_H265 == enType)
        sprintf(esName, "%s/" DUMP_STREAM_NAME "%d_ut%d.265", pPath, VeChn, testId);
    else if (PT_MJPEG == enType)
        sprintf(esName, "%s/" DUMP_STREAM_NAME "%d_ut%d.mjpg", pPath, VeChn, testId);
    else if (PT_JPEG == enType)
        sprintf(esName, "%s/" DUMP_STREAM_NAME "%d_ut%d.jpg", pPath, VeChn, testId);

    pStrm = fopen(esName, "wb");
    if (NULL == pStrm) {
        ALOGE("chn-%d: Open output file error!", VeChn);
        return NULL;
    }
    while (pstArg->bGetStrmStart) {
        s32Ret = AX_VENC_GetStream(VeChn, &stStream, 100); /* 100ms */
        if (0 != s32Ret) {
            ALOGE("CHN[%d] AX_VENC_ReleaseStream failed!", VeChn);
            continue;
        }
        s32Ret = SAMPLE_VencStreamSave(pstArg, totalGetStream, pStrm, &stStream);
        if (0 != s32Ret)
            ALOGE("CHN[%d] SAMPLE_VencStreamSave error", VeChn);
        totalGetStream++;

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

    s32Ret = AX_VENC_Init(&stModAttr);
    if (AX_SUCCESS != s32Ret) {
        ALOGE("AX_VENC_Init error");
    }

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
    s32Ret = AX_VENC_Deinit();
    if (AX_SUCCESS != s32Ret) {
        ALOGE("nChnIdx[%d] AX_VENC_Deinit failed! Error Code:0x%X", nChnIdx, s32Ret);
        return -1;
    }

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
AX_S32 SAMPLE_Link2VencInit(AX_MOD_INFO_T *ptSrcMod, AX_U32 u32Width, AX_U32 u32Height, AX_BOOL bVencMode)
{
    AX_S32 ret;
    AX_MOD_INFO_T tDstMod = {0};
    AX_MOD_ID_E dstModId = AX_ID_BUTT;

    printf("VENC u32Width:%d u32Height:%d\n", u32Width, u32Height);

    dstModId = bVencMode ? AX_ID_VENC : AX_ID_JENC;
    tDstMod.enModId = dstModId;
    tDstMod.s32GrpId = 0;
    tDstMod.s32ChnId = 0; /* the chn id of VENC is fix to 0 */
    ret = AX_SYS_Link(ptSrcMod, &tDstMod);
    if (0 != ret) {
        ALOGE("AX_SYS_Link err. ret:%d. src: modId %d grpId %d chnId %d. dst: modId %d grpId %d chnId %d", ret,
              ptSrcMod->enModId, ptSrcMod->s32GrpId, ptSrcMod->s32ChnId, tDstMod.enModId, tDstMod.s32GrpId,
              tDstMod.s32ChnId);
        return ret;
    }

    ret = SAMPLE_VencInit(tDstMod.s32ChnId, u32Width, u32Height, dstModId);
    if (0 != ret) {
        ALOGE("SAMPLE_VencInit failed, ret:0x%x", ret);
        return ret;
    }
    return 0;
}

AX_S32 SAMPLE_Link2VencDeinit(AX_MOD_INFO_T *ptSrcMod, AX_BOOL bVencMode)
{
    AX_MOD_INFO_T tDstMod = {0};
    AX_MOD_ID_E dstModId = AX_ID_BUTT;

    dstModId = bVencMode ? AX_ID_VENC : AX_ID_JENC;
    tDstMod.enModId = dstModId;
    tDstMod.s32GrpId = 0;
    tDstMod.s32ChnId = 0; /* the chn id of VENC is fix to 0 */

    SAMPLE_VencDeinit(tDstMod.s32ChnId);
    AX_SYS_UnLink(ptSrcMod, &tDstMod);
    return 0;
}