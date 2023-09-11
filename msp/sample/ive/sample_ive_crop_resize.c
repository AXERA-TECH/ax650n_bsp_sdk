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

typedef struct axTEST_CROP_RESIZE_T {
    AX_IVE_SRC_IMAGE_T stSrc;
    AX_IVE_DST_IMAGE_T stDst[MAX_OUTPUT_NUM];
    AX_IVE_SRC_IMAGE_T stSrcY;
    AX_IVE_SRC_IMAGE_T stSrcUV;
    AX_IVE_DST_IMAGE_T stDstY[MAX_OUTPUT_NUM];
    AX_IVE_DST_IMAGE_T stDstUV[MAX_OUTPUT_NUM];
    AX_IVE_CROP_IMAGE_CTRL_T stCropImageCtrl;
    AX_IVE_CROP_RESIZE_CTRL_T stCropResizeCtrl;
    FILE* pFpSrc;
    FILE* pFpDst[MAX_OUTPUT_NUM];
    AX_IVE_RECT_U16_T stBox[MAX_OUTPUT_NUM];
} TEST_CROP_RESIZE_T;
static TEST_CROP_RESIZE_T s_stTestCropResize;
static AX_U32 u32OutputNum = 0;
static AX_U32 u32WidthOut = 0;
static AX_U32 u32HeightOut = 0;

/******************************************************************************
* function : parse crop resize parameters
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestCropResize_ParseParams(TEST_CROP_RESIZE_T* pstTestCropResize, AX_CHAR *pchParamsList, AX_U32 u32Mode)
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

    if (u32Mode == 0) {
        pstTestCropResize->stCropImageCtrl.u16Num = u32OutputNum;
    } else {
        pstTestCropResize->stCropResizeCtrl.u16Num = u32OutputNum;
        item = cJSON_GetObjectItem(root, "align0");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param align0 failed!\n");
        pstTestCropResize->stCropResizeCtrl.enAlign[0] = item->valueint;
        item = cJSON_GetObjectItem(root, "align1");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param align1 failed!\n");
        pstTestCropResize->stCropResizeCtrl.enAlign[1] = item->valueint;
        item = cJSON_GetObjectItem(root, "bcolor");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param border color failed!\n");
        pstTestCropResize->stCropResizeCtrl.u32BorderColor = item->valueint;
        item = cJSON_GetObjectItem(root, "w_out");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param w_out failed!\n");
        u32WidthOut = item->valueint;
        item = cJSON_GetObjectItem(root, "h_out");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param h_out failed!\n");
        u32HeightOut = item->valueint;
    }
    item_array = cJSON_GetObjectItem(root, "boxs");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item_array, PARSE_FAIL, "Error:parse param boxs failed!\n");
    AX_U32 u32ArraySize = cJSON_GetArraySize(item_array);
    SAMPLE_IVE_CHECK_EXPR_GOTO(u32ArraySize != u32OutputNum, PARSE_FAIL,
        "Error:u32ArraySize[%d] is not equal to %d!\n",u32ArraySize, u32OutputNum);
    cJSON *item_sub = item_array->child;
    AX_S32 i = 0;
    for (;item_sub;) {
        cJSON *item_rect = cJSON_GetObjectItem(item_sub, "x");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item_rect, PARSE_FAIL, "Error:parse param x failed!\n");
        pstTestCropResize->stBox[i].u16X = item_rect->valueint;
        item_rect = cJSON_GetObjectItem(item_sub, "y");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item_rect, PARSE_FAIL, "Error:parse param y failed!\n");
        pstTestCropResize->stBox[i].u16Y = item_rect->valueint;
        item_rect = cJSON_GetObjectItem(item_sub, "w");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item_rect, PARSE_FAIL, "Error:parse param w failed!\n");
        pstTestCropResize->stBox[i].u16Width = item_rect->valueint;
        item_rect = cJSON_GetObjectItem(item_sub, "h");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item_rect, PARSE_FAIL, "Error:parse param h failed!\n");
        pstTestCropResize->stBox[i].u16Height = item_rect->valueint;
        item_sub = item_sub->next;
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
* function : test crop resize uninit
******************************************************************************/
static AX_VOID SAMPLE_IVE_TestCropResize_Uninit(TEST_CROP_RESIZE_T* pstTestCropResize)
{
    IVE_CMM_FREE(pstTestCropResize->stSrc.au64PhyAddr[0], pstTestCropResize->stSrc.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestCropResize->stSrcY.au64PhyAddr[0], pstTestCropResize->stSrcY.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestCropResize->stSrcUV.au64PhyAddr[0], pstTestCropResize->stSrcUV.au64VirAddr[0]);

    for (AX_S32 i = 0; i < u32OutputNum; i++) {
        IVE_CMM_FREE(pstTestCropResize->stDst[i].au64PhyAddr[0], pstTestCropResize->stDst[i].au64VirAddr[0]);
        IVE_CMM_FREE(pstTestCropResize->stDstY[i].au64PhyAddr[0], pstTestCropResize->stDstY[i].au64VirAddr[0]);
        IVE_CMM_FREE(pstTestCropResize->stDstUV[i].au64PhyAddr[0], pstTestCropResize->stDstUV[i].au64VirAddr[0]);
        if (NULL != pstTestCropResize->pFpDst[i]) {
            fclose(pstTestCropResize->pFpDst[i]);
            pstTestCropResize->pFpDst[i] = NULL;
        }
    }
    if (NULL != pstTestCropResize->pFpSrc) {
        fclose(pstTestCropResize->pFpSrc);
        pstTestCropResize->pFpSrc = NULL;
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
* function : test crop resize init
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestCropResize_Init(TEST_CROP_RESIZE_T* pstTestCropResize, AX_CHAR* pchSrcFileName, AX_CHAR* pchDstFileName,
    AX_U32 u32Width, AX_U32 u32Height, AX_U32 u32Mode, AX_U32 u32Engine, AX_S32 as32Type[], AX_CHAR *pchParamsList)
{
    AX_S32 s32Ret = AX_FAILURE;
    memset(pstTestCropResize, 0, sizeof(TEST_CROP_RESIZE_T));

    if (pchParamsList) {
        s32Ret = SAMPLE_IVE_TestCropResize_ParseParams(pstTestCropResize, pchParamsList, u32Mode);
        if (AX_SUCCESS != s32Ret)
            return s32Ret;
    } else {
        pstTestCropResize->stCropImageCtrl.u16Num = 1;
        pstTestCropResize->stCropResizeCtrl.u16Num = 1;
        pstTestCropResize->stCropResizeCtrl.enAlign[0] = AX_IVE_ASPECT_RATIO_FORCE_RESIZE;
        pstTestCropResize->stCropResizeCtrl.enAlign[1] = AX_IVE_ASPECT_RATIO_FORCE_RESIZE;
        pstTestCropResize->stCropResizeCtrl.u32BorderColor = 0x0000FF;
        pstTestCropResize->stBox[0].u16X = 0;
        pstTestCropResize->stBox[0].u16Y = 0;
        pstTestCropResize->stBox[0].u16Width = u32Width / 2;
        pstTestCropResize->stBox[0].u16Height = u32Height / 2;
        u32OutputNum = 1;
        u32WidthOut = pstTestCropResize->stBox[0].u16Width;
        u32HeightOut =  pstTestCropResize->stBox[0].u16Height;
    }
    if (u32Mode == 0 && u32Engine == AX_IVE_ENGINE_IVE) {
        s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestCropResize->stSrc), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[0], AX_IVE_IMAGE_TYPE_U8C1), u32Width, u32Height);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_IVE_PRT("Error(%#x),Create src image failed!\n", s32Ret);
            SAMPLE_IVE_TestCropResize_Uninit(pstTestCropResize);
            return s32Ret;
        }
    } else {
        s32Ret = SAMPLE_COMM_IVE_CreateImage_WithGlbImgFmt(&(pstTestCropResize->stSrc), (AX_IMG_FORMAT_E)as32Type[0], u32Width, u32Height);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_IVE_PRT("Error(%#x),Create src image failed!\n", s32Ret);
            SAMPLE_IVE_TestCropResize_Uninit(pstTestCropResize);
            return s32Ret;
        }
        if (u32Mode == 2) {
            s32Ret = SAMPLE_IVE_CreateImage_Y_UV(&(pstTestCropResize->stSrcY), as32Type[0], u32Width, u32Height, AX_FALSE);
                if (AX_SUCCESS != s32Ret) {
                SAMPLE_IVE_PRT("Error(%#x),Create src image failed!\n", s32Ret);
                SAMPLE_IVE_TestCropResize_Uninit(pstTestCropResize);
                return s32Ret;
            }
            s32Ret = SAMPLE_IVE_CreateImage_Y_UV(&(pstTestCropResize->stSrcUV), as32Type[0], u32Width, u32Height, AX_TRUE);
            if (AX_SUCCESS != s32Ret) {
                SAMPLE_IVE_PRT("Error(%#x),Create src image failed!\n", s32Ret);
                SAMPLE_IVE_TestCropResize_Uninit(pstTestCropResize);
                return s32Ret;
            }
        }
    }

    s32Ret = AX_FAILURE;
    pstTestCropResize->pFpSrc = fopen(pchSrcFileName, "rb");
    if (AX_NULL == pstTestCropResize->pFpSrc) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchSrcFileName);
        SAMPLE_IVE_TestCropResize_Uninit(pstTestCropResize);
        return s32Ret;
    }

    for (AX_S32 i = 0; i < u32OutputNum; i++) {
        SAMPLE_IVE_PRT("box[%d] = {x:%d,y:%d,w:%d,h:%d}\n", i, pstTestCropResize->stBox[i].u16X, pstTestCropResize->stBox[i].u16Y,
            pstTestCropResize->stBox[i].u16Width, pstTestCropResize->stBox[i].u16Height);
        if (u32Mode == 0) {
            u32WidthOut = pstTestCropResize->stBox[i].u16Width;
            u32HeightOut = pstTestCropResize->stBox[i].u16Height;
        }
        SAMPLE_IVE_PRT("u32WidthOut:%d, u32HeightOut:%d\n", u32WidthOut, u32HeightOut);
        if (u32Mode == 0 && u32Engine == AX_IVE_ENGINE_IVE) {
            s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestCropResize->stDst[i]), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[0], AX_IVE_IMAGE_TYPE_U8C1), u32WidthOut,  u32HeightOut);
            if (AX_SUCCESS != s32Ret) {
                SAMPLE_IVE_PRT("Error(%#x),Create dst image failed!\n", s32Ret);
                SAMPLE_IVE_TestCropResize_Uninit(pstTestCropResize);
                return s32Ret;
            }
        } else {
            s32Ret = SAMPLE_COMM_IVE_CreateImage_WithGlbImgFmt(&(pstTestCropResize->stDst[i]), (AX_IMG_FORMAT_E)as32Type[1], u32WidthOut, u32HeightOut);
            if (AX_SUCCESS != s32Ret) {
                SAMPLE_IVE_PRT("Error(%#x),Create dst image failed!\n", s32Ret);
                SAMPLE_IVE_TestCropResize_Uninit(pstTestCropResize);
                return s32Ret;
            }
            if (u32Mode == 2) {
                s32Ret = SAMPLE_IVE_CreateImage_Y_UV(&(pstTestCropResize->stDstY[i]), as32Type[1], u32WidthOut, u32HeightOut, AX_FALSE);
                if (AX_SUCCESS != s32Ret) {
                    SAMPLE_IVE_PRT("Error(%#x),Create dst image failed!\n", s32Ret);
                    SAMPLE_IVE_TestCropResize_Uninit(pstTestCropResize);
                    return s32Ret;
                }
                s32Ret = SAMPLE_IVE_CreateImage_Y_UV(&(pstTestCropResize->stDstUV[i]), as32Type[1], u32WidthOut, u32HeightOut, AX_TRUE);
                if (AX_SUCCESS != s32Ret) {
                    SAMPLE_IVE_PRT("Error(%#x),Create dst image failed!\n", s32Ret);
                    SAMPLE_IVE_TestCropResize_Uninit(pstTestCropResize);
                    return s32Ret;
                }
            }
        }
        s32Ret = AX_FAILURE;
        AX_CHAR FileName[256] = {0};
        snprintf(FileName, 255, "%s/crop_resize_out_mode%d_engine%d_%ux%u_%d.bin", pchDstFileName, u32Mode, u32Engine, u32WidthOut, u32HeightOut, i);
        pstTestCropResize->pFpDst[i] = fopen(FileName, "wb");
        if (AX_NULL == pstTestCropResize->pFpDst[i]) {
            SAMPLE_IVE_PRT("Error,Open file %s failed!\n", FileName);
            SAMPLE_IVE_TestCropResize_Uninit(pstTestCropResize);
            return s32Ret;
        }
    }

    return AX_SUCCESS;
}

/******************************************************************************
* function : test crop resize
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestCropResizeProc(TEST_CROP_RESIZE_T* pstTestCropResize, AX_U32 u32Mode, AX_U32 u32Engine)
{
    AX_S32 s32Ret;
    AX_IVE_HANDLE IveHandle;
    AX_BOOL bInstant = AX_TRUE;

    AX_IVE_RECT_U16_T *pstDstBoxs[MAX_OUTPUT_NUM];
    AX_IVE_DST_IMAGE_T *pstDst[MAX_OUTPUT_NUM];
    AX_U64 u64StartTime = 0;
    AX_U64 u64EndTime = 0;
    if (u32Mode == 0 && u32Engine == AX_IVE_ENGINE_IVE)
        s32Ret = SAMPLE_COMM_IVE_ReadFile(&(pstTestCropResize->stSrc), pstTestCropResize->pFpSrc);
    else
        s32Ret = SAMPLE_COMM_IVE_ReadFile_WithGlbImgFmt(&(pstTestCropResize->stSrc), pstTestCropResize->pFpSrc);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Read src file failed!\n",s32Ret);
        return s32Ret;
    }
    for (AX_S32 i = 0; i < u32OutputNum; i++)
        pstDstBoxs[i] = &pstTestCropResize->stBox[i];
    for (AX_S32 i = 0; i < u32OutputNum; i++)
        pstDst[i] = &pstTestCropResize->stDst[i];

    switch(u32Mode) {
    case 0:
        u64StartTime = SAMPLE_COMM_IVE_GetTime_US();
        s32Ret = AX_IVE_CropImage(&IveHandle, &pstTestCropResize->stSrc, pstDst, pstDstBoxs, &pstTestCropResize->stCropImageCtrl, u32Engine, bInstant);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_IVE_PRT("Error(%#x),AX_IVE_CropImage failed!\n",s32Ret);
            return s32Ret;
        }
        u64EndTime = SAMPLE_COMM_IVE_GetTime_US();
        printf("Run CropImage task cost %lld us\n", u64EndTime - u64StartTime);
        break;
    case 1:
        u64StartTime = SAMPLE_COMM_IVE_GetTime_US();
        s32Ret = AX_IVE_CropResize(&IveHandle, &pstTestCropResize->stSrc, pstDst, pstDstBoxs, &pstTestCropResize->stCropResizeCtrl, u32Engine, bInstant);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_IVE_PRT("Error(%#x),AX_IVE_CropResize failed!\n",s32Ret);
            return s32Ret;
        }
        u64EndTime = SAMPLE_COMM_IVE_GetTime_US();
        printf("Run CropResize task cost %lld us\n", u64EndTime - u64StartTime);
        break;
    case 2:
          //splite y and uv
        memcpy((AX_U8 *)(AX_UL)pstTestCropResize->stSrcY.au64VirAddr[0], (AX_U8 *)(AX_UL)pstTestCropResize->stSrc.au64VirAddr[0], pstTestCropResize->stSrcY.au32Stride[0] * pstTestCropResize->stSrcY.u32Height);
        memcpy((AX_U8 *)(AX_UL)pstTestCropResize->stSrcUV.au64VirAddr[0], (AX_U8 *)(AX_UL)pstTestCropResize->stSrc.au64VirAddr[1], 2 * pstTestCropResize->stSrcUV.au32Stride[0] * pstTestCropResize->stSrcUV.u32Height);
        AX_IVE_DST_IMAGE_T *pstDstY[MAX_OUTPUT_NUM];
        AX_IVE_DST_IMAGE_T *pstDstUV[MAX_OUTPUT_NUM];
        for (AX_S32 i = 0; i < u32OutputNum; i++) {
            pstDstY[i] = &pstTestCropResize->stDstY[i];
            pstDstUV[i] = &pstTestCropResize->stDstUV[i];
        }
        u64StartTime = SAMPLE_COMM_IVE_GetTime_US();
        s32Ret = AX_IVE_CropResizeForSplitYUV(&IveHandle, &pstTestCropResize->stSrcY, &pstTestCropResize->stSrcUV, pstDstY, pstDstUV, pstDstBoxs,&pstTestCropResize->stCropResizeCtrl, u32Engine, bInstant);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_IVE_PRT("Error(%#x),AX_IVE_CropResizeImageForSplitYUV failed!\n",s32Ret);
            return s32Ret;
        }
        u64EndTime = SAMPLE_COMM_IVE_GetTime_US();
        printf("Run CropResizeForSplitYUV task cost %lld us\n", u64EndTime - u64StartTime);
        break;
    default:
        SAMPLE_IVE_PRT("No support mode:%d\n", u32Mode);
        return AX_FAILURE;
    }

    for (AX_S32 i = 0; i < u32OutputNum; i++) {
        if (u32Mode) {
            //concat y and uv
            memcpy((AX_U8 *)(AX_UL)pstTestCropResize->stDst[i].au64VirAddr[0], (AX_U8 *)(AX_UL)pstTestCropResize->stDstY[i].au64VirAddr[0], pstTestCropResize->stDstY[i].au32Stride[0] * pstTestCropResize->stDstY[i].u32Height);
            memcpy((AX_U8 *)(AX_UL)pstTestCropResize->stDst[i].au64VirAddr[1], (AX_U8 *)(AX_UL)pstTestCropResize->stDstUV[i].au64VirAddr[0], 2 * pstTestCropResize->stDstUV[i].au32Stride[0] * pstTestCropResize->stDstUV[i].u32Height);
        }
        if (u32Mode == 0 && u32Engine == AX_IVE_ENGINE_IVE)
            s32Ret = SAMPLE_COMM_IVE_WriteFile(&pstTestCropResize->stDst[i], pstTestCropResize->pFpDst[i]);
        else
            s32Ret = SAMPLE_COMM_IVE_WriteFile_WithGlbImgFmt(&pstTestCropResize->stDst[i], pstTestCropResize->pFpDst[i]);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_IVE_PRT("Error,Write dst file failed!\n");
            return s32Ret;
        }
    }

    return s32Ret;
}
/******************************************************************************
* function : Show test crop resize calculate sample
******************************************************************************/
AX_VOID SAMPLE_IVE_CropResize_TEST(AX_U32 u32Engine, AX_U32 u32Mode, AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList)
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
    memset(&s_stTestCropResize, 0, sizeof(TEST_CROP_RESIZE_T));
    s32Ret = SAMPLE_IVE_TestCropResize_Init(&s_stTestCropResize, pchSrcFile, pchDstFile, u32Width, u32Height, u32Mode, u32Engine, as32Type, pchParamsList);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),SAMPLE_IVE_TestCropResize_Init failed!\n", s32Ret);
        return;
    }

    s32Ret =  SAMPLE_IVE_TestCropResizeProc(&s_stTestCropResize, u32Mode, u32Engine);
    if (AX_SUCCESS == s32Ret)
        SAMPLE_IVE_PRT("Process success!\n");
    else
        SAMPLE_IVE_PRT("Error:process failed!\n");

    SAMPLE_IVE_TestCropResize_Uninit(&s_stTestCropResize);
    memset(&s_stTestCropResize, 0, sizeof(TEST_CROP_RESIZE_T));
}

/******************************************************************************
* function :CropResize Test sample signal handle
******************************************************************************/
AX_VOID SAMPLE_IVE_CropResize_TEST_HandleSig(AX_VOID)
{
    SAMPLE_IVE_TestCropResize_Uninit(&s_stTestCropResize);
    memset(&s_stTestCropResize, 0, sizeof(TEST_CROP_RESIZE_T));

    AX_IVE_Exit();
    AX_SYS_Deinit();
}
