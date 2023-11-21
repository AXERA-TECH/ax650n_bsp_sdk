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
#include <stdio.h>
#include "AppLogApi.h"
#include "istream.hpp"

class CStreamObserver : public IStreamObserver {
public:
    CStreamObserver(AX_VOID) = default;

    AX_BOOL OnRecvStreamData(CONST STREAM_FRAME_T&) override {
        return AX_TRUE;
    }

    AX_BOOL OnRecvStreamInfo(CONST STREAM_INFO_T& stInfo) override {
        LOG_M_C("OBS", "%s tracks:", stInfo.strURL.c_str());
        for (auto&& kv : stInfo.tracks) {
            if (PT_H264 == kv.second.enPayload || PT_H265 == kv.second.enPayload) {
                LOG_M_C("OBS", "video: payload %d, profile %d level %d, num_ref_frames %d, %dx%d %d fps", kv.second.enPayload,
                        kv.second.info.stVideo.nProfile, kv.second.info.stVideo.nLevel, kv.second.info.stVideo.nNumRefs,
                        kv.second.info.stVideo.nWidth, kv.second.info.stVideo.nHeight, kv.second.info.stVideo.nFps);
            }
        }

        return AX_TRUE;
    }

    AX_VOID OnNotifyConnStatus(CONST AX_CHAR* pUrl, CONNECT_STATUS_E enStatus) override {
        switch (enStatus) {
            case CONNECTED:
                LOG_M_C("OBS", "%s is connected", pUrl);
                break;
            case RECONNECT:
                LOG_M_C("OBS", "reconnecting to %s", pUrl);
                break;
            case DISCONNECT:
                LOG_M_C("OBS", "%s is disconnected", pUrl);
                break;
        }
    }
};