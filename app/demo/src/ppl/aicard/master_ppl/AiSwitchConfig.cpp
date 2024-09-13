/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "AiSwitchConfig.hpp"
#include <unistd.h>
#include "GlobalDef.h"
#include "AppLogApi.h"
#include "ax_global_type.h"

using namespace std;
using namespace aicard_mst;

#define AI_CFG "AI_SWITCH_CONFIG"


AX_BOOL CAiSwitchConfig::Init(AX_VOID) {
    string strIniPath = GetExecPath() + "ai_switch.conf";
    if (!m_iniParser.Load(strIniPath)) {
        return AX_FALSE;
    }

    if (!ParseConfig()) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CAiSwitchConfig::ParseConfig() {
    const AX_CHAR *STRGY = "STRATEGY";
    m_tAiSwitchStrgy.bEnable = m_iniParser.GetIntValue(STRGY, "enable", 0) == 1 ? AX_TRUE : AX_FALSE;
    m_tAiSwitchStrgy.eType = (AI_CARD_AI_SWITCH_TYPE_E)m_iniParser.GetIntValue(STRGY, "type", 0);
    m_tAiSwitchStrgy.nInterval = m_iniParser.GetIntValue(STRGY, "interval", 60);
    m_tAiSwitchStrgy.nAttrCount = m_iniParser.GetIntValue(STRGY, "count", 0);

    if (m_tAiSwitchStrgy.nAttrCount > MAX_SWITCH_CONFIG_NUM) {
        LOG_MM_W(AI_CFG, "AI attribute count %d is out of range(%d, %d), max value %d is set to count.", m_tAiSwitchStrgy.nAttrCount, 0, MAX_SWITCH_CONFIG_NUM, MAX_SWITCH_CONFIG_NUM);
        m_tAiSwitchStrgy.nAttrCount = MAX_SWITCH_CONFIG_NUM;
    }

    for (AX_U8 i = 0; i < m_tAiSwitchStrgy.nAttrCount; i++) {
        std::string strAttrSec = "AI_ATTR_GRP_" + std::to_string(i);
        string strModelPath = m_iniParser.GetStringValue(strAttrSec.c_str(), "model path", "");

        if (strModelPath.empty()) {
            LOG_MM_E(AI_CFG, "%s is not configured.", strAttrSec.c_str());
            return AX_FALSE;
        }

        strcpy(m_arrSwitchAttrConfig[i].szModelPath, strModelPath.c_str());
        m_arrSwitchAttrConfig[i].nChannelNum = m_iniParser.GetIntValue(strAttrSec.c_str(), "channel num", 0);

        for (AX_S32 j = 0; j < m_arrSwitchAttrConfig[i].nChannelNum; ++j) {
            std::string str = "channel" + std::to_string(j) + " attr";

            vector<AX_S32> vec;
            m_iniParser.GetIntValue(strAttrSec.c_str(), str, vec);

            if (vec.size() == 3) {
                m_arrSwitchAttrConfig[i].arrChnParam[j].nPPL = vec[0];
                m_arrSwitchAttrConfig[i].arrChnParam[j].bTrackEnable = (AX_BOOL)vec[1];
                m_arrSwitchAttrConfig[i].arrChnParam[j].nVNPU = vec[2];
            } else {
                m_arrSwitchAttrConfig[i].arrChnParam[j].nPPL = 4;
                m_arrSwitchAttrConfig[i].arrChnParam[j].bTrackEnable = AX_FALSE;
                m_arrSwitchAttrConfig[i].arrChnParam[j].nVNPU = 0;
            }
        }
    }

    return AX_TRUE;
}

AX_BOOL CAiSwitchConfig::IsEnabled() {
    return m_tAiSwitchStrgy.bEnable;
}

AX_S8 CAiSwitchConfig::GetAttrCount() {
    return m_tAiSwitchStrgy.nAttrCount;
}

AX_S32 CAiSwitchConfig::GetInterval() {
    return m_tAiSwitchStrgy.nInterval;
}

AX_BOOL CAiSwitchConfig::GetNextAttr(AI_CARD_AI_SWITCH_ATTR_T& tAttr) {
    m_nCurrAttrIndex = GetNextAttrIndex();
    if (-1 != m_nCurrAttrIndex) {
        tAttr = m_arrSwitchAttrConfig[m_nCurrAttrIndex];
        return AX_TRUE;
    }

    return AX_FALSE;
}

AX_S8 CAiSwitchConfig::GetNextAttrIndex() {
    if (AI_CARD_AI_SWITCH_TYPE_ORDERED == m_tAiSwitchStrgy.eType) {
        return (m_nCurrAttrIndex + 1) % m_tAiSwitchStrgy.nAttrCount;
    } else if (AI_CARD_AI_SWITCH_TYPE_RANDOM == m_tAiSwitchStrgy.eType) {
        AX_U8 nNextIndex = 0;
        while (m_tAiSwitchStrgy.nAttrCount > 1 && nNextIndex == m_nCurrAttrIndex) {
            nNextIndex = rand() % m_tAiSwitchStrgy.nAttrCount;
        }

        return nNextIndex;
    } else {
        LOG_MM_E(AI_CFG, "Unrecognized ai switch type: %d", m_tAiSwitchStrgy.eType);
    }

    return -1;
}

string CAiSwitchConfig::GetExecPath(AX_VOID) {
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
