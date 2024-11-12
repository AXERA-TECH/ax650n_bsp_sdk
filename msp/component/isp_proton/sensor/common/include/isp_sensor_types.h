/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __ISP_SENSOR_TYPES_H__
#define __ISP_SENSOR_TYPES_H__

#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include "sensor_user_debug.h"

typedef enum {
    SNS_SUCCESS                           = 0x0,
    SNS_ERR_CODE_FAILED                   = 0x1,
    SNS_ERR_CODE_PTR_NULL                 = 0x2,
    SNS_ERR_CODE_INIT_FAILD,
    SNS_ERR_CODE_NOT_MEM,
    SNS_ERR_CODE_INVALID_ADDRESS,
    SNS_ERR_CODE_ILLEGAL_PARAMS,

} SNS_ERR_CODE_E;

/* macro definition: Function returns an error */
#define SNS_CHECK_PTR_VALID(h)                                                      \
do {                                                                            \
    if (AX_NULL == (h)) {                                                          \
        SNS_ERR("[ISP_CHECK_PTR] error: pointer is null!\n");              \
        return SNS_ERR_CODE_PTR_NULL;                                             \
    }                                                                           \
} while(0)

#define SNS_CHECK_VALUE_RANGE_VALID(val, min, max)                                                      \
do {                                                                                                \
    if ((val) < (min) || (val) > (max)) {                                                           \
        SNS_ERR("[ISP_CHECK_VALUE_RANGE] error: value:%d beyond the range:(%d, %d)\n",         \
                    (val), (min), (max));                                                 \
        return SNS_ERR_CODE_ILLEGAL_PARAMS;                                                            \
    }                                                                                               \
} while(0)

#ifndef EPS
    #define EPS         (1E-06)
#endif


/********************************************************************************
* AXSNS_CLIP3(x,min,max)        clip x within [min,max]
* IS_WITHIN_RANGE(x,min,max)    if x is between [min,max] return true, else false
********************************************************************************/
#define AXSNS_CLIP_MIN(x,min)           (((x) >= (min)) ? (x) : (min))
#define AXSNS_CLIP_MAX(x,max)           (((x) <= (max)) ? (x) : (max))
#define AXSNS_CLIP3(x,min,max)          ( (x)< (min) ? (min) : ((x)>(max)?(max):(x)) )
#define IS_WITHIN_RANGE(x,min,max)      (((x)>=(min)) && ((x) <= (max)))


/******************************************************************************
** AXSNS_MAX(x,y)               maximum of x and y
** AXSNS_MIN(x,y)               minimum of x and y
** AXSNS_MAX3(x,y,z)            maximum of x, y and z
** AXSNS_MIN3(x,y,z)            minimun of x, y and z
******************************************************************************/
#define AXSNS_MAX(x,y)                  ( (x)>(y) ? (x):(y) )
#define AXSNS_MIN(x,y)                  ( (x)<(y) ? (x):(y) )
#define AXSNS_MAX3(x,y,z)               ( (x)>(y) ? AXSNS_MAX(x,z) : AXSNS_MAX(y,z) )
#define AXSNS_MIN3(x,y,z)               ( (x)<(y) ? AXSNS_MIN(x,z) : AXSNS_MIN(y,z) )

#define AXSNS_ALIGN_UP(x, a)            ( ( ((x) + ((a) - 1) ) / a ) * a )
#define AXSNS_ALIGN_DOWN(x, a)          ( ( (x) / (a)) * (a) )

/* To avoid divide-0 exception in code. */
#define AXSNS_DIV_0_TO_1(a)             ( (0 == (a)) ? 1 : (a) )
#define AXSNS_DIV_0_TO_1_FLOAT(a)       ((((a) < 1E-6) && ((a) > -1E-6)) ? 1 : (a))

/********************************************************************************
* float type campare
********************************************************************************/
#define AXSNS_CAMPARE_FLOAT(a, b)           (((a - b) > (EPS)) ? (1) : (0))
#define IS_SNS_FPS_EQUAL(float_a, float_b)  ( (fabs(float_a - float_b) < EPS) ? (1) : (0) )
#define IS_FLOAT_ZERO(a)                    ( (fabs(a) < EPS) ? (1) : (0) )


#define REG_HIGH_8BITS(x)       ((x & 0xff00) >> 8)
#define REG_LOW_8BITS(x)        (x & 0x00ff)

#ifndef AX_SYS_API_PUBLIC
#define AX_SYS_API_PUBLIC __attribute__((visibility("default")))
#endif

#endif  /* __ISP_SENSOR_TYPES_H__ */

