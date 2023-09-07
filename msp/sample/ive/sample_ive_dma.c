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

typedef struct axTEST_DMA_T {
    AX_IVE_SRC_IMAGE_T stSrc;
    AX_IVE_DST_IMAGE_T stDst;
    AX_IVE_DMA_CTRL_T stDmaCtrl;
    FILE* pFpSrc;
    FILE* pFpDst;
} TEST_DMA_T;
static TEST_DMA_T s_stTestDma;
static AX_U32 u32WidthOut = 0;
static AX_U32 u32HeightOut = 0;

/******************************************************************************
* function : parse dma parameters
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestDma_ParseParams(TEST_DMA_T* pstTestDma, AX_CHAR *pchParamsList)
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
    pstTestDma->stDmaCtrl.enMode = (AX_IVE_DMA_MODE_E)item->valueint;
    switch (pstTestDma->stDmaCtrl.enMode) {
    case 0:
        item = cJSON_GetObjectItem(root, "x0");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param x0 failed!\n");
        pstTestDma->stDmaCtrl.u16CrpX0 = item->valueint;
        item = cJSON_GetObjectItem(root, "y0");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param y0 failed!\n");
        pstTestDma->stDmaCtrl.u16CrpY0 = item->valueint;
        item = cJSON_GetObjectItem(root, "w_out");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param w_out failed!\n");
        u32WidthOut = item->valueint;
        item = cJSON_GetObjectItem(root, "h_out");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param h_out failed!\n");
        u32HeightOut = item->valueint;
        break;
    case 1:
        item = cJSON_GetObjectItem(root, "h_seg");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param h_seg failed!\n");
        pstTestDma->stDmaCtrl.u8HorSegSize = item->valueint;
        item = cJSON_GetObjectItem(root, "v_seg");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param v_seg failed!\n");
        pstTestDma->stDmaCtrl.u8VerSegRows = item->valueint;
        item = cJSON_GetObjectItem(root, "elem_size");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param elem_size failed!\n");
        pstTestDma->stDmaCtrl.u8ElemSize = item->valueint;
        break;
    case 2:
    case 3:
        item = cJSON_GetObjectItem(root, "set_val");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param set_val failed!\n");
        pstTestDma->stDmaCtrl.u64Val = item->valueint;
        break;
    default :
        cJSON_Delete(root);
        SAMPLE_IVE_PRT("Error:DMA control mode %d illegal!\n", pstTestDma->stDmaCtrl.enMode);
        return AX_FAILURE;
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
* function : test dma uninit
******************************************************************************/
static AX_VOID SAMPLE_IVE_TestDma_Uninit(TEST_DMA_T* pstTestDma)
{
    IVE_CMM_FREE(pstTestDma->stSrc.au64PhyAddr[0], pstTestDma->stSrc.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestDma->stDst.au64PhyAddr[0], pstTestDma->stDst.au64VirAddr[0]);

    if (NULL != pstTestDma->pFpSrc) {
        fclose(pstTestDma->pFpSrc);
        pstTestDma->pFpSrc = NULL;
    }
    if (NULL != pstTestDma->pFpDst) {
        fclose(pstTestDma->pFpDst);
        pstTestDma->pFpDst = NULL;
    }
}
/******************************************************************************
* function : test dma init
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestDma_Init(TEST_DMA_T* pstTestDma, AX_CHAR* pchSrcFileName,
        AX_CHAR* pchDstFileName, AX_U32 u32Width, AX_U32 u32Height, AX_S32 as32Type[], AX_CHAR *pchParamsList)
{
    AX_S32 s32Ret = AX_FAILURE;
    u32WidthOut = u32Width;
    u32HeightOut = u32Height;
    memset(pstTestDma, 0, sizeof(TEST_DMA_T));

    if (pchParamsList) {
        s32Ret = SAMPLE_IVE_TestDma_ParseParams(pstTestDma, pchParamsList);
        if (AX_SUCCESS != s32Ret)
            return s32Ret;
    } else {
        pstTestDma->stDmaCtrl.enMode = AX_IVE_DMA_MODE_DIRECT_COPY;
        pstTestDma->stDmaCtrl.u64Val = 0x12345;
        pstTestDma->stDmaCtrl.u8HorSegSize = 2;//[2,255]
        pstTestDma->stDmaCtrl.u8VerSegRows = 1;//[1,255]
        pstTestDma->stDmaCtrl.u8ElemSize = 1;
        pstTestDma->stDmaCtrl.u16CrpX0 = 0;
        pstTestDma->stDmaCtrl.u16CrpY0 = 0;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestDma->stSrc), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[0], AX_IVE_IMAGE_TYPE_U8C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create src1 image failed!\n", s32Ret);
        SAMPLE_IVE_TestDma_Uninit(pstTestDma);
        return s32Ret;
    }
    /* For testing interval copy */
    if (pstTestDma->stDmaCtrl.enMode == 1) {
        /* width_out = (width_in/u8HorSegSize)*u8ElemSize */
        u32WidthOut = (u32Width / pstTestDma->stDmaCtrl.u8HorSegSize) *  pstTestDma->stDmaCtrl.u8ElemSize;
        /* height_out = height_in/u8VerSegRows */
        u32HeightOut = u32Height / pstTestDma->stDmaCtrl.u8VerSegRows;
    }
    SAMPLE_IVE_PRT("u32WidthOut:%d, u32HeightOut:%d\n", u32WidthOut, u32HeightOut);
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestDma->stDst), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[1], AX_IVE_IMAGE_TYPE_U8C1), u32WidthOut, u32HeightOut);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create dst image failed!\n", s32Ret);
        SAMPLE_IVE_TestDma_Uninit(pstTestDma);
        return s32Ret;
    }

    s32Ret = AX_FAILURE;
    pstTestDma->pFpSrc = fopen(pchSrcFileName, "rb");
    if (AX_NULL == pstTestDma->pFpSrc) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchSrcFileName);
        SAMPLE_IVE_TestDma_Uninit(pstTestDma);
        return s32Ret;
    }
    AX_CHAR FileName[256] = {0};
    snprintf(FileName, 255, "%s/dma_out_mode%d_%dx%d.bin", pchDstFileName, pstTestDma->stDmaCtrl.enMode, u32WidthOut, u32HeightOut);
    pstTestDma->pFpDst = fopen(FileName, "wb");
    if (AX_NULL == pstTestDma->pFpDst) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", FileName);
        SAMPLE_IVE_TestDma_Uninit(pstTestDma);
        return s32Ret;
    }

    return AX_SUCCESS;
}

/******************************************************************************
* function : test dma
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestDmaProc(TEST_DMA_T* pstTestDma)
{
    AX_S32 s32Ret;
    AX_IVE_HANDLE IveHandle;
    AX_BOOL bInstant = AX_FALSE;
    AX_BOOL bBlock = AX_TRUE;
    AX_BOOL bFinish = AX_FALSE;
    AX_IVE_SRC_DATA_T stSrcData;
    AX_IVE_DST_DATA_T stDstData;

    s32Ret = SAMPLE_COMM_IVE_ReadFile(&(pstTestDma->stSrc), pstTestDma->pFpSrc);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Read src file failed!\n",s32Ret);
        return s32Ret;
    }

    stSrcData.u64VirAddr = pstTestDma->stSrc.au64VirAddr[0];
    stSrcData.u64PhyAddr = pstTestDma->stSrc.au64PhyAddr[0];
    stSrcData.u32Width = pstTestDma->stSrc.u32Width;
    stSrcData.u32Height = pstTestDma->stSrc.u32Height;
    stSrcData.u32Stride = pstTestDma->stSrc.au32Stride[0];

    stDstData.u64VirAddr = pstTestDma->stDst.au64VirAddr[0];
    stDstData.u64PhyAddr = pstTestDma->stDst.au64PhyAddr[0];
    stDstData.u32Width = pstTestDma->stDst.u32Width;
    stDstData.u32Height = pstTestDma->stDst.u32Height;
    stDstData.u32Stride = pstTestDma->stDst.au32Stride[0];

    bInstant = AX_TRUE;
    AX_U64 u64StartTime = SAMPLE_COMM_IVE_GetTime_US();
    s32Ret = AX_IVE_DMA(&IveHandle, &stSrcData, &stDstData, &pstTestDma->stDmaCtrl, bInstant);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),AX_IVE_DMA failed!\n",s32Ret);
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
    printf("Run DMA task cost %lld us\n", u64EndTime - u64StartTime);

    s32Ret = SAMPLE_COMM_IVE_WriteFile(&pstTestDma->stDst, pstTestDma->pFpDst);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error,Write dst file failed!\n");
        return s32Ret;
    }

    return s32Ret;
}
/******************************************************************************
* function : Show test dma sample
******************************************************************************/
AX_VOID SAMPLE_IVE_DMA_TEST(AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList)
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
    if(AX_TRUE != SAMPLE_COMM_IVE_CheckDir(pchDstFile)) {
        SAMPLE_IVE_PRT("Error: dst must be specified as directory!\n");
        return;
    }

    memset(&s_stTestDma, 0, sizeof(TEST_DMA_T));
    s32Ret = SAMPLE_IVE_TestDma_Init(&s_stTestDma, pchSrcFile, pchDstFile, u32Width, u32Height, as32Type, pchParamsList);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),SAMPLE_IVE_TestDma_Init failed!\n", s32Ret);
        return;
    }

    s32Ret =  SAMPLE_IVE_TestDmaProc(&s_stTestDma);
    if (AX_SUCCESS == s32Ret)
        SAMPLE_IVE_PRT("Process success!\n");
    else
        SAMPLE_IVE_PRT("Error:process failed!\n");

    SAMPLE_IVE_TestDma_Uninit(&s_stTestDma);
    memset(&s_stTestDma, 0, sizeof(TEST_DMA_T));
}

/******************************************************************************
* function : DMA Test sample signal handle
******************************************************************************/
AX_VOID SAMPLE_IVE_DMA_TEST_HandleSig(AX_VOID)
{
    SAMPLE_IVE_TestDma_Uninit(&s_stTestDma);
    memset(&s_stTestDma, 0, sizeof(TEST_DMA_T));

    AX_IVE_Exit();
    AX_SYS_Deinit();
}