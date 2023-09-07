/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef SAMPLE_IVE_COMMON__h
#define SAMPLE_IVE_COMMON__h

#include "ax_sys_api.h"
#include "ax_ive_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define AX_IVE_CMM_TOKEN "ax_ive"
#define AX_IVE_ALIGN   16
#define ALIGN_UP(x, a)  ( ( ((x) + ((a) - 1) ) / a ) * a )
#define ALIGN_DOWN(x, a)  ( ( (x) / (a)) * (a) )
#ifndef AX_UL
    typedef unsigned long AX_UL;
#endif

#define IMAGE_TYPE_SPECIFY(input, default) (((input) < 0) ? (default) : (input))

#define SAMPLE_IVE_PRT(fmt...)   \
do {\
    printf("[%s]-%d: ", __FUNCTION__, __LINE__);\
    printf(fmt);\
}while(0)

#define SAMPLE_IVE_CHECK_EXPR_RET(expr, ret, fmt...)\
do\
{\
    if(expr)\
    {\
        SAMPLE_IVE_PRT(fmt);\
        return (ret);\
    }\
}while(0)

#define SAMPLE_IVE_CHECK_EXPR_GOTO(expr, label, fmt...)\
do\
{\
    if(expr)\
    {\
        SAMPLE_IVE_PRT(fmt);\
        goto label;\
    }\
}while(0)

#define SAMPLE_IVE_CHECK_NULL_POINTER_RET(p, ret) \
do\
{\
    if (!(p))\
    {\
        SAMPLE_IVE_PRT("%s is NULL\n", #p); \
        return (ret); \
    }\
} while (0)

/* free cmm */
#define IVE_CMM_FREE(phy,vir)\
    do{\
        if ((0 != (phy)) && (0 != (vir)))\
        {\
            AX_SYS_MemFree((phy),(AX_VOID *)(AX_UL)(vir));\
            (phy) = 0;\
            (vir) = 0;\
        }\
    }while(0)

extern AX_BOOL g_bAlignNeed;

/* The cJSON structure: */
typedef struct cJSON {
  struct cJSON *next,*prev;/* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
  struct cJSON *child;/* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */

  AX_S32 type;/* The type of the item, as above. */

  AX_CHAR *valuestring;/* The item's string, if type==cJSON_String */
  AX_S32 valueint;/* The item's number, if type==cJSON_Number */
  AX_F64 valuedouble;/* The item's number, if type==cJSON_Number */

  AX_CHAR *string;/* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
} cJSON;

/* Supply a block of JSON, and this returns a cJSON object you can interrogate. Call cJSON_Delete when finished. */
extern cJSON *cJSON_Parse(const AX_CHAR *value);

/* Delete a cJSON entity and all subentities. */
extern AX_VOID cJSON_Delete(cJSON *c);

/* Returns the number of items in an array (or object). */
extern AX_S32 cJSON_GetArraySize(cJSON *array);

/* Retrieve item number "item" from array "array". Returns NULL if unsuccessful. */
extern cJSON *cJSON_GetArrayItem(cJSON *array, AX_S32 item);

/* Get item "string" from object. Case insensitive. */
extern cJSON *cJSON_GetObjectItem(cJSON *object, const AX_CHAR *string);

AX_BOOL SAMPLE_COMM_IVE_CheckDir(AX_CHAR *path);

AX_U64 SAMPLE_COMM_IVE_GetTime_US(AX_VOID);

AX_U16 SAMPLE_COMM_IVE_CalcStride(AX_U32 u32Width, AX_U8 u8Align);

AX_S32 SAMPLE_COMM_IVE_CreateImage(AX_IVE_IMAGE_T* pstImg, AX_IVE_IMAGE_TYPE_E enType, AX_U32 u32Width, AX_U32 u32Height);

AX_S32 SAMPLE_COMM_IVE_CreateMemInfo(AX_IVE_MEM_INFO_T* pstMemInfo, AX_U32 u32Size);

AX_S32 SAMPLE_COMM_IVE_ReadFile(AX_IVE_IMAGE_T* pstImg, FILE* pFp);

AX_S32 SAMPLE_COMM_IVE_WriteFile(AX_IVE_IMAGE_T* pstImg, FILE* pFp);

AX_S32 SAMPLE_COMM_IVE_ReadMemInfoFile(AX_IVE_SRC_MEM_INFO_T* pstMemInfo, FILE* pFp);

AX_S32 SAMPLE_COMM_IVE_WriteMemInfoFile(AX_IVE_DST_MEM_INFO_T* pstMemInfo, FILE* pFp);

AX_S32 SAMPLE_COMM_IVE_CreateImage_WithGlbImgFmt(AX_IVE_IMAGE_T* pstImg, AX_IMG_FORMAT_E enType, AX_U32 u32Width, AX_U32 u32Height);

AX_S32 SAMPLE_COMM_IVE_ReadFile_WithGlbImgFmt(AX_IVE_IMAGE_T* pstImg, FILE* pFp);

AX_S32 SAMPLE_COMM_IVE_WriteFile_WithGlbImgFmt(AX_IVE_IMAGE_T* pstImg, FILE* pFp);

AX_S32 SAMPLE_COMM_IVE_CreateMauBlob(AX_IVE_MAU_BLOB_T *pstBolb, AX_IVE_MAU_DATA_TYPE_E enType, AX_S32 *pShape, AX_U8 u8Dims, AX_BOOL bRandom);

AX_VOID SAMPLE_COMM_IVE_FreeMauBlob(AX_IVE_MAU_BLOB_T *pstBolb);

AX_VOID SAMPLE_COMM_IVE_Invalid_MauBlobCache(AX_IVE_MAU_BLOB_T *pstBolb);

AX_S32 SAMPLE_COMM_IVE_Bits_MAU_DataType(AX_IVE_MAU_DATA_TYPE_E enType);

#ifdef __cplusplus
}
#endif

#endif
