/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in ay isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "framebufferPaint.hpp"
#include "AppLogApi.h"
#include "ElapsedTimer.hpp"

#include "ax_vo_api.h"

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <unistd.h>
#include <arm_neon.h>

#define TAG "FBPAINT"

namespace {
static AX_VOID RectOffset(FB_RECT_T &rectOut, const FB_RECT_T &rectIn, AX_U32 uMaxWidth, AX_U32 uMaxHeight) {
    if ((rectOut.x > (rectIn.x + rectIn.w)) ||
        (rectOut.y > (rectIn.y + rectIn.h))) {
        rectOut.w = 0;
        rectOut.h = 0;
        return;
    }

    if ((rectOut.x + rectOut.w) > (rectIn.x + rectIn.w) ) {
        rectOut.w = (rectIn.x + rectIn.w) - rectOut.x;
    }

    if ((rectOut.y + rectOut.h) > (rectIn.y + rectIn.h) ) {
        rectOut.h = (rectIn.y + rectIn.h) - rectOut.y;
    }
    rectOut.x += rectIn.x;
    rectOut.y += rectIn.y;

    if ((rectOut.x > uMaxWidth - 1) ||
        (rectOut.y > uMaxHeight - 1)) {
        rectOut.w = 0;
        rectOut.h = 0;
        return;
    }

    if ((rectOut.x + rectOut.w) > uMaxWidth) {
        rectOut.w = uMaxWidth - rectOut.x;
    }

    if ((rectOut.y + rectOut.h) > uMaxHeight) {
        rectOut.h = uMaxHeight - rectOut.y;
    }
}

static AX_U32 ConvertToDrawColorFromColorKey(AX_U32 uKeyColor, AX_IMG_FORMAT_E enPixFmt) {
    if (enPixFmt == AX_FORMAT_ARGB8888) {
        uKeyColor = (0xFF << 24) | (((uKeyColor >> 16) & 0xFF) << 16) |
                                  (((uKeyColor >> 8) & 0xFF) << 8) |
                                  (((uKeyColor >> 0) & 0xFF) << 0);
    } else {
        // AX_FORMAT_ARGB1555
        uKeyColor = (0x1 << 15) | (((uKeyColor >> 16) & 0x1F) << 10) |
                                  (((uKeyColor >> 8) & 0x1F) << 5) |
                                  (((uKeyColor >> 0) & 0x1F) << 0);
    }

    return uKeyColor;
}

}  // namespace


struct CFramebufferPaint::NVRFBContentImpl {
    AX_U8 *pShowScreen{nullptr};
    struct fb_fix_screeninfo stFix;
    struct fb_var_screeninfo stVar;
    AX_S32 s32Fd{-1};
    AX_FB_COLORKEY_T stColorKey;
    AX_U32 uClearScreenColor{0};
    AX_BOOL bOffsetScreen{AX_FALSE};
    NVRFBContentImpl() {
        memset(this, 0, sizeof(NVRFBContentImpl));
    }
};

// just for std::unique_ptr<NVRFBContentImpl> m_spNVRFBContentImpl;
CFramebufferPaint::CFramebufferPaint(AX_VOID) = default;
CFramebufferPaint::~CFramebufferPaint(AX_VOID) = default;

AX_BOOL CFramebufferPaint::Init(const AX_NVR_FB_INIT_PARAM_T& stInitParam) {
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);

    if (m_bInit) {
        LOG_M_E(TAG, "[%s][%d] FB had been initialize!", __func__, __LINE__);
        return AX_FALSE;
    }

    AX_S32 s32Ret = AX_VO_Init();
    LOG_M_I(TAG, "[%s][%d] AX_VO_Init:%d ", __func__, __LINE__, s32Ret);

    m_bInit = AX_TRUE;
    m_stInitParam = stInitParam;
    m_strFBPath = stInitParam.pFBPath;
    m_nThreadGroup = (stInitParam.nGroup > 0) ? stInitParam.nGroup : m_nThreadGroup;
    m_stInitParam.nTimeIntervalPanMS = m_stInitParam.nTimeIntervalPanMS ? m_stInitParam.nTimeIntervalPanMS : 10;
    AX_U32 nStride = m_stInitParam.nWidth * 4;
    if (m_stInitParam.enPixFmt == AX_FORMAT_ARGB1555) {
        nStride = m_stInitParam.nWidth * 2;
    }
    m_stInitParam.nStride = m_stInitParam.nStride ? m_stInitParam.nStride : nStride;

    m_spNVRFBContentImpl.reset(new NVRFBContentImpl);
    AX_BOOL ret = InitDevice();

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);

    return ret;
}

AX_BOOL CFramebufferPaint::InitDevice(AX_VOID) {
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);
    AX_BOOL ret = AX_FALSE;

    /* 1.Open framebuffer device */
    const AX_CHAR *fbPath = &m_strFBPath[0];
    AX_U8 *pShowScreen = nullptr;
    struct fb_fix_screeninfo &stFix = m_spNVRFBContentImpl->stFix;
    struct fb_var_screeninfo &stVar = m_spNVRFBContentImpl->stVar;
    struct fb_bitfield r = {16, 8, 0};
    struct fb_bitfield g = {8, 8, 0};
    struct fb_bitfield b = {0, 8, 0};
    struct fb_bitfield a = {24, 8, 0};
    if (m_stInitParam.enPixFmt == AX_FORMAT_ARGB1555) {
        r.offset = 10;
        r.length = 5;
        g.offset = 5;
        g.length = 5;
        b.offset = 0;
        b.length = 5;
        a.offset = 15;
        a.length = 1;
    }

    AX_FB_COLORKEY_T& stColorKey = m_spNVRFBContentImpl->stColorKey;
    stColorKey.u16Enable = 1;
    stColorKey.u16Inv = 0;
    stColorKey.u32KeyLow = m_stInitParam.uKeyColor;
    stColorKey.u32KeyHigh = m_stInitParam.uKeyColor;
    do {
        AX_S32 s32Fd = open(&m_strFBPath[0], O_RDWR);
        if (s32Fd < 0) {
            LOG_M_E(TAG, "open %s failed, err:%s", fbPath, strerror(errno));
            break;
        }
        m_spNVRFBContentImpl->s32Fd = s32Fd;

        /* 2.Get the variable screen info */
        AX_S32 s32Ret = ioctl(s32Fd, FBIOGET_VSCREENINFO, &stVar);
        if (s32Ret < 0) {
            LOG_M_E(TAG, "get variable screen info from %s failed", fbPath);
            break;
        }

        /* 3.Modify the variable screen info, the screen size: u32Width*u32Height, the
         * virtual screen size: u32Width*(u32Height*2), the pixel format: ARGB8888
         */
        stVar.xres = m_stInitParam.nWidth;
        stVar.yres = m_stInitParam.nHeight;
        stVar.xres_virtual = stVar.xres;
        stVar.yres_virtual = stVar.yres * 2;
        stVar.transp = a;
        stVar.red = r;
        stVar.green = g;
        stVar.blue = b;
        stVar.bits_per_pixel = (m_stInitParam.enPixFmt == AX_FORMAT_ARGB1555) ? 16 : 32;

        /* 4.Set the variable screeninfo */
        s32Ret = ioctl(s32Fd, FBIOPUT_VSCREENINFO, &stVar);
        if (s32Ret < 0) {
            LOG_M_E(TAG, "put variable screen info to %s failed", fbPath);
            break;
        }
        /* 5.Get the fix screen info */
        s32Ret = ioctl(s32Fd, FBIOGET_FSCREENINFO, &stFix);
        if (s32Ret < 0) {
            LOG_M_E(TAG, "get fix screen info from %s failed", fbPath);
            break;
        }
        /* 6.Map the physical video memory for user use */
        pShowScreen = (AX_U8 *)mmap(NULL, stFix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, s32Fd, 0);
        if (pShowScreen == (AX_U8 *) - 1) {
            LOG_M_E(TAG, "map %s failed", fbPath);
            break;
        }

        ioctl(m_spNVRFBContentImpl->s32Fd, AX_FBIOPUT_COLORKEY, &m_spNVRFBContentImpl->stColorKey);
        m_spNVRFBContentImpl->pShowScreen = pShowScreen;
        m_spNVRFBContentImpl->uClearScreenColor =
            ConvertToDrawColorFromColorKey(m_stInitParam.uKeyColor, m_stInitParam.enPixFmt);
        ret = AX_TRUE;
    } while (0);

    if (!ret) {
        DeinitDevice();
    }

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
    return ret;
}

AX_BOOL CFramebufferPaint::DeInit(AX_VOID) {
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);

    Stop();
    DestroyAllChannel();
    DeinitDevice();

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);

    return AX_TRUE;
}

AX_BOOL CFramebufferPaint::DeinitDevice(AX_VOID) {
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);

    if (m_spNVRFBContentImpl) {
        if (m_spNVRFBContentImpl->pShowScreen) {
            munmap(m_spNVRFBContentImpl->pShowScreen, m_spNVRFBContentImpl->stFix.smem_len);
            m_spNVRFBContentImpl->pShowScreen = nullptr;
        }
        if (m_spNVRFBContentImpl->s32Fd > 0) {
            // clear screen
            ioctl(m_spNVRFBContentImpl->s32Fd, AX_FBIOPUT_COLORKEY, &m_spNVRFBContentImpl->stColorKey);
            close(m_spNVRFBContentImpl->s32Fd);
            m_spNVRFBContentImpl->s32Fd = -1;
        }
        m_spNVRFBContentImpl.reset();
    }

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
    return AX_TRUE;
}

AX_VOID CFramebufferPaint::Start(AX_VOID) {
    LOG_M_I(TAG, "[%s][%d] +++ group:%d, channel count:%d.", __func__, __LINE__, m_nThreadGroup, m_mapChannel.size());

    if (!m_bLaunched) {
        m_bShouldQuit = AX_FALSE;
        m_bLaunched = AX_TRUE;
        m_spNVRFBContentImpl->bOffsetScreen = AX_FALSE;

        m_mainThread.Start([this](AX_VOID* pArg) { MainThreadEntry(); }, nullptr, "FBPMain");
        LOG_M_I(TAG, "[%s][%d] Launch FB Paint work threads.", __func__, __LINE__);
        for (AX_U8 i = 0; i < m_nThreadGroup; ++i) {
            m_vecPaintThread.emplace_back(new CAXThread);
            constexpr AX_U8 nBufferSize = 16;
            AX_CHAR szName[nBufferSize] = {0};
            snprintf(szName, nBufferSize, "FBPWorker-%d", i);
            m_vecPaintThread[i]->Start([this, i](AX_VOID* pArg) { PaintEntry(i); }, nullptr, szName);
        }
    }

    LOG_M_I(TAG, "[%s][%d] --- ", __func__, __LINE__);
}

AX_VOID CFramebufferPaint::MainThreadEntry(AX_VOID) {
    LOG_M_I(TAG, "[%s][%d] MainThreadEntry +++ ", __func__, __LINE__);
    m_vecStatusCompleted.resize(m_nThreadGroup);
    std::fill(m_vecStatusCompleted.begin(), m_vecStatusCompleted.end(), AX_TRUE);
    const FB_RECT_T stRectScreen{0, 0, m_stInitParam.nWidth, m_stInitParam.nHeight};

    for (;;) {
        if (m_bShouldQuit) {
            break;
        }

        if (m_bChannelsRectChannged) {
            TranslateRectsFromChannelToScreen();
        }

        // Prepare painting
        // Clear screen
        FillRectWithKeyColor(stRectScreen);
        // launch painting by children thread.
        std::fill(m_vecStatusCompleted.begin(), m_vecStatusCompleted.end(), AX_FALSE);
        WaitForChildrenThreadPaintingCompleted();
        if (m_bShouldQuit) {
            break;
        }

        EraseBackgroundAndDrawChannelRects();
        if (m_bShouldQuit) {
            break;
        }

        // call display to fronend
        if (m_bSchedulePaint) {
            FlushToPan();
        }
        if (m_bShouldQuit) {
            break;
        }

        CElapsedTimer::GetInstance()->mSleep(m_stInitParam.nTimeIntervalPanMS);
    }

    LOG_M_I(TAG, "[%s][%d] MainThreadEntry --- ", __func__, __LINE__);
}

AX_VOID CFramebufferPaint::TranslateRectsFromChannelToScreen(AX_VOID) {
    LOG_M_I(TAG, "[%s][%d] +++ ", __func__, __LINE__);

    std::map<AX_U8, NVRFB_CHANNEL> mapChannelsCopy;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        mapChannelsCopy = m_mapChannel;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    AX_U32 uMaxWidth = m_stInitParam.nWidth;
    AX_U32 uMaxHeight = m_stInitParam.nHeight;
    m_bSchedulePaint = AX_FALSE;
    std::vector<RectList>().swap(m_vecRectToPaint);
    std::map<AX_U8, RectList>().swap(m_mapRectToPaintClearBackground);

    for (AX_U8 i = 0; i < m_nThreadGroup; ++i) {
        RectList vecTargetRect;
        for (const auto& item : mapChannelsCopy) {
            const NVRFB_CHANNEL &stChannelData = item.second;
            if (stChannelData.uThreadIndex != i) {
                continue;
            }

            AX_U8 nChn = item.first;
            const RectList &vecChildRect = stChannelData.vecChildRect;
            FB_RECT_T stChannelArea = stChannelData.stArea;
            for (auto childRect : vecChildRect) {
                RectOffset(childRect, stChannelArea, uMaxWidth, uMaxHeight);
                if (childRect.h == 0 || childRect.w == 0) {
                    continue;
                }

                if (stChannelData.bEraseBackground) {
                    m_mapRectToPaintClearBackground[nChn].emplace_back(childRect);
                } else {
                    vecTargetRect.emplace_back(childRect);
                }
            }
        }
        m_vecRectToPaint.emplace_back(std::move(vecTargetRect));
    }
    m_bChannelsRectChannged = AX_FALSE;
    m_bSchedulePaint = AX_TRUE;

    LOG_M_I(TAG, "[%s][%d] --- ", __func__, __LINE__);
}

AX_VOID CFramebufferPaint::WaitForChildrenThreadPaintingCompleted() {
    do {
        if (m_bShouldQuit || !m_bSchedulePaint) {
           break;
        }

        // paint all channel rect to FB, and wait for completed.
        AX_BOOL bCompleted = AX_TRUE;
        for (AX_U8 i = 0; i < m_nThreadGroup; ++i) {
            if (!m_vecStatusCompleted[i]) {
                bCompleted = AX_FALSE;
                break;
            }
        }
        if (bCompleted) {
            break;
        }

        // std::this_thread::yield();
        CElapsedTimer::mSleep(10);
    } while (1);
}

AX_VOID CFramebufferPaint::FlushToPan(AX_VOID) {
    LOG_M_I(TAG, "[%s][%d] +++ ", __func__, __LINE__);

    struct fb_var_screeninfo &stVar = m_spNVRFBContentImpl->stVar;
    AX_BOOL& bOffsetScreen = m_spNVRFBContentImpl->bOffsetScreen;
    AX_S32 yResOffset = bOffsetScreen * m_stInitParam.nHeight;
    stVar.yoffset = yResOffset;
    ioctl(m_spNVRFBContentImpl->s32Fd, FBIOPAN_DISPLAY, &stVar);
    bOffsetScreen = bOffsetScreen ? AX_FALSE : AX_TRUE;

    LOG_M_I(TAG, "[%s][%d] --stVar.yoffset:%d- ", __func__, __LINE__, stVar.yoffset);
}

AX_VOID CFramebufferPaint::PaintEntry(AX_U8 index) {
    LOG_M_I(TAG, "[%s][%d] Paint thread:%d +++ ", __func__, __LINE__, index);
    for (;;) {
        if (m_bShouldQuit) {
            break;
        }

        if (!m_bSchedulePaint || m_vecStatusCompleted[index]) {
            // std::this_thread::yield();
            CElapsedTimer::mSleep(10);
            continue;
        }

        const RectList &vecRect = m_vecRectToPaint[index];
        LOG_M_I(TAG, "Paint thread:%d, rects count:%d.", index, vecRect.size());
        for (const auto &childRect : vecRect) {
            if (m_bShouldQuit) {
                break;
            }
            PaintToDevice(childRect);
        }

        m_vecStatusCompleted[index] = AX_TRUE;
    }
    LOG_M_I(TAG, "[%s][%d] Paint thread:%d --- ", __func__, __LINE__, index);
}

AX_VOID CFramebufferPaint::PaintToDevice(const FB_RECT_T &stRect) {
    AX_U32 u32Stride = m_stInitParam.nStride;
    AX_U8 *pShowScreen = m_spNVRFBContentImpl->pShowScreen +
        m_spNVRFBContentImpl->bOffsetScreen * m_stInitParam.nStride * m_spNVRFBContentImpl->stVar.yres;

    if (m_stInitParam.enPixFmt == AX_FORMAT_ARGB8888) {
        AX_U32 u32ColorDraw = m_stInitParam.nColorDraw;
        // top board
        // char unit
        AX_S32 s32Offset = stRect.y * u32Stride;
        AX_U32 *u32Pixel = (AX_U32 *)(pShowScreen + s32Offset) + stRect.x;
        for (AX_U32 j = 0; j < stRect.w; ++j) {
            u32Pixel[j] = u32ColorDraw;
        }
        // buttom board
        s32Offset = (stRect.y + stRect.h - 1) * u32Stride;
        u32Pixel = (AX_U32 *)(pShowScreen + s32Offset) + stRect.x;
        for (AX_U32 j = 0; j < stRect.w; ++j) {
            u32Pixel[j] = u32ColorDraw;
        }
        // left board
        for (AX_U32 j = 0; j < stRect.h; ++j) {
            s32Offset = (stRect.y + j) * u32Stride;
            u32Pixel = (AX_U32 *)(pShowScreen + s32Offset) + stRect.x;
            u32Pixel[0] = u32ColorDraw;
        }

        // right board
        for (AX_U32 j = 0; j < stRect.h; ++j) {
            s32Offset = (stRect.y + j) * u32Stride;
            u32Pixel = (AX_U32 *)(pShowScreen + s32Offset) + stRect.w + stRect.x - 1;
            u32Pixel[0] = u32ColorDraw;
        }
    } else {
        AX_U32 u16ColorDraw = m_stInitParam.nColorDraw;
        // top board
        // char unit
        AX_S32 s32Offset = stRect.y * u32Stride;
        AX_U16 *u16Pixel = (AX_U16 *)(pShowScreen + s32Offset) + stRect.x;
        for (AX_U32 j = 0; j < stRect.w; ++j) {
            u16Pixel[j] = u16ColorDraw;
        }
        // buttom board
        s32Offset = (stRect.y + stRect.h - 1) * u32Stride;
        u16Pixel = (AX_U16 *)(pShowScreen + s32Offset) + stRect.x;
        for (AX_U32 j = 0; j < stRect.w; ++j) {
            u16Pixel[j] = u16ColorDraw;
        }
        // left board
        for (AX_U32 j = 0; j < stRect.h; ++j) {
            s32Offset = (stRect.y + j) * u32Stride;
            u16Pixel = (AX_U16 *)(pShowScreen + s32Offset) + stRect.x;
            u16Pixel[0] = u16ColorDraw;
        }

        // right board
        for (AX_U32 j = 0; j < stRect.h; ++j) {
            s32Offset = (stRect.y + j) * u32Stride;
            u16Pixel = (AX_U16 *)(pShowScreen + s32Offset) + stRect.w + stRect.x - 1;
            u16Pixel[0] = u16ColorDraw;
        }
    }
}

 AX_VOID CFramebufferPaint::FillRectWithKeyColor(const FB_RECT_T &stRect, AX_BOOL bCheckQuit) {
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);

    AX_U32 u32Stride = m_stInitParam.nStride;
    AX_U8 *pShowScreen = m_spNVRFBContentImpl->pShowScreen +
        m_spNVRFBContentImpl->bOffsetScreen * m_stInitParam.nStride * m_spNVRFBContentImpl->stVar.yres;
    if (m_stInitParam.enPixFmt == AX_FORMAT_ARGB8888) {
        static constexpr AX_U8 u32Step = 4;
        const AX_U32 uKeyColor = m_spNVRFBContentImpl->uClearScreenColor;
        const AX_U32 aryKeycolor[u32Step] = {uKeyColor, uKeyColor, uKeyColor, uKeyColor};
        uint32x4_t u32x4Keycolor = vld1q_u32(aryKeycolor);
        for (AX_U32 i = 0; i < stRect.h; ++i) {
            AX_S32 s32Offset = (stRect.y + i) * u32Stride;
            AX_U32 *u32Pixel = (AX_U32 *)(pShowScreen + s32Offset) + stRect.x;
           for (AX_U32 j = 0; j < stRect.w; j += u32Step) {
                if (stRect.w - j > u32Step) {
                    vst1q_u32(u32Pixel, u32x4Keycolor);
                    u32Pixel += u32Step;
                } else {
                    std::fill(u32Pixel, (u32Pixel+ (stRect.w - j)), uKeyColor);
                    break;
                }
            }

            if (bCheckQuit && m_bShouldQuit) {
                break;
            }
        }
    } else {
        static constexpr AX_U8 u16Step = 8;
        const AX_U16 uKeyColor = m_spNVRFBContentImpl->uClearScreenColor;
        const AX_U16 aryKeycolor[u16Step] = {uKeyColor, uKeyColor, uKeyColor, uKeyColor,
            uKeyColor, uKeyColor, uKeyColor, uKeyColor};
        uint16x8_t u16x8Keycolor = vld1q_u16(aryKeycolor);
        for (AX_U32 i = 0; i < stRect.h; ++i) {
            AX_S32 s32Offset = (stRect.y + i) * u32Stride;
            AX_U16 *u16Pixel = (AX_U16 *)(pShowScreen + s32Offset) + stRect.x;
           for (AX_U32 j = 0; j < stRect.w; j += u16Step) {
            if (stRect.w - j > u16Step) {
                    vst1q_u16(u16Pixel, u16x8Keycolor);
                    u16Pixel += u16Step;
                } else {
                    std::fill(u16Pixel, (u16Pixel+ (stRect.w - j)), uKeyColor);
                    break;
                }
            }

            if (bCheckQuit && m_bShouldQuit) {
                break;
            }
        }
    }

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
 }

AX_VOID CFramebufferPaint::EraseBackgroundAndDrawChannelRects(AX_VOID) {
    // only clean background.
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto &item : m_mapChannel) {
        const NVRFB_CHANNEL &stChannelData = item.second;
        if (stChannelData.bEraseBackground) {
            const FB_RECT_T &stRect = item.second.stArea;
            FillRectWithKeyColor(stRect);
        }
    }

    // Draw target rects.
    for (const auto &itemRect : m_mapRectToPaintClearBackground) {
        AX_U8 nChn = itemRect.first;
        if (m_mapChannel.find(nChn) == m_mapChannel.end()) {
            continue;
        }

        const RectList &vecRect = itemRect.second;
        for (const auto& childRect : vecRect) {
            if (m_bShouldQuit) {
                 break;
            }

            PaintToDevice(childRect);
        }
    }
}

AX_VOID CFramebufferPaint::Stop(AX_VOID) {
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);

    m_bShouldQuit = AX_TRUE;
    for (auto& item : m_vecPaintThread) {
        item->Join();
        item.reset();
    }

    m_mainThread.Join();
    m_spNVRFBContentImpl->bOffsetScreen = AX_FALSE;
    ClearScreen();

    m_vecPaintThread.clear();
    m_bLaunched = AX_FALSE;

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
}

AX_VOID CFramebufferPaint::CreateChannel(AX_U8 nChn, const FB_RECT_T &rect, AX_BOOL bEraseBackground) {
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);

    NVRFB_CHANNEL nvrChannelData;
    nvrChannelData.stArea = rect;
    nvrChannelData.uThreadIndex = 0;
    nvrChannelData.bEraseBackground = bEraseBackground;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_mapChannel.emplace(nChn, nvrChannelData);
    // bind channel to work thread.
    AX_U8 nThreadIndex = 0;
    for (auto &item : m_mapChannel) {
        item.second.uThreadIndex = nThreadIndex;
        if (++nThreadIndex >= m_nThreadGroup) {
            nThreadIndex = 0;
        }
    }
    m_bChannelsRectChannged = AX_TRUE;

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
}

AX_BOOL CFramebufferPaint::DestroyChannel(AX_U8 nChn) {
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);

    std::lock_guard<std::mutex> lock(m_mutex);
    AX_BOOL ret = AX_FALSE;
    size_t nRemoved = m_mapChannel.erase(nChn);
    if (nRemoved > 0) {
        m_bChannelsRectChannged = AX_TRUE;
        ret = AX_TRUE;
    }

    if (!ret) {
        LOG_M_I(TAG, "[%s][%d] target channel:%d no found! ", __func__, __LINE__, nChn);
    }

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
    return ret;
}

AX_VOID CFramebufferPaint::DestroyAllChannel(AX_VOID) {
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);

    std::lock_guard<std::mutex> lock(m_mutex);
    std::map<AX_U8, NVRFB_CHANNEL>().swap(m_mapChannel);
    m_bChannelsRectChannged = AX_TRUE;

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
}

AX_VOID CFramebufferPaint::ClearScreen(AX_VOID) {
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);

    const FB_RECT_T stRect{0, 0, m_stInitParam.nWidth, m_stInitParam.nHeight};
    FillRectWithKeyColor(stRect, AX_FALSE);
    FlushToPan();

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
}

AX_BOOL CFramebufferPaint::DrawChannelRects(AX_U8 nChn, const RectList &rects) {
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);

    std::lock_guard<std::mutex> lock(m_mutex);
    AX_BOOL ret = AX_FALSE;
    auto iter = m_mapChannel.find(nChn);
    if (iter != m_mapChannel.end()) {
        iter->second.vecChildRect = rects;
        ret = AX_TRUE;
        m_bChannelsRectChannged = AX_TRUE;
    }

    if (!ret) {
        LOG_M_I(TAG, "[%s][%d] target channel:%d no found! ", __func__, __LINE__, nChn);
    }

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
    return ret;
}

AX_BOOL CFramebufferPaint::ClearChannelRects(AX_U8 nChn) {
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);

    std::lock_guard<std::mutex> lock(m_mutex);
    AX_BOOL ret = AX_FALSE;
    auto iter = m_mapChannel.find(nChn);
    if (iter != m_mapChannel.end()) {
        RectList().swap(iter->second.vecChildRect);
        m_bChannelsRectChannged = AX_TRUE;
        ret = AX_TRUE;
    }

    if (!ret) {
        LOG_M_I(TAG, "[%s][%d] target channel:%d no found! ", __func__, __LINE__, nChn);
    }

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
    return ret;
}

AX_VOID CFramebufferPaint::ClearAllChannelRects(AX_VOID) {
    LOG_M_D(TAG, "[%s][%d] +++ ", __func__, __LINE__);

    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto &item : m_mapChannel) {
        RectList().swap(item.second.vecChildRect);
    }

    LOG_M_D(TAG, "[%s][%d] --- ", __func__, __LINE__);
}
