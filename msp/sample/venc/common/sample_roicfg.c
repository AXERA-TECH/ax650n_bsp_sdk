/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include "sample_venc_log.h"
#include "sample_roicfg.h"

AX_VENC_ROI_ATTR_T stRoiAttr[MAX_JENC_ROI_NUM];

static AX_S32 SampleJpegReadRoimap(AX_CHAR *roimapFile, AX_VENC_ROI_ATTR_T *pstRoiAttr)
{
    FILE *fpROI;
    AX_CHAR buf[30];
    AX_U32 roiRectNum = 0;
    AX_U32 bEnable = 0;

    if (NULL == roimapFile || NULL == pstRoiAttr) {
        SAMPLE_LOG_ERR("NULL pointer roimapFile %p pstRoiAttr %p\n", roimapFile, pstRoiAttr);
        return -1;
    }

    fpROI = fopen(roimapFile, "r");
    if (fpROI == NULL) {
        SAMPLE_LOG_ERR("Error, Can Not Open File %s\n", roimapFile);
        return -1;
    }
    while ((fgets(buf, 30, fpROI) != NULL) && roiRectNum < MAX_JENC_ROI_NUM) {
        if (buf[0] == 'r') {
            sscanf(buf, "roi=(%d,%d,%d,%d,%d)", &bEnable, &pstRoiAttr[roiRectNum].stRoiArea.u32X,
                   &pstRoiAttr[roiRectNum].stRoiArea.u32Y, &pstRoiAttr[roiRectNum].stRoiArea.u32Width,
                   &pstRoiAttr[roiRectNum].stRoiArea.u32Height);
            pstRoiAttr[roiRectNum].bEnable = bEnable;
            roiRectNum++;
        }
    }
    fclose(fpROI);

    return AX_SUCCESS;
}

AX_S32 SampleJpegRoiCfg(SAMPLE_ROI_CFG_T *pCmdl, AX_VENC_JPEG_PARAM_T *pStJpegParam)
{
    AX_S32 s32Ret = AX_SUCCESS;
    AX_U32 i = 0;
    AX_U32 index = 0;

    s32Ret = SampleJpegReadRoimap(pCmdl->roimapFile, &stRoiAttr[0]);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("SampleJpegReadRoimap:failed!s32Ret=0x%x\n", s32Ret);
        return -1;
    }

    pStJpegParam->bEnableRoi = 1;
    pStJpegParam->u32Qfactor = pCmdl->qFactor;
    pStJpegParam->bSaveNonRoiQt = 0;
    pStJpegParam->u32RoiQfactor = pCmdl->qRoiFactor;
    memcpy(pStJpegParam->u8YQt, std_luminance_quant_tbl, sizeof(std_luminance_quant_tbl));
    memcpy(pStJpegParam->u8CbCrQt, std_chrominance_quant_tbl, sizeof(std_chrominance_quant_tbl));
    memcpy(pStJpegParam->u8RoiYQt, std_luminance_quant_tbl, sizeof(std_luminance_quant_tbl));
    memcpy(pStJpegParam->u8RoiCbCrQt, std_chrominance_quant_tbl, sizeof(std_chrominance_quant_tbl));
    for (i = 0; i < MAX_JENC_ROI_NUM; i++) {
        SAMPLE_LOG_DEBUG("i %d enable %d area %d %d %d %d \n", i, stRoiAttr[i].bEnable, stRoiAttr[i].stRoiArea.u32X,
                   stRoiAttr[i].stRoiArea.u32Y, stRoiAttr[i].stRoiArea.u32Width, stRoiAttr[i].stRoiArea.u32Height);
        if (stRoiAttr[i].bEnable) {
            index = i;
            pStJpegParam->bEnable[index] = 1;
            pStJpegParam->stRoiArea[index].u32X = stRoiAttr[i].stRoiArea.u32X;
            pStJpegParam->stRoiArea[index].u32Y = stRoiAttr[i].stRoiArea.u32Y;
            pStJpegParam->stRoiArea[index].u32Width = stRoiAttr[i].stRoiArea.u32Width;
            pStJpegParam->stRoiArea[index].u32Height = stRoiAttr[i].stRoiArea.u32Height;
        }
    }

    return AX_SUCCESS;
}