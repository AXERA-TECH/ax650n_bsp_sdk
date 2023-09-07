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
#include "AXSingleton.h"
#include "IPPLBuilder.h"
#include "ISensor.hpp"

#define APP_SENSOR_CONFIG(nSnsIndex, tOutConfig) \
        CSensorOptionHelper::GetInstance()->GetSensorConfig(nSnsIndex, tOutConfig)
#define APP_SENSOR_COUNT() \
        CSensorOptionHelper::GetInstance()->GetSensorCount()
#define APP_CURR_SCENARIO() \
        CSensorOptionHelper::GetInstance()->GetCurrScenario()
#define SET_APP_SENSOR_COUNT(_Count_) \
            CSensorOptionHelper::GetInstance()->SetSensorCount(_Count_)
#define SET_APP_CURR_SCENARIO(_Scenario_) \
                CSensorOptionHelper::GetInstance()->SwitchScenario(_Scenario_)
#define SET_APP_PANO_MODE(_Mode_) \
            CSensorOptionHelper::GetInstance()->SetPanoMode(_Mode_)
#define APP_PANO_MODE() \
        CSensorOptionHelper::GetInstance()->GetPanoMode()

/**
 * Load configuration
 */
class CSensorOptionHelper final : public CAXSingleton<CSensorOptionHelper> {
    friend class CAXSingleton<CSensorOptionHelper>;

public:
    AX_BOOL GetSensorConfig(AX_U8 nSnsIndex, SENSOR_CONFIG_T& tOutSensorCfg);

    AX_U32 GetSensorCount() {
        return m_nSensorCount;
    };

    AX_U8 GetPanoMode() {
        return m_nPanoMode;
    };

    AX_VOID SetSensorCount(AX_U32 nCount);
    AX_VOID SetPanoMode(AX_U8 nMode);

    AX_BOOL SwitchScenario(AX_IPC_SCENARIO_E eScenario);
    AX_S32 GetCurrScenario() {
        return m_nCurrScenario;
    };

private:
    CSensorOptionHelper(AX_VOID) = default;
    ~CSensorOptionHelper(AX_VOID) = default;

    AX_BOOL InitOnce() override;

    std::map<AX_U8, SENSOR_CONFIG_T>* GetScenarioCfg(AX_S32 nScenario);

private:
    AX_S32 m_nCurrScenario{0};
    AX_U32 m_nSensorCount{0};
    AX_U8 m_nPanoMode{0};

    std::map<AX_U8, std::map<AX_U8, SENSOR_CONFIG_T>> m_mapSensorCfg;
};
