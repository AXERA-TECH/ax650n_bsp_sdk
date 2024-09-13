/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _IVPS_GLOBAL_H_
#define _IVPS_GLOBAL_H_
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/epoll.h>

/* #define IVPS_SAMPLE_LOG_EN */

#ifdef IVPS_SAMPLE_LOG_EN
#define ALOGD(fmt, ...) printf("\033[1;30;37mDEBUG  :[%s:%d] " fmt "\033[0m\n", __func__, __LINE__, ##__VA_ARGS__) // white
#define ALOGI(fmt, ...) printf("\033[1;30;32mINFO   :[%s:%d] " fmt "\033[0m\n", __func__, __LINE__, ##__VA_ARGS__) // green
#else
#define ALOGD(fmt, ...) \
    do                  \
    {                   \
    } while (0)
#define ALOGI(fmt, ...) \
    do                  \
    {                   \
    } while (0)
#endif
#define ALOGW(fmt, ...) printf("\033[1;30;33mWARN   :[%s:%d] " fmt "\033[0m\n", __func__, __LINE__, ##__VA_ARGS__) // yellow
#define ALOGE(fmt, ...) printf("\033[1;30;31mERROR  :[%s:%d] " fmt "\033[0m\n", __func__, __LINE__, ##__VA_ARGS__) // red
#define ALOGN(fmt, ...) printf("\033[1;30;37mINFO   :[%s:%d] " fmt "\033[0m\n", __func__, __LINE__, ##__VA_ARGS__) // white

#define LIMIT_MIN(a, min)                           \
    {                                               \
        if (a < min)                                \
        {                                           \
            ALOGE("a:%d should >= min:%d", a, min); \
            return -1;                              \
        }                                           \
    }

#define CHECK_RESULT(statement)                   \
    {                                             \
        AX_U32 ret = statement;                   \
        if (ret)                                  \
        {                                         \
            ALOGE("return error, ret=0x%x", ret); \
            return -1;                            \
        }                                         \
    }

#define CHECK_RESULT_V2(statement)                \
    {                                             \
        AX_U32 ret = statement;                   \
        if (ret)                                  \
        {                                         \
            ALOGE("return error, ret=0x%x", ret); \
            return NULL;                          \
        }                                         \
    }

#define CHECK_POINTER(p, key_str)          \
    if (!p)                                \
    {                                      \
        ALOGE("%s:null pointer", key_str); \
        return -1;                         \
    }
#endif /* _IVPS_GLOBAL_H_ */
