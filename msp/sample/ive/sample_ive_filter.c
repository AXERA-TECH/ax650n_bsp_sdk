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

typedef struct axTEST_FILTER_T {
    AX_IVE_SRC_IMAGE_T stSrc;
    AX_IVE_DST_IMAGE_T stDst;
    AX_IVE_FILTER_CTRL_T stFltCtrl;
    FILE* pFpSrc;
    FILE* pFpDst;
} TEST_FILTER_T;
static TEST_FILTER_T s_stTestFilter;

/******************************************************************************
* function : parse filter parameters
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestFilter_ParseParams(TEST_FILTER_T* pstTestFilter, AX_CHAR *pchParamsList)
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
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param fiter mask failed!\n");
    AX_U32 u32ArraySize = cJSON_GetArraySize(item);
    SAMPLE_IVE_CHECK_EXPR_GOTO(u32ArraySize != 25, PARSE_FAIL, "Error:u32ArraySize[%d] is not equal to 25!\n", u32ArraySize);
    for (AX_S32 i = 0; i < u32ArraySize; i++) {
        item_array = cJSON_GetArrayItem(item, i);
        pstTestFilter->stFltCtrl.as6q10Mask[i] = item_array->valueint;
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
* function : test filter uninit
******************************************************************************/
static AX_VOID SAMPLE_IVE_TestFilter_Uninit(TEST_FILTER_T* pstTestFilter)
{
    IVE_CMM_FREE(pstTestFilter->stSrc.au64PhyAddr[0], pstTestFilter->stSrc.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestFilter->stDst.au64PhyAddr[0], pstTestFilter->stDst.au64VirAddr[0]);

    if (NULL != pstTestFilter->pFpSrc) {
        fclose(pstTestFilter->pFpSrc);
        pstTestFilter->pFpSrc = NULL;
    }
    if (NULL != pstTestFilter->pFpDst) {
        fclose(pstTestFilter->pFpDst);
        pstTestFilter->pFpDst = NULL;
    }

}
/******************************************************************************
* function : test filter init
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestFilter_Init(TEST_FILTER_T* pstTestFilter, AX_CHAR* pchSrcFileName,
    AX_CHAR* pchDstFileName, AX_U32 u32Width, AX_U32 u32Height, AX_S32 as32Type[])
{
    AX_S32 s32Ret = AX_FAILURE;
    memset(pstTestFilter, 0, sizeof(TEST_FILTER_T));

    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestFilter->stSrc), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[0], AX_IVE_IMAGE_TYPE_U8C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create src image failed!\n", s32Ret);
        SAMPLE_IVE_TestFilter_Uninit(pstTestFilter);
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestFilter->stDst), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[1], AX_IVE_IMAGE_TYPE_U8C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create dst image failed!\n", s32Ret);
        SAMPLE_IVE_TestFilter_Uninit(pstTestFilter);
        return s32Ret;
    }

    s32Ret = AX_FAILURE;
    pstTestFilter->pFpSrc = fopen(pchSrcFileName, "rb");
    if (AX_NULL == pstTestFilter->pFpSrc) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchSrcFileName);
        SAMPLE_IVE_TestFilter_Uninit(pstTestFilter);
        return s32Ret;
    }
    pstTestFilter->pFpDst = fopen(pchDstFileName, "wb");
    if (AX_NULL == pstTestFilter->pFpDst) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchDstFileName);
        SAMPLE_IVE_TestFilter_Uninit(pstTestFilter);
        return s32Ret;
    }

    return AX_SUCCESS;
}

/******************************************************************************
* function : test filter
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestFilterProc(TEST_FILTER_T* pstTestFilter, AX_CHAR *pchParamsList)
{
    AX_S32 s32Ret;
    AX_IVE_HANDLE IveHandle;
    AX_BOOL bInstant = AX_FALSE;
    AX_BOOL bBlock = AX_TRUE;
    AX_BOOL bFinish = AX_FALSE;

    if (pchParamsList) {
        s32Ret = SAMPLE_IVE_TestFilter_ParseParams(pstTestFilter, pchParamsList);
        if (AX_SUCCESS != s32Ret)
            return s32Ret;
    } else {
        AX_S6Q10 as6q10Mask[25] = {
            -60926, -48381, -47545, 33147, 49431,
            63723, -64488, 37430, -27681, -48659,
            19031, -16709, 26667, 2023, 51858,
            -34899, 19455, 19648, 55833, -2102,
            47260, -11678, 13204, 20532, -2051
        };
        memcpy(pstTestFilter->stFltCtrl.as6q10Mask, as6q10Mask, sizeof(as6q10Mask));
    }
    s32Ret = SAMPLE_COMM_IVE_ReadFile(&(pstTestFilter->stSrc), pstTestFilter->pFpSrc);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Read src file failed!\n",s32Ret);
        return s32Ret;
    }

    bInstant = AX_TRUE;
    AX_U64 u64StartTime = SAMPLE_COMM_IVE_GetTime_US();
    s32Ret = AX_IVE_Filter(&IveHandle, &pstTestFilter->stSrc, &pstTestFilter->stDst, &pstTestFilter->stFltCtrl, bInstant);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),AX_IVE_Filter failed!\n",s32Ret);
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
    printf("Run Filter task cost %lld us\n", u64EndTime - u64StartTime);

    s32Ret = SAMPLE_COMM_IVE_WriteFile(&pstTestFilter->stDst, pstTestFilter->pFpDst);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error,Write dst file failed!\n");
        return s32Ret;
    }

    return s32Ret;
}
/******************************************************************************
* function : Show test filter sample
******************************************************************************/
AX_VOID SAMPLE_IVE_Filter_TEST(AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList)
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

    memset(&s_stTestFilter, 0, sizeof(TEST_FILTER_T));
    s32Ret = SAMPLE_IVE_TestFilter_Init(&s_stTestFilter, pchSrcFile, pchDstFile, u32Width, u32Height, as32Type);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),SAMPLE_IVE_TestFilter_Init failed!\n", s32Ret);
        return;
    }

    s32Ret =  SAMPLE_IVE_TestFilterProc(&s_stTestFilter, pchParamsList);
    if (AX_SUCCESS == s32Ret)
        SAMPLE_IVE_PRT("Process success!\n");
    else
        SAMPLE_IVE_PRT("Error:process failed!\n");

    SAMPLE_IVE_TestFilter_Uninit(&s_stTestFilter);
    memset(&s_stTestFilter, 0, sizeof(TEST_FILTER_T));
}

/******************************************************************************
* function : Filter Test sample signal handle
******************************************************************************/
AX_VOID SAMPLE_IVE_Filter_TEST_HandleSig(AX_VOID)
{
    SAMPLE_IVE_TestFilter_Uninit(&s_stTestFilter);
    memset(&s_stTestFilter, 0, sizeof(TEST_FILTER_T));

    AX_IVE_Exit();
    AX_SYS_Deinit();
}
