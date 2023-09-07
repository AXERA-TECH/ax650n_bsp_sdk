/**********************************************************************************
 *
 * Copyright (c) 2019-2020 Beijing AXera Technology Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Beijing AXera Technology Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Beijing AXera Technology Co., Ltd.
 *
 **********************************************************************************/

#include <stdio.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "ax_sys_log.h"
#include "sensor_user_debug.h"

#define SENSOR_LOG_TAG "SENSOR"

static AX_LOG_LEVEL_E sensor_print_level = SYS_LOG_DEBUG;
static AX_LOG_TARGET_E sensor_print_target = SYS_LOG_TARGET_SYSLOG;

AX_S32 sns_printf(AX_U32 level, AX_CHAR *fmt, ...)
{
    va_list args;
    AX_S32 r = 0;
    if (level <= sensor_print_level) {
        va_start(args, fmt);
        switch (level) {
        case SYS_LOG_DEBUG:
            AX_SYS_LogOutput_Ex(sensor_print_target, SYS_LOG_DEBUG, SENSOR_LOG_TAG, AX_ID_SENSOR, fmt, args);
            break;
        case SYS_LOG_INFO:
            AX_SYS_LogOutput_Ex(sensor_print_target, SYS_LOG_INFO, SENSOR_LOG_TAG, AX_ID_SENSOR, fmt, args);
            break;
        case SYS_LOG_NOTICE:
            AX_SYS_LogOutput_Ex(sensor_print_target, SYS_LOG_NOTICE, SENSOR_LOG_TAG, AX_ID_SENSOR, fmt, args);
            break;
        case SYS_LOG_WARN:
            AX_SYS_LogOutput_Ex(sensor_print_target, SYS_LOG_WARN, SENSOR_LOG_TAG, AX_ID_SENSOR, fmt, args);
            break;
        case SYS_LOG_ERROR:
            AX_SYS_LogOutput_Ex(sensor_print_target, SYS_LOG_ERROR, SENSOR_LOG_TAG, AX_ID_SENSOR, fmt, args);
            break;
        default:
            break;
        }
        va_end(args);
    }
    return r;
}

AX_S32 ax_sensor_set_log_level(AX_LOG_LEVEL_E level)
{
    if (level >= SYS_LOG_MAX || level <= SYS_LOG_MIN) {
        SNS_ERR("sensor log level is invalid[%d]\n", level);
        return -1;
    }
    sensor_print_level = level;
    return 0;
}

AX_LOG_LEVEL_E ax_sensor_get_log_level(void)
{
    return sensor_print_level;
}

AX_S32 ax_sensor_set_log_target(AX_LOG_TARGET_E target)
{
    if (target >= SYS_LOG_TARGET_MAX || target <= SYS_LOG_TARGET_MIN) {
        SNS_ERR("log target is invalid[%d]\n", target);
        return -1;
    }
    sensor_print_target = target;
    return 0;
}

AX_S32 ax_sensor_set_user_debug_log(AX_VOID)
{
    AX_CHAR *log_level;
    AX_CHAR *log_target;
    AX_LOG_LEVEL_E level = SYS_LOG_WARN;
    AX_LOG_TARGET_E target = SYS_LOG_TARGET_SYSLOG;

    log_level = (AX_CHAR *)getenv("SENSOR_LOG_level");
    if (log_level) {
        level = (AX_LOG_LEVEL_E)atoi((const char *)log_level);
        ax_sensor_set_log_level(level);
    }

    log_target = (AX_CHAR *)getenv("SENSOR_LOG_target");
    if (log_target) {
        target = (AX_LOG_TARGET_E)atoi((const char *)log_target);
        ax_sensor_set_log_target(target);
    }

    return 0;
}
