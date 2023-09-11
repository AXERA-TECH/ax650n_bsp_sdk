/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include "sample_ivps_main.h"

static SAMPLE_IVPS_MAIN_T gSampleIvpsMain;

AX_S32 main(AX_S32 argc, char *argv[])
{
    AX_S32 ret = IVPS_SUCC;
    IVPS_ARG_T *pIvpsArg = &gSampleIvpsMain.tIvpsArg;
    SAMPLE_IVPS_GRP_T *pGrp = &gSampleIvpsGrp;
    SAMPLE_IVPS_GRP_T *pGrpExt = &gSampleIvpsGrpExt;
    IVPS_BLK_T arrBlkInfo[16];
    char strFilePath[32], *pFrameFile;
    AX_U32 BlkSize;

    printf("IVPS Sample. Build at %s %s\n", __DATE__, __TIME__);

    pGrp->bSaveFile = AX_TRUE;
    /* Parse the input parameter */
    IVPS_ArgsParser(argc, argv, pIvpsArg);
    if (pIvpsArg->pPipelineIni)
        IVPS_GrpIniParser(pIvpsArg->pPipelineIni, pGrp);
    else
        IVPS_ChnInfoParser(pIvpsArg, &pGrp->tPipelineAttr);
    if (pIvpsArg->pPipelineExtIni)
        IVPS_GrpIniParser(pIvpsArg->pPipelineExtIni, pGrpExt);
    if (pIvpsArg->pChangeIni)
        IVPS_ChangeIniParser(pIvpsArg->pChangeIni, &gSampleChange);
    if (pIvpsArg->pDewarpIni)
        IVPS_DewarpIniParser(pIvpsArg->pDewarpIni, &gSampleDewarp);
    /* SYS global init */
    CHECK_RESULT(AX_SYS_Init());

    /* Create common pool */
    arrBlkInfo[0].nSize = CalcImgSize(1920, 1920, 1080, AX_FORMAT_RGBA8888, 16);
    arrBlkInfo[0].nCnt = 15;
    arrBlkInfo[1].nSize = CalcImgSize(3840, 3840, 2160, AX_FORMAT_RGBA8888, 16);
    arrBlkInfo[1].nCnt = 15;
    arrBlkInfo[2].nSize = CalcImgSize(256, 256, 256, AX_FORMAT_RGBA8888, 16);
    arrBlkInfo[2].nCnt = 36;
    ret = IVPS_CommonPoolCreate(&arrBlkInfo[0], 3);
    if (ret)
    {
        ALOGE("AX_IVPS_Init failed, ret=0x%x.", ret);
        goto error0;
    }

    pGrp->ePoolSrc = pIvpsArg->ePoolSrc;
    if (POOL_SOURCE_USER == pIvpsArg->ePoolSrc) {
        /* Create user pool, get pool id */
        BlkSize = CalcImgSize(4096, 4096, 3840, AX_FORMAT_RGBA8888, 16);
        pGrp->user_pool_id = IVPS_UserPoolCreate(BlkSize, 10);
    }

    /* IVPS initialization */
    ret = AX_IVPS_Init();
    if (ret)
    {
        ALOGE("AX_IVPS_Init failed, ret=0x%x.", ret);
        goto error1;
    }

    /* Source image buffer and info get */
    pFrameFile = FrameInfoGet(pIvpsArg->pFrameInfo, &pGrp->tFrameInput);

    if (pIvpsArg->nStreamNum <= 0)
        FrameBufGet(0, &pGrp->tFrameInput, pFrameFile);

    printf("INPUT nW:%d nH:%d File:%s\n", pGrp->tFrameInput.u32Width, pGrp->tFrameInput.u32Height, pFrameFile);

    pGrp->pFileName = pFrameFile;
    pGrp->nIvpsStreamNum = pIvpsArg->nStreamNum;
    pGrp->nIvpsRepeatNum = pIvpsArg->nRepeatCount;
    pGrp->nRegionNum = pIvpsArg->nRegionNum;
    pGrpExt->nIvpsRepeatNum = pIvpsArg->nRepeatCount;
    pGrpExt->tFrameInput = pGrp->tFrameInput;

    pGrp->pFilePath = &strFilePath[0];
    strcpy(pGrp->pFilePath, pFrameFile);
    FilePathExtract(pGrp->pFilePath); /* The path is also output frame path */
    pGrpExt->pFilePath = pGrp->pFilePath;

    ALOGI("nRepeatCount:%d nRegionNum:%d", pIvpsArg->nRepeatCount, pIvpsArg->nRegionNum);

    /* IVPS sync api */
    SAMPLE_IVPS_SyncApi(pIvpsArg, pGrp, &gSampleIvpsSyncApi);

    /* IVPS dewarp api */
    SAMPLE_IVPS_Dewarp(&pGrp->tFrameInput, pGrp->pFilePath, &gSampleDewarp);

    /* IVPS pipeline start */
    if (pIvpsArg->nIvpsGrp) {
        pGrp->nIvpsGrp = pIvpsArg->nIvpsGrp;
    }
    ret = IVPS_StartGrp(pGrp);
    if (ret)
    {
        ALOGE("IVPS_StartGrp failed, ret=0x%x.", ret);
        goto error2;
    }

    /* IVPS region start */
    if (pIvpsArg->nRegionNum > 0)
    {
        /* Start region with parameter */
        if (0 != IVPS_StartRegion(pIvpsArg->nRegionNum))
        {
            ThreadLoopStateSet(AX_TRUE);
        }
        IVPS_UpdateThreadStart(pIvpsArg->nRegionNum, pGrp);
    }

    switch (pIvpsArg->nLinkMode)
    {
    case IVPS_LINK_IVPS:
        SAMPLE_IVPS_LinkIvps(pGrp->nIvpsGrp, 0, pGrpExt);
        IVPS_ThreadStart(pGrp, NULL);
        /* Dynamic change channel attributes */
        if (pIvpsArg->pChangeIni)
        {
            pGrp->bSaveFile = AX_FALSE;
            IVPS_FilterAttrThreadStart(pGrp);
            IVPS_FilterAttrThreadStart(pGrpExt);
        }
        break;
    case IVPS_LINK_VENC:
        SAMPLE_IVPS_LinkVenc(pGrp, AX_TRUE);
        IVPS_ThreadStart(pGrp, NULL);
        break;
    case IVPS_LINK_JENC:
        SAMPLE_IVPS_LinkVenc(pGrp, AX_FALSE);
        IVPS_ThreadStart(pGrp, NULL);
        break;
    default:
        IVPS_ThreadStart(pGrp, pGrp);
        /* Dynamic change channel attributes */
        if (pIvpsArg->pChangeIni)
        {
            pGrp->bSaveFile = AX_FALSE;
            IVPS_FilterAttrThreadStart(pGrp);
        }
        break;
    }

    while (!ThreadLoopStateGet() && (pGrp->nIvpsRepeatNum || pGrp->nIvpsStreamNum))
    {
        sleep(1);
    }
    if (pIvpsArg->nRepeatCount)
        sleep(1);

    /* Stop IVPS region */
    if (pIvpsArg->nRegionNum > 0)
    {
        IVPS_UpdateThreadStop(pGrp);
        IVPS_StopRegion();
    }

    ThreadLoopStateSet(AX_TRUE);
    if (pIvpsArg->pChangeIni)
    {
        IVPS_FilterAttrThreadStop(pGrp);
        if (IVPS_LINK_IVPS == pIvpsArg->nLinkMode)
        {
            IVPS_FilterAttrThreadStop(pGrpExt);
        }
    }
    /* Stop IVPS pipeline */
    IVPS_ThreadStop(pGrp);
    IVPS_StopGrp(pGrp);
    /* Stop other pipeline */
    switch (pIvpsArg->nLinkMode)
    {
    case IVPS_LINK_IVPS:
        IVPS_ThreadStop(pGrpExt);
        IVPS_StopGrp(pGrpExt);
        break;
    default:
        break;
    }
error2:
    /* IVPS release */
    AX_IVPS_Deinit();

error1:
    /* POOL release */
    if (POOL_SOURCE_USER == pIvpsArg->ePoolSrc) {
        AX_POOL_DestroyPool(pGrp->user_pool_id);
    }
    AX_POOL_Exit();

error0:
    /* SYS release */
    AX_SYS_Deinit();

    printf("\nsample test run success\n");
    return 0;
}
