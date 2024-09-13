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

#include "AXSingleton.h"
#include "DspStage.h"
#include "picojson.h"

class CDspCfgParser : public CAXSingleton<CDspCfgParser> {
    friend class CAXSingleton<CDspCfgParser>;

public:
    AX_BOOL GetDspConfig(AX_U8 nScenario, AX_U32 nGroup, DSP_ATTR_S& tOutConfig);

private:
    CDspCfgParser(AX_VOID) = default;
    ~CDspCfgParser(AX_VOID) = default;

    AX_BOOL InitOnce() override;
    AX_BOOL ParseFile(const std::string& strPath);
    AX_BOOL ParseJson(picojson::object& objJsonRoot);

private:
    std::map<AX_U8, std::vector<DSP_ATTR_S>> m_mapScenario2GrpSetting;
};
