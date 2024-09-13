/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "AiCardMstConfig.hpp"
#include <unistd.h>
#include "GlobalDef.h"
#include "ax_global_type.h"
#include "ax_venc_rc.h"
using namespace std;
using namespace aicard_mst;

string CAiCardMstConfig::GetExecPath(AX_VOID) {
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

AX_BOOL CAiCardMstConfig::Init(AX_VOID) {
    string strIniPath = GetExecPath() + "aicard_master.conf";
    if (!m_Ini.Load(strIniPath)) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

STREAM_CONFIG_T CAiCardMstConfig::GetStreamConfig(AX_VOID) {
    STREAM_CONFIG_T conf;
    const AX_CHAR *SECT = "STREAM";

    conf.nMaxGrpW = m_Ini.GetIntValue(SECT, "max width", 1920);
    conf.nMaxGrpH = m_Ini.GetIntValue(SECT, "max height", 1080);
    for (AX_U32 i = 0; i < 3; ++i) {
        AX_CHAR szKey[32];
        sprintf(szKey, "chn%d depth", i);
        conf.nChnDepth[i] = m_Ini.GetIntValue(SECT, szKey, 8);
    }
    conf.nDefaultFps = m_Ini.GetIntValue(SECT, "default fps", 30);
    conf.nInputMode = m_Ini.GetIntValue(SECT, "input mode", 0);

    conf.nUserPool = m_Ini.GetIntValue(SECT, "user pool", 1);
    if (conf.nUserPool > 2) {
        conf.nUserPool = 1;
    }

    conf.nMaxStreamBufSize = m_Ini.GetIntValue(SECT, "max stream buf size", 0x200000);
    AX_U32 nCount = m_Ini.GetIntValue(SECT, "count", 1);
    if (nCount > 0) {
        conf.v.resize(nCount);
        for (AX_U32 i = 1; i <= nCount; ++i) {
            AX_CHAR szKey[32];
            sprintf(szKey, "stream%02d", i);
            conf.v[i - 1] = m_Ini.GetStringValue(SECT, szKey, "");
        }
    }

    conf.nDecodeGrps = m_Ini.GetIntValue(SECT, "vdec count", 0);
    if (0 == conf.nDecodeGrps || conf.nDecodeGrps > nCount) {
        conf.nDecodeGrps = nCount;
    }

    return conf; /* RVO: optimized by compiler */
}

DISPVO_CONFIG_T CAiCardMstConfig::GetDispVoConfig(AX_VOID) {
    DISPVO_CONFIG_T conf;
    const AX_CHAR *SECT = "DISPC";

    conf.nDevId = m_Ini.GetIntValue(SECT, "dev", 0);
    conf.nHDMI = m_Ini.GetIntValue(SECT, "HDMI", 10);
    conf.nLayerDepth = m_Ini.GetIntValue(SECT, "layer depth", 3);
    /* if 0, vo using default tolerance, VO_LAYER_TOLERATION_DEF = 10*1000*1000 */
    conf.nTolerance = m_Ini.GetIntValue(SECT, "tolerance", 0);
    conf.bShowLogo = (AX_BOOL)m_Ini.GetIntValue(SECT, "show logo", 1);
    conf.bShowNoVideo = (AX_BOOL)m_Ini.GetIntValue(SECT, "show no video", 1);
    conf.strResDirPath = GetExecPath() + "res";
    conf.strBmpPath = conf.strResDirPath + "/font.bmp";

    return conf; /* RVO: optimized by compiler */
}

PCIE_CONFIG_T CAiCardMstConfig::GetPCIECofnig(AX_VOID) {
    PCIE_CONFIG_T conf;
    const AX_CHAR *SECT = "PCIE";

    conf.nSlaveCount  = m_Ini.GetIntValue(SECT, "slave count", 1);
    conf.nBuffSize    = m_Ini.GetIntValue(SECT, "buff size", 600);
    conf.nBuffCount   = m_Ini.GetIntValue(SECT, "buff count", 2);
    conf.nSendTimeout = m_Ini.GetIntValue(SECT, "send timeout", -1);
    conf.nRecvTimeout = m_Ini.GetIntValue(SECT, "recv timeout", -1);
    conf.nTraceData   = m_Ini.GetIntValue(SECT, "log data", 0);
    conf.nRetryCount  = m_Ini.GetIntValue(SECT, "retry count", 1);
    return conf; /* RVO: optimized by compiler */
}

