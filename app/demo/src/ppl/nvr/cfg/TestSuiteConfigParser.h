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
#include "picojson.h"

#include "AXSingleton.h"
#include "AXNVRFrameworkDefine.h"


typedef struct AX_NVR_TS_CASE_INFO {
    std::string strName;
    AX_U32 nRepeatCount;
    AX_U32 nOprInterval; /* Interval between two operations, Unit: milliseconds */
} AX_NVR_TS_CASE_INFO_T, *AX_NVR_TS_CASE_INFO_PTR;

typedef struct AX_NVR_TS_MODULE_INFO {
    std::string strName;
    std::string strDataPath;
    std::string strDate;
    AX_BOOL bEnable;
    AX_BOOL bRandom;
    AX_BOOL bThread;
    AX_S32 nRoundCount;
    AX_U32 nRepeatCount;
    AX_U32 nCaseInterval; /* Interval between two cases, Unit: milliseconds */
    vector<AX_NVR_TS_CASE_INFO_T> vecCaseInfo;
} AX_NVR_TS_MODULE_INFO_T, *AX_NVR_TS_MODULE_INFO_PTR;

typedef struct AX_NVR_TS_CONFIG {
    AX_BOOL bRepeat;
    AX_BOOL bCloseOnFinish;
    AX_U32  nModuleCount;
    AX_BOOL bExportResultToFile;
    std::string strResultDir;
    vector<AX_NVR_TS_MODULE_INFO_T> vecModuleInfo;

    AX_NVR_TS_CONFIG() {
        bRepeat = AX_FALSE;
        bCloseOnFinish = AX_TRUE;
        bExportResultToFile = AX_FALSE;
        nModuleCount = 0;
        vecModuleInfo.clear();
    }
} AX_NVR_TS_CONFIG_T, *AX_NVR_TS_CONFIG_PTR;


class CTestSuiteConfigParser : public CAXSingleton<CTestSuiteConfigParser> {
    friend class CAXSingleton<CTestSuiteConfigParser>;

public:
    AX_NVR_TS_CONFIG_T GetConfig();

private:
    CTestSuiteConfigParser(AX_VOID) = default;
    ~CTestSuiteConfigParser(AX_VOID) = default;
};
