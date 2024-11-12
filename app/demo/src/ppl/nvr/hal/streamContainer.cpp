/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "streamContainer.hpp"
#include "AppLogApi.h"

#define TAG "STM_CONTAINER"

AX_VOID CVideoStreamContainer::Flush(AX_VOID) {
    {
        std::unique_lock<std::mutex> lck(m_mtx);
        /* wait consume */
        m_cvW.wait(lck, [this]() -> bool { return GetLeftCnt() > 0 || m_bAbortW; });
        if (m_bAbortW) {
            m_bAbortW = false;
            return;
        }

        size_t sz = m_stacks[m_wIndex % m_nCnt].size();
        if (0 == sz) {
#ifdef __PRINT_DEBUG_INFO__
            printf("nothing flush, wIndex %lld, rIndex %lld\n", m_wIndex, m_rIndex);
#endif
            return;
        }

        ++m_wIndex;

#ifdef __PRINT_DEBUG_INFO__
        printf("flush %d, swap: wIndex %lld, rIndex %lld\n", sz, m_wIndex, m_rIndex);
#endif
    }

    m_cvR.notify_one();
}

AX_BOOL CVideoStreamContainer::Push(const AX_VENC_PACK_T& pack, AX_S32 nTimeOut /* = -1 */) {
    std::unique_lock<std::mutex> lck(m_mtx);
    if (0 == GetLeftCnt()) {
        if (0 == nTimeOut) {
            return AX_FALSE;
        } else if (nTimeOut < 0) {
            m_cvW.wait(lck, [this]() -> bool { return GetLeftCnt() > 0 || m_bAbortW; });
        } else {
            if (!m_cvW.wait_for(lck, std::chrono::milliseconds(nTimeOut), [this]() -> bool { return GetLeftCnt() > 0 || m_bAbortW; })) {
                return AX_FALSE;
            }
        }
    }

    if (m_bAbortW) {
#ifdef __PRINT_DEBUG_INFO__
        printf("CVideoStreamContainer::Push aborted\n");
#endif
        m_bAbortW = false;
        return AX_FALSE;
    }

    std::stack<VIDEO_STREAM_INFO_T>& stack = m_stacks[m_wIndex % m_nCnt];
    stack.push(pack);
    if (stack.size() == m_nCap) {
        ++m_wIndex;

#ifdef __PRINT_DEBUG_INFO__
        printf("push %d, swap: wIndex %lld, rIndex %lld\n", m_nCap, m_wIndex, m_rIndex);
#endif
        m_cvR.notify_one();
    }

    return AX_TRUE;
}

AX_BOOL CVideoStreamContainer::Pop(VIDEO_STREAM_INFO_T& m, AX_S32 nTimeOut /* = -1 */) {
    while (1) {
        std::unique_lock<std::mutex> lck(m_mtx);
        if (m_rIndex == m_wIndex) {
            if (0 == nTimeOut) {
                return AX_FALSE;
            } else if (nTimeOut < 0) {
                m_cvR.wait(lck, [this]() -> bool { return (m_wIndex > m_rIndex) || m_bAbortR; });
            } else {
                if (!m_cvR.wait_for(lck, std::chrono::milliseconds(nTimeOut),
                                    [this]() -> bool { return (m_wIndex > m_rIndex) || m_bAbortR; })) {
                    return AX_FALSE;
                }
            }
        }

        if (m_bAbortR) {
#ifdef __PRINT_DEBUG_INFO__
            printf("CVideoStreamContainer::Pop aborted\n");
#endif
            m_bAbortR = false;
            return AX_FALSE;
        }

        std::stack<VIDEO_STREAM_INFO_T>& stack = m_stacks[m_rIndex % m_nCnt];
        auto sz = stack.size();
        if (sz > 0) {
            m = stack.top();
            stack.pop();
#ifdef __PRINT_DEBUG_INFO__
            ++m_nPopCnt;
#endif

            if (--sz == 0) {
                ++m_rIndex;

#ifdef __PRINT_DEBUG_INFO__
                printf("pop %d, swap: wIndex %lld, rIndex %lld\n", m_nPopCnt, m_wIndex, m_rIndex);
                m_nPopCnt = 0;
#endif
                m_cvW.notify_one();
            }

            return AX_TRUE;
        }
    }

    return AX_FALSE;
}

AX_VOID CVideoStreamContainer::Clear(AX_VOID) {
#ifdef __PRINT_DEBUG_INFO__
    printf("CVideoStreamContainer::Clear +++ AbortW = %d, AbortR = %d\n", m_bAbortW, m_bAbortR);
#endif
    {
        std::lock_guard<std::mutex> lck(m_mtx);
        for (auto i = m_rIndex; i < m_wIndex; ++i) {
            std::stack<VIDEO_STREAM_INFO_T>& stack = m_stacks[i % m_nCnt];
            auto sz = stack.size();
            while (sz > 0) {
                VIDEO_STREAM_INFO_T m = stack.top();
                stack.pop();
                --sz;
                m.FreeMem();
            }
        }

        m_bAbortW = true;
        m_bAbortR = true;
    }

    m_cvW.notify_one();
    m_cvR.notify_one();

#ifdef __PRINT_DEBUG_INFO__
    printf("CVideoStreamContainer::Clear --- AbortW = %d, AbortR = %d\n", m_bAbortW, m_bAbortR);
#endif
}

AX_VOID CVideoStreamContainer::AdjustPTS(AX_U32 nAdjustPTSDiff) {
    std::unique_lock<std::mutex> lck(m_mtx);

    size_t sz = m_stacks[m_wIndex % m_nCnt].size();
    if (0 == sz) {
        return;
    }

    std::stack<VIDEO_STREAM_INFO_T> stackTmp;
    for (size_t i = 0; i < sz; ++i) {
        VIDEO_STREAM_INFO_T m = m_stacks[m_wIndex % m_nCnt].top();
        m_stacks[m_wIndex % m_nCnt].pop();
        if (m.u64PTS > nAdjustPTSDiff) {
            m.u64PTS -= nAdjustPTSDiff;
        }

        stackTmp.push(m);
    }

    for (size_t i = 0; i < sz; ++i) {
        VIDEO_STREAM_INFO_T m = stackTmp.top();
        m_stacks[m_wIndex % m_nCnt].push(m);
        stackTmp.pop();
    }
}