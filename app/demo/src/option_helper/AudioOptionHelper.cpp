/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "AudioCfgParser.h"
#include "AudioOptionHelper.h"
#include "CommonUtils.hpp"

AX_BOOL CAudioOptionHelper::InitOnce() {
    memset(&m_stAudioAttr, 0x00, sizeof(m_stAudioAttr));

    return CAudioCfgParser::GetInstance()->GetConfig(m_stAudioAttr, m_stAudioCfg);
}

const AX_APP_AUDIO_ATTR_T& CAudioOptionHelper::GetAudioAttr() {
    return m_stAudioAttr;
}

AX_VOID CAudioOptionHelper::SetAudioAttr(const AX_APP_AUDIO_ATTR_T& stAttr) {
    m_stAudioAttr = stAttr;
}

AX_BOOL CAudioOptionHelper::GetAudioAvailable() {
    return (AX_BOOL)(m_stAudioCfg.bAudioCapAvailable || m_stAudioCfg.bAudioPlayAvailable);
}

AX_BOOL CAudioOptionHelper::GetAudioCapAvailable() {
    return m_stAudioCfg.bAudioCapAvailable;
}

AX_BOOL CAudioOptionHelper::GetAudioPlayAvailable() {
    return m_stAudioCfg.bAudioPlayAvailable;
}

AX_APP_AUDIO_CHAN_E CAudioOptionHelper::GetAudioRtspStreamingChannel() {
    return m_stAudioCfg.eAudioRtspStreamingChannel;
}

AX_APP_AUDIO_CHAN_E CAudioOptionHelper::GetAudioWebStreamingChannel() {
    return m_stAudioCfg.eAudioWebStreamingChannel;
}

AX_APP_AUDIO_CHAN_E CAudioOptionHelper::GetMp4RecordingChannel() {
    return m_stAudioCfg.eMp4RecordingChannel;
}

AX_APP_AUDIO_CHAN_E CAudioOptionHelper::GetWebTalkingChannel() {
    return m_stAudioCfg.eWebTalkingChannel;
}

AX_APP_AUDIO_CHAN_E CAudioOptionHelper::GetLocalPlayingChannel() {
    return m_stAudioCfg.eLocalPlayingChannel;
}

AX_APP_AUDIO_WELCOME_CFG_T& CAudioOptionHelper::GetWelcomeConfig() {
    return m_stAudioCfg.stWelcome;
}
