/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "event.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "log.h"

#define TAG "EVENT"

typedef struct {
    AX_CHAR name[64];
    pthread_mutex_t mutx;
    pthread_cond_t cond;
    AX_BOOL pred;
} event_obj_t;

AX_HANDLE sample_create_event(const AX_CHAR *name) {
    event_obj_t *obj = CALLOC(1, sizeof(event_obj_t));
    if (!obj) {
        LOG_M_E(TAG, "create %s event obj fail", name);
        return AX_INVALID_HANDLE;
    }

    obj->pred = AX_FALSE;
    if (name) {
        strcpy(obj->name, name);
    } else {
        strcpy(obj->name, "unknown");
    }

    pthread_mutex_init(&obj->mutx, AX_NULL);

    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);
    pthread_cond_init(&obj->cond, &cond_attr);
    pthread_condattr_destroy(&cond_attr);

    return (AX_HANDLE)obj;
}

AX_BOOL sample_destory_event(AX_HANDLE handle) {
    event_obj_t *obj = (event_obj_t *)handle;
    if (!obj) {
        LOG_M_E(TAG, "event obj is nil");
        return AX_FALSE;
    }

    pthread_cond_destroy(&obj->cond);
    pthread_mutex_destroy(&obj->mutx);

    FREE(obj);
    return AX_TRUE;
}

AX_BOOL sample_set_event(AX_HANDLE handle) {
    event_obj_t *obj = (event_obj_t *)handle;
    if (!obj) {
        LOG_M_E(TAG, "event obj is nil");
        return AX_FALSE;
    }

    pthread_mutex_lock(&obj->mutx);
    obj->pred = AX_TRUE;
    pthread_cond_signal(&obj->cond);
    pthread_mutex_unlock(&obj->mutx);

    return AX_TRUE;
}

AX_BOOL sample_reset_event(AX_HANDLE handle) {
    event_obj_t *obj = (event_obj_t *)handle;
    if (!obj) {
        LOG_M_E(TAG, "event obj is nil");
        return AX_FALSE;
    }

    pthread_mutex_lock(&obj->mutx);
    obj->pred = AX_FALSE;
    pthread_mutex_unlock(&obj->mutx);

    return AX_TRUE;
}
AX_BOOL sample_wait_event(AX_HANDLE handle, AX_S32 timeout) {
    event_obj_t *obj = (event_obj_t *)handle;
    if (!obj) {
        LOG_M_E(TAG, "event obj is nil");
        return AX_FALSE;
    }

    AX_BOOL ret = AX_TRUE;
    pthread_mutex_lock(&obj->mutx);

    if (timeout < 0) {
        while (!obj->pred) {
            pthread_cond_wait(&obj->cond, &obj->mutx);
        }
    } else if (0 == timeout) {
        ret = obj->pred ? AX_TRUE : AX_FALSE;
    } else {
        struct timespec now, abs;
        clock_gettime(CLOCK_MONOTONIC, &now);

        abs.tv_sec = now.tv_sec + (timeout / 1000);
        abs.tv_nsec = now.tv_nsec + (timeout % 1000) * 1000000;
        if (abs.tv_nsec >= 1000000000) {
            abs.tv_nsec -= 1000000000;
            abs.tv_sec += 1;
        }

        while (!obj->pred) {
            if (0 != pthread_cond_timedwait(&obj->cond, &obj->mutx, &abs)) {
                ret = AX_FALSE;
                break;
            }
        }
    }

    pthread_mutex_unlock(&obj->mutx);
    return ret;
}