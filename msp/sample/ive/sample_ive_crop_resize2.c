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

#define  MAX_OUTPUT_NUM  (32)

typedef struct axTEST_CROP_RESIZE2_T {
    AX_IVE_SRC_IMAGE_T stSrc;
    AX_IVE_IMAGE_T stDst[MAX_OUTPUT_NUM];
    AX_IVE_SRC_IMAGE_T stSrcY;
    AX_IVE_SRC_IMAGE_T stSrcUV;
    AX_IVE_DST_IMAGE_T stDstY[MAX_OUTPUT_NUM];
    AX_IVE_DST_IMAGE_T stDstUV[MAX_OUTPUT_NUM];
    AX_IVE_CROP_IMAGE_CTRL_T stCropResize2Ctrl;
    FILE* pFpSrc;
    FILE* pFpBg[MAX_OUTPUT_NUM];
    FILE* pFpDst[MAX_OUTPUT_NUM];
    AX_IVE_RECT_U16_T stSrcBox[MAX_OUTPUT_NUM];
    AX_IVE_RECT_U16_T stDstBox[MAX_OUTPUT_NUM];
} TEST_CROP_RESIZE2_T;
static TEST_CROP_RESIZE2_T s_stTestCropResize2;
static AX_U32 u32OutputNum = 0;
static AX_U32 u32WidthOut[MAX_OUTPUT_NUM] = {0};
static AX_U32 u32HeightOut[MAX_OUTPUT_NUM] = {0};

/******************************************************************************
* function : parse crop resize2 parameters
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestCropResize2_ParseParams(TEST_CROP_RESIZE2_T* pstTestCropResize2, AX_CHAR *pchParamsList)
{
    cJSON *root = NULL;
    AX_CHAR *buf = NULL;
    FILE *fp = fopen(pchParamsList, "r");
    if (!fp) {
        root = cJSON_Parse(pchParamsList);
        if (!root) {
            SAMPLE_IVE_PRT("Error:parse parameters from string %s failed!\n", pchParamsList);
            return AX_FAILURE;
        }
    } else {
        fseek(fp, 0, SEEK_END);
        AX_LONG file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        buf = (AX_CHAR*)malloc(file_size + 1);
        if (buf) {
            fread(buf, 1, file_size, fp);
            root = cJSON_Parse(buf);
            if (!root) {
                SAMPLE_IVE_PRT("Error:parse parameters from file %s failed!\n", pchParamsList);
                return AX_FAILURE;
            }
        }
    }

    cJSON *item = NULL;
    cJSON *item_array = NULL;
    item = cJSON_GetObjectItem(root, "num");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param num failed!\n");
    u32OutputNum = item->valueint;
    SAMPLE_IVE_CHECK_EXPR_GOTO((u32OutputNum > MAX_OUTPUT_NUM), PARSE_FAIL, "Error:u32OutputNum[%d] cannot exceed %d!\n", u32OutputNum, MAX_OUTPUT_NUM);

    pstTestCropResize2->stCropResize2Ctrl.u16Num = u32OutputNum;
    item_array = cJSON_GetObjectItem(root, "res_out");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item_array, PARSE_FAIL, "Error:parse param res_out failed!\n");
    AX_U32 u32ArraySize = cJSON_GetArraySize(item_array);
    SAMPLE_IVE_CHECK_EXPR_GOTO(u32ArraySize != u32OutputNum, PARSE_FAIL,
        "Error:u32ArraySize[%d] is not equal to %d!\n",u32ArraySize, u32OutputNum);
    cJSON *item_sub_res_out = item_array->child;
    AX_S32 i = 0;
    for (;item_sub_res_out;) {
        cJSON *item_rect = cJSON_GetObjectItem(item_sub_res_out, "w");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item_rect, PARSE_FAIL, "Error:parse param w failed!\n");
        u32WidthOut[i] = item_rect->valueint;
        item_rect = cJSON_GetObjectItem(item_sub_res_out, "h");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item_rect, PARSE_FAIL, "Error:parse param h failed!\n");
        u32HeightOut[i] = item_rect->valueint;
        item_sub_res_out = item_sub_res_out->next;
        SAMPLE_IVE_PRT("ResOut[%d]:%dx%d \n", i, u32WidthOut[i], u32HeightOut[i]);
        i++;
    }
    item_array = cJSON_GetObjectItem(root, "src_boxs");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item_array, PARSE_FAIL, "Error:parse param src_boxs failed!\n");
    u32ArraySize = cJSON_GetArraySize(item_array);
    SAMPLE_IVE_CHECK_EXPR_GOTO(u32ArraySize != u32OutputNum, PARSE_FAIL,
        "Error:u32ArraySize[%d] is not equal to %d!\n",u32ArraySize, u32OutputNum);
    cJSON *item_sub = item_array->child;
    i = 0;
    for (;item_sub;) {
        cJSON *item_rect = cJSON_GetObjectItem(item_sub, "x");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item_rect, PARSE_FAIL, "Error:parse param x failed!\n");
        pstTestCropResize2->stSrcBox[i].u16X = item_rect->valueint;
        item_rect = cJSON_GetObjectItem(item_sub, "y");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item_rect, PARSE_FAIL, "Error:parse param y failed!\n");
        pstTestCropResize2->stSrcBox[i].u16Y = item_rect->valueint;
        item_rect = cJSON_GetObjectItem(item_sub, "w");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item_rect, PARSE_FAIL, "Error:parse param w failed!\n");
        pstTestCropResize2->stSrcBox[i].u16Width = item_rect->valueint;
        item_rect = cJSON_GetObjectItem(item_sub, "h");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item_rect, PARSE_FAIL, "Error:parse param h failed!\n");
        pstTestCropResize2->stSrcBox[i].u16Height = item_rect->valueint;
        item_sub = item_sub->next;
        SAMPLE_IVE_PRT("stSrcBox[%d]: [%d %d %d %d]\n", i, pstTestCropResize2->stSrcBox[i].u16X, pstTestCropResize2->stSrcBox[i].u16Y,
        pstTestCropResize2->stSrcBox[i].u16Width, pstTestCropResize2->stSrcBox[i].u16Height);
        i++;
    }

    item_array = cJSON_GetObjectItem(root, "dst_boxs");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item_array, PARSE_FAIL, "Error:parse param dst_boxs failed!\n");
    u32ArraySize = cJSON_GetArraySize(item_array);
    SAMPLE_IVE_CHECK_EXPR_GOTO(u32ArraySize != u32OutputNum, PARSE_FAIL,
        "Error:u32ArraySize[%d] is not equal to %d!\n",u32ArraySize, u32OutputNum);
    cJSON *item_sub_dst = item_array->child;
    i = 0;
    for (;item_sub_dst;) {
        cJSON *item_rect_dst = cJSON_GetObjectItem(item_sub_dst, "x");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item_rect_dst, PARSE_FAIL, "Error:parse param x failed!\n");
        pstTestCropResize2->stDstBox[i].u16X = item_rect_dst->valueint;
        item_rect_dst = cJSON_GetObjectItem(item_sub_dst, "y");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item_rect_dst, PARSE_FAIL, "Error:parse param y failed!\n");
        pstTestCropResize2->stDstBox[i].u16Y = item_rect_dst->valueint;
        item_rect_dst = cJSON_GetObjectItem(item_sub_dst, "w");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item_rect_dst, PARSE_FAIL, "Error:parse param w failed!\n");
        pstTestCropResize2->stDstBox[i].u16Width = item_rect_dst->valueint;
        item_rect_dst = cJSON_GetObjectItem(item_sub_dst, "h");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item_rect_dst, PARSE_FAIL, "Error:parse param h failed!\n");
        pstTestCropResize2->stDstBox[i].u16Height = item_rect_dst->valueint;
        item_sub_dst = item_sub_dst->next;
        SAMPLE_IVE_PRT("stDstBox[%d]: [%d %d %d %d]\n", i, pstTestCropResize2->stDstBox[i].u16X, pstTestCropResize2->stDstBox[i].u16Y,
        pstTestCropResize2->stDstBox[i].u16Width, pstTestCropResize2->stDstBox[i].u16Height);
        i++;
    }

    cJSON_Delete(root);
    if(fp)
        fclose(fp);
    if(buf) {
        free(buf);
        SAMPLE_IVE_PRT("free buf\n");
    }
    SAMPLE_IVE_PRT("Parse params success!\n");
    return AX_SUCCESS;

PARSE_FAIL:
    cJSON_Delete(root);
    if(fp)
        fclose(fp);
    if(buf) {
        free(buf);
        SAMPLE_IVE_PRT("free buf\n");
    }
    return AX_FAILURE;
}
/******************************************************************************
* function : test crop resize2 uninit
******************************************************************************/
static AX_VOID SAMPLE_IVE_TestCropResize2_Uninit(TEST_CROP_RESIZE2_T* pstTestCropResize2)
{
    IVE_CMM_FREE(pstTestCropResize2->stSrc.au64PhyAddr[0], pstTestCropResize2->stSrc.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestCropResize2->stSrcY.au64PhyAddr[0], pstTestCropResize2->stSrcY.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestCropResize2->stSrcUV.au64PhyAddr[0], pstTestCropResize2->stSrcUV.au64VirAddr[0]);
    for (AX_S32 i = 0; i < u32OutputNum; i++) {
        IVE_CMM_FREE(pstTestCropResize2->stDst[i].au64PhyAddr[0], pstTestCropResize2->stDst[i].au64VirAddr[0]);
        IVE_CMM_FREE(pstTestCropResize2->stDstY[i].au64PhyAddr[0], pstTestCropResize2->stDstY[i].au64VirAddr[0]);
        IVE_CMM_FREE(pstTestCropResize2->stDstUV[i].au64PhyAddr[0], pstTestCropResize2->stDstUV[i].au64VirAddr[0]);
        if (NULL != pstTestCropResize2->pFpDst[i]) {
            fclose(pstTestCropResize2->pFpDst[i]);
            pstTestCropResize2->pFpDst[i] = NULL;
        }
        if (NULL != pstTestCropResize2->pFpBg[i]) {
            fclose(pstTestCropResize2->pFpBg[i]);
            pstTestCropResize2->pFpBg[i] = NULL;
        }
    }
    if (NULL != pstTestCropResize2->pFpSrc) {
        fclose(pstTestCropResize2->pFpSrc);
        pstTestCropResize2->pFpSrc = NULL;
    }
}

static AX_S32 SAMPLE_IVE_CreateImage_Y_UV(AX_IVE_IMAGE_T* pstImg, AX_S32 enType, AX_U32 u32Width, AX_U32 u32Height, AX_BOOL bIsUV)
{
    AX_U32 u32Size = 0;
    AX_S32 s32Ret;
    if (NULL == pstImg) {
        SAMPLE_IVE_PRT("pstImg is null\n");
        return AX_FAILURE;
    }
    AX_U32 u32WidthAlign = (g_bAlignNeed == AX_TRUE) ? ALIGN_UP(u32Width, 2) : u32Width;
    AX_U32 u32HeightAlign = (g_bAlignNeed == AX_TRUE) ? ALIGN_UP(u32Height, 2) : u32Height;
    pstImg->enGlbType = (AX_IMG_FORMAT_E)enType;
    pstImg->u32Width = ((bIsUV == AX_TRUE) ? u32WidthAlign / 2 : u32WidthAlign);
    pstImg->u32Height = ((bIsUV == AX_TRUE) ? u32HeightAlign / 2 : u32HeightAlign);
    //stride align to 16
    pstImg->au32Stride[0] = (g_bAlignNeed == AX_TRUE) ? SAMPLE_COMM_IVE_CalcStride(pstImg->u32Width, AX_IVE_ALIGN) : pstImg->u32Width;
    u32Size = pstImg->au32Stride[0] * pstImg->u32Height;
    if (bIsUV)
        u32Size = 2 * u32Size;
    s32Ret = AX_SYS_MemAlloc(&pstImg->au64PhyAddr[0], (AX_VOID**)&pstImg->au64VirAddr[0], u32Size, AX_IVE_ALIGN, (AX_S8 *)AX_IVE_CMM_TOKEN);
    if (s32Ret != AX_SUCCESS) {
        SAMPLE_IVE_PRT("Cmm Alloc fail,Error(%#x)\n", s32Ret);
        return s32Ret;
    }
    return AX_SUCCESS;
}

/******************************************************************************
* function : test crop resize2 init
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestCropResize2_Init(TEST_CROP_RESIZE2_T* pstTestCropResize2, AX_CHAR* pchSrcFile1Name, AX_CHAR* pchSrcFile2Name, AX_CHAR* pchDstFileName,
    AX_U32 u32Width, AX_U32 u32Height, AX_U32 u32Mode, AX_U32 u32Engine, AX_S32 as32Type[], AX_CHAR *pchParamsList)
{
    AX_S32 s32Ret = AX_FAILURE;
    memset(pstTestCropResize2, 0, sizeof(TEST_CROP_RESIZE2_T));

    if (pchParamsList) {
        s32Ret = SAMPLE_IVE_TestCropResize2_ParseParams(pstTestCropResize2, pchParamsList);
        if (AX_SUCCESS != s32Ret)
            return s32Ret;
    } else {
        pstTestCropResize2->stCropResize2Ctrl.u16Num = 1;
        pstTestCropResize2->stSrcBox[0].u16X = 0;
        pstTestCropResize2->stSrcBox[0].u16Y = 0;
        pstTestCropResize2->stSrcBox[0].u16Width = u32Width / 2;
        pstTestCropResize2->stSrcBox[0].u16Height = u32Height / 2;
        pstTestCropResize2->stDstBox[0].u16X = u32Width / 2;
        pstTestCropResize2->stDstBox[0].u16Y = u32Height / 2;
        pstTestCropResize2->stDstBox[0].u16Width = u32Width / 2;
        pstTestCropResize2->stDstBox[0].u16Height = u32Height / 2;
        u32OutputNum = 1;
        u32WidthOut[0] = u32Width;
        u32HeightOut[0] =  u32Height;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateImage_WithGlbImgFmt(&(pstTestCropResize2->stSrc), (AX_IMG_FORMAT_E)as32Type[0], u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create src image failed!\n", s32Ret);
        SAMPLE_IVE_TestCropResize2_Uninit(pstTestCropResize2);
        return s32Ret;
    }
    if (u32Mode == 1) {
        s32Ret = SAMPLE_IVE_CreateImage_Y_UV(&(pstTestCropResize2->stSrcY), as32Type[0], u32Width, u32Height, AX_FALSE);
            if (AX_SUCCESS != s32Ret) {
            SAMPLE_IVE_PRT("Error(%#x),Create src image failed!\n", s32Ret);
            SAMPLE_IVE_TestCropResize2_Uninit(pstTestCropResize2);
            return s32Ret;
        }
        s32Ret = SAMPLE_IVE_CreateImage_Y_UV(&(pstTestCropResize2->stSrcUV), as32Type[0], u32Width, u32Height, AX_TRUE);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_IVE_PRT("Error(%#x),Create src image failed!\n", s32Ret);
            SAMPLE_IVE_TestCropResize2_Uninit(pstTestCropResize2);
            return s32Ret;
        }
    }
    s32Ret = AX_FAILURE;
    pstTestCropResize2->pFpSrc = fopen(pchSrcFile1Name, "rb");
    if (AX_NULL == pstTestCropResize2->pFpSrc) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchSrcFile1Name);
        SAMPLE_IVE_TestCropResize2_Uninit(pstTestCropResize2);
        return s32Ret;
    }
    for (AX_S32 i = 0; i < u32OutputNum; i++) {
        s32Ret = SAMPLE_COMM_IVE_CreateImage_WithGlbImgFmt(&(pstTestCropResize2->stDst[i]), (AX_IMG_FORMAT_E)as32Type[1], u32WidthOut[i], u32HeightOut[i]);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_IVE_PRT("Error(%#x),Create dst image failed!\n", s32Ret);
            SAMPLE_IVE_TestCropResize2_Uninit(pstTestCropResize2);
            return s32Ret;
        }
        if (u32Mode == 1) {
            s32Ret = SAMPLE_IVE_CreateImage_Y_UV(&(pstTestCropResize2->stDstY[i]), as32Type[1], u32WidthOut[i], u32HeightOut[i], AX_FALSE);
            if (AX_SUCCESS != s32Ret) {
                SAMPLE_IVE_PRT("Error(%#x),Create dst image failed!\n", s32Ret);
                SAMPLE_IVE_TestCropResize2_Uninit(pstTestCropResize2);
                return s32Ret;
            }
            s32Ret = SAMPLE_IVE_CreateImage_Y_UV(&(pstTestCropResize2->stDstUV[i]), as32Type[1], u32WidthOut[i], u32HeightOut[i], AX_TRUE);
            if (AX_SUCCESS != s32Ret) {
                SAMPLE_IVE_PRT("Error(%#x),Create dst image failed!\n", s32Ret);
                SAMPLE_IVE_TestCropResize2_Uninit(pstTestCropResize2);
                return s32Ret;
            }
        }
        pstTestCropResize2->pFpBg[i] = fopen(pchSrcFile2Name, "rb");
        if (!pstTestCropResize2->pFpBg[i]) {
            SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchSrcFile2Name);
            SAMPLE_IVE_TestCropResize2_Uninit(pstTestCropResize2);
            return s32Ret;
        }
        AX_CHAR FileName[256] = {0};
        snprintf(FileName, 255, "%s/crop_resize2_out_mode%d_engine%d_crop%ux%u_resize%ux%u_out%ux%u_%d.bin",
        pchDstFileName, u32Mode, u32Engine, pstTestCropResize2->stSrcBox[i].u16Width, pstTestCropResize2->stSrcBox[i].u16Height,
        pstTestCropResize2->stDstBox[i].u16Width, pstTestCropResize2->stDstBox[i].u16Height, u32WidthOut[i], u32HeightOut[i], i);
        pstTestCropResize2->pFpDst[i] = fopen(FileName, "wb");
        if (AX_NULL == pstTestCropResize2->pFpDst[i]) {
            SAMPLE_IVE_PRT("Error,Open file %s failed!\n", FileName);
            SAMPLE_IVE_TestCropResize2_Uninit(pstTestCropResize2);
            return s32Ret;
        }
    }

    return AX_SUCCESS;
}

/******************************************************************************
* function : test crop resize2
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestCropResize2Proc(TEST_CROP_RESIZE2_T* pstTestCropResize2, AX_U32 u32Mode, AX_U32 u32Engine)
{
    AX_S32 s32Ret;
    AX_IVE_HANDLE IveHandle;
    AX_BOOL bInstant = AX_TRUE;
    AX_IVE_RECT_U16_T *pstSrcBoxs[MAX_OUTPUT_NUM];
    AX_IVE_RECT_U16_T *pstDstBoxs[MAX_OUTPUT_NUM];
    AX_IVE_DST_IMAGE_T *pstDst[MAX_OUTPUT_NUM];
    AX_U64 u64StartTime = 0;
    AX_U64 u64EndTime = 0;
    s32Ret = SAMPLE_COMM_IVE_ReadFile_WithGlbImgFmt(&(pstTestCropResize2->stSrc), pstTestCropResize2->pFpSrc);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Read src file failed!\n",s32Ret);
        return s32Ret;
    }
    for (AX_S32 i = 0; i < u32OutputNum; i++) {
         s32Ret = SAMPLE_COMM_IVE_ReadFile_WithGlbImgFmt(&(pstTestCropResize2->stDst[i]), pstTestCropResize2->pFpBg[i]);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_IVE_PRT("Error(%#x),Read bg file failed!\n",s32Ret);
            return s32Ret;
        }
        pstSrcBoxs[i] = &pstTestCropResize2->stSrcBox[i];
        pstDstBoxs[i] = &pstTestCropResize2->stDstBox[i];
        pstDst[i] = &pstTestCropResize2->stDst[i];
    }
    switch(u32Mode) {
    case 0:
        u64StartTime = SAMPLE_COMM_IVE_GetTime_US();
        s32Ret = AX_IVE_CropResize2(&IveHandle, &pstTestCropResize2->stSrc, pstDst, pstSrcBoxs, pstDstBoxs, &pstTestCropResize2->stCropResize2Ctrl, u32Engine, bInstant);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_IVE_PRT("Error(%#x),AX_IVE_CropResize2 failed!\n",s32Ret);
            return s32Ret;
        }
        u64EndTime = SAMPLE_COMM_IVE_GetTime_US();
        printf("Run CropResize2 task cost %lld us\n", u64EndTime - u64StartTime);
        break;
    case 1:
        //splite y and uv
        memcpy((AX_U8 *)(AX_UL)pstTestCropResize2->stSrcY.au64VirAddr[0], (AX_U8 *)(AX_UL)pstTestCropResize2->stSrc.au64VirAddr[0], pstTestCropResize2->stSrcY.au32Stride[0] * pstTestCropResize2->stSrcY.u32Height);
        memcpy((AX_U8 *)(AX_UL)pstTestCropResize2->stSrcUV.au64VirAddr[0], (AX_U8 *)(AX_UL)pstTestCropResize2->stSrc.au64VirAddr[1], 2 * pstTestCropResize2->stSrcUV.au32Stride[0] * pstTestCropResize2->stSrcUV.u32Height);
        AX_IVE_DST_IMAGE_T *pstDstY[MAX_OUTPUT_NUM];
        AX_IVE_DST_IMAGE_T *pstDstUV[MAX_OUTPUT_NUM];
        for (AX_S32 i = 0; i < u32OutputNum; i++) {
            memcpy((AX_U8 *)(AX_UL)pstTestCropResize2->stDstY[i].au64VirAddr[0], (AX_U8 *)(AX_UL)pstTestCropResize2->stDst[i].au64VirAddr[0], pstTestCropResize2->stDstY[i].au32Stride[0] * pstTestCropResize2->stDstY[i].u32Height);
            memcpy((AX_U8 *)(AX_UL)pstTestCropResize2->stDstUV[i].au64VirAddr[0], (AX_U8 *)(AX_UL)pstTestCropResize2->stDst[i].au64VirAddr[1], 2 * pstTestCropResize2->stDstUV[i].au32Stride[0] * pstTestCropResize2->stDstUV[i].u32Height);
            pstDstY[i] = &pstTestCropResize2->stDstY[i];
            pstDstUV[i] = &pstTestCropResize2->stDstUV[i];
        }
        u64StartTime = SAMPLE_COMM_IVE_GetTime_US();
        s32Ret = AX_IVE_CropResize2ForSplitYUV(&IveHandle, &pstTestCropResize2->stSrcY, &pstTestCropResize2->stSrcUV, pstDstY, pstDstUV, pstSrcBoxs, pstDstBoxs, &pstTestCropResize2->stCropResize2Ctrl, u32Engine, bInstant);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_IVE_PRT("Error(%#x),AX_IVE_CropResize2ForSplitYUV failed!\n",s32Ret);
            return s32Ret;
        }
        u64EndTime = SAMPLE_COMM_IVE_GetTime_US();
        printf("Run CropResize2ForSplitYUV task cost %lld us\n", u64EndTime - u64StartTime);
        break;
    default:
        SAMPLE_IVE_PRT("No support mode:%d\n", u32Mode);
        return AX_FAILURE;
    }
    for (AX_S32 i = 0; i < u32OutputNum; i++) {
        if (u32Mode == 1) {
            //concat y and uv
            memcpy((AX_U8 *)(AX_UL)pstTestCropResize2->stDst[i].au64VirAddr[0], (AX_U8 *)(AX_UL)pstTestCropResize2->stDstY[i].au64VirAddr[0], pstTestCropResize2->stDstY[i].au32Stride[0] * pstTestCropResize2->stDstY[i].u32Height);
            memcpy((AX_U8 *)(AX_UL)pstTestCropResize2->stDst[i].au64VirAddr[1], (AX_U8 *)(AX_UL)pstTestCropResize2->stDstUV[i].au64VirAddr[0], 2 * pstTestCropResize2->stDstUV[i].au32Stride[0] * pstTestCropResize2->stDstUV[i].u32Height);
        }
        s32Ret = SAMPLE_COMM_IVE_WriteFile_WithGlbImgFmt(&pstTestCropResize2->stDst[i], pstTestCropResize2->pFpDst[i]);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_IVE_PRT("Error,Write dst file failed!\n");
            return s32Ret;
        }
    }

    return s32Ret;
}
/******************************************************************************
* function : Show test crop resize2 calculate sample
******************************************************************************/
AX_VOID SAMPLE_IVE_CropResize2_TEST(AX_U32 u32Engine, AX_U32 u32Mode, AX_S32 as32Type[], AX_CHAR **pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList)
{
    AX_S32 s32Ret;
    AX_U32 u32Width = u32WidthSrc;
    AX_U32 u32Height = u32HeightSrc;
    AX_CHAR* pchSrcFile1 = pchSrcPath[0];
    AX_CHAR* pchSrcFile2 = pchSrcPath[1];
    AX_CHAR* pchDstFile = pchDstPath;
    if (!pchSrcFile1 || !pchSrcFile2 || !pchDstFile) {
        SAMPLE_IVE_PRT("Error: null pointer(src or dst path not specified)!\n");
        return;
    }
    if(AX_TRUE != SAMPLE_COMM_IVE_CheckDir(pchDstFile)) {
        SAMPLE_IVE_PRT("Error: dst must be specified as directory!\n");
        return;
    }
    memset(&s_stTestCropResize2, 0, sizeof(TEST_CROP_RESIZE2_T));
    s32Ret = SAMPLE_IVE_TestCropResize2_Init(&s_stTestCropResize2, pchSrcFile1, pchSrcFile2, pchDstFile, u32Width, u32Height, u32Mode, u32Engine, as32Type, pchParamsList);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),SAMPLE_IVE_TestCropResize2_Init failed!\n", s32Ret);
        return;
    }

    s32Ret =  SAMPLE_IVE_TestCropResize2Proc(&s_stTestCropResize2, u32Mode, u32Engine);
    if (AX_SUCCESS == s32Ret)
        SAMPLE_IVE_PRT("Process success!\n");
    else
        SAMPLE_IVE_PRT("Error:process failed!\n");

    SAMPLE_IVE_TestCropResize2_Uninit(&s_stTestCropResize2);
    memset(&s_stTestCropResize2, 0, sizeof(TEST_CROP_RESIZE2_T));
}

/******************************************************************************
* function :CropResize2 Test sample signal handle
******************************************************************************/
AX_VOID SAMPLE_IVE_CropResize2_TEST_HandleSig(AX_VOID)
{
    SAMPLE_IVE_TestCropResize2_Uninit(&s_stTestCropResize2);
    memset(&s_stTestCropResize2, 0, sizeof(TEST_CROP_RESIZE2_T));

    AX_IVE_Exit();
    AX_SYS_Deinit();
}
