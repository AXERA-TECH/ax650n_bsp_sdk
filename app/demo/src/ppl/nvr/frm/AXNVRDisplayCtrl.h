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
#include "ax_base_type.h"
#include "vo.hpp"
#include "framebufferPaint.hpp"
#include "frm/AXNVRFrameworkDefine.h"

#include <vector>
#include <tuple>
#include <random>

using namespace std;

const AX_U32 NVR_MAX_LAYOUT_NUM = 5;

typedef struct _AX_NVR_VO_INFO_T {
    AX_NVR_DISPDEV_TYPE enDispDevType = AX_NVR_DISPDEV_TYPE::PRIMARY;
    AX_U32 u32PreviewFifoDepth = 2; /* IN FIFO of VO */
    AX_U32 u32PlaybakFifoDepth = 2; /* IN FIFO of VO */
    AX_U32 u32Priority = 0;
    AX_F32 fFps = 60.0;
    VO_ATTR_T stVoAttr;
} AX_NVR_VO_INFO_T;

class CAXNVRDisplayCtrl {
public:
    CAXNVRDisplayCtrl(AX_VOID) = default;
    virtual ~CAXNVRDisplayCtrl(AX_VOID) = default;

    AX_BOOL Init(const AX_NVR_VO_INFO_T &stAttr);
    AX_VOID DeInit();

    AX_BOOL GetChannelRect(VO_CHN nChn, AX_NVR_RECT_T &rect);

    AX_VOID StartPipFBChannel(const AX_NVR_RECT_T &rect);
    AX_VOID StopPipFBChannel(AX_VOID);
    AX_BOOL EnablePip(const AX_NVR_RECT_T &rect);
    AX_BOOL DisablePip(AX_VOID);

    AX_BOOL UpdateView(const ax_nvr_channel_vector vecViewChns,
                    const vector<AX_NVR_RECT_T> vecVoRect,
                    AX_NVR_VIEW_TYPE enViewType,
                    AX_NVR_VIEW_CHANGE_TYPE enChangeType);
    AX_VOID StartFBChannels(AX_VOID);
    AX_VOID StopFBChannels(AX_VOID);

    AX_BOOL UpdateViewRound(AX_NVR_VIEW_CHANGE_TYPE enChangeType);
    AX_BOOL UpdateChnResolution(VO_CHN nChn, AX_U32 nWidth, AX_U32 nHeight);

    const CVO *GetVo() const { return m_pVo; };
    const CVOLayerRegionObserver *GetVoRegionObs() const { return m_pVoRegionObs; };
    const CFramebufferPaint *GetFBPaint() const { return m_pFbPaint; };
    const AX_NVR_VO_INFO_T& GetAttr() const { return m_stAttr; };

    void SetStartIndex(AX_U32 nStartndex) {
        m_nCurrentLayoutIndex = nStartndex;
        if (m_nCurrentLayoutIndex >= (AX_S32)NVR_MAX_LAYOUT_NUM) {
            m_nCurrentLayoutIndex = NVR_MAX_LAYOUT_NUM - 1;
        }
    };

    AX_U32 GetCurrentLayoutCnt() {
        AX_U32 nCurrentLayoutCnt = 1;
        switch (m_szLayoutVideos[m_nCurrentLayoutIndex])
        {
        case AX_NVR_VO_SPLITE_TYPE::ONE:
            nCurrentLayoutCnt = 1;
            break;
        case AX_NVR_VO_SPLITE_TYPE::FOUR:
            nCurrentLayoutCnt = 4;
            break;
        case AX_NVR_VO_SPLITE_TYPE::EIGHT:
            nCurrentLayoutCnt = 8;
            break;
        case AX_NVR_VO_SPLITE_TYPE::SIXTEEN:
            nCurrentLayoutCnt = 16;
            break;
        case AX_NVR_VO_SPLITE_TYPE::THIRTYSIX:
            nCurrentLayoutCnt = 36;
            break;
        default:
            break;
        }
        return nCurrentLayoutCnt;
    };

protected:
    std::mutex mutex_;

private:
    CVO *m_pVo = nullptr;
    CFramebufferPaint *m_pFbPaint = nullptr;
    AX_NVR_VO_INFO_T m_stAttr;
    vector<AX_NVR_RECT_T> m_vecRect;
    AX_NVR_RECT_T m_pipRect;
    CVOLayerRegionObserver* m_pVoRegionObs {nullptr};

    // polling vo config: 1<-->4<-->8<-->16<-->36
    map<AX_NVR_VO_SPLITE_TYPE, vector<AX_NVR_RECT_T>> m_mapPollingLayout;
    AX_NVR_VO_SPLITE_TYPE m_szLayoutVideos[NVR_MAX_LAYOUT_NUM] = {AX_NVR_VO_SPLITE_TYPE::ONE,
                                                                AX_NVR_VO_SPLITE_TYPE::FOUR,
                                                                AX_NVR_VO_SPLITE_TYPE::EIGHT,
                                                                AX_NVR_VO_SPLITE_TYPE::SIXTEEN,
                                                                AX_NVR_VO_SPLITE_TYPE::THIRTYSIX};
    AX_S32 m_nCurrentLayoutIndex = 0;
    AX_S32 m_nRoundPatrolDirection = 1;

    AX_S32 initFramebuffer(AX_U32 u32Width, AX_U32 u32Height, AX_U32 u32FbIndex);
    AX_BOOL initPollingLayout(AX_U32 u32Width, AX_U32 u32Height);
    const vector<AX_NVR_RECT_T> calcLayouts(AX_NVR_VO_SPLITE_TYPE enSplitType, int nWidth, int nHeight,
            int nLeftMargin, int nTopMargin, int nRightMargin, int nBottomMargin) const;
    AX_BOOL getDispInfoFromIntfSync(AX_VO_INTF_SYNC_E eIntfSync, AX_VO_RECT_T& stArea, AX_U32& nHz);
    AX_BOOL is4K(AX_VO_INTF_SYNC_E eIntfSync, AX_VO_RECT_T& stArea);
    AX_VOID createFbChannels(AX_VOID);
    AX_VOID destoryFbChannels(AX_VOID);
    AX_NVR_RECT_T mapRect(AX_NVR_RECT_T &rect1920x1080, int dst_width=3840, int dst_height=2160);
    AX_S32 getNextLayout();

    AX_BOOL m_bRoundPatrolStop = AX_FALSE;

};