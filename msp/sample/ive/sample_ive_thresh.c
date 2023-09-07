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

typedef struct axTEST_THRESH_T {
    AX_IVE_SRC_IMAGE_T stSrc;
    AX_IVE_DST_IMAGE_T stDst;
    AX_IVE_THRESH_CTRL_T stThrCtrl;
    FILE* pFpSrc;
    FILE* pFpDst;
} TEST_THRESH_T;
static TEST_THRESH_T s_stTestThresh;

/******************************************************************************
* function : parse thresh parameters
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestThresh_ParseParams(TEST_THRESH_T* pstTestThresh, AX_CHAR *pchParamsList)
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
    item = cJSON_GetObjectItem(root, "mode");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param mode failed!\n");
    pstTestThresh->stThrCtrl.enMode = (AX_IVE_THRESH_MODE_E)item->valueint;
    item = cJSON_GetObjectItem(root, "thr_l");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param thr_l failed!\n");
    pstTestThresh->stThrCtrl.u8LowThr = item->valueint;
    item = cJSON_GetObjectItem(root, "thr_h");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param thr_h failed!\n");
    pstTestThresh->stThrCtrl.u8HighThr = item->valueint;
    item = cJSON_GetObjectItem(root, "min_val");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param min_val failed!\n");
    pstTestThresh->stThrCtrl.u8MinVal = item->valueint;
    item = cJSON_GetObjectItem(root, "mid_val");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param mid_val failed!\n");
    pstTestThresh->stThrCtrl.u8MidVal = item->valueint;
    item = cJSON_GetObjectItem(root, "max_val");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param max_val failed!\n");
    pstTestThresh->stThrCtrl.u8MaxVal = item->valueint;
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
* function : test thresh uninit
******************************************************************************/
static AX_VOID SAMPLE_IVE_TestThresh_Uninit(TEST_THRESH_T* pstTestThresh)
{
    IVE_CMM_FREE(pstTestThresh->stSrc.au64PhyAddr[0], pstTestThresh->stSrc.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestThresh->stDst.au64PhyAddr[0], pstTestThresh->stDst.au64VirAddr[0]);

    if (NULL != pstTestThresh->pFpSrc) {
        fclose(pstTestThresh->pFpSrc);
        pstTestThresh->pFpSrc = NULL;
    }
    if (NULL != pstTestThresh->pFpDst) {
        fclose(pstTestThresh->pFpDst);
        pstTestThresh->pFpDst = NULL;
    }

}
/******************************************************************************
* function : test thresh init
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestThresh_Init(TEST_THRESH_T* pstTestThresh, AX_CHAR* pchSrcFileName,
    AX_CHAR* pchDstFileName, AX_U32 u32Width, AX_U32 u32Height, AX_S32 as32Type[])
{
    AX_S32 s32Ret = AX_FAILURE;
    memset(pstTestThresh, 0, sizeof(TEST_THRESH_T));

    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestThresh->stSrc), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[0], AX_IVE_IMAGE_TYPE_U8C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create src image failed!\n", s32Ret);
        SAMPLE_IVE_TestThresh_Uninit(pstTestThresh);
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestThresh->stDst), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[1], AX_IVE_IMAGE_TYPE_U8C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create dst image failed!\n", s32Ret);
        SAMPLE_IVE_TestThresh_Uninit(pstTestThresh);
        return s32Ret;
    }

    s32Ret = AX_FAILURE;
    pstTestThresh->pFpSrc = fopen(pchSrcFileName, "rb");
    if (AX_NULL == pstTestThresh->pFpSrc) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchSrcFileName);
        SAMPLE_IVE_TestThresh_Uninit(pstTestThresh);
        return s32Ret;
    }
    pstTestThresh->pFpDst = fopen(pchDstFileName, "wb");
    if (AX_NULL == pstTestThresh->pFpDst) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchDstFileName);
        SAMPLE_IVE_TestThresh_Uninit(pstTestThresh);
        return s32Ret;
    }

    return AX_SUCCESS;
}
/******************************************************************************
* function : test thresh
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestThreshProc(TEST_THRESH_T* pstTestThresh, AX_CHAR *pchParamsList)
{
    AX_S32 s32Ret;
    AX_IVE_HANDLE IveHandle;
    AX_BOOL bInstant = AX_FALSE;
    AX_BOOL bBlock = AX_TRUE;
    AX_BOOL bFinish = AX_FALSE;

    if (pchParamsList) {
        s32Ret = SAMPLE_IVE_TestThresh_ParseParams(pstTestThresh, pchParamsList);
        if (AX_SUCCESS != s32Ret)
            return s32Ret;
    } else {
        pstTestThresh->stThrCtrl.enMode = AX_IVE_THRESH_MODE_BINARY;
        pstTestThresh->stThrCtrl.u8HighThr = 65;
        pstTestThresh->stThrCtrl.u8LowThr = 42;
        pstTestThresh->stThrCtrl.u8MinVal = 32;
        pstTestThresh->stThrCtrl.u8MidVal = 49;
        pstTestThresh->stThrCtrl.u8MaxVal = 199;
    }
    s32Ret = SAMPLE_COMM_IVE_ReadFile(&(pstTestThresh->stSrc), pstTestThresh->pFpSrc);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Read src file failed!\n",s32Ret);
        return s32Ret;
    }

    bInstant = AX_TRUE;
    AX_U64 u64StartTime = SAMPLE_COMM_IVE_GetTime_US();
    s32Ret = AX_IVE_Thresh(&IveHandle, &pstTestThresh->stSrc, &pstTestThresh->stDst, &pstTestThresh->stThrCtrl, bInstant);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),AX_IVE_Thresh failed!\n",s32Ret);
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
    printf("Run Thresh task cost %lld us\n", u64EndTime - u64StartTime);

    s32Ret = SAMPLE_COMM_IVE_WriteFile(&pstTestThresh->stDst, pstTestThresh->pFpDst);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error,Write dst file failed!\n");
        return s32Ret;
    }

    return s32Ret;
}
/******************************************************************************
* function : Show test thresh calculate sample
******************************************************************************/
AX_VOID SAMPLE_IVE_Thresh_TEST(AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList)
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

    memset(&s_stTestThresh, 0, sizeof(TEST_THRESH_T));
    s32Ret = SAMPLE_IVE_TestThresh_Init(&s_stTestThresh, pchSrcFile, pchDstFile, u32Width, u32Height, as32Type);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),SAMPLE_IVE_TestThresh_Init failed!\n", s32Ret);
        return;
    }

    s32Ret =  SAMPLE_IVE_TestThreshProc(&s_stTestThresh, pchParamsList);
    if (AX_SUCCESS == s32Ret)
        SAMPLE_IVE_PRT("Process success!\n");
    else
        SAMPLE_IVE_PRT("Error:process failed!\n");

    SAMPLE_IVE_TestThresh_Uninit(&s_stTestThresh);
    memset(&s_stTestThresh, 0, sizeof(TEST_THRESH_T));
}

/******************************************************************************
* function :Thresh Test sample signal handle
******************************************************************************/
AX_VOID SAMPLE_IVE_Thresh_TEST_HandleSig(AX_VOID)
{
    SAMPLE_IVE_TestThresh_Uninit(&s_stTestThresh);
    memset(&s_stTestThresh, 0, sizeof(TEST_THRESH_T));

    AX_IVE_Exit();
    AX_SYS_Deinit();
}
