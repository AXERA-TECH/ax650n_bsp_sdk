/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "NVRConfigParser.h"
#include "AppLogApi.h"

#ifndef AX_MIN
#define AX_MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

AX_BOOL CNVRConfigParser::InitOnce(AX_VOID) {
    string strIniPath = GetExecPath() + "/config/nvr.conf";
    if (!m_iniConfig.Load(strIniPath)) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_NVR_DEVICE_CONFIG_T CNVRConfigParser::GetDeviceConfig(AX_VOID) {
    AX_NVR_DEVICE_CONFIG_T conf;
    const AX_CHAR *SECT = "DEVICE";

    conf.strPath = m_iniConfig.GetStringValue(SECT, "path", "./config/remote_device.json");

    return conf;
}

AX_NVR_RECORD_CONFIG_T CNVRConfigParser::GetRecordConfig(AX_VOID) {
    AX_NVR_RECORD_CONFIG_T conf;
    const AX_CHAR *SECT = "RECORD";

    conf.strPath = m_iniConfig.GetStringValue(SECT, "path", "");
    conf.uMaxDevSpace = m_iniConfig.GetIntValue(SECT, "max space", 128);
    conf.uMaxFilePeriod = m_iniConfig.GetIntValue(SECT, "max period", 1);
    // Do not support IDR backward playback anymore.
    // conf.bOnlyIFrameOnReverse = m_iniConfig.GetIntValue(SECT, "iframe only", 1) ? AX_TRUE : AX_FALSE;
    conf.bOnlyIFrameOnReverse = AX_FALSE;

    return conf;
}

AX_NVR_RPATROL_CONFIG_T CNVRConfigParser::GetRoundPatrolConfig(AX_VOID) {
    AX_NVR_RPATROL_CONFIG_T conf;
    const AX_CHAR *SECT = "ROUND_PATROL";

    conf.bEnable = (AX_BOOL)m_iniConfig.GetIntValue(SECT, "enable", 0);
    conf.enType = (AX_NVR_RPATROL_TYPE)m_iniConfig.GetIntValue(SECT, "type", 0);
    conf.enSpliteType = (AX_NVR_VO_SPLITE_TYPE)m_iniConfig.GetIntValue(SECT, "split", 1);
    conf.uIntelval = m_iniConfig.GetIntValue(SECT, "interval", 5);
    conf.nStrategy = m_iniConfig.GetIntValue(SECT, "strategy", 0);

    return conf;
}

AX_VOID CNVRConfigParser::SetRoundPatrolConfig(AX_NVR_RPATROL_CONFIG_T &conf) {
    const AX_CHAR *SECT = "ROUND_PATROL";
    m_iniConfig.SetIntValue(SECT, "enable", conf.bEnable);
    m_iniConfig.SetIntValue(SECT, "type", (AX_U32)conf.enType);
    m_iniConfig.SetIntValue(SECT, "split", (AX_U32)conf.enSpliteType);
    m_iniConfig.SetIntValue(SECT, "interval", conf.uIntelval);
    m_iniConfig.SetIntValue(SECT, "strategy", conf.nStrategy);
}

AX_NVR_DETECT_CONFIG_T CNVRConfigParser::GetDetectConfig(AX_VOID) {
    AX_NVR_DETECT_CONFIG_T conf;
    const AX_CHAR *SECT = "DETECT";

    conf.bEnable = (AX_BOOL)m_iniConfig.GetIntValue(SECT, "enable", 0);
    conf.nW = m_iniConfig.GetIntValue(SECT, "width", 960);
    conf.nH = m_iniConfig.GetIntValue(SECT, "height", 640);
    conf.nSkipRate = m_iniConfig.GetIntValue(SECT, "skip rate", 3);
    conf.nDepth = m_iniConfig.GetIntValue(SECT, "fifo depth", 1);
    conf.nChannelNum = m_iniConfig.GetIntValue(SECT, "channel num", 1);
    conf.nChannelNum = AX_MIN(conf.nChannelNum, 3);

    for (AX_S32 i = 0; i < conf.nChannelNum; ++i) {
        std::string str = "channel" + std::to_string(i) + " attr";

        vector<AX_S32> vec;
        m_iniConfig.GetIntValue(SECT, str, vec);

        if (vec.size() == 3) {
            conf.tChnParam[i].nPPL = vec[0];
            conf.tChnParam[i].bTrackEnable = (AX_BOOL)vec[1];
            conf.tChnParam[i].nVNPU = vec[2];
        } else {
            conf.tChnParam[i].nPPL = 4;
            conf.tChnParam[i].bTrackEnable = AX_FALSE;
            conf.tChnParam[i].nVNPU = 0;
        }
    }

    conf.strModelPath = m_iniConfig.GetStringValue(SECT, "model path", "");

    return conf; /* RVO: optimized by compiler */
}

AX_NVR_DISPVO_CONFIG_T CNVRConfigParser::GetPrimaryDispConfig(AX_VOID) {
    AX_NVR_DISPVO_CONFIG_T conf;
    const AX_CHAR *SECT = "Primary";

    conf.nDevId = m_iniConfig.GetIntValue(SECT, "dev", 0);
    conf.nFBQt = m_iniConfig.GetIntValue(SECT, "fb qt", 0);
    conf.nFBRect = m_iniConfig.GetIntValue(SECT, "fb rects", -1);
    conf.bLink = m_iniConfig.GetIntValue(SECT, "link", 1) == 0 ? AX_FALSE : AX_TRUE;
    conf.nHDMI = m_iniConfig.GetIntValue(SECT, "HDMI", 10);
    conf.nLayerDepth = m_iniConfig.GetIntValue(SECT, "layer depth", 3);
    conf.nOnlineMode = m_iniConfig.GetIntValue(SECT, "online mode", 1);
    /* if 0, vo using default tolerance, VO_LAYER_TOLERATION_DEF = 10*1000*1000 */
    conf.nTolerance = m_iniConfig.GetIntValue(SECT, "tolerance", 0);

    return conf; /* RVO: optimized by compiler */
}

AX_NVR_DISPVO_CONFIG_T CNVRConfigParser::GetSecondaryDispConfig(AX_VOID) {
    AX_NVR_DISPVO_CONFIG_T conf;
    const AX_CHAR *SECT = "Secondary";

    conf.nDevId = m_iniConfig.GetIntValue(SECT, "dev", 0);
    conf.nFBQt = m_iniConfig.GetIntValue(SECT, "fb qt", 2);
    conf.nFBRect = m_iniConfig.GetIntValue(SECT, "fb rects", -1);
    conf.bLink = m_iniConfig.GetIntValue(SECT, "link", 1) == 0 ? AX_FALSE : AX_TRUE;
    conf.nHDMI = m_iniConfig.GetIntValue(SECT, "HDMI", 10);
    conf.nLayerDepth = m_iniConfig.GetIntValue(SECT, "layer depth", 3);
    /* if 0, vo using default tolerance, VO_LAYER_TOLERATION_DEF = 10*1000*1000 */
    conf.nTolerance = m_iniConfig.GetIntValue(SECT, "tolerance", 0);

    return conf; /* RVO: optimized by compiler */
}

AX_NVR_DATA_STREAM_CONFIG_T CNVRConfigParser::GetDataStreamConfig(AX_VOID) {
    AX_NVR_DATA_STREAM_CONFIG_T conf;
    const AX_CHAR *SECT = "DATA_STREAM";

    conf.bSaveDisk = (AX_BOOL)m_iniConfig.GetIntValue(SECT, "save disk", 0);
    conf.nFrequency = m_iniConfig.GetIntValue(SECT, "frequency", 10);

    return conf; /* RVO: optimized by compiler */
}

AX_NVR_TEST_SUITE_CONFIG_T CNVRConfigParser::GetTestSuiteConfig(AX_VOID) {
    AX_NVR_TEST_SUITE_CONFIG_T conf;
    const AX_CHAR *SECT = "TEST_SUITE";

    conf.eMode = m_eTestSuiteMode;
    conf.strStabilityPath = m_iniConfig.GetStringValue(SECT, "path_stability", "./config/test_suite.json");
    conf.strUTPath = m_iniConfig.GetStringValue(SECT, "path_ut", "./config/test_suite.json");

    return conf; /* RVO: optimized by compiler */
}

AX_NVR_FBC_CONFIG_T CNVRConfigParser::GetFBCConfig(AX_VOID) {
    AX_NVR_FBC_CONFIG_T conf;
    const AX_CHAR *SECT = "FBC";

    conf.nLv = m_iniConfig.GetIntValue(SECT, "lossy level", 0);

    return conf; /* RVO: optimized by compiler */
}

string CNVRConfigParser::GetExecPath(AX_VOID) {
    string strPath;
    AX_CHAR szPath[260] = {0};
    ssize_t sz = readlink("/proc/self/exe", szPath, sizeof(szPath));
    if (sz <= 0) {
        strPath = "./";
    } else {
        strPath = szPath;
        strPath = strPath.substr(0, strPath.rfind('/') + 1);
    }

    return strPath;
}
