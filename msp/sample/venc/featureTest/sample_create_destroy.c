/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/


#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ax_sys_api.h"
#include "ax_venc_comm.h"
#include "common_venc.h"
#include "sample_cmd_params.h"
#include "sample_global.h"
#include "sample_unit_test.h"
#include "sample_venc_log.h"

static AX_BOOL gLoopExit = AX_FALSE;
static pthread_t gPid[MAX_VENC_CHN_NUM];

static SAMPLE_VENC_SENDFRAME_PARA_T gstFrmParam[MAX_VENC_CHN_NUM];
static SAMPLE_VENC_GETSTREAM_PARA_T gstStrmParam[MAX_VENC_CHN_NUM];

#define SAMPLE_VENC_MAX_WIDTH  (2560)
#define SAMPLE_VENC_MIN_WIDTH  (32)
#define SAMPLE_VENC_MAX_HEIGHT (1920)
#define SAMPLE_VENC_MIN_HEIGHT (32)

#define SAMPLE_ENCODE_NUM (30)

#define SAMPLE_GET_STREAM_TIMEOUT_MS     (20 * 1000)
#define SAMPLE_NO_STREAM_TOLERANCE_TIMES (3)

typedef struct axSAMPLE_SEND_GET_PARAMS_T
{
    VENC_CHN VeChn;
    SAMPLE_VENC_CMD_PARA_T *pstArg;
} SAMPLE_SEND_GET_PARAMS_T;

static SAMPLE_SEND_GET_PARAMS_T gstSendGetParams[MAX_VENC_CHN_NUM];

static void SigHandler(AX_S32 signo)
{
    SAMPLE_LOG("catch signal(%d).\n", signo);
    gLoopExit = AX_TRUE;
}

static AX_S32 SAMPLE_VENC_CreateDestroy_Start(VENC_CHN VeChn, AX_PAYLOAD_TYPE_E enType, SAMPLE_VENC_RC_E rcMode,
                                              SAMPLE_VENC_CMD_PARA_T *pstArg)
{
    AX_S32 s32Ret;

    s32Ret = COMMON_VENC_Create(VeChn, enType, rcMode, pstArg);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: COMM_VENC_Creat faild with%#x!.\n", VeChn, s32Ret);
        return -1;
    }

    return AX_SUCCESS;
}

static AX_S32 SAMPLE_VENC_CreateDestroy_Stop(VENC_CHN VeChn)
{
    AX_S32 s32Ret;

    s32Ret = AX_VENC_DestroyChn(VeChn);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_DestroyChn failed with%#x! \n", VeChn, s32Ret);
        return -1;
    }

    return AX_SUCCESS;
}

AX_S32 SAMPLE_VENC_SendFrame(AX_VOID *arg)
{
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

    pstArg = (SAMPLE_VENC_SENDFRAME_PARA_T *)arg;

    VENC_CHN VeChn = pstArg->VeChn;
    syncType = pstArg->syncType;
    eFmt = pstArg->eFmt;
    width = pstArg->width;
    height = pstArg->height;
    strideY = pstArg->strideY;
    strideU = pstArg->strideU;
    strideV = pstArg->strideV;
    fileInput = pstArg->fileInput;
    AX_S32 frameSize = pstArg->frameSize;
    AX_S32 blkSize = pstArg->blkSize;
    AX_S32 poolId = pstArg->poolId;
    FILE *fFileIn = NULL;
    AX_BLK blkId = AX_INVALID_BLOCKID;

    memset(&stFrame, 0, sizeof(stFrame));

    fFileIn = fopen(fileInput, "rb");
    if (fFileIn == NULL) {
        SAMPLE_LOG_ERR("chn-%d: Open input file error!\n", VeChn);
        return -1;
    }

    /* get block from user pool */
    blkId = AX_POOL_GetBlock(poolId, blkSize, NULL);
    if (AX_INVALID_BLOCKID == blkId) {
        usleep(5000);
    }

    stFrame.stVFrame.u32FrameSize = frameSize;
    stFrame.stVFrame.u64PhyAddr[0] = AX_POOL_Handle2PhysAddr(blkId);
    stFrame.stVFrame.u64VirAddr[0] = (AX_U64)AX_POOL_GetBlockVirAddr(blkId);
    stFrame.stVFrame.u32BlkId[0] = blkId;
    stFrame.stVFrame.u32BlkId[1] = 0;
    stFrame.stVFrame.u32BlkId[2] = 0;

    /* read frame data from yuv file */
    readSize = COMMON_VENC_ReadFile(fFileIn, width, strideY, height, eFmt, (void *)stFrame.stVFrame.u64VirAddr[0]);
    if (readSize <= 0)
        SAMPLE_LOG_ERR("chn-%d: Warning: read frame size : %d less than %d\n", VeChn, readSize, frameSize);

    stFrame.stVFrame.u64SeqNum = pstArg->totalSendFrame + 1;
    stFrame.stVFrame.enImgFormat = eFmt;
    stFrame.stVFrame.u32Width = width;
    stFrame.stVFrame.u32Height = height;
    stFrame.stVFrame.u32PicStride[0] = strideY;
    stFrame.stVFrame.u32PicStride[1] = strideU;
    stFrame.stVFrame.u32PicStride[2] = strideV;

    s32Ret = AX_VENC_SendFrame(VeChn, &stFrame, syncType);
    if (AX_SUCCESS != s32Ret)
        SAMPLE_LOG_ERR("chn-%d: AX_VENC_SendFrame failed, ret=%x\n", VeChn, s32Ret);

    pstArg->totalSendFrame++;

    if (NULL != fFileIn) {
        fclose(fFileIn);
        fFileIn = NULL;
    }

    if (AX_INVALID_BLOCKID != blkId)
        AX_POOL_ReleaseBlock(blkId);

    return 0;
}

AX_S32 SAMPLE_VENC_GetRlsStream(AX_VOID *arg)
{
    AX_S32 s32Ret = 0;
    static AX_S32 nostreamMax = 0;
    AX_VENC_STREAM_T stStream;
    SAMPLE_VENC_GETSTREAM_PARA_T *pstArg = (SAMPLE_VENC_GETSTREAM_PARA_T *)arg;
    VENC_CHN VeChn = pstArg->VeChn;

    memset(&stStream, 0, sizeof(stStream));

    s32Ret = AX_VENC_GetStream(VeChn, &stStream, SAMPLE_GET_STREAM_TIMEOUT_MS);
    if (AX_SUCCESS == s32Ret) {
        pstArg->totalGetStream++;
        s32Ret = AX_VENC_ReleaseStream(VeChn, &stStream);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_LOG_ERR("chn-%d: AX_VENC_ReleaseStream failed!\n", VeChn);
            return -1;
        }
    } else {
        SAMPLE_LOG("chn-%d: Create Destroy no stream, more than %d(ms) !!!\n", VeChn, SAMPLE_GET_STREAM_TIMEOUT_MS);
        nostreamMax++;
        if (nostreamMax > SAMPLE_NO_STREAM_TOLERANCE_TIMES) {
            SAMPLE_LOG_ERR("chn-%d: Create Destroy no stream, more than (%d) times!!!\n", VeChn,
                           SAMPLE_NO_STREAM_TOLERANCE_TIMES);
            exit(1);
        }
    }

    return 0;
}

AX_VOID *SAMPLE_VENC_SendGetProc(AX_VOID *arg)
{
    AX_S32 s32Ret = -1;
    SAMPLE_SEND_GET_PARAMS_T *pSendGetParams;
    AX_S32 s32EncWidth, s32EncHeight;
    AX_S32 chnNum = 0;
    AX_VENC_RECV_PIC_PARAM_T stRecvParam;
    SAMPLE_VENC_CMD_PARA_T stCml;
    SAMPLE_VENC_CMD_PARA_T *pCml;
    struct timespec beginTime = {0};
    struct timespec endTime = {0};

    pSendGetParams = (SAMPLE_SEND_GET_PARAMS_T *)arg;
    chnNum = pSendGetParams->VeChn;
    pCml = pSendGetParams->pstArg;

    memcpy(&stCml, pCml, sizeof(SAMPLE_VENC_CMD_PARA_T));

    SAMPLE_VENC_SetThreadName("SendGetProc-%d", chnNum);

    clock_gettime(CLOCK_MONOTONIC, &beginTime);

    while (!gLoopExit) {
        s32EncWidth = ((rand() % 2) + SAMPLE_VENC_MIN_WIDTH);
        if (0 != (s32EncWidth & 1))
            s32EncWidth++;
        s32EncHeight = ((rand() % 2) + SAMPLE_VENC_MIN_HEIGHT);
        if (0 != (s32EncHeight & 1))
            s32EncHeight++;

        s32Ret = SAMPLE_VENC_CreateDestroy_Stop(chnNum);
        if (AX_SUCCESS != s32Ret)
            SAMPLE_LOG_ERR("chn-%d: SAMPLE_VENC_CreateDestroy_Stop err!\n", chnNum);

        stCml.picW = s32EncWidth;
        stCml.picH = s32EncHeight;
        stCml.strideY = s32EncWidth;
        stCml.strideU = s32EncWidth;

        SAMPLE_LOG_DEBUG("chn-%d: encW=%d, encH=%d.\n", chnNum, s32EncWidth, s32EncHeight);

        s32Ret = SAMPLE_VENC_CreateDestroy_Start(chnNum, PT_MJPEG, SAMPLE_RC_FIXQP, &stCml);
        if (AX_SUCCESS != s32Ret)
            SAMPLE_LOG_ERR("chn-%d: SAMPLE_VENC_CreateDestroy_Start err!\n", chnNum);

        for (AX_S32 encNum = 0; encNum < SAMPLE_ENCODE_NUM; encNum++) {
            stRecvParam.s32RecvPicNum = -1;
            s32Ret = AX_VENC_StartRecvFrame(chnNum, &stRecvParam);
            if (AX_SUCCESS != s32Ret) {
                SAMPLE_LOG_ERR("chn-%d: AX_VENC_StartRecvFrame failed with%#x! \n", chnNum, s32Ret);
                goto exit;
            }

            SampleSendFrameInit(chnNum, PT_MJPEG, &gstFrmParam[chnNum], &stCml);
            s32Ret = SAMPLE_VENC_SendFrame(&gstFrmParam[chnNum]);
            if (AX_SUCCESS != s32Ret) {
                SAMPLE_LOG_ERR("chn-%d: SAMPLE_VENC_SendFrame err!\n", chnNum);
            }

            SampleGetStreamInit(chnNum, PT_MJPEG, &gstStrmParam[chnNum], &stCml);
            s32Ret = SAMPLE_VENC_GetRlsStream(&gstStrmParam[chnNum]);
            if (AX_SUCCESS != s32Ret) {
                SAMPLE_LOG_ERR("chn-%d: SAMPLE_VENC_GetRlsStream err.\n", chnNum);
            }

            s32Ret = AX_VENC_StopRecvFrame(chnNum);
            if (AX_SUCCESS != s32Ret) {
                SAMPLE_LOG_ERR("chn-%d: AX_VENC_StopRecvFrame failed with%#x! \n", chnNum, s32Ret);
                goto exit;
            }
        }

        if (pCml->encFrameNum) {
            clock_gettime(CLOCK_MONOTONIC, &endTime);
            if ((endTime.tv_sec - beginTime.tv_sec) > pCml->encFrameNum) {
                SAMPLE_LOG("chn-%d: encode (%llu) seconds, begin exit!\n", chnNum, endTime.tv_sec - beginTime.tv_sec);
                goto exit;
            }
        }
    }

exit:

    COMMON_VENC_AdjustLoopExit(&pCml->vencLoopExit, chnNum);
    SAMPLE_LOG("chn-%d: Total send(%llu), get(%llu), Encoder exit!\n", chnNum, gstFrmParam[chnNum].totalSendFrame,
               gstStrmParam[chnNum].totalGetStream);

    return 0;
}

AX_S32 UTestCreateDestroy(SAMPLE_VENC_CMD_PARA_T *pCml)
{
    AX_S32 s32Ret = 0;
    AX_U32 chnNum;
    AX_S32 chnIdx;
    chnNum = pCml->chnNum;
    AX_PAYLOAD_TYPE_E enType;
    SAMPLE_VENC_RC_E enRcMode = pCml->rcMode;
    AX_U64 gVencCaseLoopExit = 0;

    signal(SIGINT, SigHandler);

    if (pCml->qpMapQpType || pCml->qpMapBlkType) {
        SAMPLE_LOG_ERR(" This UT not support QpMap!\n");
        return -1;
    }

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

        s32Ret = SAMPLE_VENC_CreateDestroy_Start(chnIdx, enType, enRcMode, pCml);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_LOG_ERR("chn-%d: SAMPLE_VENC_CreateDestroy_Start failed.\n", chnIdx);
            s32Ret = -1;
            goto FREE;
        }

        sleep(1);

        /* create thread to startRecvFrame->sendFrame->getStream */
        gstSendGetParams[chnIdx].VeChn = chnIdx;
        gstSendGetParams[chnIdx].pstArg = pCml;
        s32Ret = pthread_create(&gPid[chnIdx], 0, SAMPLE_VENC_SendGetProc, (AX_VOID *)&gstSendGetParams[chnIdx]);
        if (0 != s32Ret)
            SAMPLE_LOG("create thread %d err!\n", chnIdx);

        sleep(1);

        COMMON_VENC_AdjustLoopExit(&gVencCaseLoopExit, chnIdx);
    }

    while (!gLoopExit && (gVencCaseLoopExit != pCml->vencLoopExit))
        sleep(2);

FREE:

    for (chnIdx = 0; chnIdx < chnNum; chnIdx++) {
        AX_VENC_StopRecvFrame(chnIdx);
        SAMPLE_VENC_CreateDestroy_Stop(chnIdx);
    }

    gLoopExit = AX_FALSE;

    return s32Ret;
}