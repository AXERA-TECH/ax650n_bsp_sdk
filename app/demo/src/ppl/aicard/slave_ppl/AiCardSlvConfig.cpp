/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "AiCardSlvConfig.hpp"
#include <unistd.h>
#include "GlobalDef.h"
#include "ax_global_type.h"
#include "ax_venc_rc.h"
using namespace std;
using namespace aicard_slv;

string CAiCardSlvConfig::GetExecPath(AX_VOID) {
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

AX_BOOL CAiCardSlvConfig::Init(AX_VOID) {
    string strIniPath = GetExecPath() + "aicard_slave.conf";
    if (!m_iniParser.Load(strIniPath)) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

VDEC_CONFIG_T CAiCardSlvConfig::GetVdecConfig(AX_VOID) {
    VDEC_CONFIG_T conf;
    const AX_CHAR *SECT = "VDEC";

    conf.eVideoType = (m_iniParser.GetIntValue(SECT, "video type", 0) == 0 ? PT_H264 : PT_H265);
    conf.nMaxGrpW = m_iniParser.GetIntValue(SECT, "width", 1920);
    conf.nMaxGrpH = m_iniParser.GetIntValue(SECT, "height", 1080);
    conf.bEnableReset = (AX_BOOL)m_iniParser.GetIntValue(SECT, "enable reset", 0);
    for (AX_U32 i = 0; i < 3; ++i) {
        AX_CHAR szKey[32];
        sprintf(szKey, "chn%d depth", i);
        conf.nChnDepth[i] = m_iniParser.GetIntValue(SECT, szKey, 8);
    }
    conf.nDefaultFps = m_iniParser.GetIntValue(SECT, "fps", 30);
    conf.nInputMode = m_iniParser.GetIntValue(SECT, "input mode", 0);

    conf.nUserPool = m_iniParser.GetIntValue(SECT, "user pool", 1);
    if (conf.nUserPool > 2) {
        conf.nUserPool = 1;
    }

    conf.nMaxStreamBufSize = m_iniParser.GetIntValue(SECT, "max stream buf size", 0x200000);
    AX_U32 nCount = m_iniParser.GetIntValue(SECT, "count", 1);
    if (nCount > 0) {
        conf.v.resize(nCount);
        for (AX_U32 i = 1; i <= nCount; ++i) {
            AX_CHAR szKey[32];
            sprintf(szKey, "stream%02d", i);
            conf.v[i - 1] = m_iniParser.GetStringValue(SECT, szKey, "");
        }
    }

    conf.nDecodeGrps = m_iniParser.GetIntValue(SECT, "vdec count", 0);
    if (0 == conf.nDecodeGrps || conf.nDecodeGrps > nCount) {
        conf.nDecodeGrps = nCount;
    }

    return conf; /* RVO: optimized by compiler */
}

DETECT_CONFIG_T CAiCardSlvConfig::GetDetectConfig(AX_VOID) {
    DETECT_CONFIG_T conf;
    const AX_CHAR *SECT = "DETECT";

    conf.bEnable = (AX_BOOL)m_iniParser.GetIntValue(SECT, "enable", 0);
    conf.bEnableSimulator = (AX_BOOL)m_iniParser.GetIntValue(SECT, "enable simulator", 0);
    conf.nW = m_iniParser.GetIntValue(SECT, "width", 960);
    conf.nH = m_iniParser.GetIntValue(SECT, "height", 640);
    conf.nSkipRate = m_iniParser.GetIntValue(SECT, "skip rate", 1);
    conf.nDepth = m_iniParser.GetIntValue(SECT, "fifo depth", 1);
    conf.nChannelNum = m_iniParser.GetIntValue(SECT, "channel num", 1);
    conf.nChannelNum = AX_MIN(conf.nChannelNum, 3);

    for (AX_S32 i = 0; i < conf.nChannelNum; ++i) {
        std::string str = "channel" + std::to_string(i) + " attr";

        vector<AX_S32> vec;
        m_iniParser.GetIntValue(SECT, str, vec);

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

    conf.strModelPath = m_iniParser.GetStringValue(SECT, "model path", "");

    return conf; /* RVO: optimized by compiler */
}

PCIE_CONFIG_T CAiCardSlvConfig::GetPCIECofnig(AX_VOID) {
    PCIE_CONFIG_T conf;
    const AX_CHAR *SECT = "PCIE";

    conf.nBuffSize    = m_iniParser.GetIntValue(SECT, "buff size", 600); // Unit: KB
    conf.nBuffCount   = m_iniParser.GetIntValue(SECT, "buff count", 2);
    conf.nSendTimeout = m_iniParser.GetIntValue(SECT, "send timeout", -1);
    conf.nRecvTimeout = m_iniParser.GetIntValue(SECT, "recv timeout", -1);
    conf.nTraceData   = m_iniParser.GetIntValue(SECT, "log data", 0);
    conf.nRetryCount  = m_iniParser.GetIntValue(SECT, "retry count", 1);

    return conf; /* RVO: optimized by compiler */
}
