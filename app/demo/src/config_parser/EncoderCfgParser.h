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
#include "picojson.h"
#include "AXSingleton.h"
#include "JpegEncoder.h"
#include "VideoEncoder.h"

class CEncoderCfgParser : public CAXSingleton<CEncoderCfgParser> {
    friend class CAXSingleton<CEncoderCfgParser>;

public:
    AX_BOOL GetConfig(std::map<AX_U8, std::map<AX_U8, VIDEO_CONFIG_T>>& mapScenario2VencChnConfig,
                        std::map<AX_U8, std::map<AX_U8, JPEG_CONFIG_T>>& mapScenario2JencChnConfig);

private:
    CEncoderCfgParser(AX_VOID) = default;
    ~CEncoderCfgParser(AX_VOID) = default;

    AX_BOOL InitOnce() override;
    AX_BOOL ParseFile(const std::string& strPath,
                        std::map<AX_U8, std::map<AX_U8, VIDEO_CONFIG_T>>& mapScenario2VencChnConfig,
                        std::map<AX_U8, std::map<AX_U8, JPEG_CONFIG_T>>& mapScenario2JencChnConfig);
    AX_BOOL ParseJson(picojson::object& objJsonRoot,
                        std::map<AX_U8, std::map<AX_U8, VIDEO_CONFIG_T>>& mapScenario2VencChnConfig,
                        std::map<AX_U8, std::map<AX_U8, JPEG_CONFIG_T>>& mapScenario2JencChnConfig);
};
