/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#pragma once

#include <string>
#include "AXSingleton.h"
#include "PanoAVSCfgParser.h"

#define APP_AVS_ATTR() CAVSOptionHelper::GetInstance()->GetAVSCfg()
#define APP_AVS_PIPE_NUM() CAVSOptionHelper::GetInstance()->GetAVSPipeNum()
#define APP_AVS_IS_SYNC_PIPE() CAVSOptionHelper::GetInstance()->GetAVSSyncPipe()
#define APP_AVS_MODE() CAVSOptionHelper::GetInstance()->GetAVSMode()
#define APP_AVS_IS_DYNAMIC_SEAM() CAVSOptionHelper::GetInstance()->GetAVSDynamicSeam()
#define APP_AVS_BLEND_MODE() CAVSOptionHelper::GetInstance()->GetAVSBlendMode()
#define APP_AVS_PARAM_TYPE() CAVSOptionHelper::GetInstance()->GetAVSParamType()
#define APP_AVS_PROJECTION_TYPE() CAVSOptionHelper::GetInstance()->GetAVSProjectionType();
#define APP_AVS_PARAM_FILE_PATH() CAVSOptionHelper::GetInstance()->GetAVSParamFilePath()

#define SET_APP_AVS_ATTR(_Attr_) CAVSOptionHelper::GetInstance()->SetAVSCfg(_Attr_)

/**
 * Load configuration
 */
class CAVSOptionHelper final : public CAXSingleton<CAVSOptionHelper> {
    friend class CAXSingleton<CAVSOptionHelper>;

public:
    const AX_APP_AVS_CFG_T& GetAVSCfg(AX_VOID);
    AX_U8 GetAVSPipeNum(AX_VOID);
    AX_BOOL GetAVSSyncPipe(AX_VOID);
    AX_AVS_MODE_E GetAVSMode(AX_VOID);
    AX_BOOL GetAVSDynamicSeam(AX_VOID);
    AX_AVS_BLEND_MODE_E GetAVSBlendMode(AX_VOID);
    std::string GetAVSParamFilePath(AX_VOID);
    PANO_AVS_PARAM_TYPE_E GetAVSParamType(AX_VOID);
    AX_AVS_PROJECTION_MODE_E GetAVSProjectionType(AX_VOID);

    AX_VOID SetAVSCfg(const AX_APP_AVS_CFG_T& stAttr);

private:
    CAVSOptionHelper(AX_VOID) = default;
    ~CAVSOptionHelper(AX_VOID) = default;

    AX_BOOL InitOnce() override;

private:
    AX_APP_AVS_CFG_T m_stAVSCfg { 0 };
};
