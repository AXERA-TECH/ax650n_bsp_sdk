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
#include <string>
#include <vector>
#include <atomic>
#include "AXFrame.hpp"
#include "ax_vo_api.h"

#define LOGO_CHN (0)
#define MAX_VO_CHN_NUM (AX_VO_CHN_MAX)

typedef struct VO_CHN_INFO_S {
    AX_U32 nDepth;
    AX_U32 nPriority;
} VO_CHN_INFO_T;

typedef struct VO_LOGO_INFO_S {
    AX_BOOL bShow;
    std::string strLogoPath;
    /* logo always shown at top of display area */
    AX_U32 nW;
    AX_U32 nH;
    VO_LOGO_INFO_S(AX_VOID) {
        bShow = AX_FALSE;
        nW = 0;
        nH = 0;
    }
} VO_LOGO_INFO_T;

typedef struct VO_ATTR_S {
    VO_DEV voDev;
    AX_VO_MODE_E enMode;
    VO_LAYER voLayer;
    AX_VO_INTF_TYPE_E enIntfType;
    AX_VO_INTF_SYNC_E enIntfSync;
    AX_U32 nW;
    AX_U32 nH;
    AX_U32 nHz;
    AX_U32 nBgClr;
    AX_U32 nLayerDepth;
    AX_U32 nTolerance;
    std::vector<VO_CHN_INFO_T> arrChns; /* video channels */
    std::string strResDirPath;          /* resource directory path */
    AX_BOOL bShowLogo;
    AX_BOOL bShowNoVideo;

    VO_ATTR_S(AX_VOID) {
        voDev = 0;
        enMode = AX_VO_MODE_OFFLINE;
        voLayer = 0;
        enIntfType = AX_VO_INTF_HDMI;
        enIntfSync = AX_VO_OUTPUT_1080P60;
        nW = 0;
        nH = 0;
        nHz = 0;
        nBgClr = 0x000000; /* RGB888 black */
        nLayerDepth = 0;
        nTolerance = 0;
        bShowLogo = AX_FALSE;
        bShowNoVideo = AX_FALSE;
    }
} VO_ATTR_T;

/*
 * @brief: no graphic layer, just video layer
 *
 */
class CVo {
public:
    CVo(AX_VOID) = default;

    AX_BOOL Init(const VO_ATTR_T& stAttr);
    AX_BOOL DeInit(AX_VOID);

    /* LOGO CHN: 0, others: nID + 1 */
    VO_CHN GetVideoChn(AX_U32 nID) const;
    AX_BOOL SetChnFrameRate(VO_CHN voChn, AX_U32 nFps);
    std::vector<AX_VO_RECT_T> GetVideoLayout(AX_VOID);

    AX_BOOL Start(AX_VOID);
    AX_BOOL Stop(AX_VOID);

    AX_BOOL SendFrame(VO_CHN voChn, CAXFrame& axFrame, AX_S32 nTimeOut = -1);
    const VO_ATTR_T& GetAttr(AX_VOID) const;
    VO_LAYER GetVideoLayer(AX_VOID) const;

protected:
    AX_BOOL InitLayout(AX_U32 nVideoCount);
    AX_VOID InitResource(AX_VOID);
    AX_BOOL IsLogoChn(VO_CHN voChn) const;
    AX_BOOL IsNoVideoChn(VO_CHN voChn) const;
    AX_BOOL GetDispInfoFromIntfSync(AX_VO_INTF_SYNC_E eIntfSync, AX_VO_RECT_T& stArea, AX_U32& nHz);

    AX_BOOL ShowLogo(AX_VOID);
    AX_BOOL ShowNoVideo(AX_VOID);
    AX_BOOL CreatePools(AX_U32 nLayerFifoDepth);

protected:
    VO_ATTR_T m_stAttr;
    AX_BOOL m_bInited{AX_FALSE};
    std::atomic<AX_BOOL> m_bStarted = {AX_FALSE};
    AX_U32 m_nVideoChnCount{0};
    std::vector<AX_VO_CHN_ATTR_T> m_arrChns;
    AX_POOL m_LayerPool{AX_INVALID_POOLID};

    /* logo and nonsignal resouce files */
    struct VO_RES_INFO_T {
        AX_BOOL bShow;
        std::string strPath;
        AX_U32 nW;
        AX_U32 nH;
        AX_IMG_FORMAT_E eImgFormat;
        AX_POOL pool;
        VO_RES_INFO_T(AX_VOID) {
            bShow = AX_FALSE;
            nW = 0;
            nH = 0;
            eImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
            pool = AX_INVALID_POOLID;
        }
    } m_rcLogo, m_rcNoVideo;
};

inline const VO_ATTR_T& CVo::GetAttr(AX_VOID) const {
    return m_stAttr;
}

inline VO_LAYER CVo::GetVideoLayer(AX_VOID) const {
    return m_stAttr.voLayer;
}

inline VO_CHN CVo::GetVideoChn(AX_U32 nID) const {
    return (VO_CHN)(m_rcLogo.bShow ? (nID + LOGO_CHN + 1) : nID);
}

inline AX_BOOL CVo::IsLogoChn(VO_CHN voChn) const {
    return (m_rcLogo.bShow && (LOGO_CHN == voChn)) ? AX_TRUE : AX_FALSE;
}

inline AX_BOOL CVo::IsNoVideoChn(VO_CHN voChn) const {
    return (voChn >= (m_rcLogo.bShow ? (m_nVideoChnCount + 1) : m_nVideoChnCount)) ? AX_TRUE : AX_FALSE;
}