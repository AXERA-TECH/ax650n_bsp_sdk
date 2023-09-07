#include "ITSTestSuiteCfgParser.h"
#include <fstream>
#include "AXTypeConverter.hpp"
#include "AppLogApi.h"
#include "CommonUtils.hpp"
#define TEST_PARSER "TEST_PARSER"
#define SEARCHKEY(obj, key) (obj.find(key) != obj.end())

using namespace std;
using namespace AX_ITS;

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

AX_BOOL CTestSuiteCfgParser::ParseFile(const std::string& strPath) {
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

AX_BOOL CTestSuiteCfgParser::ParseJson(picojson::object& objJsonRoot) {
    picojson::array arrGrpSetting;
    picojson::object moduleSetting;

    for (AX_U32 i = 0; i < E_IPC_SCENARIO_MAX; i++) {
        vector<TESTSUITE_OPERATION_T> vecReq;
        string strScenario = CCmdLineParser::ScenarioEnum2Str((AX_IPC_SCENARIO_E)i);
        if (objJsonRoot.end() == objJsonRoot.find(strScenario.c_str())) {
            continue;
        }
        m_nCurrScenario = i;
        APP_TEST_SUITE_CONFIG_T t_TestConfig;
        picojson::object objSetting = objJsonRoot[strScenario.c_str()].get<picojson::object>();
        if (SEARCHKEY(objSetting, "LoopNum") && SEARCHKEY(objSetting, "DefIntervalMs")) {
            t_TestConfig.nLoopNum = objSetting["LoopNum"].get<double>();

            t_TestConfig.nDefIntervalMs = objSetting["DefIntervalMs"].get<double>();
            t_TestConfig.bRandomEnable = (AX_BOOL)objSetting["RandomEnable"].get<double>();

            m_mapTestCfg[m_nCurrScenario] = t_TestConfig;
            if (SEARCHKEY(objSetting, "VIN")) {
                ParseVINJson(objSetting, vecReq);
            }
            if (SEARCHKEY(objSetting, "VENC")) {
                ParseVENCJson(objSetting, vecReq);
            }

            if (SEARCHKEY(objSetting, "AI")) {
                ParseAIJson(objSetting, vecReq);
            }

            if (SEARCHKEY(objSetting, "OSD")) {
                ParseOSDJson(objSetting, vecReq);
            }
            m_mapScenario2GrpSetting[m_nCurrScenario] = vecReq;
        }
    }

    return m_mapScenario2GrpSetting.size() > 0 ? AX_TRUE : AX_FALSE;
}

AX_BOOL CTestSuiteCfgParser::ParseVINJson(picojson::object& objSetting, std::vector<TESTSUITE_OPERATION_T>& vecReq) {
    picojson::array arrGrpSns;
    picojson::array arrOpt;
    picojson::object moduleSetting, operObj;
    TESTSUITE_OPERATION_T tOperation;
    AX_U32 nSnsID = 0;
    std::string strOperDes;
    moduleSetting = objSetting["VIN"].get<picojson::object>();
    tOperation.eReqType = E_REQ_TYPE_CAMERA;

    if (SEARCHKEY(moduleSetting, "captureSwitch")) {
        AX_U32 nSize = moduleSetting["captureSwitch"].get<picojson::array>().size();

        for (AX_U32 index = 0; index < nSize; index++) {
            operObj = moduleSetting["captureSwitch"].get<picojson::array>()[index].get<picojson::object>();
            if (SEARCHKEY(operObj, "sns_id_option") && SEARCHKEY(operObj, "enable_option")) {
                if (SEARCHKEY(operObj, "description")) {
                    strOperDes = tOperation.strDesc = operObj["description"].get<std::string>();
                }
                arrGrpSns = operObj["sns_id_option"].get<picojson::array>();
                for (AX_U32 j = 0; j < arrGrpSns.size(); j++) {
                    nSnsID = arrGrpSns[j].get<double>();
                    arrOpt = operObj["enable_option"].get<picojson::array>();
                    for (AX_U32 index = 0; index < arrOpt.size(); index++) {
                        AX_U32 captureEnable = arrOpt[index].get<double>();
                        tOperation.SetOperaType(E_WEB_OPERATION_TYPE_CAPTURE_AUTO);
                        tOperation.nSnsID = nSnsID;
                        tOperation.tCapEnable.bOn = (AX_BOOL)captureEnable;
                        vecReq.emplace_back(tOperation);
                        LOG_MM_D(TEST_PARSER, "operaDesc:%s", strOperDes.c_str());
                    }
                }
            }
        }
    }

    if (SEARCHKEY(moduleSetting, "daynight")) {
        AX_U32 nSize = moduleSetting["daynight"].get<picojson::array>().size();
        for (AX_U32 index = 0; index < nSize; index++) {
            operObj = moduleSetting["daynight"].get<picojson::array>()[index].get<picojson::object>();
            if (SEARCHKEY(operObj, "sns_id_option") && SEARCHKEY(operObj, "daynight_mode_option")) {
                if (SEARCHKEY(operObj, "description")) {
                    strOperDes = tOperation.strDesc = operObj["description"].get<std::string>();
                }
                arrGrpSns = operObj["sns_id_option"].get<picojson::array>();
                for (AX_U32 j = 0; j < arrGrpSns.size(); j++) {
                    nSnsID = arrGrpSns[j].get<double>();
                    arrOpt = operObj["daynight_mode_option"].get<picojson::array>();
                    for (AX_U32 index = 0; index < arrOpt.size(); index++) {
                        AX_U32 nDayNight = arrOpt[index].get<double>();
                        tOperation.SetOperaType(E_WEB_OPERATION_TYPE_DAYNIGHT);
                        tOperation.nSnsID = nSnsID;
                        tOperation.tDaynight.nDayNightMode = nDayNight;
                        vecReq.emplace_back(tOperation);
                        LOG_MM_D(TEST_PARSER, "operaDesc:%s", strOperDes.c_str());
                    }
                }
            }
        }
    }

    if (SEARCHKEY(moduleSetting, "frameRate")) {
        AX_U32 nSize = moduleSetting["frameRate"].get<picojson::array>().size();
        for (AX_U32 index = 0; index < nSize; index++) {
            operObj = moduleSetting["frameRate"].get<picojson::array>()[index].get<picojson::object>();
            if (SEARCHKEY(operObj, "sns_id_option") && SEARCHKEY(operObj, "framerate_option")) {
                if (SEARCHKEY(operObj, "description")) {
                    strOperDes = tOperation.strDesc = operObj["description"].get<std::string>();
                }
                arrGrpSns = operObj["sns_id_option"].get<picojson::array>();
                for (AX_U32 j = 0; j < arrGrpSns.size(); j++) {
                    nSnsID = arrGrpSns[j].get<double>();
                    arrOpt = operObj["framerate_option"].get<picojson::array>();
                    for (AX_U32 index = 0; index < arrOpt.size(); index++) {
                        AX_U32 nFrameRate = arrOpt[index].get<double>();
                        tOperation.SetOperaType(E_WEB_OPERATION_TYPE_CAMERA_FPS);
                        tOperation.nSnsID = nSnsID;
                        tOperation.tSnsFpsAttr.fSnsFrameRate = nFrameRate;
                        vecReq.emplace_back(tOperation);
                        LOG_MM_D(TEST_PARSER, "operaDesc:%s,nSnsID:%d, frameRate:%f", strOperDes.c_str(), nSnsID,
                                 tOperation.tSnsFpsAttr.fSnsFrameRate);
                    }
                }
            }
        }
    }

    if (SEARCHKEY(moduleSetting, "imageAttr")) {
        AX_U32 nSize = moduleSetting["imageAttr"].get<picojson::array>().size();
        for (AX_U32 index = 0; index < nSize; index++) {
            operObj = moduleSetting["imageAttr"].get<picojson::array>()[index].get<picojson::object>();
            if (SEARCHKEY(operObj, "sns_id_option") && SEARCHKEY(operObj, "auto_mode")) {
                if (SEARCHKEY(operObj, "description")) {
                    strOperDes = tOperation.strDesc = operObj["description"].get<std::string>();
                }
                arrGrpSns = operObj["sns_id_option"].get<picojson::array>();
                for (AX_U32 j = 0; j < arrGrpSns.size(); j++) {
                    nSnsID = arrGrpSns[j].get<double>();
                    arrOpt = operObj["auto_mode"].get<picojson::array>();
                    for (AX_U32 index = 0; index < arrOpt.size(); index++) {
                        AX_U8 nAutoMode = arrOpt[index].get<double>();
                        tOperation.SetOperaType(E_WEB_OPERATION_TYPE_IMAGE_ATTR);
                        tOperation.nSnsID = nSnsID;
                        tOperation.tImageAttr.nAutoMode = nAutoMode;
                        if (1 == nAutoMode) {
                            tOperation.tImageAttr.nSharpness = 78;
                            tOperation.tImageAttr.nBrightness = 100;
                            tOperation.tImageAttr.nContrast = 93;
                            tOperation.tImageAttr.nSaturation = 100;
                            tOperation.tImageAttr.nAutoMode = nAutoMode;
                        }
                        vecReq.emplace_back(tOperation);
                        LOG_MM_I(TEST_PARSER, "operaDesc:%s,nSnsID:%d, nAutoMode:%f", strOperDes.c_str(), nSnsID,
                                 tOperation.tImageAttr.nAutoMode);
                    }
                }
            }
        }
    }

    return AX_TRUE;
}

AX_BOOL CTestSuiteCfgParser::ParseVENCJson(picojson::object& objSetting, std::vector<TESTSUITE_OPERATION_T>& vecReq) {
    picojson::array arrSnsChn;
    picojson::array arrOpt, appType;
    picojson::object moduleSetting, operObj;
    TESTSUITE_OPERATION_T tOperation;
    AX_U32 nSnsID = 0;
    AX_U32 nChn = 0;
    AX_U8 nEncodeTYpe = 0;
    std::string strOperDes;
    moduleSetting = objSetting["VENC"].get<picojson::object>();
    tOperation.eReqType = E_REQ_TYPE_VIDEO;

    if (SEARCHKEY(moduleSetting, "channelSwitch")) {
        AX_U32 nSize = moduleSetting["channelSwitch"].get<picojson::array>().size();
        for (AX_U32 index = 0; index < nSize; index++) {
            operObj = moduleSetting["channelSwitch"].get<picojson::array>()[index].get<picojson::object>();
            if (SEARCHKEY(operObj, "sns_channel_option") && SEARCHKEY(operObj, "encode_type_option") &&
                SEARCHKEY(operObj, "enable_option")) {
                if (SEARCHKEY(operObj, "description")) {
                    strOperDes = tOperation.strDesc = operObj["description"].get<std::string>();
                }
                arrSnsChn = operObj["sns_channel_option"].get<picojson::array>();
                appType = operObj["encode_type_option"].get<picojson::array>();
                for (AX_U32 ti = 0; ti < appType.size(); ti++) {
                    nEncodeTYpe = appType[ti].get<double>();
                    for (AX_U32 j = 0; j < arrSnsChn.size(); j++) {
                        nSnsID = arrSnsChn[j].get<picojson::array>()[0].get<double>();
                        nChn = arrSnsChn[j].get<picojson::array>()[1].get<double>();
                        arrOpt = operObj["enable_option"].get<picojson::array>();
                        for (AX_U32 index = 0; index < arrOpt.size(); index++) {
                            AX_U32 enable = arrOpt[index].get<double>();
                            tOperation.nSnsID = nSnsID;
                            tOperation.nGroup = 0;
                            tOperation.nChannel = nChn;
                            tOperation.SetOperaType(E_WEB_OPERATION_TYPE_CHANNEL_SWITCH);
                            tOperation.tChnSwitch.bOn = (AX_BOOL)enable;
                            tOperation.tChnSwitch.nEncoderType = nEncodeTYpe;
                            vecReq.emplace_back(tOperation);
                            LOG_MM_D(TEST_PARSER, "operaDesc:%s", strOperDes.c_str());
                        }
                    }
                }
            }
        }
    }

    if (SEARCHKEY(moduleSetting, "bitRate")) {
        AX_U32 nSize = moduleSetting["bitRate"].get<picojson::array>().size();
        for (AX_U32 index = 0; index < nSize; index++) {
            operObj = moduleSetting["bitRate"].get<picojson::array>()[index].get<picojson::object>();
            if (SEARCHKEY(operObj, "sns_channel_option") && SEARCHKEY(operObj, "encode_type_option") &&
                SEARCHKEY(operObj, "bitrate_option")) {
                if (SEARCHKEY(operObj, "description")) {
                    strOperDes = tOperation.strDesc = operObj["description"].get<std::string>();
                }
                arrSnsChn = operObj["sns_channel_option"].get<picojson::array>();
                appType = operObj["encode_type_option"].get<picojson::array>();
                for (AX_U32 ti = 0; ti < appType.size(); ti++) {
                    nEncodeTYpe = appType[ti].get<double>();
                    for (AX_U32 j = 0; j < arrSnsChn.size(); j++) {
                        nSnsID = arrSnsChn[j].get<picojson::array>()[0].get<double>();
                        nChn = arrSnsChn[j].get<picojson::array>()[1].get<double>();
                        arrOpt = operObj["bitrate_option"].get<picojson::array>();
                        for (AX_U32 index = 0; index < arrOpt.size(); index++) {
                            AX_U32 nBitrate = arrOpt[index].get<double>();
                            tOperation.nSnsID = nSnsID;
                            tOperation.nGroup = 0;
                            tOperation.nChannel = nChn;
                            tOperation.SetOperaType(E_WEB_OPERATION_TYPE_BITRATE);
                            tOperation.tBitrate.nEncoderType = nEncodeTYpe;
                            tOperation.tBitrate.nBitrate = nBitrate;
                            vecReq.emplace_back(tOperation);
                            LOG_MM_D(TEST_PARSER, "operaDesc:%s", strOperDes.c_str());
                        }
                    }
                }
            }
        }
    }

    if (SEARCHKEY(moduleSetting, "resolution")) {
        AX_U32 nSize = moduleSetting["resolution"].get<picojson::array>().size();
        for (AX_U32 index = 0; index < nSize; index++) {
            operObj = moduleSetting["resolution"].get<picojson::array>()[index].get<picojson::object>();
            if (SEARCHKEY(operObj, "sns_channel_option") && SEARCHKEY(operObj, "encode_type_option") &&
                SEARCHKEY(operObj, "resolution_option")) {
                if (SEARCHKEY(operObj, "description")) {
                    strOperDes = tOperation.strDesc = operObj["description"].get<std::string>();
                }
                arrSnsChn = operObj["sns_channel_option"].get<picojson::array>();
                appType = operObj["encode_type_option"].get<picojson::array>();
                for (AX_U32 ti = 0; ti < appType.size(); ti++) {
                    nEncodeTYpe = appType[ti].get<double>();
                    for (AX_U32 j = 0; j < arrSnsChn.size(); j++) {
                        nSnsID = arrSnsChn[j].get<picojson::array>()[0].get<double>();
                        nChn = arrSnsChn[j].get<picojson::array>()[1].get<double>();
                        arrOpt = operObj["resolution_option"].get<picojson::array>();
                        for (AX_U32 index = 0; index < arrOpt.size(); index++) {
                            AX_U32 nWidth = arrOpt[index].get<picojson::array>()[0].get<double>();
                            AX_U32 nHeight = arrOpt[index].get<picojson::array>()[1].get<double>();
                            tOperation.nSnsID = nSnsID;
                            tOperation.nGroup = 0;
                            tOperation.nChannel = nChn;
                            tOperation.SetOperaType(E_WEB_OPERATION_TYPE_RESOLUTION);
                            tOperation.tResolution.nWidth = nWidth;
                            tOperation.tResolution.nHeight = nHeight;
                            tOperation.tResolution.nEncoderType = nEncodeTYpe;
                            vecReq.emplace_back(tOperation);
                            LOG_MM_D(TEST_PARSER, "operaDesc:%s", strOperDes.c_str());
                        }
                    }
                }
            }
        }
    }

    if (SEARCHKEY(moduleSetting, "rcType")) {
        AX_U32 nSize = moduleSetting["rcType"].get<picojson::array>().size();
        for (AX_U32 index = 0; index < nSize; index++) {
            operObj = moduleSetting["rcType"].get<picojson::array>()[index].get<picojson::object>();
            if (SEARCHKEY(operObj, "sns_channel_option") && SEARCHKEY(operObj, "encode_type_option") &&
                SEARCHKEY(operObj, "rc_type_option")) {
                if (SEARCHKEY(operObj, "description")) {
                    strOperDes = tOperation.strDesc = operObj["description"].get<std::string>();
                }
                arrSnsChn = operObj["sns_channel_option"].get<picojson::array>();
                appType = operObj["encode_type_option"].get<picojson::array>();
                for (AX_U32 ti = 0; ti < appType.size(); ti++) {
                    nEncodeTYpe = appType[ti].get<double>();
                    for (AX_U32 j = 0; j < arrSnsChn.size(); j++) {
                        nSnsID = arrSnsChn[j].get<picojson::array>()[0].get<double>();
                        nChn = arrSnsChn[j].get<picojson::array>()[1].get<double>();
                        arrOpt = operObj["rc_type_option"].get<picojson::array>();
                        for (AX_U32 index = 0; index < arrOpt.size(); index++) {
                            AX_U32 nRctype = arrOpt[index].get<double>();
                            tOperation.nSnsID = nSnsID;
                            tOperation.nGroup = 0;
                            tOperation.nChannel = nChn;
                            tOperation.SetOperaType(E_WEB_OPERATION_TYPE_RC_INFO);
                            tOperation.tRcInfo.nEncoderType = nEncodeTYpe;
                            tOperation.tRcInfo.eRcType = CAXTypeConverter::FormatRcMode(nEncodeTYpe, nRctype);
                            tOperation.tRcInfo.nMinQp = operObj["min_qp"].get<double>();
                            tOperation.tRcInfo.nMaxQp = operObj["max_qp"].get<double>();
                            tOperation.tRcInfo.nMinIQp = operObj["min_iqp"].get<double>();
                            tOperation.tRcInfo.nMaxIQp = operObj["max_iqp"].get<double>();
                            tOperation.tRcInfo.nMinIProp = operObj["min_iprop"].get<double>();
                            tOperation.tRcInfo.nMaxIProp = operObj["max_iprop"].get<double>();
                            tOperation.tRcInfo.nBitrate = operObj["bitRate"].get<double>();
                            vecReq.emplace_back(tOperation);
                        }
                    }
                }
            }
        }
    }

    if (SEARCHKEY(moduleSetting, "iprop")) {
        AX_U32 nSize = moduleSetting["iprop"].get<picojson::array>().size();
        for (AX_U32 index = 0; index < nSize; index++) {
            operObj = moduleSetting["iprop"].get<picojson::array>()[index].get<picojson::object>();
            if (SEARCHKEY(operObj, "sns_channel_option") && SEARCHKEY(operObj, "encode_type_option") &&
                SEARCHKEY(operObj, "iprop_option")) {
                if (SEARCHKEY(operObj, "description")) {
                    strOperDes = tOperation.strDesc = operObj["description"].get<std::string>();
                }
                AX_U32 nRctype = operObj["rc_type"].get<double>();
                arrSnsChn = operObj["sns_channel_option"].get<picojson::array>();
                appType = operObj["encode_type_option"].get<picojson::array>();
                for (AX_U32 ti = 0; ti < appType.size(); ti++) {
                    nEncodeTYpe = appType[ti].get<double>();
                    for (AX_U32 j = 0; j < arrSnsChn.size(); j++) {
                        nSnsID = arrSnsChn[j].get<picojson::array>()[0].get<double>();
                        nChn = arrSnsChn[j].get<picojson::array>()[1].get<double>();
                        arrOpt = operObj["iprop_option"].get<picojson::array>();
                        for (AX_U32 iIndex = 0; iIndex < arrOpt.size(); iIndex++) {
                            for (AX_U32 jIndex = 0; jIndex < arrOpt.size(); jIndex++) {
                                AX_U32 min_iprop = arrOpt[iIndex].get<double>();
                                AX_U32 max_iprop = arrOpt[jIndex].get<double>();
                                if (max_iprop > min_iprop) {
                                    tOperation.nSnsID = nSnsID;
                                    tOperation.nGroup = 0;
                                    tOperation.nChannel = nChn;
                                    tOperation.SetOperaType(E_WEB_OPERATION_TYPE_RC_INFO);
                                    tOperation.tRcInfo.nEncoderType = nEncodeTYpe;
                                    tOperation.tRcInfo.eRcType = CAXTypeConverter::FormatRcMode(nEncodeTYpe, nRctype);
                                    tOperation.tRcInfo.nMaxIProp = max_iprop;
                                    tOperation.tRcInfo.nMinIProp = min_iprop;
                                    vecReq.emplace_back(tOperation);
                                    LOG_MM_D(TEST_PARSER, "operaDesc:%s, min:%d, max:%d", strOperDes.c_str(), min_iprop, max_iprop);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (SEARCHKEY(moduleSetting, "linkEnable")) {
        AX_U32 nSize = moduleSetting["linkEnable"].get<picojson::array>().size();
        for (AX_U32 index = 0; index < nSize; index++) {
            operObj = moduleSetting["linkEnable"].get<picojson::array>()[index].get<picojson::object>();
            if (SEARCHKEY(operObj, "sns_channel_option") && SEARCHKEY(operObj, "enable_option")) {
                if (SEARCHKEY(operObj, "description")) {
                    strOperDes = tOperation.strDesc = operObj["description"].get<std::string>();
                }

                arrSnsChn = operObj["sns_channel_option"].get<picojson::array>();
                for (AX_U32 j = 0; j < arrSnsChn.size(); j++) {
                    nSnsID = arrSnsChn[j].get<picojson::array>()[0].get<double>();
                    nChn = arrSnsChn[j].get<picojson::array>()[1].get<double>();
                    arrOpt = operObj["enable_option"].get<picojson::array>();
                    for (AX_U32 index = 0; index < arrOpt.size(); index++) {
                        AX_U32 enable = arrOpt[index].get<double>();
                        tOperation.SetOperaType(E_WEB_OPERATION_TYPE_VENC_LINK_ENABLE);
                        tOperation.nSnsID = nSnsID;
                        tOperation.nGroup = 0;
                        tOperation.nChannel = nChn;
                        tOperation.tVencLinkEnable.bLinkEnable = (AX_BOOL)enable;
                        vecReq.emplace_back(tOperation);
                        LOG_MM_D(TEST_PARSER, "operaDesc:%s, bLinkEnable:%d", strOperDes.c_str(), enable);
                    }
                }
            }
        }
    }

    if (SEARCHKEY(moduleSetting, "encodeType")) {
        AX_U32 nSize = moduleSetting["encodeType"].get<picojson::array>().size();
        for (AX_U32 index = 0; index < nSize; index++) {
            operObj = moduleSetting["encodeType"].get<picojson::array>()[index].get<picojson::object>();
            if (SEARCHKEY(operObj, "sns_channel_option") && SEARCHKEY(operObj, "encode_type_option")) {
                if (SEARCHKEY(operObj, "description")) {
                    strOperDes = tOperation.strDesc = operObj["description"].get<std::string>();
                }
                arrSnsChn = operObj["sns_channel_option"].get<picojson::array>();
                appType = operObj["encode_type_option"].get<picojson::array>();
                for (AX_U32 ti = 0; ti < appType.size(); ti++) {
                    nEncodeTYpe = appType[ti].get<double>();
                    for (AX_U32 j = 0; j < arrSnsChn.size(); j++) {
                        nSnsID = arrSnsChn[j].get<picojson::array>()[0].get<double>();
                        nChn = arrSnsChn[j].get<picojson::array>()[1].get<double>();
                        tOperation.nSnsID = nSnsID;
                        tOperation.nGroup = 0;
                        tOperation.nChannel = nChn;
                        tOperation.SetOperaType(E_WEB_OPERATION_TYPE_ENC_TYPE);
                        tOperation.tEncType.nEncoderType = nEncodeTYpe;
                        vecReq.emplace_back(tOperation);
                        LOG_MM_D(TEST_PARSER, "operaDesc:%s", strOperDes.c_str());
                    }
                }
            }
        }
    }
    return AX_TRUE;
}

AX_BOOL CTestSuiteCfgParser::ParseAIJson(picojson::object& objSetting, std::vector<TESTSUITE_OPERATION_T>& vecReq) {
    picojson::array arrGrpSns;
    picojson::array arrOpt;
    picojson::object moduleSetting, operObj;
    TESTSUITE_OPERATION_T tOperation;
    AX_U32 nSnsID = 0;
    std::string strOperDes;
    moduleSetting = objSetting["AI"].get<picojson::object>();
    tOperation.eReqType = E_REQ_TYPE_AI;

    if (SEARCHKEY(moduleSetting, "rectSwitch")) {
        AX_U32 nSize = moduleSetting["rectSwitch"].get<picojson::array>().size();
        for (AX_U32 index = 0; index < nSize; index++) {
            operObj = moduleSetting["rectSwitch"].get<picojson::array>()[index].get<picojson::object>();
            if (SEARCHKEY(operObj, "sns_id_option") && SEARCHKEY(operObj, "enable_option")) {
                if (SEARCHKEY(operObj, "description")) {
                    strOperDes = tOperation.strDesc = operObj["description"].get<std::string>();
                }
                arrGrpSns = operObj["sns_id_option"].get<picojson::array>();
                for (AX_U32 j = 0; j < arrGrpSns.size(); j++) {
                    nSnsID = arrGrpSns[j].get<double>();
                    arrOpt = operObj["enable_option"].get<picojson::array>();
                    for (AX_U32 index = 0; index < arrOpt.size(); index++) {
                        AX_U32 enable = arrOpt[index].get<double>();
                        tOperation.SetOperaType(E_WEB_OPERATION_TYPE_AI_ENABLE);
                        tOperation.nSnsID = nSnsID;
                        tOperation.tAiEnable.bOn = (AX_BOOL)enable;
                        vecReq.emplace_back(tOperation);
                        LOG_MM_D(TEST_PARSER, "operaDesc:%s", strOperDes.c_str());
                    }
                }
            }
        }
    }
    if (SEARCHKEY(moduleSetting, "pushMode")) {
        AX_U32 nSize = moduleSetting["pushMode"].get<picojson::array>().size();
        for (AX_U32 index = 0; index < nSize; index++) {
            operObj = moduleSetting["pushMode"].get<picojson::array>()[index].get<picojson::object>();
            if (SEARCHKEY(operObj, "sns_id_option") && SEARCHKEY(operObj, "push_mode_option") &&
                SEARCHKEY(operObj, "push_interval_option") && SEARCHKEY(operObj, "push_count")) {
                if (SEARCHKEY(operObj, "description")) {
                    strOperDes = tOperation.strDesc = operObj["description"].get<std::string>();
                }
                picojson::array arrTest;
                arrGrpSns = operObj["sns_id_option"].get<picojson::array>();
                arrOpt = operObj["push_mode_option"].get<picojson::array>();
                arrTest = operObj["push_interval_option"].get<picojson::array>();
                for (AX_U32 j = 0; j < arrGrpSns.size(); j++) {
                    nSnsID = arrGrpSns[j].get<double>();
                    for (AX_U32 index = 0; index < arrOpt.size(); index++) {
                        AX_U32 nMode = arrOpt[index].get<double>();
                        for (AX_U32 iIndex = 0; iIndex < arrTest.size(); iIndex++) {
                            tOperation.SetOperaType(E_WEB_OPERATION_TYPE_AI_PUSH_MODE);
                            tOperation.nSnsID = nSnsID;
                            tOperation.tAiPushStategy.ePushMode = (E_AI_DETECT_PUSH_MODE_TYPE)nMode;
                            tOperation.tAiPushStategy.nPushIntervalMs = arrTest[iIndex].get<double>();
                            tOperation.tAiPushStategy.nPushCounts = operObj["push_count"].get<double>();
                            vecReq.emplace_back(tOperation);
                            LOG_MM_D(TEST_PARSER, "operaDesc:%s", strOperDes.c_str());
                        }
                    }
                }
            }
        }
    }
    if (SEARCHKEY(moduleSetting, "eventReport")) {
        AX_U32 nSize = moduleSetting["eventReport"].get<picojson::array>().size();
        for (AX_U32 index = 0; index < nSize; index++) {
            operObj = moduleSetting["eventReport"].get<picojson::array>()[index].get<picojson::object>();
            if (SEARCHKEY(operObj, "sns_id_option") && SEARCHKEY(operObj, "event_option") && SEARCHKEY(operObj, "enable_option") &&
                SEARCHKEY(operObj, "threshold_y") && SEARCHKEY(operObj, "confidence")) {
                if (SEARCHKEY(operObj, "description")) {
                    strOperDes = tOperation.strDesc = operObj["description"].get<std::string>();
                }
                picojson::array arrTestEnable;
                picojson::array arrTestThresholdY;
                picojson::array arrTestConfidence;
                arrGrpSns = operObj["sns_id_option"].get<picojson::array>();
                arrOpt = operObj["event_option"].get<picojson::array>();
                arrTestEnable = operObj["enable_option"].get<picojson::array>();
                arrTestThresholdY = operObj["threshold_y"].get<picojson::array>();
                arrTestConfidence = operObj["confidence"].get<picojson::array>();
                for (AX_U32 j = 0; j < arrGrpSns.size(); j++) {
                    nSnsID = arrGrpSns[j].get<double>();
                    for (AX_U32 indexOpt = 0; indexOpt < arrOpt.size(); indexOpt++) {
                        string eventType = arrOpt[indexOpt].get<string>();
                        for (AX_U32 iIndexEnable = 0; iIndexEnable < arrTestEnable.size(); iIndexEnable++) {
                            AX_U32 enable = arrTestEnable[iIndexEnable].get<double>();
                            for (AX_U32 iIndexThreshold = 0; iIndexThreshold < arrTestThresholdY.size(); iIndexThreshold++) {
                                for (AX_U32 iIndexConfidence = 0; iIndexConfidence < arrTestConfidence.size(); iIndexConfidence++) {
                                    tOperation.SetOperaType(E_WEB_OPERATION_TYPE_AI_EVENT);
                                    tOperation.nSnsID = nSnsID;
                                    if (0 == strcmp(eventType.c_str(), "motion_detect")) {
                                        tOperation.tEvent.tMD.bEnable = ADAPTER_INT2BOOL(enable);
                                        tOperation.tEvent.tMD.nThrsHoldY = arrTestThresholdY[iIndexThreshold].get<double>();
                                        tOperation.tEvent.tMD.nConfidence = arrTestConfidence[iIndexConfidence].get<double>();
                                        tOperation.tEvent.tOD.bEnable = AX_FALSE;
                                        tOperation.tEvent.tOD.nThrsHoldY = 1;
                                        tOperation.tEvent.tOD.nConfidence = 1;
                                        tOperation.tEvent.tSCD.bEnable = AX_FALSE;
                                        tOperation.tEvent.tSCD.nThrsHoldY = 1;
                                        tOperation.tEvent.tSCD.nConfidence = 1;
                                    } else if (0 == strcmp(eventType.c_str(), "occlusion_detect")) {
                                        tOperation.tEvent.tMD.bEnable = AX_FALSE;
                                        tOperation.tEvent.tMD.nThrsHoldY = 1;
                                        tOperation.tEvent.tMD.nConfidence = 1;
                                        tOperation.tEvent.tOD.bEnable = ADAPTER_INT2BOOL(enable);
                                        tOperation.tEvent.tOD.nThrsHoldY = arrTestThresholdY[iIndexThreshold].get<double>();
                                        tOperation.tEvent.tOD.nConfidence = arrTestConfidence[iIndexConfidence].get<double>();
                                        tOperation.tEvent.tSCD.bEnable = AX_FALSE;
                                        tOperation.tEvent.tSCD.nThrsHoldY = 1;
                                        tOperation.tEvent.tSCD.nConfidence = 1;
                                    } else if (0 == strcmp(eventType.c_str(), "scene_change_detect")) {
                                        tOperation.tEvent.tMD.bEnable = AX_FALSE;
                                        tOperation.tEvent.tMD.nThrsHoldY = 1;
                                        tOperation.tEvent.tMD.nConfidence = 1;
                                        tOperation.tEvent.tOD.bEnable = AX_FALSE;
                                        tOperation.tEvent.tOD.nThrsHoldY = 1;
                                        tOperation.tEvent.tOD.nConfidence = 1;
                                        tOperation.tEvent.tSCD.bEnable = ADAPTER_INT2BOOL(enable);
                                        tOperation.tEvent.tSCD.nThrsHoldY = arrTestThresholdY[iIndexThreshold].get<double>();
                                        tOperation.tEvent.tSCD.nConfidence = arrTestConfidence[iIndexConfidence].get<double>();
                                    } else {
                                        LOG_MM_E(TEST_PARSER, "invalid event type!");
                                    }
                                    vecReq.emplace_back(tOperation);
                                    LOG_MM_D(TEST_PARSER, "operaDesc:%s", strOperDes.c_str());
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return AX_TRUE;
}

AX_BOOL CTestSuiteCfgParser::ParseOSDJson(picojson::object& objSetting, std::vector<TESTSUITE_OPERATION_T>& vecReq) {
    picojson::array arrGrpSns;
    picojson::array arrOpt;
    picojson::object moduleSetting, operObj;
    TESTSUITE_OPERATION_T tOperation;
    AX_U32 nSnsID = 0;
    std::string strOperDes;
    moduleSetting = objSetting["OSD"].get<picojson::object>();
    tOperation.eReqType = E_REQ_TYPE_OSD;

    if (SEARCHKEY(moduleSetting, "osdSwitch")) {
        AX_U32 nSize = moduleSetting["osdSwitch"].get<picojson::array>().size();
        for (AX_U32 index = 0; index < nSize; index++) {
            operObj = moduleSetting["osdSwitch"].get<picojson::array>()[index].get<picojson::object>();
            if (SEARCHKEY(operObj, "sns_id_option") && SEARCHKEY(operObj, "enable_option")) {
                if (SEARCHKEY(operObj, "description")) {
                    strOperDes = tOperation.strDesc = operObj["description"].get<std::string>();
                }
                arrGrpSns = operObj["sns_id_option"].get<picojson::array>();
                for (AX_U32 j = 0; j < arrGrpSns.size(); j++) {
                    nSnsID = arrGrpSns[j].get<double>();
                    arrOpt = operObj["enable_option"].get<picojson::array>();
                    for (AX_U32 index = 0; index < arrOpt.size(); index++) {
                        AX_U32 enable = arrOpt[index].get<double>();
                        tOperation.SetOperaType(E_WEB_OPERATION_TYPE_OSD_ENABLE);
                        tOperation.nSnsID = nSnsID;
                        tOperation.tOsdEnable.bOn = (AX_BOOL)enable;
                        vecReq.emplace_back(tOperation);
                        LOG_MM_D(TEST_PARSER, "operaDesc:%s", strOperDes.c_str());
                    }
                }
            }
        }
    }
    return AX_TRUE;
}

AX_BOOL CTestSuiteCfgParser::GetUTCase(AX_U8 nScenario, std::vector<TESTSUITE_OPERATION_T>& vecOutConfig) {
    auto it = m_mapScenario2GrpSetting.find(nScenario);
    if (m_mapScenario2GrpSetting.end() == it) {
        return AX_FALSE;
    }
    vecOutConfig = it->second;
    return AX_TRUE;
}

AX_BOOL CTestSuiteCfgParser::GetTestAttr(AX_U8 nScenario, APP_TEST_SUITE_CONFIG_T& tTestCfg) {
    auto it = m_mapTestCfg.find(nScenario);
    if (m_mapTestCfg.end() == it) {
        return AX_FALSE;
    }
    tTestCfg = it->second;
    return AX_TRUE;
}