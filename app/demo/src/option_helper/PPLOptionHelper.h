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

#define APP_PPL_MOD_RELATIONS(nScenario, vecModRelations) \
        CPPLOptionHelper::GetInstance()->GetModRelations(nScenario, vecModRelations)

/**
 * Load configuration
 */
class CPPLOptionHelper final : public CAXSingleton<CPPLOptionHelper> {
    friend class CAXSingleton<CPPLOptionHelper>;

public:
    AX_BOOL GetModRelations(AX_U8 nScenario, std::vector<IPC_MOD_RELATIONSHIP_T>& vecModRelations);

private:
    CPPLOptionHelper(AX_VOID) = default;
    ~CPPLOptionHelper(AX_VOID) = default;

    AX_BOOL InitOnce() override;

private:
    std::map<AX_U8, std::vector<IPC_MOD_RELATIONSHIP_T>> m_mapScenario2Relations;
};
