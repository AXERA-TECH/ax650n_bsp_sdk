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
#include <list>
#include <mutex>
#include "AXFrame.hpp"
#include "AXLockQ.hpp"
#include "AXThread.hpp"
#include "BmpOSD.hpp"
#include "DetectResult.hpp"
#include "IObserver.h"

#define DRAW_FHVP_LABEL

enum class AX_DISP_TYPE {
    SRC_SAME = 0,
    SRC_DIFF = 1,
    BUTT = 2,
};

/**
 * @brief
 *
 */
typedef struct {
    AX_S32 vdGrp;
    std::string strBmpFontPath;
    AX_S32 nDepth;
    AX_DISP_TYPE enDispType;
} DISPATCH_ATTR_T;

class CDispatcher {
public:
    CDispatcher(AX_VOID) = default;

    AX_BOOL Init(const DISPATCH_ATTR_T& stAttr);
    AX_BOOL DeInit(AX_VOID);

    AX_BOOL Start(AX_VOID);
    AX_BOOL Stop(AX_VOID);

    AX_BOOL Clear(AX_VOID);

    AX_BOOL SendFrame(const CAXFrame& axFrame);

    AX_BOOL RegObserver(IObserver* pObs);
    AX_BOOL UnRegObserver(IObserver* pObs);

protected:
    AX_VOID DispatchThread(AX_VOID* pArg);
    AX_VOID ClearQueue(AX_VOID);
    AX_VOID DrawBox(const CAXFrame& axFrame, const DETECT_RESULT_T& fhvp);

protected:
    CAXLockQ<CAXFrame> m_qFrame;
    CAXThread m_DispatchThread;
    std::list<IObserver*> m_lstObs;
    std::mutex m_mtxObs;
    AX_S32 m_vdGrp{0};
    AX_DISP_TYPE m_enDispType{AX_DISP_TYPE::BUTT};

#ifdef DRAW_FHVP_LABEL
    CBmpOSD m_font;
#endif
};