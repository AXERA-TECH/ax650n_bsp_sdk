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

typedef struct axTEST_MAG_ANG_T {
    AX_IVE_SRC_IMAGE_T stSrc1; //GradH
    AX_IVE_SRC_IMAGE_T stSrc2; //GradV
    AX_IVE_DST_IMAGE_T stDst1; //MAG(AbsGrad)
    AX_IVE_DST_IMAGE_T stDst2; //ANG(Theta)
    FILE* pFpSrc1;
    FILE* pFpSrc2;
    FILE* pFpDst1;
    FILE* pFpDst2;
} TEST_MAG_ANG_T;
static TEST_MAG_ANG_T s_stTestMagAng;

/******************************************************************************
* function : test mag ang uninit
******************************************************************************/
static AX_VOID SAMPLE_IVE_TestMagAng_Uninit(TEST_MAG_ANG_T* pstTestMagAng)
{
    IVE_CMM_FREE(pstTestMagAng->stSrc1.au64PhyAddr[0], pstTestMagAng->stSrc1.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestMagAng->stSrc2.au64PhyAddr[0], pstTestMagAng->stSrc2.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestMagAng->stDst1.au64PhyAddr[0], pstTestMagAng->stDst1.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestMagAng->stDst2.au64PhyAddr[0], pstTestMagAng->stDst2.au64VirAddr[0]);

    if (NULL != pstTestMagAng->pFpSrc1) {
        fclose(pstTestMagAng->pFpSrc1);
        pstTestMagAng->pFpSrc1 = NULL;
    }
    if (NULL != pstTestMagAng->pFpSrc2) {
        fclose(pstTestMagAng->pFpSrc2);
        pstTestMagAng->pFpSrc2 = NULL;
    }
    if (NULL != pstTestMagAng->pFpDst1) {
        fclose(pstTestMagAng->pFpDst1);
        pstTestMagAng->pFpDst1 = NULL;
    }
    if (NULL != pstTestMagAng->pFpDst2) {
        fclose(pstTestMagAng->pFpDst2);
        pstTestMagAng->pFpDst2 = NULL;
    }

}
/******************************************************************************
* function : test mag ang init
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestMagAng_Init(TEST_MAG_ANG_T* pstTestMagAng, AX_CHAR* pchSrcFile1Name, AX_CHAR* pchSrcFile2Name,
    AX_CHAR* pchDstFile1Name,  AX_CHAR* pchDstFile2Name, AX_U32 u32Width, AX_U32 u32Height, AX_S32 as32Type[])
{
    AX_S32 s32Ret = AX_FAILURE;
    memset(pstTestMagAng, 0, sizeof(TEST_MAG_ANG_T));

    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestMagAng->stSrc1), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[0], AX_IVE_IMAGE_TYPE_U16C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create src1 image failed!\n", s32Ret);
        SAMPLE_IVE_TestMagAng_Uninit(pstTestMagAng);
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestMagAng->stSrc2), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[1], AX_IVE_IMAGE_TYPE_U16C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create src2 image failed!\n", s32Ret);
        SAMPLE_IVE_TestMagAng_Uninit(pstTestMagAng);
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestMagAng->stDst1), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[2], AX_IVE_IMAGE_TYPE_U16C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create dst1 image failed!\n", s32Ret);
        SAMPLE_IVE_TestMagAng_Uninit(pstTestMagAng);
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestMagAng->stDst2), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[3], AX_IVE_IMAGE_TYPE_U8C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create dst2 image failed!\n", s32Ret);
        SAMPLE_IVE_TestMagAng_Uninit(pstTestMagAng);
        return s32Ret;
    }

    s32Ret = AX_FAILURE;
    pstTestMagAng->pFpSrc1 = fopen(pchSrcFile1Name, "rb");
    if (AX_NULL == pstTestMagAng->pFpSrc1) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchSrcFile1Name);
        SAMPLE_IVE_TestMagAng_Uninit(pstTestMagAng);
        return s32Ret;
    }
    pstTestMagAng->pFpSrc2 = fopen(pchSrcFile2Name, "rb");
    if (AX_NULL == pstTestMagAng->pFpSrc2) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchSrcFile2Name);
        SAMPLE_IVE_TestMagAng_Uninit(pstTestMagAng);
        return s32Ret;
    }
    pstTestMagAng->pFpDst1 = fopen(pchDstFile1Name, "wb");
    if (AX_NULL == pstTestMagAng->pFpDst1) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchDstFile1Name);
        SAMPLE_IVE_TestMagAng_Uninit(pstTestMagAng);
        return s32Ret;
    }
    pstTestMagAng->pFpDst2 = fopen(pchDstFile2Name, "wb");
    if (AX_NULL == pstTestMagAng->pFpDst2) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchDstFile2Name);
        SAMPLE_IVE_TestMagAng_Uninit(pstTestMagAng);
        return s32Ret;
    }

    return AX_SUCCESS;
}
/******************************************************************************
* function : test mag ang
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestMagAngProc(TEST_MAG_ANG_T* pstTestMagAng)
{
    AX_S32 s32Ret;
    AX_IVE_HANDLE IveHandle;
    AX_BOOL bInstant = AX_FALSE;
    AX_BOOL bBlock = AX_TRUE;
    AX_BOOL bFinish = AX_FALSE;

    s32Ret = SAMPLE_COMM_IVE_ReadFile(&(pstTestMagAng->stSrc1), pstTestMagAng->pFpSrc1);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Read src1 file failed!\n",s32Ret);
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_ReadFile(&(pstTestMagAng->stSrc2), pstTestMagAng->pFpSrc2);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Read src2 file failed!\n",s32Ret);
        return s32Ret;
    }

    bInstant = AX_TRUE;
    AX_U64 u64StartTime = SAMPLE_COMM_IVE_GetTime_US();
    s32Ret = AX_IVE_MagAndAng(&IveHandle, &pstTestMagAng->stSrc1, &pstTestMagAng->stSrc2,
        &pstTestMagAng->stDst1, &pstTestMagAng->stDst2, bInstant);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),AX_IVE_MagAndAng failed!\n",s32Ret);
        return s32Ret;
    }
    if (bInstant == AX_FALSE) {
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
    }
    AX_U64 u64EndTime = SAMPLE_COMM_IVE_GetTime_US();
    printf("Run MagAndAng task cost %lld us\n", u64EndTime - u64StartTime);

    s32Ret = SAMPLE_COMM_IVE_WriteFile(&pstTestMagAng->stDst1, pstTestMagAng->pFpDst1);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error,Write dst1 file failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_IVE_WriteFile(&pstTestMagAng->stDst2, pstTestMagAng->pFpDst2);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error,Write dst2 file failed!\n");
        return s32Ret;
    }
    return s32Ret;
}
/******************************************************************************
* function : Show test mag ang calculate sample
******************************************************************************/
AX_VOID SAMPLE_IVE_MagAng_TEST(AX_S32 as32Type[], AX_CHAR **pchSrcPath, AX_CHAR **pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc)
{
    AX_S32 s32Ret;
    AX_U32 u32Width = u32WidthSrc;
    AX_U32 u32Height = u32HeightSrc;
    AX_CHAR* pchSrcFile1 = pchSrcPath[0];
    AX_CHAR* pchSrcFile2 = pchSrcPath[1];
    AX_CHAR* pchDstFile1 = pchDstPath[0];
    AX_CHAR* pchDstFile2 = pchDstPath[1];
    if (!pchSrcFile1 || !pchSrcFile2
        || !pchDstFile1 || !pchDstFile2) {
        SAMPLE_IVE_PRT("Error: null pointer(src or dst path not specified)!\n");
        return;
    }

    memset(&s_stTestMagAng, 0, sizeof(TEST_MAG_ANG_T));
    s32Ret = SAMPLE_IVE_TestMagAng_Init(&s_stTestMagAng, pchSrcFile1,  pchSrcFile2,
        pchDstFile1, pchDstFile2, u32Width, u32Height, as32Type);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),SAMPLE_IVE_TestMagAng_Init failed!\n", s32Ret);
        return;
    }

    s32Ret =  SAMPLE_IVE_TestMagAngProc(&s_stTestMagAng);
    if (AX_SUCCESS == s32Ret)
        SAMPLE_IVE_PRT("Process success!\n");
    else
        SAMPLE_IVE_PRT("Error:process failed!\n");

    SAMPLE_IVE_TestMagAng_Uninit(&s_stTestMagAng);
    memset(&s_stTestMagAng, 0, sizeof(TEST_MAG_ANG_T));
}

/******************************************************************************
* function :Mag Ang Test sample signal handle
******************************************************************************/
AX_VOID SAMPLE_IVE_MagAng_TEST_HandleSig(AX_VOID)
{
    SAMPLE_IVE_TestMagAng_Uninit(&s_stTestMagAng);
    memset(&s_stTestMagAng, 0, sizeof(TEST_MAG_ANG_T));

    AX_IVE_Exit();
    AX_SYS_Deinit();
}
