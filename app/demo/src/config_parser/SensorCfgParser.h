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

#include "picojson.h"
#include "AXSingleton.h"
#include "ISensor.hpp"

class CSensorCfgParser : public CAXSingleton<CSensorCfgParser> {
    friend class CAXSingleton<CSensorCfgParser>;

public:
    AX_BOOL GetConfig(std::map<AX_U8, std::map<AX_U8, SENSOR_CONFIG_T>>& mapSensorCfg,
                        AX_S32& nCurrScenario, AX_U32& nSensorCount);

private:
    CSensorCfgParser(AX_VOID) = default;
    ~CSensorCfgParser(AX_VOID) = default;

    AX_BOOL InitOnce() override;

    AX_BOOL ParseFile(const std::string& strPath,
                        std::map<AX_U8, std::map<AX_U8, SENSOR_CONFIG_T>>& mapSensorCfg);
    AX_BOOL ParseJson(picojson::object& objJsonRoot,
                        std::map<AX_U8, std::map<AX_U8, SENSOR_CONFIG_T>>& mapSensorCfg);
    std::string LoadType2FileName(AX_S32 nLoadType);
};
