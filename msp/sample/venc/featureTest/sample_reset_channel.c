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
#include "sample_unit_test.h"

static SAMPLE_VENC_SENDFRAME_PARA_T gstFrmParam[MAX_VENC_CHN_NUM];
static SAMPLE_VENC_GETSTREAM_PARA_T gstStrmParam[MAX_VENC_CHN_NUM];
static AX_S32 gCaseFail = 0;

static AX_BOOL gLoopExit = AX_FALSE;
static void SigHandler(AX_S32 signo)
{
    SAMPLE_LOG("catch signal(%d).\n", signo);
    gLoopExit = AX_TRUE;
}

static AX_VOID *SAMPLE_VENC_SendFramec_ResetChn_Proc(AX_VOID *arg)
{
    AX_BOOL bLoopEncode;
    AX_U32 encFrmNum;
    const AX_CHAR *fileInput;
    AX_S32 syncType;
    AX_IMG_FORMAT_E eFmt;
    AX_U32 width;
    AX_U32 height;
    AX_U32 strideY, strideU, strideV;
    AX_VIDEO_FRAME_INFO_T stFrame;
    SAMPLE_VENC_SENDFRAME_PARA_T *pstArg;
    AX_S32 readSize;
    AX_S32 s32Ret;
    AX_U64 totalInputFrames = 0;
    pstArg = (SAMPLE_VENC_SENDFRAME_PARA_T *)arg;

    VENC_CHN VeChn = pstArg->VeChn;
    bLoopEncode = pstArg->bLoopEncode;
    encFrmNum = pstArg->encFrmNum;
    syncType = pstArg->syncType;
    eFmt = pstArg->eFmt;
    width = pstArg->width;
    height = pstArg->height;
    strideY = pstArg->strideY;
    strideU = pstArg->strideU;
    strideV = pstArg->strideV;
    fileInput = pstArg->fileInput;
    AX_S32 frameSize = pstArg->frameSize;
    AX_S32 poolId = pstArg->poolId;
    FILE *fFileIn = NULL;
    AX_BLK blkId = AX_INVALID_BLOCKID;
    SAMPLE_VENC_CMD_PARA_T *pCml = (SAMPLE_VENC_CMD_PARA_T *)pstArg->ptrPrivate;

    memset(&stFrame, 0, sizeof(stFrame));

    fFileIn = fopen(fileInput, "rb");
    if (fFileIn == NULL) {
        SAMPLE_LOG_ERR("chn-%d: Open input file error!\n", VeChn);
        gCaseFail = 1;
        return NULL;
    }

    while (pstArg->bSendFrmStart) {
        /* get block from user pool */
        blkId = AX_POOL_GetBlock(poolId, frameSize, NULL);
        if (AX_INVALID_BLOCKID == blkId) {
            usleep(5000);
            continue;
        }

        stFrame.stVFrame.u32FrameSize = frameSize;
        stFrame.stVFrame.u64PhyAddr[0] = AX_POOL_Handle2PhysAddr(blkId);
        stFrame.stVFrame.u64VirAddr[0] = (AX_U64)AX_POOL_GetBlockVirAddr(blkId);
        stFrame.stVFrame.u32BlkId[0] = blkId;
        stFrame.stVFrame.u32BlkId[1] = 0;
        stFrame.stVFrame.u32BlkId[2] = 0;

        /* read frame data from yuv file */
        readSize = COMMON_VENC_ReadFile(fFileIn, width, strideY, height, eFmt, (void *)stFrame.stVFrame.u64VirAddr[0]);

        if (!bLoopEncode && (readSize <= 0))
            SAMPLE_LOG_WARN("chn-%d: Warning: read frame size : %d less than %d\n", VeChn, readSize, frameSize);

        if (feof(fFileIn)) {
            if (bLoopEncode) {
                fseek(fFileIn, 0, SEEK_SET);
                COMMON_VENC_ReadFile(fFileIn, pstArg->width, pstArg->strideY, pstArg->height, eFmt,
                                     (void *)stFrame.stVFrame.u64VirAddr[0]);
            } else {
                SAMPLE_LOG_WARN("chn-%d: End of input file!\n", VeChn);
                /* no more frames, stop encoder */
                goto EXIT;
            }
        }

        stFrame.stVFrame.u64SeqNum = totalInputFrames + 1;
        stFrame.stVFrame.enImgFormat = eFmt;
        stFrame.stVFrame.u32Width = width;
        stFrame.stVFrame.u32Height = height;
        stFrame.stVFrame.u32PicStride[0] = strideY;
        stFrame.stVFrame.u32PicStride[1] = strideU;
        stFrame.stVFrame.u32PicStride[2] = strideV;

        if ((totalInputFrames + 1) == pstArg->dynAttrIdx) {
            SAMPLE_LOG("chn-%d: totalInputFrames %llu dynAttrIdx %d \n", VeChn, totalInputFrames, pstArg->dynAttrIdx);
            if (pstArg->function) {
                s32Ret = pstArg->function(VeChn, (AX_VOID *)pstArg);
                if (AX_SUCCESS != s32Ret) {
                    SAMPLE_LOG_ERR("chn-%d: pfn failed, ret=%x\n", VeChn, s32Ret);
                    goto EXIT;
                }
            }
        }

        usleep(random() % 2000);
    RE_SEND:
        s32Ret = AX_VENC_SendFrame(VeChn, &stFrame, syncType);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_LOG_WARN("chn-%d: AX_VENC_SendFrame failed, ret=%x\n", VeChn, s32Ret);
            if (AX_ERR_VENC_QUEUE_FULL == s32Ret || AX_ERR_VENC_NOT_PERMIT == s32Ret) {
                SAMPLE_LOG_WARN("chn-%d: push to input fifo err, retry...\n", VeChn);
                if (pstArg->bSendFrmStart)
                    goto RE_SEND;
            }
        }

        totalInputFrames++;

        if (totalInputFrames == encFrmNum) {
            SAMPLE_LOG("chn-%d: Want to encode %llu frames, exit!\n", VeChn, totalInputFrames);
            goto EXIT;
        }

        if (AX_INVALID_BLOCKID != blkId)
            AX_POOL_ReleaseBlock(blkId);
    }


EXIT:

    if (NULL != fFileIn) {
        fclose(fFileIn);
        fFileIn = NULL;
    }

    if (AX_INVALID_BLOCKID != blkId)
        AX_POOL_ReleaseBlock(blkId);

    SAMPLE_LOG("chn-%d - Total input %llu frames, Encoder exit!\n", VeChn, totalInputFrames);

    gCaseFail = 0;
    COMMON_VENC_AdjustLoopExit(&pCml->vencLoopExit, VeChn);

    return NULL;
}

static AX_S32 SAMPLE_VENC_StartSendFrame_ResetChn(SAMPLE_VENC_SENDFRAME_PARA_T *pstArg)
{
    AX_S32 s32Ret;

    s32Ret = pthread_create(&pstArg->sendFrmPid, 0, SAMPLE_VENC_SendFramec_ResetChn_Proc, (AX_VOID *)pstArg);

    return s32Ret;
}

static AX_VOID *SAMPLE_VENC_GetStream_ResetChn_Proc(AX_VOID *arg)
{
    AX_S32 s32Ret = -1;
    AX_VENC_STREAM_T stStream;
    FILE *pStrm = NULL;
    SAMPLE_VENC_GETSTREAM_PARA_T *pstArg = (SAMPLE_VENC_GETSTREAM_PARA_T *)arg;
    VENC_CHN VeChn = pstArg->VeChn;
    AX_S32 testId = pstArg->testId;
    AX_CHAR esName[50];
    AX_PAYLOAD_TYPE_E enType = pstArg->enType;
    SAMPLE_VENC_CMD_PARA_T *pCml = (SAMPLE_VENC_CMD_PARA_T *)pstArg->ptrPrivate;

    memset(&stStream, 0, sizeof(stStream));
    memset(esName, 0, 50);

    if (PT_H264 == enType)
        sprintf(esName, "es_chn%d_ut%d_%s.264", VeChn, testId, pstArg->strmSuffix);
    else if (PT_H265 == enType)
        sprintf(esName, "es_chn%d_ut%d_%s.265", VeChn, testId, pstArg->strmSuffix);
    else if (PT_MJPEG == enType)
        sprintf(esName, "es_chn%d_ut%d_%s.mjpg", VeChn, testId, pstArg->strmSuffix);

    if (PT_JPEG != enType) {
        pStrm = fopen(esName, "wb");
        if (NULL == pStrm) {
            SAMPLE_LOG_ERR("chn-%d: Open output file error!\n", VeChn);
            return NULL;
        }
    }

    while (pstArg->bGetStrmStart) {
        s32Ret = AX_VENC_GetStream(VeChn, &stStream, pstArg->syncType);
        if (AX_SUCCESS == s32Ret) {
            s32Ret = COMMON_VENC_WriteStream(pstArg, AX_FALSE, pstArg->totalGetStream, pStrm, &stStream);
            if (AX_SUCCESS != s32Ret)
                SAMPLE_LOG_ERR("COMMON_VENC_WriteStream err.\n");

            pstArg->totalGetStream++;

            SAMPLE_LOG_DEBUG("chn-%d: get stream success, addr=%p, len=%u, codecType=%d, frameType=%d.\n", VeChn,
                             stStream.stPack.pu8Addr, stStream.stPack.u32Len, stStream.stPack.enType,
                             stStream.stPack.enCodingType);

            SAMPLE_VENC_PrintNaluInfo(VeChn, &stStream);

            s32Ret = AX_VENC_ReleaseStream(VeChn, &stStream);
            if (AX_SUCCESS != s32Ret) {
                SAMPLE_LOG_ERR("chn-%d: AX_VENC_ReleaseStream failed, ret=0x%x\n", VeChn, s32Ret);
                goto EXIT;
            }

            if (pstArg->totalGetStream == (3 * pCml->dynAttrIdx)) {
                sleep(1);
                if (pstArg->function) {
                    s32Ret = pstArg->function(VeChn, (AX_VOID *)pstArg);
                    if (AX_SUCCESS != s32Ret) {
                        SAMPLE_LOG_ERR("chn-%d: pfn failed, ret=%x\n", VeChn, s32Ret);
                        goto EXIT;
                    }
                }
            }
        }

        usleep(random() % 1000);
    }

EXIT:
    if (pStrm != NULL) {
        fclose(pStrm);
        pStrm = NULL;
    }

    SAMPLE_LOG("chn-%d: Total get %llu encoded frames. getStream Exit!\n", VeChn, pstArg->totalGetStream);

    return (void *)(intptr_t)s32Ret;
}

static AX_S32 SAMPLE_VENC_StartGetStream_ResetChn(SAMPLE_VENC_GETSTREAM_PARA_T *pstArg)
{
    AX_S32 s32Ret;

    s32Ret = pthread_create(&pstArg->getStrmPid, 0, SAMPLE_VENC_GetStream_ResetChn_Proc, (AX_VOID *)pstArg);

    return s32Ret;
}

AX_S32 UTestResetChn(SAMPLE_VENC_CMD_PARA_T *pCml)
{
    AX_S32 s32Ret = 0;
    AX_U32 chnNum;
    AX_S32 chnIdx;
    chnNum = pCml->chnNum;
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

        SampleSendFrameInit(chnIdx, enType, &gstFrmParam[chnIdx], pCml);

        s32Ret = SAMPLE_VENC_StartSendFrame_ResetChn(&gstFrmParam[chnIdx]);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_LOG_ERR("chn-%d: COMMON_VENC_StartSendFrame err.\n", chnIdx);
            s32Ret = -1;
            goto FREE;
        }

        sleep(1);

        SampleGetStreamInit(chnIdx, enType, &gstStrmParam[chnIdx], pCml);

        s32Ret = SAMPLE_VENC_StartGetStream_ResetChn(&gstStrmParam[chnIdx]);
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
        COMMON_VENC_Stop(chnIdx);
        COMMON_VENC_StopGetStream(&gstStrmParam[chnIdx]);
    }

    pCml->vencLoopExit = 0;

    return s32Ret | gCaseFail;
}
