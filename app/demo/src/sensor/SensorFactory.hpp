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
#include "ISensor.hpp"

class CSensorFactory : public CAXSingleton<CSensorFactory> {
    friend class CAXSingleton<CSensorFactory>;

public:
    ISensor *CreateSensor(const SENSOR_CONFIG_T &tSensorCfg);
    AX_VOID DestorySensor(ISensor *&pSensor);

public:
    CSensorFactory(AX_VOID) noexcept = default;
    virtual ~CSensorFactory(AX_VOID) = default;
};