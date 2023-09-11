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

typedef struct axTEST_DUALPIC_CALC_T {
    AX_IVE_SRC_IMAGE_T stSrc1;
    AX_IVE_SRC_IMAGE_T stSrc2;
    AX_IVE_DST_IMAGE_T stDst;
    AX_IVE_ADD_CTRL_T stAddCtrl;
    AX_IVE_SUB_CTRL_T stSubCtrl;
    AX_IVE_MSE_CTRL_T stMseCtrl;
    FILE* pFpSrc1;
    FILE* pFpSrc2;
    FILE* pFpDst;
} TEST_DUALPIC_CALC_T;
static TEST_DUALPIC_CALC_T s_stTestDualPicCalc;

/******************************************************************************
* function : parse dualpiccalc parameters
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestDualPic_ParseParams(TEST_DUALPIC_CALC_T* pstTestDualPic, AX_CHAR *pchParamsList, AX_U32 u32Mode)
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
        item = cJSON_GetObjectItem(root, "x");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param x failed!\n");
        pstTestDualPic->stAddCtrl.u1q7X = item->valueint;
        item = cJSON_GetObjectItem(root, "y");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param y failed!\n");
        pstTestDualPic->stAddCtrl.u1q7Y = item->valueint;
    } else if (u32Mode == 1) {
        item = cJSON_GetObjectItem(root, "mode");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param mode failed!\n");
        pstTestDualPic->stSubCtrl.enMode = item->valueint;
    } else if (u32Mode == 5) {
        item = cJSON_GetObjectItem(root, "mse_coef");
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param mse_coef failed!\n");
        pstTestDualPic->stMseCtrl.u1q15MseCoef = item->valueint;
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
* function : test dualpiccalc uninit
******************************************************************************/
static AX_VOID SAMPLE_IVE_TestDualPic_Uninit(TEST_DUALPIC_CALC_T* pstTestDualPic)
{
    IVE_CMM_FREE(pstTestDualPic->stSrc1.au64PhyAddr[0], pstTestDualPic->stSrc1.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestDualPic->stSrc2.au64PhyAddr[0], pstTestDualPic->stSrc2.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestDualPic->stDst.au64PhyAddr[0], pstTestDualPic->stDst.au64VirAddr[0]);

    if (NULL != pstTestDualPic->pFpSrc1) {
        fclose(pstTestDualPic->pFpSrc1);
        pstTestDualPic->pFpSrc1 = NULL;
    }
    if (NULL != pstTestDualPic->pFpSrc2) {
        fclose(pstTestDualPic->pFpSrc2);
        pstTestDualPic->pFpSrc2 = NULL;
    }
    if (NULL != pstTestDualPic->pFpDst) {
        fclose(pstTestDualPic->pFpDst);
        pstTestDualPic->pFpDst = NULL;
    }

}
/******************************************************************************
* function : test dualpiccalc init
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestDualPic_Init(TEST_DUALPIC_CALC_T* pstTestDualPic, AX_CHAR* pchSrcFile1Name, AX_CHAR* pchSrcFile2Name,
    AX_CHAR* pchDstFileName, AX_U32 u32Width, AX_U32 u32Height, AX_U32 u32Mode, AX_S32 as32Type[], AX_CHAR *pchParamsList)
{
    AX_S32 s32Ret = AX_FAILURE;
    AX_IVE_IMAGE_TYPE_E enDstImageType = IMAGE_TYPE_SPECIFY(as32Type[2], AX_IVE_IMAGE_TYPE_U8C1);
    memset(pstTestDualPic, 0, sizeof(TEST_DUALPIC_CALC_T));
    if (pchParamsList) {
        s32Ret = SAMPLE_IVE_TestDualPic_ParseParams(pstTestDualPic, pchParamsList, u32Mode);
        if (AX_SUCCESS != s32Ret)
            return s32Ret;
    } else {
        pstTestDualPic->stAddCtrl.u1q7X = 50;
        pstTestDualPic->stAddCtrl.u1q7Y = 50;
        pstTestDualPic->stSubCtrl.enMode = AX_IVE_SUB_MODE_ABS;
        pstTestDualPic->stMseCtrl.u1q15MseCoef = 32767;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestDualPic->stSrc1), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[0], AX_IVE_IMAGE_TYPE_U8C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create src1 image failed!\n", s32Ret);
        SAMPLE_IVE_TestDualPic_Uninit(pstTestDualPic);
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestDualPic->stSrc2), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[1], AX_IVE_IMAGE_TYPE_U8C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create src2 image failed!\n", s32Ret);
        SAMPLE_IVE_TestDualPic_Uninit(pstTestDualPic);
        return s32Ret;
    }
    /*Note: MSE:AX_IVE_IMAGE_TYPE_U16C1; Sub_Shift:AX_IVE_IMAGE_TYPE_S8C1 Other:AX_IVE_IMAGE_TYPE_U8C1 */
    if (u32Mode == 1 && pstTestDualPic->stSubCtrl.enMode == AX_IVE_SUB_MODE_SHIFT)
        enDstImageType = IMAGE_TYPE_SPECIFY(as32Type[2], AX_IVE_IMAGE_TYPE_S8C1);
    else if(u32Mode == 5)
        enDstImageType = IMAGE_TYPE_SPECIFY(as32Type[2], AX_IVE_IMAGE_TYPE_U16C1);
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestDualPic->stDst), enDstImageType, u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create dst image failed!\n", s32Ret);
        SAMPLE_IVE_TestDualPic_Uninit(pstTestDualPic);
        return s32Ret;
    }

    s32Ret = AX_FAILURE;
    pstTestDualPic->pFpSrc1 = fopen(pchSrcFile1Name, "rb");
    if (AX_NULL == pstTestDualPic->pFpSrc1) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchSrcFile1Name);
        SAMPLE_IVE_TestDualPic_Uninit(pstTestDualPic);
        return s32Ret;
    }
    pstTestDualPic->pFpSrc2 = fopen(pchSrcFile2Name, "rb");
    if (AX_NULL == pstTestDualPic->pFpSrc2) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchSrcFile2Name);
        SAMPLE_IVE_TestDualPic_Uninit(pstTestDualPic);
        return s32Ret;
    }
    pstTestDualPic->pFpDst = fopen(pchDstFileName, "wb");
    if (AX_NULL == pstTestDualPic->pFpDst) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchDstFileName);
        SAMPLE_IVE_TestDualPic_Uninit(pstTestDualPic);
        return s32Ret;
    }

    return AX_SUCCESS;
}
/******************************************************************************
* function : test dualpiccalc
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestDualPicCalcProc(TEST_DUALPIC_CALC_T* pstTestDualPic, AX_U32 u32Mode)
{
    AX_S32 s32Ret;
    AX_IVE_HANDLE IveHandle;
    AX_BOOL bInstant = AX_FALSE;
    AX_BOOL bBlock = AX_TRUE;
    AX_BOOL bFinish = AX_FALSE;

    s32Ret = SAMPLE_COMM_IVE_ReadFile(&(pstTestDualPic->stSrc1), pstTestDualPic->pFpSrc1);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Read src1 file failed!\n",s32Ret);
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_IVE_ReadFile(&(pstTestDualPic->stSrc2), pstTestDualPic->pFpSrc2);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Read src2 file failed!\n",s32Ret);
        return s32Ret;
    }

    bInstant = AX_TRUE;
    AX_U64 u64StartTime = SAMPLE_COMM_IVE_GetTime_US();
    switch(u32Mode) {
        case 0:
            s32Ret = AX_IVE_Add(&IveHandle, &pstTestDualPic->stSrc1, &pstTestDualPic->stSrc2, &pstTestDualPic->stDst, &pstTestDualPic->stAddCtrl, bInstant);
            if (AX_SUCCESS != s32Ret) {
                SAMPLE_IVE_PRT("Error(%#x),AX_IVE_Add failed!\n",s32Ret);
                return s32Ret;
            }
            break;
        case 1:
            s32Ret = AX_IVE_Sub(&IveHandle, &pstTestDualPic->stSrc1, &pstTestDualPic->stSrc2, &pstTestDualPic->stDst, &pstTestDualPic->stSubCtrl, bInstant);
            if (AX_SUCCESS != s32Ret) {
                SAMPLE_IVE_PRT("Error(%#x),AX_IVE_Sub failed!\n",s32Ret);
                return s32Ret;
            }
            break;
        case 2:
            s32Ret = AX_IVE_And(&IveHandle, &pstTestDualPic->stSrc1, &pstTestDualPic->stSrc2, &pstTestDualPic->stDst, bInstant);
            if (AX_SUCCESS != s32Ret) {
                SAMPLE_IVE_PRT("Error(%#x),AX_IVE_And failed!\n",s32Ret);
                return s32Ret;
            }
            break;
        case 3:
            s32Ret = AX_IVE_Or(&IveHandle, &pstTestDualPic->stSrc1, &pstTestDualPic->stSrc2, &pstTestDualPic->stDst, bInstant);
            if (AX_SUCCESS != s32Ret) {
                SAMPLE_IVE_PRT("Error(%#x),AX_IVE_Or failed!\n",s32Ret);
                return s32Ret;
            }
            break;
        case 4:
            s32Ret = AX_IVE_Xor(&IveHandle, &pstTestDualPic->stSrc1, &pstTestDualPic->stSrc2, &pstTestDualPic->stDst, bInstant);
            if (AX_SUCCESS != s32Ret) {
                SAMPLE_IVE_PRT("Error(%#x),AX_IVE_Xor failed!\n",s32Ret);
                return s32Ret;
            }
            break;
        case 5:
            s32Ret = AX_IVE_Mse(&IveHandle, &pstTestDualPic->stSrc1, &pstTestDualPic->stSrc2, &pstTestDualPic->stDst, &pstTestDualPic->stMseCtrl, bInstant);
            if (AX_SUCCESS != s32Ret) {
                SAMPLE_IVE_PRT("Error(%#x),AX_IVE_Mse failed!\n",s32Ret);
                return s32Ret;
            }
            break;
        default:
            SAMPLE_IVE_PRT("Error(%#x), not support!\n",s32Ret);
            return -1;
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
    printf("Run DualPicsCalc task cost %lld us\n", u64EndTime - u64StartTime);

    s32Ret = SAMPLE_COMM_IVE_WriteFile(&pstTestDualPic->stDst, pstTestDualPic->pFpDst);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error,Write dst file failed!\n");
        return s32Ret;
    }

    return s32Ret;
}
/******************************************************************************
* function : Show test dual pictures calculate sample
******************************************************************************/
AX_VOID SAMPLE_IVE_DualPicCalc_TEST(AX_U32 u32Mode, AX_S32 as32Type[], AX_CHAR **pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList)
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

    memset(&s_stTestDualPicCalc, 0, sizeof(TEST_DUALPIC_CALC_T));
    s32Ret = SAMPLE_IVE_TestDualPic_Init(&s_stTestDualPicCalc, pchSrcFile1, pchSrcFile2, pchDstFile, u32Width, u32Height, u32Mode, as32Type, pchParamsList);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),SAMPLE_IVE_TestDualPics_Init failed!\n", s32Ret);
        return;
    }

    s32Ret =  SAMPLE_IVE_TestDualPicCalcProc(&s_stTestDualPicCalc, u32Mode);
    if (AX_SUCCESS == s32Ret)
        SAMPLE_IVE_PRT("Process success!\n");
    else
        SAMPLE_IVE_PRT("Error:process failed!\n");

    SAMPLE_IVE_TestDualPic_Uninit(&s_stTestDualPicCalc);
    memset(&s_stTestDualPicCalc, 0, sizeof(TEST_DUALPIC_CALC_T));
}

/******************************************************************************
* function : DualPicCalc Test sample signal handle
******************************************************************************/
AX_VOID SAMPLE_IVE_DualPicCalc_TEST_HandleSig(AX_VOID)
{
    SAMPLE_IVE_TestDualPic_Uninit(&s_stTestDualPicCalc);
    memset(&s_stTestDualPicCalc, 0, sizeof(TEST_DUALPIC_CALC_T));

    AX_IVE_Exit();
    AX_SYS_Deinit();
}
