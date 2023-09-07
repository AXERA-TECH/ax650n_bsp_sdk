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
#include "ax_engine_api.h"

#define MAT_DIM (2)

typedef struct axTEST_MAT_MUL_T {
    AX_IVE_MAU_MATMUL_INPUT_T stMatMulInput;
    AX_IVE_MAU_MATMUL_OUTPUT_T stMatMulOutput;
    AX_IVE_MAU_MATMUL_CTRL_T stMatMulCtrl;
} TEST_MAT_MUL_T;

typedef struct {
    AX_U32 index;
    AX_F32 score;
} TopNRes_T;

static TEST_MAT_MUL_T s_stTestMatMul;
static AX_BOOL g_bCheck = AX_TRUE;
static AX_IVE_MAU_DATA_TYPE_E g_enDataTypeIn = 0;
static AX_IVE_MAU_DATA_TYPE_E g_enDataTypeMulRes = 0;
static AX_IVE_MAU_DATA_TYPE_E g_enDataTypeTopNRes = 0;
static AX_S32 g_MatQShape[MAT_DIM] = {0};
static AX_S32 g_MatBShape[MAT_DIM] = {0};

static AX_BOOL SAMPLE_IVE_ElemEqual_WithType(AX_VOID *pData1, AX_VOID *pData2, size_t Size, AX_IVE_MAU_DATA_TYPE_E enType)
{
    AX_S32 i;
    AX_S32 s32TypeSize = SAMPLE_COMM_IVE_Bits_MAU_DataType(enType) / 8;
    if (s32TypeSize == AX_FAILURE)
        return AX_FALSE;

    for (i = 0; i < Size; i++) {
        AX_VOID *pData1Addr = (AX_CHAR *)pData1 + i * s32TypeSize;
        AX_VOID *pData2Addr = (AX_CHAR *)pData2 + i * s32TypeSize;
        if (memcmp(pData1Addr, pData2Addr, s32TypeSize) != 0) {
            SAMPLE_IVE_PRT("Not equal index: %d\n", i);
            return AX_FALSE;
        }
    }

    return AX_TRUE;
}

static AX_BOOL SAMPLE_IVE_Compare_MauBlopData2(AX_IVE_MAU_BLOB_T *pData1, AX_IVE_MAU_BLOB_T *pData2)
{
    SAMPLE_IVE_CHECK_NULL_POINTER_RET(pData1, AX_FALSE);
    SAMPLE_IVE_CHECK_NULL_POINTER_RET(pData2, AX_FALSE);
    if (pData1->enDataType != pData2->enDataType || pData1->u8ShapeSize != pData2->u8ShapeSize) {
        SAMPLE_IVE_PRT("Data1 data_type:%d shape_size:%d not equel to Data2 data_type:%d shape_size:%d\n",
        pData1->enDataType, pData1->u8ShapeSize, pData2->enDataType, pData2->u8ShapeSize);
        return AX_FALSE;
    }

    size_t total_elem_num = 1;
    for (AX_S32 i = 0; i <  pData1->u8ShapeSize; i++) {
        if (pData1->pShape[i] != pData2->pShape[i]) {
            SAMPLE_IVE_PRT("Data1 shape[%d]:%d not equel to Data2 shape[%d]:%d\n",
            i, pData1->pShape[i], i, pData2->pShape[i]);
            return AX_FALSE;
        }
        total_elem_num *= pData1->pShape[i];
    }
    if (!SAMPLE_IVE_ElemEqual_WithType(pData1->pVirAddr, pData2->pVirAddr, total_elem_num, pData1->enDataType)) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

static inline AX_F32 Vec_Sum(const AX_F32* pStart, size_t Len)
{
    if (Len == 2)
        return (AX_F32)pStart[0] + (AX_F32)pStart[1];

    return Vec_Sum(pStart, Len / 2) + Vec_Sum(pStart + Len / 2, Len / 2);
}

#define VEC_MUL_WITH_DIFF_TYPE(res, data1, data2, DATA_TYPE, Len, Concurrency, bPatitionSum) \
    do {                                                                            \
        AX_VOID *pResult = (AX_VOID *)res;                                          \
        DATA_TYPE *pData1 = (DATA_TYPE *)data1;                                     \
        DATA_TYPE *pData2 = (DATA_TYPE *)data2;                                     \
        AX_F32 part_sum[4] = {0, 0, 0, 0};                                          \
        for (size_t i = 0; i < Len / Concurrency; i++) {                            \
            AX_F32 chunk[Concurrency];                                              \
            for (size_t j = 0; j < Concurrency; j++)                                \
                chunk[j] = (AX_F32)pData1[i * Concurrency + j] * (AX_F32)pData2[i * Concurrency + j]; \
            part_sum[i % (bPatitionSum ? 4 : 1)] = part_sum[i % (bPatitionSum ? 4 : 1)] + Vec_Sum(chunk, Concurrency); \
        }                                                                           \
        part_sum[0] = part_sum[0] + part_sum[1];                                    \
        part_sum[2] = part_sum[2] + part_sum[3];                                    \
        AX_F32* f32Res = (AX_F32*)pResult;                                          \
        *f32Res = part_sum[0] + part_sum[2];                                        \
    } while (0);

static AX_S32 SAMPLE_IVE_MatMul_Reference(AX_IVE_MAU_BLOB_T *pData1, AX_IVE_MAU_BLOB_T *pData2, AX_IVE_MAU_BLOB_T *pDataOut)
{
    SAMPLE_IVE_CHECK_NULL_POINTER_RET(pData1, AX_FAILURE);
    SAMPLE_IVE_CHECK_NULL_POINTER_RET(pData2, AX_FAILURE);
    SAMPLE_IVE_CHECK_NULL_POINTER_RET(pDataOut, AX_FAILURE);
    if (pData1->u8ShapeSize != 2 || pData2->u8ShapeSize != 2 || pDataOut->u8ShapeSize != 2) {
        SAMPLE_IVE_PRT("Only support shape_size = 2\n");
        return AX_FAILURE;
    }
    if (pData1->pShape[0] != pDataOut->pShape[0] || pData2->pShape[0] != pDataOut->pShape[1]) {
        SAMPLE_IVE_PRT("Shape error!\n");
        return AX_FAILURE;
    }
    AX_U32 len = pData1->pShape[1];
    for (AX_S32 i = 0; i < pData1->pShape[0]; i++) {
        for (AX_S32 j = 0; j < pData2->pShape[0]; j++) {
            AX_U64 data1_start = (AX_U64)pData1->pVirAddr + i * len * SAMPLE_COMM_IVE_Bits_MAU_DataType(pData1->enDataType) / 8;
            AX_U64 data2_start = (AX_U64)pData2->pVirAddr + j * len * SAMPLE_COMM_IVE_Bits_MAU_DataType(pData2->enDataType) / 8;
            AX_U64 out_start = (AX_U64)pDataOut->pVirAddr + (i * pData2->pShape[0] + j) * 4;
            switch(pData1->enDataType) {
            case AX_IVE_MAU_DT_UINT8:
                VEC_MUL_WITH_DIFF_TYPE(out_start, data1_start, data2_start, AX_U8, len, 8, AX_FALSE);
                break;
            case AX_IVE_MAU_DT_SINT8:
                VEC_MUL_WITH_DIFF_TYPE(out_start, data1_start, data2_start, AX_S8, len, 8, AX_FALSE);
                break;
            case AX_IVE_MAU_DT_UINT16:
                VEC_MUL_WITH_DIFF_TYPE(out_start, data1_start, data2_start, AX_U16, len, 8, AX_FALSE);
                break;
            case AX_IVE_MAU_DT_SINT16:
            case AX_IVE_MAU_DT_FLOAT16:
            case AX_IVE_MAU_DT_BFLOAT16:
                VEC_MUL_WITH_DIFF_TYPE(out_start, data1_start, data2_start, AX_S16, len, 8, AX_FALSE);
                break;
            case AX_IVE_MAU_DT_UINT32:
                VEC_MUL_WITH_DIFF_TYPE(out_start, data1_start, data2_start, AX_U32, len, 8, AX_FALSE);
                break;
            case AX_IVE_MAU_DT_SINT32:
                VEC_MUL_WITH_DIFF_TYPE(out_start, data1_start, data2_start, AX_S32, len, 8, AX_FALSE);
                break;
            case AX_IVE_MAU_DT_FLOAT32:
                VEC_MUL_WITH_DIFF_TYPE(out_start, data1_start, data2_start, AX_F32, len, 8, AX_FALSE);
                break;
            case AX_IVE_MAU_DT_UINT64:
                VEC_MUL_WITH_DIFF_TYPE(out_start, data1_start, data2_start, AX_U64, len, 8, AX_FALSE);
                break;
            case AX_IVE_MAU_DT_SINT64:
                VEC_MUL_WITH_DIFF_TYPE(out_start, data1_start, data2_start, AX_S64, len, 8, AX_FALSE);
                break;
            default:
                SAMPLE_IVE_PRT("No support mau data type:%d!\n", pData1->enDataType);
                return AX_FAILURE;
            }
        }
    }
    return AX_SUCCESS;
}

static AX_VOID Swap_TopNRes(TopNRes_T *pData1, TopNRes_T *pData2)
{
    TopNRes_T tmp = {0};
    tmp.index = pData1->index;
    tmp.score = pData1->score;
    pData1->index = pData2->index;
    pData1->score = pData2->score;
    pData2->index = tmp.index;
    pData2->score = tmp.score;
}

static AX_S32 SAMPLE_IVE_Toptopn_reference(AX_IVE_MAU_BLOB_T *pDataM, AX_IVE_MAU_BLOB_T *pDataTopN, AX_S32 st32QSize, AX_S32 stNSize, AX_IVE_MAU_ORDER_E enOrder)
{
    SAMPLE_IVE_CHECK_NULL_POINTER_RET(pDataM, AX_FAILURE);
    SAMPLE_IVE_CHECK_NULL_POINTER_RET(pDataTopN, AX_FAILURE);
    if (pDataM->u8ShapeSize != 2 || pDataTopN->u8ShapeSize != 2) {
        SAMPLE_IVE_PRT("Only support shape_size = 2\n");
        return AX_FAILURE;
    }
    if (pDataM->pShape[1] != st32QSize || pDataTopN->pShape[0] != st32QSize || pDataTopN->pShape[1] != stNSize) {
        SAMPLE_IVE_PRT("Shape error!\n");
        return AX_FAILURE;
    }

    AX_F32 init_value = enOrder == AX_IVE_MAU_ORDER_ASCEND ? FLT_MAX : FLT_MIN;

    for (AX_S32 i = 0; i < pDataTopN->pShape[0] * pDataTopN->pShape[1]; i++) {
        TopNRes_T *res   = (TopNRes_T*)pDataTopN->pVirAddr + i;
        res->index = 0;
        res->score = init_value;
    }

    for (AX_S32 i = 0; i < pDataM->pShape[0] * pDataM->pShape[1]; i++) {
        AX_F32 score = ((AX_F32*)pDataM->pVirAddr)[i];
        AX_U32 q_index = i % st32QSize;
        AX_U32 b_index = i / st32QSize;
        AX_U64 topn_start_addr = (AX_U64)pDataTopN->pVirAddr + stNSize * q_index * sizeof(TopNRes_T);
        TopNRes_T *topn_start = (TopNRes_T*)topn_start_addr;
        TopNRes_T current = {b_index, score};
        if (enOrder == AX_IVE_MAU_ORDER_ASCEND) {
            if (current.score >=  topn_start[stNSize - 1].score)
                continue;
        } else {
            if (current.score <=  topn_start[stNSize - 1].score)
                continue;
        }
        topn_start[stNSize - 1] = current;
        for (AX_S32 j = stNSize - 1; j > 0; j--) {
            if (enOrder == AX_IVE_MAU_ORDER_ASCEND) {
                if (topn_start[j].score >=  topn_start[j - 1].score)
                    break;
            } else {
                if (topn_start[j].score <=  topn_start[j - 1].score)
                    break;
            }
            Swap_TopNRes(&topn_start[j], &topn_start[j - 1]);
        }
    }
    AX_SYS_MflushCache(pDataTopN->u64PhyAddr, pDataTopN->pVirAddr, pDataTopN->pShape[0] * pDataTopN->pShape[1] * SAMPLE_COMM_IVE_Bits_MAU_DataType(pDataTopN->enDataType) / 8);
    return AX_SUCCESS;
}

static AX_BOOL SAMPLE_IVE_Compare_TopN(AX_IVE_MAU_BLOB_T *pData1, AX_IVE_MAU_BLOB_T *pData2)
{
    SAMPLE_IVE_CHECK_NULL_POINTER_RET(pData1, AX_FALSE);
    SAMPLE_IVE_CHECK_NULL_POINTER_RET(pData2, AX_FALSE);
    if (pData1->u8ShapeSize != pData2->u8ShapeSize) {
        SAMPLE_IVE_PRT("pData1 shape_size:%d not equel to pData2 shape_size:%d\n", pData1->u8ShapeSize, pData2->u8ShapeSize);
        return AX_FALSE;
    }
    size_t total_elem_num = 1;
    for (AX_S32 i = 0; i < pData1->u8ShapeSize; i++) {
        if (pData1->pShape[i] != pData2->pShape[i]) {
            SAMPLE_IVE_PRT("pData1 shape[%d]:%d not equel to pData2 shape[%d]:%d\n",
            i, pData1->pShape[i], i, pData2->pShape[i]);
            return AX_FALSE;
        }
        total_elem_num *= pData1->pShape[i];
    }
    for (AX_S32 i = 0; i < total_elem_num; i++) {
        TopNRes_T* data1_item = (TopNRes_T*)pData1->pVirAddr + i;
        TopNRes_T* data2_item = (TopNRes_T*)pData2->pVirAddr + i;
        if (data1_item->score != data2_item->score) {
            SAMPLE_IVE_PRT("data1:%f not equal to data2:%f, index: %d\n", data1_item->score, data2_item->score, i);
            return AX_FALSE;
        }
    }
    return AX_TRUE;
}


/******************************************************************************
* function : parse mat mul parameters
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestMatMul_ParseParams(TEST_MAT_MUL_T* pstTestMatMul, AX_CHAR *pchParamsList)
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
    item = cJSON_GetObjectItem(root, "mau_id");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param mau_id failed!\n");
    pstTestMatMul->stMatMulCtrl.enMauId = item->valueint;
    item = cJSON_GetObjectItem(root, "ddr_rdw");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param ddr_rdw failed!\n");
    pstTestMatMul->stMatMulCtrl.s32DdrReadBandwidthLimit = item->valueint;
    item = cJSON_GetObjectItem(root, "en_mul_res");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param en_mul_res failed!\n");
    pstTestMatMul->stMatMulCtrl.bEnableMulRes = item->valueint;
    item = cJSON_GetObjectItem(root, "en_topn_res");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param en_topn_res failed!\n");
    pstTestMatMul->stMatMulCtrl.bEnableTopNRes = item->valueint;
    item = cJSON_GetObjectItem(root, "order");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param order failed!\n");
    pstTestMatMul->stMatMulCtrl.enOrder = item->valueint;
    item = cJSON_GetObjectItem(root, "topn");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param topn failed!\n");
    pstTestMatMul->stMatMulCtrl.s32TopN = item->valueint;
    item = cJSON_GetObjectItem(root, "type_in");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param type_in failed!\n");
    g_enDataTypeIn = (AX_IVE_MAU_DATA_TYPE_E)item->valueint;
    item = cJSON_GetObjectItem(root, "type_mul_res");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param type_mul_res failed!\n");
    g_enDataTypeMulRes = (AX_IVE_MAU_DATA_TYPE_E)item->valueint;
    item = cJSON_GetObjectItem(root, "type_topn_res");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item, PARSE_FAIL, "Error:parse param type_topn_res failed!\n");
    g_enDataTypeTopNRes = (AX_IVE_MAU_DATA_TYPE_E)item->valueint;

    item_array = cJSON_GetObjectItem(root, "q_shape");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item_array, PARSE_FAIL, "Error:parse param q_shape failed!\n");
    AX_U32 u32ArraySize = cJSON_GetArraySize(item_array);
    SAMPLE_IVE_CHECK_EXPR_GOTO(u32ArraySize != MAT_DIM, PARSE_FAIL,
        "Error:u32ArraySize[%d] is not equal to %d!\n",u32ArraySize, MAT_DIM);
    for (AX_S32 i = 0; i < u32ArraySize; i++) {
        cJSON *item_shape = cJSON_GetArrayItem(item_array, i);
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item_shape, PARSE_FAIL, "Error:parse param q_shape failed!\n");
        g_MatQShape[i] = item_shape->valueint;
    }

    item_array = cJSON_GetObjectItem(root, "b_shape");
    SAMPLE_IVE_CHECK_EXPR_GOTO(!item_array, PARSE_FAIL, "Error:parse param b_shape failed!\n");
    u32ArraySize = cJSON_GetArraySize(item_array);
    SAMPLE_IVE_CHECK_EXPR_GOTO(u32ArraySize != MAT_DIM, PARSE_FAIL,
        "Error:u32ArraySize[%d] is not equal to %d!\n",u32ArraySize, MAT_DIM);
    for (AX_S32 i = 0; i < u32ArraySize; i++) {
        cJSON *item_shape = cJSON_GetArrayItem(item_array, i);
        SAMPLE_IVE_CHECK_EXPR_GOTO(!item_shape, PARSE_FAIL, "Error:parse param b_shape failed!\n");
        g_MatBShape[i] = item_shape->valueint;
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

static AX_S32 Sample_MatMul_Check_Result(TEST_MAT_MUL_T* pstTestMatMul, AX_BOOL bCheck)
{
    if (bCheck) {
        AX_IVE_MAU_BLOB_T res_reference = {0};
        AX_S32 ref_shape[MAT_DIM] = {g_MatBShape[0], g_MatQShape[0]};
        AX_S32 s32Ret = SAMPLE_COMM_IVE_CreateMauBlob(&res_reference, pstTestMatMul->stMatMulOutput.stMulRes.enDataType, ref_shape, 2, AX_FALSE);
        if (AX_SUCCESS != s32Ret) {
            SAMPLE_IVE_PRT("Error(%#x),Create mul blob failed!\n", s32Ret);
            return s32Ret;
        }
        SAMPLE_IVE_MatMul_Reference(&pstTestMatMul->stMatMulInput.stMatB, &pstTestMatMul->stMatMulInput.stMatQ, &res_reference);

        if (pstTestMatMul->stMatMulCtrl.bEnableMulRes) {
            SAMPLE_COMM_IVE_Invalid_MauBlobCache(&pstTestMatMul->stMatMulOutput.stMulRes);
            if (SAMPLE_IVE_Compare_MauBlopData2(&pstTestMatMul->stMatMulOutput.stMulRes, &res_reference) != AX_TRUE)
                SAMPLE_IVE_PRT("MatMul hw res is not equal to reference!\n");
            else
                SAMPLE_IVE_PRT("MatMul hw res is equal to reference!\n");
        }

        if (pstTestMatMul->stMatMulCtrl.bEnableTopNRes) {
            AX_IVE_MAU_BLOB_T topn_reference = {0};
            AX_S32 topn_shape[MAT_DIM] = {g_MatQShape[0], pstTestMatMul->stMatMulCtrl.s32TopN};
            s32Ret = SAMPLE_COMM_IVE_CreateMauBlob(&topn_reference, pstTestMatMul->stMatMulOutput.stTopNRes.enDataType, topn_shape, 2, AX_FALSE);
            if (AX_SUCCESS != s32Ret) {
                SAMPLE_IVE_PRT("Error(%#x),Create mul blob failed!\n", s32Ret);
                SAMPLE_COMM_IVE_FreeMauBlob(&res_reference);
                return s32Ret;
            }

            SAMPLE_IVE_Toptopn_reference(&res_reference, &topn_reference, g_MatQShape[0], pstTestMatMul->stMatMulCtrl.s32TopN, pstTestMatMul->stMatMulCtrl.enOrder);

            SAMPLE_COMM_IVE_Invalid_MauBlobCache(&pstTestMatMul->stMatMulOutput.stTopNRes);
            if (SAMPLE_IVE_Compare_TopN(&pstTestMatMul->stMatMulOutput.stTopNRes, &topn_reference) != AX_TRUE)
                SAMPLE_IVE_PRT("TopN hw res is not equal to reference!\n");
            else
                SAMPLE_IVE_PRT("TopN hw res is equal to reference!\n");
            SAMPLE_COMM_IVE_FreeMauBlob(&topn_reference);
        }
        SAMPLE_COMM_IVE_FreeMauBlob(&res_reference);
    }
    return AX_SUCCESS;
}
/******************************************************************************
* function : test matrix multiplication uninit
******************************************************************************/
static AX_VOID SAMPLE_IVE_TestMatMul_Uninit(TEST_MAT_MUL_T* pstTestMatMul)
{
    SAMPLE_COMM_IVE_FreeMauBlob(&pstTestMatMul->stMatMulInput.stMatQ);
    SAMPLE_COMM_IVE_FreeMauBlob(&pstTestMatMul->stMatMulInput.stMatB);
    SAMPLE_COMM_IVE_FreeMauBlob(&pstTestMatMul->stMatMulOutput.stMulRes);
    SAMPLE_COMM_IVE_FreeMauBlob(&pstTestMatMul->stMatMulOutput.stTopNRes);
}
/******************************************************************************
* function : test matrix multiplication init
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestMatMul_Init(TEST_MAT_MUL_T* pstTestMatMul, AX_CHAR *pchParamsList)
{
    AX_S32 s32Ret = AX_FAILURE;

    memset(pstTestMatMul, 0, sizeof(TEST_MAT_MUL_T));
    if (pchParamsList) {
        s32Ret = SAMPLE_IVE_TestMatMul_ParseParams(pstTestMatMul, pchParamsList);
        if (AX_SUCCESS != s32Ret)
            return s32Ret;
    } else {
        pstTestMatMul->stMatMulCtrl.enMauId = AX_IVE_MAU_ID_0;
        pstTestMatMul->stMatMulCtrl.s32DdrReadBandwidthLimit = 0;
        pstTestMatMul->stMatMulCtrl.bEnableMulRes = AX_TRUE;
        pstTestMatMul->stMatMulCtrl.bEnableTopNRes = AX_TRUE;
        pstTestMatMul->stMatMulCtrl.enOrder = AX_IVE_MAU_ORDER_ASCEND;
        pstTestMatMul->stMatMulCtrl.s32TopN = 32;
        g_enDataTypeIn = AX_IVE_MAU_DT_SINT16;
        g_enDataTypeMulRes = AX_IVE_MAU_DT_FLOAT32;
        g_enDataTypeTopNRes = AX_IVE_MAU_DT_SINT64;
        g_MatQShape[0] = 16;
        g_MatQShape[1] = 512;
        g_MatBShape[0] = 1000;
        g_MatBShape[1] = 512;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateMauBlob(&(pstTestMatMul->stMatMulInput.stMatQ), g_enDataTypeIn, g_MatQShape, MAT_DIM, AX_TRUE);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create mat query data failed!\n", s32Ret);
        SAMPLE_IVE_TestMatMul_Uninit(pstTestMatMul);
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateMauBlob(&(pstTestMatMul->stMatMulInput.stMatB), g_enDataTypeIn, g_MatBShape, MAT_DIM, AX_TRUE);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create mat base data failed!\n", s32Ret);
        SAMPLE_IVE_TestMatMul_Uninit(pstTestMatMul);
        return s32Ret;
    }
    AX_S32 ref_shape[MAT_DIM] = {g_MatBShape[0], g_MatQShape[0]};
    AX_S32 topn_shape[MAT_DIM] = {g_MatQShape[0], pstTestMatMul->stMatMulCtrl.s32TopN};
    s32Ret = SAMPLE_COMM_IVE_CreateMauBlob(&(pstTestMatMul->stMatMulOutput.stMulRes), g_enDataTypeMulRes, ref_shape, MAT_DIM, AX_FALSE);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create mul result data failed!\n", s32Ret);
        SAMPLE_IVE_TestMatMul_Uninit(pstTestMatMul);
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_IVE_CreateMauBlob(&(pstTestMatMul->stMatMulOutput.stTopNRes), g_enDataTypeTopNRes, topn_shape, MAT_DIM, AX_FALSE);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),Create topn result data failed!\n", s32Ret);
        SAMPLE_IVE_TestMatMul_Uninit(pstTestMatMul);
        return s32Ret;
    }

    return s32Ret;
}
/******************************************************************************
* function : test matrix multiplication
******************************************************************************/
static AX_S32 SAMPLE_IVE_TestMatMulProc(TEST_MAT_MUL_T* pstTestMatMul)
{
    AX_S32 s32Ret;
    AX_IVE_HANDLE IveHandle;
    AX_BOOL bInstant = AX_TRUE;
    AX_IVE_ENGINE_E enEngine = AX_IVE_ENGINE_MAU;

    s32Ret = AX_IVE_MAU_MatMul(&IveHandle, &pstTestMatMul->stMatMulInput, &pstTestMatMul->stMatMulOutput, &pstTestMatMul->stMatMulCtrl, enEngine, bInstant);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),AX_IVE_MAU_MatMul failed!\n",s32Ret);
        return s32Ret;
    }

    s32Ret = Sample_MatMul_Check_Result(pstTestMatMul, g_bCheck);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error, MatMul result check failed!\n");
    }

    return s32Ret;
}

/******************************************************************************
* function : Show test matrix multiplication sample
******************************************************************************/
AX_VOID SAMPLE_IVE_MatMul_TEST(AX_CHAR *pchParamsList)
{
    AX_S32 s32Ret;

    memset(&s_stTestMatMul,0,sizeof(TEST_MAT_MUL_T));
    s32Ret = SAMPLE_IVE_TestMatMul_Init(&s_stTestMatMul, pchParamsList);
    if (AX_SUCCESS != s32Ret) {
        SAMPLE_IVE_PRT("Error(%#x),SAMPLE_IVE_TestMatMul_Init failed!\n", s32Ret);
        return;
    }

    s32Ret = SAMPLE_IVE_TestMatMulProc(&s_stTestMatMul);
    if (AX_SUCCESS == s32Ret)
        SAMPLE_IVE_PRT("Process success!\n");
    else
        SAMPLE_IVE_PRT("Process fail!\n");

    SAMPLE_IVE_TestMatMul_Uninit(&s_stTestMatMul);
    memset(&s_stTestMatMul,0,sizeof(TEST_MAT_MUL_T));
}

/******************************************************************************
* function : Test matrix multiplication sample signal handle
******************************************************************************/
AX_VOID SAMPLE_IVE_MatMul_TEST_HandleSig(AX_BOOL bInitEngine)
{
    SAMPLE_IVE_TestMatMul_Uninit(&s_stTestMatMul);
    memset(&s_stTestMatMul,0,sizeof(TEST_MAT_MUL_T));

    if (bInitEngine) {
        AX_ENGINE_Deinit();
    }
    AX_IVE_Exit();
    AX_SYS_Deinit();

}
