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
#include "EncoderCfgParser.h"
#include "AXTypeConverter.hpp"
#include "AppLogApi.h"
#include "CommonUtils.hpp"

using namespace std;

#define ENC_MAX_BUF_SIZE (3840 * 2160 * 3 / 2)
#define ENC_PARSER "ENC_PARSER"

AX_BOOL CEncoderCfgParser::InitOnce() {
    return AX_TRUE;
}

AX_BOOL CEncoderCfgParser::GetConfig(std::map<AX_U8, std::map<AX_U8, VIDEO_CONFIG_T>>& mapScenario2VencChnConfig,
                                        std::map<AX_U8, std::map<AX_U8, JPEG_CONFIG_T>>& mapScenario2JencChnConfig) {
    string strConfigDir = CCommonUtils::GetPPLConfigDir();
    if (strConfigDir.empty()) {
        return AX_FALSE;
    }

    string strEncoderCfgFile = strConfigDir + "/encoder.json";

    ifstream ifs(strEncoderCfgFile.c_str());
    picojson::value v;
    ifs >> v;

    string err = picojson::get_last_error();
    if (!err.empty()) {
        LOG_M_E(ENC_PARSER, "Failed to load json config file: %s", strEncoderCfgFile.c_str());
        return AX_FALSE;
    }

    if (!v.is<picojson::object>()) {
        LOG_M_E(ENC_PARSER, "Loaded config file is not a well-formatted JSON.");
        return AX_FALSE;
    }

    if (!ParseFile(strEncoderCfgFile, mapScenario2VencChnConfig, mapScenario2JencChnConfig)) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CEncoderCfgParser::ParseFile(const string& strPath,
                                        std::map<AX_U8, std::map<AX_U8, VIDEO_CONFIG_T>>& mapScenario2VencChnConfig,
                                        std::map<AX_U8, std::map<AX_U8, JPEG_CONFIG_T>>& mapScenario2JencChnConfig) {
    picojson::value v;
    ifstream fIn(strPath.c_str());
    if (!fIn.is_open()) {
        return AX_FALSE;
    }

    string strParseRet = picojson::parse(v, fIn);
    if (!strParseRet.empty() || !v.is<picojson::object>()) {
        return AX_FALSE;
    }

    return ParseJson(v.get<picojson::object>(), mapScenario2VencChnConfig, mapScenario2JencChnConfig);
}

AX_BOOL CEncoderCfgParser::ParseJson(picojson::object& objJsonRoot,
                                        std::map<AX_U8, std::map<AX_U8, VIDEO_CONFIG_T>>& mapScenario2VencChnConfig,
                                        std::map<AX_U8, std::map<AX_U8, JPEG_CONFIG_T>>& mapScenario2JencChnConfig) {
    for (AX_U32 i = 0; i < E_IPC_SCENARIO_MAX; i++) {
        AX_U8 nVencCnt = 0, nJencCnt = 0;
        string strScenario = CCmdLineParser::ScenarioEnum2Str((AX_IPC_SCENARIO_E)i);
        if (objJsonRoot.end() == objJsonRoot.find(strScenario.c_str())) {
            continue;
        }

        std::vector<APP_ENC_RC_CONFIG> vecEncRc;
        /* Parse VENC settings */
        picojson::object &objScenario = objJsonRoot[strScenario.c_str()].get<picojson::object>();
        picojson::array &arrChnSettings = objScenario["instance"].get<picojson::array>();
        picojson::array &rcInfoSettings = objScenario["enc_rc_info"].get<picojson::array>();
        {
            for (size_t k = 0; k < rcInfoSettings.size(); k++) {
                picojson::object &objEncSetting = rcInfoSettings[k].get<picojson::object>();
                AX_U32 nEncodeType = objEncSetting["type"].get<double>();
                APP_ENC_RC_CONFIG stEncRc;
                stEncRc.ePayloadType = CAXTypeConverter::Int2EncoderType(nEncodeType);
                picojson::array &arrRc = rcInfoSettings[k].get<picojson::object>()["rc"].get<picojson::array>();
                for (size_t index = 0; index < arrRc.size(); index++) {
                    picojson::object objRcInfo = arrRc[index].get<picojson::object>();
                    AX_U32 nRcType = objRcInfo["type"].get<double>();
                    stEncRc.stRCInfo[index].eRcType = CAXTypeConverter::FormatRcMode(nEncodeType, nRcType);
                    stEncRc.stRCInfo[index].nMinQp = objRcInfo["min_qp"].get<double>();
                    stEncRc.stRCInfo[index].nMaxQp = objRcInfo["max_qp"].get<double>();
                    if (2 == nRcType) {
                        /*fixqp*/
                        stEncRc.stRCInfo[index].nFixQp = objRcInfo["fixed_qp"].get<double>();
                    } else if (0 == nRcType) {
                        /*cbr*/
                        stEncRc.stRCInfo[index].nMinQp = objRcInfo["min_qp"].get<double>();
                        stEncRc.stRCInfo[index].nMaxQp = objRcInfo["max_qp"].get<double>();
                        stEncRc.stRCInfo[index].nMinIQp = objRcInfo["min_iqp"].get<double>();
                        stEncRc.stRCInfo[index].nMaxIQp = objRcInfo["max_iqp"].get<double>();
                        stEncRc.stRCInfo[index].nMinIProp = objRcInfo["min_i_prop"].get<double>();
                        stEncRc.stRCInfo[index].nMaxIProp = objRcInfo["max_i_prop"].get<double>();
                        stEncRc.stRCInfo[index].nIntraQpDelta = objRcInfo["intra_qp_delta"].get<double>();
                    } else if (1 == nRcType) {
                        /*vbr*/
                        stEncRc.stRCInfo[index].nMinQp = objRcInfo["min_qp"].get<double>();
                        stEncRc.stRCInfo[index].nMaxQp = objRcInfo["max_qp"].get<double>();
                        stEncRc.stRCInfo[index].nMinIQp = objRcInfo["min_iqp"].get<double>();
                        stEncRc.stRCInfo[index].nMaxIQp = objRcInfo["max_iqp"].get<double>();
                        stEncRc.stRCInfo[index].nIntraQpDelta = objRcInfo["intra_qp_delta"].get<double>();
                    } else {
                        LOG_MM_E(ENC_PARSER, "rcType unsupport.");
                    }
                }

                vecEncRc.emplace_back(stEncRc);
            }
        }
        if (arrChnSettings.size() > 0) {
            for (size_t j = 0; j < arrChnSettings.size(); j++) {
                picojson::object objSetting = arrChnSettings[j].get<picojson::object>();

                VIDEO_CONFIG_T tOutConfig;
                tOutConfig.nChannel = objSetting["channel"].get<double>();
                AX_U32 nEncodeType = objSetting["encoder_type"].get<double>();
                tOutConfig.ePayloadType = CAXTypeConverter::Int2EncoderType(nEncodeType);
                tOutConfig.nBufSize = ENC_MAX_BUF_SIZE;
                if (3 == nEncodeType) {
                    /* Parse JENC settings */
                    JPEG_CONFIG_T tOutConfig;
                    tOutConfig.nChannel = objSetting["channel"].get<double>();
                    tOutConfig.nPipeSrc = objSetting["pipe_src"].get<double>();
                    tOutConfig.nJpegType = objSetting["jpeg_type"].get<double>();
                    tOutConfig.nInFifoDepth = objSetting["in_fifo_depth"].get<double>();
                    tOutConfig.nOutFifoDepth = objSetting["out_fifo_depth"].get<double>();
                    tOutConfig.eMemSource = (AX_MEMORY_SOURCE_E)objSetting["mem_source"].get<double>();
                    tOutConfig.nQpLevel = objSetting["qp_level"].get<double>();
                    tOutConfig.nDstFrameRate = objSetting["frame_rate"].get<double>();
                    tOutConfig.nBufSize = ENC_MAX_BUF_SIZE;
                    mapScenario2JencChnConfig[i][nJencCnt++] = tOutConfig;
                    continue;
                }

                tOutConfig.nPipeSrc = objSetting["pipe_src"].get<double>();
                tOutConfig.nInFifoDepth = objSetting["in_fifo_depth"].get<double>();
                tOutConfig.nOutFifoDepth = objSetting["out_fifo_depth"].get<double>();
                tOutConfig.eMemSource = (AX_MEMORY_SOURCE_E)objSetting["mem_source"].get<double>();
                tOutConfig.nGOP = objSetting["gop"].get<double>();
                tOutConfig.nBitrate = objSetting["bitrate"].get<double>();

                AX_U32 nRcType = objSetting["rc_type"].get<double>();
                tOutConfig.eRcType = CAXTypeConverter::FormatRcMode(nEncodeType, nRcType);
                for (size_t k = 0; k < vecEncRc.size(); k++) {
                    tOutConfig.stEncodeCfg[k] = vecEncRc[k];
                }

                mapScenario2VencChnConfig[i][nVencCnt++] = tOutConfig;
            }
        }
    }

    return mapScenario2VencChnConfig.size() + mapScenario2JencChnConfig.size() > 0 ? AX_TRUE : AX_FALSE;
}
