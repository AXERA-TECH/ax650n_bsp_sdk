/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "sample_ive.h"

typedef struct axTEST_ED_CALC_T {
    AX_IVE_SRC_IMAGE_T stSrc;
    AX_IVE_DST_IMAGE_T stDst;
    AX_U8 au8Mask[25];
    FILE* pFpSrc;
    FILE* pFpDst;
} TEST_ED_CALC_T;
static TEST_ED_CALC_T s_stTestEDCalc;

/******************************************************************************
* function : parse erode and dilate parameters
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestED_ParseParams(TEST_ED_CALC_T* pstTestEDCalce, AX_CHAR *pchParamsList)
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
    cJSON *item_array = NULL;
    item = cJSON_GetObjectItem(root, "mask");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param mask failed!\n");
    AX_U32 u32ArraySize = cJSON_GetArraySize(item);
    SAMPLE_IVE_CHECK_EXPR_GOTO(u32ArraySize != 25, PARSE_FAIL, "Error:u32ArraySize[%d] is not equal to 25!\n", u32ArraySize);
    for (AX_S32 i = 0; i < u32ArraySize; i++) {
        item_array = cJSON_GetArrayItem(item, i);
        pstTestEDCalce->au8Mask[i] = item_array->valueint;
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
* function : test erode and dilate uninit
******************************************************************************/
static AX_VOID SAMPLE_IVE_TestED_Uninit(TEST_ED_CALC_T* pstTestEDCalce)
{
    IVE_CMM_FREE(pstTestEDCalce->stSrc.au64PhyAddr[0], pstTestEDCalce->stSrc.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestEDCalce->stDst.au64PhyAddr[0], pstTestEDCalce->stDst.au64VirAddr[0]);

    if (NULL != pstTestEDCalce->pFpSrc) {
        fclose(pstTestEDCalce->pFpSrc);
        pstTestEDCalce->pFpSrc = NULL;
    }
    if (NULL != pstTestEDCalce->pFpDst) {
        fclose(pstTestEDCalce->pFpDst);
        pstTestEDCalce->pFpDst = NULL;
    }
}
/******************************************************************************
* function : test erode and dilate init
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestED_Init(TEST_ED_CALC_T* pstTestEDCalce, AX_CHAR* pchSrcFileName,
    AX_CHAR* pchDstFileName, AX_U32 u32Width, AX_U32 u32Height, AX_S32 as32Type[])
{
    AX_S32 s32Ret = AX_FAILURE;
    memset(pstTestEDCalce, 0, sizeof(TEST_ED_CALC_T));

    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestEDCalce->stSrc), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[0], AX_IVE_IMAGE_TYPE_U8C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create src image failed!\n", s32Ret);
        SAMPLE_IVE_TestED_Uninit(pstTestEDCalce);
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestEDCalce->stDst), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[1], AX_IVE_IMAGE_TYPE_U8C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create dst image failed!\n", s32Ret);
        SAMPLE_IVE_TestED_Uninit(pstTestEDCalce);
        return s32Ret;
    }

    s32Ret = AX_FAILURE;
    pstTestEDCalce->pFpSrc = fopen(pchSrcFileName, "rb");
    if (AX_NULL == pstTestEDCalce->pFpSrc) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchSrcFileName);
        SAMPLE_IVE_TestED_Uninit(pstTestEDCalce);
        return s32Ret;
    }
    pstTestEDCalce->pFpDst = fopen(pchDstFileName, "wb");
    if (AX_NULL == pstTestEDCalce->pFpDst) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchDstFileName);
        SAMPLE_IVE_TestED_Uninit(pstTestEDCalce);
        return s32Ret;
    }

    return AX_SUCCESS;
}
/******************************************************************************
* function : test erode and dilate
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestEDCalcProc(TEST_ED_CALC_T* pstTestEDCalce, AX_U32 u32Mode, AX_CHAR *pchParamsList)
{
    AX_S32 s32Ret;
    AX_IVE_HANDLE IveHandle;
    AX_BOOL bInstant = AX_FALSE;
    AX_BOOL bBlock = AX_TRUE;
    AX_BOOL bFinish = AX_FALSE;
    AX_IVE_ERODE_CTRL_T stErodeCtrl;
    AX_IVE_DILATE_CTRL_T stDilateCtrl;
    if (pchParamsList) {
        s32Ret = SAMPLE_IVE_TestED_ParseParams(pstTestEDCalce, pchParamsList);
        if (AX_SUCCESS != s32Ret)
            return s32Ret;
        if (u32Mode == 0)
            memcpy(stErodeCtrl.au8Mask, pstTestEDCalce->au8Mask, sizeof(stErodeCtrl.au8Mask));
        else
            memcpy(stDilateCtrl.au8Mask, pstTestEDCalce->au8Mask, sizeof(stDilateCtrl.au8Mask));
    } else {
        AX_U8 au8Mask[25] = {
            0, 0, 0, 0, 0,
            0, 0, 0, 0, 0,
            255, 0, 0, 255, 0,
            0, 0, 0, 255, 255,
            0, 255, 255, 255, 255
        };
        memcpy(stErodeCtrl.au8Mask, au8Mask, sizeof(stErodeCtrl.au8Mask));
        memcpy(stDilateCtrl.au8Mask, au8Mask, sizeof(stDilateCtrl.au8Mask));
    }
    s32Ret = SAMPLE_COMM_IVE_ReadFile(&(pstTestEDCalce->stSrc), pstTestEDCalce->pFpSrc);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Read src file failed!\n",s32Ret);
        return s32Ret;
    }

    bInstant = AX_TRUE;
    AX_U64 u64StartTime = SAMPLE_COMM_IVE_GetTime_US();
    if (u32Mode == 0) {
        s32Ret = AX_IVE_Erode(&IveHandle, &pstTestEDCalce->stSrc, &pstTestEDCalce->stDst, &stErodeCtrl, bInstant);
        if (AX_SUCCESS != s32Ret){
            SAMPLE_IVE_PRT("Error(%#x),AX_IVE_Erode failed!\n",s32Ret);
            return s32Ret;
        }
    } else {
        s32Ret = AX_IVE_Dilate(&IveHandle, &pstTestEDCalce->stSrc, &pstTestEDCalce->stDst, &stDilateCtrl, bInstant);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_IVE_PRT("Error(%#x),AX_IVE_Dilate failed!\n",s32Ret);
            return s32Ret;
        }
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
    printf("Run ED (Erode or Dilate) task cost %lld us\n", u64EndTime - u64StartTime);

    s32Ret = SAMPLE_COMM_IVE_WriteFile(&pstTestEDCalce->stDst, pstTestEDCalce->pFpDst);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error,Write dst file failed!\n");
        return s32Ret;
    }

    return s32Ret;
}
/******************************************************************************
* function : Show test erode and dilate calculate sample
******************************************************************************/
AX_VOID SAMPLE_IVE_ED_TEST(AX_U32 u32Mode, AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList)
{
    AX_S32 s32Ret;
    AX_U32 u32Width = u32WidthSrc;
    AX_U32 u32Height = u32HeightSrc;
    AX_CHAR* pchSrcFile = pchSrcPath;
    AX_CHAR* pchDstFile = pchDstPath;
    if (!pchSrcFile || !pchDstFile) {
        SAMPLE_IVE_PRT("Error: null pointer(src or dst path not specified)!\n");
        return;
    }

    memset(&s_stTestEDCalc, 0, sizeof(TEST_ED_CALC_T));
    s32Ret = SAMPLE_IVE_TestED_Init(&s_stTestEDCalc, pchSrcFile, pchDstFile, u32Width, u32Height, as32Type);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),SAMPLE_IVE_TestED_Init failed!\n", s32Ret);
        return;
    }

    s32Ret =  SAMPLE_IVE_TestEDCalcProc(&s_stTestEDCalc, u32Mode, pchParamsList);
    if (AX_SUCCESS == s32Ret)
        SAMPLE_IVE_PRT("Process success!\n");
    else
        SAMPLE_IVE_PRT("Error:process failed!\n");

    SAMPLE_IVE_TestED_Uninit(&s_stTestEDCalc);
    memset(&s_stTestEDCalc, 0, sizeof(TEST_ED_CALC_T));
}

/******************************************************************************
* function : Erode and Dilate Test sample signal handle
******************************************************************************/
AX_VOID SAMPLE_IVE_ED_TEST_HandleSig(AX_VOID)
{
    SAMPLE_IVE_TestED_Uninit(&s_stTestEDCalc);
    memset(&s_stTestEDCalc, 0, sizeof(TEST_ED_CALC_T));

    AX_IVE_Exit();
    AX_SYS_Deinit();
}
