/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include "PanoTestSuiteCfgParser.h"
#include <fstream>
#include "AppLogApi.h"
#include "CommonUtils.hpp"

#define TEST_PARSER "TEST_PARSER"
using namespace std;
using namespace AX_PANO;

AX_BOOL CTestSuiteCfgParser::InitOnce() {
    string strConfigDir = CCommonUtils::GetPPLConfigDir();
    if (strConfigDir.empty()) {
        return AX_FALSE;
    }

    string strUTCfgFile = strConfigDir + "/testsuite.json";

    ifstream ifs(strUTCfgFile.c_str());
    picojson::value v;
    ifs >> v;

    string err = picojson::get_last_error();
    if (!err.empty()) {
        LOG_M_E(TEST_PARSER, "Failed to load json config file: %s", strUTCfgFile.c_str());
        return AX_FALSE;
    }

    if (!v.is<picojson::object>()) {
        LOG_M_E(TEST_PARSER, "Loaded config file is not a well-formatted JSON.");
        return AX_FALSE;
    }

    return ParseFile(strUTCfgFile);
}

AX_BOOL CTestSuiteCfgParser::ParseFile(const std::string &strPath) {
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
AX_BOOL CTestSuiteCfgParser::ParseJson(picojson::object &objJsonRoot) {
    picojson::array arrGrpSetting;
    for (AX_U32 i = 0; i < E_IPC_SCENARIO_MAX; i++) {
        vector<WEB_REQ_OPERATION_T> vecReq;
        WEB_REQ_OPERATION_T tOperation;
        string strScenario = CCmdLineParser::ScenarioEnum2Str((AX_IPC_SCENARIO_E)i);
        if (objJsonRoot.end() == objJsonRoot.find(strScenario.c_str())) {
            continue;
        }
        m_nCurrScenario = i;
        APP_TEST_SUITE_CONFIG_T t_TestConfig;
        picojson::object objSetting = objJsonRoot[strScenario.c_str()].get<picojson::object>();
        picojson::object moduleSetting;
        if (objSetting.find("LoopNum") == objSetting.end() || objSetting.find("DefIntervalMs") == objSetting.end() ||
            objSetting.find("RandomEnable") == objSetting.end()) {
            continue;
        }

        t_TestConfig.nLoopNum = objSetting["LoopNum"].get<double>();
        t_TestConfig.nDefIntervalMs = objSetting["DefIntervalMs"].get<double>();
        t_TestConfig.bRandomEnable = (AX_BOOL)objSetting["RandomEnable"].get<double>();
        m_mapTestCfg[m_nCurrScenario] = t_TestConfig;
        if (objSetting.find("VIN") != objSetting.end()) {
            arrGrpSetting = objSetting["VIN"].get<picojson::array>();
            if (arrGrpSetting.size()) {
                for (AX_U32 j = 0; j < arrGrpSetting.size(); j++) {
                    moduleSetting = arrGrpSetting[j].get<picojson::object>();
                    if (moduleSetting.find("sns_id") == moduleSetting.end()) {
                        continue;
                    }
                    tOperation.eReqType = E_REQ_TYPE_CAMERA;
                    AX_U32 snsID = moduleSetting["sns_id"].get<double>();
                    if (moduleSetting.find("capture_enable") != moduleSetting.end()) {
                        AX_U32 captureEnable = moduleSetting["capture_enable"].get<double>();
                        tOperation.SetOperaType(E_WEB_OPERATION_TYPE_CAPTURE_AUTO);
                        tOperation.nGroup = snsID;
                        tOperation.tCapEnable.bOn = (AX_BOOL)captureEnable;
                        if (moduleSetting.find("nIntervalMs") != moduleSetting.end()) {
                            AX_U32 intervalMs = moduleSetting["nIntervalMs"].get<double>();
                            tOperation.nIntervalMs = intervalMs;
                        }
                        vecReq.emplace_back(tOperation);
                        continue;
                    }

                    /* todo: not support dynamic swiching framerate for yet*/
                    if (moduleSetting.find("pipe_framerate") != moduleSetting.end()) {
                        // AX_F32 pipeFramerate = moduleSetting["pipe_framerate"].get<double>();
                        tOperation.SetOperaType(E_WEB_OPERATION_TYPE_MAX);
                        tOperation.nGroup = snsID;
                        if (moduleSetting.find("nIntervalMs") != moduleSetting.end()) {
                            AX_U32 intervalMs = moduleSetting["nIntervalMs"].get<double>();
                            tOperation.nIntervalMs = intervalMs;
                        }
                        // vecReq.emplace_back(tOperation);
                        continue;
                    }
                    /* todo: not support dynamic swiching daynight_mode for yet*/
                    if (moduleSetting.find("daynight_mode") != moduleSetting.end()) {
                        // AX_U32 daynightMode = moduleSetting["daynight_mode"].get<double>();
                        tOperation.SetOperaType(E_WEB_OPERATION_TYPE_MAX);
                        tOperation.nGroup = snsID;
                        if (moduleSetting.find("nIntervalMs") != moduleSetting.end()) {
                            AX_U32 intervalMs = moduleSetting["nIntervalMs"].get<double>();
                            tOperation.nIntervalMs = intervalMs;
                        }
                        // vecReq.emplace_back(tOperation);
                        continue;
                    }
                }
            }
        }

        if (objSetting.find("VENC") != objSetting.end()) {
            arrGrpSetting = objSetting["VENC"].get<picojson::array>();
            if (0 == arrGrpSetting.size()) {
                return AX_FALSE;
            }
            for (AX_U32 j = 0; j < arrGrpSetting.size(); j++) {
                moduleSetting = arrGrpSetting[j].get<picojson::object>();
                if (moduleSetting.find("sns_id") == moduleSetting.end()) {
                    continue;
                }
                if (moduleSetting.find("channel") == moduleSetting.end()) {
                    continue;
                }
                tOperation.eReqType = E_REQ_TYPE_VIDEO;
                AX_U32 snsID = moduleSetting["sns_id"].get<double>();
                AX_U32 channel = moduleSetting["channel"].get<double>();

                if (moduleSetting.find("encoder_type") != moduleSetting.end()) {
                    AX_U32 encodeType = moduleSetting["encoder_type"].get<double>();
                    tOperation.nGroup = snsID;
                    tOperation.nChannel = channel;
                    tOperation.SetOperaType(E_WEB_OPERATION_TYPE_ENC_TYPE);
                    tOperation.tEncType.nEncoderType = encodeType;
                    if (moduleSetting.find("nIntervalMs") != moduleSetting.end()) {
                        AX_U32 intervalMs = moduleSetting["nIntervalMs"].get<double>();
                        tOperation.nIntervalMs = intervalMs;
                    }
                    vecReq.emplace_back(tOperation);
                    continue;
                }
                if (moduleSetting.find("encoder_type") != moduleSetting.end() && moduleSetting.find("bitrate") != moduleSetting.end()) {
                    AX_U32 encodeType = moduleSetting["encoder_type"].get<double>();
                    AX_U32 bitrate = moduleSetting["bitrate"].get<double>();
                    tOperation.nGroup = snsID;
                    tOperation.nChannel = channel;
                    tOperation.SetOperaType(E_WEB_OPERATION_TYPE_BITRATE);
                    tOperation.tBitrate.nEncoderType = encodeType;
                    tOperation.tBitrate.nBitrate = bitrate;
                    if (moduleSetting.find("nIntervalMs") != moduleSetting.end()) {
                        AX_U32 intervalMs = moduleSetting["nIntervalMs"].get<double>();
                        tOperation.nIntervalMs = intervalMs;
                    }
                    vecReq.emplace_back(tOperation);
                    continue;
                }
                if (moduleSetting.find("width") != moduleSetting.end() && moduleSetting.find("height") != moduleSetting.end()) {
                    AX_U32 width = moduleSetting["width"].get<double>();
                    AX_U32 height = moduleSetting["height"].get<double>();
                    tOperation.nGroup = snsID;
                    tOperation.nChannel = channel;
                    tOperation.SetOperaType(E_WEB_OPERATION_TYPE_RESOLUTION);
                    tOperation.tResolution.nWidth = width;
                    tOperation.tResolution.nHeight = height;
                    if (moduleSetting.find("nIntervalMs") != moduleSetting.end()) {
                        AX_U32 intervalMs = moduleSetting["nIntervalMs"].get<double>();
                        tOperation.nIntervalMs = intervalMs;
                    }
                    vecReq.emplace_back(tOperation);
                    continue;
                }
            }
        }

        if (objSetting.find("OSD") != objSetting.end()) {
            arrGrpSetting = objSetting["OSD"].get<picojson::array>();
            if (0 == arrGrpSetting.size()) {
                return AX_FALSE;
            }

            for (AX_U32 j = 0; j < arrGrpSetting.size(); j++) {
                moduleSetting = arrGrpSetting[j].get<picojson::object>();
                if (moduleSetting.find("sns_id") == moduleSetting.end()) {
                    continue;
                }

                tOperation.eReqType = E_REQ_TYPE_OSD;
                AX_U32 snsID = moduleSetting["sns_id"].get<double>();
                if (moduleSetting.find("enable") != moduleSetting.end()) {
                    AX_U32 enable = moduleSetting["enable"].get<double>();
                    tOperation.nGroup = snsID;
                    tOperation.SetOperaType(E_WEB_OPERATION_TYPE_OSD_ENABLE);
                    tOperation.tOsdEnable.bOn = (AX_BOOL)enable;
                    if (moduleSetting.find("nIntervalMs") != moduleSetting.end()) {
                        AX_U32 intervalMs = moduleSetting["nIntervalMs"].get<double>();
                        tOperation.nIntervalMs = intervalMs;
                    }
                    vecReq.emplace_back(tOperation);
                    continue;
                }
            }
        }

        if (objSetting.find("AI") != objSetting.end()) {
            arrGrpSetting = objSetting["AI"].get<picojson::array>();
            if (0 == arrGrpSetting.size()) {
                return AX_FALSE;
            }
            for (AX_U32 j = 0; j < arrGrpSetting.size(); j++) {
                moduleSetting = arrGrpSetting[j].get<picojson::object>();
                if (moduleSetting.find("sns_id") == moduleSetting.end()) {
                    continue;
                }
                tOperation.eReqType = E_REQ_TYPE_AI;
                AX_U32 snsID = moduleSetting["sns_id"].get<double>();
                if (moduleSetting.find("enable") != moduleSetting.end()) {
                    AX_U32 enable = moduleSetting["enable"].get<double>();
                    tOperation.nGroup = snsID;
                    tOperation.SetOperaType(E_WEB_OPERATION_TYPE_AI_ENABLE);
                    tOperation.tOsdEnable.bOn = (AX_BOOL)enable;
                    if (moduleSetting.find("nIntervalMs") != moduleSetting.end()) {
                        AX_U32 intervalMs = moduleSetting["nIntervalMs"].get<double>();
                        tOperation.nIntervalMs = intervalMs;
                    }
                    vecReq.emplace_back(tOperation);
                    continue;
                }
            }
        }
        m_mapScenario2GrpSetting[m_nCurrScenario] = vecReq;
    }

    return m_mapScenario2GrpSetting.size() > 0 ? AX_TRUE : AX_FALSE;
}

AX_BOOL CTestSuiteCfgParser::GetUTCase(AX_U8 nScenario, std::vector<WEB_REQ_OPERATION_T> &vecOutConfig) {
    auto it = m_mapScenario2GrpSetting.find(nScenario);
    if (m_mapScenario2GrpSetting.end() == it) {
        return AX_FALSE;
    }
    vecOutConfig = it->second;
    return AX_TRUE;
}

AX_BOOL CTestSuiteCfgParser::GetTestAttr(AX_U8 nScenario, APP_TEST_SUITE_CONFIG_T &tTestCfg) {
    auto it = m_mapTestCfg.find(nScenario);
    if (m_mapTestCfg.end() == it) {
        return AX_FALSE;
    }
    tTestCfg = it->second;
    return AX_TRUE;
}