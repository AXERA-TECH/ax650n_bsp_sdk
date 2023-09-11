/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/


#include "sample_ive.h"

typedef struct axTEST_GMM_T {
    AX_IVE_SRC_IMAGE_T stSrc;
    AX_IVE_DST_IMAGE_T stFg;
    AX_IVE_DST_IMAGE_T stBg;
    AX_IVE_MEM_INFO_T stModel;
    AX_IVE_GMM_CTRL_T stGmmCtrl;
    AX_IVE_GMM2_CTRL_T stGmm2Ctrl;
    FILE* pFpSrc;
    FILE* pFpSrcModel;
    FILE* pFpDstFg;
    FILE* pFpDstBg;
    FILE* pFpDstModel;
} TEST_GMM_T;
static TEST_GMM_T s_stTestGmm;

/******************************************************************************
* function : parse gmm and gmm2 parameters
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestGmm_ParseParams(TEST_GMM_T* pstTestGmm, AX_CHAR *pchParamsList, AX_U32 u32Mode)
{
    cJSON *root = NULL;
    FILE *fp = fopen(pchParamsList, "r");
    if (!fp) {
        root = cJSON_Parse(pchParamsList);
        if (!root) {
            SAMPLE_IVE_PRT("Error:parse parameters from string %s failed!\n", pchParamsList);
            return AX_FAILURE;
        }
    } else {
        AX_CHAR buf[512] = {0};
        fread(buf, 1, sizeof(buf), fp);
        root = cJSON_Parse(buf);
        if (!root) {
            SAMPLE_IVE_PRT("Error:parse parameters from file %s failed!\n", pchParamsList);
            return AX_FAILURE;
        }
    }
    cJSON *item = NULL;
    if (u32Mode == 0) {
        item = cJSON_GetObjectItem(root, "init_var");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param init_var failed!\n");
        pstTestGmm->stGmmCtrl.u14q4InitVar = item->valueint;
        item = cJSON_GetObjectItem(root, "min_var");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param min_var failed!\n");
        pstTestGmm->stGmmCtrl.u14q4MinVar = item->valueint;
        item = cJSON_GetObjectItem(root, "init_w");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param init_weight failed!\n");
        pstTestGmm->stGmmCtrl.u1q10InitWeight = item->valueint;
        item = cJSON_GetObjectItem(root, "lr");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param learn_rate failed!\n");
        pstTestGmm->stGmmCtrl.u1q7LearnRate = item->valueint;
        item = cJSON_GetObjectItem(root, "bg_r");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param bg_ratio failed!\n");
        pstTestGmm->stGmmCtrl.u1q7BgRatio = item->valueint;
        item = cJSON_GetObjectItem(root, "var_thr");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param var_thr failed!\n");
        pstTestGmm->stGmmCtrl.u4q4VarThr = item->valueint;
        item = cJSON_GetObjectItem(root, "thr");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param thr failed!\n");
        pstTestGmm->stGmmCtrl.u8Thr = item->valueint;
    } else {
        item = cJSON_GetObjectItem(root, "init_var");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param init_var failed!\n");
        pstTestGmm->stGmm2Ctrl.u14q4InitVar = item->valueint;
        item = cJSON_GetObjectItem(root, "min_var");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param min_var failed!\n");
        pstTestGmm->stGmm2Ctrl.u14q4MinVar = item->valueint;
        item = cJSON_GetObjectItem(root, "max_var");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param max_var failed!\n");
        pstTestGmm->stGmm2Ctrl.u14q4MaxVar = item->valueint;
        item = cJSON_GetObjectItem(root, "lr");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param learn_rate failed!\n");
        pstTestGmm->stGmm2Ctrl.u1q7LearnRate = item->valueint;
        item = cJSON_GetObjectItem(root, "bg_r");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param bg_ratio failed!\n");
        pstTestGmm->stGmm2Ctrl.u1q7BgRatio = item->valueint;
        item = cJSON_GetObjectItem(root, "var_thr");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param var_thr failed!\n");
        pstTestGmm->stGmm2Ctrl.u4q4VarThr = item->valueint;
        item = cJSON_GetObjectItem(root, "var_thr_chk");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param var_thr_check failed!\n");
        pstTestGmm->stGmm2Ctrl.u4q4VarThrCheck = item->valueint;
        item = cJSON_GetObjectItem(root, "ct");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param ct failed!\n");
        pstTestGmm->stGmm2Ctrl.s1q7CT = item->valueint;
        item = cJSON_GetObjectItem(root, "thr");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param thr failed!\n");
        pstTestGmm->stGmm2Ctrl.u8Thr = item->valueint;
    }
    cJSON_Delete(root);
    if(fp)
        fclose(fp);
    SAMPLE_IVE_PRT("Parse params success!\n");
    return AX_SUCCESS;

PARSE_FAIL:
    cJSON_Delete(root);
    if(fp)
        fclose(fp);
    return AX_FAILURE;
}
/******************************************************************************
* function : test gmm and gmm2 uninit
******************************************************************************/
static AX_VOID SAMPLE_IVE_TestGmm_Uninit(TEST_GMM_T* pstTestGmm)
{
    IVE_CMM_FREE(pstTestGmm->stSrc.au64PhyAddr[0], pstTestGmm->stSrc.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestGmm->stFg.au64PhyAddr[0], pstTestGmm->stFg.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestGmm->stBg.au64PhyAddr[0], pstTestGmm->stBg.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestGmm->stModel.u64PhyAddr, pstTestGmm->stModel.u64VirAddr);

    if (NULL != pstTestGmm->pFpSrc) {
        fclose(pstTestGmm->pFpSrc);
        pstTestGmm->pFpSrc = NULL;
    }
    if (NULL != pstTestGmm->pFpSrcModel) {
        fclose(pstTestGmm->pFpSrcModel);
        pstTestGmm->pFpSrcModel = NULL;
    }
    if (NULL != pstTestGmm->pFpDstFg) {
        fclose(pstTestGmm->pFpDstFg);
        pstTestGmm->pFpDstFg = NULL;
    }
    if (NULL != pstTestGmm->pFpDstBg) {
        fclose(pstTestGmm->pFpDstBg);
        pstTestGmm->pFpDstBg = NULL;
    }
    if (NULL != pstTestGmm->pFpDstModel) {
        fclose(pstTestGmm->pFpDstModel);
        pstTestGmm->pFpDstModel = NULL;
    }

}
/******************************************************************************
* function : test gmm and gmm2 init
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestGmm_Init(TEST_GMM_T* pstTestGmm, AX_CHAR* pchSrcFileName, AX_CHAR* pchSrcModelFileName,
    AX_CHAR* pchDstFgFileName,  AX_CHAR* pchDstBgFileName,  AX_CHAR* pchDstModelFileName, AX_U32 u32Width, AX_U32 u32Height, AX_S32 as32Type[])
{
    AX_S32 s32Ret = AX_FAILURE;
    AX_IVE_IMAGE_TYPE_E enSrcImageType = (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[0], AX_IVE_IMAGE_TYPE_U8C1);
    AX_U32 u32Size = (enSrcImageType == AX_IVE_IMAGE_TYPE_U8C1) ? 48 : 64;
    memset(pstTestGmm, 0, sizeof(TEST_GMM_T));

    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestGmm->stSrc), enSrcImageType, u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create stSrc image failed!\n", s32Ret);
        SAMPLE_IVE_TestGmm_Uninit(pstTestGmm);
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestGmm->stFg), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[1], AX_IVE_IMAGE_TYPE_U8C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create stFg image failed!\n", s32Ret);
        SAMPLE_IVE_TestGmm_Uninit(pstTestGmm);
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestGmm->stBg), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[2], AX_IVE_IMAGE_TYPE_U8C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create stBg image failed!\n", s32Ret);
        SAMPLE_IVE_TestGmm_Uninit(pstTestGmm);
        return s32Ret;
    }
    u32Size = u32Width * u32Height * u32Size;
    s32Ret = SAMPLE_COMM_IVE_CreateMemInfo(&(pstTestGmm->stModel), u32Size);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create gmm stModel failed!\n", s32Ret);
        SAMPLE_IVE_TestGmm_Uninit(pstTestGmm);
        return s32Ret;
    }

    s32Ret = AX_FAILURE;
    pstTestGmm->pFpSrc = fopen(pchSrcFileName, "rb");
    if (AX_NULL == pstTestGmm->pFpSrc) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchSrcFileName);
        SAMPLE_IVE_TestGmm_Uninit(pstTestGmm);
        return s32Ret;
    }
    pstTestGmm->pFpSrcModel = fopen(pchSrcModelFileName, "rb");
    if (AX_NULL == pstTestGmm->pFpSrcModel) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchSrcModelFileName);
        SAMPLE_IVE_TestGmm_Uninit(pstTestGmm);
        return s32Ret;
    }
    pstTestGmm->pFpDstFg = fopen(pchDstFgFileName, "wb");
    if (AX_NULL == pstTestGmm->pFpDstFg) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchDstFgFileName);
        SAMPLE_IVE_TestGmm_Uninit(pstTestGmm);
        return s32Ret;
    }
    pstTestGmm->pFpDstBg = fopen(pchDstBgFileName, "wb");
    if (AX_NULL == pstTestGmm->pFpDstBg) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchDstBgFileName);
        SAMPLE_IVE_TestGmm_Uninit(pstTestGmm);
        return s32Ret;
    }
     pstTestGmm->pFpDstModel = fopen(pchDstModelFileName, "wb");
    if (AX_NULL == pstTestGmm->pFpDstModel) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchDstModelFileName);
        SAMPLE_IVE_TestGmm_Uninit(pstTestGmm);
        return s32Ret;
    }

    return AX_SUCCESS;
}
/******************************************************************************
* function : test gmm and gmm2
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestGmmProc(TEST_GMM_T* pstTestGmm, AX_U32 u32Mode, AX_CHAR *pchParamsList)
{
    AX_S32 s32Ret;
    AX_IVE_HANDLE IveHandle;
    AX_BOOL bInstant = AX_FALSE;
    AX_BOOL bBlock = AX_TRUE;
    AX_BOOL bFinish = AX_FALSE;

    if (pchParamsList) {
        s32Ret = SAMPLE_IVE_TestGmm_ParseParams(pstTestGmm, pchParamsList, u32Mode);
        if (AX_SUCCESS != s32Ret)
            return s32Ret;
    } else {
        pstTestGmm->stGmmCtrl.u14q4InitVar = 22723;
        pstTestGmm->stGmmCtrl.u14q4MinVar = 238382;
        pstTestGmm->stGmmCtrl.u1q10InitWeight = 124;
        pstTestGmm->stGmmCtrl.u1q7LearnRate = 113;
        pstTestGmm->stGmmCtrl.u1q7BgRatio = 83;
        pstTestGmm->stGmmCtrl.u4q4VarThr = 221;
        pstTestGmm->stGmmCtrl.u8Thr = 166;

        pstTestGmm->stGmm2Ctrl.u14q4InitVar = 202820;
        pstTestGmm->stGmm2Ctrl.u14q4MinVar = 110515;
        pstTestGmm->stGmm2Ctrl.u14q4MaxVar = 197086;
        pstTestGmm->stGmm2Ctrl.u1q7LearnRate = 114;
        pstTestGmm->stGmm2Ctrl.u1q7BgRatio = 124;
        pstTestGmm->stGmm2Ctrl.u4q4VarThr = 84;
        pstTestGmm->stGmm2Ctrl.u4q4VarThrCheck = 243;
        pstTestGmm->stGmm2Ctrl.s1q7CT = 31;
        pstTestGmm->stGmm2Ctrl.u8Thr = 199;
    }
    s32Ret = SAMPLE_COMM_IVE_ReadFile(&(pstTestGmm->stSrc), pstTestGmm->pFpSrc);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Read src file failed!\n",s32Ret);
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_ReadMemInfoFile(&(pstTestGmm->stModel), pstTestGmm->pFpSrcModel);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Read src model file failed!\n",s32Ret);
        return s32Ret;
    }

    AX_U64 u64StartTime = SAMPLE_COMM_IVE_GetTime_US();
    bInstant = AX_TRUE;
    if (u32Mode == 0) {
        s32Ret = AX_IVE_GMM(&IveHandle, &pstTestGmm->stSrc, &pstTestGmm->stFg,
            &pstTestGmm->stBg, &pstTestGmm->stModel, &pstTestGmm->stGmmCtrl, bInstant);
    } else if (u32Mode == 1) {
        s32Ret = AX_IVE_GMM2(&IveHandle, &pstTestGmm->stSrc, &pstTestGmm->stFg,
            &pstTestGmm->stBg, &pstTestGmm->stModel, &pstTestGmm->stGmm2Ctrl, bInstant);
    }
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),AX_IVE_GMM/GMM2 failed!\n",s32Ret);
        return s32Ret;
    }
    s32Ret = AX_IVE_Query(IveHandle, &bFinish, bBlock);
    while (AX_ERR_IVE_QUERY_TIMEOUT == s32Ret) {
        usleep(1000*100);
        SAMPLE_IVE_PRT("AX_IVE_Query timeout, retry...\n");
        s32Ret = AX_IVE_Query(IveHandle, &bFinish, bBlock);
    }
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),AX_IVE_Query failed!\n",s32Ret);
        return s32Ret;
    }
    AX_U64 u64EndTime = SAMPLE_COMM_IVE_GetTime_US();
    printf("Run GMM(GMM or GMM2) task cost %lld us\n", u64EndTime - u64StartTime);

    s32Ret = SAMPLE_COMM_IVE_WriteFile(&pstTestGmm->stFg, pstTestGmm->pFpDstFg);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error,Write DstFg file failed!\n");
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_WriteFile(&pstTestGmm->stBg, pstTestGmm->pFpDstBg);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error,Write DstBg file failed!\n");
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_WriteMemInfoFile(&pstTestGmm->stModel, pstTestGmm->pFpDstModel);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error,Write model file failed!\n");
        return s32Ret;
    }

    return s32Ret;
}
/******************************************************************************
* function : Show test gmm and gmm2 sample
******************************************************************************/
AX_VOID SAMPLE_IVE_GMM_TEST(AX_U32 u32Mode, AX_S32 as32Type[], AX_CHAR **pchSrcPath, AX_CHAR **pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList)
{
    AX_S32 s32Ret;
    AX_U32 u32Width = u32WidthSrc;
    AX_U32 u32Height = u32HeightSrc;
    AX_CHAR* pchSrcFile = pchSrcPath[0];
    AX_CHAR* pchSrcModelFile = pchSrcPath[1];
    AX_CHAR* pchDstFgFile = pchDstPath[0];
    AX_CHAR* pchDstBgFile = pchDstPath[1];
    AX_CHAR* pchDstModelFile = pchDstPath[2];
    if (!pchSrcFile || !pchSrcModelFile
        || !pchDstBgFile || !pchDstFgFile || !pchDstModelFile) {
        SAMPLE_IVE_PRT("Error: null pointer(src or dst path not specified)!\n");
        return;
    }

    memset(&s_stTestGmm, 0, sizeof(TEST_GMM_T));
    s32Ret = SAMPLE_IVE_TestGmm_Init(&s_stTestGmm, pchSrcFile, pchSrcModelFile, pchDstFgFile,
        pchDstBgFile, pchDstModelFile, u32Width, u32Height, as32Type);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),SAMPLE_IVE_TestGmm_Init failed!\n", s32Ret);
        return;
    }

    s32Ret =  SAMPLE_IVE_TestGmmProc(&s_stTestGmm, u32Mode, pchParamsList);
    if (AX_SUCCESS == s32Ret)
        SAMPLE_IVE_PRT("Process success!\n");
    else
        SAMPLE_IVE_PRT("Error:process failed!\n");

    SAMPLE_IVE_TestGmm_Uninit(&s_stTestGmm);
    memset(&s_stTestGmm, 0, sizeof(TEST_GMM_T));
}

/******************************************************************************
* function : Gmm and Gmm2 Test sample signal handle
******************************************************************************/
AX_VOID SAMPLE_IVE_GMM_TEST_HandleSig(AX_VOID)
{
    SAMPLE_IVE_TestGmm_Uninit(&s_stTestGmm);
    memset(&s_stTestGmm, 0, sizeof(TEST_GMM_T));

    AX_IVE_Exit();
    AX_SYS_Deinit();
}
