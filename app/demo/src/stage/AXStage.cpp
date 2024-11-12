/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "AXStage.hpp"
using namespace std;

AX_VOID CAXStage::StageThreadFunc(AX_VOID* pArg) {
    while (m_StageThread.IsRunning()) {
        CAXFrame* pFrame{nullptr};
        if (!m_qFrame.Pop(pFrame, -1)) {
            continue;
        }

        if (!pFrame) {
            continue;
        }

        if (ProcessFrame(pFrame)) {
            CAXStage* pNextStage = GetNextStage();
            if (pNextStage) {
                if (pNextStage->EnqueueFrame(pFrame)) {
                    continue;
                }
            }
        }

        pFrame->FreeMem();
    }
}

CAXStage::CAXStage(const string& strName) : m_strStageName(strName) {
}

CAXStage::~CAXStage() {
    do {
        CAXFrame* pFrame{nullptr};
        if (!m_qFrame.Pop(pFrame, 0)) {
            break;
        }
        if (pFrame) {
            pFrame->FreeMem();
        }
    } while(1);
}

AX_BOOL CAXStage::Init(AX_VOID) {
    return AX_TRUE;
}

AX_BOOL CAXStage::DeInit(AX_VOID) {
    return AX_TRUE;
}

AX_BOOL CAXStage::Start(AX_VOID) {
    if (m_StageThread.IsRunning()) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CAXStage::Start(STAGE_START_PARAM_PTR pStartParams) {
    if (m_StageThread.IsRunning()) {
        return AX_FALSE;
    }

    if (pStartParams && pStartParams->bStartProcessingThread) {
        if (!m_StageThread.Start([this](AX_VOID* pArg) -> AX_VOID { StageThreadFunc(pArg); }, nullptr, m_strStageName.c_str())) {
            return AX_FALSE;
        }
    }

    return AX_TRUE;
}

AX_BOOL CAXStage::Stop(AX_VOID) {
    m_StageThread.Stop();
    m_qFrame.Wakeup();
    m_StageThread.Join();

    return AX_TRUE;
}

AX_BOOL CAXStage::EnqueueFrame(CAXFrame* pFrame) {
    if (!m_StageThread.IsRunning() ||
        !m_qFrame.Push(pFrame)) {
        pFrame->FreeMem();
        return AX_FALSE;
    }
    return AX_TRUE;
}

AX_BOOL CAXStage::ProcessFrame(CAXFrame* pFrame) {
    return AX_TRUE;
}

CAXStage* CAXStage::BindNextStage(CAXStage* pNext) {
    if (!pNext) {
        return nullptr;
    }

    m_pNextStage = pNext;
    return pNext;
}

CAXStage* CAXStage::GetNextStage(AX_VOID) {
    return m_pNextStage;
}

const string& CAXStage::GetStageName(AX_VOID) const {
    return m_strStageName;
}

AX_VOID CAXStage::SetCapacity(AX_S32 nCapacity) {
    m_qFrame.SetCapacity(nCapacity);
}