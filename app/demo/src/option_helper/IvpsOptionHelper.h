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

#include <map>
#include <vector>
#include "AXSingleton.h"
#include "IVPSGrpStage.h"

#define APP_IVPS_CONFIG(nScenario, nGroup, tOutConfig) \
        CIvpsOptionHelper::GetInstance()->GetIvpsConfig(nScenario, nGroup, tOutConfig)

/**
 * Load configuration
 */
class CIvpsOptionHelper final : public CAXSingleton<CIvpsOptionHelper> {
    friend class CAXSingleton<CIvpsOptionHelper>;

public:
    AX_BOOL GetIvpsConfig(AX_U8 nScenario, AX_U32 nGroup, IVPS_GROUP_CFG_T& tOutConfig);

private:
    CIvpsOptionHelper(AX_VOID) = default;
    ~CIvpsOptionHelper(AX_VOID) = default;

    AX_BOOL InitOnce() override;

private:
    std::map<AX_U8, std::vector<IVPS_GROUP_CFG_T>> m_mapScenario2GrpSetting;
};
