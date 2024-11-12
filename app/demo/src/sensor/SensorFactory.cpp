/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "SensorFactory.hpp"
#include <exception>
#include "OS04a10.h"
#include "OS08a20.h"
#include "OS08b10.h"
#include "SC910gs.h"
ISensor *CSensorFactory::CreateSensor(const SENSOR_CONFIG_T &tSensorCfg) {
    ISensor *pSnsInstance{nullptr};
    switch (tSensorCfg.eSensorType) {
        case E_SNS_TYPE_OS04A10:
        case E_SNS_TYPE_OS04A10_LF:
        case E_SNS_TYPE_OS04A10_SF:
        case E_SNS_TYPE_OS04A10_DUAL_PANO: {
            pSnsInstance = new COS04a10(tSensorCfg);
            break;
        }
        case E_SNS_TYPE_OS08A20:
        case E_SNS_TYPE_OS08A20_LF:
        case E_SNS_TYPE_OS08A20_SF: {
            pSnsInstance = new COS08a20(tSensorCfg);
            break;
        }
        case E_SNS_TYPE_OS08B10:
            pSnsInstance = new COS08b10(tSensorCfg);
            break;
        case E_SNS_TYPE_SC910GS:
            pSnsInstance = new CSC910gs(tSensorCfg);
            break;
        default: {
            return nullptr;
        }
    }

    return pSnsInstance;
}

AX_VOID CSensorFactory::DestorySensor(ISensor *&pSensor) {
    if (pSensor) {
        ((CBaseSensor *)pSensor)->DeInit();
        delete pSensor;
        pSensor = nullptr;
    }
}