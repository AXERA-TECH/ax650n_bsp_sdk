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
#include "AvsCfgParser.h"

#define APP_AVS_ATTR()            CAvsOptionHelper::GetInstance()->GetAvsCfg()
#define APP_AVS_PIPE_NUM()        CAvsOptionHelper::GetInstance()->GetAvsPipeNum()
#define APP_AVS_IS_SYNC_PIPE()    CAvsOptionHelper::GetInstance()->GetAvsSyncPipe()
#define APP_AVS_MODE()            CAvsOptionHelper::GetInstance()->GetAvsMode()
#define APP_AVS_IS_DYNAMIC_SEAM() CAvsOptionHelper::GetInstance()->GetAvsDynamicSeam()
#define APP_AVS_BLEND_MODE()      CAvsOptionHelper::GetInstance()->GetAvsBlendMode()
#define APP_AVS_PARAM_TYPE()      CAvsOptionHelper::GetInstance()->GetAvsParamType()
#define APP_AVS_PROJECTION_TYPE() CAvsOptionHelper::GetInstance()->GetAvsProjectionType()
#define APP_AVS_PARAM_FILE_PATH() CAvsOptionHelper::GetInstance()->GetAvsParamFilePath()

#define SET_APP_AVS_ATTR(_Attr_)  CAvsOptionHelper::GetInstance()->SetAvsCfg(_Attr_)

/**
 * Load configuration
 */
class CAvsOptionHelper final : public CAXSingleton<CAvsOptionHelper> {
    friend class CAXSingleton<CAvsOptionHelper>;

public:
    const AX_APP_AVS_CFG_T& GetAvsCfg(AX_VOID);
    AX_U8 GetAvsPipeNum(AX_VOID);
    AX_BOOL GetAvsSyncPipe(AX_VOID);
    AX_AVS_MODE_E GetAvsMode(AX_VOID);
    AX_BOOL GetAvsDynamicSeam(AX_VOID);
    AX_AVS_BLEND_MODE_E GetAvsBlendMode(AX_VOID);
    std::string GetAvsParamFilePath(AX_VOID);
    AX_APP_AVS_PARAM_TYPE_E GetAvsParamType(AX_VOID);
    AX_AVS_PROJECTION_MODE_E GetAvsProjectionType(AX_VOID);

    AX_VOID SetAvsCfg(const AX_APP_AVS_CFG_T& stAvsCfg);

private:
    CAvsOptionHelper(AX_VOID) = default;
    ~CAvsOptionHelper(AX_VOID) = default;

    AX_BOOL InitOnce() override;

private:
    AX_APP_AVS_CFG_T m_stAvsCfg {0};
};
