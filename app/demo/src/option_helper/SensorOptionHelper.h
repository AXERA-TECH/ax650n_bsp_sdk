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

typedef enum {
    E_WEB_SHOW_SENSOR_MODE_SINGLE = 0,
    E_WEB_SHOW_SENSOR_MODE_DUAL,
    E_WEB_SHOW_SENSOR_MODE_PANO_SINGLE,
    E_WEB_SHOW_SENSOR_MODE_PANO_DUAL,
    E_WEB_SHOW_SENSOR_MODE_MAX
} WEB_SHOW_SENSOR_MODE_E;

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
#define SET_APP_WEB_SHOW_SENSOR_MODE(_Mode_) \
            CSensorOptionHelper::GetInstance()->SetWebShowSnsMode(_Mode_)
#define SET_APP_WEB_PANO_SENSOR_ID(_PanoSnsId_) \
            CSensorOptionHelper::GetInstance()->SetPanoSensorId(_PanoSnsId_)
#define APP_WEB_SHOW_SENSOR_MODE() \
        CSensorOptionHelper::GetInstance()->GetWebShowSnsMode()
#define APP_WEB_SHOW_SENSOR_COUNT() \
        CSensorOptionHelper::GetInstance()->GetWebShowSnsCount()
#define APP_WEB_PANO_SENSOR_ID() \
        CSensorOptionHelper::GetInstance()->GetPanoSensorId()

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

    WEB_SHOW_SENSOR_MODE_E GetWebShowSnsMode() {
        return m_eWebShowSnsMode;
    };

    AX_U32 GetWebShowSnsCount();

    AX_VOID SetSensorCount(AX_U32 nCount);
    AX_VOID SetWebShowSnsMode(WEB_SHOW_SENSOR_MODE_E eWebShowSnsMode);

    AX_BOOL SwitchScenario(AX_IPC_SCENARIO_E eScenario);
    AX_S32 GetCurrScenario() {
        return m_nCurrScenario;
    };

    AX_VOID SetPanoSensorId(AX_S32 nPanoSnsId);
    AX_S32 GetPanoSensorId() {
        return m_nPanoSensorId;
    };


private:
    CSensorOptionHelper(AX_VOID) = default;
    ~CSensorOptionHelper(AX_VOID) = default;

    AX_BOOL InitOnce() override;

    std::map<AX_U8, SENSOR_CONFIG_T>* GetScenarioCfg(AX_S32 nScenario);

private:
    AX_S32 m_nCurrScenario{0};
    AX_U32 m_nSensorCount{0};
    AX_S32 m_nPanoSensorId{-1};
    WEB_SHOW_SENSOR_MODE_E m_eWebShowSnsMode{E_WEB_SHOW_SENSOR_MODE_SINGLE};

    std::map<AX_U8, std::map<AX_U8, SENSOR_CONFIG_T>> m_mapSensorCfg;
};
