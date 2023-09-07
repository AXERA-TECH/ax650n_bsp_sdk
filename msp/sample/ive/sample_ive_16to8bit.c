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

typedef struct axTEST_16TO8_T {
    AX_IVE_SRC_IMAGE_T stSrc;
    AX_IVE_DST_IMAGE_T stDst;
    AX_IVE_16BIT_TO_8BIT_CTRL_T st16BitTo8BitCtrl;
    FILE* pFpSrc;
    FILE* pFpDst;
} TEST_16TO8_T;
static TEST_16TO8_T s_stTest16To8Bit;

/******************************************************************************
* function : parse 16bit to 8bit  parameters
******************************************************************************/
static AX_S32 SAMPLE_IVE_Test16To8Bit_ParseParams(TEST_16TO8_T* pstTest16To8Bit, AX_CHAR *pchParamsList)
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
    pstTest16To8Bit->st16BitTo8BitCtrl.enMode = (AX_IVE_16BIT_TO_8BIT_MODE_E)item->valueint;
    item = cJSON_GetObjectItem(root, "gain");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param gain failed!\n");
    pstTest16To8Bit->st16BitTo8BitCtrl.s1q14Gain = item->valueint;
    item = cJSON_GetObjectItem(root, "bias");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param bias failed!\n");
    pstTest16To8Bit->st16BitTo8BitCtrl.s16Bias = item->valueint;
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
* function : test 16bit to 8bit uninit
******************************************************************************/
static AX_VOID SAMPLE_IVE_Test16To8Bit_Uninit(TEST_16TO8_T* pstTest16To8Bit)
{
    IVE_CMM_FREE(pstTest16To8Bit->stSrc.au64PhyAddr[0], pstTest16To8Bit->stSrc.au64VirAddr[0]);
    IVE_CMM_FREE(pstTest16To8Bit->stDst.au64PhyAddr[0], pstTest16To8Bit->stDst.au64VirAddr[0]);

    if (NULL != pstTest16To8Bit->pFpSrc) {
        fclose(pstTest16To8Bit->pFpSrc);
        pstTest16To8Bit->pFpSrc = NULL;
    }
    if (NULL != pstTest16To8Bit->pFpDst) {
        fclose(pstTest16To8Bit->pFpDst);
        pstTest16To8Bit->pFpDst = NULL;
    }

}
/******************************************************************************
* function : test 16bit to 8bit init
******************************************************************************/
static AX_S32 SAMPLE_IVE_Test16To8Bit_Init(TEST_16TO8_T* pstTest16To8Bit, AX_CHAR* pchSrcFileName,
    AX_CHAR* pchDstFileName, AX_U32 u32Width, AX_U32 u32Height, AX_S32 as32Type[])
{
    AX_S32 s32Ret = AX_FAILURE;
    memset(pstTest16To8Bit, 0, sizeof(TEST_16TO8_T));

    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTest16To8Bit->stSrc), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[0], AX_IVE_IMAGE_TYPE_S16C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create src image failed!\n", s32Ret);
        SAMPLE_IVE_Test16To8Bit_Uninit(pstTest16To8Bit);
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTest16To8Bit->stDst), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[1], AX_IVE_IMAGE_TYPE_S8C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create dst image failed!\n", s32Ret);
        SAMPLE_IVE_Test16To8Bit_Uninit(pstTest16To8Bit);
        return s32Ret;
    }

    s32Ret = AX_FAILURE;
    pstTest16To8Bit->pFpSrc = fopen(pchSrcFileName, "rb");
    if (AX_NULL == pstTest16To8Bit->pFpSrc) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchSrcFileName);
        SAMPLE_IVE_Test16To8Bit_Uninit(pstTest16To8Bit);
        return s32Ret;
    }
    pstTest16To8Bit->pFpDst = fopen(pchDstFileName, "wb");
    if (AX_NULL == pstTest16To8Bit->pFpDst) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchDstFileName);
        SAMPLE_IVE_Test16To8Bit_Uninit(pstTest16To8Bit);
        return s32Ret;
    }

    return AX_SUCCESS;
}
/******************************************************************************
* function : test 16bit to 8bit
******************************************************************************/
static AX_S32 SAMPLE_IVE_Test16To8BitProc(TEST_16TO8_T* pstTest16To8Bit, AX_CHAR *pchParamsList)
{
    AX_S32 s32Ret;
    AX_IVE_HANDLE IveHandle;
    AX_BOOL bInstant = AX_FALSE;
    AX_BOOL bBlock = AX_TRUE;
    AX_BOOL bFinish = AX_FALSE;

    if (pchParamsList) {
        s32Ret = SAMPLE_IVE_Test16To8Bit_ParseParams(pstTest16To8Bit, pchParamsList);
        if (AX_SUCCESS != s32Ret)
            return s32Ret;
    } else {
        pstTest16To8Bit->st16BitTo8BitCtrl.enMode = AX_IVE_16BIT_TO_8BIT_MODE_S16_TO_S8;
        pstTest16To8Bit->st16BitTo8BitCtrl.s1q14Gain = 8;
        pstTest16To8Bit->st16BitTo8BitCtrl.s16Bias = 50;
    }
    s32Ret = SAMPLE_COMM_IVE_ReadFile(&(pstTest16To8Bit->stSrc), pstTest16To8Bit->pFpSrc);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Read src file failed!\n",s32Ret);
        return s32Ret;
    }

    bInstant = AX_TRUE;
    AX_U64 u64StartTime = SAMPLE_COMM_IVE_GetTime_US();
    s32Ret = AX_IVE_16BitTo8Bit(&IveHandle, &pstTest16To8Bit->stSrc, &pstTest16To8Bit->stDst, &pstTest16To8Bit->st16BitTo8BitCtrl, bInstant);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),AX_IVE_16BitTo8Bit failed!\n",s32Ret);
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
    printf("Run 16BitTo8Bit task cost %lld us\n", u64EndTime - u64StartTime);

    s32Ret = SAMPLE_COMM_IVE_WriteFile(&pstTest16To8Bit->stDst, pstTest16To8Bit->pFpDst);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error,Write dst file failed!\n");
        return s32Ret;
    }

    return s32Ret;
}
/******************************************************************************
* function : Show test 16bit to 8bit calculate sample
******************************************************************************/
AX_VOID SAMPLE_IVE_16To8Bit_TEST(AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList)
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

    memset(&s_stTest16To8Bit, 0, sizeof(TEST_16TO8_T));
    s32Ret = SAMPLE_IVE_Test16To8Bit_Init(&s_stTest16To8Bit, pchSrcFile, pchDstFile, u32Width, u32Height, as32Type);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),SAMPLE_IVE_Test16To8Bit_Init failed!\n", s32Ret);
        return;
    }

    s32Ret =  SAMPLE_IVE_Test16To8BitProc(&s_stTest16To8Bit, pchParamsList);
    if (AX_SUCCESS == s32Ret)
        SAMPLE_IVE_PRT("Process success!\n");
    else
        SAMPLE_IVE_PRT("Error:process failed!\n");

    SAMPLE_IVE_Test16To8Bit_Uninit(&s_stTest16To8Bit);
    memset(&s_stTest16To8Bit, 0, sizeof(TEST_16TO8_T));
}

/******************************************************************************
* function :Test 16bit to 8bit sample signal handle
******************************************************************************/
AX_VOID SAMPLE_IVE_16To8Bit_TEST_HandleSig(AX_VOID)
{
    SAMPLE_IVE_Test16To8Bit_Uninit(&s_stTest16To8Bit);
    memset(&s_stTest16To8Bit, 0, sizeof(TEST_16TO8_T));

    AX_IVE_Exit();
    AX_SYS_Deinit();
}
