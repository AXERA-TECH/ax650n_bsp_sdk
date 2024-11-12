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
#include <vector>
#include "AXFrame.hpp"
#include "AXThread.hpp"
#include "IObserver.h"

/**
 * @brief
 *
 */

class CCapture {
public:
    CCapture(AX_VOID) = default;
    ~CCapture(AX_VOID) = default;

    AX_BOOL Init(AX_VOID);
    AX_BOOL DeInit(AX_VOID);

    AX_BOOL SendFrame(AX_U32 nGrp, CAXFrame* axFrame);
    AX_BOOL CapturePicture(AX_U8 nGrp, AX_U8 nChn, AX_U32 nQpLevel, AX_VOID **ppCallbackData);

    AX_VOID RegObserver(IObserver* pObserver);
    AX_VOID UnregObserver(IObserver* pObserver);

    AX_VOID NotifyAll(AX_U32 nGrp, AX_U32 nType, AX_VOID* pStream);

private:
    AX_VOID ResetCaptureStatus(AX_VOID);

private:
    std::vector<IObserver*> m_vecObserver;

    CAXFrame* m_pAXFrame{nullptr};
    std::mutex m_mutexStat;
    std::mutex m_mutexCapture;
    std::condition_variable m_cvCapture;

    AX_BOOL m_bCapture{AX_FALSE};
    AX_U8 m_nCaptureGrp{0};
    AX_U8 m_nCaptureChn{0};
};
