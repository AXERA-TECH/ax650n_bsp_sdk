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
#include "condition_variable.hpp"
#include <mutex>
#include "ax_base_type.h"

class CAXEvent final {
public:
    CAXEvent(AX_VOID) = default;
    ~CAXEvent(AX_VOID) = default;

    AX_BOOL WaitEvent(AX_S32 nTimeOut = -1) {
        std::unique_lock<std::mutex> lck(m_mtx);
        if (m_bSignal) {
            return AX_TRUE;
        }

        if (0 == nTimeOut) {
            return AX_FALSE;
        } else if (nTimeOut < 0) {
            m_cv.wait(lck, [this]() -> bool { return m_bSignal; });
            return AX_TRUE;
        } else {
            return m_cv.wait_for(lck, std::chrono::milliseconds(nTimeOut), [this]() -> bool { return m_bSignal; }) ? AX_TRUE : AX_FALSE;
        }
    }

    AX_VOID SetEvent(AX_VOID) {
        m_mtx.lock();
        m_bSignal = true;
        m_mtx.unlock();

        m_cv.notify_one();
    }

    AX_VOID ResetEvent(AX_VOID) {
        m_mtx.lock();
        m_bSignal = false;
        m_mtx.unlock();
    }

private:
    std::mutex m_mtx;
    std::condition_variable m_cv;
    bool m_bSignal{false};
};
