/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include "sample_case.h"
#include "sample_global.h"
#include "sample_roicfg.h"

SAMPLE_VENC_ENCODEONCEFRAME_PARA_T gstOnceFrmParam[MAX_ENCODE_ONCE_NUM];
static AX_U32 gTileSizeTable[] = {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320};

static void SampleJpegEncOnceFrmInit(VENC_CHN VeChn, SAMPLE_VENC_ENCODEONCEFRAME_PARA_T *pstArg,
                                     SAMPLE_VENC_CMD_PARA_T *pCml)
{
    pstArg->bEncOnceFrmStart = AX_TRUE;
    pstArg->VeChn = VeChn;
    pstArg->bLoopEncode = pCml->bLoopEncode;
    pstArg->encFrmNum = pCml->encFrameNum;
    pstArg->fileInput = pCml->input;
    pstArg->syncType = -1;
    pstArg->eFmt = pCml->picFormat;
    pstArg->width = pCml->picW;
    pstArg->height = pCml->picH;
    pstArg->strideY = pCml->strideY;
    pstArg->strideU = pCml->strideU;
    pstArg->strideV = pCml->strideV;
    pstArg->qFactor = pCml->qFactor;
    pstArg->bCrop = pCml->bCrop;
    pstArg->cropX = pCml->cropX;
    pstArg->cropY = pCml->cropY;
    pstArg->cropW = pCml->cropW;
    pstArg->cropH = pCml->cropH;
    pstArg->roiEnable = pCml->roiEnable;
    pstArg->roimapFile = pCml->jencRoiMap;
    pstArg->qRoiFactor = pCml->qRoiFactor;
    pstArg->frameSize = pCml->frameSize;
    pstArg->strmBufSize = pCml->strmBufSize;
    pstArg->testId = pCml->ut;
    pstArg->bSaveStrm = pCml->bSaveStrm;
    /* fbc init */
    pstArg->stFbcInfo.fbcType = pCml->fbcType;
    pstArg->stFbcInfo.bitDepth = pCml->bitDepth;
    pstArg->stFbcInfo.compLevel = pCml->compLevel;
    pstArg->stFbcInfo.yHdrSize = pCml->yHdrSize;
    pstArg->stFbcInfo.yPadSize = pCml->yPadSize;
    pstArg->stFbcInfo.uvHdrSize = pCml->uvHdrSize;
    pstArg->stFbcInfo.uvPadSize = pCml->uvPadSize;
}

static void SampleJpegRoiInit(SAMPLE_ROI_CFG_T *pstArg, SAMPLE_VENC_ENCODEONCEFRAME_PARA_T *pCml)
{
    pstArg->qFactor = pCml->qFactor;
    pstArg->roiEnable = pCml->roiEnable;
    pstArg->roimapFile = pCml->roimapFile;
    pstArg->qRoiFactor = pCml->qRoiFactor;
}

static AX_S32 SampleJpegSendFrmCfg(SAMPLE_VENC_ENCODEONCEFRAME_PARA_T *pCmdl, AX_VIDEO_FRAME_INFO_T *pStFrame)
{
    AX_IMG_FORMAT_E picFormat = AX_FORMAT_INVALID;
    AX_U32 stride[3] = {0};
    AX_U32 frameSize = 0;
    AX_U32 input_width = 0;
    AX_U32 input_height = 0;
    AX_U64 phyBuff = 0;
    AX_VOID *virBuff = NULL;
    AX_S32 s32Ret = AX_SUCCESS;
    AX_S32 i = 0;

    input_width = pCmdl->width;
    input_height = pCmdl->height;
    picFormat = pCmdl->eFmt;
    pStFrame->stVFrame.enImgFormat = picFormat;
    pStFrame->stVFrame.u32Width = input_width;
    pStFrame->stVFrame.u32Height = input_height;

    /********** stride config ************/
    stride[0] = pCmdl->strideY;
    stride[1] = pCmdl->strideU;
    stride[2] = pCmdl->strideV;
    frameSize = pCmdl->frameSize;

    s32Ret = AX_SYS_MemAlloc(&phyBuff, &virBuff, frameSize, 0, (AX_S8 *)JPEG_ENCODE_ONCE_NAME);
    if (s32Ret) {
        SAMPLE_LOG_ERR("alloc mem err, size(%d).\n", frameSize);
        return -1;
    }
    pStFrame->stVFrame.u32FrameSize = frameSize;
    pStFrame->stVFrame.u64PhyAddr[0] = phyBuff;
    pStFrame->stVFrame.u64VirAddr[0] = (AX_U64)virBuff;

    switch (picFormat) {
    case AX_FORMAT_YUV420_PLANAR:
        pStFrame->stVFrame.u64PhyAddr[1] = pStFrame->stVFrame.u64PhyAddr[0] + stride[0] * input_height;
        pStFrame->stVFrame.u64PhyAddr[2] = pStFrame->stVFrame.u64PhyAddr[1] + stride[1] * input_height / 2;
        pStFrame->stVFrame.u64VirAddr[1] = pStFrame->stVFrame.u64VirAddr[0] + stride[0] * input_height;
        pStFrame->stVFrame.u64VirAddr[2] = pStFrame->stVFrame.u64VirAddr[1] + stride[1] * input_height / 2;
        break;
    case AX_FORMAT_YUV420_SEMIPLANAR:
    case AX_FORMAT_YUV420_SEMIPLANAR_VU:
        pStFrame->stVFrame.u64PhyAddr[1] = pStFrame->stVFrame.u64PhyAddr[0] + stride[0] * input_height;
        pStFrame->stVFrame.u64PhyAddr[2] = 0;
        pStFrame->stVFrame.u64VirAddr[1] = pStFrame->stVFrame.u64VirAddr[0] + stride[0] * input_height;
        pStFrame->stVFrame.u64VirAddr[2] = 0;
        break;
    case AX_FORMAT_YUV422_INTERLEAVED_YUYV:
    case AX_FORMAT_YUV422_INTERLEAVED_UYVY:
        pStFrame->stVFrame.u64PhyAddr[1] = 0;
        pStFrame->stVFrame.u64PhyAddr[2] = 0;
        pStFrame->stVFrame.u64VirAddr[1] = 0;
        pStFrame->stVFrame.u64VirAddr[2] = 0;
        break;
    default:
        break;
    }

    for (i = 0; i < 3; i++)
        pStFrame->stVFrame.u32PicStride[i] = stride[i];

    return AX_SUCCESS;
}

static AX_S32 SampleJpegOnceOutBufCfg(AX_JPEG_ENCODE_ONCE_PARAMS_T *pStJpegEncodeOnceParam, AX_U32 frameSize)
{
    AX_U64 phyBuff = 0;
    AX_VOID *virBuff = NULL;
    AX_S32 s32Ret = AX_SUCCESS;

    s32Ret = AX_SYS_MemAllocCached(&phyBuff, &virBuff, frameSize, 0, (AX_S8 *)"JPEG_ONCE_STREAM");
    if (s32Ret) {
        SAMPLE_LOG_ERR("alloc mem err, size(%d).\n", frameSize);
        return -1;
    }

    pStJpegEncodeOnceParam->u32Len = frameSize;
    pStJpegEncodeOnceParam->ulPhyAddr = phyBuff;
    pStJpegEncodeOnceParam->pu8Addr = (AX_U8 *)virBuff;

    return AX_SUCCESS;
}

static AX_S32 SampleJpegOnceReadFbcFile(FILE *pFileIn, AX_VIDEO_FRAME_T *pstFrame, SAMPLE_VENC_FBC_INFO_T *pFbc)
{
    AX_S32 lumaData = 0, chromaData = 0;
    AX_S32 readSize = 0;

    if (VENC_FBC_LOSSLESS == pFbc->fbcType) {
        lumaData = pFbc->yHdrSize + pFbc->yPadSize;
        chromaData = pFbc->uvHdrSize + pFbc->uvPadSize;

        readSize = fread((AX_VOID *)pstFrame->u64VirAddr[0], 1, lumaData, pFileIn);
        if (readSize < lumaData)
            return -1;

        pstFrame->u64PhyAddr[1] = pstFrame->u64PhyAddr[0] + lumaData;
        pstFrame->u64VirAddr[1] = pstFrame->u64VirAddr[0] + lumaData;

        readSize += fread((AX_VOID *)pstFrame->u64VirAddr[1], 1, chromaData, pFileIn);
        if (readSize < chromaData)
            return -1;

        pstFrame->stCompressInfo.enCompressMode = AX_COMPRESS_MODE_LOSSLESS;
        pstFrame->u32HeaderSize[0] = pFbc->yHdrSize;
        pstFrame->u32HeaderSize[1] = pFbc->uvHdrSize;

    } else if (VENC_FBC_LOSSY == pFbc->fbcType) {
        pstFrame->stCompressInfo.enCompressMode = AX_COMPRESS_MODE_LOSSY;
        pstFrame->stCompressInfo.u32CompressLevel = pFbc->compLevel;

        lumaData = (TILE_ALIGN(pstFrame->u32Width, 128) / 128) * (TILE_ALIGN(pstFrame->u32Height, 2) / 2) *
                   gTileSizeTable[pFbc->compLevel];

        chromaData = lumaData / 2;
        SAMPLE_LOG("Luma size=%d, chroma size=%d.\n", lumaData, chromaData);

        readSize = fread((AX_VOID *)pstFrame->u64VirAddr[0], 1, lumaData + chromaData, pFileIn);
        if (readSize < (lumaData + chromaData))
            return -1;

        pstFrame->u64PhyAddr[1] = pstFrame->u64PhyAddr[0] + lumaData;
    }

    return readSize;
}

static AX_S32 JpegEncodeOneFrame(SAMPLE_VENC_ENCODEONCEFRAME_PARA_T *pCmdl)
{
    AX_JPEG_ENCODE_ONCE_PARAMS_T stJpegEncodeOnceParam;
    AX_IMG_FORMAT_E picFormat = AX_FORMAT_INVALID;
    AX_U32 frameSize = 0;
    AX_U32 strmBufSize = 0;
    AX_U32 input_width = 0;
    AX_U32 input_height = 0;
    FILE *fFileIn = NULL;
    AX_VIDEO_FRAME_INFO_T stFrame;
    AX_S32 s32Ret = AX_SUCCESS;
    AX_S32 readSize = 0;
    AX_BOOL bLoopEncode = 0;
    AX_U64 totalEncodedFrames = 0;
    AX_U32 i = 0;
    SAMPLE_ROI_CFG_T roiCfg;
    AX_S32 testId = 0;

    if (NULL == pCmdl) {
        SAMPLE_LOG_ERR("pCmdl == NULL!\n");
        return -1;
    }

    if (NULL == pCmdl->fileInput) {
        SAMPLE_LOG_ERR("pCmdl->input == NULL!\n");
        return -1;
    }

    testId = pCmdl->testId;
    bLoopEncode = pCmdl->bLoopEncode;
    memset(&stJpegEncodeOnceParam, 0, sizeof(stJpegEncodeOnceParam));
    memset(&stFrame, 0, sizeof(stFrame));
    fFileIn = fopen(pCmdl->fileInput, "rb");
    if (fFileIn == NULL) {
        SAMPLE_LOG_ERR("open input file error!\n");
        return -1;
    }

    if (pCmdl->roiEnable) {
        memset(&roiCfg, 0x0, sizeof(SAMPLE_ROI_CFG_T));
        SampleJpegRoiInit(&roiCfg, pCmdl);
        s32Ret = SampleJpegRoiCfg(&roiCfg, &stJpegEncodeOnceParam.stJpegParam);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_LOG_ERR("SampleJpegRoiCfg err.\n");
            s32Ret = -1;
            goto EXIT;
        }
    }

    s32Ret = SampleJpegSendFrmCfg(pCmdl, &stFrame);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("SampleJpegSendFrmCfg err.\n");
        s32Ret = -1;
        goto EXIT;
    }

    picFormat = stFrame.stVFrame.enImgFormat;
    input_width = stFrame.stVFrame.u32Width;
    input_height = stFrame.stVFrame.u32Height;
    frameSize = stFrame.stVFrame.u32FrameSize;
    strmBufSize = pCmdl->strmBufSize == 0 ? frameSize : pCmdl->strmBufSize;

    stJpegEncodeOnceParam.stJpegParam.u32Qfactor = pCmdl->qFactor;
    stJpegEncodeOnceParam.u32Width = input_width;
    stJpegEncodeOnceParam.u32Height = input_height;
    stJpegEncodeOnceParam.enImgFormat = picFormat;
    if (pCmdl->bCrop) {
        stJpegEncodeOnceParam.s16CropX = pCmdl->cropX;
        stJpegEncodeOnceParam.s16CropY = pCmdl->cropY;
        stJpegEncodeOnceParam.s16CropHeight = pCmdl->cropW;
        stJpegEncodeOnceParam.s16CropHeight = pCmdl->cropH;
    }

    stJpegEncodeOnceParam.enStrmBufType = AX_STREAM_BUF_CACHE;
    if (pCmdl->stFbcInfo.fbcType) {
        stJpegEncodeOnceParam.stCompressInfo.enCompressMode = pCmdl->stFbcInfo.fbcType;
        stJpegEncodeOnceParam.stCompressInfo.u32CompressLevel = pCmdl->stFbcInfo.compLevel;
    }

    while (1) {
        s32Ret = SampleJpegOnceOutBufCfg(&stJpegEncodeOnceParam, strmBufSize);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_LOG_ERR("SampleJpegOnceOutBufCfg err.\n");
            s32Ret = -1;
            goto EXIT;
        }

        if (pCmdl->stFbcInfo.fbcType)
            readSize = SampleJpegOnceReadFbcFile(fFileIn, &stFrame.stVFrame, &pCmdl->stFbcInfo);
        else
            readSize = COMMON_VENC_ReadFile(fFileIn, input_width, stFrame.stVFrame.u32PicStride[0], input_height,
                                            picFormat, (void *)stFrame.stVFrame.u64VirAddr[0]);

        if (!bLoopEncode && (readSize <= 0))
            SAMPLE_LOG_WARN("Warning: read frame size : %d less than %d\n", readSize, frameSize);

        if (feof(fFileIn)) {
            if (bLoopEncode) {
                fseek(fFileIn, 0, SEEK_SET);
                if (pCmdl->stFbcInfo.fbcType)
                    readSize = SampleJpegOnceReadFbcFile(fFileIn, &stFrame.stVFrame, &pCmdl->stFbcInfo);
                else
                    readSize = COMMON_VENC_ReadFile(fFileIn, input_width, stFrame.stVFrame.u32PicStride[0],
                                                    input_height, picFormat, (void *)stFrame.stVFrame.u64VirAddr[0]);
            } else {
                SAMPLE_LOG_WARN("End of input file!\n");
                /* no more frames, stop encoder */
                goto EXIT;
            }
        }

        for (i = 0; i < 3; i++) {
            stJpegEncodeOnceParam.u64PhyAddr[i] = stFrame.stVFrame.u64PhyAddr[i];
            stJpegEncodeOnceParam.u32PicStride[i] = stFrame.stVFrame.u32PicStride[i];
            stJpegEncodeOnceParam.u32HeaderSize[i] = stFrame.stVFrame.u32HeaderSize[i];
        }

        s32Ret = AX_VENC_JpegEncodeOneFrame(&stJpegEncodeOnceParam);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_LOG_ERR("encode error, ret = %x.\n", s32Ret);
            s32Ret = -1;
            goto EXIT;
        }

        s32Ret = COMMON_VENC_SaveJpegFile(0, testId, totalEncodedFrames, stJpegEncodeOnceParam.pu8Addr,
                                          stJpegEncodeOnceParam.u32Len, pCmdl->bSaveStrm);
        if (AX_SUCCESS != s32Ret)
            SAMPLE_LOG_ERR("COMMON_VENC_SaveJpegFile err.\n");

        SAMPLE_LOG_DEBUG("phyAddr=%llx, vir_addr=%p, packetSize=%d.\n", stJpegEncodeOnceParam.ulPhyAddr,
                         stJpegEncodeOnceParam.pu8Addr, stJpegEncodeOnceParam.u32Len);

        totalEncodedFrames++;
        if (stJpegEncodeOnceParam.ulPhyAddr && (NULL != stJpegEncodeOnceParam.pu8Addr)) {
            AX_SYS_MemFree(stJpegEncodeOnceParam.ulPhyAddr, stJpegEncodeOnceParam.pu8Addr);
        }
    }

EXIT:
    if (stFrame.stVFrame.u64PhyAddr[0] && (0 != stFrame.stVFrame.u64VirAddr[0])) {
        AX_SYS_MemFree(stFrame.stVFrame.u64PhyAddr[0], (void *)stFrame.stVFrame.u64VirAddr[0]);
    }

    if (stJpegEncodeOnceParam.ulPhyAddr && (NULL != stJpegEncodeOnceParam.pu8Addr)) {
        AX_SYS_MemFree(stJpegEncodeOnceParam.ulPhyAddr, stJpegEncodeOnceParam.pu8Addr);
    }

    if (NULL != fFileIn) {
        fclose(fFileIn);
        fFileIn = NULL;
    }

    SAMPLE_LOG("Total encode %llu frames, Encoder exit!\n", totalEncodedFrames);

    return s32Ret;
}

AX_S32 UTestJpegStartEncOnce(SAMPLE_VENC_CMD_PARA_T *pCml)
{
    AX_S32 s32Ret = AX_SUCCESS;

    for (AX_S32 i = 0; i < MAX_ENCODE_ONCE_NUM; i++) {
        SampleJpegEncOnceFrmInit(i, &gstOnceFrmParam[i], pCml);
        s32Ret = JpegEncodeOneFrame(&gstOnceFrmParam[i]);
    }

    return s32Ret;
}
