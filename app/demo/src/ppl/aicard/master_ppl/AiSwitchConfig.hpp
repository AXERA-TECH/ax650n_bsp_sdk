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
#include <vector>
#include "AXSingleton.h"
#include "IniWrapper.hpp"
#include "PcieAdapter.hpp"

namespace aicard_mst {

#define MAX_SWITCH_CONFIG_NUM (10)

typedef enum {
    AI_CARD_AI_SWITCH_TYPE_ORDERED = 0,
    AI_CARD_AI_SWITCH_TYPE_RANDOM,
    AI_CARD_AI_SWITCH_TYPE_MAX
} AI_CARD_AI_SWITCH_TYPE_E;

typedef struct {
    AX_BOOL bEnable;
    AI_CARD_AI_SWITCH_TYPE_E eType;
    AX_U32 nInterval;
    AX_U8  nAttrCount;
} AI_CARD_AI_SWITCH_STRATEGY_T;

/**
 * @brief
 *
 */
class CAiSwitchConfig : public CAXSingleton<CAiSwitchConfig> {
    friend class CAXSingleton<CAiSwitchConfig>;

public:
    AX_BOOL Init(AX_VOID);
    AX_BOOL IsEnabled();
    AX_S8   GetAttrCount();
    AX_BOOL GetNextAttr(AI_CARD_AI_SWITCH_ATTR_T& tAttr);
    AX_S32  GetInterval();

private:
    CAiSwitchConfig(AX_VOID) = default;
    virtual ~CAiSwitchConfig(AX_VOID) = default;

    AX_BOOL ParseConfig();
    AX_S8   GetNextAttrIndex();

    string  GetExecPath(AX_VOID);

private:
    CIniWrapper m_iniParser;
    AI_CARD_AI_SWITCH_STRATEGY_T m_tAiSwitchStrgy;
    AI_CARD_AI_SWITCH_ATTR_T m_arrSwitchAttrConfig[MAX_SWITCH_CONFIG_NUM];
    AX_U8 m_nCurrAttrIndex {0};
};

}  // namespace aicard_mst
