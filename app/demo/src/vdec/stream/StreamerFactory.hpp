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
#include <algorithm>
#include "AXSingleton.h"
#include "FileStreamer.hpp"
#ifndef __RTSP_UNSUPPORT__
#include "RtspStreamer.hpp"
#endif
#include "make_unique.hpp"

/**
 * @brief
 *
 */
class CStreamerFactory : public CAXSingleton<CStreamerFactory> {
    friend class CAXSingleton<CStreamerFactory>;

public:
    IStreamerHandlerPtr CreateHandler(const std::string& strPath) noexcept {
        std::string s = strPath;
        std::transform(s.begin(), s.end(), s.begin(), (int (*)(int))tolower);
        STREAM_TYPE_E eType = (s.find("rtsp:") != std::string::npos) ? STREAM_TYPE_E::RTSP : STREAM_TYPE_E::FILE;
        switch (eType) {
            case STREAM_TYPE_E::FILE:
                return std::make_unique<CFileStreamer>();
#ifndef __RTSP_UNSUPPORT__
            case STREAM_TYPE_E::RTSP:
                return std::make_unique<CRtspStreamer>();
#endif
            default:
                break;
        }

        return nullptr;
    }

private:
    CStreamerFactory(AX_VOID) noexcept = default;
    virtual ~CStreamerFactory(AX_VOID) = default;
};
