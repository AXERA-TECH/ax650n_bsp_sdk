/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef __SAMPLE_LOG_H__
#define __SAMPLE_LOG_H__

#include <stdarg.h>
#include "def.h"

#define MACRO_LOG_BLACK     "\033[1;30;30m"
#define MACRO_LOG_RED       "\033[1;30;31m"
#define MACRO_LOG_GREEN     "\033[1;30;32m"
#define MACRO_LOG_YELLOW    "\033[1;30;33m"
#define MACRO_LOG_BLUE      "\033[1;30;34m"
#define MACRO_LOG_PURPLE    "\033[1;30;35m"
#define MACRO_LOG_WHITE     "\033[1;30;37m"
#define MACRO_LOG_END       "\033[0m\n"

typedef enum {
    LOG_LEVEL_NONE  = 0,
    LOG_LEVEL_CRIT  = 1,
    LOG_LEVEL_ERROR = 2,
    LOG_LEVEL_WARN  = 3,
    LOG_LEVEL_INFO  = 4,
    LOG_LEVEL_DEBUG = 5,
    LOG_LEVEL_BUTT
} LOG_LEVEL_E;

AX_VOID logger(AX_S32 lv, const AX_CHAR* fmt, ...);
AX_VOID set_log_level(AX_S32 lv);
#define LOGGER logger

#define LOG_M_C(tag, fmt, ...) LOGGER(LOG_LEVEL_CRIT,  MACRO_LOG_YELLOW "K %s: " fmt MACRO_LOG_END, tag, ##__VA_ARGS__)
#define LOG_M_E(tag, fmt, ...) LOGGER(LOG_LEVEL_ERROR, MACRO_LOG_RED    "E %s: " fmt MACRO_LOG_END, tag, ##__VA_ARGS__)
#define LOG_M_W(tag, fmt, ...) LOGGER(LOG_LEVEL_WARN,  MACRO_LOG_YELLOW "W %s: " fmt MACRO_LOG_END, tag, ##__VA_ARGS__)
#define LOG_M_I(tag, fmt, ...) LOGGER(LOG_LEVEL_INFO,  MACRO_LOG_GREEN  "I %s: " fmt MACRO_LOG_END, tag, ##__VA_ARGS__)
#define LOG_M_D(tag, fmt, ...) LOGGER(LOG_LEVEL_DEBUG, MACRO_LOG_WHITE  "D %s: " fmt MACRO_LOG_END, tag, ##__VA_ARGS__)

#define LOG_MM_C(tag, fmt, ...) \
    LOGGER(LOG_LEVEL_CRIT,  MACRO_LOG_GREEN "K %s <%s>.%d: " fmt MACRO_LOG_END, tag, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_MM_E(tag, fmt, ...) \
    LOGGER(LOG_LEVEL_ERROR, MACRO_LOG_GREEN "E %s <%s>.%d: " fmt MACRO_LOG_END, tag, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_MM_W(tag, fmt, ...) \
    LOGGER(LOG_LEVEL_WARN,  MACRO_LOG_GREEN "W %s <%s>.%d: " fmt MACRO_LOG_END, tag, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_MM_I(tag, fmt, ...) \
    LOGGER(LOG_LEVEL_INFO,  MACRO_LOG_GREEN "I %s <%s>.%d: " fmt MACRO_LOG_END, tag, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_MM_D(tag, fmt, ...) \
    LOGGER(LOG_LEVEL_DEBUG, MACRO_LOG_GREEN "D %s <%s>.%d: " fmt MACRO_LOG_END, tag, __func__, __LINE__, ##__VA_ARGS__)

#define ENTER_FUNC() LOG_MM_I(TAG, "+++")
#define LEAVE_FUNC() LOG_MM_I(TAG, "---")

#endif /* __SAMPLE_LOG_H__ */
