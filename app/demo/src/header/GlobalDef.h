/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#pragma once

#ifdef AX_MEM_CHECK
#include <mcheck.h>
#define AX_MTRACE_ENTER(name) do { \
    if (!getenv("MALLOC_TRACE")) { \
        setenv("MALLOC_TRACE", ""#name"_mtrace.log", 1); \
    } \
    mtrace();\
}while(0)

#define AX_MTRACE_LEAVE do { \
    printf("please wait mtrace flush log to file...\n"); \
    sleep(30);\
}while(0)

#else
#define AX_MTRACE_ENTER(name)
#define AX_MTRACE_LEAVE
#endif

#define SDK_VERSION_PREFIX  "Ax_Version"

#ifndef AX_MAX
#define AX_MAX(a, b)        (((a) > (b)) ? (a) : (b))
#endif

#ifndef AX_MIN
#define AX_MIN(a, b)        (((a) < (b)) ? (a) : (b))
#endif

#ifndef ALIGN_UP
#define ALIGN_UP(x, align) (((x) + ((align) - 1)) & ~((align)-1))
#endif

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, align) ((x) & ~((align)-1))
#endif

#ifndef ALIGN_COMM_UP
#define ALIGN_COMM_UP(x, align) ((((x) + ((align) - 1)) / (align)) * (align))
#endif

#ifndef ALIGN_COMM_DOWN
#define ALIGN_COMM_DOWN(x, align) (((x) / (align)) * (align))
#endif

#ifndef ADAPTER_RANGE
#define ADAPTER_RANGE(v, min, max)    ((v) < (min)) ? (min) : ((v) > (max)) ? (max) : (v)
#endif

#ifndef AX_BIT_CHECK
#define AX_BIT_CHECK(v, b) (((AX_U32)(v) & (1 << (b))))
#endif

#ifndef AX_BIT_SET
#define AX_BIT_SET(v, b) ((v) |= (1 << (b)))
#endif

#ifndef AX_BIT_CLEAR
#define AX_BIT_CLEAR(v, b) ((v) &= ~(1 << (b)))
#endif

#define SAFE_DELETE_PTR(p) {if(p){delete p;p = nullptr;}}

#define NOT_USED(x) ((void*) x)

#define AX_VIN_FBC_WIDTH_ALIGN_VAL (256)
#define AX_IVPS_FBC_WIDTH_ALIGN_VAL (128)
#define AX_ENCODER_FBC_WIDTH_ALIGN_VAL (128)

#define AX_IVPS_FBC_STRIDE_ALIGN_VAL (256)
#define AX_IVPS_NONE_FBC_STRIDE_ALIGN_VAL (16)
#define AX_ENCODER_FBC_STRIDE_ALIGN_VAL (256)
#define AX_ENCODER_NONE_FBC_STRIDE_ALIGN_VAL (16)

#define AX_SHIFT_LEFT_ALIGN(a) (1 << (a))

/* VDEC stride align 256 */
#define VDEC_STRIDE_ALIGN AX_SHIFT_LEFT_ALIGN(8)

#define ADAPTER_INT2BOOLSTR(val) (val == 1 ? "true" : "false")
#define ADAPTER_INT2BOOL(val) (val == 1 ? AX_TRUE : AX_FALSE)
#define ADAPTER_BOOL2BOOLSTR(val) (val == AX_TRUE ? "true" : "false")
#define ADAPTER_BOOLSTR2INT(val) (strcmp(val, "true") == 0 ? 1 : 0)

#define AX_APP_LOCKQ_CAPACITY (3)