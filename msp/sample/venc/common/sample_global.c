/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ax_sys_api.h"
#include "ax_venc_api.h"
#include "ax_venc_comm.h"
#include "common_venc.h"
#include "sample_case.h"
#include "sample_qpmap.h"
#include "sample_unit_test.h"
#include "sample_venc_log.h"

static AX_BLK gBlkArr[SAMPLE_VENC_MAX_BLK_CNT];


const AX_CHAR *SAMPLE_VENC_H264NaluType(AX_S32 VeChn, AX_S32 type)
{
    switch (type) {
    case AX_H264E_NALU_SPS:
        return "AX_H264E_NALU_SPS";
    case AX_H264E_NALU_PPS:
        return "AX_H264E_NALU_PPS";
    case AX_H264E_NALU_SEI:
        return "AX_H264E_NALU_SEI";
    case AX_H264E_NALU_IDRSLICE:
        return "AX_H264E_NALU_IDRSLICE";
    case AX_H264E_NALU_ISLICE:
        return "AX_H264E_NALU_ISLICE";
    case AX_H264E_NALU_PSLICE:
        return "AX_H264E_NALU_PSLICE";
    case AX_H264E_NALU_BSLICE:
        return "AX_H264E_NALU_BSLICE";
    case AX_H264E_NALU_PREFIX_14:
        return "AX_H264E_NALU_PREFIX_14";
    default:
        SAMPLE_LOG_WARN("VencChn %d: Invalid h264 nalu type(%d)!\n", VeChn, type);
        return "NA";
    }
}

const AX_CHAR *SAMPLE_VENC_H265NaluType(AX_S32 VeChn, AX_S32 type)
{
    switch (type) {
    case AX_H265E_NALU_VPS:
        return "AX_H265E_NALU_VPS";
    case AX_H265E_NALU_SPS:
        return "AX_H265E_NALU_SPS";
    case AX_H265E_NALU_PPS:
        return "AX_H265E_NALU_PPS";
    case AX_H265E_NALU_SEI:
        return "AX_H265E_NALU_SEI";
    case AX_H265E_NALU_IDRSLICE:
        return "AX_H265E_NALU_IDRSLICE";
    case AX_H265E_NALU_ISLICE:
        return "AX_H265E_NALU_ISLICE";
    case AX_H265E_NALU_PSLICE:
        return "AX_H265E_NALU_PSLICE";
    case AX_H265E_NALU_BSLICE:
        return "AX_H265E_NALU_BSLICE";
    case AX_H265E_NALU_TSA_R:
        return "AX_H265E_NALU_TSA_R";
    default:
        SAMPLE_LOG_WARN("VencChn %d: Invalid h265 nalu type(%d)!\n", VeChn, type);
        return "NA";
    }
}

AX_VOID SAMPLE_VENC_PrintNaluInfo(AX_S32 VeChn, AX_VENC_STREAM_T *pstStream)
{
    if (NULL == pstStream)
        return;

    for (AX_S32 naluIdx = 0; naluIdx < pstStream->stPack.u32NaluNum; naluIdx++) {
        if (PT_H264 == pstStream->stPack.enType) {
            SAMPLE_LOG_DEBUG(
                "VencChn %d: nalu idx=%d, nalu type=(%s), size=%d, offset=%d.\n", VeChn, naluIdx,
                SAMPLE_VENC_H264NaluType(VeChn, pstStream->stPack.stNaluInfo[naluIdx].unNaluType.enH264EType),
                pstStream->stPack.stNaluInfo[naluIdx].u32NaluLength,
                pstStream->stPack.stNaluInfo[naluIdx].u32NaluOffset);
        } else if (PT_H265 == pstStream->stPack.enType) {
            SAMPLE_LOG_DEBUG(
                "VencChn %d: nalu idx=%d, nalu type=(%s), size=%d, offset=%d.\n", VeChn, naluIdx,
                SAMPLE_VENC_H265NaluType(VeChn, pstStream->stPack.stNaluInfo[naluIdx].unNaluType.enH265EType),
                pstStream->stPack.stNaluInfo[naluIdx].u32NaluLength,
                pstStream->stPack.stNaluInfo[naluIdx].u32NaluOffset);
        }
    }

    return;
}

AX_PAYLOAD_TYPE_E SampleGetCodecType(AX_S32 codecId)
{
    AX_PAYLOAD_TYPE_E enType = PT_BUTT;

    switch (codecId) {
    case SAMPLE_CODEC_H264:
        enType = PT_H264;
        break;
    case SAMPLE_CODEC_H265:
        enType = PT_H265;
        break;
    case SAMPLE_CODEC_MJPEG:
        enType = PT_MJPEG;
        break;
    case SAMPLE_CODEC_JPEG:
        enType = PT_JPEG;
        break;

    default:
        SAMPLE_LOG_WARN("Invalid codec id(%d).\n", codecId);
        enType = PT_BUTT;
        break;
    }

    return enType;
}

AX_VOID SampleSendFrameInit(VENC_CHN VeChn, AX_PAYLOAD_TYPE_E enType, SAMPLE_VENC_SENDFRAME_PARA_T *pstArg,
                            SAMPLE_VENC_CMD_PARA_T *pCml)
{
    pstArg->bLoopEncode = pCml->bLoopEncode;
    pstArg->bSendFrmStart = AX_TRUE;
    pstArg->eFmt = pCml->picFormat;
    pstArg->encFrmNum = pCml->encFrameNum;
    pstArg->fileInput = pCml->input;
    pstArg->width = pCml->picW;
    pstArg->height = pCml->picH;
    pstArg->strideY = pCml->strideY;
    pstArg->strideU = pCml->strideU;
    pstArg->strideV = pCml->strideV;
    pstArg->syncType = pCml->syncType;
    pstArg->VeChn = VeChn;
    pstArg->frameSize = pCml->frameSize;
    pstArg->blkSize = pCml->BlkSize;

    pstArg->poolId = pCml->poolId;

    pstArg->dynAttrIdx = pCml->dynAttrIdx;
    pstArg->ut = pCml->ut;
    pstArg->function = pCml->function;

    pstArg->stFbcInfo.fbcType = pCml->fbcType;
    pstArg->stFbcInfo.bitDepth = pCml->bitDepth;
    pstArg->stFbcInfo.compLevel = pCml->compLevel;
    pstArg->stFbcInfo.yHdrSize = pCml->yHdrSize;
    pstArg->stFbcInfo.yPadSize = pCml->yPadSize;
    pstArg->stFbcInfo.uvHdrSize = pCml->uvHdrSize;
    pstArg->stFbcInfo.uvPadSize = pCml->uvPadSize;

    pstArg->lumaSize = pCml->lumaSize;
    pstArg->chromaSize = pCml->chromaSize;

    pstArg->qpMapQpType = pCml->qpMapQpType;
    pstArg->qpMapBlkUnit = pCml->qpMapBlkUnit;
    pstArg->qpMapBlkType = pCml->qpMapBlkType;

    pstArg->qpMapSize = pCml->qpMapSize;

    pstArg->rcMode = pCml->rcMode;
    pstArg->rcModeNew = pCml->rcModeNew;

    pstArg->bitRate = pCml->bitRate;
    pstArg->srcFrameRate = pCml->srcFrameRate;
    pstArg->dstFrameRate = pCml->dstFrameRate;
    pstArg->qpMin = pCml->qpMin;
    pstArg->qpMax = pCml->qpMax;
    pstArg->qpMinI = pCml->qpMinI;
    pstArg->qpMaxI = pCml->qpMaxI;
    pstArg->gopLen = pCml->gopLen;
    pstArg->IQp = pCml->IQp;
    pstArg->PQp = pCml->PQp;
    pstArg->BQp = pCml->BQp;
    pstArg->fixedQp = pCml->fixedQp;
    pstArg->enType = enType;
    pstArg->roiEnable = pCml->roiEnable;
    pstArg->qFactor = pCml->qFactor;
    pstArg->qRoiFactor = pCml->qRoiFactor;
    pstArg->vencRoiMap = pCml->vencRoiMap;
    pstArg->jencRoiMap = pCml->jencRoiMap;
    pstArg->u32RefreshNum = pCml->refreshNum;
    pstArg->uDataSize = pCml->uDataSize;

    if (PT_H264 == enType)
        pstArg->maxCuSize = MAX_AVC_CU_SIZE;
    else if (PT_H265 == enType)
        pstArg->maxCuSize = MAX_CU_SIZE;

    if (pCml->bDynRes) {
        pstArg->bDynRes = pCml->bDynRes;
        pstArg->newInput = pCml->newInput;
        pstArg->newPicW = pCml->newPicW;
        pstArg->newPicH = pCml->newPicH;
    }

    pstArg->bPerf = pCml->bPerf;
    pstArg->vbCnt = pCml->vbCnt;

    pstArg->bInsertIDR = pCml->bInsertIDR;
    /* rate jam */
    pstArg->drpFrmMode = pCml->drpFrmMode;
    pstArg->encFrmGap = pCml->encFrmGap;
    pstArg->frmThrBps = pCml->frmThrBps;
    /* super frame */
    pstArg->pri = pCml->pri;
    pstArg->thrI = pCml->thrI;
    pstArg->thrP = pCml->thrP;
    /* multi slice */
    pstArg->sliceNum = pCml->sliceNum;
    /* cvbr parms */
    pstArg->ltMaxBt = pCml->ltMaxBt;
    pstArg->ltMinBt = pCml->ltMinBt;
    pstArg->ltStaTime = pCml->ltStaTime;
    pstArg->shtStaTime = pCml->shtStaTime;
    pstArg->minQpDelta = pCml->minQpDelta;
    pstArg->maxQpDelta = pCml->maxQpDelta;
    pstArg->maxIprop = pCml->maxIprop;
    pstArg->minIprop = pCml->minIprop;
    pstArg->ptrPrivate = (AX_VOID *)pCml;
}

AX_VOID SampleGetStreamInit(VENC_CHN VeChn, AX_PAYLOAD_TYPE_E enType, SAMPLE_VENC_GETSTREAM_PARA_T *pstArg,
                            SAMPLE_VENC_CMD_PARA_T *pCml)
{
    pstArg->bGetStrmStart = AX_TRUE;
    pstArg->enType = enType;
    pstArg->syncType = pCml->syncType;
    pstArg->VeChn = VeChn;
    pstArg->testId = pCml->ut;
    pstArg->gopMode = pCml->gopMode;
    pstArg->temporalID = pCml->temporalID;
    pstArg->bGetStrmBufInfo = pCml->bGetStrmBufInfo;
    pstArg->bQueryStatus = pCml->bQueryStatus;
    pstArg->bSaveStrm = pCml->bSaveStrm;
    pstArg->grpId = pCml->grpId;
    pstArg->output = pCml->output;
    strcpy(pstArg->strmSuffix, pCml->strmSuffix);
    pstArg->ptrPrivate = (AX_VOID *)pCml;
    pstArg->function = pCml->function;
}

AX_VOID *SAMPLE_VENC_SendFrameProc(AX_VOID *arg)
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
    AX_S32 blkSize = pstArg->blkSize;
    AX_S32 poolId = pstArg->poolId;
    FILE *fFileIn = NULL;
    AX_BLK blkId = AX_INVALID_BLOCKID;
    SAMPLE_VENC_CMD_PARA_T *pCml = (SAMPLE_VENC_CMD_PARA_T *)pstArg->ptrPrivate;

    memset(&stFrame, 0, sizeof(stFrame));

    fFileIn = fopen(fileInput, "rb");
    if (fFileIn == NULL) {
        SAMPLE_LOG_ERR("chn-%d: Open input file error!\n", VeChn);
        return NULL;
    }

    while (pstArg->bSendFrmStart) {
        /* get block from user pool */
        blkId = AX_POOL_GetBlock(poolId, blkSize, NULL);
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
        stFrame.stVFrame.enImgFormat = eFmt;

        if (pstArg->stFbcInfo.fbcType) {
            readSize = COMMON_VENC_ReadFbcFile(fFileIn, strideY, height, &pstArg->stFbcInfo, &stFrame.stVFrame);
        } else {
            /* read frame data from yuv file */
            readSize =
                COMMON_VENC_ReadFile(fFileIn, width, strideY, height, eFmt, (void *)stFrame.stVFrame.u64VirAddr[0]);
        }

        if (!bLoopEncode && (readSize <= 0))
            SAMPLE_LOG_WARN("chn-%d: Warning: read frame size : %d less than %d\n", VeChn, readSize, frameSize);

        if (feof(fFileIn)) {
            if (bLoopEncode) {
                fseek(fFileIn, 0, SEEK_SET);
                if (pstArg->stFbcInfo.fbcType) {
                    readSize = COMMON_VENC_ReadFbcFile(fFileIn, strideY, height, &pstArg->stFbcInfo, &stFrame.stVFrame);
                } else {
                    /* read frame data from yuv file */
                    readSize = COMMON_VENC_ReadFile(fFileIn, width, strideY, height, eFmt,
                                                    (void *)stFrame.stVFrame.u64VirAddr[0]);
                }
            } else {
                SAMPLE_LOG_WARN("chn-%d: End of input file!\n", VeChn);
                /* no more frames, stop encoder */
                goto EXIT;
            }
        }

        stFrame.stVFrame.u64SeqNum = pstArg->totalSendFrame + 1;
        stFrame.stVFrame.u32Width = width;
        stFrame.stVFrame.u32Height = height;
        stFrame.stVFrame.u32PicStride[0] = strideY;
        stFrame.stVFrame.u32PicStride[1] = strideU;
        stFrame.stVFrame.u32PicStride[2] = strideV;

        if ((pstArg->totalSendFrame + 1) == pstArg->dynAttrIdx) {
            if (pstArg->function) {
                SAMPLE_LOG_DEBUG("chn-%d: totalInputFrames %llu dynAttrIdx %d \n", VeChn, pstArg->totalSendFrame,
                                 pstArg->dynAttrIdx);
                s32Ret = pstArg->function(VeChn, (AX_VOID *)pstArg);
                if (AX_SUCCESS != s32Ret) {
                    SAMPLE_LOG_ERR("chn-%d: pfn failed, ret=%x\n", VeChn, s32Ret);
                    goto EXIT;
                }
            }
        }

        s32Ret = AX_VENC_SendFrame(VeChn, &stFrame, syncType);
        if (AX_SUCCESS != s32Ret)
            SAMPLE_LOG_WARN("chn-%d: AX_VENC_SendFrame failed, ret=%x\n", VeChn, s32Ret);

        pstArg->totalSendFrame++;

        if (pstArg->totalSendFrame == encFrmNum) {
            SAMPLE_LOG("chn-%d: Want to encode %llu frames, exit!\n", VeChn, pstArg->totalSendFrame);
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

    SAMPLE_LOG("chn-%d - Total input %llu frames, Encoder exit!\n", VeChn, pstArg->totalSendFrame);

    COMMON_VENC_AdjustLoopExit(&pCml->vencLoopExit, VeChn);

    return NULL;
}

AX_VOID *SAMPLE_VENC_SendFramePerfProc(AX_VOID *arg)
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
    AX_S32 blkSize = pstArg->blkSize;
    AX_S32 poolId = pstArg->poolId;
    FILE *fFileIn = NULL;
    AX_BLK blkId = AX_INVALID_BLOCKID;
    SAMPLE_VENC_CMD_PARA_T *pCml = (SAMPLE_VENC_CMD_PARA_T *)pstArg->ptrPrivate;

    memset(&stFrame, 0, sizeof(stFrame));

    fFileIn = fopen(fileInput, "rb");
    if (fFileIn == NULL) {
        SAMPLE_LOG_ERR("chn-%d: Open input file error!\n", VeChn);
        return NULL;
    }

    for (AX_S32 blk = 0; blk < pstArg->vbCnt; blk++) {
        /* get block from user pool */
        blkId = AX_POOL_GetBlock(poolId, blkSize, NULL);
        if (AX_INVALID_BLOCKID == blkId) {
            SAMPLE_LOG("chn-%d: get block err!\n", VeChn);
            usleep(5000);
            continue;
        }

        stFrame.stVFrame.u32FrameSize = frameSize;
        stFrame.stVFrame.u64PhyAddr[0] = AX_POOL_Handle2PhysAddr(blkId);
        stFrame.stVFrame.u64VirAddr[0] = (AX_U64)AX_POOL_GetBlockVirAddr(blkId);
        stFrame.stVFrame.u32BlkId[0] = blkId;
        stFrame.stVFrame.u32BlkId[1] = 0;
        stFrame.stVFrame.u32BlkId[2] = 0;
        stFrame.stVFrame.enImgFormat = eFmt;

        /* read frame data from yuv file */
        readSize = COMMON_VENC_ReadFile(fFileIn, width, strideY, height, eFmt, (void *)stFrame.stVFrame.u64VirAddr[0]);

        if (!bLoopEncode && (readSize <= 0))
            SAMPLE_LOG_WARN("chn-%d: Warning: read frame size : %d less than %d\n", VeChn, readSize, frameSize);

        if (feof(fFileIn)) {
            if (bLoopEncode) {
                fseek(fFileIn, 0, SEEK_SET);
                /* read frame data from yuv file */
                readSize =
                    COMMON_VENC_ReadFile(fFileIn, width, strideY, height, eFmt, (void *)stFrame.stVFrame.u64VirAddr[0]);
            } else {
                SAMPLE_LOG_WARN("chn-%d: End of input file!\n", VeChn);
                /* no more frames, stop encoder */
                goto EXIT;
            }
        }

        gBlkArr[blk] = blkId;
    }

    for (AX_S32 blk = pstArg->vbCnt - 1; blk >= 0; blk--) {
        if (AX_INVALID_BLOCKID != gBlkArr[blk])
            AX_POOL_ReleaseBlock(gBlkArr[blk]);
    }

    memset(&stFrame, 0, sizeof(stFrame));

    while (pstArg->bSendFrmStart) {
        /* get block from user pool */
        blkId = AX_POOL_GetBlock(poolId, blkSize, NULL);
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
        stFrame.stVFrame.enImgFormat = eFmt;

        stFrame.stVFrame.u64SeqNum = pstArg->totalSendFrame + 1;
        stFrame.stVFrame.u32Width = width;
        stFrame.stVFrame.u32Height = height;
        stFrame.stVFrame.u32PicStride[0] = strideY;
        stFrame.stVFrame.u32PicStride[1] = strideU;
        stFrame.stVFrame.u32PicStride[2] = strideV;

        s32Ret = AX_VENC_SendFrame(VeChn, &stFrame, syncType);
        if (AX_SUCCESS != s32Ret)
            SAMPLE_LOG("chn-%d: AX_VENC_SendFrame failed, ret=%x\n", VeChn, s32Ret);

        pstArg->totalSendFrame++;

        if (pstArg->totalSendFrame == encFrmNum) {
            SAMPLE_LOG("chn-%d: Want to encode %llu frames, exit!\n", VeChn, pstArg->totalSendFrame);
            goto EXIT;
        }

        if (0 == (pstArg->totalSendFrame % pstArg->vbCnt)) {
            for (AX_S32 blk = pstArg->vbCnt - 1; blk >= 0; blk--) {
                if (AX_INVALID_BLOCKID != gBlkArr[blk])
                    AX_POOL_ReleaseBlock(gBlkArr[blk]);
            }
        }
    }


EXIT:

    if (NULL != fFileIn) {
        fclose(fFileIn);
        fFileIn = NULL;
    }

    for (AX_S32 blk = 0; blk < pstArg->vbCnt; blk++) {
        if (AX_INVALID_BLOCKID != gBlkArr[blk])
            AX_POOL_ReleaseBlock(gBlkArr[blk]);
    }

    SAMPLE_LOG("chn-%d - Total input %llu frames, Encoder exit!\n", VeChn, pstArg->totalSendFrame);

    COMMON_VENC_AdjustLoopExit(&pCml->vencLoopExit, VeChn);

    return NULL;
}

AX_VOID *SAMPLE_VENC_SendFrameQpMapProc(AX_VOID *arg)
{
    AX_BOOL bLoopEncode;
    AX_U32 encFrmNum;
    const AX_CHAR *fileInput;
    AX_S32 syncType;
    AX_IMG_FORMAT_E eFmt;
    AX_U32 width;
    AX_U32 height;
    AX_U32 strideY, strideU, strideV;
    AX_USER_FRAME_INFO_T stFrame;
    SAMPLE_VENC_SENDFRAME_PARA_T *pstArg;
    AX_S32 readSize;
    AX_S32 s32Ret;
    pstArg = (SAMPLE_VENC_SENDFRAME_PARA_T *)arg;
    AX_VOID *handle = NULL;

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

    AX_BLK blkId = AX_INVALID_BLOCKID;
    AX_S32 frameSize = pstArg->frameSize;
    AX_S32 blkSize = pstArg->blkSize;
    AX_S32 poolId = pstArg->poolId;

    AX_U64 u64QpMapPhyAddr;
    AX_S8 *QpMapVirAddr;

    AX_S32 qpmapSize = pstArg->qpMapSize;
    AX_S32 qpmapQpType = pstArg->qpMapQpType;
    AX_S32 qpmapBlkType = pstArg->qpMapBlkType;
    AX_S32 qpmapBlkUnit = pstArg->qpMapBlkUnit;
    AX_S32 maxCuSize = pstArg->maxCuSize;
    SAMPLE_VENC_CMD_PARA_T *pCml = (SAMPLE_VENC_CMD_PARA_T *)pstArg->ptrPrivate;

    FILE *fFileIn = NULL;

    memset(&stFrame, 0, sizeof(stFrame));

    fFileIn = fopen(fileInput, "rb");
    if (fFileIn == NULL) {
        SAMPLE_LOG_ERR("chn-%d: Open input file error!\n", VeChn);
        return NULL;
    }

    while (pstArg->bSendFrmStart) {
        /* get block from user pool */
        blkId = AX_POOL_GetBlock(poolId, blkSize, NULL);
        if (AX_INVALID_BLOCKID == blkId) {
            usleep(5000);
            continue;
        }

        stFrame.stUserFrame.stVFrame.u32FrameSize = frameSize;
        stFrame.stUserFrame.stVFrame.u64PhyAddr[0] = AX_POOL_Handle2PhysAddr(blkId);
        stFrame.stUserFrame.stVFrame.u64VirAddr[0] = (AX_U64)AX_POOL_GetBlockVirAddr(blkId);
        stFrame.stUserFrame.stVFrame.u32BlkId[0] = blkId;
        stFrame.stUserFrame.stVFrame.u32BlkId[1] = 0;
        stFrame.stUserFrame.stVFrame.u32BlkId[2] = 0;

        u64QpMapPhyAddr = stFrame.stUserFrame.stVFrame.u64PhyAddr[0] + frameSize;
        QpMapVirAddr = (AX_S8 *)(stFrame.stUserFrame.stVFrame.u64VirAddr[0] + frameSize);

        stFrame.stUserRcInfo.u64QpMapPhyAddr = u64QpMapPhyAddr;
        stFrame.stUserRcInfo.pQpMapVirAddr = QpMapVirAddr;
        stFrame.stUserRcInfo.u32RoiMapDeltaSize = qpmapSize;

        if (pstArg->stFbcInfo.fbcType) {
            readSize =
                COMMON_VENC_ReadFbcFile(fFileIn, strideY, height, &pstArg->stFbcInfo, &stFrame.stUserFrame.stVFrame);
        } else {
            /* read frame data from yuv file */
            readSize = COMMON_VENC_ReadFile(fFileIn, width, strideY, height, eFmt,
                                            (void *)stFrame.stUserFrame.stVFrame.u64VirAddr[0]);
        }

        if (!bLoopEncode && (readSize <= 0))
            SAMPLE_LOG_WARN("chn-%d: Warning: read frame size : %d less than %d\n", VeChn, readSize, frameSize);

        if (feof(fFileIn)) {
            if (bLoopEncode) {
                fseek(fFileIn, 0, SEEK_SET);
                if (pstArg->stFbcInfo.fbcType) {
                    readSize = COMMON_VENC_ReadFbcFile(fFileIn, strideY, height, &pstArg->stFbcInfo,
                                                       &stFrame.stUserFrame.stVFrame);
                } else {
                    /* read frame data from yuv file */
                    readSize = COMMON_VENC_ReadFile(fFileIn, width, strideY, height, eFmt,
                                                    (void *)stFrame.stUserFrame.stVFrame.u64VirAddr[0]);
                }
            } else {
                SAMPLE_LOG_WARN("chn-%d: End of input file!\n", VeChn);
                /* no more frames, stop encoder */
                goto EXIT;
            }
        }

        memset(QpMapVirAddr, 0x0, qpmapSize);
        /* qpmap setting*/
        if (AX_VENC_QPMAP_QP_DELTA == qpmapQpType || AX_VENC_QPMAP_QP_ABS == qpmapQpType)
            SampleCopyQPDelta2Memory(width, height, maxCuSize, qpmapBlkUnit, QpMapVirAddr, qpmapQpType);

        /* skip/ipcm map setting */
        if (AX_VENC_QPMAP_BLOCK_SKIP == qpmapBlkType || AX_VENC_QPMAP_BLOCK_IPCM == qpmapBlkType)
            SampleCopyFlagsMap2Memory(width, height, maxCuSize, qpmapBlkUnit, QpMapVirAddr, qpmapBlkType);

        stFrame.stUserFrame.stVFrame.u64SeqNum = pstArg->totalSendFrame + 1;
        stFrame.stUserFrame.stVFrame.enImgFormat = eFmt;
        stFrame.stUserFrame.stVFrame.u32Width = width;
        stFrame.stUserFrame.stVFrame.u32Height = height;
        stFrame.stUserFrame.stVFrame.u32PicStride[0] = strideY;
        stFrame.stUserFrame.stVFrame.u32PicStride[1] = strideU;
        stFrame.stUserFrame.stVFrame.u32PicStride[2] = strideV;

        if ((pstArg->totalSendFrame + 1) == pstArg->dynAttrIdx) {
            if (pstArg->function) {
                SAMPLE_LOG_DEBUG("chn-%d: totalInputFrames %llu dynAttrIdx %d \n", VeChn, pstArg->totalSendFrame,
                                 pstArg->dynAttrIdx);
                handle = (AX_VOID *)pstArg;
                s32Ret = pstArg->function(VeChn, handle);
                if (AX_SUCCESS != s32Ret) {
                    SAMPLE_LOG_ERR("chn-%d: pfn failed, ret=%x\n", VeChn, s32Ret);
                    goto EXIT;
                }
            }
        }

        s32Ret = AX_VENC_SendFrameEx(VeChn, &stFrame, syncType);
        if (AX_SUCCESS != s32Ret)
            SAMPLE_LOG_WARN("chn-%d: AX_VENC_SendFrameEx failed, ret=%x\n", VeChn, s32Ret);

        pstArg->totalSendFrame++;

        if (pstArg->totalSendFrame == encFrmNum) {
            SAMPLE_LOG("chn-%d: Want to encode %llu frames, exit!\n", VeChn, pstArg->totalSendFrame);
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

    SAMPLE_LOG("chn-%d - Total input %llu frames, Encoder exit!\n", VeChn, pstArg->totalSendFrame);

    COMMON_VENC_AdjustLoopExit(&pCml->vencLoopExit, VeChn);

    return NULL;
}

AX_VOID *SAMPLE_VENC_SendFrameDynResolutionProc(AX_VOID *arg)
{
    AX_BOOL bLoopEncode;
    AX_U32 encFrmNum;
    AX_S32 syncType;
    AX_IMG_FORMAT_E eFmt;
    AX_VIDEO_FRAME_INFO_T stFrame;
    SAMPLE_VENC_SENDFRAME_PARA_T *pstArg;
    AX_S32 readSize;
    AX_S32 s32Ret;
    SAMPLE_VENC_CMD_PARA_T *pCml = NULL;
    pstArg = (SAMPLE_VENC_SENDFRAME_PARA_T *)arg;

    VENC_CHN VeChn = pstArg->VeChn;
    bLoopEncode = pstArg->bLoopEncode;
    encFrmNum = pstArg->encFrmNum;
    syncType = pstArg->syncType;
    eFmt = pstArg->eFmt;
    AX_BLK blkId = AX_INVALID_BLOCKID;
    AX_VOID *handle = NULL;
    pCml = (SAMPLE_VENC_CMD_PARA_T *)pstArg->ptrPrivate;

    memset(&stFrame, 0, sizeof(stFrame));

    pstArg->fFileIn = fopen(pstArg->fileInput, "rb");
    if (pstArg->fFileIn == NULL) {
        SAMPLE_LOG_ERR("chn-%d: Open input file error!\n", VeChn);
        return NULL;
    }

    while (pstArg->bSendFrmStart) {
        if ((pstArg->totalSendFrame + 1) == pstArg->dynAttrIdx) {
            if (pstArg->function) {
                SAMPLE_LOG_DEBUG("chn-%d: totalInputFrames %llu dynAttrIdx %d \n", VeChn, pstArg->totalSendFrame,
                                 pstArg->dynAttrIdx);
                handle = (AX_VOID *)pstArg;
                s32Ret = pstArg->function(VeChn, handle);
                if (AX_SUCCESS != s32Ret) {
                    SAMPLE_LOG_ERR("chn-%d: pfn failed, ret=%x\n", VeChn, s32Ret);
                    goto EXIT;
                }
            }
        }

        /* get block from user pool */
        blkId = AX_POOL_GetBlock(pstArg->poolId, pstArg->blkSize, NULL);
        if (AX_INVALID_BLOCKID == blkId) {
            usleep(5000);
            continue;
        }

        stFrame.stVFrame.u32FrameSize = pstArg->frameSize;
        stFrame.stVFrame.u64PhyAddr[0] = AX_POOL_Handle2PhysAddr(blkId);
        stFrame.stVFrame.u64VirAddr[0] = (AX_U64)AX_POOL_GetBlockVirAddr(blkId);
        stFrame.stVFrame.u32BlkId[0] = blkId;
        stFrame.stVFrame.u32BlkId[1] = 0;
        stFrame.stVFrame.u32BlkId[2] = 0;

        if (pstArg->stFbcInfo.fbcType) {
            readSize = COMMON_VENC_ReadFbcFile(pstArg->fFileIn, pstArg->strideY, pstArg->height, &pstArg->stFbcInfo,
                                               &stFrame.stVFrame);
        } else {
            /* read frame data from yuv file */
            readSize = COMMON_VENC_ReadFile(pstArg->fFileIn, pstArg->width, pstArg->strideY, pstArg->height, eFmt,
                                            (void *)stFrame.stVFrame.u64VirAddr[0]);
        }

        if (!bLoopEncode && (readSize <= 0))
            SAMPLE_LOG_WARN("chn-%d: Warning: read frame size : %d less than %d\n", VeChn, readSize, pstArg->frameSize);

        if (feof(pstArg->fFileIn)) {
            if (bLoopEncode) {
                fseek(pstArg->fFileIn, 0, SEEK_SET);
                if (pstArg->stFbcInfo.fbcType) {
                    readSize = COMMON_VENC_ReadFbcFile(pstArg->fFileIn, pstArg->strideY, pstArg->height,
                                                       &pstArg->stFbcInfo, &stFrame.stVFrame);
                } else {
                    /* read frame data from yuv file */
                    readSize = COMMON_VENC_ReadFile(pstArg->fFileIn, pstArg->width, pstArg->strideY, pstArg->height,
                                                    eFmt, (void *)stFrame.stVFrame.u64VirAddr[0]);
                }
            } else {
                SAMPLE_LOG_WARN("chn-%d: End of input file!\n", VeChn);
                /* no more frames, stop encoder */
                goto EXIT;
            }
        }

        stFrame.stVFrame.u64SeqNum = pstArg->totalSendFrame + 1;
        stFrame.stVFrame.enImgFormat = eFmt;
        stFrame.stVFrame.u32Width = pstArg->width;
        stFrame.stVFrame.u32Height = pstArg->height;
        stFrame.stVFrame.u32PicStride[0] = pstArg->strideY;
        stFrame.stVFrame.u32PicStride[1] = pstArg->strideU;
        stFrame.stVFrame.u32PicStride[2] = pstArg->strideV;

        s32Ret = AX_VENC_SendFrame(VeChn, &stFrame, syncType);
        if (AX_SUCCESS != s32Ret)
            SAMPLE_LOG_WARN("chn-%d: AX_VENC_SendFrame failed, ret=%x\n", VeChn, s32Ret);

        pstArg->totalSendFrame++;

        if (pstArg->totalSendFrame == encFrmNum) {
            SAMPLE_LOG("chn-%d: Want to encode %llu frames, exit!\n", VeChn, pstArg->totalSendFrame);
            goto EXIT;
        }

        if (AX_INVALID_BLOCKID != blkId)
            AX_POOL_ReleaseBlock(blkId);
    }


EXIT:

    if (NULL != pstArg->fFileIn) {
        fclose(pstArg->fFileIn);
        pstArg->fFileIn = NULL;
    }

    if (AX_INVALID_BLOCKID != blkId)
        AX_POOL_ReleaseBlock(blkId);

    SAMPLE_LOG("chn-%d - Total input %llu frames, Encoder exit!\n", VeChn, pstArg->totalSendFrame);

    pCml->poolIdDynRes = pstArg->poolId;

    COMMON_VENC_AdjustLoopExit(&pCml->vencLoopExit, VeChn);

    return NULL;
}

AX_S32 SAMPLE_VENC_StartSendFrame(SAMPLE_VENC_SENDFRAME_PARA_T *pstArg)
{
    AX_S32 s32Ret;

    if (pstArg->bDynRes)
        s32Ret = pthread_create(&pstArg->sendFrmPid, 0, SAMPLE_VENC_SendFrameDynResolutionProc, (AX_VOID *)pstArg);
    else if (pstArg->qpMapQpType || pstArg->qpMapBlkType)
        s32Ret = pthread_create(&pstArg->sendFrmPid, 0, SAMPLE_VENC_SendFrameQpMapProc, (AX_VOID *)pstArg);
    else if (pstArg->bPerf)
        s32Ret = pthread_create(&pstArg->sendFrmPid, 0, SAMPLE_VENC_SendFramePerfProc, (AX_VOID *)pstArg);
    else
        s32Ret = pthread_create(&pstArg->sendFrmPid, 0, SAMPLE_VENC_SendFrameProc, (AX_VOID *)pstArg);

    return s32Ret;
}

AX_S32 SAMPLE_VENC_StopSendFrame(SAMPLE_VENC_SENDFRAME_PARA_T *pstArg)
{
    if (pstArg->bSendFrmStart) {
        pstArg->bSendFrmStart = AX_FALSE;
        pthread_join(pstArg->sendFrmPid, 0);
    }

    return AX_SUCCESS;
}

AX_VOID *SAMPLE_VENC_GetStreamProc(AX_VOID *arg)
{
    AX_S32 s32Ret = -1;
    AX_VENC_STREAM_T stStream;
    FILE *pStrm = NULL;
    SAMPLE_VENC_GETSTREAM_PARA_T *pstArg = (SAMPLE_VENC_GETSTREAM_PARA_T *)arg;
    VENC_CHN VeChn = pstArg->VeChn;
    AX_S32 testId = pstArg->testId;
    AX_CHAR esName[50];
    AX_PAYLOAD_TYPE_E enType = pstArg->enType;

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
        }
    }

EXIT:
    if (pStrm != NULL) {
        fclose(pStrm);
        pStrm = NULL;
    }

    SAMPLE_LOG("chn-%d: Total get %llu encoded frames. getStream Exit!\n", VeChn, pstArg->totalGetStream);

    return (void *)(intptr_t)s32Ret;
}

AX_S32 SAMPLE_VENC_StartGetStream(SAMPLE_VENC_GETSTREAM_PARA_T *pstArg)
{
    AX_S32 s32Ret;

    s32Ret = pthread_create(&pstArg->getStrmPid, 0, SAMPLE_VENC_GetStreamProc, (AX_VOID *)pstArg);

    return s32Ret;
}

AX_S32 SAMPLE_VENC_StopGetStream(SAMPLE_VENC_GETSTREAM_PARA_T *pstArg)
{
    if (pstArg->bGetStrmStart) {
        pstArg->bGetStrmStart = AX_FALSE;
        pthread_join(pstArg->getStrmPid, 0);
    }

    return AX_SUCCESS;
}

AX_VOID SAMPLE_VENC_FWRITE(const AX_VOID *ptr, AX_S32 size, AX_S32 nmemb, FILE *stream, AX_BOOL bSaveFile)
{
    if (!bSaveFile)
        return;

    fwrite(ptr, 1, nmemb, stream);

    return;
}

AX_VOID SAMPLE_VENC_SetThreadName(const AX_CHAR *nameFmt, ...)
{
    AX_CHAR name[50];
    va_list args;

    va_start(args, nameFmt);
    vsnprintf(name, sizeof(name), nameFmt, args);
    va_end(args);

    prctl(PR_SET_NAME, name, NULL, NULL, NULL);
}