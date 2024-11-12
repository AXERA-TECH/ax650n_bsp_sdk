/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

//#pragma once

#define AX_DBG_FATAL 7  /* system is unusable                   */
#define AX_DBG_ALERT 6  /* action must be taken immediately     */
#define AX_DBG_CRIT 5   /* critical conditions                  */
#define AX_DBG_ERR 4    /* error conditions                     */
#define AX_DBG_WARN 3   /* warning conditions                   */
#define AX_DBG_NOTICE 2 /* normal but significant condition     */
#define AX_DBG_INFO 1   /* informational                        */
#define AX_DBG_DEBUG 0  /* debug-level messages                 */

/*
 * Set Dsp Log Level.
 */
void AX_SET_LOG_LEVEL(int level);

/*
 * Get Dsp Log Level.
 */
int AX_GET_LOG_LEVEL();

int AX_IN_LOG_LEVEL(int level);

/*
 */
const char *AX_GET_LEVEL_STR(int level);

#define AX_TRACE_DSP(level,  msg_type, format,...)                                               \
  do {                                                                         \
    if (AX_IN_LOG_LEVEL(level)) {                                              \
        xt_printf((char*)("[AX_VDSP][%s][%s %d]: " format "\n"), msg_type, __func__, __LINE__,  ##__VA_ARGS__); \
    }                                                                          \
  } while (0)

#define AX_CHECK_EXPR_RET(expr, ret, level, ...)                               \
  do {                                                                         \
    if (expr) {                                                                \
      AX_TRACE_DSP(level, "EX",##__VA_ARGS__);                                  \
      return ret;                                                              \
    }                                                                          \
  } while (0)

#define AX_LOG(level, ...)                                                     \
  do {                                                                         \
    AX_TRACE_DSP(level, "L", ##__VA_ARGS__);                                          \
  } while (0)

#define AX_VDSP_LOG_ERROR(format, ...)      AX_TRACE_DSP(AX_DBG_ERR, "E", format, ##__VA_ARGS__)
#define AX_VDSP_LOG_WARN(format, ...)       AX_TRACE_DSP(AX_DBG_WARN, "W", format, ##__VA_ARGS__)
#define AX_VDSP_LOG_INFO(format, ...)       AX_TRACE_DSP(AX_DBG_INFO, "I", format, ##__VA_ARGS__)
#define AX_VDSP_LOG_DBG(format, ...)        AX_TRACE_DSP(AX_DBG_DEBUG, "D", format, ##__VA_ARGS__)

//const char *GetComputeErrorInfo(int status);
