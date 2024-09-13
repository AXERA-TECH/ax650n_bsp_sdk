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
#include "AXFrame.hpp"
#include "AXLockQ.hpp"
#include "AXThread.hpp"
#include "BmpOSD.hpp"
#include "DetectResult.hpp"
#include "IObserver.h"

#define DRAW_FHVP_LABEL
/**
 * @brief
 *
 */
typedef struct {
    AX_S32 vdGrp;
    std::string strBmpFontPath;
    AX_S32 nDepth;
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

    AX_S32  GetGrp() { return m_vdGrp; };

protected:
    AX_VOID DispatchThread(AX_VOID* pArg);
    AX_VOID ClearQueue(AX_VOID);
    AX_VOID DrawBox(const CAXFrame& axFrame, const DETECT_RESULT_T& fhvp);

protected:
    CAXLockQ<CAXFrame> m_qFrame;
    CAXThread m_DispatchThread;
    std::list<IObserver*> m_lstObs;
    AX_S32 m_vdGrp{0};

#ifdef DRAW_FHVP_LABEL
    CBmpOSD m_font;
#endif
};