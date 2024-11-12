/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef __SAMPLE_DEF_H__
#define __SAMPLE_DEF_H__

#include "ax_global_type.h"

typedef AX_VOID* AX_HANDLE;

#define AX_INVALID_HANDLE (AX_NULL)
#define INVALID_VDEC_GRP (-1)
#define INVALID_VENC_CHN (-1)

#define MAX_PATH (260)

#ifndef AX_MAX
#define AX_MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef AX_MIN
#define AX_MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef ALIGN_UP
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#endif

/* VDEC stride align 256 */
#define AX_SHIFT_LEFT_ALIGN(a) (1 << (a))
#define VDEC_STRIDE_ALIGN AX_SHIFT_LEFT_ALIGN(8)

/* VENC stride align */
#define VENC_FBC_STRIDE_ALIGN_VAL (256)
#define VENC_NONE_FBC_STRIDE_ALIGN_VAL (16)

#define VDEC_PP0 (0)
#define VDEC_PP1 (1)
#define VDEC_PP2 (2)

#define MALLOC malloc
#define FREE free
#define CALLOC calloc

#endif /* __SAMPLE_DEF_H__ */