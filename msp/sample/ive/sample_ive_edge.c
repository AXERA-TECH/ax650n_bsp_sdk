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

typedef struct axTEST_EDGE_DETECTION_T {
    AX_IVE_SRC_IMAGE_T stSrc1;
    AX_IVE_SRC_IMAGE_T stSrc2;
    AX_IVE_DST_IMAGE_T stDst;
    AX_IVE_HYS_EDGE_CTRL_T stHysEdgeCtrl;
    AX_IVE_CANNY_EDGE_CTRL_T stCannyEdgeCtrl;
    FILE* pFpSrc1;
    FILE* pFpSrc2;
    FILE* pFpDst;
} TEST_EDGE_DETECTION_T;
static TEST_EDGE_DETECTION_T s_stTestEdgeDetection;

/******************************************************************************
* function : parse edge detection parameters
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestEdgeDetection_ParseParams(TEST_EDGE_DETECTION_T* pstTestEdgeDetection, AX_CHAR *pchParamsList, AX_U32 u32Mode)
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
        item = cJSON_GetObjectItem(root, "thr_h");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param thr_h failed!\n");
        pstTestEdgeDetection->stHysEdgeCtrl.u16HighThr = item->valueint;
        item = cJSON_GetObjectItem(root, "thr_l");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param thr_l failed!\n");
        pstTestEdgeDetection->stHysEdgeCtrl.u16LowThr = item->valueint;
    } else {
        item = cJSON_GetObjectItem(root, "thr");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param thr failed!\n");
        pstTestEdgeDetection->stCannyEdgeCtrl.u8Thr = item->valueint;
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
* function : test edge detection uninit
******************************************************************************/
static AX_VOID SAMPLE_IVE_TestEdgeDetection_Uninit(TEST_EDGE_DETECTION_T* pstTestEdgeDetection)
{
    IVE_CMM_FREE(pstTestEdgeDetection->stSrc1.au64PhyAddr[0], pstTestEdgeDetection->stSrc1.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestEdgeDetection->stSrc2.au64PhyAddr[0], pstTestEdgeDetection->stSrc2.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestEdgeDetection->stDst.au64PhyAddr[0], pstTestEdgeDetection->stDst.au64VirAddr[0]);

    if (NULL != pstTestEdgeDetection->pFpSrc1) {
        fclose(pstTestEdgeDetection->pFpSrc1);
        pstTestEdgeDetection->pFpSrc1 = NULL;
    }
    if (NULL != pstTestEdgeDetection->pFpSrc2) {
        fclose(pstTestEdgeDetection->pFpSrc2);
        pstTestEdgeDetection->pFpSrc2 = NULL;
    }
    if (NULL != pstTestEdgeDetection->pFpDst) {
        fclose(pstTestEdgeDetection->pFpDst);
        pstTestEdgeDetection->pFpDst = NULL;
    }

}
/******************************************************************************
* function : test edge detection init
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestEdgeDetection_Init(TEST_EDGE_DETECTION_T* pstTestEdgeDetection, AX_CHAR* pchSrcFile1Name,
        AX_CHAR* pchSrcFile2Name, AX_CHAR* pchDstFileName, AX_U32 u32Width, AX_U32 u32Height, AX_U32 u32Mode, AX_S32 as32Type[])
{
    AX_S32 s32Ret = AX_FAILURE;
    AX_IVE_IMAGE_TYPE_E enDstImageType = (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[1], AX_IVE_IMAGE_TYPE_U8C1);;
    memset(pstTestEdgeDetection, 0, sizeof(TEST_EDGE_DETECTION_T));

    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestEdgeDetection->stSrc1), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[0], AX_IVE_IMAGE_TYPE_U8C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create src1 image failed!\n", s32Ret);
        SAMPLE_IVE_TestEdgeDetection_Uninit(pstTestEdgeDetection);
        return s32Ret;
    }
    if (u32Mode == 0) {
        s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestEdgeDetection->stSrc2), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[1], AX_IVE_IMAGE_TYPE_U16C1), u32Width, u32Height);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_IVE_PRT("Error(%#x),Create src2 image failed!\n", s32Ret);
            SAMPLE_IVE_TestEdgeDetection_Uninit(pstTestEdgeDetection);
            return s32Ret;
        }
        enDstImageType = (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[2], AX_IVE_IMAGE_TYPE_U8C1);
    }
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestEdgeDetection->stDst), enDstImageType, u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create dst image failed!\n", s32Ret);
        SAMPLE_IVE_TestEdgeDetection_Uninit(pstTestEdgeDetection);
        return s32Ret;
    }

    s32Ret = AX_FAILURE;
    pstTestEdgeDetection->pFpSrc1 = fopen(pchSrcFile1Name, "rb");
    if (AX_NULL == pstTestEdgeDetection->pFpSrc1) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchSrcFile1Name);
        SAMPLE_IVE_TestEdgeDetection_Uninit(pstTestEdgeDetection);
        return s32Ret;
    }
    if (u32Mode == 0) {
        pstTestEdgeDetection->pFpSrc2 = fopen(pchSrcFile2Name, "rb");
        if (AX_NULL == pstTestEdgeDetection->pFpSrc2) {
            SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchSrcFile2Name);
            SAMPLE_IVE_TestEdgeDetection_Uninit(pstTestEdgeDetection);
            return s32Ret;
        }
    }
    pstTestEdgeDetection->pFpDst = fopen(pchDstFileName, "wb");
    if (AX_NULL == pstTestEdgeDetection->pFpDst) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchDstFileName);
        SAMPLE_IVE_TestEdgeDetection_Uninit(pstTestEdgeDetection);
        return s32Ret;
    }

    return AX_SUCCESS;
}
/******************************************************************************
* function : test edge detection
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestEdgeDetectionProc(TEST_EDGE_DETECTION_T* pstTestEdgeDetection, AX_U32 u32Mode, AX_CHAR *pchParamsList)
{
    AX_S32 s32Ret;
    AX_IVE_HANDLE IveHandle;
    AX_BOOL bInstant = AX_FALSE;
    AX_BOOL bBlock = AX_TRUE;
    AX_BOOL bFinish = AX_FALSE;

    if (pchParamsList) {
        s32Ret = SAMPLE_IVE_TestEdgeDetection_ParseParams(pstTestEdgeDetection, pchParamsList, u32Mode);
        if (AX_SUCCESS != s32Ret)
            return s32Ret;
    } else {
        pstTestEdgeDetection->stHysEdgeCtrl.u16HighThr = 800;
        pstTestEdgeDetection->stHysEdgeCtrl.u16LowThr = 300;
        pstTestEdgeDetection->stCannyEdgeCtrl.u8Thr = 128;
    }
    s32Ret = SAMPLE_COMM_IVE_ReadFile(&(pstTestEdgeDetection->stSrc1), pstTestEdgeDetection->pFpSrc1);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Read src1 file failed!\n",s32Ret);
        return s32Ret;
    }
    if (u32Mode == 0) {
        s32Ret = SAMPLE_COMM_IVE_ReadFile(&(pstTestEdgeDetection->stSrc2), pstTestEdgeDetection->pFpSrc2);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_IVE_PRT("Error(%#x),Read src2 file failed!\n",s32Ret);
            return s32Ret;
        }
    }

    bInstant = AX_TRUE;
    AX_U64 u64StartTime = SAMPLE_COMM_IVE_GetTime_US();
    if (u32Mode == 0) {
        s32Ret = AX_IVE_CannyHysEdge(&IveHandle, &pstTestEdgeDetection->stSrc1, &pstTestEdgeDetection->stSrc2, &pstTestEdgeDetection->stDst, &pstTestEdgeDetection->stHysEdgeCtrl, bInstant);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_IVE_PRT("Error(%#x),AX_IVE_CannyHysEdge failed!\n",s32Ret);
            return s32Ret;
        }
    } else {
        s32Ret = AX_IVE_CannyEdge(&IveHandle, &pstTestEdgeDetection->stSrc1, &pstTestEdgeDetection->stDst, &pstTestEdgeDetection->stCannyEdgeCtrl, bInstant);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_IVE_PRT("Error(%#x),AX_IVE_CannyEdge failed!\n",s32Ret);
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
    printf("Run Edge (HysEdge or CannyEdge) task cost %lld us\n", u64EndTime - u64StartTime);

    s32Ret = SAMPLE_COMM_IVE_WriteFile(&pstTestEdgeDetection->stDst, pstTestEdgeDetection->pFpDst);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error,Write dst file failed!\n");
        return s32Ret;
    }

    return s32Ret;
}

/******************************************************************************
* function : Show test edge detection (HysEdge and CannyEdge) sample
******************************************************************************/
AX_VOID SAMPLE_IVE_EdgeDetection_TEST(AX_U32 u32Mode, AX_S32 as32Type[], AX_CHAR **pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList)
{
    AX_S32 s32Ret;
    AX_U32 u32Width = u32WidthSrc;
    AX_U32 u32Height = u32HeightSrc;
    AX_CHAR* pchSrcFile1 = pchSrcPath[0];
    AX_CHAR* pchSrcFile2 = pchSrcPath[1];
    AX_CHAR* pchDstFile = pchDstPath;
    if (!pchSrcFile1 || !pchDstFile) {
        SAMPLE_IVE_PRT("Error: null pointer(src or dst path not specified)!\n");
        return;
    }
    if (u32Mode == 0 && !pchSrcFile2) {
        SAMPLE_IVE_PRT("Error: null pointer(src2 path not specified)!\n");
        return;
    }
    memset(&s_stTestEdgeDetection, 0, sizeof(TEST_EDGE_DETECTION_T));
    s32Ret = SAMPLE_IVE_TestEdgeDetection_Init(&s_stTestEdgeDetection, pchSrcFile1, pchSrcFile2, pchDstFile, u32Width, u32Height, u32Mode, as32Type);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),SAMPLE_IVE_TestEdgeDetection_Init failed!\n", s32Ret);
        return;
    }

    s32Ret = SAMPLE_IVE_TestEdgeDetectionProc(&s_stTestEdgeDetection, u32Mode, pchParamsList);
    if (AX_SUCCESS == s32Ret)
        SAMPLE_IVE_PRT("Process success!\n");
    else
        SAMPLE_IVE_PRT("Error:process failed!\n");

    SAMPLE_IVE_TestEdgeDetection_Uninit(&s_stTestEdgeDetection);
    memset(&s_stTestEdgeDetection, 0, sizeof(TEST_EDGE_DETECTION_T));
}

/******************************************************************************
* function : Test edge detection (HysEdge and CannyEdge) sample signal handle
******************************************************************************/
AX_VOID SAMPLE_IVE_EdgeDetection_TEST_HandleSig(AX_VOID)
{
    SAMPLE_IVE_TestEdgeDetection_Uninit(&s_stTestEdgeDetection);
    memset(&s_stTestEdgeDetection, 0, sizeof(TEST_EDGE_DETECTION_T));

    AX_IVE_Exit();
    AX_SYS_Deinit();
}
