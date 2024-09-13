/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __APP_LOG_API_H_678759A9_8DC4_42AF_90D5_82CDC4EF1386__
#define __APP_LOG_API_H_678759A9_8DC4_42AF_90D5_82CDC4EF1386__

#include <stdarg.h>
#include "ax_global_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#if 1
#define MACRO_LOG_BLACK "\033[1;30;30m"
#define MACRO_LOG_RED "\033[1;30;31m"
#define MACRO_LOG_GREEN "\033[1;30;32m"
#define MACRO_LOG_YELLOW "\033[1;30;33m"
#define MACRO_LOG_BLUE "\033[1;30;34m"
#define MACRO_LOG_PURPLE "\033[1;30;35m"
#define MACRO_LOG_WHITE "\033[1;30;37m"
#define MACRO_LOG_END "\033[0m\n"
#else
#define MACRO_LOG_BLACK
#define MACRO_LOG_RED
#define MACRO_LOG_GREEN
#define MACRO_LOG_YELLOW
#define MACRO_LOG_BLUE
#define MACRO_LOG_PURPLE
#define MACRO_LOG_WHITE
#define MACRO_LOG_END "\n"
#endif

typedef enum {
    APP_LOG_SYNC_SEND = 0,
    APP_LOG_SYNC_RECV = 1,
    APP_LOG_ASYN_SEND = 2,
    APP_LOG_ASYN_RECV = 3,
    APP_LOG_FLAG_BUTT
} APP_LOG_FLAG_E;

typedef enum {
    APP_LOG_TARGET_NULL = 0,
    APP_LOG_TARGET_SYSLOG = 1,
    APP_LOG_TARGET_APPLOG = 2,
    APP_LOG_TARGET_STDOUT = 4,
    APP_LOG_TARGET_BUTT
} APP_LOG_TARGET_E;

typedef enum {
#if defined(SLT) || defined(__SLT__)
    APP_LOG_ERROR = 1,
    APP_LOG_CRITICAL = 2,
#else
    APP_LOG_CRITICAL = 1,
    APP_LOG_ERROR = 2,
#endif
    APP_LOG_WARN = 3,
    APP_LOG_NOTICE = 4,
    APP_LOG_INFO = 5,
    APP_LOG_DEBUG = 6,
    APP_LOG_DATA = 7,
    APP_LOG_BUTT
} APP_LOG_LEVEL_E;

#define MAX_APP_NAME_LEN (32)
typedef struct {
    AX_U32 nTarget;
    AX_S32 nLv;
    AX_CHAR szAppName[MAX_APP_NAME_LEN];
} APP_LOG_ATTR_T;

/* APIs */
AX_S32 AX_APP_Log_Init(const APP_LOG_ATTR_T *pstAttr);
AX_VOID AX_APP_Log_DeInit(AX_VOID);
AX_S32 AX_APP_GetLogLevel(AX_VOID);
AX_VOID AX_APP_SetLogLevel(AX_S32 nLv);
AX_VOID AX_APP_Log_SetSysModuleInited(AX_BOOL bInited);
AX_VOID AX_APP_LogFmtStr(AX_S32 nLv, const AX_CHAR *pFmt, ...);
AX_VOID AX_APP_LogBufData(AX_S32 nLv, const AX_VOID *pBuf, AX_U32 nBufSize, AX_U32 nFlag);

#define LOGSTR AX_APP_LogFmtStr
#define LOGBUF AX_APP_LogBufData
#define LOG LOG_W
#define LOG_M LOG_M_W
#define LOG_MM LOG_MM_W

#define LOG_C(fmt, ...) LOGSTR(APP_LOG_CRITICAL, MACRO_LOG_YELLOW "K: " fmt MACRO_LOG_END, ##__VA_ARGS__)
#define LOG_E(fmt, ...) LOGSTR(APP_LOG_ERROR, MACRO_LOG_RED "E: " fmt MACRO_LOG_END, ##__VA_ARGS__)
#define LOG_W(fmt, ...) LOGSTR(APP_LOG_WARN, MACRO_LOG_YELLOW "W: " fmt MACRO_LOG_END, ##__VA_ARGS__)
#define LOG_I(fmt, ...) LOGSTR(APP_LOG_INFO, MACRO_LOG_GREEN "I: " fmt MACRO_LOG_END, ##__VA_ARGS__)
#define LOG_D(fmt, ...) LOGSTR(APP_LOG_DEBUG, MACRO_LOG_WHITE "D: " fmt MACRO_LOG_END, ##__VA_ARGS__)
#define LOG_N(fmt, ...) LOGSTR(APP_LOG_NOTICE, MACRO_LOG_PURPLE "N: " fmt MACRO_LOG_END, ##__VA_ARGS__)
#define LOG_B(buf, size, flag) LOGBUF(APP_LOG_DATA, buf, size, flag)

#define LOG_M_C(tag, fmt, ...) LOGSTR(APP_LOG_CRITICAL, MACRO_LOG_YELLOW "K %s: " fmt MACRO_LOG_END, tag, ##__VA_ARGS__)
#define LOG_M_E(tag, fmt, ...) LOGSTR(APP_LOG_ERROR, MACRO_LOG_RED "E %s: " fmt MACRO_LOG_END, tag, ##__VA_ARGS__)
#define LOG_M_W(tag, fmt, ...) LOGSTR(APP_LOG_WARN, MACRO_LOG_YELLOW "W %s: " fmt MACRO_LOG_END, tag, ##__VA_ARGS__)
#define LOG_M_I(tag, fmt, ...) LOGSTR(APP_LOG_INFO, MACRO_LOG_GREEN "I %s: " fmt MACRO_LOG_END, tag, ##__VA_ARGS__)
#define LOG_M_D(tag, fmt, ...) LOGSTR(APP_LOG_DEBUG, MACRO_LOG_WHITE "D %s: " fmt MACRO_LOG_END, tag, ##__VA_ARGS__)
#define LOG_M_N(tag, fmt, ...) LOGSTR(APP_LOG_NOTICE, MACRO_LOG_PURPLE "N %s: " fmt MACRO_LOG_END, tag, ##__VA_ARGS__)

#define LOG_MM_C(tag, fmt, ...) LOGSTR(APP_LOG_CRITICAL, MACRO_LOG_YELLOW "K %s <%s>.%d: " fmt MACRO_LOG_END, tag, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_MM_E(tag, fmt, ...) \
    LOGSTR(APP_LOG_ERROR, MACRO_LOG_RED "E %s <%s>.%d: " fmt MACRO_LOG_END, tag, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_MM_W(tag, fmt, ...) LOGSTR(APP_LOG_WARN, MACRO_LOG_YELLOW "W %s <%s>.%d: " fmt MACRO_LOG_END, tag, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_MM_I(tag, fmt, ...) \
    LOGSTR(APP_LOG_INFO, MACRO_LOG_GREEN "I %s <%s>.%d: " fmt MACRO_LOG_END, tag, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_MM_D(tag, fmt, ...) LOGSTR(APP_LOG_DEBUG, MACRO_LOG_WHITE "D %s <%s>.%d: " fmt MACRO_LOG_END, tag, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_MM_N(tag, fmt, ...) LOGSTR(APP_LOG_NOTICE, MACRO_LOG_PURPLE "N %s <%s>.%d: " fmt MACRO_LOG_END, tag, __func__, __LINE__, ##__VA_ARGS__)

#ifdef __cplusplus
}

#endif
#endif /* __APP_LOG_API_H_678759A9_8DC4_42AF_90D5_82CDC4EF1386__ */