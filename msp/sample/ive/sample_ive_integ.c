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

typedef struct axTEST_INTEG_T {
    AX_IVE_SRC_IMAGE_T stSrc;
    AX_IVE_DST_IMAGE_T stDst;
    AX_IVE_INTEG_CTRL_T stIntegCtrl;
    FILE* pFpSrc;
    FILE* pFpDst;
} TEST_INTEG_T;
static TEST_INTEG_T s_stTestInteg;

/******************************************************************************
* function : parse integ parameters
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestInteg_ParseParams(TEST_INTEG_T* pstTestInteg, AX_CHAR *pchParamsList)
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
    item = cJSON_GetObjectItem(root, "out_ctl");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param out_ctl failed!\n");
    pstTestInteg->stIntegCtrl.enOutCtrl = (AX_IVE_CCL_MODE_E)item->valueint;
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
* function : test integ uninit
******************************************************************************/
static AX_VOID SAMPLE_IVE_TestInteg_Uninit(TEST_INTEG_T* pstTestInteg)
{
    IVE_CMM_FREE(pstTestInteg->stSrc.au64PhyAddr[0], pstTestInteg->stSrc.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestInteg->stDst.au64PhyAddr[0], pstTestInteg->stDst.au64VirAddr[0]);

    if (NULL != pstTestInteg->pFpSrc) {
        fclose(pstTestInteg->pFpSrc);
        pstTestInteg->pFpSrc = NULL;
    }
    if (NULL != pstTestInteg->pFpDst) {
        fclose(pstTestInteg->pFpDst);
        pstTestInteg->pFpDst = NULL;
    }

}
/******************************************************************************
* function : test integ init
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestInteg_Init(TEST_INTEG_T* pstTestInteg, AX_CHAR* pchSrcFileName,
    AX_CHAR* pchDstFileName, AX_U32 u32Width, AX_U32 u32Height, AX_S32 as32Type[])
{
    AX_S32 s32Ret = AX_FAILURE;
    memset(pstTestInteg, 0, sizeof(TEST_INTEG_T));

    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestInteg->stSrc), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[0], AX_IVE_IMAGE_TYPE_U8C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create src image failed!\n", s32Ret);
        SAMPLE_IVE_TestInteg_Uninit(pstTestInteg);
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestInteg->stDst), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[1], AX_IVE_IMAGE_TYPE_U64C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create dst image failed!\n", s32Ret);
        SAMPLE_IVE_TestInteg_Uninit(pstTestInteg);
        return s32Ret;
    }

    s32Ret = AX_FAILURE;
    pstTestInteg->pFpSrc = fopen(pchSrcFileName, "rb");
    if (AX_NULL == pstTestInteg->pFpSrc) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchSrcFileName);
        SAMPLE_IVE_TestInteg_Uninit(pstTestInteg);
        return s32Ret;
    }
    pstTestInteg->pFpDst = fopen(pchDstFileName, "wb");
    if (AX_NULL == pstTestInteg->pFpDst) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchDstFileName);
        SAMPLE_IVE_TestInteg_Uninit(pstTestInteg);
        return s32Ret;
    }

    return AX_SUCCESS;
}
/******************************************************************************
* function : test integ
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestIntegProc(TEST_INTEG_T* pstTestInteg, AX_CHAR *pchParamsList)
{
    AX_S32 s32Ret;
    AX_IVE_HANDLE IveHandle;
    AX_BOOL bInstant = AX_FALSE;
    AX_BOOL bBlock = AX_TRUE;
    AX_BOOL bFinish = AX_FALSE;

    if (pchParamsList) {
        s32Ret = SAMPLE_IVE_TestInteg_ParseParams(pstTestInteg, pchParamsList);
        if (AX_SUCCESS != s32Ret)
            return s32Ret;
    } else {
        pstTestInteg->stIntegCtrl.enOutCtrl = AX_IVE_INTEG_OUT_CTRL_COMBINE;
    }
    s32Ret = SAMPLE_COMM_IVE_ReadFile(&(pstTestInteg->stSrc), pstTestInteg->pFpSrc);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Read src file failed!\n",s32Ret);
        return s32Ret;
    }

    bInstant = AX_TRUE;
    AX_U64 u64StartTime = SAMPLE_COMM_IVE_GetTime_US();
    s32Ret = AX_IVE_Integ(&IveHandle, &pstTestInteg->stSrc, &pstTestInteg->stDst, &pstTestInteg->stIntegCtrl, bInstant);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),AX_IVE_Integ failed!\n",s32Ret);
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
    printf("Run Integ task cost %lld us\n", u64EndTime - u64StartTime);

    s32Ret = SAMPLE_COMM_IVE_WriteFile(&pstTestInteg->stDst, pstTestInteg->pFpDst);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error,Write dst file failed!\n");
        return s32Ret;
    }

    return s32Ret;
}
/******************************************************************************
* function : Show test integ calculate sample
******************************************************************************/
AX_VOID SAMPLE_IVE_Integ_TEST(AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList)
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

    memset(&s_stTestInteg, 0, sizeof(TEST_INTEG_T));
    s32Ret = SAMPLE_IVE_TestInteg_Init(&s_stTestInteg, pchSrcFile, pchDstFile, u32Width, u32Height, as32Type);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),SAMPLE_IVE_TestInteg_Init failed!\n", s32Ret);
        return;
    }

    s32Ret =  SAMPLE_IVE_TestIntegProc(&s_stTestInteg, pchParamsList);
    if (AX_SUCCESS == s32Ret)
        SAMPLE_IVE_PRT("Process success!\n");
    else
        SAMPLE_IVE_PRT("Error:process failed!\n");

    SAMPLE_IVE_TestInteg_Uninit(&s_stTestInteg);
    memset(&s_stTestInteg, 0, sizeof(TEST_INTEG_T));
}

/******************************************************************************
* function :Integ Test sample signal handle
******************************************************************************/
AX_VOID SAMPLE_IVE_Integ_TEST_HandleSig(AX_VOID)
{
    SAMPLE_IVE_TestInteg_Uninit(&s_stTestInteg);
    memset(&s_stTestInteg, 0, sizeof(TEST_INTEG_T));

    AX_IVE_Exit();
    AX_SYS_Deinit();
}
