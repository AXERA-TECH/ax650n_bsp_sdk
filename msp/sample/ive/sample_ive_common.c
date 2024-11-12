/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "sample_ive_common.h"

#define RAND_U16  ((AX_U16)(rand() % 65536))
#define RAND_S16  ((AX_S16)((rand() % 65536) - 32768))

/* JSON parser in C. */
/* cJSON Types: */
#define cJSON_False 0
#define cJSON_True 1
#define cJSON_NULL 2
#define cJSON_Number 3
#define cJSON_String 4
#define cJSON_Array 5
#define cJSON_Object 6

#define cJSON_IsReference 256
#define cJSON_StringIsConst 512

static const AX_CHAR *ep;

const AX_CHAR *cJSON_GetErrorPtr(void)
{
    return ep;
}

static AX_S32 cJSON_strcasecmp(const AX_CHAR *s1, const AX_CHAR *s2)
{
    if (!s1)
        return (s1 == s2) ? 0 : 1;
    if (!s2)
        return 1;
    for (; tolower(*s1) == tolower(*s2); ++s1, ++s2)
        if(*s1 == 0)
            return 0;

    return tolower(*(const AX_U8 *)s1) - tolower(*(const AX_U8 *)s2);
}

static AX_VOID *(*cJSON_malloc)(size_t sz) = malloc;
static AX_VOID (*cJSON_free)(AX_VOID *ptr) = free;

/* Internal constructor. */
static cJSON *cJSON_New_Item(AX_VOID)
{
    cJSON* node = (cJSON*)cJSON_malloc(sizeof(cJSON));
    if (node)
        memset(node, 0, sizeof(cJSON));
    return node;
}

/******************************************************************************
* function : Delete a cJSON structure.
******************************************************************************/
AX_VOID cJSON_Delete(cJSON *c)
{
    cJSON *next;
    while (c) {
        next=c->next;
        if (!(c->type & cJSON_IsReference) && c->child)
            cJSON_Delete(c->child);
        if (!(c->type & cJSON_IsReference) && c->valuestring)
            cJSON_free(c->valuestring);
        if (!(c->type & cJSON_StringIsConst) && c->string)
            cJSON_free(c->string);
        cJSON_free(c);
        c = next;
    }
}

/* Parse the input text to generate a number, and populate the result into item. */
static const AX_CHAR *parse_number(cJSON *item, const AX_CHAR *num)
{
    AX_F64 n = 0, sign = 1, scale = 0;
    AX_S32 subscale = 0, signsubscale = 1;

    if (*num == '-')/* Has sign? */
        sign=-1, num++;
    if (*num == '0')/* is zero */
        num++;
    if (*num >= '1' && *num <= '9')/* Number? */
        do n = (n * 10.0) + (*num++ - '0');
        while (*num >= '0' && *num <= '9');
    if (*num == '.' && num[1] >= '0' && num[1] <= '9') {/* Fractional part? */
        num++;
        do n = (n * 10.0) + (*num++ - '0') , scale--;
        while (*num >= '0' && *num <= '9');
    }
    if (*num == 'e' || *num == 'E') {/* Exponent? */
        num++;
        if (*num == '+')/* With sign? */
            num++;
        else if (*num == '-')
            signsubscale = -1, num++;
        while (*num >= '0' && *num <= '9')/* Number? */
            subscale = (subscale * 10) + (*num++ - '0');
    }

    n = sign * n * pow(10.0, (scale + subscale * signsubscale));/* number = +/- number.fraction * 10^+/- exponent */

    item->valuedouble = n;
    item->valueint = (AX_S32)n;
    item->type = cJSON_Number;
    return num;
}

static AX_U32 parse_hex4(const AX_CHAR *str)
{
    AX_U32 h = 0;
    if (*str >= '0' && *str <= '9')
        h += (*str) - '0';
    else if (*str >= 'A' && *str <= 'F')
        h += 10 + (*str) - 'A';
    else if (*str >= 'a' && *str <= 'f')
        h += 10 + (*str) - 'a';
    else
        return 0;
    h = h << 4;
    str++;
    if (*str >= '0' && *str <= '9')
        h += (*str) - '0';
    else if (*str >= 'A' && *str <= 'F')
        h += 10 + (*str) - 'A';
    else if (*str >= 'a' && *str <= 'f')
        h += 10 + (*str) - 'a';
    else
        return 0;
    h = h << 4;
    str++;
    if (*str >= '0' && *str <= '9')
        h += (*str) - '0';
    else if (*str >= 'A' && *str <= 'F')
        h += 10 + (*str) - 'A';
    else if (*str >= 'a' && *str <= 'f')
        h += 10 + (*str) - 'a';
    else
        return 0;
    h = h << 4;
    str++;
    if (*str >= '0' && *str <= '9')
        h += (*str) - '0';
    else if (*str >= 'A' && *str <= 'F')
        h += 10 + (*str) - 'A';
    else if (*str >= 'a' && *str <= 'f')
        h += 10 + (*str) - 'a';
    else
        return 0;

    return h;
}

/* Parse the input text into an unescaped cstring, and populate item. */
static const AX_U8 firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
static const AX_CHAR *parse_string(cJSON *item, const AX_CHAR *str)
{
    const AX_CHAR *ptr = str + 1;
    AX_CHAR *ptr2;
    AX_CHAR *out;
    AX_S32 len = 0;
    AX_U32 uc, uc2;
    if (*str != '\"') {/* not a string! */
        ep = str;
        SAMPLE_IVE_PRT("Not a string!\n");
        return 0;
    }

    while (*ptr != '\"' && *ptr && ++len)/* Skip escaped quotes. */
        if (*ptr++ == '\\')
            ptr++;

    out = (AX_CHAR*)cJSON_malloc(len+1);/* This is how long we need for the string, roughly. */
    if (!out) {
        SAMPLE_IVE_PRT("Malloc fail!\n");
        return 0;
    }

    ptr = str+1;
    ptr2 = out;
    while (*ptr != '\"' && *ptr) {
        if (*ptr != '\\')
            *ptr2++ = *ptr++;
        else {
            ptr++;
            switch (*ptr) {
            case 'b':
                *ptr2++='\b';
                break;
            case 'f':
                *ptr2++='\f';
                break;
            case 'n':
                *ptr2++='\n';
                break;
            case 'r':
                *ptr2++='\r';
                break;
            case 't':
                *ptr2++='\t';
                break;
            case 'u':/* transcode utf16 to utf8. */
                uc = parse_hex4(ptr + 1);/* get the unicode char. */
                ptr += 4;

                if ((uc >= 0xDC00 && uc <= 0xDFFF) || uc == 0)/* check for invalid. */
                    break;

                if (uc >= 0xD800 && uc <= 0xDBFF) {/* UTF16 surrogate pairs. */
                    if (ptr[1] != '\\' || ptr[2] != 'u')/* missing second-half of surrogate. */
                        break;
                    uc2 = parse_hex4(ptr + 3);
                    ptr += 6;
                    if (uc2 < 0xDC00 || uc2 > 0xDFFF)/* invalid second-half of surrogate. */
                        break;
                    uc = 0x10000 + (((uc & 0x3FF) << 10) | (uc2 & 0x3FF));
                }

                len=4;
                if (uc < 0x80)
                    len = 1;
                else if (uc < 0x800)
                    len = 2;
                else if (uc < 0x10000)
                    len=3;
                ptr2 += len;

                switch (len) {
                case 4: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
                case 3: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
                case 2: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
                case 1: *--ptr2 =(uc | firstByteMark[len]);
                }
                ptr2 += len;
                break;
            default:
                *ptr2++ = *ptr;
                break;
            }
            ptr++;
        }
    }
    *ptr2 = 0;
    if (*ptr == '\"')
        ptr++;
    item->valuestring = out;
    item->type = cJSON_String;
    return ptr;
}

/* Predeclare these prototypes. */
static const AX_CHAR *parse_value(cJSON *item, const AX_CHAR *value);
static const AX_CHAR *parse_array(cJSON *item, const AX_CHAR *value);
static const AX_CHAR *parse_object(cJSON *item, const AX_CHAR *value);

/* Utility to jump whitespace and cr/lf */
static const AX_CHAR *skip(const AX_CHAR *in)
{
    while (in && *in && (AX_U8)*in <= 32)
        in++;
    return in;
}

/* Parse an object - create a new root, and populate. */
cJSON *cJSON_ParseWithOpts(const AX_CHAR *value, const AX_CHAR **return_parse_end, AX_S32 require_null_terminated)
{
    const AX_CHAR *end = 0;
    cJSON *c = cJSON_New_Item();
    ep = 0;
    if (!c) {/* memory fail */
        SAMPLE_IVE_PRT("Memory fail!\n");
        return 0;
    }

    end = parse_value(c, skip(value));
    if (!end) {/* parse failure. ep is set. */
        SAMPLE_IVE_PRT("Parse failure. ep is set!\n");
        cJSON_Delete(c);
        return 0;
    }

    /* if we require null-terminated JSON without appended garbage, skip and then check for a null terminator */
    if (require_null_terminated) {
        end = skip(end);
        if (*end) {
            cJSON_Delete(c);
            ep = end;
            return 0;
        }
    }
    if (return_parse_end)
        *return_parse_end = end;
    return c;
}

/******************************************************************************
* function :  Default options for cJSON_Parse.
******************************************************************************/
cJSON *cJSON_Parse(const AX_CHAR *value)
{
    return cJSON_ParseWithOpts(value, 0, 0);
}

/* Parser core - when encountering text, process appropriately. */
static const AX_CHAR *parse_value(cJSON *item, const AX_CHAR *value)
{
    if (!value) {/* Fail on null. */
        SAMPLE_IVE_PRT("Fail on null!\n");
        return 0;
    }
    if (!strncmp(value, "null", 4)) {
        item->type = cJSON_NULL;
        SAMPLE_IVE_PRT("Value is null!\n");
        return value + 4;
    }
    if (!strncmp(value, "false", 5)) {
        item->type = cJSON_False;
        SAMPLE_IVE_PRT("Value is false!\n");
        return value + 5;
    }
    if (!strncmp(value, "true", 4)) {
        item->type = cJSON_True;
        item->valueint = 1;
        SAMPLE_IVE_PRT("Value is true!\n");
        return value + 4;
    }
    if (*value == '\"')
        return parse_string(item, value);
    if (*value == '-' || (*value >= '0' && *value <= '9'))
        return parse_number(item, value);
    if (*value == '[')
        return parse_array(item, value);
    if (*value == '{')
        return parse_object(item, value);

    ep = value;
    return 0;/* failure. */
}

/* Build an array from input text. */
static const AX_CHAR *parse_array(cJSON *item, const AX_CHAR *value)
{
    cJSON *child;
    if (*value != '[') {/* not an array! */
        ep = value;
        SAMPLE_IVE_PRT("Not a array!\n");
        return 0;
    }

    item->type = cJSON_Array;
    value = skip(value + 1);
    if (*value == ']')/* empty array. */
        return value + 1;

    item->child = child = cJSON_New_Item();
    if (!item->child) {/* memory fail */
        SAMPLE_IVE_PRT("Memory fail!\n");
        return 0;
    }
    value = skip(parse_value(child, skip(value)));/* skip any spacing, get the value. */
    if (!value) {
        SAMPLE_IVE_PRT("Get value fail!\n");
        return 0;
    }

    while (*value == ',') {
        cJSON *new_item;
        if (!(new_item = cJSON_New_Item())) {/* memory fail */
            SAMPLE_IVE_PRT("Memory fail!\n");
            return 0;
        }
        child->next = new_item;
        new_item->prev = child;
        child = new_item;
        value = skip(parse_value(child, skip(value + 1)));
        if (!value) {/* memory fail */
            SAMPLE_IVE_PRT("Skip fail!\n");
            return 0;
        }
    }

    if (*value == ']')/* end of array */
        return value + 1;
    ep = value;
    return 0;/* malformed. */
}

/* Build an object from the text. */
static const AX_CHAR *parse_object(cJSON *item, const AX_CHAR *value)
{
    cJSON *child;
    if (*value != '{') {/* not an object! */
        ep = value;
        SAMPLE_IVE_PRT("Not an object!\n");
        return 0;
    }

    item->type = cJSON_Object;
    value = skip(value + 1);
    if (*value == '}')/* empty array. */
        return value + 1;

    item->child = child = cJSON_New_Item();
    if (!item->child) {
        SAMPLE_IVE_PRT("New item fail!\n");
        return 0;
    }
    value = skip(parse_string(child, skip(value)));
    if (!value) {
        SAMPLE_IVE_PRT("Skip fail!\n");
        return 0;
    }
    child->string = child->valuestring;
    child->valuestring = 0;
    if (*value != ':') {/* fail! */
        ep = value;
        SAMPLE_IVE_PRT("Not ':' !\n");
        return 0;
    }
    value = skip(parse_value(child, skip(value + 1)));/* skip any spacing, get the value. */
    if (!value) {
        SAMPLE_IVE_PRT("Skip fail!\n");
        return 0;
    }

    while (*value == ',') {
        cJSON *new_item;
        if (!(new_item = cJSON_New_Item())) {/* memory fail */
            SAMPLE_IVE_PRT("New item fail!\n");
            return 0;
        }
        child->next = new_item;
        new_item->prev = child;
        child = new_item;
        value = skip(parse_string(child, skip(value + 1)));
        if (!value) {
            SAMPLE_IVE_PRT("Skip fail!\n");
            return 0;
        }
        child->string = child->valuestring;
        child->valuestring = 0;
        if (*value != ':') {/* fail! */
            ep=value;
             SAMPLE_IVE_PRT("Not ':' !\n");
            return 0;
        }
        value = skip(parse_value(child, skip(value + 1)));/* skip any spacing, get the value. */
        if (!value) {
            SAMPLE_IVE_PRT("Skip fail!\n");
            return 0;
        }
    }

    if (*value == '}')/* end of array */
        return value + 1;
    ep = value;
    return 0;/* malformed. */
}

/******************************************************************************
* function : Get Array size/item / object item.
******************************************************************************/
AX_S32 cJSON_GetArraySize(cJSON *array)
{
    cJSON *c = array->child;
    AX_S32 i = 0;
    while(c)
        i++, c = c->next;
    return i;
}

cJSON *cJSON_GetArrayItem(cJSON *array, AX_S32 item)
{
    cJSON *c = array->child;
    while (c && item > 0)
        item--, c = c->next;
    return c;
}

cJSON *cJSON_GetObjectItem(cJSON *object, const AX_CHAR *string)
{
    cJSON *c = object->child;
    while (c && cJSON_strcasecmp(c->string, string))
        c = c->next;
    return c;
}
/******************************************************************************/
AX_BOOL SAMPLE_COMM_IVE_CheckDir(AX_CHAR *path)
{
    struct stat info;
    if (stat(path, &info) != 0 || !(info.st_mode & S_IFDIR))
        return AX_FALSE;
    else
        return AX_TRUE;
}

AX_U64 SAMPLE_COMM_IVE_GetTime_US(AX_VOID)
{
    AX_U64 us;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    us = tv.tv_sec * 1000000 + tv.tv_usec;

    return us;
}

AX_U16 SAMPLE_COMM_IVE_CalcStride(AX_U32 u32Width, AX_U8 u8Align)
{
    return (u32Width + (u8Align - u32Width % u8Align) % u8Align);
}

/******************************************************************************
* function : Create ive image
******************************************************************************/
AX_S32 SAMPLE_COMM_IVE_CreateImage(AX_IVE_IMAGE_T* pstImg, AX_IVE_IMAGE_TYPE_E enType, AX_U32 u32Width, AX_U32 u32Height)
{
    AX_U32 u32Size = 0;
    AX_S32 s32Ret;
    if (NULL == pstImg) {
        SAMPLE_IVE_PRT("pstImg is null\n");
        return AX_FAILURE;
    }

    pstImg->enType = enType;
    pstImg->u32Width = (g_bAlignNeed == AX_TRUE) ? ALIGN_UP(u32Width, 2) : u32Width;
    pstImg->u32Height = u32Height;
    pstImg->au32Stride[0] = (g_bAlignNeed == AX_TRUE) ? SAMPLE_COMM_IVE_CalcStride(pstImg->u32Width, AX_IVE_ALIGN) : pstImg->u32Width;
    switch (enType) {
    case AX_IVE_IMAGE_TYPE_U8C1:
    case AX_IVE_IMAGE_TYPE_S8C1:
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height;
        s32Ret = AX_SYS_MemAlloc(&pstImg->au64PhyAddr[0], (AX_VOID**)&pstImg->au64VirAddr[0], u32Size, AX_IVE_ALIGN, (AX_S8 *)AX_IVE_CMM_TOKEN);
        if (s32Ret != AX_SUCCESS) {
            SAMPLE_IVE_PRT("Cmm Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
        break;
    case AX_IVE_IMAGE_TYPE_YUV420SP:
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height * 3 / 2;
        s32Ret = AX_SYS_MemAlloc(&pstImg->au64PhyAddr[0], (AX_VOID**)&pstImg->au64VirAddr[0], u32Size, AX_IVE_ALIGN, (AX_S8 *)AX_IVE_CMM_TOKEN);
        if (s32Ret != AX_SUCCESS) {
            SAMPLE_IVE_PRT("Cmm Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
        pstImg->au32Stride[1] = pstImg->au32Stride[0];
        pstImg->au64PhyAddr[1] = pstImg->au64PhyAddr[0] + pstImg->au32Stride[0] * pstImg->u32Height;
        pstImg->au64VirAddr[1] = pstImg->au64VirAddr[0] + pstImg->au32Stride[0] * pstImg->u32Height;
        break;
    case AX_IVE_IMAGE_TYPE_YUV422SP:
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height * 2;
        s32Ret = AX_SYS_MemAlloc(&pstImg->au64PhyAddr[0], (AX_VOID**)&pstImg->au64VirAddr[0], u32Size, AX_IVE_ALIGN, (AX_S8 *)AX_IVE_CMM_TOKEN);
        if (s32Ret != AX_SUCCESS) {
            SAMPLE_IVE_PRT("Cmm Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
        pstImg->au32Stride[1] = pstImg->au32Stride[0];
        pstImg->au64PhyAddr[1] = pstImg->au64PhyAddr[0] + pstImg->au32Stride[0] * pstImg->u32Height;
        pstImg->au64VirAddr[1] = pstImg->au64VirAddr[0] + pstImg->au32Stride[0] * pstImg->u32Height;
        break;
    case AX_IVE_IMAGE_TYPE_YUV420P:
        break;
    case AX_IVE_IMAGE_TYPE_YUV422P:
        break;
    case AX_IVE_IMAGE_TYPE_S8C2_PACKAGE:
        break;
    case AX_IVE_IMAGE_TYPE_S8C2_PLANAR:
        break;
    case AX_IVE_IMAGE_TYPE_S16C1:
    case AX_IVE_IMAGE_TYPE_U16C1:
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height * sizeof(AX_U16);
        s32Ret = AX_SYS_MemAlloc(&pstImg->au64PhyAddr[0], (AX_VOID**)&pstImg->au64VirAddr[0], u32Size, AX_IVE_ALIGN, (AX_S8 *)AX_IVE_CMM_TOKEN);
        if (s32Ret != AX_SUCCESS) {
            SAMPLE_IVE_PRT("Cmm Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
        break;
    case AX_IVE_IMAGE_TYPE_U8C3_PACKAGE:
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height * 3;
        s32Ret = AX_SYS_MemAlloc(&pstImg->au64PhyAddr[0], (AX_VOID**)&pstImg->au64VirAddr[0], u32Size, AX_IVE_ALIGN, (AX_S8 *)AX_IVE_CMM_TOKEN);
        if (s32Ret != AX_SUCCESS) {
            SAMPLE_IVE_PRT("Cmm Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
        break;
    case AX_IVE_IMAGE_TYPE_U8C3_PLANAR:
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height * 3;
        s32Ret = AX_SYS_MemAlloc(&pstImg->au64PhyAddr[0], (AX_VOID**)&pstImg->au64VirAddr[0], u32Size, AX_IVE_ALIGN, (AX_S8 *)AX_IVE_CMM_TOKEN);
        if (s32Ret != AX_SUCCESS) {
            SAMPLE_IVE_PRT("Cmm Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
        pstImg->au64VirAddr[1] = pstImg->au64VirAddr[0] + pstImg->au32Stride[0] * pstImg->u32Height;
        pstImg->au64VirAddr[2] = pstImg->au64VirAddr[1] + pstImg->au32Stride[0] * pstImg->u32Height;
        pstImg->au64PhyAddr[1] = pstImg->au64PhyAddr[0] + pstImg->au32Stride[0] * pstImg->u32Height;
        pstImg->au64PhyAddr[2] = pstImg->au64PhyAddr[1] + pstImg->au32Stride[0] * pstImg->u32Height;
        pstImg->au32Stride[1] = pstImg->au32Stride[0];
        pstImg->au32Stride[2] = pstImg->au32Stride[0];
        break;
    case AX_IVE_IMAGE_TYPE_S32C1:
    case AX_IVE_IMAGE_TYPE_U32C1:
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height * sizeof(AX_U32);
        s32Ret = AX_SYS_MemAlloc(&pstImg->au64PhyAddr[0], (AX_VOID**)&pstImg->au64VirAddr[0], u32Size, AX_IVE_ALIGN, (AX_S8 *)AX_IVE_CMM_TOKEN);
        if (s32Ret != AX_SUCCESS) {
            SAMPLE_IVE_PRT("Cmm Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
        break;
    case AX_IVE_IMAGE_TYPE_S64C1:
    case AX_IVE_IMAGE_TYPE_U64C1:
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height * sizeof(AX_U64);
        s32Ret = AX_SYS_MemAlloc(&pstImg->au64PhyAddr[0], (AX_VOID**)&pstImg->au64VirAddr[0], u32Size, AX_IVE_ALIGN, (AX_S8 *)AX_IVE_CMM_TOKEN);
        if (s32Ret != AX_SUCCESS) {
            SAMPLE_IVE_PRT("Cmm Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
        break;
    default:
        break;

    }

    return AX_SUCCESS;
}

/******************************************************************************
* function : Create memory info
******************************************************************************/
AX_S32 SAMPLE_COMM_IVE_CreateMemInfo(AX_IVE_MEM_INFO_T* pstMemInfo, AX_U32 u32Size)
{
    AX_S32 s32Ret;
    if (NULL == pstMemInfo) {
        SAMPLE_IVE_PRT("pstMemInfo is null\n");
        return AX_FAILURE;
    }
    pstMemInfo->u32Size = u32Size;
    s32Ret = AX_SYS_MemAlloc(&pstMemInfo->u64PhyAddr, (AX_VOID**)&pstMemInfo->u64VirAddr, u32Size, AX_IVE_ALIGN, (AX_S8 *)AX_IVE_CMM_TOKEN);
    if (s32Ret != AX_SUCCESS) {
        SAMPLE_IVE_PRT("Cmm Alloc fail,Error(%#x)\n", s32Ret);
        return AX_FAILURE;
    }

    return AX_SUCCESS;
}

AX_S32 SAMPLE_COMM_IVE_ReadFile(AX_IVE_IMAGE_T* pstImg, FILE* pFp)
{
    AX_U16 y;
    AX_U8* pU8;
    AX_U16 height;
    AX_U16 width;
    AX_U16 loop;
    AX_S32 s32Ret;

    (AX_VOID)fgetc(pFp);
    if (feof(pFp)) {
        SAMPLE_IVE_PRT("end of file!\n");
        s32Ret = fseek(pFp, 0 , SEEK_SET );
        if (0 != s32Ret) {
            SAMPLE_IVE_PRT("fseek failed!\n");
            return s32Ret;
        }
    } else {
        s32Ret = fseek(pFp, -1 , SEEK_CUR );
        if (0 != s32Ret) {
            SAMPLE_IVE_PRT("fseek failed!\n");
            return s32Ret;
        }
    }

    height = pstImg->u32Height;
    width = pstImg->u32Width;

    switch (pstImg->enType) {
    case AX_IVE_IMAGE_TYPE_U8C1:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++) {
            if ( 1 != fread(pU8, width, 1, pFp)) {
                SAMPLE_IVE_PRT("Read file fail, please check image data!\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0];
        }
        break;
    case  AX_IVE_IMAGE_TYPE_YUV420SP:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++) {
            if ( 1 != fread(pU8, width, 1, pFp)) {
                SAMPLE_IVE_PRT("Read file fail, please check image data!\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0];
        }

        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[1];
        for (y = 0; y < height / 2; y++) {
            if ( 1 != fread(pU8, width, 1, pFp)) {
                SAMPLE_IVE_PRT("Read file fail, please check image data!\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[1];
        }
        break;
    case AX_IVE_IMAGE_TYPE_YUV422SP:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++) {
            if ( 1 != fread(pU8, width, 1, pFp)) {
                SAMPLE_IVE_PRT("Read file fail, please check image data!\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0];
        }

        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[1];
        for (y = 0; y < height; y++) {
            if ( 1 != fread(pU8, width, 1, pFp)) {
                SAMPLE_IVE_PRT("Read file fail, please check image data!\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[1];
        }
        break;
    case AX_IVE_IMAGE_TYPE_U8C3_PACKAGE:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++) {
            if ( 1 != fread(pU8, width * 3, 1, pFp)) {
                SAMPLE_IVE_PRT("Read file fail, please check image data!\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0] * 3;
        }
        break;
    case AX_IVE_IMAGE_TYPE_U8C3_PLANAR:
        for (loop = 0; loop < 3; loop++) {
            pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[loop];
            for (y = 0; y < height; y++) {
                if ( 1 != fread(pU8, width, 1, pFp)) {
                    SAMPLE_IVE_PRT("Read file fail, please check image data!\n");
                    return AX_FAILURE;
                }
                pU8 += pstImg->au32Stride[loop];
            }
        }
        break;
    case AX_IVE_IMAGE_TYPE_S16C1:
    case AX_IVE_IMAGE_TYPE_U16C1:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for ( y = 0; y < height; y++ ) {
            if ( sizeof(AX_U16) != fread(pU8, width, sizeof(AX_U16), pFp)) {
                SAMPLE_IVE_PRT("Read file fail, please check image data!\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0] * 2;
        }
        break;
    default:
        break;
    }

    return AX_SUCCESS;
}

AX_S32 SAMPLE_COMM_IVE_WriteFile(AX_IVE_IMAGE_T* pstImg, FILE* pFp)
{
    AX_U16 y;
    AX_U8* pU8;
    AX_U16 height;
    AX_U16 width;

    height = pstImg->u32Height;
    width = pstImg->u32Width;

    switch (pstImg->enType) {
    case AX_IVE_IMAGE_TYPE_U8C1:
    case AX_IVE_IMAGE_TYPE_S8C1:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++) {
            if ( 1 != fwrite(pU8, width, 1, pFp)) {
                SAMPLE_IVE_PRT("Write file fail\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0];
        }
        break;
    case AX_IVE_IMAGE_TYPE_YUV420SP:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++) {
            if ( width != fwrite(pU8, 1, width, pFp)) {
                SAMPLE_IVE_PRT("Write file fail\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0];
        }

        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[1];
        for (y = 0; y < height / 2; y++) {
            if ( width != fwrite(pU8, 1, width, pFp)) {
                SAMPLE_IVE_PRT("Write file fail\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[1];
        }
        break;
    case AX_IVE_IMAGE_TYPE_YUV422SP:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++) {
            if ( width != fwrite(pU8, 1, width, pFp)) {
                SAMPLE_IVE_PRT("Write file fail\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0];
        }

        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[1];
        for (y = 0; y < height; y++) {
            if ( width != fwrite(pU8, 1, width, pFp)) {
                SAMPLE_IVE_PRT("Write file fail\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[1];
        }
        break;
    case AX_IVE_IMAGE_TYPE_S16C1:
    case AX_IVE_IMAGE_TYPE_U16C1:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for ( y = 0; y < height; y++ ) {
            if ( sizeof(AX_U16) != fwrite(pU8, width, sizeof(AX_U16), pFp)) {
                SAMPLE_IVE_PRT("Write file fail\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0] * 2;
        }
        break;
    case AX_IVE_IMAGE_TYPE_U32C1:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for ( y = 0; y < height; y++ ) {
            if ( width != fwrite(pU8, sizeof(AX_U32), width, pFp)) {
                SAMPLE_IVE_PRT("Write file fail\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0] * 4;
        }
        break;
    case AX_IVE_IMAGE_TYPE_U64C1:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for ( y = 0; y < height; y++ ) {
            if ( width != fwrite(pU8, sizeof(AX_U64), width, pFp)) {
                SAMPLE_IVE_PRT("Write file fail\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0] * 8;
        }
        break;
    case AX_IVE_IMAGE_TYPE_U8C3_PACKAGE:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++) {
            if ( 1 != fwrite(pU8, width * 3, 1, pFp)) {
                SAMPLE_IVE_PRT("Write file fail\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0] * 3;
        }
        break;
    case AX_IVE_IMAGE_TYPE_U8C3_PLANAR:
        for (AX_S32 loop = 0; loop < 3; loop++) {
            pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[loop];
            for (y = 0; y < height; y++) {
                if ( 1 != fwrite(pU8, width, 1, pFp)) {
                    SAMPLE_IVE_PRT("Write file fail\n");
                    return AX_FAILURE;
                }
                pU8 += pstImg->au32Stride[loop];
            }
        }
        break;

    default:
        break;
    }

    return AX_SUCCESS;
}

AX_S32 SAMPLE_COMM_IVE_ReadMemInfoFile(AX_IVE_SRC_MEM_INFO_T* pstMemInfo, FILE* pFp)
{
    AX_U8* pU8;
    AX_U32 size;
    AX_S32 s32Ret;
    size = pstMemInfo->u32Size;
    (AX_VOID)fgetc(pFp);
    if (feof(pFp)) {
        SAMPLE_IVE_PRT("end of file!\n");
        s32Ret = fseek(pFp, 0 , SEEK_SET );
        if (0 != s32Ret) {
            SAMPLE_IVE_PRT("fseek failed!\n");
            return s32Ret;
        }
    } else {
        s32Ret = fseek(pFp, -1 , SEEK_CUR );
        if (0 != s32Ret) {
            SAMPLE_IVE_PRT("fseek failed!\n");
            return s32Ret;
        }
    }

    pU8 = (AX_U8 *)(AX_UL)pstMemInfo->u64VirAddr;
    if ( sizeof(AX_U8) != fread(pU8,  size, sizeof(AX_U8), pFp)) {
        SAMPLE_IVE_PRT("Read file fail\n");
        return AX_FAILURE;
    }
    return AX_SUCCESS;
}

AX_S32 SAMPLE_COMM_IVE_WriteMemInfoFile(AX_IVE_DST_MEM_INFO_T* pstMemInfo, FILE* pFp)
{
    AX_U8* pU8;
    AX_U32 size = pstMemInfo->u32Size;
    pU8 = (AX_U8 *)(AX_UL)pstMemInfo->u64VirAddr;
    if ( size != fwrite(pU8, sizeof(AX_U8), size, pFp)) {
        SAMPLE_IVE_PRT("Write file fail\n");
        return AX_FAILURE;
    }
    return AX_SUCCESS;
}

/******************************************************************************
* function : Create image/Read file/Write file with globel image format.
******************************************************************************/
AX_S32 SAMPLE_COMM_IVE_CreateImage_WithGlbImgFmt(AX_IVE_IMAGE_T* pstImg, AX_IMG_FORMAT_E enType, AX_U32 u32Width, AX_U32 u32Height)
{
    AX_U32 u32Size = 0;
    AX_S32 s32Ret;
    if (NULL == pstImg) {
        SAMPLE_IVE_PRT("pstImg is null\n");
        return AX_FAILURE;
    }

    pstImg->enGlbType = enType;
    pstImg->u32Width = (g_bAlignNeed == AX_TRUE) ? ALIGN_UP(u32Width, 2) : u32Width;
    pstImg->u32Height = (g_bAlignNeed == AX_TRUE) ? ALIGN_UP(u32Height, 2) : u32Height;
    pstImg->au32Stride[0] = (g_bAlignNeed == AX_TRUE) ? SAMPLE_COMM_IVE_CalcStride(pstImg->u32Width, AX_IVE_ALIGN) : pstImg->u32Width;

    switch (enType) {
    case AX_FORMAT_YUV400:
    case AX_FORMAT_BITMAP:
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height;
        s32Ret = AX_SYS_MemAlloc(&pstImg->au64PhyAddr[0], (AX_VOID**)&pstImg->au64VirAddr[0], u32Size, AX_IVE_ALIGN, (AX_S8 *)AX_IVE_CMM_TOKEN);
        if (s32Ret != AX_SUCCESS) {
            SAMPLE_IVE_PRT("Cmm Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
        break;
    case AX_FORMAT_YUV420_SEMIPLANAR:
    case AX_FORMAT_YUV420_SEMIPLANAR_VU:
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height * 3 / 2;
        s32Ret = AX_SYS_MemAlloc(&pstImg->au64PhyAddr[0], (AX_VOID**)&pstImg->au64VirAddr[0], u32Size, AX_IVE_ALIGN, (AX_S8 *)AX_IVE_CMM_TOKEN);
        if (s32Ret != AX_SUCCESS) {
            SAMPLE_IVE_PRT("Cmm Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
        pstImg->au32Stride[1] = pstImg->au32Stride[0];
        pstImg->au64PhyAddr[1] = pstImg->au64PhyAddr[0] + pstImg->au32Stride[0] * pstImg->u32Height;
        pstImg->au64VirAddr[1] = pstImg->au64VirAddr[0] + pstImg->au32Stride[0] * pstImg->u32Height;
        break;
    case AX_FORMAT_YUV422_SEMIPLANAR:
    case AX_FORMAT_YUV422_SEMIPLANAR_VU:
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height * 2;
        s32Ret = AX_SYS_MemAlloc(&pstImg->au64PhyAddr[0], (AX_VOID**)&pstImg->au64VirAddr[0], u32Size, AX_IVE_ALIGN, (AX_S8 *)AX_IVE_CMM_TOKEN);
        if (s32Ret != AX_SUCCESS) {
            SAMPLE_IVE_PRT("Cmm Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
        pstImg->au32Stride[1] = pstImg->au32Stride[0];
        pstImg->au64PhyAddr[1] = pstImg->au64PhyAddr[0] + pstImg->au32Stride[0] * pstImg->u32Height;
        pstImg->au64VirAddr[1] = pstImg->au64VirAddr[0] + pstImg->au32Stride[0] * pstImg->u32Height;
        break;
    case AX_FORMAT_RGB565:
    case AX_FORMAT_BGR565:
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height * sizeof(AX_U16);
        s32Ret = AX_SYS_MemAlloc(&pstImg->au64PhyAddr[0], (AX_VOID**)&pstImg->au64VirAddr[0], u32Size, AX_IVE_ALIGN, (AX_S8 *)AX_IVE_CMM_TOKEN);
        if (s32Ret != AX_SUCCESS) {
            SAMPLE_IVE_PRT("Cmm Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
        break;
    case AX_FORMAT_RGB888:
    case AX_FORMAT_BGR888:
    case AX_FORMAT_YUV444_PACKED:
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height * 3;
        s32Ret = AX_SYS_MemAlloc(&pstImg->au64PhyAddr[0], (AX_VOID**)&pstImg->au64VirAddr[0], u32Size, AX_IVE_ALIGN, (AX_S8 *)AX_IVE_CMM_TOKEN);
        if (s32Ret != AX_SUCCESS) {
            SAMPLE_IVE_PRT("Cmm Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
        break;
    case AX_FORMAT_ARGB8888:
    case AX_FORMAT_RGBA8888:
        u32Size = pstImg->au32Stride[0] * pstImg->u32Height * 4;
        s32Ret = AX_SYS_MemAlloc(&pstImg->au64PhyAddr[0], (AX_VOID**)&pstImg->au64VirAddr[0], u32Size, AX_IVE_ALIGN, (AX_S8 *)AX_IVE_CMM_TOKEN);
        if (s32Ret != AX_SUCCESS) {
            SAMPLE_IVE_PRT("Cmm Alloc fail,Error(%#x)\n", s32Ret);
            return s32Ret;
        }
        break;
    default:
        SAMPLE_IVE_PRT("Globel image format %d cannot create image !\n", pstImg->enGlbType);
        return AX_FAILURE;

    }

    return AX_SUCCESS;
}

AX_S32 SAMPLE_COMM_IVE_ReadFile_WithGlbImgFmt(AX_IVE_IMAGE_T* pstImg, FILE* pFp)
{
    AX_U16 y;
    AX_U8* pU8;
    AX_U16 height;
    AX_U16 width;
    AX_S32 s32Ret;

    (AX_VOID)fgetc(pFp);
    if (feof(pFp)) {
        SAMPLE_IVE_PRT("end of file!\n");
        s32Ret = fseek(pFp, 0 , SEEK_SET );
        if (0 != s32Ret) {
            SAMPLE_IVE_PRT("fseek failed!\n");
            return s32Ret;
        }
    } else {
        s32Ret = fseek(pFp, -1 , SEEK_CUR );
        if (0 != s32Ret) {
            SAMPLE_IVE_PRT("fseek failed!\n");
            return s32Ret;
        }
    }

    height = pstImg->u32Height;
    width = pstImg->u32Width;

    switch (pstImg->enGlbType) {
    case AX_FORMAT_YUV400:
    case AX_FORMAT_BITMAP:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++) {
            if ( 1 != fread(pU8, width, 1, pFp)) {
                SAMPLE_IVE_PRT("Read file fail, please check image data!\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0];
        }
        break;
    case AX_FORMAT_YUV420_SEMIPLANAR:
    case AX_FORMAT_YUV420_SEMIPLANAR_VU:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++) {
            if ( 1 != fread(pU8, width, 1, pFp)) {
                SAMPLE_IVE_PRT("Read file fail, please check image data!\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0];
        }

        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[1];
        for (y = 0; y < height / 2; y++) {
            if ( 1 != fread(pU8, width, 1, pFp)) {
                SAMPLE_IVE_PRT("Read file fail, please check image data!\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[1];
        }
        break;
    case AX_FORMAT_YUV422_SEMIPLANAR:
    case AX_FORMAT_YUV422_SEMIPLANAR_VU:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++) {
            if ( 1 != fread(pU8, width, 1, pFp)) {
                SAMPLE_IVE_PRT("Read file fail, please check image data!\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0];
        }

        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[1];
        for (y = 0; y < height; y++) {
            if ( 1 != fread(pU8, width, 1, pFp)) {
                SAMPLE_IVE_PRT("Read file fail, please check image data!\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[1];
        }
        break;
    case AX_FORMAT_RGB565:
    case AX_FORMAT_BGR565:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for ( y = 0; y < height; y++ ) {
            if ( sizeof(AX_U16) != fread(pU8, width, sizeof(AX_U16), pFp)) {
                SAMPLE_IVE_PRT("Read file fail, please check image data!\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0] * 2;
        }
        break;
    case AX_FORMAT_RGB888:
    case AX_FORMAT_BGR888:
    case AX_FORMAT_YUV444_PACKED:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++) {
            if ( 1 != fread(pU8, width * 3, 1, pFp)) {
                SAMPLE_IVE_PRT("Read file fail, please check image data!\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0] * 3;
        }
        break;
    case AX_FORMAT_ARGB8888:
    case AX_FORMAT_RGBA8888:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for ( y = 0; y < height; y++ ) {
            if ( 1 != fread(pU8, width * 4, 1, pFp)) {
                SAMPLE_IVE_PRT("Read file fail, please check image data!\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0] * 4;
        }
        break;
    default:
        SAMPLE_IVE_PRT("Globel image format %d cannot read image file!\n", pstImg->enGlbType);
        return AX_FAILURE;

    }

    return AX_SUCCESS;
}

AX_S32 SAMPLE_COMM_IVE_WriteFile_WithGlbImgFmt(AX_IVE_IMAGE_T* pstImg, FILE* pFp)
{
    AX_U16 y;
    AX_U8* pU8;
    AX_U16 height;
    AX_U16 width;

    height = pstImg->u32Height;
    width = pstImg->u32Width;

    switch (pstImg->enGlbType) {
    case AX_FORMAT_YUV400:
    case AX_FORMAT_BITMAP:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++) {
            if ( 1 != fwrite(pU8, width, 1, pFp)) {
                SAMPLE_IVE_PRT("Write file fail\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0];
        }
        break;
    case AX_FORMAT_YUV420_SEMIPLANAR:
    case AX_FORMAT_YUV420_SEMIPLANAR_VU:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++) {
            if ( width != fwrite(pU8, 1, width, pFp)) {
                SAMPLE_IVE_PRT("Write file fail\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0];
        }

        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[1];
        for (y = 0; y < height / 2; y++) {
            if ( width != fwrite(pU8, 1, width, pFp)) {
                SAMPLE_IVE_PRT("Write file fail\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[1];
        }
        break;
    case AX_FORMAT_YUV422_SEMIPLANAR:
    case AX_FORMAT_YUV422_SEMIPLANAR_VU:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++) {
            if ( width != fwrite(pU8, 1, width, pFp)) {
                SAMPLE_IVE_PRT("Write file fail\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0];
        }

        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[1];
        for (y = 0; y < height; y++) {
            if ( width != fwrite(pU8, 1, width, pFp)) {
                SAMPLE_IVE_PRT("Write file fail\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[1];
        }
        break;
    case AX_FORMAT_RGB565:
    case AX_FORMAT_BGR565:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for ( y = 0; y < height; y++ ) {
            if ( sizeof(AX_U16) != fwrite(pU8, width, sizeof(AX_U16), pFp)) {
                SAMPLE_IVE_PRT("Write file fail\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0] * 2;
        }
        break;
    case AX_FORMAT_RGB888:
    case AX_FORMAT_BGR888:
    case AX_FORMAT_YUV444_PACKED:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++) {
            if ( 1 != fwrite(pU8, width * 3, 1, pFp)) {
                SAMPLE_IVE_PRT("Write file fail\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0] * 3;
        }

        break;
    case AX_FORMAT_ARGB8888:
    case AX_FORMAT_RGBA8888:
        pU8 = (AX_U8 *)(AX_UL)pstImg->au64VirAddr[0];
        for (y = 0; y < height; y++) {
            if ( 1 != fwrite(pU8, width * 4, 1, pFp)) {
                SAMPLE_IVE_PRT("Write file fail\n");
                return AX_FAILURE;
            }
            pU8 += pstImg->au32Stride[0] * 4;
        }
        break;
    default:
        SAMPLE_IVE_PRT("Globel image format %d cannot write image file!\n", pstImg->enGlbType);
        return AX_FAILURE;

    }

    return AX_SUCCESS;
}

AX_S32 SAMPLE_COMM_IVE_Bits_MAU_DataType(AX_IVE_MAU_DATA_TYPE_E enType)
{
    switch(enType) {
    case AX_IVE_MAU_DT_UINT8:
    case AX_IVE_MAU_DT_SINT8:
        return 8;
    case AX_IVE_MAU_DT_UINT16:
    case AX_IVE_MAU_DT_SINT16:
    case AX_IVE_MAU_DT_FLOAT16:
    case AX_IVE_MAU_DT_BFLOAT16:
        return 16;
    case AX_IVE_MAU_DT_UINT32:
    case AX_IVE_MAU_DT_SINT32:
    case AX_IVE_MAU_DT_FLOAT32:
        return 32;
    case AX_IVE_MAU_DT_UINT64:
    case AX_IVE_MAU_DT_SINT64:
        return 64;
    default:
        SAMPLE_IVE_PRT("No support mau data type:%d!\n", enType);
        return AX_FAILURE;
    }
}

static AX_S32 SAMPLE_IVE_Gen_Random_MauData(AX_VOID *pAddr, AX_IVE_MAU_DATA_TYPE_E enType, AX_U32 u32Size)
{
    AX_S32 i;
    srand((AX_U32)time(NULL));
    switch(enType) {
    case AX_IVE_MAU_DT_UINT8: {
        AX_U8 *base_data = (AX_U8*)pAddr;
        for (i = 0; i < u32Size; i++)
            *(base_data++) = (AX_U8)(rand() % (256));
        }
        break;
    case AX_IVE_MAU_DT_SINT8: {
        AX_S8 *base_data = (AX_S8*)pAddr;
        for (i = 0; i < u32Size; i++)
            *(base_data++) = (AX_S8)(rand() % 256  - 128);
        }
        break;
    case AX_IVE_MAU_DT_UINT16: {
        AX_U16 * base_data = (AX_U16*)pAddr;
        for (i = 0; i < u32Size; i++)
            *(base_data++) = RAND_U16;
        }
        break;
    case AX_IVE_MAU_DT_SINT16: {
        AX_S16 * base_data = (AX_S16*)pAddr;
        for (i = 0; i < u32Size; i++)
            *(base_data++) = RAND_S16;
        }
        break;
    case AX_IVE_MAU_DT_FLOAT16:
    case AX_IVE_MAU_DT_BFLOAT16: {
        AX_S16 * base_data = (AX_S16*)pAddr;
        for (i = 0; i < u32Size; i++)
            *(base_data++) = (AX_S16)((-1.0F + (AX_F32) (rand()) /( (AX_F32) (RAND_MAX/(2.0F))))); // [-1, 1]
        }
        break;
    case AX_IVE_MAU_DT_UINT32: {
        AX_U32 * base_data = (AX_U32*)pAddr;
        for (i = 0; i < u32Size; i++)
            *(base_data++) = ((AX_U32)((RAND_U16) << 16) | (AX_U32)(RAND_U16));
        }
        break;
    case AX_IVE_MAU_DT_SINT32: {
        AX_S32 * base_data = (AX_S32*)pAddr;
        for (i = 0; i < u32Size; i++)
            *(base_data++) = ((AX_S32)((RAND_S16) << 16) | (AX_S32)(RAND_S16));
        }
        break;
    case AX_IVE_MAU_DT_FLOAT32: {
        AX_F32 *base_data = (AX_F32 *)pAddr;
        for (i = 0; i < u32Size; ++i)
            *(base_data++) =  -1.0F + (AX_F32) (rand()) /( (AX_F32) (RAND_MAX/(2.0F))); // [-1, 1]
        }
        break;
    case AX_IVE_MAU_DT_UINT64: {
        AX_U64 * base_data = (AX_U64*)pAddr;
        for (i = 0; i < u32Size; i++)
            *(base_data++) = (((AX_U64)(RAND_U16) << 48) | ((AX_U64)(RAND_U16) << 32) | ((AX_U64)(RAND_U16) << 16) | ((AX_U64)RAND_U16));
        }
        break;
    case AX_IVE_MAU_DT_SINT64: {
        AX_S64 * base_data = (AX_S64*)pAddr;
        for (i = 0; i < u32Size; i++)
            *(base_data++) = (((AX_S64)(RAND_S16) << 48) | ((AX_S64)(RAND_S16) << 32) | ((AX_S64)(RAND_S16) << 16) | ((AX_S64)RAND_S16));
        }
        break;
    default:
        SAMPLE_IVE_PRT("No support mau data type:%d!\n", enType);
        return AX_FAILURE;
    }

    return AX_SUCCESS;
}

AX_S32 SAMPLE_COMM_IVE_CreateMauBlob(AX_IVE_MAU_BLOB_T *pstBolb, AX_IVE_MAU_DATA_TYPE_E enType, AX_S32 *pShape, AX_U8 u8Dims, AX_BOOL bRandom)
{
    if (NULL == pstBolb || NULL == pShape) {
        SAMPLE_IVE_PRT("pstBolb or pShape is null\n");
        return AX_FAILURE;
    }
    pstBolb->pShape = (AX_S32*)malloc(u8Dims * sizeof(AX_S32));
    if (!pstBolb->pShape) {
        SAMPLE_IVE_PRT("Malloc failed!\n");
        return AX_FAILURE;
    }
    pstBolb->u8ShapeSize = u8Dims;
    pstBolb->enDataType = enType;
    AX_U32 u32Size = 1;
    for (AX_S32 i = 0; i < u8Dims; i++) {
        u32Size *= pShape[i];
        pstBolb->pShape[i] = pShape[i];
    }
    AX_S32 s32Bits = SAMPLE_COMM_IVE_Bits_MAU_DataType(enType);
    if (s32Bits < 0)
        return AX_FAILURE;
    AX_S32 s32Ret = AX_SYS_MemAllocCached(&pstBolb->u64PhyAddr, &pstBolb->pVirAddr, u32Size * s32Bits / 8, 32, NULL);
    if (s32Ret != AX_SUCCESS) {
        SAMPLE_IVE_PRT("AX_SYS_MemAllocCached fail,Error(%#x)\n", s32Ret);
        return s32Ret;
    }
    if (bRandom) {
         SAMPLE_IVE_Gen_Random_MauData((AX_VOID*)pstBolb->pVirAddr, enType, u32Size);
    } else {
        memset((AX_VOID*)pstBolb->pVirAddr, 0,  u32Size * s32Bits / 8);
    }
    AX_SYS_MflushCache(pstBolb->u64PhyAddr, pstBolb->pVirAddr,  u32Size * s32Bits / 8);

    return AX_SUCCESS;
}

AX_VOID SAMPLE_COMM_IVE_FreeMauBlob(AX_IVE_MAU_BLOB_T *pstBolb)
{
    if ((0 != (pstBolb->u64PhyAddr)) && (NULL != (pstBolb->pVirAddr))) {
        AX_S32 s32Ret = AX_SYS_MemFree(pstBolb->u64PhyAddr, pstBolb->pVirAddr);
        if (s32Ret != AX_SUCCESS) {
            SAMPLE_IVE_PRT("AX_SYS_MemFree fail,Error(%#x)\n", s32Ret);
        }
    }
    if (NULL != pstBolb->pShape) {
        free(pstBolb->pShape);
        pstBolb->pShape = NULL;
    }
}

AX_VOID SAMPLE_COMM_IVE_Invalid_MauBlobCache(AX_IVE_MAU_BLOB_T *pstBolb)
{
    AX_U32 u32Size = 1;
    for (AX_S32 i = 0; i < pstBolb->u8ShapeSize; i++) {
        u32Size *= pstBolb->pShape[i];
    }
    AX_S32 s32Bits = SAMPLE_COMM_IVE_Bits_MAU_DataType(pstBolb->enDataType);
    AX_S32 s32Ret = AX_SYS_MinvalidateCache(pstBolb->u64PhyAddr, pstBolb->pVirAddr, u32Size * s32Bits / 8);
    if (s32Ret != AX_SUCCESS) {
        SAMPLE_IVE_PRT("AX_SYS_MinvalidateCache fail,Error(%#x)\n", s32Ret);
    }
}
