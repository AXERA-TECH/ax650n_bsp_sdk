/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#pragma once

#include <map>
#include "AXSingleton.h"
#include "VideoEncoder.h"
#include "JpegEncoder.h"

#define APP_VENC_CONFIG(nScenario, nIndex, tOutConfig) \
        CEncoderOptionHelper::GetInstance()->GetVencConfig(nScenario, nIndex, tOutConfig)
#define APP_JENC_CONFIG(nScenario, nIndex, tOutConfig) \
        CEncoderOptionHelper::GetInstance()->GetJencConfig(nScenario, nIndex, tOutConfig)

/**
 * Load configuration
 */
class CEncoderOptionHelper final : public CAXSingleton<CEncoderOptionHelper> {
    friend class CAXSingleton<CEncoderOptionHelper>;

public:
    AX_BOOL GetVencConfig(AX_U8 nScenario, AX_U32 nIndex, VIDEO_CONFIG_T& tOutConfig);
    AX_BOOL GetJencConfig(AX_U8 nScenario, AX_U32 nIndex, JPEG_CONFIG_T& tOutConfig);

private:
    CEncoderOptionHelper(AX_VOID) = default;
    ~CEncoderOptionHelper(AX_VOID) = default;

    AX_BOOL InitOnce() override;

private:
    std::map<AX_U8, std::map<AX_U8, VIDEO_CONFIG_T>> m_mapScenario2VencChnConfig;
    std::map<AX_U8, std::map<AX_U8, JPEG_CONFIG_T>> m_mapScenario2JencChnConfig;
};
