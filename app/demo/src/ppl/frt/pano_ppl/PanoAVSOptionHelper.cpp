/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#include "PanoAVSCfgParser.h"
#include "PanoAVSOptionHelper.h"
#include "CommonUtils.hpp"

AX_BOOL CAVSOptionHelper::InitOnce() {
    return CAVSCfgParser::GetInstance()->GetConfig(m_stAVSCfg);
}

const AX_APP_AVS_CFG_T& CAVSOptionHelper::GetAVSCfg() {
    return m_stAVSCfg;
}

AX_VOID CAVSOptionHelper::SetAVSCfg(const AX_APP_AVS_CFG_T& stCfg) {
    m_stAVSCfg = stCfg;
}

AX_U8 CAVSOptionHelper::GetAVSPipeNum(AX_VOID) {
    return m_stAVSCfg.u8PipeNum;
}

AX_BOOL CAVSOptionHelper::GetAVSSyncPipe(AX_VOID) {
    return m_stAVSCfg.bSyncPipe;
}

AX_AVS_MODE_E CAVSOptionHelper::GetAVSMode(AX_VOID) {
    return (AX_AVS_MODE_E)m_stAVSCfg.u8Mode;
}

AX_BOOL CAVSOptionHelper::GetAVSDynamicSeam(AX_VOID) {
    return m_stAVSCfg.bDynamicSeam;
}

AX_AVS_BLEND_MODE_E CAVSOptionHelper::GetAVSBlendMode(AX_VOID) {
    return (AX_AVS_BLEND_MODE_E)m_stAVSCfg.u8BlendMode;
}

std::string CAVSOptionHelper::GetAVSParamFilePath(AX_VOID) {
    return m_stAVSCfg.strParamFilePath;
}

PANO_AVS_PARAM_TYPE_E CAVSOptionHelper::GetAVSParamType(AX_VOID) {
    return (PANO_AVS_PARAM_TYPE_E)m_stAVSCfg.u8ParamType;
}

AX_AVS_PROJECTION_MODE_E CAVSOptionHelper::GetAVSProjectionType(AX_VOID) {
    return (AX_AVS_PROJECTION_MODE_E)m_stAVSCfg.u8ProjectionType;
}