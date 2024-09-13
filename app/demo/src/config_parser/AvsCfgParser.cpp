/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include <fstream>
#include "AvsCfgParser.h"
#include "AppLogApi.h"
#include "CommonUtils.hpp"

using namespace std;

#define AVS_PARSER "AVS_PARSER"

AX_BOOL CAvsCfgParser::InitOnce() {
    return AX_TRUE;
}

AX_BOOL CAvsCfgParser::GetConfig(AX_APP_AVS_CFG_T& stAvsCfg) {
    string strConfigDir = CCommonUtils::GetPPLConfigDir();
    if (strConfigDir.empty()) {
        return AX_FALSE;
    }

    string strAVSCfgFile = strConfigDir + "/avs.json";

    ifstream ifs(strAVSCfgFile.c_str());
    picojson::value v;
    ifs >> v;

    string err = picojson::get_last_error();
    if (!err.empty()) {
        LOG_M_E(AVS_PARSER, "Failed to load json config file: %s", strAVSCfgFile.c_str());
        return AX_FALSE;
    }

    if (!v.is<picojson::object>()) {
        LOG_M_E(AVS_PARSER, "Loaded config file is not a well-formatted JSON.");
        return AX_FALSE;
    }

    if (!ParseFile(strAVSCfgFile, stAvsCfg)) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CAvsCfgParser::ParseFile(const string& strPath, AX_APP_AVS_CFG_T& stAvsCfg) {
    picojson::value v;
    ifstream fIn(strPath.c_str());
    if (!fIn.is_open()) {
        return AX_TRUE;
    }

    string strParseRet = picojson::parse(v, fIn);
    if (!strParseRet.empty() || !v.is<picojson::object>()) {
        return AX_TRUE;
    }

    return ParseJson(v.get<picojson::object>(), stAvsCfg);
}

AX_BOOL CAvsCfgParser::ParseJson(picojson::object& objJsonRoot, AX_APP_AVS_CFG_T& stAvsCfg) {
    AX_BOOL bSucc = AX_TRUE;

    do {
        if (objJsonRoot.end() == objJsonRoot.find("avs_settings")) {
            bSucc = AX_FALSE;
            break;
        }

        picojson::array& arrAVSSetting = objJsonRoot["avs_settings"].get<picojson::array>();
        if (0 == arrAVSSetting.size()) {
            bSucc = AX_FALSE;
            break;
        }

        picojson::object objSetting = arrAVSSetting[0].get<picojson::object>();
        picojson::object objAVSGlbSetting = objSetting["avs_global_settings"].get<picojson::object>();
         /* pipe num */
        stAvsCfg.u8PipeNum = (AX_U8)objAVSGlbSetting["pipe_num"].get<double>();
        /* sync pipe flag */
        stAvsCfg.bSyncPipe = (AX_BOOL)objAVSGlbSetting["sync_pipe"].get<bool>();
        /* avs mode */
        stAvsCfg.u8Mode = (AX_U8)objAVSGlbSetting["mode"].get<double>();
        /* dynamic seam flag */
        stAvsCfg.bDynamicSeam = (AX_BOOL)objAVSGlbSetting["dynamic_seam"].get<bool>();

        /* blend mode */
        stAvsCfg.u8BlendMode = (AX_U8)objAVSGlbSetting["blend_mode"].get<double>();

        /* parameter file path */
        stAvsCfg.strParamFilePath = objAVSGlbSetting["param_file_path"].get<std::string>();

        /* parameter type */
        stAvsCfg.u8ParamType = objAVSGlbSetting["param_type"].get<double>();

        /* projection type */
        stAvsCfg.u8ProjectionType = objAVSGlbSetting["projection_type"].get<double>();

        /* compress info */
        stAvsCfg.stAvsCompress.enCompressMode =
            (AX_COMPRESS_MODE_E)objAVSGlbSetting["avs_compress"].get<picojson::array>()[0].get<double>();

        stAvsCfg.stAvsCompress.u32CompressLevel =
            objAVSGlbSetting["avs_compress"].get<picojson::array>()[1].get<double>();

        /* calibration enable flag */
        stAvsCfg.u8CaliEnable = objAVSGlbSetting["cali_enable"].get<double>();

        /* calibration pc server ip address */
        stAvsCfg.strCaliServerIP = objAVSGlbSetting["cali_server_ip"].get<std::string>();

        /* calibration pc server port */
        stAvsCfg.u16CaliServerPort = objAVSGlbSetting["cali_server_port"].get<double>();
    } while(0);

   return bSucc;
}