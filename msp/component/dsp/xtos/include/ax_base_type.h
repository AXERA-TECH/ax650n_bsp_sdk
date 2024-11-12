/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_BASE_TYPE_H_
#define _AX_BASE_TYPE_H_

#include <stdbool.h>

/* types of variables typedef */
typedef unsigned long long int  AX_U64;
typedef unsigned int            AX_U32;
typedef unsigned short          AX_U16;
typedef unsigned char           AX_U8;
typedef signed char           	AX_S8;
typedef char           		AX_CHAR;
typedef long long int           AX_S64;
typedef int                     AX_S32;
typedef short                   AX_S16;
typedef long                    AX_LONG;
typedef unsigned long           AX_ULONG;
typedef unsigned long           AX_ADDR;
typedef float                   AX_F32;
typedef double                  AX_F64;
typedef void                    AX_VOID;

#define AX_UL unsigned long
#define AX_SL signed long
#define AX_FLOAT float
#define AX_DOUBLE double


typedef enum {
    AX_FALSE = 0,
    AX_TRUE  = 1,
} AX_BOOL;

/*Mem information*/
typedef struct axSVP_MEM_INFO_S {
  AX_U64 u64PhyAddr; /* RW;The physical address of the memory */
  AX_U64 u64VirAddr; /* RW;The virtual address of the memory */
  AX_U32 u32Size;    /* RW;The size of memory */
} AX_MEM_INFO_T;

#endif //_AX_BASE_TYPE_H_
