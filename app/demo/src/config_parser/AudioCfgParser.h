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
#include "AudioWrapper.hpp"

typedef struct axAPP_AUDIO_WELCOME_CFG_T {
    AX_BOOL bEnable{AX_FALSE};
    AX_PAYLOAD_TYPE_E eType{PT_AAC};
    std::string strFileName;
} AX_APP_AUDIO_WELCOME_CFG_T, *AX_APP_AUDIO_WELCOME_CFG_PTR;

typedef struct axAPP_AUDIO_CFG_T {
    AX_BOOL bAudioCapAvailable{AX_FALSE};
    AX_BOOL bAudioPlayAvailable{AX_FALSE};
    AX_APP_AUDIO_CHAN_E eAudioRtspStreamingChannel{AX_APP_AUDIO_CHAN_BUTT};
    AX_APP_AUDIO_CHAN_E eAudioWebStreamingChannel{AX_APP_AUDIO_CHAN_BUTT};
    AX_APP_AUDIO_CHAN_E eMp4RecordingChannel{AX_APP_AUDIO_CHAN_BUTT};
    AX_APP_AUDIO_CHAN_E eWebTalkingChannel{AX_APP_AUDIO_CHAN_BUTT};
    AX_APP_AUDIO_CHAN_E eLocalPlayingChannel{AX_APP_AUDIO_CHAN_BUTT};
    AX_APP_AUDIO_WELCOME_CFG_T stWelcome;
} AX_APP_AUDIO_CFG_T, *AX_APP_AUDIO_CFG_PTR;

class CAudioCfgParser : public CAXSingleton<CAudioCfgParser> {
    friend class CAXSingleton<CAudioCfgParser>;

public:
    AX_BOOL GetConfig(AX_APP_AUDIO_ATTR_T& stAudioAttr, AX_APP_AUDIO_CFG_T& stAudioCfg);

private:
    CAudioCfgParser(AX_VOID) = default;
    ~CAudioCfgParser(AX_VOID) = default;

    AX_BOOL InitOnce() override;
    AX_BOOL ParseFile(const std::string& strPath, AX_APP_AUDIO_ATTR_T& stAudioAttr, AX_APP_AUDIO_CFG_T& m_stAudioCfg);
    AX_BOOL ParseJson(picojson::object& objJsonRoot, AX_APP_AUDIO_ATTR_T& stAudioAttr, AX_APP_AUDIO_CFG_T& m_stAudioCfg);
    AX_BOOL ParseDevCommJson(picojson::object& objJsonRoot, AX_APP_AUDIO_ATTR_T& stAudioAttr);
    AX_BOOL ParseACapJson(picojson::object& objJsonRoot, AX_APP_AUDIO_ATTR_T& stAudioAttr, AX_APP_AUDIO_CFG_T& m_stAudioCfg);
    AX_BOOL ParseAPlayJson(picojson::object& objJsonRoot, AX_APP_AUDIO_ATTR_T& stAudioAttr, AX_APP_AUDIO_CFG_T& m_stAudioCfg);
    AX_BOOL ParseWelcomeJson(picojson::object& objJsonRoot, AX_APP_AUDIO_CFG_T& m_stAudioCfg);
};
