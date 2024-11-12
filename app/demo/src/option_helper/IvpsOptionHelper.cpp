/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "IvpsCfgParser.h"
#include "IvpsOptionHelper.h"
#include "CommonUtils.hpp"

AX_BOOL CIvpsOptionHelper::InitOnce() {
    return CIvpsCfgParser::GetInstance()->GetConfig(m_mapScenario2GrpSetting);
}

AX_BOOL CIvpsOptionHelper::GetIvpsConfig(AX_U8 nScenario, AX_U32 nGroup, IVPS_GROUP_CFG_T& tOutConfig) {
    std::map<AX_U8, std::vector<IVPS_GROUP_CFG_T>>::iterator it = m_mapScenario2GrpSetting.find(nScenario);
    if (m_mapScenario2GrpSetting.end() == it) {
        return AX_FALSE;
    }

    if (nGroup >= it->second.size()) {
        return AX_FALSE;
    }

    tOutConfig = it->second.at(nGroup);

    return AX_TRUE;
}
