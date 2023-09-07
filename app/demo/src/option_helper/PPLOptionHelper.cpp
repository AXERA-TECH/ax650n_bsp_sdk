/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "PPLCfgParser.h"
#include "PPLOptionHelper.h"
#include "CommonUtils.hpp"

AX_BOOL CPPLOptionHelper::InitOnce() {
    return CPPLCfgParser::GetInstance()->GetConfig(m_mapScenario2Relations);
}

AX_BOOL CPPLOptionHelper::GetModRelations(AX_U8 nScenario, std::vector<IPC_MOD_RELATIONSHIP_T>& vecModRelations) {
    std::map<AX_U8, std::vector<IPC_MOD_RELATIONSHIP_T>>::iterator it = m_mapScenario2Relations.find(nScenario);
    if (m_mapScenario2Relations.end() == it) {
        return AX_FALSE;
    }

    vecModRelations = it->second;

    return AX_TRUE;
}
