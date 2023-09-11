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

typedef struct axTEST_SOBEL_T {
    AX_IVE_SRC_IMAGE_T stSrc;
    AX_IVE_DST_IMAGE_T stDst;
    AX_IVE_SOBEL_CTRL_T stSobelCtrl;
    FILE* pFpSrc;
    FILE* pFpDst;
} TEST_SOBEL_T;
static TEST_SOBEL_T s_stTestSobel;

/******************************************************************************
* function : parse Sobel parameters
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestSobel_ParseParams(TEST_SOBEL_T* pstTestSobel, AX_CHAR *pchParamsList)
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
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param sobel mask failed!\n");
    AX_U32 u32ArraySize = cJSON_GetArraySize(item);
    SAMPLE_IVE_CHECK_EXPR_GOTO(u32ArraySize != 25, PARSE_FAIL, "Error:u32ArraySize[%d] is not equal to 25!\n", u32ArraySize);
    for (AX_S32 i = 0; i < u32ArraySize; i++) {
        item_array = cJSON_GetArrayItem(item, i);
        pstTestSobel->stSobelCtrl.as6q10Mask[i] = item_array->valueint;
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
* function : test Sobel uninit
******************************************************************************/
static AX_VOID SAMPLE_IVE_TestSobel_Uninit(TEST_SOBEL_T* pstTestSobel)
{
    IVE_CMM_FREE(pstTestSobel->stSrc.au64PhyAddr[0], pstTestSobel->stSrc.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestSobel->stDst.au64PhyAddr[0], pstTestSobel->stDst.au64VirAddr[0]);

    if (NULL != pstTestSobel->pFpSrc) {
        fclose(pstTestSobel->pFpSrc);
        pstTestSobel->pFpSrc = NULL;
    }
    if (NULL != pstTestSobel->pFpDst) {
        fclose(pstTestSobel->pFpDst);
        pstTestSobel->pFpDst = NULL;
    }

}
/******************************************************************************
* function : test Sobel init
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestSobel_Init(TEST_SOBEL_T* pstTestSobel, AX_CHAR* pchSrcFileName,
    AX_CHAR* pchDstFileName, AX_U32 u32Width, AX_U32 u32Height, AX_S32 as32Type[])
{
    AX_S32 s32Ret = AX_FAILURE;
    memset(pstTestSobel, 0, sizeof(TEST_SOBEL_T));

    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestSobel->stSrc), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[0], AX_IVE_IMAGE_TYPE_U8C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create src image failed!\n", s32Ret);
        SAMPLE_IVE_TestSobel_Uninit(pstTestSobel);
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestSobel->stDst), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[1], AX_IVE_IMAGE_TYPE_U16C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create dst image failed!\n", s32Ret);
        SAMPLE_IVE_TestSobel_Uninit(pstTestSobel);
        return s32Ret;
    }

    s32Ret = AX_FAILURE;
    pstTestSobel->pFpSrc = fopen(pchSrcFileName, "rb");
    if (AX_NULL == pstTestSobel->pFpSrc) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchSrcFileName);
        SAMPLE_IVE_TestSobel_Uninit(pstTestSobel);
        return s32Ret;
    }
    pstTestSobel->pFpDst = fopen(pchDstFileName, "wb");
    if (AX_NULL == pstTestSobel->pFpDst) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchDstFileName);
        SAMPLE_IVE_TestSobel_Uninit(pstTestSobel);
        return s32Ret;
    }

    return AX_SUCCESS;
}

/******************************************************************************
* function : test Sobel
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestSobelProc(TEST_SOBEL_T* pstTestSobel, AX_CHAR *pchParamsList)
{
    AX_S32 s32Ret;
    AX_IVE_HANDLE IveHandle;
    AX_BOOL bInstant = AX_FALSE;
    AX_BOOL bBlock = AX_TRUE;
    AX_BOOL bFinish = AX_FALSE;

    if (pchParamsList) {
        s32Ret = SAMPLE_IVE_TestSobel_ParseParams(pstTestSobel, pchParamsList);
        if (AX_SUCCESS != s32Ret)
            return s32Ret;
    } else {
        AX_S6Q10 as6q10Mask[25] = {
            25252, 8660, -36306, -60426, 988,
            -46813, 37695, 4333, -3035, 19407,
            -25850, 4841, -28472, -55465, 32375,
            -62613, -26025, -46045, -60382, -3120,
            -39000, 30873, 14127, -44957, 18347
        };
        memcpy(pstTestSobel->stSobelCtrl.as6q10Mask, as6q10Mask, sizeof(as6q10Mask));
    }
    s32Ret = SAMPLE_COMM_IVE_ReadFile(&(pstTestSobel->stSrc), pstTestSobel->pFpSrc);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Read src file failed!\n",s32Ret);
        return s32Ret;
    }

    bInstant = AX_TRUE;
    AX_U64 u64StartTime = SAMPLE_COMM_IVE_GetTime_US();
    s32Ret = AX_IVE_Sobel(&IveHandle, &pstTestSobel->stSrc, &pstTestSobel->stDst, &pstTestSobel->stSobelCtrl, bInstant);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),AX_IVE_Sobel failed!\n",s32Ret);
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
    printf("Run Sobel task cost %lld us\n", u64EndTime - u64StartTime);

    s32Ret = SAMPLE_COMM_IVE_WriteFile(&pstTestSobel->stDst, pstTestSobel->pFpDst);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error,Write dst file failed!\n");
        return s32Ret;
    }

    return s32Ret;
}
/******************************************************************************
* function : Show test Sobel sample
******************************************************************************/
AX_VOID SAMPLE_IVE_Sobel_TEST(AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList)
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

    memset(&s_stTestSobel, 0, sizeof(TEST_SOBEL_T));
    s32Ret = SAMPLE_IVE_TestSobel_Init(&s_stTestSobel, pchSrcFile, pchDstFile, u32Width, u32Height, as32Type);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),SAMPLE_IVE_TestSobel_Init failed!\n", s32Ret);
        return;
    }

    s32Ret =  SAMPLE_IVE_TestSobelProc(&s_stTestSobel, pchParamsList);
    if (AX_SUCCESS == s32Ret)
        SAMPLE_IVE_PRT("Process success!\n");
    else
        SAMPLE_IVE_PRT("Error:process failed!\n");

    SAMPLE_IVE_TestSobel_Uninit(&s_stTestSobel);
    memset(&s_stTestSobel, 0, sizeof(TEST_SOBEL_T));
}

/******************************************************************************
* function : Sobel Test sample signal handle
******************************************************************************/
AX_VOID SAMPLE_IVE_Sobel_TEST_HandleSig(AX_VOID)
{
    SAMPLE_IVE_TestSobel_Uninit(&s_stTestSobel);
    memset(&s_stTestSobel, 0, sizeof(TEST_SOBEL_T));

    AX_IVE_Exit();
    AX_SYS_Deinit();
}
