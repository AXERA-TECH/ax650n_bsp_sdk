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
#include "BaseSensor.h"
#include "IModule.h"
#include "IVPSGrpStage.h"
#include "IniWrapper.hpp"
#include "OsdConfig.h"
#include "VideoEncoder.h"
#include "ax_base_type.h"
namespace AX_ITS {
class CITSSPEC {
    CITSSPEC() = default;
    virtual ~CITSSPEC(AX_VOID) = default;

public:
    static AX_BOOL Init();
    static AX_VOID VideoChnIndex2IvpsGrp();

private:
    std::map<AX_U8, OSD_SENSOR_CONFIG_T> m_mapSns2OsdConfig;
};
};  // namespace AX_ITS