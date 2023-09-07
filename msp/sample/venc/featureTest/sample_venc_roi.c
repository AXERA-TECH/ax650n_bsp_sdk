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

static void SampleChnAttrDebug(AX_VENC_ROI_ATTR_T *pstRoiAttr)
{
    if (NULL == pstRoiAttr) {
        SAMPLE_LOG_ERR("NULL pointer\n");
        return;
    }

    if (pstRoiAttr->bEnable)
        SAMPLE_LOG_DEBUG("u32Index %d bEnable %d bAbsQp %d s32RoiQp %d Area %d %d %d %d\n", pstRoiAttr->u32Index,
                         pstRoiAttr->bEnable, pstRoiAttr->bAbsQp, pstRoiAttr->s32RoiQp, pstRoiAttr->stRoiArea.u32X,
                         pstRoiAttr->stRoiArea.u32Y, pstRoiAttr->stRoiArea.u32Width, pstRoiAttr->stRoiArea.u32Height);

    return;
}

static AX_S32 SampleVencReadRoimap(AX_CHAR *roimapFile, AX_VENC_ROI_ATTR_T *pstRoiAttr)
{
    FILE *fpROI;
    AX_CHAR buf[50];
    AX_U32 roiRectNum = 0;
    AX_U32 bEnable = 0;
    AX_U32 bAbsQp = 0;

    if (NULL == roimapFile || NULL == pstRoiAttr) {
        SAMPLE_LOG_ERR("NULL pointer roimapFile %p pstRoiAttr %p\n", roimapFile, pstRoiAttr);
        return -1;
    }

    fpROI = fopen(roimapFile, "r");
    if (fpROI == NULL) {
        SAMPLE_LOG_ERR("Error, Can Not Open File %s\n", roimapFile);
        return -1;
    }
    while ((fgets(buf, 50, fpROI) != NULL) && roiRectNum < MAX_VENC_ROI_NUM) {
        if (buf[0] == 'r') {
            sscanf(buf, "roi=(%d,%d,%d,%d,%d,%d,%d,%d)", &pstRoiAttr[roiRectNum].u32Index, &bEnable, &bAbsQp,
                   &pstRoiAttr[roiRectNum].s32RoiQp, &pstRoiAttr[roiRectNum].stRoiArea.u32X,
                   &pstRoiAttr[roiRectNum].stRoiArea.u32Y, &pstRoiAttr[roiRectNum].stRoiArea.u32Width,
                   &pstRoiAttr[roiRectNum].stRoiArea.u32Height);
            pstRoiAttr[roiRectNum].bEnable = bEnable;
            pstRoiAttr[roiRectNum].bAbsQp = bAbsQp;
            roiRectNum++;
        }
    }
    fclose(fpROI);

    return AX_SUCCESS;
}

AX_S32 SampleVencRoi(AX_S32 VencChn, AX_VOID *handle)
{
    AX_S32 s32Ret = AX_SUCCESS;
    int i = 0;
    AX_VENC_ROI_ATTR_T stRoiAttr[MAX_VENC_ROI_NUM];
    AX_VENC_ROI_ATTR_T stRoiAttrGet;
    SAMPLE_VENC_SENDFRAME_PARA_T *pCml = (SAMPLE_VENC_SENDFRAME_PARA_T *)handle;

    if (pCml->enType == PT_JPEG || pCml->enType == PT_MJPEG)
        return AX_SUCCESS;

    memset(&stRoiAttr[0], 0x0, sizeof(AX_VENC_ROI_ATTR_T) * MAX_VENC_ROI_NUM);
    memset(&stRoiAttrGet, 0x0, sizeof(AX_VENC_ROI_ATTR_T));
    s32Ret = SampleVencReadRoimap(pCml->vencRoiMap, &stRoiAttr[0]);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_LOG_ERR("Chn %d: SampleVencReadRoimap failed, ret=%x\n", VencChn, s32Ret);
        return -1;
    }

    for (i = 0; i < MAX_VENC_ROI_NUM; i++) {
        if (stRoiAttr[i].bEnable) {
            s32Ret = AX_VENC_GetRoiAttr(VencChn, i, &stRoiAttrGet);
            if (AX_SUCCESS != s32Ret) {
                SAMPLE_LOG_ERR("Chn %d: AX_VENC_GetRoiAttr %d failed, ret=%x\n", VencChn, i, s32Ret);
                return -1;
            }

            SAMPLE_LOG_DEBUG("old Roi param, Index %d\n", i);
            SampleChnAttrDebug(&stRoiAttrGet);
            s32Ret = AX_VENC_SetRoiAttr(VencChn, &stRoiAttr[i]);
            if (AX_SUCCESS != s32Ret) {
                SAMPLE_LOG_ERR("Chn %d: AX_VENC_SetRoiAttr %d failed, ret=%x\n", VencChn, i, s32Ret);
                return -1;
            }

            s32Ret = AX_VENC_GetRoiAttr(VencChn, i, &stRoiAttrGet);
            if (AX_SUCCESS != s32Ret) {
                SAMPLE_LOG_ERR("Chn %d: AX_VENC_GetRoiAttr %d failed, ret=%x\n", VencChn, i, s32Ret);
                return -1;
            }

            SAMPLE_LOG_DEBUG("new Roi param, Index %d\n", i);
            SampleChnAttrDebug(&stRoiAttrGet);
        }
    }

    return AX_SUCCESS;
}