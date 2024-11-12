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

typedef struct axTEST_CCL_T {
    AX_IVE_SRC_IMAGE_T stSrc;
    AX_IVE_DST_IMAGE_T stDst;
    AX_IVE_DST_IMAGE_T stDstFinal;
    AX_IVE_DST_MEM_INFO_T stBlob;
    AX_IVE_CCL_CTRL_T stCclCtrl;
    FILE* pFpSrc;
    FILE* pFpDst;
} TEST_CCL_T;
static TEST_CCL_T s_stTestCcl;
static AX_IVE_DST_MEM_INFO_T stDumpStat[6];
static FILE* pFpDumpStat[6];

/******************************************************************************
* function : parse ccl parameters
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestCcl_ParseParams(TEST_CCL_T* pstTestCcl, AX_CHAR *pchParamsList)
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
    pstTestCcl->stCclCtrl.enMode = (AX_IVE_CCL_MODE_E)item->valueint;
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
* function : test ccl uninit
******************************************************************************/
static AX_VOID SAMPLE_IVE_TestCcl_Uninit(TEST_CCL_T* pstTestCcl)
{
    IVE_CMM_FREE(pstTestCcl->stSrc.au64PhyAddr[0], pstTestCcl->stSrc.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestCcl->stDst.au64PhyAddr[0], pstTestCcl->stDst.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestCcl->stDstFinal.au64PhyAddr[0], pstTestCcl->stDstFinal.au64VirAddr[0]);
    IVE_CMM_FREE(pstTestCcl->stBlob.u64PhyAddr, pstTestCcl->stBlob.u64VirAddr);

    if (NULL != pstTestCcl->pFpSrc) {
        fclose(pstTestCcl->pFpSrc);
        pstTestCcl->pFpSrc = NULL;
    }
    if (NULL != pstTestCcl->pFpDst) {
        fclose(pstTestCcl->pFpDst);
        pstTestCcl->pFpDst = NULL;
    }
    for (AX_S32 i = 0; i < 6; i++) {
        IVE_CMM_FREE(stDumpStat[i].u64PhyAddr, stDumpStat[i].u64VirAddr);
        if (NULL != pFpDumpStat[i]) {
            fclose(pFpDumpStat[i]);
            pFpDumpStat[i] = NULL;
        }
    }
}
/******************************************************************************
* function : test ccl init
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestCcl_Init(TEST_CCL_T* pstTestCcl, AX_CHAR* pchSrcFileName,
    AX_CHAR* pchDstFileName,  AX_CHAR* pchDstMemInfoName, AX_U32 u32Width, AX_U32 u32Height, AX_S32 as32Type[])
{
    AX_S32 s32Ret = AX_FAILURE;
    AX_U32 u32Size = 0;
    memset(pstTestCcl, 0, sizeof(TEST_CCL_T));

    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestCcl->stSrc), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[0], AX_IVE_IMAGE_TYPE_U8C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create src image failed!\n", s32Ret);
        SAMPLE_IVE_TestCcl_Uninit(pstTestCcl);
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestCcl->stDst), (AX_IVE_IMAGE_TYPE_E)IMAGE_TYPE_SPECIFY(as32Type[1], AX_IVE_IMAGE_TYPE_U32C1), u32Width, u32Height);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create dst image failed!\n", s32Ret);
        SAMPLE_IVE_TestCcl_Uninit(pstTestCcl);
        return s32Ret;
    }
    /* Only for validation results */
    s32Ret = SAMPLE_COMM_IVE_CreateImage(&(pstTestCcl->stDstFinal), AX_IVE_IMAGE_TYPE_U8C3_PACKAGE, u32Width, u32Height);//24bit
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create dst image failed!\n", s32Ret);
        SAMPLE_IVE_TestCcl_Uninit(pstTestCcl);
        return s32Ret;
    }
    u32Size = sizeof(AX_IVE_CCBLOB_T);
    s32Ret = SAMPLE_COMM_IVE_CreateMemInfo(&pstTestCcl->stBlob, u32Size);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create ccl blob mem failed!\n", s32Ret);
        SAMPLE_IVE_TestCcl_Uninit(pstTestCcl);
        return s32Ret;
    }

    s32Ret = AX_FAILURE;
    pstTestCcl->pFpSrc = fopen(pchSrcFileName, "rb");
    if (AX_NULL == pstTestCcl->pFpSrc) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchSrcFileName);
        SAMPLE_IVE_TestCcl_Uninit(pstTestCcl);
        return s32Ret;
    }
    pstTestCcl->pFpDst = fopen(pchDstFileName, "wb");
    if (AX_NULL == pstTestCcl->pFpDst) {
        SAMPLE_IVE_PRT("Error,Open file %s failed!\n", pchDstFileName);
        SAMPLE_IVE_TestCcl_Uninit(pstTestCcl);
        return s32Ret;
    }
    AX_CHAR FileName[256] = {0};
    AX_CHAR FileNamePrefix[128] = {0};
    snprintf(FileNamePrefix, 127, "%s/ccl_blob_mode%d_%dx%d", pchDstMemInfoName, pstTestCcl->stSrc.u32Width, pstTestCcl->stSrc.u32Height, pstTestCcl->stCclCtrl.enMode);
    for (AX_S32 i = 0; i < 6; i++) {
        if (i == 0)
            snprintf(FileName, 255, "%s_label_status.bin", FileNamePrefix);
        else if (i == 1)
            snprintf(FileName, 255, "%s_area.bin", FileNamePrefix);
        else if (i == 2)
            snprintf(FileName, 255, "%s_left.bin", FileNamePrefix);
        else if (i == 3)
            snprintf(FileName, 255, "%s_right.bin", FileNamePrefix);
        else if (i == 4)
            snprintf(FileName, 255, "%s_top.bin", FileNamePrefix);
        else if (i == 5)
            snprintf(FileName, 255, "%s_bottom.bin", FileNamePrefix);
        pFpDumpStat[i] = fopen(FileName, "wb");
        if (AX_NULL == pFpDumpStat[i]) {
            SAMPLE_IVE_PRT("Error,Open file %s failed!\n", FileName);
            return s32Ret;
        }
    }
    return AX_SUCCESS;
}

static AX_VOID SAMPLE_IVE_CCL_OutImageConvert(const AX_IVE_IMAGE_T* pstSrcImg, AX_IVE_IMAGE_T* pstDstImg)
{
    AX_U32 size = ALIGN_UP(pstSrcImg->u32Width * pstSrcImg->u32Height * 24 / 8, 8);
    for (AX_S32 i = 0, j = 0; i < size; i+=3, j++) {
        *((AX_U8 *)pstDstImg->au64VirAddr[0] + i) = (*((AX_U32 *)pstSrcImg->au64VirAddr[0] + j)) & 0xFF;
        *((AX_U8 *)pstDstImg->au64VirAddr[0] + i + 1) = (*((AX_U32 *)pstSrcImg->au64VirAddr[0] + j) >> 8) & 0xFF;
        *((AX_U8 *)pstDstImg->au64VirAddr[0] + i + 2) = (*((AX_U32 *)pstSrcImg->au64VirAddr[0] + j) >> 16) & 0x3;
    }
}

static AX_VOID SAMPLE_IVE_CCL_GetOutStat(const AX_IVE_MEM_INFO_T* pstSrcMem)
{
    AX_IVE_CCBLOB_T stBlob = {0};
    memcpy(&stBlob, (AX_VOID *)pstSrcMem->u64VirAddr, sizeof(AX_IVE_CCBLOB_T));
    for (AX_S32 i = 0; i < 6; i++) {
        if (i == 0) { //label_status
            for (AX_S32 k = 0, j = 0; j < stDumpStat[i].u32Size; k++, j++) {
                *((AX_U8 *)stDumpStat[i].u64VirAddr + j) = stBlob.astRegion[k].u8LabelStatus & 0x1;
            }
        } else if (i == 1) { //area
            for (AX_S32 k = 0, j = 0; j < stDumpStat[i].u32Size; k++, j+=3) {
                *((AX_U8 *)stDumpStat[i].u64VirAddr + j + 2) = (stBlob.astRegion[k].u32Area >> 16) & 0xF;
                *((AX_U8 *)stDumpStat[i].u64VirAddr + j + 1) = (stBlob.astRegion[k].u32Area >> 8) & 0xFF;
                *((AX_U8 *)stDumpStat[i].u64VirAddr + j) = (stBlob.astRegion[k].u32Area) & 0xFF;
            }
        } else if (i == 2) { //left
            for (AX_S32 k = 0, j = 0; j < stDumpStat[i].u32Size; k++, j+=2) {
                *((AX_U8 *)stDumpStat[i].u64VirAddr + j + 1) = (stBlob.astRegion[k].u16Left >> 8) & 0x7;
                *((AX_U8 *)stDumpStat[i].u64VirAddr + j) = (stBlob.astRegion[k].u16Left) & 0xFF;
            }
        } else if (i == 3) { //right
            for (AX_S32 k = 0, j = 0; j < stDumpStat[i].u32Size; k++, j+=2) {
                *((AX_U8 *)stDumpStat[i].u64VirAddr + j + 1) = (stBlob.astRegion[k].u16Right >> 8) & 0x7;
                *((AX_U8 *)stDumpStat[i].u64VirAddr + j) = (stBlob.astRegion[k].u16Right) & 0xFF;
            }
        } else if (i == 4) { //top
            for (AX_S32 k = 0, j = 0; j < stDumpStat[i].u32Size; k++, j+=2) {
                *((AX_U8 *)stDumpStat[i].u64VirAddr + j + 1) = (stBlob.astRegion[k].u16Top >> 8) & 0x3;
                *((AX_U8 *)stDumpStat[i].u64VirAddr + j) = (stBlob.astRegion[k].u16Top) & 0xFF;
            }
        } else if (i == 5) { //bottom
            for (AX_S32 k = 0, j = 0; j < stDumpStat[i].u32Size; k++, j+=2) {
                *((AX_U8 *)stDumpStat[i].u64VirAddr + j + 1) = (stBlob.astRegion[k].u16Bottom >> 8) & 0x3;
                *((AX_U8 *)stDumpStat[i].u64VirAddr + j) = (stBlob.astRegion[k].u16Bottom) & 0xFF;
            }
        }
    }
}

static AX_S32 SAMPLE_IVE_CCL_WriteFile(AX_IVE_IMAGE_T* pstImage, FILE* pFp)
{
    AX_U8* pU8;
    AX_U32 size = pstImage->u32Height * pstImage->u32Width * 24 / 8;
    pU8 = (AX_U8 *)(AX_UL)pstImage->au64VirAddr[0];
    if ( 1 != fwrite(pU8, size, 1, pFp) ){
        SAMPLE_IVE_PRT("Write file fail\n");
        return AX_FAILURE;
    }
    return AX_SUCCESS;
}
/******************************************************************************
* function : test ccl
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestCclProc(TEST_CCL_T* pstTestCcl, AX_CHAR *pchParamsList)
{
    AX_S32 s32Ret;
    AX_IVE_HANDLE IveHandle;
    AX_BOOL bInstant = AX_FALSE;
    AX_BOOL bBlock = AX_TRUE;
    AX_BOOL bFinish = AX_FALSE;
    AX_U32 u32BlobCnt = 0;

    if (pchParamsList) {
        s32Ret = SAMPLE_IVE_TestCcl_ParseParams(pstTestCcl, pchParamsList);
        if (AX_SUCCESS != s32Ret)
            return s32Ret;
    } else {
        pstTestCcl->stCclCtrl.enMode = AX_IVE_CCL_MODE_8C;
    }
    s32Ret = SAMPLE_COMM_IVE_ReadFile(&(pstTestCcl->stSrc), pstTestCcl->pFpSrc);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Read src1 file failed!\n",s32Ret);
        return s32Ret;
    }

    bInstant = AX_TRUE;
    AX_U64 u64StartTime = SAMPLE_COMM_IVE_GetTime_US();
    s32Ret = AX_IVE_CCL(&IveHandle, &pstTestCcl->stSrc, &pstTestCcl->stDst, &pstTestCcl->stBlob, &pstTestCcl->stCclCtrl, bInstant);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),AX_IVE_CCL failed!\n",s32Ret);
        return s32Ret;
    }
    if (bInstant == AX_FALSE) {
        s32Ret = AX_IVE_Query(IveHandle, &bFinish, bBlock);
        while (AX_ERR_IVE_QUERY_TIMEOUT == s32Ret){
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
    printf("Run CCL task cost %lld us\n", u64EndTime - u64StartTime);

    u32BlobCnt = *((AX_U16 *)pstTestCcl->stBlob.u64VirAddr);
    SAMPLE_IVE_PRT("u32BlobCnt:%d\n", u32BlobCnt);

    SAMPLE_IVE_CCL_OutImageConvert(&pstTestCcl->stDst, &pstTestCcl->stDstFinal);
    s32Ret = SAMPLE_IVE_CCL_WriteFile(&pstTestCcl->stDstFinal, pstTestCcl->pFpDst);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error,Write dst file failed!\n");
        return s32Ret;
    }

    if (u32BlobCnt > 0) {
        AX_U32 bit_width;
        for (AX_S32 i = 0; i < 6; i++) {
            if (i == 0)
                bit_width = 8;
            else if (i == 1)
                bit_width = 24;
            else
                bit_width = 16;
            AX_U32 u32Size = u32BlobCnt * 1 * bit_width / 8;
            s32Ret = SAMPLE_COMM_IVE_CreateMemInfo(&stDumpStat[i], u32Size);
            if (AX_SUCCESS != s32Ret) {
                SAMPLE_IVE_PRT("Error(%#x),Create ccl mem failed!\n", s32Ret);
                SAMPLE_IVE_TestCcl_Uninit(pstTestCcl);
                return s32Ret;
            }
        }

        SAMPLE_IVE_CCL_GetOutStat(&pstTestCcl->stBlob);
        for (AX_S32 i = 0; i < 6; i++) {
            s32Ret = SAMPLE_COMM_IVE_WriteMemInfoFile(&stDumpStat[i], pFpDumpStat[i]);
        }
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_IVE_PRT("Error,Write meminfo file failed!\n");
            return s32Ret;
        }
    }

    return s32Ret;
}

/******************************************************************************
* function : Show test ccl sample
******************************************************************************/
AX_VOID SAMPLE_IVE_CCL_TEST(AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR **pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList)
{
    AX_S32 s32Ret;
    AX_U32 u32Width = u32WidthSrc;
    AX_U32 u32Height = u32HeightSrc;
    AX_CHAR* pchSrcFile = pchSrcPath;
    AX_CHAR* pchDstFile = pchDstPath[0];
    AX_CHAR* pchMemInfoFile = pchDstPath[1];
    if (!pchSrcFile || !pchDstFile || !pchMemInfoFile) {
        SAMPLE_IVE_PRT("Error: null pointer(src or dst path not specified)!\n");
        return;
    }
    if(AX_TRUE != SAMPLE_COMM_IVE_CheckDir(pchMemInfoFile)) {
        SAMPLE_IVE_PRT("Error: dst2(blob info) must be specified as directory!\n");
        return;
    }
    memset(&s_stTestCcl, 0, sizeof(TEST_CCL_T));
    s32Ret = SAMPLE_IVE_TestCcl_Init(&s_stTestCcl, pchSrcFile, pchDstFile, pchMemInfoFile, u32Width, u32Height, as32Type);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),SAMPLE_IVE_TestCcl_Init failed!\n", s32Ret);
        return;
    }

    s32Ret = SAMPLE_IVE_TestCclProc(&s_stTestCcl, pchParamsList);
    if (AX_SUCCESS == s32Ret)
        SAMPLE_IVE_PRT("Process success!\n");
    else
        SAMPLE_IVE_PRT("Error:process failed!\n");

    SAMPLE_IVE_TestCcl_Uninit(&s_stTestCcl);
    memset(&s_stTestCcl, 0, sizeof(TEST_CCL_T));
}

/******************************************************************************
* function : Test CCL sample signal handle
******************************************************************************/
AX_VOID SAMPLE_IVE_CCL_TEST_HandleSig(AX_VOID)
{
    SAMPLE_IVE_TestCcl_Uninit(&s_stTestCcl);
    memset(&s_stTestCcl, 0, sizeof(TEST_CCL_T));

    AX_IVE_Exit();
    AX_SYS_Deinit();
}
