/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "utils.h"
#include <errno.h>
#include <signal.h>
#include <time.h>

AX_U64 get_tick_count(AX_VOID) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
}

AX_VOID msleep(AX_U32 milliseconds) {
    struct timespec ts = {(milliseconds / 1000), (milliseconds % 1000) * 1000000};
    while ((-1 == nanosleep(&ts, &ts)) && (EINTR == errno))
        ;
}
