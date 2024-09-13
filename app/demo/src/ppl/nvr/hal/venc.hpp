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
#include <memory>
#include <mutex>
#include "AXFrame.hpp"
#include "VideoEncoderEx.hpp"
#include "haltype.hpp"


#define INVALID_VENC_GRP (-1)

class CVENC {
public:
    CVENC(AX_VOID) = default;

    AX_BOOL Init(CONST VENC_ATTR_T& stAttr);
    AX_BOOL DeInit(AX_VOID);

    AX_BOOL Start(AX_VOID);
    AX_BOOL Stop(AX_VOID);

    AX_BOOL RegisterObserver(IVencPackObserver* obs);
    AX_VOID UnRegisterObserver(AX_VOID);

private:
    std::unique_ptr<CVideoEncoderEx> m_venc;
    std::mutex m_mtxObs;
    IVencPackObserver* m_obs = {nullptr};
    AX_S32 m_veChn = {-1};
};
