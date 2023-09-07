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
#include <sys/time.h>
#include <time.h>
#include <algorithm>
#include "ax_base_type.h"

class CIOPerf {
public:
    CIOPerf(AX_VOID) = default;

    AX_VOID Update(AX_U32 nBytes, AX_U64 nElapsed) {
        m_nTotalElapsed += nElapsed;
        m_nTotalBytes += nBytes;

        AX_F64 nSpeed = CalcSpeed(nBytes, nElapsed);
        if (nBytes > 0) {
            m_nMinSpeed = std::min(m_nMinSpeed, nSpeed);
            m_nMaxSpeed = std::max(m_nMaxSpeed, nSpeed);
        }
    }

    AX_VOID Reset(AX_VOID) {
        m_nTotalBytes = 0;
        m_nTotalElapsed = 0;
        m_nMinSpeed = __DBL_MAX__;
        m_nMaxSpeed = 0.0f;
    }

    AX_F64 GetMinSpeed(AX_VOID) const {
        return (m_nMinSpeed >= __DBL_MAX__) ? 0.0f : m_nMinSpeed;
    }

    AX_F64 GetAvgSpeed(AX_VOID) const {
        return CalcSpeed(m_nTotalBytes, m_nTotalElapsed);
    }

    AX_F64 GetMaxSpeed(AX_VOID) const {
        return m_nMaxSpeed;
    }

    AX_U64 GetTotalBytes(AX_VOID) const {
        return m_nTotalBytes;
    }

    static AX_U64 GetTickCount(AX_VOID) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
    }

protected:
    inline AX_F64 CalcSpeed(AX_U32 nBytes, AX_U64 nElapsed) const {
        return (nBytes * 1.0 / 1024 / 1024) / nElapsed * 1000000;
    }

private:
    AX_U64 m_nTotalBytes{0};
    AX_U64 m_nTotalElapsed{0};
    AX_F64 m_nMinSpeed{__DBL_MAX__};
    AX_F64 m_nMaxSpeed{0.0f};
};