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
#include "picojson.h"

class CIvpsCfgParser : public CAXSingleton<CIvpsCfgParser> {
    friend class CAXSingleton<CIvpsCfgParser>;

public:
    AX_BOOL GetConfig(std::map<AX_U8, std::vector<IVPS_GROUP_CFG_T>>& mapScenario2GrpSetting);

private:
    CIvpsCfgParser(AX_VOID) = default;
    ~CIvpsCfgParser(AX_VOID) = default;

    AX_BOOL InitOnce() override;

    AX_BOOL ParseFile(const std::string& strPath,
                        std::map<AX_U8, std::vector<IVPS_GROUP_CFG_T>>& mapScenario2GrpSetting);
    AX_BOOL ParseJson(picojson::object& objJsonRoot,
                        std::map<AX_U8, std::vector<IVPS_GROUP_CFG_T>>& mapScenario2GrpSetting);
    AX_IVPS_ENGINE_E Str2Engine(std::string strEngine);
};
