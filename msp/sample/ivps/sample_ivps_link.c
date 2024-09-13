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

/*-------------------------------------------------------------------------*/
/**
  @brief    Sample for IVPS -> VENC.
  @param    pGrp       IVPS group information.
  @param    bVencMode     True: the output is video frame. false the output is JPEG.
  @return    0 if Ok, anything else otherwise.
 */
/*--------------------------------------------------------------------------*/
AX_S32 SAMPLE_IVPS_LinkVenc(SAMPLE_IVPS_GRP_T *pGrp, AX_BOOL bVencMode)
{
    AX_U32 nWidth = 0, nHeight = 0;
    AX_S32 ret;
    AX_MOD_INFO_T tSrcMod = {0};
    AX_MOD_ID_E dstModId = AX_ID_BUTT;

    if (pGrp->tPipelineAttr.tFilter[1][1].bEngage)
    {
        nWidth = pGrp->tPipelineAttr.tFilter[1][1].nDstPicWidth;
        nHeight = pGrp->tPipelineAttr.tFilter[1][1].nDstPicHeight;
    }
    else if (pGrp->tPipelineAttr.tFilter[1][0].bEngage)
    {
        nWidth = pGrp->tPipelineAttr.tFilter[1][0].nDstPicWidth;
        nHeight = pGrp->tPipelineAttr.tFilter[1][0].nDstPicHeight;
    }
    else if (pGrp->tPipelineAttr.tFilter[0][0].bEngage)
    {
        nWidth = pGrp->tPipelineAttr.tFilter[0][0].nDstPicWidth;
        nHeight = pGrp->tPipelineAttr.tFilter[0][0].nDstPicHeight;
    }
    else
    {
        ALOGE("IVPS module is Bypass!");
        return -1;
    }
    printf("VENC nWidth:%d nHeight:%d\n", nWidth, nHeight);
    dstModId = AX_ID_VENC;
    tSrcMod.enModId = AX_ID_IVPS;
    tSrcMod.s32GrpId = pGrp->nIvpsGrp;
    tSrcMod.s32ChnId = 0;

    ret = SAMPLE_Ivps2VencInit(nWidth, nHeight, &tSrcMod, dstModId);
    if (0 != ret)
    {
        ALOGE("UTestIvpsLinkVenc failed, ret=0x%x", ret);
        return ret;
    }
    return 0;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Sample for IVPS -> IVPS.
  @param    nGrpIdx  The group index of upstream IVPS.
  @param    nChnIdx  The channel index of upstream IVPS.
  @param    pGrp       Ivps group information.
  @return    0 if Ok, anything else otherwise.
 */
/*--------------------------------------------------------------------------*/
AX_S32 SAMPLE_IVPS_LinkIvps(AX_S32 nGrpIdx, AX_S32 nChnIdx, SAMPLE_IVPS_GRP_T *pGrp)
{
    AX_MOD_INFO_T tSrcMod = {0};
    AX_MOD_INFO_T tDstMod = {0};

    tSrcMod.enModId = AX_ID_IVPS;
    tSrcMod.s32GrpId = nGrpIdx;
    tSrcMod.s32ChnId = nChnIdx;

    tDstMod.enModId = AX_ID_IVPS;
    tDstMod.s32GrpId = pGrp->nIvpsGrp;
    tDstMod.s32ChnId = 0;
    AX_SYS_Link(&tSrcMod, &tDstMod);

    CHECK_RESULT(IVPS_StartGrp(pGrp));
    IVPS_ThreadStart(NULL, pGrp);
    return 0;
}