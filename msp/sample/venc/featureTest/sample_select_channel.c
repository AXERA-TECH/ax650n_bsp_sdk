/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/


#include "sample_select_channel.h"

#include "sample_global.h"

#define SAMPLE_TEST_GRP_NUM (2)

static SAMPLE_VENC_SENDFRAME_PARA_T gstFrmParam[MAX_VENC_CHN_NUM];
static SAMPLE_VENC_GETSTREAM_PARA_T gstStrmParam[MAX_VENC_CHN_NUM];

#define SAMPLE_STRM_NAME_LEN (20)

static AX_CHAR gSelectStrmName[MAX_VENC_CHN_NUM][SAMPLE_STRM_NAME_LEN];
static FILE *gStrmFile[MAX_VENC_CHN_NUM] = {NULL};

static AX_BOOL gLoopExit = AX_FALSE;
static void SigHandler(AX_S32 signo)
{
    SAMPLE_LOG("catch signal(%d).\n", signo);
    gLoopExit = AX_TRUE;
}

AX_S32 UTestSelectChn(SAMPLE_VENC_CMD_PARA_T *pCml)
{
    AX_S32 s32Ret;
    AX_U32 chnNum;
    AX_S32 chnIdx;
    AX_PAYLOAD_TYPE_E enType;
    SAMPLE_VENC_RC_E enRcMode = pCml->rcMode;
    AX_U64 gVencCaseLoopExit = 0;

    chnNum = (pCml->defaultUt == VENC_TEST_ALL_CASE) ? UT_DEFAULT_ENC_NUM : pCml->chnNum;

    signal(SIGINT, SigHandler);

    if (chnNum < 1)
        return -1;

    for (chnIdx = 0; chnIdx < chnNum; chnIdx++) {
        memset(&gstFrmParam[chnIdx], 0, sizeof(SAMPLE_VENC_SENDFRAME_PARA_T));
        memset(&gstStrmParam[chnIdx], 0, sizeof(SAMPLE_VENC_GETSTREAM_PARA_T));

        if (pCml->bChnCustom)
            enType = SampleGetCodecType(pCml->codecType);
        else
            enType = SampleGetCodecType(chnIdx % SAMPLE_ALL_CODEC_TYPE);

        if (PT_H264 == enType)
            sprintf(gSelectStrmName[chnIdx], "es_chn%d_ut%d.264", chnIdx, pCml->ut);
        else if (PT_H265 == enType)
            sprintf(gSelectStrmName[chnIdx], "es_chn%d_ut%d.265", chnIdx, pCml->ut);
        else if (PT_MJPEG == enType)
            sprintf(gSelectStrmName[chnIdx], "es_chn%d_ut%d.mjpg", chnIdx, pCml->ut);

        if (SampleInvalidEnType(pCml->ut, enType, pCml->rcMode))
            continue;

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

        COMMON_VENC_AdjustLoopExit(&gVencCaseLoopExit, chnIdx);
        sleep(1);
    }

    for (AX_S32 grp = 0; grp < SAMPLE_TEST_GRP_NUM; grp++) {
        SampleSelectGetStreamInit(&gstStrmParam[grp], pCml);

        gstStrmParam[grp].startChn = grp;
        gstStrmParam[grp].grpId = ((pCml->grpId + grp) % MAX_VENC_GRP_NUM);

        s32Ret = SAMPLE_VENC_SelectStartGetStream(&gstStrmParam[grp]);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_LOG_ERR("grp-%d: SAMPLE_VENC_SelectStartGetStream err.\n", gstStrmParam[grp].grpId);
            s32Ret = -1;
            goto FREE;
        }

        if (chnNum < 2) {
            SAMPLE_LOG_DEBUG("only one channel, just create one group!\n");
            break;
        }
    }

    while ((!gLoopExit) && (gVencCaseLoopExit != pCml->vencLoopExit))
        sleep(1);

FREE:

    for (AX_S32 chnIdx = 0; chnIdx < chnNum; chnIdx++) {
        SAMPLE_VENC_StopSendFrame(&gstFrmParam[chnIdx]);
        COMMON_VENC_Stop(chnIdx);
    }

    for (AX_S32 grp = 0; grp < SAMPLE_TEST_GRP_NUM; grp++) {
        SAMPLE_VENC_SelectStopGetStream(&gstStrmParam[grp]);
        if (chnNum < 2)
            break;
    }

    pCml->vencLoopExit = 0;

    return s32Ret;
}

AX_VOID *SAMPLE_VENC_SelectGetStreamProc(AX_VOID *arg)
{
    AX_S32 s32Ret = -1, grpId = 0, startChn = 0;
    AX_PAYLOAD_TYPE_E enType;
    AX_VENC_STREAM_T stStream;
    AX_VENC_SELECT_GRP_PARAM_T *pstGrpParams = NULL;
    AX_CHN_STREAM_STATUS_T *pstChnStrmState = NULL;

    AX_U64 totalGetStream = 0;
    SAMPLE_VENC_GETSTREAM_PARA_T *pstArg = (SAMPLE_VENC_GETSTREAM_PARA_T *)arg;
    VENC_CHN VeChn = pstArg->VeChn;
    AX_S32 testId = pstArg->testId;
    grpId = pstArg->grpId;
    startChn = pstArg->startChn;

    SAMPLE_LOG_DEBUG("grp-%d, startChn-%d.\n", grpId, startChn);

    memset(&stStream, 0, sizeof(stStream));

    pstChnStrmState = (AX_CHN_STREAM_STATUS_T *)calloc(1, sizeof(AX_CHN_STREAM_STATUS_T));
    if (NULL == pstChnStrmState) {
        SAMPLE_LOG_ERR("grp-%d, calloc err!\n", grpId);
        goto EXIT;
    }
    pstGrpParams = (AX_VENC_SELECT_GRP_PARAM_T *)calloc(1, sizeof(AX_VENC_SELECT_GRP_PARAM_T));
    if (NULL == pstGrpParams) {
        SAMPLE_LOG_ERR("grp-%d, calloc err!\n", grpId);
        goto EXIT;
    }

    s32Ret = AX_VENC_SelectClearGrp(grpId);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("grp-%d: AX_VENC_SelectClearGrp err!\n", grpId);
        return NULL;
    }

    for (AX_S32 i = startChn; i < pstArg->chnNum; i += 2) {
        s32Ret = AX_VENC_SelectGrpAddChn(grpId, i);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_LOG_ERR("Grp(%d) Add Chn(%d) err!\n", grpId, i);
            return NULL;
        }
    }

    s32Ret = AX_VENC_SelectGrpQuery(grpId, pstGrpParams);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("grp-%d: AX_VENC_SelectGrpQuery.\n", grpId);
        return NULL;
    }

    for (AX_S32 i = 0; i < pstGrpParams->u16TotalChnNum; i++)
        SAMPLE_LOG_DEBUG("grp-%d own chn-%d.\n", grpId, pstGrpParams->u16ChnInGrp[i]);

    while (pstArg->bGetStrmStart) {
        s32Ret = AX_VENC_SelectGrp(grpId, pstChnStrmState, -1);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_LOG_WARN("grp-%d: AX_VENC_SelectChn err!\n", grpId);
            continue;
        }

        for (AX_S32 idx = 0; idx < pstChnStrmState->u32TotalChnNum; idx++) {
            VeChn = pstChnStrmState->au32ChnIndex[idx];

            enType = pstChnStrmState->aenChnCodecType[VeChn];
            SAMPLE_LOG_DEBUG("grp-%d: total %d chn, chnId=%d, enType=%d.\n", grpId, pstChnStrmState->u32TotalChnNum,
                             VeChn, enType);

            s32Ret = AX_VENC_GetStream(VeChn, &stStream, 0);
            if (AX_SUCCESS == s32Ret) {
                if (PT_JPEG == enType) {
                    s32Ret = COMMON_VENC_SaveJpegFile(VeChn, testId, totalGetStream, stStream.stPack.pu8Addr,
                                                      stStream.stPack.u32Len, pstArg->bSaveStrm);
                    if (AX_SUCCESS != s32Ret)
                        SAMPLE_LOG_ERR("grp-%d: COMMON_VENC_SaveJpegFile err.\n", grpId);
                } else {
                    if (NULL == gStrmFile[VeChn])
                        gStrmFile[VeChn] = fopen(gSelectStrmName[VeChn], "wb");

                    SAMPLE_VENC_FWRITE(stStream.stPack.pu8Addr, 1, stStream.stPack.u32Len, gStrmFile[VeChn],
                                       pstArg->bSaveStrm);
                    fflush(gStrmFile[VeChn]);
                }

                totalGetStream++;

                SAMPLE_LOG_DEBUG("grp%d: chn-%d: get stream success, addr=%p, len=%u, codeType=%d.\n", grpId, VeChn,
                                 stStream.stPack.pu8Addr, stStream.stPack.u32Len, stStream.stPack.enCodingType);

                s32Ret = AX_VENC_ReleaseStream(VeChn, &stStream);
                if (AX_SUCCESS != s32Ret) {
                    SAMPLE_LOG_ERR("grp-%d: chn-%d: AX_VENC_ReleaseStream failed!\n", grpId, VeChn);
                    goto EXIT;
                }
            }
        }
    }

EXIT:
    for (AX_S32 i = 0; i < pstChnStrmState->u32TotalChnNum; i++) {
        if (gStrmFile[i] != NULL) {
            fclose(gStrmFile[i]);
            gStrmFile[i] = NULL;
        }
    }
    if (NULL != pstGrpParams) {
        free(pstGrpParams);
        pstGrpParams = NULL;
    }
    if (NULL != pstChnStrmState) {
        free(pstChnStrmState);
        pstChnStrmState = NULL;
    }

    SAMPLE_LOG("grp-%d: Select: Total get %llu encoded frames. getStream Exit!\n", grpId, totalGetStream);

    return (void *)(intptr_t)s32Ret;
}

AX_S32 SAMPLE_VENC_SelectStartGetStream(SAMPLE_VENC_GETSTREAM_PARA_T *pstArg)
{
    AX_S32 s32Ret = 0;

    s32Ret = pthread_create(&pstArg->getStrmPid, 0, SAMPLE_VENC_SelectGetStreamProc, (AX_VOID *)pstArg);
    if (s32Ret) {
        SAMPLE_LOG_ERR("grp-%d: pthread create err, ret=%d!\n", pstArg->grpId, s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

AX_S32 SAMPLE_VENC_SelectStopGetStream(SAMPLE_VENC_GETSTREAM_PARA_T *pstArg)
{
    if (pstArg->bGetStrmStart) {
        pstArg->bGetStrmStart = AX_FALSE;
        pthread_join(pstArg->getStrmPid, 0);
    }

    return AX_SUCCESS;
}

AX_VOID SampleSelectGetStreamInit(SAMPLE_VENC_GETSTREAM_PARA_T *pstArg, SAMPLE_VENC_CMD_PARA_T *pCml)
{
    pstArg->bGetStrmStart = AX_TRUE;
    /* 100ms */
    pstArg->syncType = 100;
    pstArg->testId = pCml->ut;
    pstArg->gopMode = pCml->gopMode;
    pstArg->temporalID = pCml->temporalID;
    pstArg->bGetStrmBufInfo = pCml->bGetStrmBufInfo;
    pstArg->bQueryStatus = pCml->bQueryStatus;
    pstArg->bSaveStrm = pCml->bSaveStrm;
    pstArg->chnNum = pCml->chnNum;
    pstArg->grpId = pCml->grpId;
    pstArg->startChn = 0;
}