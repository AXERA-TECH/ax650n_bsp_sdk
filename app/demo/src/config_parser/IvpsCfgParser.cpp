/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "IvpsCfgParser.h"
#include <fstream>
#include "AppLogApi.h"
#include "CommonUtils.hpp"

using namespace std;

#define IVPS_PARSER "IVPS_PARSER"

AX_BOOL CIvpsCfgParser::InitOnce() {
    return AX_TRUE;
}

AX_BOOL CIvpsCfgParser::GetConfig(std::map<AX_U8, std::vector<IVPS_GROUP_CFG_T>> &mapScenario2GrpSetting) {
    string strConfigDir = CCommonUtils::GetPPLConfigDir();
    if (strConfigDir.empty()) {
        return AX_FALSE;
    }

    string strIvpsCfgFile = strConfigDir + "/ivps.json";

    ifstream ifs(strIvpsCfgFile.c_str());
    picojson::value v;
    ifs >> v;

    string err = picojson::get_last_error();
    if (!err.empty()) {
        LOG_M_E(IVPS_PARSER, "Failed to load json config file: %s", strIvpsCfgFile.c_str());
        return AX_FALSE;
    }

    if (!v.is<picojson::object>()) {
        LOG_M_E(IVPS_PARSER, "Loaded config file is not a well-formatted JSON.");
        return AX_FALSE;
    }

    return ParseFile(strIvpsCfgFile, mapScenario2GrpSetting);
}

AX_BOOL CIvpsCfgParser::ParseFile(const string &strPath, std::map<AX_U8, std::vector<IVPS_GROUP_CFG_T>> &mapScenario2GrpSetting) {
    picojson::value v;
    ifstream fIn(strPath.c_str());
    if (!fIn.is_open()) {
        return AX_FALSE;
    }

    string strParseRet = picojson::parse(v, fIn);
    if (!strParseRet.empty() || !v.is<picojson::object>()) {
        return AX_FALSE;
    }

    return ParseJson(v.get<picojson::object>(), mapScenario2GrpSetting);
}

AX_BOOL CIvpsCfgParser::ParseJson(picojson::object &objJsonRoot, std::map<AX_U8, std::vector<IVPS_GROUP_CFG_T>> &mapScenario2GrpSetting) {
    for (AX_U32 i = 0; i < E_IPC_SCENARIO_MAX; i++) {
        string strScenario = CCmdLineParser::ScenarioEnum2Str((AX_IPC_SCENARIO_E)i);

        if (objJsonRoot.end() == objJsonRoot.find(strScenario.c_str())) {
            continue;
        }

        picojson::array &arrGrpSetting = objJsonRoot[strScenario.c_str()].get<picojson::array>();
        if (0 == arrGrpSetting.size()) {
            return AX_FALSE;
        }

        vector<IVPS_GROUP_CFG_T> vecGrpSetting;
        for (size_t i = 0; i < arrGrpSetting.size(); i++) {
            IVPS_GROUP_CFG_T tOutConfig;

            picojson::object objSetting = arrGrpSetting[i].get<picojson::object>();
            tOutConfig.nGrp = objSetting["grp_id"].get<double>();

            picojson::object objGroupInfo = objSetting["grp_info"].get<picojson::object>();
            tOutConfig.nGrpChnNum = objGroupInfo["grp_chn_num"].get<double>();
            tOutConfig.eGrpEngineType0 = Str2Engine(objGroupInfo["engine_filter_0"].get<std::string>());
            tOutConfig.eGrpEngineType1 = Str2Engine(objGroupInfo["engine_filter_1"].get<std::string>());

            picojson::array &arrEngineType2 = objGroupInfo["engine_filter_2"].get<picojson::array>();
            for (size_t i = 0; i < arrEngineType2.size(); i++) {
                tOutConfig.arrChnEngineType0[i] = Str2Engine(arrEngineType2[i].get<std::string>());
            }

            picojson::array &arrEngineType3 = objGroupInfo["engine_filter_3"].get<picojson::array>();
            for (size_t i = 0; i < arrEngineType3.size(); i++) {
                tOutConfig.arrChnEngineType1[i] = Str2Engine(arrEngineType3[i].get<std::string>());
            }

            picojson::object &objGrpFramerate = objGroupInfo["grp_framerate"].get<picojson::object>();
            tOutConfig.arrGrpFramerate[0] = objGrpFramerate["src"].get<double>();
            tOutConfig.arrGrpFramerate[1] = objGrpFramerate["dst"].get<double>();

            picojson::array &arrChnFramerate = objGroupInfo["chn_framerate"].get<picojson::array>();
            for (size_t i = 0; i < arrChnFramerate.size(); i++) {
                tOutConfig.arrChnFramerate[i][0] = arrChnFramerate[i].get<picojson::object>()["src"].get<double>();
                tOutConfig.arrChnFramerate[i][1] = arrChnFramerate[i].get<picojson::object>()["dst"].get<double>();
            }

            picojson::object &objGrpResolution = objGroupInfo["grp_resolution"].get<picojson::object>();
            tOutConfig.arrGrpResolution[0] = objGrpResolution["w"].get<double>();
            tOutConfig.arrGrpResolution[1] = objGrpResolution["h"].get<double>();

            picojson::array &arrChnResolution = objGroupInfo["chn_resolution"].get<picojson::array>();
            for (size_t i = 0; i < arrChnResolution.size(); i++) {
                tOutConfig.arrChnResolution[i][0] = arrChnResolution[i].get<picojson::object>()["w"].get<double>();
                tOutConfig.arrChnResolution[i][1] = arrChnResolution[i].get<picojson::object>()["h"].get<double>();
            }

            picojson::array &arrChnLinkFlag = objGroupInfo["chn_link_flag"].get<picojson::array>();
            for (size_t i = 0; i < arrChnLinkFlag.size(); i++) {
                tOutConfig.arrChnLinkFlag[i] = arrChnLinkFlag[i].get<double>();
            }

            if (objGroupInfo.end() != objGroupInfo.find("grp_fbc")) {
                picojson::object &objGrpFBC = objGroupInfo["grp_fbc"].get<picojson::object>();
                tOutConfig.arrGrpFBC[0] = objGrpFBC["mode"].get<double>();
                tOutConfig.arrGrpFBC[1] = objGrpFBC["level"].get<double>();
            }
            picojson::array &arrChnFBC = objGroupInfo["chn_fbc"].get<picojson::array>();
            for (size_t i = 0; i < arrChnFBC.size(); i++) {
                tOutConfig.arrChnFBC[i][0] = arrChnFBC[i].get<picojson::object>()["mode"].get<double>();
                tOutConfig.arrChnFBC[i][1] = arrChnFBC[i].get<picojson::object>()["level"].get<double>();
            }

            picojson::array &arrChnInplace = objGroupInfo["chn_inplace"].get<picojson::array>();
            for (size_t i = 0; i < arrChnInplace.size(); i++) {
                AX_U32 nInplace = arrChnInplace[i].get<double>();
                tOutConfig.bAarrChnInplace[i] = nInplace ? AX_TRUE : AX_FALSE;
            }

            if (objGroupInfo.end() != objGroupInfo.find("spec_engine_config")) {
                picojson::object &objEngineCfg = objGroupInfo["spec_engine_config"].get<picojson::object>();
                if (objEngineCfg.end() != objEngineCfg.find("rotation")) {
                    tOutConfig.nRotation = objEngineCfg["rotation"].get<double>();
                    tOutConfig.bRotationEngine = AX_TRUE;
                }
                tOutConfig.nMirror = objEngineCfg["mirror"].get<double>();
                tOutConfig.nFlip = objEngineCfg["flip"].get<double>();

                if (objEngineCfg.end() != objEngineCfg.find("ldc_enable") && objEngineCfg.end() != objEngineCfg.find("ldc_aspect") &&
                    objEngineCfg.end() != objEngineCfg.find("ldc_x_ratio") && objEngineCfg.end() != objEngineCfg.find("ldc_y_ratio") &&
                    objEngineCfg.end() != objEngineCfg.find("ldc_xy_ratio") &&
                    objEngineCfg.end() != objEngineCfg.find("ldc_distor_ratio")) {
                    tOutConfig.nLdcEnable = objEngineCfg["ldc_enable"].get<double>();
                    tOutConfig.bLdcAspect = objEngineCfg["ldc_aspect"].get<double>() ? AX_TRUE : AX_FALSE;
                    tOutConfig.nLdcXRatio = objEngineCfg["ldc_x_ratio"].get<double>();
                    tOutConfig.nLdcYRatio = objEngineCfg["ldc_y_ratio"].get<double>();
                    tOutConfig.nLdcXYRatio = objEngineCfg["ldc_xy_ratio"].get<double>();
                    tOutConfig.nLdcDistortionRatio = objEngineCfg["ldc_distor_ratio"].get<double>();
                }
            }

            vecGrpSetting.emplace_back(tOutConfig);
        }

        mapScenario2GrpSetting.insert(make_pair(i, vecGrpSetting));
    }

    return mapScenario2GrpSetting.size() > 0 ? AX_TRUE : AX_FALSE;
}

AX_IVPS_ENGINE_E CIvpsCfgParser::Str2Engine(string strEngine) {
    if (strEngine == "SUB") {
        return AX_IVPS_ENGINE_SUBSIDIARY;
    } else if (strEngine == "TDP") {
        return AX_IVPS_ENGINE_TDP;
    } else if (strEngine == "GDC") {
        return AX_IVPS_ENGINE_GDC;
    } else if (strEngine == "VPP") {
        return AX_IVPS_ENGINE_VPP;
    } else if (strEngine == "VGP") {
        return AX_IVPS_ENGINE_VGP;
    } else if (strEngine == "IVE") {
        return AX_IVPS_ENGINE_IVE;
    } else if (strEngine == "VO") {
        return AX_IVPS_ENGINE_VO;
    } else if (strEngine == "DSP") {
        return AX_IVPS_ENGINE_DSP;
    }

    return AX_IVPS_ENGINE_BUTT;
}
