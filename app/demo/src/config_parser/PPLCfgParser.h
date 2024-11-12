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
#include "IPPLBuilder.h"
#include "picojson.h"

class CPPLCfgParser : public CAXSingleton<CPPLCfgParser> {
    friend class CAXSingleton<CPPLCfgParser>;

public:
    AX_BOOL GetConfig(std::map<AX_U8, std::vector<IPC_MOD_RELATIONSHIP_T>>& mapScenario2Relations);

private:
    CPPLCfgParser(AX_VOID) = default;
    ~CPPLCfgParser(AX_VOID) = default;

    AX_BOOL InitOnce() override;

    AX_BOOL ParseFile(const std::string& strPath,
                        std::map<AX_U8, std::vector<IPC_MOD_RELATIONSHIP_T>>& mapScenario2Relations);
    AX_BOOL ParseJson(picojson::object& objJsonRoot,
                        std::map<AX_U8, std::vector<IPC_MOD_RELATIONSHIP_T>>& mapScenario2Relations);
    PPL_MODULE_TYPE_E Str2Module(std::string strModule);
};
