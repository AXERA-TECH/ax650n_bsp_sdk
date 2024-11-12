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

#include <string>
#include "AXSingleton.h"
#include "IniWrapper.hpp"

/**
 * Load configuration
 */
class COptionHelper final : public CAXSingleton<COptionHelper> {
    friend class CAXSingleton<COptionHelper>;

public:
    AX_U8 GetRotation();
    AX_F32 GetVencOutBuffRatio();
    AX_F32 GetJencOutBuffRatio();
    AX_U32 GetAencOutFrmSize();
    AX_U32 GetRTSPMaxFrmSize();
    AX_S32 GetNpuMode();
    AX_BOOL ISEnableMp4Record();
    AX_BOOL IsEnableOSD();
    std::string GetMp4SavedPath();
    /* SLT functions */
    AX_U32 GetSLTRunTime();
    AX_U32 GetSLTFpsCheckFreq();
    AX_U32 GetSLTFpsDiff();

    AX_U32 GetSetVencThreadNum();

private:
    COptionHelper(AX_VOID) = default;
    ~COptionHelper(AX_VOID) = default;

    AX_BOOL InitOnce() override;

    std::string GetValue(const string strKey);

private:
    CIniWrapper m_iniWrapper;
};
