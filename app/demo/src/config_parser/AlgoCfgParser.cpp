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
#include "ax_skel_type.h"
#include "AppLogApi.h"
#include "CommonUtils.hpp"
#include "AlgoCfgParser.h"

using namespace std;

#define ALGO_PARSER "ALGO_PARSER"

#define ALGO_SETTINGS_KEY_STR "algo_settings"

AX_BOOL CAlgoCfgParser::InitOnce() {
    return AX_TRUE;
}

AX_BOOL CAlgoCfgParser::GetConfig(AX_APP_ALGO_PARAM_T stAlgoParam[AX_APP_ALGO_SNS_MAX],
                                    AX_APP_ALGO_AUDIO_PARAM_T& stAudioParam) {
    string strConfigDir = CCommonUtils::GetPPLConfigDir();
    if (strConfigDir.empty()) {
        return AX_FALSE;
    }

    string strAlgoCfgFile = strConfigDir + "/algo.json";

    ifstream ifs(strAlgoCfgFile.c_str());
    picojson::value v;
    ifs >> v;

    string err = picojson::get_last_error();
    if (!err.empty()) {
        LOG_M_E(ALGO_PARSER, "Failed to load json config file: %s", strAlgoCfgFile.c_str());
        return AX_FALSE;
    }

    if (!v.is<picojson::object>()) {
        LOG_M_E(ALGO_PARSER, "Loaded config file is not a well-formatted JSON.");
        return AX_FALSE;
    }

    if (!ParseFile(strAlgoCfgFile, stAlgoParam, stAudioParam)) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CAlgoCfgParser::ParseFile(const string& strPath,
                                    AX_APP_ALGO_PARAM_T stAlgoParam[AX_APP_ALGO_SNS_MAX],
                                    AX_APP_ALGO_AUDIO_PARAM_T& stAudioParam) {
    picojson::value v;
    ifstream fIn(strPath.c_str());
    if (!fIn.is_open()) {
        return AX_TRUE;
    }

    string strParseRet = picojson::parse(v, fIn);
    if (!strParseRet.empty() || !v.is<picojson::object>()) {
        return AX_TRUE;
    }

    return ParseJson(v.get<picojson::object>(), stAlgoParam, stAudioParam);
}

AX_BOOL CAlgoCfgParser::ParseJson(picojson::object& objJsonRoot,
                                    AX_APP_ALGO_PARAM_T stAlgoParam[AX_APP_ALGO_SNS_MAX],
                                    AX_APP_ALGO_AUDIO_PARAM_T& stAudioParam) {
    // get algo settings
    if (objJsonRoot.end() == objJsonRoot.find(ALGO_SETTINGS_KEY_STR)) {
        return AX_TRUE;
    }

    picojson::array& arrGrpSettings = objJsonRoot[ALGO_SETTINGS_KEY_STR].get<picojson::array>();

    for (size_t i = 0; i < arrGrpSettings.size(); i++) {
        picojson::object objSettings = arrGrpSettings[i].get<picojson::object>();

        AX_U32 nIndex = i;

        if (objSettings.end() != objSettings.find("sns_id")) {
            nIndex = (AX_U32)objSettings["sns_id"].get<double>();
        }

        if (nIndex >= AX_APP_ALGO_SNS_MAX) {
            continue;
        }

        // skel attribute
        ParseSkelJson(objSettings, stAlgoParam[nIndex]);

        // ives attribute
        ParseIvesJson(objSettings, stAlgoParam[nIndex]);
    }

    // get audio attribute
    ParseAudioJson(objJsonRoot, stAudioParam);

    for (size_t i = 0; i < arrGrpSettings.size(); i++) {
        if (stAudioParam.bEnable) {
            stAlgoParam[i].nAlgoType |= AX_APP_ALGO_SOUND_DETECT;
        }
        else {
            stAlgoParam[i].nAlgoType &= ~AX_APP_ALGO_SOUND_DETECT;
        }
    }

    return AX_TRUE;
}

AX_BOOL CAlgoCfgParser::ParseSkelJson(picojson::object& objJsonRoot, AX_APP_ALGO_PARAM_T& stAlgoParam) {
    AX_BOOL bSucc = AX_TRUE;

    // get skel attribute
    do {
        // SKEL
        if (objJsonRoot.end() == objJsonRoot.find("SKEL")) {
            bSucc = AX_FALSE;
            break;
        }

        picojson::object& objSkel = objJsonRoot["SKEL"].get<picojson::object>();

        // SKEL.detect_algo_type
        AX_S32 nDetectAlgoType = objSkel["detect_algo_type"].get<double>();

        if (nDetectAlgoType == AX_SKEL_PPL_BODY) {
            stAlgoParam.nAlgoType |= AX_APP_ALGO_PERSON_DETECT;
        }
        else if (nDetectAlgoType == AX_SKEL_PPL_HVCFP) {
            stAlgoParam.nAlgoType |= AX_APP_ALGO_TYPE_HVCFP;
        }
#if 0
        else if (nDetectAlgoType == AX_SKEL_PPL_HVCP) {
            stAlgoParam.nAlgoType |= AX_APP_ALGO_TYPE_HVCP;
        }
#endif
        else if (nDetectAlgoType == AX_SKEL_PPL_FH) {
            stAlgoParam.nAlgoType |= AX_APP_ALGO_TYPE_FH;
        }
        else if (nDetectAlgoType == 0) {
            stAlgoParam.nAlgoType &= ~AX_APP_ALGO_TYPE_HVCP;
            stAlgoParam.nAlgoType &= ~AX_APP_ALGO_TYPE_FH;
        }

        auto &stHvcfpParam = stAlgoParam.stHvcfpParam;

        std::string strDetectModelsPath = objSkel["detect_models_path"].get<std::string>();
        strncpy(stHvcfpParam.strDetectModelsPath,
                strDetectModelsPath.c_str(),
                AX_APP_ALGO_PATH_LEN - 1);

        stHvcfpParam.stPushStrategy.ePushMode
                    = (AX_APP_ALGO_PUSH_MODE_E)objSkel["push_strategy"].get<picojson::object>()["push_mode"].get<double>();

        stHvcfpParam.stPushStrategy.nInterval
                    = (AX_U32)objSkel["push_strategy"].get<picojson::object>()["interval_time"].get<double>();

        stHvcfpParam.stPushStrategy.nPushCount
                    = (AX_U32)objSkel["push_strategy"].get<picojson::object>()["push_counts"].get<double>();

        if (objSkel.end() != objSkel.find("push_to_web")) {
            stHvcfpParam.bPushToWeb = (AX_BOOL)objSkel["push_to_web"].get<bool>();
        }
    } while(0);

    return bSucc;
}

AX_BOOL CAlgoCfgParser::ParseIvesJson(picojson::object& objJsonRoot, AX_APP_ALGO_PARAM_T& stAlgoParam) {
    AX_BOOL bSucc = AX_TRUE;

    // get ives attribute
    do {
        // IVES
        if (objJsonRoot.end() == objJsonRoot.find("IVES")) {
            bSucc = AX_FALSE;
            break;
        }

        picojson::object& objIves = objJsonRoot["IVES"].get<picojson::object>();

        // MD
        if (objIves.end() != objIves.find("MD")) {
            picojson::object& objMD = objIves["MD"].get<picojson::object>();

            stAlgoParam.stMotionParam.bEnable = (AX_BOOL)objMD["enable"].get<bool>();

            if (stAlgoParam.stMotionParam.bEnable) {
                stAlgoParam.nAlgoType |= AX_APP_ALGO_MOTION_DETECT;
            }
            else {
                stAlgoParam.nAlgoType &= ~AX_APP_ALGO_MOTION_DETECT;
            }

            stAlgoParam.stMotionParam.nRegionSize = 1;
            stAlgoParam.stMotionParam.stRegions[0].bEnable = AX_TRUE;
            stAlgoParam.stMotionParam.stRegions[0].nChan = 0;
            stAlgoParam.stMotionParam.stRegions[0].nMbWidth = 32;
            stAlgoParam.stMotionParam.stRegions[0].nMbHeight = 32;

            stAlgoParam.stMotionParam.stRegions[0].fThresholdY
                    = objMD["threshold Y"].get<double>();

            std::string strMb = objMD["mb"].get<std::string>();

            std::string::size_type pos = strMb.find('x');
            if (std::string::npos != pos) {
                AX_S32 nW = atoi(strMb.substr(0, pos).c_str());
                AX_S32 nH = atoi(strMb.substr(pos + 1, -1).c_str());
                if (nW > 0 && nH > 0) {
                    stAlgoParam.stMotionParam.stRegions[0].nMbWidth = nW;
                    stAlgoParam.stMotionParam.stRegions[0].nMbHeight = nH;
                }
            }

            stAlgoParam.stMotionParam.stRegions[0].fConfidence
                    = objMD["confidence"].get<double>();
        }

        // OD
        if (objIves.end() != objIves.find("OD")) {
            picojson::object& objOD = objIves["OD"].get<picojson::object>();

            stAlgoParam.stOcclusionParam.bEnable = (AX_BOOL)objOD["enable"].get<bool>();

            if (stAlgoParam.stOcclusionParam.bEnable) {
                stAlgoParam.nAlgoType |= AX_APP_ALGO_OCCLUSION_DETECT;
            }
            else {
                stAlgoParam.nAlgoType &= ~AX_APP_ALGO_OCCLUSION_DETECT;
            }

            stAlgoParam.stOcclusionParam.fThreshold
                    = objOD["threshold Y"].get<double>();
            stAlgoParam.stOcclusionParam.fConfidence
                    = objOD["confidence Y"].get<double>();
            stAlgoParam.stOcclusionParam.fLuxThreshold
                    = objOD["lux threshold"].get<double>();
            stAlgoParam.stOcclusionParam.fLuxConfidence
                    = objOD["lux diff"].get<double>();
        }

        // SCD
        if (objIves.end() != objIves.find("SCD")) {
            picojson::object& objSCD = objIves["SCD"].get<picojson::object>();

            stAlgoParam.stSceneChangeParam.bEnable = (AX_BOOL)objSCD["enable"].get<bool>();

            if (stAlgoParam.stSceneChangeParam.bEnable) {
                stAlgoParam.nAlgoType |= AX_APP_ALGO_SCENE_CHANGE_DETECT;
            }
            else {
                stAlgoParam.nAlgoType &= ~AX_APP_ALGO_SCENE_CHANGE_DETECT;
            }

            stAlgoParam.stSceneChangeParam.fThreshold
                    = objSCD["threshold"].get<double>();
            stAlgoParam.stSceneChangeParam.fConfidence
                    = objSCD["confidence"].get<double>();
        }
    } while(0);

    return bSucc;
}

AX_BOOL CAlgoCfgParser::ParseAudioJson(picojson::object& objJsonRoot, AX_APP_ALGO_AUDIO_PARAM_T& stAudioParam) {
    AX_BOOL bSucc = AX_TRUE;

    // get audio attribute
    do {
        // AUDIO
        if (objJsonRoot.end() == objJsonRoot.find("AUDIO")) {
            bSucc = AX_FALSE;
            break;
        }

        picojson::object& objAudio = objJsonRoot["AUDIO"].get<picojson::object>();

        stAudioParam.bEnable = (AX_BOOL)objAudio["enable"].get<bool>();
        stAudioParam.fThreshold = (AX_BOOL)objAudio["threshold"].get<double>();
    } while(0);

    return bSucc;
}
