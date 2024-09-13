/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "log.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "utils.h"

#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#define MAX_LOG_BUFF (1024)

static AX_S32 g_lv = LOG_LEVEL_WARN;

static AX_VOID __logger(const AX_CHAR* fmt, va_list va) {
    AX_CHAR buf[MAX_LOG_BUFF];
    AX_S32 len = vsnprintf(buf, sizeof(buf), fmt, va);
    if (len > 0) {
        struct timeval tv;
        struct tm t;
        gettimeofday(&tv, AX_NULL);
        time_t now = time(AX_NULL);
        localtime_r(&now, &t);
        fprintf(stdout, "%02u-%02u %02u:%02u:%02u:%03u %lld %6lu %s", t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
                (AX_U32)(tv.tv_usec / 1000), get_tick_count(), (long unsigned int)gettid(), buf);
    }
}

AX_VOID logger(AX_S32 lv, const AX_CHAR* fmt, ...) {
    if (lv <= g_lv) {
        va_list va;
        va_start(va, fmt);
        __logger(fmt, va);
        va_end(va);
    }
}

AX_VOID set_log_level(AX_S32 lv) {
    if (lv >= LOG_LEVEL_NONE && lv < LOG_LEVEL_BUTT) {
        g_lv = lv;
    }
}