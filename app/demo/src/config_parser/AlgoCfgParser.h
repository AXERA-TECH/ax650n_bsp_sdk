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

#include "picojson.h"
#include "AXSingleton.h"
#include "AXAlgo.hpp"

class CAlgoCfgParser : public CAXSingleton<CAlgoCfgParser> {
    friend class CAXSingleton<CAlgoCfgParser>;

public:
    AX_BOOL GetConfig(AX_APP_ALGO_PARAM_T stAlgoParam[AX_APP_ALGO_SNS_MAX],
                        AX_APP_ALGO_AUDIO_PARAM_T& stAudioParam);

private:
    CAlgoCfgParser(AX_VOID) = default;
    ~CAlgoCfgParser(AX_VOID) = default;

    AX_BOOL InitOnce() override;
    AX_BOOL ParseFile(const std::string& strPath,
                        AX_APP_ALGO_PARAM_T stAlgoParam[AX_APP_ALGO_SNS_MAX],
                        AX_APP_ALGO_AUDIO_PARAM_T& stAudioParam);
    AX_BOOL ParseJson(picojson::object& objJsonRoot,
                        AX_APP_ALGO_PARAM_T stAlgoParam[AX_APP_ALGO_SNS_MAX],
                        AX_APP_ALGO_AUDIO_PARAM_T& stAudioParam);
    AX_BOOL ParseSkelJson(picojson::object& objJsonRoot, AX_APP_ALGO_PARAM_T& stAlgoParam);
    AX_BOOL ParseIvesJson(picojson::object& objJsonRoot, AX_APP_ALGO_PARAM_T& stAlgoParam);
    AX_BOOL ParseAudioJson(picojson::object& objJsonRoot, AX_APP_ALGO_AUDIO_PARAM_T& stAudioParam);

private:
    AX_APP_ALGO_PARAM_T m_stAlgoParam[AX_APP_ALGO_SNS_MAX];
    AX_APP_ALGO_AUDIO_PARAM_T m_stAudioParam;
};
