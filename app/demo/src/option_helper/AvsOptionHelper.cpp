/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#include "AvsCfgParser.h"
#include "AvsOptionHelper.h"
#include "CommonUtils.hpp"
#include "AppLogApi.h"

#define AVSOPTHELPER "AvsOptionHelper"

AX_BOOL CAvsOptionHelper::InitOnce() {
    return CAvsCfgParser::GetInstance()->GetConfig(m_stAvsCfg);
}

const AX_APP_AVS_CFG_T& CAvsOptionHelper::GetAvsCfg() {
    return m_stAvsCfg;
}

AX_VOID CAvsOptionHelper::SetAvsCfg(const AX_APP_AVS_CFG_T& stAvsCfg) {
    m_stAvsCfg = stAvsCfg;
}

AX_U8 CAvsOptionHelper::GetAvsPipeNum(AX_VOID) {
    return m_stAvsCfg.u8PipeNum;
}

AX_BOOL CAvsOptionHelper::GetAvsSyncPipe(AX_VOID) {
    return m_stAvsCfg.bSyncPipe;
}

AX_AVS_MODE_E CAvsOptionHelper::GetAvsMode(AX_VOID) {
    return (AX_AVS_MODE_E)m_stAvsCfg.u8Mode;
}

AX_BOOL CAvsOptionHelper::GetAvsDynamicSeam(AX_VOID) {
    return m_stAvsCfg.bDynamicSeam;
}

AX_AVS_BLEND_MODE_E CAvsOptionHelper::GetAvsBlendMode(AX_VOID) {
    return (AX_AVS_BLEND_MODE_E)m_stAvsCfg.u8BlendMode;
}

std::string CAvsOptionHelper::GetAvsParamFilePath(AX_VOID) {
    return m_stAvsCfg.strParamFilePath;
}

AX_APP_AVS_PARAM_TYPE_E CAvsOptionHelper::GetAvsParamType(AX_VOID) {
    return (AX_APP_AVS_PARAM_TYPE_E)m_stAvsCfg.u8ParamType;
}

AX_AVS_PROJECTION_MODE_E CAvsOptionHelper::GetAvsProjectionType(AX_VOID) {
    AX_AVS_PROJECTION_MODE_E eAvsPrjMode = AVS_PROJECTION_CYLINDRICAL;
    switch (m_stAvsCfg.u8ProjectionType) {
        case 0:
            eAvsPrjMode = AVS_PROJECTION_EQUIRECTANGULER;
            break;
        case 1:
            eAvsPrjMode = AVS_PROJECTION_RECTLINEAR;
            break;
        case 2:
            eAvsPrjMode = AVS_PROJECTION_CYLINDRICAL;
            break;
        case 3:
            eAvsPrjMode = AVS_PROJECTION_CUBE_MAP;
            break;
        default:
            LOG_M_W(AVSOPTHELPER, "Invalid projection type(%d), set to AVS_PROJECTION_CYLINDRICAL by default.",
                                   m_stAvsCfg.u8ProjectionType);
            break;
    }

    return eAvsPrjMode;
}
