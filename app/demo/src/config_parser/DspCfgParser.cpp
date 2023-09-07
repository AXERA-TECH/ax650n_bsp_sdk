/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "DspCfgParser.h"
#include <fstream>
#include "AppLogApi.h"
#include "CommonUtils.hpp"

using namespace std;

#define DSP_PARSER "DSP_PARSER"

AX_BOOL CDspCfgParser::InitOnce() {
    string strConfigDir = CCommonUtils::GetPPLConfigDir();
    if (strConfigDir.empty()) {
        return AX_FALSE;
    }

    string strDspCfgFile = strConfigDir + "/dsp.json";

    ifstream ifs(strDspCfgFile.c_str());
    picojson::value v;
    ifs >> v;

    string err = picojson::get_last_error();
    if (!err.empty()) {
        LOG_M_E(DSP_PARSER, "Failed to load json config file: %s", strDspCfgFile.c_str());
        return AX_FALSE;
    }

    if (!v.is<picojson::object>()) {
        LOG_M_E(DSP_PARSER, "Loaded config file is not a well-formatted JSON.");
        return AX_FALSE;
    }

    return ParseFile(strDspCfgFile);
}

AX_BOOL CDspCfgParser::ParseFile(const string &strPath) {
    picojson::value v;
    ifstream fIn(strPath.c_str());
    if (!fIn.is_open()) {
        return AX_FALSE;
    }

    string strParseRet = picojson::parse(v, fIn);
    if (!strParseRet.empty() || !v.is<picojson::object>()) {
        return AX_FALSE;
    }

    return ParseJson(v.get<picojson::object>());
}

AX_BOOL CDspCfgParser::ParseJson(picojson::object &objJsonRoot) {
    for (AX_U32 i = 0; i < E_IPC_SCENARIO_MAX; i++) {
        string strScenario = CCmdLineParser::ScenarioEnum2Str((AX_IPC_SCENARIO_E)i);

        if (objJsonRoot.end() == objJsonRoot.find(strScenario.c_str())) {
            continue;
        }

        picojson::array &arrGrpSetting = objJsonRoot[strScenario.c_str()].get<picojson::array>();
        if (0 == arrGrpSetting.size()) {
            return AX_FALSE;
        }
        vector<DSP_ATTR_S> vecGrpSetting;
        for (size_t i = 0; i < arrGrpSetting.size(); i++) {
            DSP_ATTR_S tOutConfig;

            picojson::object objSetting = arrGrpSetting[i].get<picojson::object>();
            tOutConfig.nGrp = tOutConfig.nDspId = objSetting["grp_id"].get<double>();
            picojson::object objGroupInfo = objSetting["grp_info"].get<picojson::object>();

            tOutConfig.strSramPath = objGroupInfo["sram_bin_path"].get<std::string>();
            tOutConfig.strItcmPath = objGroupInfo["itcm_bin_path"].get<std::string>();
            tOutConfig.nDeepCnt = objGroupInfo["deep_cnt"].get<double>();
            picojson::object &objGrpResolution = objGroupInfo["dst_resolution"].get<picojson::object>();
            tOutConfig.nDstWidth = objGrpResolution["w"].get<double>();
            tOutConfig.nDstHeight = objGrpResolution["h"].get<double>();

            vecGrpSetting.emplace_back(tOutConfig);
        }

        m_mapScenario2GrpSetting.insert(make_pair(i, vecGrpSetting));
    }

    return m_mapScenario2GrpSetting.size() > 0 ? AX_TRUE : AX_FALSE;
}

AX_BOOL CDspCfgParser::GetDspConfig(AX_U8 nScenario, AX_U32 nGroup, DSP_ATTR_S &tOutConfig) {
    std::map<AX_U8, std::vector<DSP_ATTR_S>>::iterator it = m_mapScenario2GrpSetting.find(nScenario);
    if (m_mapScenario2GrpSetting.end() == it) {
        return AX_FALSE;
    }

    if (nGroup >= it->second.size()) {
        return AX_FALSE;
    }

    tOutConfig = it->second.at(nGroup);

    return AX_TRUE;
}
