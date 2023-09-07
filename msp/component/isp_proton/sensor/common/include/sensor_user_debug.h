/**********************************************************************************
 *
 * Copyright (c) 2019-2020 Beijing AXera Technology Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Beijing AXera Technology Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Beijing AXera Technology Co., Ltd.
 *
 **********************************************************************************/

#ifndef _SENSOR_USER_DEBUG_H_
#define _SENSOR_USER_DEBUG_H_

#include <stdio.h>

#include "ax_base_type.h"
#include "ax_global_type.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef SENSOR_LOG_USE_PRINTF
#define SNS_ERR(fmt, ...)       printf("[E][%32s][%4d]: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define SNS_WRN(fmt, ...)       printf("[W][%32s][%4d]: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define SNS_INFO(fmt, ...)      printf("[I][%32s][%4d]: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define SNS_DBG(fmt, ...)       printf("[D][%32s][%4d]: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define SNS_ERR(fmt, ...)       sns_printf(SYS_LOG_ERROR, "[E][%32s][%4d]: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define SNS_WRN(fmt, ...)       sns_printf(SYS_LOG_WARN,  "[W][%32s][%4d]: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define SNS_INFO(fmt, ...)      sns_printf(SYS_LOG_INFO,  "[I][%32s][%4d]: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define SNS_DBG(fmt, ...)       sns_printf(SYS_LOG_DEBUG, "[D][%32s][%4d]: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

AX_S32 sns_printf(AX_U32 level, AX_CHAR *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif //_SENSOR_USER_DEBUG_H_