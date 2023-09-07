/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#pragma once
#include <error.h>
#include <pthread.h>
#include <string.h>
#include <sys/prctl.h>
#include <atomic>
#include <exception>
#include <functional>
#include <thread>
#include "AXEvent.hpp"
#ifndef PRINT_THREAD_LOGGER
#define PRINT_THREAD_LOGGER
#include <stdio.h>
#define THREAD_LOGGER(fmt, ...) printf("%s: " fmt "\n", __func__, ##__VA_ARGS__)
#else
#define THREAD_LOGGER(fmt, ...)
#endif

using ThreadJobFunc = std::function<AX_VOID(AX_VOID *)>;

class CAXThread final {
public:
    CAXThread(AX_VOID) = default;

    AX_BOOL IsRunning(AX_VOID) const;
    AX_BOOL IsPausing(AX_VOID) const;

    AX_BOOL Start(ThreadJobFunc Job, AX_VOID *pArg = nullptr, const AX_CHAR *pszJobName = nullptr, AX_S32 nPolicy = SCHED_OTHER,
                  AX_S32 nPriority = 0) {
        if (!Job) {
            THREAD_LOGGER("thread job is nil");
            return AX_FALSE;
        }

        if (m_thread.joinable()) {
            m_thread.join();
        }

        m_Job = Job;
        m_JobArg = pArg;
        if (pszJobName) {
            if (strlen(pszJobName) > (sizeof(m_szJobName))) {
                memcpy(m_szJobName, pszJobName, sizeof(m_szJobName) - 1);
            } else {
                strcpy(m_szJobName, pszJobName);
            }
        } else {
            strcpy(m_szJobName, "AxThread");
        }

        m_event.ResetEvent();
        m_thread = std::move(std::thread([this]() -> AX_VOID { RunJob(); }));
        m_event.WaitEvent(-1);

        if (SCHED_FIFO == nPolicy || SCHED_RR == nPolicy) {
            (AX_VOID) SetSchedPolicy(nPolicy, nPriority);
        }

        return AX_TRUE;
    }

    AX_VOID Stop(AX_VOID) {
        m_state = STOP;
    }

    AX_VOID Pause(AX_VOID) {
        m_state = PAUSE;
    }

    AX_VOID Resume(AX_VOID) {
        m_state = RUNNING;
    }

    AX_VOID Join(AX_VOID) {
        if (m_thread.joinable()) {
            m_thread.join();
        }

        m_state = IDLE;
    }

private:
    AX_VOID RunJob(AX_VOID) {
        m_state = RUNNING;
        (AX_VOID) prctl(PR_SET_NAME, m_szJobName);
        m_event.SetEvent();

        m_Job(m_JobArg);
    }

    AX_BOOL SetSchedPolicy(AX_S32 nPolicy, AX_S32 nPriority) {
        sched_param sch;
        int policy;
        pthread_getschedparam(m_thread.native_handle(), &policy, &sch);
        int nMin = sched_get_priority_min(nPolicy);
        int nMax = sched_get_priority_max(nPolicy);
        if (nPriority < nMin || nPriority > nMax) {
            THREAD_LOGGER("priority %d is out of range for thread %s policy %d is [%d - %d]", nPriority, m_szJobName, nPolicy, nMin, nMax);
            return AX_FALSE;
        }

        sch.sched_priority = nPriority;
        if (0 != pthread_setschedparam(m_thread.native_handle(), nPolicy, &sch)) {
            THREAD_LOGGER("set thread %s sched to policy %d priority %d fail, %s", m_szJobName, nPolicy, nPriority, strerror(errno));
            return AX_FALSE;
        }

        return AX_TRUE;
    }

    CAXThread(const CAXThread &) = delete;
    CAXThread &operator=(const CAXThread &) = delete;

private:
    enum { IDLE = 0, RUNNING = 2, STOP = 3, PAUSE = 4 };

    ThreadJobFunc m_Job = {nullptr};
    AX_VOID *m_JobArg = {nullptr};
    AX_CHAR m_szJobName[16] = {0};
    std::thread m_thread;
    std::atomic<AX_S32> m_state = {IDLE};
    CAXEvent m_event;
};

inline AX_BOOL CAXThread::IsRunning(AX_VOID) const {
    return (RUNNING == m_state) ? AX_TRUE : AX_FALSE;
}

inline AX_BOOL CAXThread::IsPausing(AX_VOID) const {
    return (PAUSE == m_state) ? AX_TRUE : AX_FALSE;
}