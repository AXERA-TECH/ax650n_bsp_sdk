/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include <fstream>
#include "PanoAVSCfgParser.h"
#include "AppLogApi.h"
#include "CommonUtils.hpp"

using namespace std;

#define AVS_PARSER "AVS_PARSER"

AX_BOOL CAVSCfgParser::InitOnce() {
    return AX_TRUE;
}

AX_BOOL CAVSCfgParser::GetConfig(AX_APP_AVS_CFG_T& stAVSCfg) {
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

    if (!ParseFile(strAVSCfgFile, stAVSCfg)) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CAVSCfgParser::ParseFile(const string& strPath, AX_APP_AVS_CFG_T& stAVSCfg) {
    picojson::value v;
    ifstream fIn(strPath.c_str());
    if (!fIn.is_open()) {
        return AX_TRUE;
    }

    string strParseRet = picojson::parse(v, fIn);
    if (!strParseRet.empty() || !v.is<picojson::object>()) {
        return AX_TRUE;
    }

    return ParseJson(v.get<picojson::object>(), stAVSCfg);
}

AX_BOOL CAVSCfgParser::ParseJson(picojson::object& objJsonRoot, AX_APP_AVS_CFG_T& stAVSCfg) {
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
        stAVSCfg.u8PipeNum = (AX_U8)objAVSGlbSetting["pipe_num"].get<double>();
        /* sync pipe flag */
        stAVSCfg.bSyncPipe = (AX_BOOL)objAVSGlbSetting["sync_pipe"].get<bool>();
        /* avs mode */
        stAVSCfg.u8Mode = (AX_U8)objAVSGlbSetting["mode"].get<double>();
        /* dynamic seam flag */
        stAVSCfg.bDynamicSeam = (AX_BOOL)objAVSGlbSetting["dynamic_seam"].get<bool>();

        /* blend mode */
        stAVSCfg.u8BlendMode = (AX_U8)objAVSGlbSetting["blend_mode"].get<double>();

        /* parameter file path */
        stAVSCfg.strParamFilePath = objAVSGlbSetting["param_file_path"].get<std::string>();

        /* parameter type */
        stAVSCfg.u8ParamType = objAVSGlbSetting["param_type"].get<double>();

        /* projection type */
        stAVSCfg.u8ProjectionType = objAVSGlbSetting["projection_type"].get<double>();

        /* calibration enable flag */
        stAVSCfg.u8CaliEnable = objAVSGlbSetting["cali_enable"].get<double>();

        /* calibration pc server ip address */
        stAVSCfg.strCaliServerIP = objAVSGlbSetting["cali_server_ip"].get<std::string>();

        /* calibration pc server port */
        stAVSCfg.u16CaliServerPort = objAVSGlbSetting["cali_server_port"].get<double>();

        picojson::array &arrPipeId = objAVSGlbSetting["pipe_id"].get<picojson::array>();
        for (size_t i = 0; i < arrPipeId.size(); i++) {
            stAVSCfg.arrPipeId[i] = arrPipeId[i].get<double>();
        }

        picojson::array &arrChnId = objAVSGlbSetting["chn_id"].get<picojson::array>();
        for (size_t i = 0; i < arrChnId.size(); i++) {
            stAVSCfg.arrChnId[i] = arrChnId[i].get<double>();
        }

        stAVSCfg.u8MasterPipeId = objAVSGlbSetting["master_pipe_id"].get<double>();

        objSetting = arrAVSSetting[1].get<picojson::object>();
        picojson::object objAVSGrpSetting = objSetting["avs_group_settings"].get<picojson::object>();
        stAVSCfg.u8GrpId = (AX_U8)objAVSGrpSetting["grp_id"].get<double>();
        if (objAVSGrpSetting.end() == objAVSGrpSetting.find("grp_info")) {
            bSucc = AX_FALSE;
            break;
        }

        picojson::object objAVSGrpInfo = objAVSGrpSetting["grp_info"].get<picojson::object>();
        stAVSCfg.u8ChnId = (AX_U8)objAVSGrpInfo["chn_id"].get<double>();
        picojson::array &arrPipeFrameRate = objAVSGrpInfo["pipe_frame_rate"].get<picojson::array>();
        for (size_t i = 0; i < arrPipeFrameRate.size(); i++) {
            stAVSCfg.arrPipeFrameRate[i] = arrPipeFrameRate[i].get<double>();
        }
        picojson::object &arrGrpFrameRate = objAVSGrpInfo["grp_framerate"].get<picojson::object>();
        stAVSCfg.arrGrpFrameRate[0] = arrGrpFrameRate["src"].get<double>();
        stAVSCfg.arrGrpFrameRate[1] = arrGrpFrameRate["dst"].get<double>();

        picojson::object &arrChnFrameRate = objAVSGrpInfo["chn_framerate"].get<picojson::object>();
        stAVSCfg.arrChnFrameRate[0] = arrChnFrameRate["src"].get<double>();
        stAVSCfg.arrChnFrameRate[1] = arrChnFrameRate["dst"].get<double>();

        picojson::array &arrPipeResolution = objAVSGrpInfo["pipe_resolution"].get<picojson::array>();
        for (size_t i = 0; i < arrPipeResolution.size(); i++) {
            stAVSCfg.arrPipeResolution[i][0] = arrPipeResolution[i].get<picojson::object>()["w"].get<double>();
            stAVSCfg.arrPipeResolution[i][1] = arrPipeResolution[i].get<picojson::object>()["h"].get<double>();
        }

        picojson::object &arrGrpResolution = objAVSGrpInfo["grp_resolution"].get<picojson::object>();
        stAVSCfg.arrGrpResolution[0] = arrGrpResolution["w"].get<double>();
        stAVSCfg.arrGrpResolution[1] = arrGrpResolution["h"].get<double>();

        picojson::object &arrChnResolution = objAVSGrpInfo["chn_resolution"].get<picojson::object>();
        stAVSCfg.arrChnResolution[0] = arrChnResolution["w"].get<double>();
        stAVSCfg.arrChnResolution[1] = arrChnResolution["h"].get<double>();

        picojson::array &arrPipeLinkFlag = objAVSGrpInfo["pipe_link_flag"].get<picojson::array>();
        for (size_t i = 0; i < arrPipeLinkFlag.size(); i++) {
            stAVSCfg.arrPipeLinkFlag[i] =  arrPipeLinkFlag[i].get<double>();
        }

        picojson::object &arrGrpFBC = objAVSGrpInfo["grp_fbc"].get<picojson::object>();
        stAVSCfg.arrGrpFBC[0] = arrGrpFBC["mode"].get<double>();
        stAVSCfg.arrGrpFBC[1] = arrGrpFBC["level"].get<double>();

        picojson::object &arrChnFBC = objAVSGrpInfo["chn_fbc"].get<picojson::object>();
        stAVSCfg.arrChnFBC[0] = arrChnFBC["mode"].get<double>();
        stAVSCfg.arrChnFBC[1] = arrChnFBC["level"].get<double>();

        stAVSCfg.bChnInplace = (AX_BOOL)objAVSGrpInfo["chn_inplace"].get<double>();
    } while(0);

   return bSucc;
}