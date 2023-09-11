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

typedef struct axTEST_HIST_T {
    AX_IVE_SRC_IMAGE_T stSrc;
    AX_IVE_DST_MEM_INFO_T stDst;
    AX_IVE_DST_MEM_INFO_T stDstSave;/* Only for validation results */
    AX_IVE_EQUALIZE_HIST_CTRL_T stEqualizeHistCtrl;
    FILE* pFpSrc;
    FILE* pFpDstMemInfo;
} TEST_HIST_T;
static TEST_HIST_T s_stTestHist;

/******************************************************************************
* function : parse equalize hist parameters
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestHist_ParseParams(TEST_HIST_T* pstTestHist, AX_CHAR *pchParamsList, AX_U32 u32Mode)
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
    if (u32Mode) {
        item = cJSON_GetObjectItem(root, "histeq_coef");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param histequal_coef failed!\n");
        pstTestHist->stEqualizeHistCtrl.u0q20HistEqualCoef = item->valueint;
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
* function : test hist and equalize hist uninit
******************************************************************************/
static AX_VOID SAMPLE_IVE_TestHist_Uninit(TEST_HIST_T* pstTestHist)
{
    IVE_CMM_FREE(pstTestHist->stSrc.au64PhyAddr[0], pstTestHist->stSrc.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestHist->stDst.u64PhyAddr, pstTestHist->stDst.u64VirAddr);

    if (NULL != pstTestHist->pFpSrc) {
        fclose(pstTestHist->pFpSrc);
        pstTestHist->pFpSrc = NULL;
    }
    if (NULL != pstTestHist->pFpDstMemInfo) {
        fclose(pstTestHist->pFpDstMemInfo);
        pstTestHist->pFpDstMemInfo = NULL;
    }
}
/******************************************************************************
* function : test hist and equalize hist init
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestHist_Init(TEST_HIST_T* pstTestHist, AX_CHAR* pchSrcFileName,
    AX_CHAR* pchDstMemInfoName, AX_U32 u32Width, AX_U32 u32Height, AX_U32 u32Mode, AX_S32 as32Type[])
{
    AX_S32 s32Ret = AX_FAILURE;
    /* 256 x 1 x 32 / 8 */
    AX_U32 u32Size =  AX_IVE_HIST_NUM * sizeof(AX_U32);
    memset(pstTestHist, 0, sizeof(TEST_HIST_T));

    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestHist->stSrc), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[0], AX_IVE_IMAGE_TYPE_U8C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create src image failed!\n", s32Ret);
        SAMPLE_IVE_TestHist_Uninit(pstTestHist);
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateMemInfo(&pstTestHist->stDst, u32Size);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create mem failed!\n", s32Ret);
        SAMPLE_IVE_TestHist_Uninit(pstTestHist);
        return s32Ret;
    }

    if (u32Mode == 1)
        u32Size = 256;
    else
        u32Size = 768;
    s32Ret = SAMPLE_COMM_IVE_CreateMemInfo(&pstTestHist->stDstSave, u32Size);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create mem failed!\n", s32Ret);
        SAMPLE_IVE_TestHist_Uninit(pstTestHist);
        return s32Ret;
    }

    s32Ret = AX_FAILURE;
    pstTestHist->pFpSrc = fopen(pchSrcFileName, "rb");
    if (AX_NULL == pstTestHist->pFpSrc) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchSrcFileName);
        SAMPLE_IVE_TestHist_Uninit(pstTestHist);
        return s32Ret;
    }
    pstTestHist->pFpDstMemInfo = fopen(pchDstMemInfoName, "wb");
    if (AX_NULL == pstTestHist->pFpDstMemInfo) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchDstMemInfoName);
        SAMPLE_IVE_TestHist_Uninit(pstTestHist);
        return s32Ret;
    }

    return AX_SUCCESS;
}

static AX_VOID SAMPLE_COMM_IVE_Hist_OutConvert(AX_U32 u32Mode, const AX_IVE_MEM_INFO_T *pstSrc, AX_IVE_MEM_INFO_T *pstDst)
{
    if (u32Mode == 0) {
        for (AX_S32 i = 0, j = 0; i < pstSrc->u32Size; i++, j++) {
            AX_S32 cnt = 0;
            for (;cnt < 3;) {
                *((AX_U8 *)pstDst->u64VirAddr + i) = *((AX_U8 *)pstSrc->u64VirAddr + j);
                i++;
                j++;
                cnt++;
            }
            i--;
        }
    } else if (u32Mode == 1) {
        for (AX_S32 i = 0, j = 0; i < pstSrc->u32Size; i++, j+=4) {
            *((AX_U8 *)pstDst->u64VirAddr + i) = *((AX_U8 *)pstSrc->u64VirAddr + j);
        }
    }

}
/******************************************************************************
* function : test hist and equalize hist
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestHistProc(TEST_HIST_T* pstTestHist, AX_U32 u32Mode, AX_CHAR *pchParamsList)
{
    AX_S32 s32Ret;
    AX_IVE_HANDLE IveHandle;
    AX_BOOL bInstant = AX_FALSE;
    AX_BOOL bBlock = AX_TRUE;
    AX_BOOL bFinish = AX_FALSE;

    if (pchParamsList) {
        s32Ret = SAMPLE_IVE_TestHist_ParseParams(pstTestHist, pchParamsList, u32Mode);
        if (AX_SUCCESS != s32Ret)
            return s32Ret;
    } else {
        /* u0.20 255/(w*h) */
        pstTestHist->stEqualizeHistCtrl.u0q20HistEqualCoef = 128;
    }
    s32Ret = SAMPLE_COMM_IVE_ReadFile(&(pstTestHist->stSrc), pstTestHist->pFpSrc);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Read src1 file failed!\n",s32Ret);
        return s32Ret;
    }

    bInstant = AX_TRUE;
    AX_U64 u64StartTime = SAMPLE_COMM_IVE_GetTime_US();
    if (u32Mode)
        s32Ret = AX_IVE_EqualizeHist(&IveHandle, &pstTestHist->stSrc, &pstTestHist->stDst, &pstTestHist->stEqualizeHistCtrl, bInstant);
    else
        s32Ret = AX_IVE_Hist(&IveHandle, &pstTestHist->stSrc, &pstTestHist->stDst, bInstant);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),AX_IVE_Hist/EqualizeHist failed!\n",s32Ret);
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
    printf("Run Hist(EqualizeHist or Hist) task cost %lld us\n", u64EndTime - u64StartTime);

    SAMPLE_COMM_IVE_Hist_OutConvert(u32Mode, &pstTestHist->stDst, &pstTestHist->stDstSave);

    s32Ret = SAMPLE_COMM_IVE_WriteMemInfoFile(&pstTestHist->stDstSave, pstTestHist->pFpDstMemInfo);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error,Write meminfo file failed!\n");
        return s32Ret;
    }

    return s32Ret;
}

/******************************************************************************
* function : Show test hist and equalize hist sample
******************************************************************************/
AX_VOID SAMPLE_IVE_Hist_TEST(AX_U32 u32Mode, AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList)
{
    AX_S32 s32Ret;
    AX_U32 u32Width = u32WidthSrc;
    AX_U32 u32Height = u32HeightSrc;
    AX_CHAR* pchSrcFile = pchSrcPath;
    AX_CHAR* pchMemInfoFile = pchDstPath;
    if (!pchSrcFile || !pchMemInfoFile) {
        SAMPLE_IVE_PRT("Error: null pointer(src or dst path not specified)!\n");
        return;
    }

    memset(&s_stTestHist, 0, sizeof(TEST_HIST_T));
    s32Ret = SAMPLE_IVE_TestHist_Init(&s_stTestHist, pchSrcFile, pchMemInfoFile, u32Width, u32Height, u32Mode, as32Type);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),SAMPLE_IVE_TestHist_Init failed!\n", s32Ret);
        return;
    }

    s32Ret = SAMPLE_IVE_TestHistProc(&s_stTestHist, u32Mode, pchParamsList);
    if (AX_SUCCESS == s32Ret)
        SAMPLE_IVE_PRT("Process success!\n");
    else
        SAMPLE_IVE_PRT("Error:process failed!\n");

    SAMPLE_IVE_TestHist_Uninit(&s_stTestHist);
    memset(&s_stTestHist, 0, sizeof(TEST_HIST_T));
}

/******************************************************************************
* function : Test Hist and EqualizeHist sample signal handle
******************************************************************************/
AX_VOID SAMPLE_IVE_Hist_TEST_HandleSig(AX_VOID)
{
    SAMPLE_IVE_TestHist_Uninit(&s_stTestHist);
    memset(&s_stTestHist, 0, sizeof(TEST_HIST_T));

    AX_IVE_Exit();
    AX_SYS_Deinit();
}
