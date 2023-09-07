#ifndef _AX_THREAD_SIGNAL_H_
#define _AX_THREAD_SIGNAL_H_
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct ThreadSignal_T {
    unsigned int clocktype;
    unsigned int flag;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    pthread_condattr_t cattr;
} ThreadSignal;

static int AX_ThreadSignal_Init(ThreadSignal *signal, bool relativeTimespan);
static int AX_ThreadSignal_Wait(ThreadSignal *signal, int ms);
static int AX_ThreadSignal_Signal(ThreadSignal *signal);
static void AX_ThreadSignal_Destroy(ThreadSignal *signal);

static int AX_ThreadSignal_Init(ThreadSignal *signal, bool relativeTimespan)
{
    if (relativeTimespan)
        signal->clocktype = CLOCK_MONOTONIC;
    else
        signal->clocktype = CLOCK_REALTIME;

    if (pthread_mutex_init(&signal->mutex, NULL))
        return -1;

    if (pthread_condattr_init(&signal->cattr))
            goto err_condattr_init;

    if (pthread_condattr_setclock(&signal->cattr, signal->clocktype))
        goto err_condattr_setclock;

    if (pthread_cond_init(&signal->cond, &signal->cattr))
            goto err_cond_init;
    signal->flag = 0;
    return 0;

err_cond_init:
err_condattr_setclock:
    pthread_condattr_destroy(&(signal->cattr));
err_condattr_init:
    pthread_mutex_destroy(&signal->mutex);
    return -1;
}

static int AX_ThreadSignal_Wait(ThreadSignal *signal, int ms)
{
    int ret = 0;
    uint64_t us = 0;
    struct timespec outtime;

    pthread_mutex_lock(&signal->mutex);

    clock_gettime(signal->clocktype, &outtime);
    outtime.tv_sec += ms / 1000;

    us = outtime.tv_nsec / 1000 + 1000 * (ms % 1000);
    outtime.tv_sec += us / 1000000;

    us = us % 1000000;
    outtime.tv_nsec = us * 1000;

    if (signal->flag == 0)
        ret = pthread_cond_timedwait(&signal->cond, &signal->mutex, &outtime);
    signal->flag = 0;
    pthread_mutex_unlock(&signal->mutex);

    return ret;
}

static int AX_ThreadSignal_Signal(ThreadSignal *signal)
{
    int ret = 0;

    pthread_mutex_lock(&signal->mutex);
    signal->flag = 1;
    ret = pthread_cond_signal(&signal->cond);
    pthread_mutex_unlock(&signal->mutex);
    return ret;
}

static void AX_ThreadSignal_Destroy(ThreadSignal *signal)
{
    pthread_condattr_destroy(&(signal->cattr));
    pthread_mutex_destroy(&signal->mutex);
    pthread_cond_destroy(&signal->cond);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif