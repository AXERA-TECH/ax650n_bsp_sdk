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
#include <chrono>
#include <mutex>
#include <queue>
#include "ax_base_type.h"
#include "condition_variable.hpp"

template <typename T>
class CAXLockQ {
public:
    CAXLockQ(AX_VOID) = default;
    virtual ~CAXLockQ(AX_VOID) = default;

    AX_VOID SetCapacity(AX_S32 nCapacity) {
        std::lock_guard<std::mutex> lck(m_mtx);
        m_nCapacity = (nCapacity < 0) ? (AX_U32)-1 : nCapacity;
    }

    AX_S32 GetCapacity(AX_VOID) const {
        std::lock_guard<std::mutex> lck(m_mtx);
        return (m_nCapacity == ((AX_U32)-1)) ? -1 : m_nCapacity;
    }

    AX_S32 GetCount(AX_VOID) const {
        std::lock_guard<std::mutex> lck(m_mtx);
        return m_q.size();
    }

    AX_BOOL IsFull(AX_VOID) const {
        std::lock_guard<std::mutex> lck(m_mtx);
        return (m_q.size() >= m_nCapacity) ? AX_TRUE : AX_FALSE;
    }

    AX_VOID Wakeup(AX_VOID) {
        std::lock_guard<std::mutex> lck(m_mtx);
        m_bWakeup = true;
        m_cv.notify_one();
    }

    AX_BOOL Push(const T& m) {
        std::lock_guard<std::mutex> lck(m_mtx);
        if (m_q.size() >= m_nCapacity) {
            return AX_FALSE;
        }
        m_q.push(m);
        m_cv.notify_one();
        return AX_TRUE;
    }

    AX_BOOL Pop(T& m, AX_S32 nTimeOut = -1) {
        std::unique_lock<std::mutex> lck(m_mtx);
        AX_BOOL bAvail{AX_TRUE};
        if (0 == m_q.size()) {
            if (0 == nTimeOut) {
                bAvail = AX_FALSE;
            } else {
                if (nTimeOut < 0) {
                    m_cv.wait(lck, [this]() -> bool { return !m_q.empty() || m_bWakeup; });
                } else {
                    m_cv.wait_for(lck, std::chrono::milliseconds(nTimeOut), [this]() -> bool { return !m_q.empty() || m_bWakeup; });
                }

                m_bWakeup = false;
                bAvail = m_q.empty() ? AX_FALSE : AX_TRUE;
            }
        }

        if (bAvail) {
            m = m_q.front();
            m_q.pop();
        }

        return bAvail;
    }

private:
    /* delete copy and assignment ctor, std::vector<CAXLockQ<X>> is not allowed */
    CAXLockQ(const CAXLockQ&) = delete;
    CAXLockQ(CAXLockQ&&) = delete;
    CAXLockQ& operator=(const CAXLockQ&) = delete;
    CAXLockQ& operator=(CAXLockQ&&) = delete;

private:
    AX_U32 m_nCapacity{(AX_U32)-1};
    std::queue<T> m_q;
    mutable std::mutex m_mtx;
    std::condition_variable m_cv;
    bool m_bWakeup{false};
};
