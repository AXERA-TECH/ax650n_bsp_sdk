/**********************************************************************************
 *
 * Copyright (c) 2019-2022 Beijing AXera Technology Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Beijing AXera Technology Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Beijing AXera Technology Co., Ltd.
 *
 **********************************************************************************/
#pragma once
#include <mutex>
#include <vector>
#include "AXFrame.hpp"
#include "AXSingleton.h"
#include "WebServer.h"
#include "ax_ives_api.h"

typedef struct {
    AX_OD_CHN_ATTR_T stAttr;
    AX_U64 nFrameProcessed;
} OD_AREA_ATTR_T;

class COD {
public:
    COD(AX_VOID) {
        InitOnce();
    }
    ~COD(AX_VOID) = default;

    AX_BOOL Startup(AX_U32 nFrameRate, AX_U32 nWidth, AX_U32 nHeight);
    AX_VOID Cleanup(AX_VOID);

    AX_VOID GetDefaultThresholdY(AX_S32 nSnsID, AX_U8 &nThrd, AX_U8 &nConfidence);

    /* add an area and return area id (equal to OD chn id), if returns -1 means fail */
    AX_S32 AddArea(AX_U32 x, AX_U32 y, AX_U32 w, AX_U32 h, AX_U32 nWidth, AX_U32 nHeight, AX_S32 nSnsID = -1);
    AX_BOOL RemoveArea(AX_S32 nAreaId);

    /* set threshold and confidence of Y */
    AX_BOOL SetThresholdY(AX_S32 nSnsId, AX_S32 nAreaId, AX_U8 nThrd, AX_U8 nConfidence);

    /* set lux threshold and delta diff threshold */
    AX_BOOL SetLuxThreshold(AX_S32 nSnsId, AX_S32 nAreaId, AX_U32 nThrd, AX_U32 nDiff);

    /* process and return od results */
    const std::vector<AX_U8> &ProcessFrame(const AX_U32 nSnsID, const CAXFrame *pFrame);

    AX_S32 GetAreaCount(AX_VOID) const {
        return m_vecAreas.size();
    }

private:
    AX_BOOL IsEqualArea(const AX_IVES_RECT_T &a, const AX_IVES_RECT_T &b) const {
        return ((a.u32X == b.u32X) && (a.u32Y == b.u32Y) && (a.u32W == b.u32W) && (a.u32H == b.u32H)) ? AX_TRUE : AX_FALSE;
    }

    AX_BOOL LoadConfig(AX_S32 nSnsID);
    AX_VOID DestoryAreas(AX_VOID);

    AX_BOOL InitOnce(AX_VOID) {
        return AX_TRUE;
    };

private:
    std::mutex m_mutx;
    std::vector<OD_AREA_ATTR_T *> m_vecAreas;
    std::vector<AX_U8> m_vecRslts;
    AX_U32 m_nFrameRate{30};
    AX_U32 m_odImgW{0};
    AX_U32 m_odImgH{0};
    AX_BOOL m_bInited{AX_FALSE};
};
