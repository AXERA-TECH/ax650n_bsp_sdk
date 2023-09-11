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

typedef struct axTEST_CSC_T {
    AX_IVE_SRC_IMAGE_T stSrc;
    AX_IVE_DST_IMAGE_T stDst;
    FILE* pFpSrc;
    FILE* pFpDst;
} TEST_CSC_T;
static TEST_CSC_T s_stTestCsc;

/******************************************************************************
* function : test csc uninit
******************************************************************************/
static AX_VOID SAMPLE_IVE_TestCsc_Uninit(TEST_CSC_T* pstTestCsc)
{
    IVE_CMM_FREE(pstTestCsc->stSrc.au64PhyAddr[0], pstTestCsc->stSrc.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestCsc->stDst.au64PhyAddr[0], pstTestCsc->stDst.au64VirAddr[0]);

    if (NULL != pstTestCsc->pFpSrc) {
        fclose(pstTestCsc->pFpSrc);
        pstTestCsc->pFpSrc = NULL;
    }
    if (NULL != pstTestCsc->pFpDst) {
        fclose(pstTestCsc->pFpDst);
        pstTestCsc->pFpDst = NULL;
    }
}
/******************************************************************************
* function : test csc init
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestCsc_Init(TEST_CSC_T* pstTestCsc, AX_CHAR* pchSrcFileName,
    AX_CHAR* pchDstFileName, AX_U32 u32Width, AX_U32 u32Height, AX_S32 as32Type[])
{
    AX_S32 s32Ret = AX_FAILURE;

    memset(pstTestCsc, 0, sizeof(TEST_CSC_T));
    s32Ret = SAMPLE_COMM_IVE_CreateImage_WithGlbImgFmt(&(pstTestCsc->stSrc), (AX_IMG_FORMAT_E)as32Type[0], u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create src image failed!\n", s32Ret);
        SAMPLE_IVE_TestCsc_Uninit(pstTestCsc);
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateImage_WithGlbImgFmt(&(pstTestCsc->stDst), (AX_IMG_FORMAT_E)as32Type[1], u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create dst image failed!\n", s32Ret);
        SAMPLE_IVE_TestCsc_Uninit(pstTestCsc);
        return s32Ret;
    }

    s32Ret = AX_FAILURE;
    pstTestCsc->pFpSrc = fopen(pchSrcFileName, "rb");
    if (AX_NULL == pstTestCsc->pFpSrc) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchSrcFileName);
        SAMPLE_IVE_TestCsc_Uninit(pstTestCsc);
        return s32Ret;
    }
    pstTestCsc->pFpDst = fopen(pchDstFileName, "wb");
    if (AX_NULL == pstTestCsc->pFpDst) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchDstFileName);
        SAMPLE_IVE_TestCsc_Uninit(pstTestCsc);
        return s32Ret;
    }

    return AX_SUCCESS;
}
/******************************************************************************
* function : test csc
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestCscProc(TEST_CSC_T* pstTestCsc, AX_U32 u32Engine)
{
    AX_S32 s32Ret;
    AX_IVE_HANDLE IveHandle;
    AX_BOOL bInstant = AX_TRUE;
    s32Ret = SAMPLE_COMM_IVE_ReadFile_WithGlbImgFmt(&(pstTestCsc->stSrc), pstTestCsc->pFpSrc);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Read src file failed!\n",s32Ret);
        return s32Ret;
    }

    AX_U64 u64StartTime = SAMPLE_COMM_IVE_GetTime_US();
    s32Ret = AX_IVE_CSC(&IveHandle, &pstTestCsc->stSrc, &pstTestCsc->stDst, u32Engine, bInstant);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),AX_IVE_CSC failed!\n",s32Ret);
        return s32Ret;
    }
    AX_U64 u64EndTime = SAMPLE_COMM_IVE_GetTime_US();
    printf("Run CSC task cost %lld us\n", u64EndTime - u64StartTime);

    s32Ret = SAMPLE_COMM_IVE_WriteFile_WithGlbImgFmt(&pstTestCsc->stDst, pstTestCsc->pFpDst);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error,Write dst file failed!\n");
        return s32Ret;
    }

    return s32Ret;
}
/******************************************************************************
* function : Show test csc sample
******************************************************************************/
AX_VOID SAMPLE_IVE_CSC_TEST(AX_U32 u32Engine, AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc)
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

    memset(&s_stTestCsc, 0, sizeof(TEST_CSC_T));
    s32Ret = SAMPLE_IVE_TestCsc_Init(&s_stTestCsc, pchSrcFile, pchDstFile, u32Width, u32Height, as32Type);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),SAMPLE_IVE_TestCsc_Init failed!\n", s32Ret);
        return;
    }

    s32Ret =  SAMPLE_IVE_TestCscProc(&s_stTestCsc, u32Engine);
    if (AX_SUCCESS == s32Ret)
        SAMPLE_IVE_PRT("Process success!\n");
    else
        SAMPLE_IVE_PRT("Error:process failed!\n");

    SAMPLE_IVE_TestCsc_Uninit(&s_stTestCsc);
    memset(&s_stTestCsc, 0, sizeof(TEST_CSC_T));
}

/******************************************************************************
* function : CSC Test sample signal handle
******************************************************************************/
AX_VOID SAMPLE_IVE_CSC_TEST_HandleSig(AX_VOID)
{
    SAMPLE_IVE_TestCsc_Uninit(&s_stTestCsc);
    memset(&s_stTestCsc, 0, sizeof(TEST_CSC_T));

    AX_IVE_Exit();
    AX_SYS_Deinit();
}
