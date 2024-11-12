// /**************************************************************************************************
//  *
//  * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
//  *
//  * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
//  * may not be copied or distributed in any isomorphic form without the prior
//  * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
//  *
//  **************************************************************************************************/
#include "AppLogApi.h"
#include "TestSuiteConfigParser.h"
#include "NVRConfigParser.h"
#include <fstream>

using namespace std;

#define TAG "TS_CFG_PARSER"


template <typename T>
static AX_BOOL GET_NUM_VALUE(const picojson::value &argObj, const string &name, T &v, AX_BOOL bRequired) {
    const picojson::value& obj = argObj.get(name);
    if (obj.is<double>()) {
        v = (T)obj.get<double>();
        return AX_TRUE;
    } else if (!bRequired) {
        v = (T)0;
        return AX_TRUE;
    }

    LOG_M_E(TAG, "Get value of key<%s> failed.", name.c_str());
    return AX_FALSE;
}

static AX_BOOL GET_STR_VALUE(const picojson::value &argObj, const string &name, string &val, AX_BOOL bRequired) {
    const picojson::value& obj = argObj.get(name);
    if (obj.is<string>()) {
        val = obj.get<string>();
        return AX_TRUE;
    } else if (!bRequired) {
        val = "";
        return AX_TRUE;
    }

    LOG_M_E(TAG, "Get value of key<%s> failed.", name.c_str());
    return AX_FALSE;
}

AX_NVR_TS_CONFIG_T CTestSuiteConfigParser::GetConfig() {
    std::string strPath;
    AX_NVR_TEST_SUITE_CONFIG_T tTsCfg = CNVRConfigParser::GetInstance()->GetTestSuiteConfig();
    if (AX_NVR_TS_RUN_MODE::STABILITY == tTsCfg.eMode) {
        strPath = tTsCfg.strStabilityPath;
    } else {
        strPath = tTsCfg.strUTPath;
    }

    AX_NVR_TS_CONFIG_T tConfig;

    std::vector<AX_NVR_TS_MODULE_INFO_T> vecModuleInfo;
    do {
        std::ifstream file(strPath);
        if (!file.is_open()) {
            LOG_MM_E(TAG, "Failed to open json config file: %s", strPath.c_str());
            break;
        } else {
            LOG_M_C(TAG, "Load ts config file: %s", strPath.c_str());
        }

        picojson::value json;
        file >> json;
        string err = picojson::get_last_error();
        if (!err.empty()) {
            LOG_M_E(TAG, "Failed to load json config file: %s", strPath.c_str());
            break;
        }

        if (!json.is<picojson::object>()) {
            LOG_M_E(TAG, "Loaded config file is not a well-formatted JSON.");
            break;
        } else { // parse remote device
            if (!GET_NUM_VALUE(json, "repeat", tConfig.bRepeat, AX_TRUE)) {
                break;
            }

            if (!GET_NUM_VALUE(json, "close_on_finish", tConfig.bCloseOnFinish, AX_TRUE)) {
                break;
            }

            if (!GET_NUM_VALUE(json, "module_count", tConfig.nModuleCount, AX_TRUE)) {
                break;
            }

            if (!GET_NUM_VALUE(json, "result_to_file", tConfig.bExportResultToFile, AX_TRUE)) {
                break;
            }

            if (!GET_STR_VALUE(json, "result_dir", tConfig.strResultDir, AX_TRUE)) {
                break;
            }

            const picojson::value& modules = json.get("modules");
            if (modules.is<picojson::array>()) {
                const picojson::array& arr_modules = modules.get<picojson::array>();
                if (tConfig.nModuleCount > arr_modules.size()) {
                    LOG_M_W(TAG, "Configured module count is larger than actually configured modules, replace the value automatically.");
                    tConfig.nModuleCount = arr_modules.size();
                }

                for (auto module : arr_modules) {
                    AX_NVR_TS_MODULE_INFO_T moduleInfo;
                    if (!GET_STR_VALUE(module, "name", moduleInfo.strName, AX_TRUE)) break;
                    if (!GET_STR_VALUE(module, "data_path", moduleInfo.strDataPath, AX_FALSE)) break;
                    if (!GET_STR_VALUE(module, "date", moduleInfo.strDate, AX_FALSE)) break;
                    if (!GET_NUM_VALUE(module, "enable", moduleInfo.bEnable, AX_TRUE)) break;
                    if (!GET_NUM_VALUE(module, "thread", moduleInfo.bThread, AX_FALSE)) break;
                    if (!GET_NUM_VALUE(module, "round_count", moduleInfo.nRoundCount, AX_FALSE)) break;
                    if (!GET_NUM_VALUE(module, "repeat_count", moduleInfo.nRepeatCount, AX_TRUE)) break;
                    if (!GET_NUM_VALUE(module, "case_interval", moduleInfo.nCaseInterval, AX_TRUE)) break;

                    const picojson::value& cases = module.get("cases");
                    if (cases.is<picojson::array>()) {
                        const picojson::array& arr_cases = cases.get<picojson::array>();
                        for (auto& m : arr_cases) {
                            AX_NVR_TS_CASE_INFO_T caseInfo;
                            if (!GET_STR_VALUE(m, "name", caseInfo.strName, AX_TRUE)) break;
                            if (!GET_NUM_VALUE(m, "repeat_count", caseInfo.nRepeatCount, AX_TRUE)) break;
                            if (!GET_NUM_VALUE(m, "operation_interval", caseInfo.nOprInterval, AX_TRUE)) break;

                            moduleInfo.vecCaseInfo.emplace_back(caseInfo);
                        }
                    }
                    vecModuleInfo.emplace_back(moduleInfo);
                }

                tConfig.vecModuleInfo = vecModuleInfo;
            }
        }
    } while (0);

    return tConfig;
}
