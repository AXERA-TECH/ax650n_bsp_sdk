/**********************************************************************************
 *
 * Copyright (c) 2019-2020 Beijing AXera Technology Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Beijing AXera Technology Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Beijing AXera Technology Co., Ltd.
 *
 **********************************************************************************/

#pragma once

#include "AXSingleton.h"
#include "WebServer.h"
#include "picojson.h"

namespace AX_ITS {

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
    AX_BOOL GetUTCase(AX_U8 nScenario, std::vector<TESTSUITE_OPERATION_T>& vecOutConfig);
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
    AX_BOOL ParseVINJson(picojson::object& objJsonRoot, std::vector<TESTSUITE_OPERATION_T>& vecReq);
    AX_BOOL ParseVENCJson(picojson::object& objJsonRoot, std::vector<TESTSUITE_OPERATION_T>& vecReq);
    AX_BOOL ParseAIJson(picojson::object& objJsonRoot, std::vector<TESTSUITE_OPERATION_T>& vecReq);
    AX_BOOL ParseOSDJson(picojson::object& objJsonRoot, std::vector<TESTSUITE_OPERATION_T>& vecReq);

private:
    std::map<AX_U8, std::vector<TESTSUITE_OPERATION_T>> m_mapScenario2GrpSetting;
    AX_S32 m_nCurrScenario{0};
    std::map<AX_U8, APP_TEST_SUITE_CONFIG_T> m_mapTestCfg;
};

}  // namespace AX_ITS