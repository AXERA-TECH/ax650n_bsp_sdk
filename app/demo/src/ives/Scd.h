/**********************************************************************************
 *
 * Copyright (c) 2019-2022 Beijing AXera Technology Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Beijing AXera Technology Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Beijing AXera Technology Co., Ltd.~
 *
 **********************************************************************************/
#pragma once
#include "AXSingleton.h"
#ifndef SLT
#include "WebServer.h"
#endif
#include "AXFrame.hpp"
#include "ax_ives_api.h"

class CSCD {
public:
    CSCD(AX_VOID) {
        InitOnce();
    }
    ~CSCD(AX_VOID) = default;

    AX_VOID SetWebServer(CWebServer *pWebServer);
    AX_BOOL Startup(AX_U32 nWidth, AX_U32 nHeight, AX_S32 nSnsId = -1);
    AX_VOID Cleanup(AX_VOID);
    AX_BOOL ProcessFrame(const AX_U32 nSnsId, const CAXFrame *pFrame);
    AX_VOID SetThreshold(AX_U32 nSnsId, AX_S32 nThreshold, AX_S32 nConfidence);
    AX_VOID GetThreshold(AX_U32 nSnsId, AX_S32 &nThreshold, AX_S32 &nConfidence) const;

private:
    AX_BOOL LoadConfig(AX_S32 nSnsId);
    AX_BOOL InitOnce(AX_VOID) {
        return AX_TRUE;
    };

private:
    SCD_CHN m_chn{-1};
};
