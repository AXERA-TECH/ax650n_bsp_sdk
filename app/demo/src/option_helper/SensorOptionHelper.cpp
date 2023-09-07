/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "SensorCfgParser.h"
#include "SensorOptionHelper.h"
#include "CommonUtils.hpp"

#define SNS_OPTION_HELPER "SNS_HELPER"

AX_BOOL CSensorOptionHelper::InitOnce() {
    return CSensorCfgParser::GetInstance()->GetConfig(m_mapSensorCfg, m_nCurrScenario, m_nSensorCount);
}

std::map<AX_U8, SENSOR_CONFIG_T>* CSensorOptionHelper::GetScenarioCfg(AX_S32 nScenario) {
    return &m_mapSensorCfg[nScenario];
}

AX_BOOL CSensorOptionHelper::GetSensorConfig(AX_U8 nSnsIndex, SENSOR_CONFIG_T& tOutSensorCfg) {
    std::map<AX_U8, SENSOR_CONFIG_T>* pMapScenario = GetScenarioCfg(m_nCurrScenario);
    if (nullptr == pMapScenario) {
        return AX_FALSE;
    }

    if (pMapScenario->find(nSnsIndex) == pMapScenario->end()) {
        return AX_FALSE;
    }

    tOutSensorCfg = pMapScenario->at(nSnsIndex);

    return AX_TRUE;
}

AX_BOOL CSensorOptionHelper::SwitchScenario(AX_IPC_SCENARIO_E eScenario) {
    std::map<AX_U8, SENSOR_CONFIG_T>* pMapScenario = GetScenarioCfg(eScenario);
    if (nullptr == pMapScenario) {
        return AX_FALSE;
    }

    m_nCurrScenario = eScenario;
    m_nSensorCount = pMapScenario->size();

    return AX_TRUE;
}

/* Provided for dual-sensor stitching */
AX_VOID CSensorOptionHelper::SetSensorCount(AX_U32 nCount) {
    m_nSensorCount = nCount;
}

AX_VOID CSensorOptionHelper::SetPanoMode(AX_U8 nMode) {
    m_nPanoMode = nMode;
}
