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
#include <exception>
#include "AXFrame.hpp"
#include "Detector.hpp"
#include "IObserver.h"

/**
 * @brief
 *
 */
class CDetectObserver final : public IObserver {
public:
    CDetectObserver(CDetector* pSink, AX_U32 nChn) noexcept : m_pSink(pSink), m_nChn(nChn){};
    virtual ~CDetectObserver(AX_VOID) = default;

    AX_BOOL OnRecvData(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, AX_VOID* pData) override {
        if (nullptr == pData) {
            return AX_FALSE;
        }
        if (m_nChn == nChn) {
            CAXFrame* pFrame = (CAXFrame*)pData;
            return m_pSink->SendFrame(*pFrame);
        } else {
            return AX_TRUE;
        }
    }

    AX_BOOL OnRegisterObserver(OBS_TARGET_TYPE_E eTarget, AX_U32 nGrp, AX_U32 nChn, OBS_TRANS_ATTR_PTR pParams) override {
        return AX_TRUE;
    }

private:
    CDetector* m_pSink;
    AX_U32 m_nChn;
};