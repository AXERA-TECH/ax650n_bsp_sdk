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
#include "IniWrapper.hpp"
#include "AXNVRFrameworkDefine.h"

#include <string>

class CNVRConfigParser : public CAXSingleton<CNVRConfigParser> {
    friend class CAXSingleton<CNVRConfigParser>;

public:
    AX_BOOL InitOnce(AX_VOID) override;

    AX_NVR_RPATROL_CONFIG_T GetRoundPatrolConfig(AX_VOID);
    AX_VOID SetRoundPatrolConfig(AX_NVR_RPATROL_CONFIG_T &conf);
    AX_VOID SetTestSuiteRunMode(AX_NVR_TS_RUN_MODE eMode) { m_eTestSuiteMode = eMode; };

    AX_NVR_DETECT_CONFIG_T GetDetectConfig(AX_VOID);
    AX_NVR_DISPVO_CONFIG_T GetPrimaryDispConfig(AX_VOID);
    AX_NVR_DISPVO_CONFIG_T GetSecondaryDispConfig(AX_VOID);
    AX_NVR_RECORD_CONFIG_T GetRecordConfig(AX_VOID);
    AX_NVR_DEVICE_CONFIG_T GetDeviceConfig(AX_VOID);
    AX_NVR_DATA_STREAM_CONFIG_T GetDataStreamConfig(AX_VOID);
    AX_NVR_TEST_SUITE_CONFIG_T GetTestSuiteConfig(AX_VOID);
    AX_NVR_FBC_CONFIG_T GetFBCConfig(AX_VOID);

private:
    std::string GetExecPath(AX_VOID);

private:
    CIniWrapper m_iniConfig;
    AX_NVR_TS_RUN_MODE m_eTestSuiteMode {AX_NVR_TS_RUN_MODE::DISABLE};

private:
    CNVRConfigParser(AX_VOID) = default;
    ~CNVRConfigParser(AX_VOID) = default;
};
