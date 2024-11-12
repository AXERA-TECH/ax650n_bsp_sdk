/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "EncoderCfgParser.h"
#include "EncoderOptionHelper.h"
#include "CommonUtils.hpp"

#define ENC_OPTION_HELPER "ENC_HELPER"

AX_BOOL CEncoderOptionHelper::InitOnce() {
    return CEncoderCfgParser::GetInstance()->GetConfig(m_mapScenario2VencChnConfig, m_mapScenario2JencChnConfig);
}

AX_BOOL CEncoderOptionHelper::GetVencConfig(AX_U8 nScenario, AX_U32 nIndex, VIDEO_CONFIG_T &tOutConfig) {
    std::map<AX_U8, std::map<AX_U8, VIDEO_CONFIG_T>>::iterator itScenario = m_mapScenario2VencChnConfig.find(nScenario);
    if (m_mapScenario2VencChnConfig.end() == itScenario) {
        LOG_MM_E(ENC_OPTION_HELPER, "Scenario %d not found for venc", nScenario);
        return AX_FALSE;
    }

    std::map<AX_U8, VIDEO_CONFIG_T>::iterator itType = itScenario->second.find(nIndex);
    if (itScenario->second.end() == itType) {
        return AX_FALSE;
    }

    tOutConfig = itType->second;

    return AX_TRUE;
}

AX_BOOL CEncoderOptionHelper::GetJencConfig(AX_U8 nScenario, AX_U32 nIndex, JPEG_CONFIG_T &tOutConfig) {
    std::map<AX_U8, std::map<AX_U8, JPEG_CONFIG_T>>::iterator itScenario = m_mapScenario2JencChnConfig.find(nScenario);
    if (m_mapScenario2JencChnConfig.end() == itScenario) {
        return AX_FALSE;
    }

    std::map<AX_U8, JPEG_CONFIG_T>::iterator itType = itScenario->second.find(nIndex);
    if (itScenario->second.end() == itType) {
        return AX_FALSE;
    }

    tOutConfig = itType->second;

    return AX_TRUE;
}
