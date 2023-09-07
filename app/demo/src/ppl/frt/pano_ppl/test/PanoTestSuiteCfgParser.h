/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#pragma once

#include "AXSingleton.h"
#include "WebServer.h"
#include "picojson.h"

namespace AX_PANO {
struct TESTSUITE_OPERATION_T : public WEB_REQ_OPERATION_T {
    std::string strDesc;
};

typedef struct _APP_TEST_SUITE_CONFIG_T {
    AX_S64 nLoopNum{0};
    AX_U32 nDefIntervalMs{0};
    AX_BOOL bRandomEnable{AX_FALSE};
} APP_TEST_SUITE_CONFIG_T;

class CTestSuiteCfgParser : public CAXSingleton<CTestSuiteCfgParser> {
    friend class CAXSingleton<CTestSuiteCfgParser>;

public:
    AX_BOOL GetUTCase(AX_U8 nScenario, std::vector<WEB_REQ_OPERATION_T>& vecOutConfig);
    AX_BOOL GetTestAttr(AX_U8 nScenario, APP_TEST_SUITE_CONFIG_T& tTestCfg);
    AX_S32 GetCurrScenario() {
        return m_nCurrScenario;
    };

private:
    CTestSuiteCfgParser(AX_VOID) = default;
    ~CTestSuiteCfgParser(AX_VOID) = default;

    AX_BOOL InitOnce() override;

    AX_BOOL ParseFile(const std::string& strPath);
    AX_BOOL ParseJson(picojson::object& objJsonRoot);

private:
    std::map<AX_U8, std::vector<WEB_REQ_OPERATION_T>> m_mapScenario2GrpSetting;
    AX_S32 m_nCurrScenario{0};
    std::map<AX_U8, APP_TEST_SUITE_CONFIG_T> m_mapTestCfg;
};

}