/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "SensorCfgParser.h"
#include <fstream>
#include "AXStringHelper.hpp"
#include "AppLogApi.h"
#include "CommonUtils.hpp"

using namespace std;

#define SNS_PARSER "SNS_PARSER"

AX_BOOL CSensorCfgParser::InitOnce() {
    return AX_TRUE;
}

AX_BOOL CSensorCfgParser::GetConfig(std::map<AX_U8, std::map<AX_U8, SENSOR_CONFIG_T>>& mapSensorCfg,
                                    AX_S32& nCurrScenario, AX_U32& nSensorCount) {
    string strConfigDir = CCommonUtils::GetPPLConfigDir();
    if (strConfigDir.empty()) {
        return AX_FALSE;
    }

    AX_S32 nLoadType = E_LOAD_TYPE_MAX;
    if (!CCmdLineParser::GetInstance()->GetLoadType(nLoadType)) {
        return AX_FALSE;
    }

    AX_CHAR szBoardID[16] = {0};
    if (!CCommonUtils::GetBoardID(szBoardID, 16)) {
        return AX_FALSE;
    }

    string strBoardDir = "AX650A";
    if (strncmp(szBoardID, "AX650N_Demo", sizeof("AX650N_Demo") - 1) == 0 ||
        strncmp(szBoardID, "AX650N_EVB", sizeof("AX650N_EVB") - 1) == 0) {
        strBoardDir = "AX650N";
    }

    string strFileName = LoadType2FileName(nLoadType);
    string strFilePath = CAXStringHelper::Format("%s/sensor/%s/%s", strConfigDir.c_str(), strBoardDir.c_str(), strFileName.c_str());
    LOG_MM_D(SNS_PARSER, "strFilePath:%s", strFilePath.c_str());
    ifstream ifs(strFilePath.c_str());
    if (!ifs.good()) {
        LOG_M_E(SNS_PARSER, "Sensor config file: %s parse failed.", strFilePath.c_str());
        return AX_FALSE;
    }

    picojson::value v;
    ifs >> v;

    string err = picojson::get_last_error();
    if (!err.empty()) {
        LOG_M_E(SNS_PARSER, "Failed to load json config file: %s", strFilePath.c_str());
        return AX_FALSE;
    }

    if (!v.is<picojson::object>()) {
        LOG_M_E(SNS_PARSER, "Loaded config file is not a well-formatted JSON.");
        return AX_FALSE;
    }

    if (!ParseFile(strFilePath, mapSensorCfg)) {
        return AX_FALSE;
    } else {
        LOG_M(SNS_PARSER, "Parse sensor config file: %s successfully.", strFilePath.c_str());
    }
    /* Default scenario*/
    AX_S32 nCfgScenario = (E_LOAD_TYPE_SINGLE_OS08A20 == nLoadType ? E_PPL_SCENRIO_2 : E_PPL_SCENRIO_0);
    if (!CCmdLineParser::GetInstance()->GetScenario(nCfgScenario)) {
        LOG_MM(SNS_PARSER, "Apply default scenario %d.", nCfgScenario);
    }

    LOG_M(SNS_PARSER, "Load sensor config for scenario %d", nCfgScenario);

    std::map<AX_U8, SENSOR_CONFIG_T>* pMapScenario = &mapSensorCfg[nCfgScenario];

    if (nullptr == pMapScenario) {
        LOG_M_E(SNS_PARSER, "Can not get sensor config for scenario %d", nCfgScenario);
        return AX_FALSE;
    }

    nCurrScenario = nCfgScenario;
    nSensorCount = pMapScenario->size();

    return AX_TRUE;
}

AX_BOOL CSensorCfgParser::ParseFile(const string& strPath,
                                        std::map<AX_U8, std::map<AX_U8, SENSOR_CONFIG_T>>& mapSensorCfg) {
    picojson::value v;
    ifstream fIn(strPath.c_str());
    if (!fIn.is_open()) {
        return AX_FALSE;
    }

    string strParseRet = picojson::parse(v, fIn);
    if (!strParseRet.empty() || !v.is<picojson::object>()) {
        return AX_FALSE;
    }

    return ParseJson(v.get<picojson::object>(), mapSensorCfg);
}

AX_BOOL CSensorCfgParser::ParseJson(picojson::object& objJsonRoot,
                                        std::map<AX_U8, std::map<AX_U8, SENSOR_CONFIG_T>>& mapSensorCfg) {
    for (AX_U32 nScenario = 0; nScenario < E_IPC_SCENARIO_MAX; nScenario++) {
        string strScenario = CCmdLineParser::ScenarioEnum2Str((AX_IPC_SCENARIO_E)nScenario);
        if (objJsonRoot.end() == objJsonRoot.find(strScenario.c_str())) {
            continue;
        }

        picojson::array& arrSettings = objJsonRoot[strScenario.c_str()].get<picojson::array>();
        if (0 == arrSettings.size()) {
            continue;
        }

        std::map<AX_U8, SENSOR_CONFIG_T> mapDev2SnsSetting;
        picojson::object objSetting;
        for (size_t nSettingIndex = 0; nSettingIndex < arrSettings.size(); nSettingIndex++) {
            auto& objSetting = arrSettings[nSettingIndex].get<picojson::object>();

            SENSOR_CONFIG_T tSensorCfg;
            tSensorCfg.nSnsID = objSetting["sns_id"].get<double>();
            tSensorCfg.nDevID = objSetting["dev_id"].get<double>();

            if (objSetting.end() != objSetting.find("dev_node")) {
                tSensorCfg.nDevNode = objSetting["dev_node"].get<double>();
                LOG_M_D(SNS_PARSER, "Sns[%d]DevId[%d] nDevNode: %d",
                                    tSensorCfg.nSnsID, tSensorCfg.nDevID,
                                    tSensorCfg.nDevNode);
            } else {
                LOG_M_E(SNS_PARSER, "Miss 'dev_node' config!");
            }

            if (objSetting.end() != objSetting.find("clk_id")) {
                tSensorCfg.nClkID = objSetting["clk_id"].get<double>();
                LOG_M_D(SNS_PARSER, "Sns[%d]DevId[%d] nClkID: %d",
                                    tSensorCfg.nSnsID, tSensorCfg.nDevID,
                                    tSensorCfg.nClkID);
            } else {
                LOG_M_E(SNS_PARSER, "Miss 'clk_id' config!");
            }

            if (objSetting.end() != objSetting.find("master_slave_select")) {
                tSensorCfg.nMasterSlaveSel = (AX_SNS_MASTER_SLAVE_E)objSetting["master_slave_select"].get<double>();
            }
            tSensorCfg.fFrameRate = objSetting["sns_framerate"].get<double>();
            tSensorCfg.eSensorType = (SNS_TYPE_E)objSetting["sns_type"].get<double>();
            tSensorCfg.eSensorMode = (AX_SNS_HDR_MODE_E)objSetting["sns_mode"].get<double>();

            tSensorCfg.eDevMode = (AX_VIN_DEV_MODE_E)objSetting["dev_run_mode"].get<double>();
            tSensorCfg.eSnsOutputMode = (AX_SNS_OUTPUT_MODE_E)objSetting["sns_output_mode"].get<double>();
            if (objSetting.end() != objSetting.find("enable_flash")) {
                tSensorCfg.bEnableFlash = (AX_BOOL)objSetting["enable_flash"].get<double>();
            }

            PIPE_CONFIG_T tPipeConfig;
            tPipeConfig.nPipeID = objSetting["pipe_id"].get<double>();
            if (objSetting.end() != objSetting.find("pipe_framerate")) {
                tPipeConfig.fPipeFramerate = objSetting["pipe_framerate"].get<double>();
            }
            picojson::array& arrChnResInfo = objSetting["resolution"].get<picojson::array>();
            for (size_t i = 0; i < arrChnResInfo.size(); i++) {
                if (i >= AX_VIN_CHN_ID_MAX) {
                    continue;
                }
                tPipeConfig.arrChannelAttr[i].nWidth = arrChnResInfo[i].get<picojson::array>()[0].get<double>();
                tPipeConfig.arrChannelAttr[i].nHeight = arrChnResInfo[i].get<picojson::array>()[1].get<double>();
            }

            picojson::array& arrYuvDepth = objSetting["yuv_depth"].get<picojson::array>();
            for (size_t i = 0; i < arrYuvDepth.size(); i++) {
                if (i >= AX_VIN_CHN_ID_MAX) {
                    continue;
                }

                tPipeConfig.arrChannelAttr[i].nYuvDepth = arrYuvDepth[i].get<double>();
            }

            if (objSetting.end() != objSetting.find("chn_framerate")) {
                picojson::array& arrChnFrameRate = objSetting["chn_framerate"].get<picojson::array>();
                for (size_t i = 0; i < arrChnFrameRate.size(); i++) {
                    if (i >= AX_VIN_CHN_ID_MAX) {
                        continue;
                    }

                    tPipeConfig.arrChannelAttr[i].fFrameRate = arrChnFrameRate[i].get<double>();
                }
            }

            picojson::array& arrChnEnable = objSetting["enable_channel"].get<picojson::array>();
            for (size_t i = 0; i < arrChnEnable.size(); i++) {
                if (i >= AX_VIN_CHN_ID_MAX) {
                    continue;
                }

                tPipeConfig.arrChannelAttr[i].bChnEnable = arrChnEnable[i].get<double>() == 0 ? AX_FALSE : AX_TRUE;
            }

            picojson::array& arrChnCompressInfo = objSetting["chn_compress"].get<picojson::array>();
            for (size_t i = 0; i < arrChnCompressInfo.size(); i++) {
                if (i >= AX_VIN_CHN_ID_MAX) {
                    continue;
                }
                tPipeConfig.arrChannelAttr[i].tChnCompressInfo.enCompressMode =
                    (AX_COMPRESS_MODE_E)arrChnCompressInfo[i].get<picojson::array>()[0].get<double>();
                tPipeConfig.arrChannelAttr[i].tChnCompressInfo.u32CompressLevel =
                    arrChnCompressInfo[i].get<picojson::array>()[1].get<double>();
            }

            tPipeConfig.bSnapshot = objSetting["snapshot"].get<double>() == 0 ? AX_FALSE : AX_TRUE;
            tPipeConfig.bAiEnable = objSetting["enable_aiisp"].get<double>() == 0 ? AX_FALSE : AX_TRUE;
            tPipeConfig.bDummyEnable = objSetting["enable_dummy"].get<double>() == 0 ? AX_FALSE : AX_TRUE;
            if (objSetting.end() != objSetting.find("ife_compress")) {
                tPipeConfig.tIfeCompress.enCompressMode =
                    (AX_COMPRESS_MODE_E)objSetting["ife_compress"].get<picojson::array>()[0].get<double>();
                tPipeConfig.tIfeCompress.u32CompressLevel = objSetting["ife_compress"].get<picojson::array>()[1].get<double>();
            }

            if (objSetting.end() != objSetting.find("ainr_compress")) {
                tPipeConfig.tAiNrCompress.enCompressMode =
                    (AX_COMPRESS_MODE_E)objSetting["ainr_compress"].get<picojson::array>()[0].get<double>();
                tPipeConfig.tAiNrCompress.u32CompressLevel = objSetting["ainr_compress"].get<picojson::array>()[1].get<double>();
            }

            if (objSetting.end() != objSetting.find("3dnr_compress")) {
                tPipeConfig.t3DNrCompress.enCompressMode =
                    (AX_COMPRESS_MODE_E)objSetting["3dnr_compress"].get<picojson::array>()[0].get<double>();
                tPipeConfig.t3DNrCompress.u32CompressLevel = objSetting["3dnr_compress"].get<picojson::array>()[1].get<double>();
            }

            tPipeConfig.bTuning = objSetting["tuning_ctrl"].get<double>() == 0 ? AX_FALSE : AX_TRUE;
            tPipeConfig.nTuningPort = objSetting["tuning_port"].get<double>();

            picojson::array& arrTuningBin = objSetting["tuning_bin"].get<picojson::array>();
            for (size_t i = 0; i < arrTuningBin.size(); i++) {
                tPipeConfig.vecTuningBin.push_back(arrTuningBin[i].get<string>());
            }

            // parse car window enhance config
            tPipeConfig.nEnhanceModelCnt = 0;
            if (objSetting.end() != objSetting.find("cw_enhance_cfg")) {
                picojson::array& arrEnhanceCfg = objSetting["cw_enhance_cfg"].get<picojson::array>();
                AX_U32 nCount = 0;
                for (size_t i = 0; i < arrEnhanceCfg.size(); i++) {
                    picojson::object objEnhance = arrEnhanceCfg[i].get<picojson::object>();
                    tPipeConfig.tEnhanceModelTable[i].nRefValue = objEnhance["ref_value"].get<double>();
                    strcpy((char*)tPipeConfig.tEnhanceModelTable[i].szModel, objEnhance["model"].get<string>().c_str());
                    strcpy((char*)tPipeConfig.tEnhanceModelTable[i].szMask, objEnhance["mask"].get<string>().c_str());
                    // printf("****[%d] %d %s %s\n", (int)i, tPipeConfig.tEnhanceModelTable[i].nRefValue,
                    //                                 tPipeConfig.tEnhanceModelTable[i].szModel,
                    //                                 tPipeConfig.tEnhanceModelTable[i].szMask);
                    nCount++;
                    if (nCount >= ENHANCE_TABLE_CNT) {
                        printf("car window enchance cfg size <%d> is invalid, max size is %d.\n", (int)arrEnhanceCfg.size(), ENHANCE_TABLE_CNT);
                    }
                }
                tPipeConfig.nEnhanceModelCnt = nCount;
            }

            /* Merge multiple pipe configs to single sensor config with same sensor id */
            if (mapDev2SnsSetting.find(tSensorCfg.nSnsID) != mapDev2SnsSetting.end()) {
                if (MAX_PIPE_PER_DEVICE == mapDev2SnsSetting[tSensorCfg.nSnsID].nPipeCount) {
                    LOG_M_E(SNS_PARSER, "Configured sensor count exceeding the max number %d\n", MAX_PIPE_PER_DEVICE);
                    return AX_FALSE;
                }

                mapDev2SnsSetting[tSensorCfg.nSnsID].arrPipeAttr[mapDev2SnsSetting[tSensorCfg.nSnsID].nPipeCount] = tPipeConfig;
                mapDev2SnsSetting[tSensorCfg.nSnsID].nPipeCount++;
            } else {
                tSensorCfg.arrPipeAttr[tSensorCfg.nPipeCount] = tPipeConfig;
                tSensorCfg.nPipeCount++;
                mapDev2SnsSetting[tSensorCfg.nSnsID] = tSensorCfg;
            }
        }

        mapSensorCfg.insert(make_pair(nScenario, mapDev2SnsSetting));
    }

    return mapSensorCfg.size() > 0 ? AX_TRUE : AX_FALSE;
}

string CSensorCfgParser::LoadType2FileName(AX_S32 nLoadType) {
    AX_IPC_LOAD_TYPE_E eLoadType = (AX_IPC_LOAD_TYPE_E)nLoadType;
    switch (eLoadType) {
        case E_LOAD_TYPE_DUAL_OS08A20: {
            return "Dual_OS08A20.json";
        }
        case E_LOAD_TYPE_SINGLE_OS08A20: {
            return "Single_OS08A20.json";
        }
        case E_LOAD_TYPE_DUAL_OS08B10: {
            return "Dual_OS08B10.json";
        }
        case E_LOAD_TYPE_SINGLE_OS08B10: {
            return "Single_OS08B10.json";
        }
        case E_LOAD_TYPE_SINGLE_SC910GS: {
            return "Single_SC910gs.json";
        }
        case E_LOAD_TYPE_PANO_DUAL_OS04A10: {
            return "Pano_Dual_OS04A10.json";
        }
        case E_LOAD_TYPE_SINGLE_OS08A20_PANO_DUAL_OS04A10: {
            return "Single_OS08A20_Dual_OS04A10.json";
        }
        default: {
            return "Dual_OS08A20.json";
        }
    }
}
