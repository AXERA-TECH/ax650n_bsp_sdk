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
#include "ax_global_type.h"
#include "AXThread.hpp"
#include "framebufferPaint.hpp"

#include <memory>
#include <mutex>
#include <vector>
#include <map>
typedef struct FB_RECT_S {
    unsigned int x;
    unsigned int y;
    unsigned int w;
    unsigned int h;
} FB_RECT_T;

typedef struct {
    const AX_CHAR *pFBPath{nullptr};  // device path
    AX_U32 nWidth{0};
    AX_U32 nHeight{0};
    AX_U32 nStride{0};
    AX_IMG_FORMAT_E enPixFmt{AX_FORMAT_INVALID};
    AX_S32 nGroup{-1}; // work thread number
    AX_U32 nColorDraw{0x00};
    AX_U32 uKeyColor{0x10101};
    AX_U32 nTimeIntervalPanMS{10};
} AX_NVR_FB_INIT_PARAM_T;


/**
 * @brief for framebuffer draw rect by double buffer.
 *
 */
class CFramebufferPaint {
public:
    CFramebufferPaint(AX_VOID);
    ~CFramebufferPaint(AX_VOID);

public:
    using RectList = std::vector<FB_RECT_T>;

public:
    // Only do once
    AX_BOOL Init(const AX_NVR_FB_INIT_PARAM_T& stInitParam);

    // Destroy all channels and free device.
    AX_BOOL DeInit(AX_VOID);

    /**
     * @brief launch work threads.
     *
     * @return AX_VOID
     */
    AX_VOID Start(AX_VOID);

    /**
     * @brief terminate working threads.
     *
     * @return AX_VOID
     */
    AX_VOID Stop(AX_VOID);

    /**
     * @brief Create a Channel list
     *
     * @param nChn channel ID
     * @param rect channel draw area
     */
    AX_VOID CreateChannel(AX_U8 nChn, const FB_RECT_T &rect, AX_BOOL bEraseBackground=AX_FALSE);

    /**
     * @brief Destroy target channel.
     *
     * @param nChn channel ID
     * @return success
     */
    AX_BOOL DestroyChannel(AX_U8 nChn);

    /**
     * @brief Destroy all channels.
     *
     */
    AX_VOID DestroyAllChannel(AX_VOID);

    /**
     * @brief clear screen, should not call after uninit.
     *
     */
    AX_VOID ClearScreen(AX_VOID);

    /**
     * @brief draw a group rect on target channel.
     * replace all rects in module cache.
     *
     * @param nChn target channel id.
     * @param rects rect list to paint
     * @return AX_BOOL
     */
    AX_BOOL DrawChannelRects(AX_U8 nChn, const RectList &rects);

    /**
     * @brief clear all rect which belong to nChn.
     *
     * @param nChn target channel id.
     * @return AX_BOOL
     */
    AX_BOOL ClearChannelRects(AX_U8 nChn);

    /**
     * @brief clear all channel rects in cache
     *
     */
    AX_VOID ClearAllChannelRects(AX_VOID);

private:
    AX_BOOL InitDevice(AX_VOID);
    AX_BOOL DeinitDevice(AX_VOID);

    AX_VOID MainThreadEntry(AX_VOID);
    AX_VOID TranslateRectsFromChannelToScreen(AX_VOID);
    AX_VOID WaitForChildrenThreadPaintingCompleted(AX_VOID);
    AX_VOID FlushToPan(AX_VOID);
    AX_VOID PaintEntry(AX_U8 index);
    AX_VOID PaintToDevice(const FB_RECT_T &stRect);
    AX_VOID EraseBackgroundAndDrawChannelRects(AX_VOID);
    AX_VOID FillRectWithKeyColor(const FB_RECT_T &stRect, AX_BOOL bCheckQuit=AX_TRUE);

private:
    struct NVRFBContentImpl;
    std::unique_ptr<NVRFBContentImpl> m_spNVRFBContentImpl;
    AX_NVR_FB_INIT_PARAM_T m_stInitParam{0};
    std::string m_strFBPath;
    AX_BOOL m_bInit{AX_FALSE};
    AX_BOOL m_bShouldQuit{AX_FALSE};
    AX_BOOL m_bLaunched{AX_FALSE};

    // channel information data.
    struct NVRFB_CHANNEL {
        FB_RECT_T stArea;
        RectList vecChildRect;
        AX_U8 uThreadIndex{0};
        AX_BOOL bEraseBackground{AX_FALSE};
    };
    // channel id : channel information
    std::map<AX_U8, NVRFB_CHANNEL> m_mapChannel;

    // work thread number
    AX_U8 m_nThreadGroup{2};
    std::mutex m_mutex;
    std::vector<std::unique_ptr<CAXThread>> m_vecPaintThread;
    AX_BOOL m_bChannelsRectChannged{AX_FALSE};

    // rect list to paint which have been translate to screen postion.
    std::vector<RectList> m_vecRectToPaint;
    // channel id: rect list to paint which have been translate to screen postion.
    std::map<AX_U8, RectList> m_mapRectToPaintClearBackground;
    // paint thread status, and main thread will wait all children thread paint completed.
    std::vector<AX_BOOL> m_vecStatusCompleted;
    CAXThread m_mainThread;

    // enable paint child rect to FB
    AX_BOOL m_bSchedulePaint{AX_FALSE};
};  // class CFramebufferPaint
