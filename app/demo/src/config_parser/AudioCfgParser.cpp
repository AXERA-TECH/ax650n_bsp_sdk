/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include <fstream>
#include "AudioCfgParser.h"
#include "AppLogApi.h"
#include "CommonUtils.hpp"

using namespace std;

#define AUDIO_PARSER "AUDIO_PARSER"

AX_BOOL CAudioCfgParser::InitOnce() {
    return AX_TRUE;
}

AX_BOOL CAudioCfgParser::GetConfig(AX_APP_AUDIO_ATTR_T& stAudioAttr, AX_APP_AUDIO_CFG_T& stAudioCfg) {
    string strConfigDir = CCommonUtils::GetPPLConfigDir();
    if (strConfigDir.empty()) {
        return AX_FALSE;
    }

    string strAudioCfgFile = strConfigDir + "/audio.json";

    ifstream ifs(strAudioCfgFile.c_str());
    picojson::value v;
    ifs >> v;

    string err = picojson::get_last_error();
    if (!err.empty()) {
        LOG_M_E(AUDIO_PARSER, "Failed to load json config file: %s", strAudioCfgFile.c_str());
        return AX_FALSE;
    }

    if (!v.is<picojson::object>()) {
        LOG_M_E(AUDIO_PARSER, "Loaded config file is not a well-formatted JSON.");
        return AX_FALSE;
    }

    if (!ParseFile(strAudioCfgFile, stAudioAttr, stAudioCfg)) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CAudioCfgParser::ParseFile(const string& strPath, AX_APP_AUDIO_ATTR_T& stAudioAttr, AX_APP_AUDIO_CFG_T& stAudioCfg) {
    picojson::value v;
    ifstream fIn(strPath.c_str());
    if (!fIn.is_open()) {
        return AX_TRUE;
    }

    string strParseRet = picojson::parse(v, fIn);
    if (!strParseRet.empty() || !v.is<picojson::object>()) {
        return AX_TRUE;
    }

    return ParseJson(v.get<picojson::object>(), stAudioAttr, stAudioCfg);
}

AX_BOOL CAudioCfgParser::ParseJson(picojson::object& objJsonRoot, AX_APP_AUDIO_ATTR_T& stAudioAttr, AX_APP_AUDIO_CFG_T& stAudioCfg) {
    // get audio enable
    if (objJsonRoot.end() == objJsonRoot.find("enable")) {
        return AX_TRUE;
    }

    if (!objJsonRoot["enable"].get<bool>()) {
        return AX_TRUE;
    }

    // dev common attribute
    ParseDevCommJson(objJsonRoot, stAudioAttr);

    // capture attribute
    ParseACapJson(objJsonRoot, stAudioAttr, stAudioCfg);

    // play attribute
    ParseAPlayJson(objJsonRoot, stAudioAttr, stAudioCfg);

    // welcome condif
    ParseWelcomeJson(objJsonRoot, stAudioCfg);

    return AX_TRUE;
}

AX_BOOL CAudioCfgParser::ParseDevCommJson(picojson::object& objJsonRoot, AX_APP_AUDIO_ATTR_T& stAudioAttr) {
    AX_BOOL bSucc = AX_TRUE;

    // get dev common attribute
    do {
        // dev_comm
        if (objJsonRoot.end() == objJsonRoot.find("dev_comm")) {
            bSucc = AX_FALSE;
            break;
        }

        picojson::object &objDevComm = objJsonRoot["dev_comm"].get<picojson::object>();

        // dev_comm.bit_width
        stAudioAttr.stDevCommAttr.eBitWidth = (AX_APP_AUDIO_BIT_WIDTH_E)objDevComm["bit_width"].get<double>();

        // dev_comm.sample_rate
        stAudioAttr.stDevCommAttr.eSampleRate = (AX_APP_AUDIO_SAMPLE_RATE_E)objDevComm["sample_rate"].get<double>();

        // dev_comm.period_size
        stAudioAttr.stDevCommAttr.nPeriodSize = objDevComm["period_size"].get<double>();
    } while(0);

    return bSucc;
}

AX_BOOL CAudioCfgParser::ParseACapJson(picojson::object& objJsonRoot, AX_APP_AUDIO_ATTR_T& stAudioAttr, AX_APP_AUDIO_CFG_T& stAudioCfg) {
    // get capture attribute
    do {
        // capture
        if (objJsonRoot.end() == objJsonRoot.find("capture")) {
            break;
        }

        picojson::object &objCapture = objJsonRoot["capture"].get<picojson::object>();

        // capture.enable
        if (objCapture.end() == objCapture.find("enable")) {
            break;
        }

        if (!objCapture["enable"].get<bool>()) {
            break;
        }

        stAudioAttr.stCapAttr.bEnable = (AX_BOOL)objCapture["enable"].get<bool>();

        // capture.dev
        if (objCapture.end() == objCapture.find("dev")) {
            break;
        }

        picojson::object &objDev = objCapture["dev"].get<picojson::object>();

        // capture.dev.instance[]
        picojson::array &arrDevInst = objDev["instance"].get<picojson::array>();
        for (size_t i = 0; i < arrDevInst.size(); i ++) {
            auto &stCapDevAttr = stAudioAttr.stCapAttr.stDevAttr;

            // capture.dev.instance[i].card_id
            stCapDevAttr.nCardId = arrDevInst[i].get<picojson::object>()["card_id"].get<double>();

            // capture.dev.instance[i].device_id
            stCapDevAttr.nDeviceId = arrDevInst[i].get<picojson::object>()["device_id"].get<double>();

            // capture.dev.instance[i].volume
            stCapDevAttr.fVolume = arrDevInst[i].get<picojson::object>()["volume"].get<double>();

            // capture.dev.instance[i].3A
            picojson::object &objDev3A = arrDevInst[i].get<picojson::object>()["3A"].get<picojson::object>();

            // capture.dev.instance[i].3A.agc.enable
            stCapDevAttr.stVqeAttr.stAgcAttr.bEnable = 
                                    (AX_BOOL)objDev3A["agc"].get<picojson::object>()["enable"].get<bool>();

            // capture.dev.instance[i].3A.agc.agc_mode
            stCapDevAttr.stVqeAttr.stAgcAttr.eAgcMode = 
                                    (AX_APP_AUDIO_AGC_MODE_E)objDev3A["agc"].get<picojson::object>()["agc_mode"].get<double>();

            // capture.dev.instance[i].3A.agc.target_level
            stCapDevAttr.stVqeAttr.stAgcAttr.nTargetLv = 
                                    (AX_S16)objDev3A["agc"].get<picojson::object>()["target_level"].get<double>();

            // capture.dev.instance[i].3A.agc.gain
            stCapDevAttr.stVqeAttr.stAgcAttr.nGain = 
                                    (AX_S16)objDev3A["agc"].get<picojson::object>()["gain"].get<double>();

            // capture.dev.instance[i].3A.ans.enable
            stCapDevAttr.stVqeAttr.stAnsAttr.bEnable = 
                                    (AX_BOOL)objDev3A["ans"].get<picojson::object>()["enable"].get<bool>();

            // capture.dev.instance[i].3A.ans.target_level
            stCapDevAttr.stVqeAttr.stAnsAttr.eLevel = 
                                    (AX_APP_AUDIO_ANS_AGGRESSIVENESS_LEVEL_E)objDev3A["ans"].get<picojson::object>()["target_level"].get<double>();

            // capture.dev.instance[i].3A.aec.aec_mode
            stCapDevAttr.stVqeAttr.stAecAttr.eType = 
                                    (AX_APP_AUDIO_AEC_TYPE_E)objDev3A["aec"].get<picojson::object>()["aec_mode"].get<double>();

            if (AX_APP_AUDIO_AEC_FLOAT == stCapDevAttr.stVqeAttr.stAecAttr.eType) {
                // capture.dev.instance[i].3A.aec.float_attr.level
                stCapDevAttr.stVqeAttr.stAecAttr.stFloatAttr.eLevel = 
                                        (AX_APP_AUDIO_AEC_SUPPRESSION_LEVEL_E)objDev3A["aec"].get<picojson::object>()["float_attr"].get<picojson::object>()["level"].get<double>();
            }
            else if (AX_APP_AUDIO_AEC_FIXED == stCapDevAttr.stVqeAttr.stAecAttr.eType) {
                // capture.dev.instance[i].3A.aec.fixed_attr.routing_mode
                stCapDevAttr.stVqeAttr.stAecAttr.stFixedAttr.eMode = 
                                        (AX_APP_AUDIO_AEC_ROUTING_MODE_E)objDev3A["aec"].get<picojson::object>()["fixed_attr"].get<picojson::object>()["routing_mode"].get<double>();
            }
        }

        // capture.pipe
        if (objCapture.end() == objCapture.find("pipe")) {
            break;
        }

        picojson::object &objPipe = objCapture["pipe"].get<picojson::object>();

        // capture.pipe.instance[]
        picojson::array &arrPipeInst = objPipe["instance"].get<picojson::array>();
        for (size_t i = 0; i < arrPipeInst.size(); i ++) {
            // capture.pipe.instance[i].enable
            if (!arrPipeInst[i].get<picojson::object>()["enable"].get<bool>()) {
                continue;
            }

            auto &stAudioChanAttr = stAudioAttr.stCapAttr.stPipeAttr.stAudioChanAttr[i];

            stAudioChanAttr.bEnable = (AX_BOOL)arrPipeInst[i].get<picojson::object>()["enable"].get<bool>();

            // capture.pipe.instance[i].encoder_type
            stAudioChanAttr.eType = (AX_PAYLOAD_TYPE_E)arrPipeInst[i].get<picojson::object>()["encoder_type"].get<double>();

            // capture.pipe.instance[i].bitrate
            stAudioChanAttr.nBitRate = arrPipeInst[i].get<picojson::object>()["bitrate"].get<double>();

            // capture.pipe.instance[i].sound_mode
            stAudioChanAttr.eSoundMode = (AX_APP_AUDIO_SOUND_MODE_E)arrPipeInst[i].get<picojson::object>()["sound_mode"].get<double>();

            if (PT_AAC == stAudioChanAttr.eType) {
                // capture.pipe.instance[i].aac_encoder.aac_type
                stAudioChanAttr.stAacEncoder.eAacType = 
                            (AX_APP_AUDIO_AAC_TYPE_E)arrPipeInst[i].get<picojson::object>()["aac_encoder"].get<picojson::object>()["aac_type"].get<double>();

                // capture.pipe.instance[i].aac_encoder.trans_type
                stAudioChanAttr.stAacEncoder.eTransType = 
                            (AX_APP_AUDIO_AAC_TRANS_TYPE_E)arrPipeInst[i].get<picojson::object>()["aac_encoder"].get<picojson::object>()["trans_type"].get<double>();
            }
            else {
                // capture.pipe.instance[i].default_encoder.reserved
                stAudioChanAttr.stDefEncoder.nReserved = 
                            arrPipeInst[i].get<picojson::object>()["default_encoder"].get<picojson::object>()["reserved"].get<double>();
            }

            stAudioCfg.bAudioCapAvailable = AX_TRUE;
        }

        // capture.pipe.rtsp_streaming_channel
        if (objPipe.end() != objPipe.find("rtsp_streaming_channel")) {
            stAudioCfg.eAudioRtspStreamingChannel = (AX_APP_AUDIO_CHAN_E)objPipe["rtsp_streaming_channel"].get<double>();
        }

        // capture.pipe.web_streaming_channel
        if (objPipe.end() != objPipe.find("web_streaming_channel")) {
            stAudioCfg.eAudioWebStreamingChannel = (AX_APP_AUDIO_CHAN_E)objPipe["web_streaming_channel"].get<double>();
        }

        // capture.pipe.mp4_recording_channel
        if (objPipe.end() != objPipe.find("mp4_recording_channel")) {
            stAudioCfg.eMp4RecordingChannel = (AX_APP_AUDIO_CHAN_E)objPipe["mp4_recording_channel"].get<double>();
        }
    } while(0);

    return AX_TRUE;
}

AX_BOOL CAudioCfgParser::ParseAPlayJson(picojson::object &objJsonRoot, AX_APP_AUDIO_ATTR_T& stAudioAttr, AX_APP_AUDIO_CFG_T& stAudioCfg) {
    // get play attribute
    do {
        // play
        if (objJsonRoot.end() == objJsonRoot.find("play")) {
            break;
        }

        picojson::object &objPlay = objJsonRoot["play"].get<picojson::object>();

        // play.enable
        if (objPlay.end() == objPlay.find("enable")) {
            break;
        }

        if (!objPlay["enable"].get<bool>()) {
            break;
        }

        stAudioAttr.stPlayAttr.bEnable = (AX_BOOL)objPlay["enable"].get<bool>();

        // play.dev
        if (objPlay.end() == objPlay.find("dev")) {
            break;
        }

        picojson::object &objDev = objPlay["dev"].get<picojson::object>();

        // play.dev.instance[]
        picojson::array &arrDevInst = objDev["instance"].get<picojson::array>();
        for (size_t i = 0; i < arrDevInst.size(); i ++) {
            auto &stPlayDevAttr = stAudioAttr.stPlayAttr.stDevAttr;

            // play.dev.instance[i].card_id
            stPlayDevAttr.nCardId = arrDevInst[i].get<picojson::object>()["card_id"].get<double>();

            // play.dev.instance[i].device_id
            stPlayDevAttr.nDeviceId = arrDevInst[i].get<picojson::object>()["device_id"].get<double>();

            // play.dev.instance[i].volume
            stPlayDevAttr.fVolume = arrDevInst[i].get<picojson::object>()["volume"].get<double>();

            // play.dev.instance[i].3A
            picojson::object &objDev3A = arrDevInst[i].get<picojson::object>()["3A"].get<picojson::object>();

            // play.dev.instance[i].3A.agc.enable
            stPlayDevAttr.stVqeAttr.stAgcAttr.bEnable = 
                                    (AX_BOOL)objDev3A["agc"].get<picojson::object>()["enable"].get<bool>();

            // play.dev.instance[i].3A.agc.agc_mode
            stPlayDevAttr.stVqeAttr.stAgcAttr.eAgcMode = 
                                    (AX_APP_AUDIO_AGC_MODE_E)objDev3A["agc"].get<picojson::object>()["agc_mode"].get<double>();

            // play.dev.instance[i].3A.agc.target_level
            stPlayDevAttr.stVqeAttr.stAgcAttr.nTargetLv = 
                                    (AX_S16)objDev3A["agc"].get<picojson::object>()["target_level"].get<double>();

            // play.dev.instance[i].3A.agc.gain
            stPlayDevAttr.stVqeAttr.stAgcAttr.nGain = 
                                    (AX_S16)objDev3A["agc"].get<picojson::object>()["gain"].get<double>();

            // play.dev.instance[i].3A.ans.enable
            stPlayDevAttr.stVqeAttr.stAnsAttr.bEnable = 
                                    (AX_BOOL)objDev3A["ans"].get<picojson::object>()["enable"].get<bool>();

            // play.dev.instance[i].3A.ans.target_level
            stPlayDevAttr.stVqeAttr.stAnsAttr.eLevel = 
                                    (AX_APP_AUDIO_ANS_AGGRESSIVENESS_LEVEL_E)objDev3A["ans"].get<picojson::object>()["target_level"].get<double>();
        }

        // play.pipe
        if (objPlay.end() == objPlay.find("pipe")) {
            break;
        }

        picojson::object &objPipe = objPlay["pipe"].get<picojson::object>();

        // play.pipe.instance[]
        picojson::array &arrPipeInst = objPipe["instance"].get<picojson::array>();
        for (size_t i = 0; i < arrPipeInst.size(); i ++) {
            // play.pipe.instance[i].enable
            if (!arrPipeInst[i].get<picojson::object>()["enable"].get<bool>()) {
                continue;
            }

            auto &stAudioChanAttr = stAudioAttr.stPlayAttr.stPipeAttr.stAudioChanAttr[i];

            stAudioChanAttr.bEnable = (AX_BOOL)arrPipeInst[i].get<picojson::object>()["enable"].get<bool>();

            // play.pipe.instance[i].decoder_type
            stAudioChanAttr.eType = (AX_PAYLOAD_TYPE_E)arrPipeInst[i].get<picojson::object>()["decoder_type"].get<double>();

            // play.pipe.instance[i].bitrate
            stAudioChanAttr.nBitRate = arrPipeInst[i].get<picojson::object>()["bitrate"].get<double>();

            // play.pipe.instance[i].sample_rate
            stAudioChanAttr.eSampleRate = (AX_APP_AUDIO_SAMPLE_RATE_E)arrPipeInst[i].get<picojson::object>()["sample_rate"].get<double>();

            // play.pipe.instance[i].sound_mode
            stAudioChanAttr.eSoundMode = (AX_APP_AUDIO_SOUND_MODE_E)arrPipeInst[i].get<picojson::object>()["sound_mode"].get<double>();

            if (PT_AAC == stAudioChanAttr.eType) {
                // play.pipe.instance[i].default_decoder.trans_type
                stAudioChanAttr.stAacDecoder.eTransType = 
                            (AX_APP_AUDIO_AAC_TRANS_TYPE_E)arrPipeInst[i].get<picojson::object>()["aac_decoder"].get<picojson::object>()["trans_type"].get<double>();
            }
            else {
                // play.pipe.instance[i].default_decoder.bitrate
                stAudioChanAttr.stDefDecoder.nReserved = 
                            arrPipeInst[i].get<picojson::object>()["default_decoder"].get<picojson::object>()["reserved"].get<double>();
            }

            stAudioCfg.bAudioPlayAvailable = AX_TRUE;
        }

        // play.pipe.web_talking_channel
        if (objPipe.end() != objPipe.find("web_talking_channel")) {
            stAudioCfg.eWebTalkingChannel = (AX_APP_AUDIO_CHAN_E)objPipe["web_talking_channel"].get<double>();
        }

        // play.pipe.local_playing_channel
        if (objPipe.end() != objPipe.find("local_playing_channel")) {
            stAudioCfg.eLocalPlayingChannel = (AX_APP_AUDIO_CHAN_E)objPipe["local_playing_channel"].get<double>();
        }
    } while(0);

    return AX_TRUE;
}

AX_BOOL CAudioCfgParser::ParseWelcomeJson(picojson::object& objJsonRoot, AX_APP_AUDIO_CFG_T& stAudioCfg) {
    // welcome config
    do {
        stAudioCfg.stWelcome.bEnable = AX_FALSE;

        // welcome
        if (objJsonRoot.end() == objJsonRoot.find("welcome")) {
            break;
        }

        picojson::object &objWelcome = objJsonRoot["welcome"].get<picojson::object>();

        // welcome.enable
        if (objWelcome.end() == objWelcome.find("enable")) {
            break;
        }

        stAudioCfg.stWelcome.bEnable = (AX_BOOL)objWelcome["enable"].get<bool>();

        if (!stAudioCfg.stWelcome.bEnable) {
            break;
        }

        // welcome.voice_type
        if (objWelcome.end() != objWelcome.find("voice_type")) {
            stAudioCfg.stWelcome.eType = (AX_PAYLOAD_TYPE_E)objWelcome["voice_type"].get<double>();
        }

        // welcome.file_name
        if (objWelcome.end() != objWelcome.find("file_name")) {
            stAudioCfg.stWelcome.strFileName = objWelcome["file_name"].get<std::string>();
        }
    } while(0);

    return AX_TRUE;
}
