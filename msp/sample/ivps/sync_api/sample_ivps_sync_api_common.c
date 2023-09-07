/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "sample_ivps_main.h"

/*
 * AX_IVPS_CmmCopy()
 * Function: Move a piece of memory data.
 * Note: When copy, the nMemSize should be 64K Byte aligned, 256M maximum.
 */
static AX_S32 AX_IVPS_CmmCopy(IVPS_ENGINE_ID_E eEngineId, AX_U64 nSrcPhyAddr, AX_U64 nDstPhyAddr, AX_U64 nMemSize)
{
    AX_S32 ret;

    switch (eEngineId)
    {
    case IVPS_ENGINE_ID_TDP:
        ret = AX_IVPS_CmmCopyTdp(nSrcPhyAddr, nDstPhyAddr, nMemSize);
        break;
    case IVPS_ENGINE_ID_VPP:
        ret = AX_IVPS_CmmCopyVpp(nSrcPhyAddr, nDstPhyAddr, nMemSize);
        break;
    case IVPS_ENGINE_ID_VGP:
        ret = AX_IVPS_CmmCopyVgp(nSrcPhyAddr, nDstPhyAddr, nMemSize);
        break;
    default:
        ret = -1;
        break;
    }

    if (ret)
    {
        ALOGE("AX_IVPS_CmmCopy fail, engine id:%d ret=0x%x", eEngineId, ret);
        return ret;
    }
    return 0;
}

/*
 * AX_IVPS_Csc()
 * Function: Color space conversion.
 * Note: Stride should be 16 Byte aligned.
 *       The u64PhyAddr[0] of ptDst should be set. If format is AX_YUV420_SEMIPLANAR.
 */
static AX_S32 AX_IVPS_Csc(IVPS_ENGINE_ID_E eEngineId, const AX_VIDEO_FRAME_T *ptSrc, AX_VIDEO_FRAME_T *ptDst)
{
    AX_S32 ret;

    switch (eEngineId)
    {
    case IVPS_ENGINE_ID_TDP:
        ret = AX_IVPS_CscTdp(ptSrc, ptDst);
        break;
    case IVPS_ENGINE_ID_VPP:
        ret = AX_IVPS_CscVpp(ptSrc, ptDst);
        break;
    case IVPS_ENGINE_ID_VGP:
        ret = AX_IVPS_CscVgp(ptSrc, ptDst);
        break;
    default:
        ret = -1;
        break;
    }

    if (ret)
    {
        ALOGE("AX_IVPS_Csc fail, engine id:%d ret=0x%x", eEngineId, ret);
        return ret;
    }
    return 0;
}

/*
 * AX_IVPS_CropResize
 * Function: Crop and Resize.
 * Note: Stride should be 16 Byte aligned.
 *       The u64PhyAddr[0] of ptDst should be set.
 *       If crop is enabled, s16OffsetTop/s16OffsetBottom/s16OffsetRight/s16OffsetLeft should be set.
 */
static AX_S32 AX_IVPS_CropResize(IVPS_ENGINE_ID_E eEngineId, const AX_VIDEO_FRAME_T *ptSrc,
                                 AX_VIDEO_FRAME_T *ptDst, const AX_IVPS_ASPECT_RATIO_T *ptAspectRatio)
{
    AX_S32 ret;

    switch (eEngineId)
    {
    case IVPS_ENGINE_ID_TDP:
        ret = AX_IVPS_CropResizeTdp(ptSrc, ptDst, ptAspectRatio);
        break;
    case IVPS_ENGINE_ID_VPP:
        ret = AX_IVPS_CropResizeVpp(ptSrc, ptDst, ptAspectRatio);
        break;
    case IVPS_ENGINE_ID_VGP:
        ret = AX_IVPS_CropResizeVgp(ptSrc, ptDst, ptAspectRatio);
        break;
    default:
        ret = -1;
        break;
    }

    if (ret)
    {
        ALOGE("AX_IVPS_CropResize fail, engine id:%d ret=0x%x", eEngineId, ret);
        return ret;
    }
    return 0;
}

/*
 * AX_IVPS_CropResizeV2
 * Function: Crop and Resize.
 * Note: Stride should be 16 Byte aligned.
 *       The u64PhyAddr[0] of ptDst should be set.
 *       If crop is enabled, s16OffsetTop/s16OffsetBottom/s16OffsetRight/s16OffsetLeft should be set.
 */
static AX_S32 AX_IVPS_CropResizeV2(IVPS_ENGINE_ID_E eEngineId, const AX_VIDEO_FRAME_T *ptSrc, const AX_IVPS_RECT_T tBox[], AX_U32 nCropNum,
                                   AX_VIDEO_FRAME_T *ptDst[], const AX_IVPS_ASPECT_RATIO_T *ptAspectRatio)
{
    AX_S32 ret;

    switch (eEngineId)
    {
    case IVPS_ENGINE_ID_TDP:
        ret = AX_IVPS_CropResizeV2Tdp(ptSrc, tBox, nCropNum, ptDst, ptAspectRatio);
        break;
    case IVPS_ENGINE_ID_VPP:
        ret = AX_IVPS_CropResizeV2Vpp(ptSrc, tBox, nCropNum, ptDst, ptAspectRatio);
        break;
    case IVPS_ENGINE_ID_VGP:
        ret = AX_IVPS_CropResizeV2Vgp(ptSrc, tBox, nCropNum, ptDst, ptAspectRatio);
        break;
    default:
        ret = -1;
        break;
    }

    if (ret)
    {
        ALOGE("AX_IVPS_CropResizeV2 fail, engine id:%d ret=0x%x", eEngineId, ret);
        return ret;
    }
    return 0;
}

/*
 * AX_IVPS_AlphaBlending
 * Function: Overlay two images.
 * Note: Stride should be 16 Byte aligned.
 *       The u64PhyAddr[0] of ptDst should be set.
 */
static AX_S32 AX_IVPS_AlphaBlending(IVPS_ENGINE_ID_E eEngineId,
                                    const AX_VIDEO_FRAME_T *ptSrc,
                                    const AX_VIDEO_FRAME_T *ptOverlay,
                                    const AX_IVPS_POINT_T tOffset, AX_U8 nAlpha,
                                    AX_VIDEO_FRAME_T *ptDst)
{
    AX_S32 ret;

    switch (eEngineId)
    {
    case IVPS_ENGINE_ID_TDP:
        ret = AX_IVPS_AlphaBlendingTdp(ptSrc, ptOverlay, tOffset, nAlpha, ptDst);
        break;
    case IVPS_ENGINE_ID_VGP:
        ret = AX_IVPS_AlphaBlendingVgp(ptSrc, ptOverlay, tOffset, nAlpha, ptDst);
        break;
    default:
        ret = -1;
        break;
    }

    if (ret)
    {
        ALOGE("AX_IVPS_AlphaBlending fail, engine id:%d ret=0x%x", eEngineId, ret);
        return ret;
    }
    return 0;
}

/*
 * AX_IVPS_AlphaBlendingV3
 * Function: Overlay two images.
 * Note: Stride should be 16 Byte aligned.
 *       The u64PhyAddr[0] of ptDst should be set.
 */
static AX_S32 AX_IVPS_AlphaBlendingV3(IVPS_ENGINE_ID_E eEngineId, const AX_VIDEO_FRAME_T *ptSrc,
                                      const AX_OVERLAY_T *ptOverlay, AX_VIDEO_FRAME_T *ptDst)
{
    AX_S32 ret;

    switch (eEngineId)
    {
    case IVPS_ENGINE_ID_TDP:
        ret = AX_IVPS_AlphaBlendingV3Tdp(ptSrc, ptOverlay, ptDst);
        break;
    case IVPS_ENGINE_ID_VGP:
        ret = AX_IVPS_AlphaBlendingV3Vgp(ptSrc, ptOverlay, ptDst);
        break;
    default:
        ret = -1;
        break;
    }

    if (ret)
    {
        ALOGE("AX_IVPS_AlphaBlendingV3 fail, engine id:%d ret=0x%x", eEngineId, ret);
        return ret;
    }
    return 0;
}

/*
 * SAMPLE_IVPS_Mosaic()
 * Function: Draw mosaic and save output file.
 * Note: Stride should be 16 Byte aligned.
 *       Draw up to 32 mosaics at once.
 */
static AX_S32 AX_IVPS_DrawMosaic(IVPS_ENGINE_ID_E eEngineId, const AX_VIDEO_FRAME_T *ptSrc, AX_IVPS_RGN_MOSAIC_T tMosaic[], AX_U32 nNum)
{
    AX_S32 ret;
    switch (eEngineId)
    {
    case IVPS_ENGINE_ID_VPP:
        ret = AX_IVPS_DrawMosaicVpp(ptSrc, tMosaic, nNum);
        break;
    case IVPS_ENGINE_ID_VGP:
        ret = AX_IVPS_DrawMosaicVgp(ptSrc, tMosaic, nNum);
        break;
    case IVPS_ENGINE_ID_TDP:
        ret = AX_IVPS_DrawMosaicTdp(ptSrc, tMosaic, nNum);
        break;
    default:
        ret = -1;
        break;
    }

    if (ret)
    {
        ALOGE("AX_IVPS_DrawMosaic fail, engine id:%d ret=0x%x", eEngineId, ret);
        return ret;
    }
    return 0;
}

/*
 * SAMPLE_IVPS_CmmCopy()
 * Function: Move a piece of memory data and save output file.
 * Note: When copy, the nMemSize should be 64K Byte aligned, 256M maximum.
 */
AX_S32 SAMPLE_IVPS_CmmCopy(IVPS_ENGINE_ID_E eEngineId, const AX_VIDEO_FRAME_T *ptSrcFrame, char *strFilePath)
{
    AX_S32 ret = 0;
    AX_VIDEO_FRAME_T tDstFrame = {0};
    AX_BLK BlkId;
    AX_U32 nImgSize = ptSrcFrame->u32Height * ptSrcFrame->u32PicStride[0] * 3; /* assume format is rgb888 */

    /* nImgSize should be aligned according to the limitations of each module */
    nImgSize = ALIGN_UP(nImgSize, 65536);
    ALOGI("nImgSize=%d\n", nImgSize);

    CHECK_RESULT(BufPoolBlockAddrGet(-1, nImgSize, &tDstFrame.u64PhyAddr[0],
                                     (AX_VOID **)&tDstFrame.u64VirAddr[0], &BlkId));
    ALOGI("src=%llx dst=%llx", ptSrcFrame->u64PhyAddr[0], tDstFrame.u64PhyAddr[0]);
    ret = AX_IVPS_CmmCopy(eEngineId, ptSrcFrame->u64PhyAddr[0], tDstFrame.u64PhyAddr[0], nImgSize);
    if (ret)
    {
        ALOGE("AX_IVPS_CmmCopy fail ! ret=0x%x", ret);
        return ret;
    }
    ALOGI("src=%llx dst=%llx", ptSrcFrame->u64PhyAddr[0], tDstFrame.u64PhyAddr[0]);

    tDstFrame.enImgFormat = ptSrcFrame->enImgFormat;
    tDstFrame.u32PicStride[0] = ptSrcFrame->u32PicStride[0];
    tDstFrame.u32Height = ptSrcFrame->u32Height;
    tDstFrame.u32FrameSize = nImgSize;
    /* sprintf(strFileName, "CmmCopy_%d_", eEngineId); */
    SaveFile(&tDstFrame, 0, 0, strFilePath, "CmmCopy_");
    ret = AX_POOL_ReleaseBlock(BlkId);
    if (ret)
    {
        ALOGE("Rls BlkId fail, ret=0x%x", ret);
    }

    return ret;
}

/*
 * SAMPLE_Pyra_Gen()
 * Function: pyralite gen test code.
 */
AX_S32 SAMPLE_Pyra_Gen(const AX_VIDEO_FRAME_T *ptSrcFrame, char *strFilePath, AX_BOOL bPyraMode)
{
    AX_S32 ret = 0;
    AX_S32 i;
    char strFileName[30] = {0};
    AX_VIDEO_FRAME_T tDstFrame[2] = {0};
    AX_PYRA_FRAME_T tSrcFrame = {0};
    AX_PYRA_FRAME_T tPyraDstFrame[2] = {0};
    AX_BLK BlkId[2];
    AX_U32 nImgSize;
    if (bPyraMode) {
        nImgSize = ptSrcFrame->u32Height * ptSrcFrame->u32PicStride[0] / 4;
    } else {
        nImgSize = ptSrcFrame->u32Height * ptSrcFrame->u32PicStride[0] * 1.5 / 4 * 2;
    }

    tSrcFrame.bEnable = 1;
    tSrcFrame.nWidth = ptSrcFrame->u32Width;
    tSrcFrame.nHeight = ptSrcFrame->u32Height;
    tSrcFrame.nStride = ptSrcFrame->u32PicStride[0];
    tSrcFrame.eFormat = ptSrcFrame->enImgFormat;
    tSrcFrame.nPhyAddr[0] = ptSrcFrame->u64PhyAddr[0];
    if (!bPyraMode) {
        if (ptSrcFrame->enImgFormat == AX_FORMAT_YUV420_SEMIPLANAR) {
                tSrcFrame.nPhyAddr[1] = ptSrcFrame->u64PhyAddr[0] + ptSrcFrame->u32Width * ptSrcFrame->u32PicStride[0];
        } else {
                tSrcFrame.nPhyAddr[1] = ptSrcFrame->u64PhyAddr[0] + ptSrcFrame->u32Width * ptSrcFrame->u32PicStride[0] * 2;
        }
    }

    ALOGI("nImgSize=%d bPyraMode:%d\n", nImgSize, bPyraMode);
    CHECK_RESULT(BufPoolBlockAddrGet(-1, nImgSize, &tDstFrame[0].u64PhyAddr[0],(AX_VOID **)&tDstFrame[0].u64VirAddr[0], &BlkId[0]));
    tPyraDstFrame[0].nWidth = ptSrcFrame->u32Width / 2;
    tPyraDstFrame[0].nHeight = ptSrcFrame->u32Height / 2;
    tPyraDstFrame[0].nStride = ptSrcFrame->u32PicStride[0] / 2;
    if (bPyraMode) {
        tPyraDstFrame[0].eFormat = ptSrcFrame->enImgFormat;
    } else {
        tPyraDstFrame[0].eFormat = AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P010;
    }
    tPyraDstFrame[0].nPhyAddr[0] = tDstFrame[0].u64PhyAddr[0];
    tPyraDstFrame[0].nPhyAddr[1] = tDstFrame[0].u64PhyAddr[0] + tPyraDstFrame[0].nHeight * tPyraDstFrame[0].nStride * 2;
    ALOGI("nHeight=%d nStride=%d", tPyraDstFrame[0].nHeight, tPyraDstFrame[0].nStride);
    ALOGI("dst0=%llx dst1=%llx", tPyraDstFrame[0].nPhyAddr[0], tPyraDstFrame[0].nPhyAddr[1]);

    CHECK_RESULT(BufPoolBlockAddrGet(-1, nImgSize * 4 , &tDstFrame[1].u64PhyAddr[0],(AX_VOID **)&tDstFrame[1].u64VirAddr[0], &BlkId[1]));
    tPyraDstFrame[1].nWidth = ptSrcFrame->u32Width;
    tPyraDstFrame[1].nHeight = ptSrcFrame->u32Height;
    tPyraDstFrame[1].nStride = ptSrcFrame->u32PicStride[0];
    tPyraDstFrame[1].nPhyAddr[0] = tDstFrame[1].u64PhyAddr[0];
    if (bPyraMode) {
        tPyraDstFrame[1].eFormat = ptSrcFrame->enImgFormat;
    } else {
        tPyraDstFrame[1].eFormat = AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P010;
    }
    tPyraDstFrame[1].nPhyAddr[1] = tDstFrame[1].u64PhyAddr[0] + tPyraDstFrame[1].nHeight * tPyraDstFrame[1].nStride * 2;
    ALOGI("nHeight=%d nStride=%d", tPyraDstFrame[1].nHeight, tPyraDstFrame[1].nStride);
    ALOGI("dst0=%llx dst1=%llx", tPyraDstFrame[1].nPhyAddr[0], tPyraDstFrame[1].nPhyAddr[1]);
    ALOGI("src=%llx src1=%llx", tSrcFrame.nPhyAddr[0], tSrcFrame.nPhyAddr[1]);

    ret = AX_PyraLite_Gen(&tSrcFrame, &tPyraDstFrame[0], bPyraMode);
    if (ret)
    {
        ALOGE("AX_PyraLite_Gen Test fail ! ret=0x%x", ret);
        return ret;
    }

    tDstFrame[0].u32FrameSize = nImgSize;
    tDstFrame[1].u32FrameSize = nImgSize * 4;

    for (i = 0; i < 2; i++) {
        if (bPyraMode) {
                tDstFrame[i].enImgFormat = ptSrcFrame->enImgFormat;
        } else {
                tDstFrame[i].enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR_10BIT_P010;
        }
        tDstFrame[i].u32PicStride[0] = tPyraDstFrame[i].nStride;
        tDstFrame[i].u32Height = tPyraDstFrame[i].nHeight;
        tDstFrame[i].u32Width = tPyraDstFrame[i].nWidth;
        tDstFrame[i].u64PhyAddr[1] = tPyraDstFrame[i].nPhyAddr[1];
        ALOGI("nHeight=%d nStride=%d  w:%d", tPyraDstFrame[i].nHeight, tPyraDstFrame[i].nStride, tDstFrame[i].u32Height);
        sprintf(strFileName, "pyralite_gen%d_", i);
        SaveFile(&tDstFrame[i], 0, 0, strFilePath, strFileName);
        ret = AX_POOL_ReleaseBlock(BlkId[i]);
        if (ret)
        {
                ALOGE("Rls BlkId fail, ret=0x%x", ret);
        }
     }

    return ret;
}

/*
 * SAMPLE_Pyra_Rcn()
 * Function: pyralite rcn test code.
 */
AX_S32 SAMPLE_Pyra_Rcn(const AX_VIDEO_FRAME_T *ptGauFrame, AX_VIDEO_FRAME_T *ptLapFrame, char *strFilePath, AX_BOOL bPyraMode)
{
    AX_S32 ret = 0;
    int i = 0;
    AX_VIDEO_FRAME_T tDstFrame = {0};
    AX_PYRA_FRAME_T tSrcFrame[5] = {0};
    AX_PYRA_FRAME_T tPyraDstFrame = {0};
    AX_BLK BlkId;
    AX_U32 nImgSize;
    ALOGI("bottom%d", bPyraMode);

    tSrcFrame[0].bEnable = 1;
    tSrcFrame[0].nWidth = ptGauFrame->u32Width;
    tSrcFrame[0].nHeight = ptGauFrame->u32Height;
    tSrcFrame[0].nStride = ptGauFrame->u32PicStride[0];
    tSrcFrame[0].eFormat = ptGauFrame->enImgFormat;
    tSrcFrame[0].nPhyAddr[0] = ptGauFrame->u64PhyAddr[0];
    if (bPyraMode) {
        tSrcFrame[0].nPhyAddr[1] = ptGauFrame->u64PhyAddr[0] + tSrcFrame[0].nHeight * ptGauFrame->u32PicStride[0] * 2;
    } else {
        tSrcFrame[0].nPhyAddr[1] = ptGauFrame->u64PhyAddr[0] + tSrcFrame[0].nHeight * ptGauFrame->u32PicStride[0];
    }
    ALOGI("num【%d】Width=%d nHeight=%d", 0,  tSrcFrame[0].nWidth, tSrcFrame[0].nHeight);
    ALOGI("num【%d】src=%llx src1=%llx", 0,  tSrcFrame[0].nPhyAddr[0], tSrcFrame[0].nPhyAddr[1]);
    for (i = 1; i < 5; i++) {
        tSrcFrame[i].bEnable = 1;
        tSrcFrame[i].nWidth = ptLapFrame[i-1].u32Width;
        tSrcFrame[i].nHeight = ptLapFrame[i-1].u32Height;
        tSrcFrame[i].nStride = ptLapFrame[i-1].u32PicStride[0];
        tSrcFrame[i].eFormat = ptLapFrame[i-1].enImgFormat;
        tSrcFrame[i].nPhyAddr[0] = ptLapFrame[i-1].u64PhyAddr[0];
        if (i > 2) {
            tSrcFrame[i].nPhyAddr[1] = ptLapFrame[i-1].u64PhyAddr[0] + tSrcFrame[i].nHeight * tSrcFrame[i].nStride;
        } else {
            tSrcFrame[i].nPhyAddr[1] = ptLapFrame[i-1].u64PhyAddr[0] + tSrcFrame[i].nHeight * tSrcFrame[i].nStride * 2;
        }
        ALOGI("num【%d】Width=%d nHeight=%d", i,  tSrcFrame[i].nWidth, tSrcFrame[i].nHeight);
        ALOGI("num【%d】src=%llx src1=%llx", i,  tSrcFrame[i].nPhyAddr[0], tSrcFrame[i].nPhyAddr[1]);
    }

    if (tSrcFrame[3].nWidth != tSrcFrame[1].nWidth) {
        tSrcFrame[3].bMaskScaler = AX_TRUE;
        tSrcFrame[3].nScalerRatio = tSrcFrame[3].nWidth / tSrcFrame[1].nWidth - 1;
    }
    ALOGI("mask up【%d】nScalerRatio=% d", tSrcFrame[3].bMaskScaler, tSrcFrame[3].nScalerRatio);

    if (bPyraMode) {
        nImgSize = ptGauFrame->u32Height * ptGauFrame->u32PicStride[0] * 1.5;
    } else {
        nImgSize = ptGauFrame->u32Height * ptGauFrame->u32PicStride[0] * 4 * 1.5;
    }
    ALOGI("nImgSize=%d\n", nImgSize);
    CHECK_RESULT(BufPoolBlockAddrGet(-1, nImgSize, &tDstFrame.u64PhyAddr[0], (AX_VOID **)&tDstFrame.u64VirAddr[0], &BlkId));
    ALOGI("src=%llx src1=%llx", ptGauFrame->u64PhyAddr[0], ptGauFrame->u64PhyAddr[1]);
    for ( i = 0; i < 4; i++) {
        ALOGI("num【%d】src=%llx src1=%llx", i, ptLapFrame[i].u64PhyAddr[0], ptLapFrame[i].u64PhyAddr[1]);
    }
    ALOGI("dst0=%llx dst1=%llx", tDstFrame.u64PhyAddr[0], tDstFrame.u64PhyAddr[1]);

    tPyraDstFrame.bEnable = 1;
    if (bPyraMode) {
        tDstFrame.u64PhyAddr[1] = tDstFrame.u64PhyAddr[0] + ptGauFrame->u32Width * ptGauFrame->u32PicStride[0];
        tPyraDstFrame.nWidth = ptGauFrame->u32Width;
        tPyraDstFrame.nHeight = ptGauFrame->u32Height;
        tPyraDstFrame.nStride = ptGauFrame->u32PicStride[0];
    } else {
        tPyraDstFrame.nWidth = ptGauFrame->u32Width * 2;
        tPyraDstFrame.nHeight = ptGauFrame->u32Height * 2;
        tPyraDstFrame.nStride = ptGauFrame->u32PicStride[0] * 2;
        tDstFrame.u64PhyAddr[1] = tDstFrame.u64PhyAddr[0] + tPyraDstFrame.nStride * tPyraDstFrame.nHeight;
    }
    ALOGI("dst w=%d h=%d  s:%d", tPyraDstFrame.nWidth,tPyraDstFrame.nHeight, tPyraDstFrame.nStride);

    tPyraDstFrame.nPhyAddr[0] = tDstFrame.u64PhyAddr[0];
    tPyraDstFrame.nPhyAddr[1] = tDstFrame.u64PhyAddr[1];
    tPyraDstFrame.eFormat = AX_FORMAT_YUV420_SEMIPLANAR;

    ALOGI("src=%llx src1=%llx", tSrcFrame[0].nPhyAddr[0], tSrcFrame[0].nPhyAddr[1]);
    ALOGI("src=%llx src1=%llx", tSrcFrame[1].nPhyAddr[0], tSrcFrame[1].nPhyAddr[1]);
    ALOGI("dst0=%llx dst1=%llx", tPyraDstFrame.nPhyAddr[0], tPyraDstFrame.nPhyAddr[1]);

    ret = AX_PyraLite_Rcn(&tSrcFrame[0], &tPyraDstFrame, bPyraMode);
    if (ret)
    {
        ALOGE("AX_PyraLite_Test fail ! ret=0x%x", ret);
        return ret;
    }
    tDstFrame.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;

    if (bPyraMode) {
        tDstFrame.u32PicStride[0] = ptGauFrame->u32PicStride[0];
        tDstFrame.u32Height = ptGauFrame->u32Height;
        tDstFrame.u32FrameSize = nImgSize;
    } else {
        tDstFrame.u32PicStride[0] = ptGauFrame->u32PicStride[0] * 2;
        tDstFrame.u32Height = ptGauFrame->u32Height * 2;
        tDstFrame.u32FrameSize = nImgSize;
        tDstFrame.u32Width = ptGauFrame->u32Width * 2;
    }
    SaveFile(&tDstFrame, 0, 0, strFilePath, "pyralite_rcn_");
    ret = AX_POOL_ReleaseBlock(BlkId);
    if (ret)
    {
        ALOGE("Rls BlkId fail, ret=0x%x", ret);
    }

    return ret;
}

/*
 * SAMPLE_IVPS_Csc()
 * Function: Color space conversion and save output file.
 * Note: Stride should be 16 Byte aligned.
 *       The u64PhyAddr[0] of ptDst should be set. If format is AX_YUV420_SEMIPLANAR.
 */
AX_S32 SAMPLE_IVPS_Csc(IVPS_ENGINE_ID_E eEngineId, const AX_VIDEO_FRAME_T *ptSrcFrame, char *strFilePath)
{
    AX_S32 ret = 0;
    AX_VIDEO_FRAME_T tDstFrame = {0};
    AX_BLK BlkId;
    AX_U32 nImgSize;

    ALOGI("CSC input u32Width =%d Height:%d", ptSrcFrame->u32Width, ptSrcFrame->u32Height);
    ALOGI("CSC input format =%d ", ptSrcFrame->enImgFormat);

    tDstFrame.enImgFormat = AX_FORMAT_RGB888;
    tDstFrame.u32Height = ptSrcFrame->u32Height;
    tDstFrame.u32Width = ptSrcFrame->u32Width;
    tDstFrame.u32PicStride[0] = ptSrcFrame->u32PicStride[0];
    tDstFrame.stCompressInfo.enCompressMode = AX_COMPRESS_MODE_NONE; /*AX_COMPRESS_MODE_LOSSLESS*/
    nImgSize = CalcImgSize(ptSrcFrame->u32PicStride[0], ptSrcFrame->u32Width,
                           ptSrcFrame->u32Height, ptSrcFrame->enImgFormat, 16);
    ALOGI("CSC nImgSize =%d", nImgSize);
    ALOGI("CSC ptSrcFrame->u32PicStride[0]=%d", ptSrcFrame->u32PicStride[0]);
    if (tDstFrame.stCompressInfo.enCompressMode)
    {
        /* If enable compress, add header information to the address of Y and UV.
        Y headers size = H/2*128
        UV header size = H/2*64 */
        nImgSize = nImgSize + DIV_ROUND_UP(ptSrcFrame->u32Height, 2) * 64 * 3;
    }
    CHECK_RESULT(BufPoolBlockAddrGet(-1, nImgSize, &tDstFrame.u64PhyAddr[0],
                                     (AX_VOID **)&tDstFrame.u64VirAddr[0], &BlkId));

    ALOGI("tDstFrame.u64PhyAddr[1] =0x%llx", tDstFrame.u64PhyAddr[1]);
    ALOGI("CSC ptSrcFrame->u32PicStride[0] =%d  ptSrcFrame->u32Height:%d", ptSrcFrame->u32PicStride[0], ptSrcFrame->u32Height);
    tDstFrame.u64PhyAddr[1] = tDstFrame.u64PhyAddr[0] + ptSrcFrame->u32PicStride[0] * ptSrcFrame->u32Height; // for dstFormat nv12
    ALOGI("tDstFrame.u64PhyAddr[1] =0x%llx", tDstFrame.u64PhyAddr[1]);

    if (tDstFrame.stCompressInfo.enCompressMode)
    {
        tDstFrame.u64PhyAddr[0] += DIV_ROUND_UP(ptSrcFrame->u32Height, 2) * 64 * 2;
        tDstFrame.u64PhyAddr[1] += DIV_ROUND_UP(ptSrcFrame->u32Height, 2) * 64 * 3;
    }

    ALOGI("AX_IVPS_CscTdp src=%llx dst=%llx", ptSrcFrame->u64PhyAddr[0], tDstFrame.u64PhyAddr[0]);
    ALOGI("AX_IVPS_CscTdp src=%llx dst=%llx", ptSrcFrame->u64PhyAddr[1], tDstFrame.u64PhyAddr[1]);
    ret = AX_IVPS_Csc(eEngineId, ptSrcFrame, &tDstFrame);
    if (ret)
    {
        ALOGE("ret=0x%x", ret);
        return ret;
    }

    tDstFrame.u32PicStride[0] = ptSrcFrame->u32PicStride[0];
    tDstFrame.u32Height = ptSrcFrame->u32Height;
    SaveFile(&tDstFrame, 0, 0, strFilePath, "CSC_");
    ret = AX_POOL_ReleaseBlock(BlkId);
    if (ret)
    {
        ALOGE("Rls BlkId fail, ret=0x%x", ret);
    }
    return ret;
}

/*
 * SAMPLE_IVPS_FlipAndRotation
 * Function: Flip/Mirror/Rotate 0/90/180/270 and save output file.
 * Note: Stride and width should be 16 Byte aligned.
 *       The u64PhyAddr[0] of ptDst should be set. If format is AX_YUV420_SEMIPLANAR, u64PhyAddr[1] should be set.
 */
AX_S32 SAMPLE_IVPS_FlipAndRotation(const AX_VIDEO_FRAME_T *ptSrcFrame,
                                   AX_S32 nFlipCode, AX_S32 nRotation, char *strFilePath)
{
    AX_S32 ret = 0;
    AX_VIDEO_FRAME_T tDstFrame = {0};
    AX_BLK BlkId;
    AX_U32 nImgSize;

    ALOGI("Rotate u32Width =%d", ptSrcFrame->u32Width);
    tDstFrame.enImgFormat = ptSrcFrame->enImgFormat;
    tDstFrame.u32Height = ptSrcFrame->u32Width;
    tDstFrame.u32Width = ptSrcFrame->u32Height;
    tDstFrame.u32PicStride[0] = ptSrcFrame->u32Height;
    nImgSize = CalcImgSize(ptSrcFrame->u32PicStride[0], ptSrcFrame->u32Width,
                           ALIGN_UP(ptSrcFrame->u32Height, 64), ptSrcFrame->enImgFormat, 16);
    CHECK_RESULT(BufPoolBlockAddrGet(-1, nImgSize, &tDstFrame.u64PhyAddr[0], (AX_VOID **)&tDstFrame.u64VirAddr[0], &BlkId));
    tDstFrame.u64PhyAddr[1] = tDstFrame.u64PhyAddr[0] + ptSrcFrame->u32PicStride[0] * ALIGN_UP(ptSrcFrame->u32Height, 64);

    ret = AX_IVPS_FlipAndRotationTdp(ptSrcFrame, 1, AX_IVPS_ROTATION_90, &tDstFrame);
    if (ret)
    {
        ALOGE("ret=0x%x", ret);
        return ret;
    }

    SaveFile(&tDstFrame, 0, 0, strFilePath, "FlipRotate_");
    ret = AX_POOL_ReleaseBlock(BlkId);
    if (ret)
    {
        ALOGE("Rls BlkId fail, ret=0x%x", ret);
    }
    return ret;
}

/*
 * SAMPLE_IVPS_AlphaBlendingV3
 * Function: Overlay two images and save output file.
 * Note: Stride should be 16 Byte aligned.
 *       The u64PhyAddr[0] of ptDst should be set.
 */
AX_S32 SAMPLE_IVPS_AlphaBlendingV3(IVPS_ENGINE_ID_E eEngineId, const AX_VIDEO_FRAME_T *ptSrcFrame,
                                 AX_VIDEO_FRAME_T *ptOverlay, AX_U8 nAlpha, char *strFilePath)
{

    AX_S32 ret = 0;
    AX_OVERLAY_T tOverlayV3 = {0};
    AX_VIDEO_FRAME_T tDstFrame = {0};
    AX_BLK BlkId;
    AX_U32 nImgSize;

    tDstFrame.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
    tDstFrame.u32Height = 1080;
    tDstFrame.u32Width = 1920;
    tDstFrame.u32PicStride[0] = 1920;

    nImgSize = CalcImgSize(tDstFrame.u32PicStride[0], tDstFrame.u32Width,
                           tDstFrame.u32Height, tDstFrame.enImgFormat, 16);
    tDstFrame.u32FrameSize = nImgSize;
    CHECK_RESULT(BufPoolBlockAddrGet(-1, nImgSize, &tDstFrame.u64PhyAddr[0],
                                     (AX_VOID **)(&tDstFrame.u64VirAddr[0]), &BlkId));
    tDstFrame.u64PhyAddr[1] = tDstFrame.u64PhyAddr[0] + tDstFrame.u32PicStride[0] * tDstFrame.u32Height;

    tOverlayV3.bEnable = AX_TRUE;
    tOverlayV3.nWidth = ptOverlay->u32Width;
    tOverlayV3.nHeight = ptOverlay->u32Height;
    tOverlayV3.nStride = ptOverlay->u32PicStride[0];
    tOverlayV3.eFormat = ptOverlay->enImgFormat;
    tOverlayV3.u64PhyAddr[0] = ptOverlay->u64PhyAddr[0];
    tOverlayV3.nAlpha = 128;
    tOverlayV3.tOffset.nX = 0;
    tOverlayV3.tOffset.nY = 0;
    tOverlayV3.tColorKey.u16Enable = AX_TRUE;
    tOverlayV3.tColorKey.u16Inv = 0;
    tOverlayV3.tColorKey.u32KeyHigh = 0xffffff;
    tOverlayV3.tColorKey.u32KeyLow = 0x000000;
    ret = AX_IVPS_AlphaBlendingV3(eEngineId, ptSrcFrame, &tOverlayV3, &tDstFrame);
    if (ret)
    {
        ALOGE("AX_IVPS_AlphaBlendingV3 failed, ret=0x%x.", ret);
         return ret;
    }
    SaveFile(&tDstFrame, 0, 0, strFilePath, "AlphaBlend3_");
    ret = AX_POOL_ReleaseBlock(BlkId);
    if (ret)
    {
            ALOGE("Rls BlkId fail, ret=0x%x", ret);
    }
    return ret;
}


/*
 * SAMPLE_IVPS_AlphaBlending
 * Function: Overlay two images and save output file.
 * Note: Stride should be 16 Byte aligned.
 *       The u64PhyAddr[0] of ptDst should be set.
 */
AX_S32 SAMPLE_IVPS_AlphaBlending(IVPS_ENGINE_ID_E eEngineId, const AX_VIDEO_FRAME_T *ptSrcFrame,
                                 AX_VIDEO_FRAME_T *ptOverlay, AX_U8 nAlpha, char *strFilePath)
{

    AX_S32 ret = 0;
    AX_VIDEO_FRAME_T tDstFrame = {0};
    AX_BLK BlkId;
    AX_U32 nImgSize;
    AX_IVPS_POINT_T tOffset;

    tOffset.nX = 128;
    tOffset.nY = 80;
    tDstFrame.enImgFormat = ptSrcFrame->enImgFormat;
    tDstFrame.u32Height = ptSrcFrame->u32Height;
    tDstFrame.u32Width = ptSrcFrame->u32Width;
    tDstFrame.u32PicStride[0] = ptSrcFrame->u32PicStride[0];
    nImgSize = CalcImgSize(ptSrcFrame->u32PicStride[0], ptSrcFrame->u32Width,
                           ptSrcFrame->u32Height, ptSrcFrame->enImgFormat, 16);
    CHECK_RESULT(BufPoolBlockAddrGet(-1, nImgSize, &tDstFrame.u64PhyAddr[0],
                                     (AX_VOID **)(&tDstFrame.u64VirAddr[0]), &BlkId));

    tDstFrame.u64PhyAddr[1] = tDstFrame.u64PhyAddr[0] + ptSrcFrame->u32PicStride[0] * ptSrcFrame->u32Height;

    ret = AX_IVPS_AlphaBlending(eEngineId, ptSrcFrame, ptOverlay, tOffset, nAlpha, &tDstFrame);
    if (ret)
    {
        ALOGE("ret=0x%x", ret);
        return ret;
    }

    SaveFile(&tDstFrame, 0, 0, strFilePath, "AlphaBlend_");
    ret = AX_POOL_ReleaseBlock(BlkId);
    if (ret)
    {
        ALOGE("Rls BlkId fail, ret=0x%x", ret);
    }
    return ret;
}

/*
 * SAMPLE_IVPS_AlphaBlendingV2
 * Function: Overlay two images and save output file.
 * Note: Stride should be 16 Byte aligned.
 *       The u64PhyAddr[0] of ptDst should be set.
 */
AX_S32 SAMPLE_IVPS_AlphaBlendingV2(const AX_VIDEO_FRAME_T *ptSrcFrame, AX_VIDEO_FRAME_T *ptOverlay,
                                   const AX_IVPS_ALPHA_LUT_T *ptSpAlpha, char *strFilePath)
{

    AX_S32 ret = 0;
    AX_VIDEO_FRAME_T tDstFrame = {0};
    AX_BLK BlkId;
    AX_U32 nImgSize;
    AX_IVPS_POINT_T tOffset;

    tOffset.nX = 0;
    tOffset.nY = 0;
    tDstFrame.enImgFormat = ptSrcFrame->enImgFormat;
    tDstFrame.u32Height = ptSrcFrame->u32Height;
    tDstFrame.u32Width = ptSrcFrame->u32Width;
    tDstFrame.u32PicStride[0] = ptSrcFrame->u32PicStride[0];
    nImgSize = CalcImgSize(ptSrcFrame->u32PicStride[0], ptSrcFrame->u32Width,
                           ptSrcFrame->u32Height, ptSrcFrame->enImgFormat, 16);
    CHECK_RESULT(BufPoolBlockAddrGet(-1, nImgSize, &tDstFrame.u64PhyAddr[0],
                                     (AX_VOID **)(&tDstFrame.u64VirAddr[0]), &BlkId));

    tDstFrame.u64PhyAddr[1] = tDstFrame.u64PhyAddr[0] + ptSrcFrame->u32PicStride[0] * ptSrcFrame->u32Height;

    ret = AX_IVPS_AlphaBlendingV2Vgp(ptSrcFrame, ptOverlay, tOffset, ptSpAlpha, &tDstFrame);
    if (ret)
    {
        ALOGE("ret=0x%x", ret);
        return ret;
    }

    SaveFile(&tDstFrame, 0, 0, strFilePath, "AlphaBlendV2_");
    ret = AX_POOL_ReleaseBlock(BlkId);
    if (ret)
    {
        ALOGE("Rls BlkId fail, ret=0x%x", ret);
    }
    return ret;
}

/*
 * SAMPLE_IVPS_CropResize
 * Function: Crop and Resize and save output file.
 * Note: Stride should be 16 Byte aligned.
 *       The u64PhyAddr[0] of ptDst should be set.
 *       If crop is enabled, s16OffsetTop/s16OffsetBottom/s16OffsetRight/s16OffsetLeft should be set.
 */
AX_S32 SAMPLE_IVPS_CropResize(IVPS_ENGINE_ID_E eEngineId, const AX_VIDEO_FRAME_T *ptSrcFrame, char *strFilePath)
{

    AX_S32 ret = 0;
    AX_VIDEO_FRAME_T tDstFrame = {0};
    AX_BLK BlkId;
    AX_U32 nImgSize;
    AX_IVPS_ASPECT_RATIO_T tAspectRatio;
    AX_U32 WidthTemp, HeightTemp;

    tAspectRatio.eMode = AX_IVPS_ASPECT_RATIO_STRETCH;
    tAspectRatio.eAligns[0] = AX_IVPS_ASPECT_RATIO_HORIZONTAL_CENTER;
    tAspectRatio.eAligns[1] = AX_IVPS_ASPECT_RATIO_VERTICAL_CENTER;
    tAspectRatio.nBgColor = 0x0000FF;

    tDstFrame.u32PicStride[0] = 1024;
    WidthTemp = tDstFrame.u32Width = 1024;
    HeightTemp = tDstFrame.u32Height = 576;
    tDstFrame.enImgFormat = ptSrcFrame->enImgFormat;
    ALOGI("tDstFrame stride:%d, width: %d, height: %d, format: %d",
          tDstFrame.u32PicStride[0], tDstFrame.u32Width, tDstFrame.u32Height,
          tDstFrame.enImgFormat);

    nImgSize = CalcImgSize(tDstFrame.u32PicStride[0], tDstFrame.u32Width,
                           tDstFrame.u32Height, tDstFrame.enImgFormat, 16);
    CHECK_RESULT(BufPoolBlockAddrGet(-1, nImgSize, &tDstFrame.u64PhyAddr[0],
                                     (AX_VOID **)(&tDstFrame.u64VirAddr[0]), &BlkId));

    /* memset((AX_VOID *)((AX_U32)tDstFrame.u64VirAddr[0]), tAspectRatio.nBgColor, nImgSize * 2); */

    ALOGI("tAspectRatio.eMode =%d", tAspectRatio.eMode);

    ret = AX_IVPS_CropResize(eEngineId, ptSrcFrame, &tDstFrame, &tAspectRatio);
    if (ret)
    {
        ALOGE("ret=0x%x", ret);
        return ret;
    }

    tDstFrame.u32Width = WidthTemp;
    tDstFrame.u32Height = HeightTemp;
    printf("OFFSET X0:%d Y0:%d W:%d H:%d", tDstFrame.s16CropX, tDstFrame.s16CropWidth,
           tDstFrame.s16CropY, tDstFrame.s16CropHeight);

    LIMIT_MIN(tDstFrame.u32PicStride[0], 16);
    SaveFile(&tDstFrame, 0, 0, strFilePath, "CropResize_");
    ret = AX_POOL_ReleaseBlock(BlkId);
    if (ret)
    {
        ALOGE("Rls BlkId fail, ret=0x%x", ret);
    }
    return ret;
}

/*
 * SAMPLE_IVPS_CropResizeV2
 * Function: Crop and Resize.Support max 1 in and 32 out.
 * Note: Stride and width should be 16 Byte aligned.
 *       The u64PhyAddr[0] of ptDst should be set. If format is AX_YUV420_SEMIPLANAR, u64PhyAddr[1] should be set.
 *       If enable crop, s16OffsetTop/s16OffsetBottom/s16OffsetRight/s16OffsetLeft should be set.
 */
AX_S32 SAMPLE_IVPS_CropResizeV2(IVPS_ENGINE_ID_E eEngineId, const AX_VIDEO_FRAME_T *ptSrcFrame, AX_IVPS_RECT_T tBox[], AX_U32 nNum,
                                AX_S32 nDstStride[], AX_S32 nDstWidth[], AX_S32 nDstHeight[], char *strFilePath)
{

    AX_S32 ret = 0;
    AX_VIDEO_FRAME_T tDstFrame[32] = {{0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}};
    AX_VIDEO_FRAME_T *ptDstFrame[32];
    AX_BLK BlkId[32];
    AX_U32 nImgSize;
    AX_IVPS_ASPECT_RATIO_T tAspectRatio;
    AX_U32 WidthTemp[32], HeightTemp[32];
    AX_U32 i;
    tAspectRatio.eMode = AX_IVPS_ASPECT_RATIO_STRETCH;
    tAspectRatio.eAligns[0] = AX_IVPS_ASPECT_RATIO_HORIZONTAL_CENTER;
    tAspectRatio.eAligns[1] = AX_IVPS_ASPECT_RATIO_VERTICAL_CENTER;
    tAspectRatio.nBgColor = 0x0000FF;
    ALOGI("tAspectRatio.eMode =%d\n", tAspectRatio.eMode);

    for (i = 0; i < nNum; i++)
    {
        ptDstFrame[i] = &tDstFrame[i]; /* ptDstFrame is only for passing parameters */
        tDstFrame[i].u32PicStride[0] = nDstStride[i];
        tDstFrame[i].u32Width = nDstWidth[i];
        tDstFrame[i].u32Height = nDstHeight[i];
        tDstFrame[i].enImgFormat = ptSrcFrame->enImgFormat;
        ALOGI("tDstFrame[%d] stride:%d, width: %d, height: %d, format: %d\n",
              i, tDstFrame[i].u32PicStride[0], tDstFrame[i].u32Width, tDstFrame[i].u32Height,
              tDstFrame[i].enImgFormat);

        WidthTemp[i] = tDstFrame[i].u32Width;
        HeightTemp[i] = tDstFrame[i].u32Height;
        nImgSize = CalcImgSize(tDstFrame[i].u32PicStride[0], tDstFrame[i].u32Width,
                               tDstFrame[i].u32Height, tDstFrame[i].enImgFormat, 16);

        CHECK_RESULT(BufPoolBlockAddrGet(-1, nImgSize, &(tDstFrame[i].u64PhyAddr[0]),
                                         (AX_VOID **)(&(tDstFrame[i].u64VirAddr[0])), &BlkId[i]));
    }

    ret = AX_IVPS_CropResizeV2(eEngineId, ptSrcFrame, tBox, nNum, ptDstFrame, &tAspectRatio);
    if (ret)
    {
        ALOGE("ret=0x%x\n", ret);
        return ret;
    }

    for (i = 0; i < nNum; i++)
    {
        tDstFrame[i].u32Width = WidthTemp[i];
        tDstFrame[i].u32Height = HeightTemp[i];
        ALOGI("CHN:%d OFFSET X0:%d Y0:%d W:%d H:%d\n", i, tDstFrame[i].s16CropX, tDstFrame[i].s16CropY,
              tDstFrame[i].s16CropWidth, tDstFrame[i].s16CropHeight);
        SaveFile(&tDstFrame[i], 0, i, strFilePath, "CropResizeV2_");
        ret = AX_POOL_ReleaseBlock(BlkId[i]);
        if (ret)
        {
            ALOGE("Rls BlkId[%d] fail, ret=0x%x\n", i, ret);
        }
    }

    return ret;
}

/*
 * SAMPLE_IVPS_CropResizeV3
 * Function: Crop and Resize.Support one in and four out.
 * Note: Stride should be 16 Byte aligned.
 *       The u64PhyAddr[0] of ptDst should be set.
 *       If crop is enabled, s16OffsetTop/s16OffsetBottom/s16OffsetRight/s16OffsetLeft should be set.
 */
AX_S32 SAMPLE_IVPS_CropResizeV3(const AX_VIDEO_FRAME_T *ptSrcFrame,
                                AX_S32 nDstStride[], AX_S32 nDstWidth[],
                                AX_S32 nDstHeight[], AX_U32 nNum, char *strFilePath)
{

    AX_S32 ret = 0;
    AX_VIDEO_FRAME_T tDstFrame[4] = {{0}, {0}, {0}, {0}};
    AX_VIDEO_FRAME_T *ptDstFrame[4];
    AX_BLK BlkId[4];
    AX_U32 nImgSize;
    AX_IVPS_ASPECT_RATIO_T tAspectRatio;
    AX_U32 WidthTemp[4], HeightTemp[4];
    AX_U32 i;
    tAspectRatio.eMode = AX_IVPS_ASPECT_RATIO_STRETCH;
    tAspectRatio.eAligns[0] = AX_IVPS_ASPECT_RATIO_HORIZONTAL_CENTER;
    tAspectRatio.eAligns[1] = AX_IVPS_ASPECT_RATIO_VERTICAL_CENTER;
    tAspectRatio.nBgColor = 0x0000FF;
    ALOGI("tAspectRatio.eMode =%d", tAspectRatio.eMode);

    for (i = 0; i < nNum; i++)
    {
        ptDstFrame[i] = &tDstFrame[i]; /* ptDstFrame is only for passing parameters */
        tDstFrame[i].u32PicStride[0] = nDstStride[i];
        tDstFrame[i].u32Width = nDstWidth[i];
        tDstFrame[i].u32Height = nDstHeight[i];
        tDstFrame[i].enImgFormat = ptSrcFrame->enImgFormat;
        ALOGI("tDstFrame[%d] stride:%d, width: %d, height: %d, format: %d",
              i, tDstFrame[i].u32PicStride[0], tDstFrame[i].u32Width, tDstFrame[i].u32Height,
              tDstFrame[i].enImgFormat);

        WidthTemp[i] = tDstFrame[i].u32Width;
        HeightTemp[i] = tDstFrame[i].u32Height;
        nImgSize = CalcImgSize(tDstFrame[i].u32PicStride[0], tDstFrame[i].u32Width,
                               tDstFrame[i].u32Height, tDstFrame[i].enImgFormat, 16);

        CHECK_RESULT(BufPoolBlockAddrGet(-1, nImgSize, &(tDstFrame[i].u64PhyAddr[0]),
                                         (AX_VOID **)(&(tDstFrame[i].u64VirAddr[0])), &BlkId[i]));
    }

    ret = AX_IVPS_CropResizeV3Vpp(ptSrcFrame, ptDstFrame, nNum, &tAspectRatio);
    if (ret)
    {
        ALOGE("ret=0x%x", ret);
        return ret;
    }

    for (i = 0; i < nNum; i++)
    {
        tDstFrame[i].u32Width = WidthTemp[i];
        tDstFrame[i].u32Height = HeightTemp[i];
        ALOGI("CHN:%d OFFSET X0:%d Y0:%d W:%d H:%d", i, tDstFrame[i].s16CropX, tDstFrame[i].s16CropY,
              tDstFrame[i].s16CropWidth, tDstFrame[i].s16CropHeight);
        SaveFile(&tDstFrame[i], 0, i, strFilePath, "CropResizeV3_");
        ret = AX_POOL_ReleaseBlock(BlkId[i]);
        if (ret)
        {
            ALOGE("Rls BlkId[%d] fail, ret=0x%x", i, ret);
        }
    }

    return ret;
}

/*
 * SAMPLE_IVPS_Mosaic()
 * Function: Draw mosaic and save output file.
 * Note: Stride should be 16 Byte aligned.
 *       Draw up to 32 mosaics at once.
 */
AX_S32 SAMPLE_IVPS_DrawMosaic(IVPS_ENGINE_ID_E eEngineId, const AX_VIDEO_FRAME_T *ptSrcFrame, AX_IVPS_RGN_MOSAIC_T tMosaic[], AX_U32 nNum, char *strFilePath)
{
    AX_S32 ret = 0;
    AX_VIDEO_FRAME_T tDstFrame = {0};
    ret = AX_IVPS_DrawMosaic(eEngineId, ptSrcFrame, tMosaic, nNum);
    if (ret)
    {
        ALOGE("ret=0x%x", ret);
        return ret;
    }

    tDstFrame.u32PicStride[0] = ptSrcFrame->u32PicStride[0];
    tDstFrame.u32Width = ptSrcFrame->u32Width;
    tDstFrame.u32Height = ptSrcFrame->u32Height;
    tDstFrame.enImgFormat = ptSrcFrame->enImgFormat;
    tDstFrame.u64PhyAddr[0] = ptSrcFrame->u64PhyAddr[0];
    tDstFrame.u64PhyAddr[1] = ptSrcFrame->u64PhyAddr[1];
    SaveFile(&tDstFrame, 0, 0, strFilePath, "Mosaic_");

    return ret;
}
