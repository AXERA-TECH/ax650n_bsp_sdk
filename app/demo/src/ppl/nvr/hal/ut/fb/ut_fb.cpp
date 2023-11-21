/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include "AppLogApi.h"
#include "ax_ivps_api.h"
#include "cmdline.h"
#include "help.hpp"
#include "framebufferPaint.hpp"
#include "ElapsedTimer.hpp"


#define TAG "APP"
using namespace std;

static AX_POOL CreatePool(const VO_CHN_INFO_T &stChnInfo);
static AX_BOOL DestoryPool(AX_POOL &pool);


namespace {

#define MAKE_RGBA(stRGB, u8R, u8G, u8B, u8A) \
    (SHIFT_COLOR8(&(stRGB)->stRed, (u8R)) | \
     SHIFT_COLOR8(&(stRGB)->stGreen, (u8G)) | \
     SHIFT_COLOR8(&(stRGB)->stBlue, (u8B)) | \
     SHIFT_COLOR8(&(stRGB)->stAlpha, (u8A)))

typedef struct UTIL_COLOR_COMPONENT {
    AX_U32 u32Length;
    AX_U32 u32Offset;
} UTIL_COLOR_COMPONENT_S;

typedef struct UTIL_RGB_INFO {
    UTIL_COLOR_COMPONENT_S stRed;
    UTIL_COLOR_COMPONENT_S stGreen;
    UTIL_COLOR_COMPONENT_S stBlue;
    UTIL_COLOR_COMPONENT_S stAlpha;
} UTIL_RGB_INFO_S;

#define MAKE_RGB_INFO(u32RedLen, u32RedOffs, u32GreenLen, u32GreenOffs, u32BlueLen, u32BlueOffs, u32AlphaLen, u32AlphaOffs) \
    { .stRed = { (u32RedLen), (u32RedOffs) }, \
      .stGreen = { (u32GreenLen), (u32GreenOffs) }, \
      .stBlue = { (u32BlueLen), (u32BlueOffs) }, \
      .stAlpha = { (u32AlphaLen), (u32AlphaOffs) } }

static inline AX_U32 SHIFT_COLOR8(const UTIL_COLOR_COMPONENT_S *pstComp,
                                  AX_U32 u32Value) {
    u32Value &= 0xff;
    /* Fill the low bits with the high bits. */
    u32Value = (u32Value << 8) | u32Value;
    /* Shift down to remove unwanted low bits */
    u32Value = u32Value >> (16 - pstComp->u32Length);
    /* Shift back up to where the u32Value should be */
    return u32Value << pstComp->u32Offset;
}

// const color from msp/sample/common/common_vo_pattern.c.
const AX_U16 u16ColorsTop[] = {
//    MAKE_RGBA(stRGB, 192, 192, 192, 255),   /* grey */
//    MAKE_RGBA(stRGB, 192, 192, 0, 255), /* yellow */
//    MAKE_RGBA(stRGB, 0, 192, 192, 255), /* cyan */
//    MAKE_RGBA(stRGB, 0, 192, 0, 255),       /* green */
//    MAKE_RGBA(stRGB, 192, 0, 192, 255), /* magenta */
//    MAKE_RGBA(stRGB, 192, 0, 0, 255),       /* red */
//    MAKE_RGBA(stRGB, 0, 0, 192, 255),       /* blue */
};

const AX_U16 u16ColorsMiddle[] = {
//    MAKE_RGBA(stRGB, 0, 0, 192, 127),       /* blue */
//    MAKE_RGBA(stRGB, 19, 19, 19, 127),  /* black */
//    MAKE_RGBA(stRGB, 192, 0, 192, 127), /* magenta */
//    MAKE_RGBA(stRGB, 19, 19, 19, 127),  /* black */
//    MAKE_RGBA(stRGB, 0, 192, 192, 127), /* cyan */
//    MAKE_RGBA(stRGB, 19, 19, 19, 127),  /* black */
//    MAKE_RGBA(stRGB, 192, 192, 192, 127),   /* grey */
};

}


int main(int argc, char *argv[]) {
    cmdline::parser a;
    a.add<AX_S32>("log", 'v', "log level", false, APP_LOG_WARN);
    a.add<AX_S32>("times", 't', "run tims", false, 10);
    a.add<AX_S32>("index", 'i', "framebuffer index", false, 0);
    a.add<AX_S32>("double", 'd', "double cache", false, 1);
    a.add<AX_S32>("group", 'g', "group number", false, 2);
    a.add<AX_S32>("pixfmt", 'p', "Pix Format, 0: ARGB888; 1: ARGB1555", false, 0);
    a.parse_check(argc, argv);

    APP_SYS_INIT(0, AX_TRUE, AX_FALSE);
    AX_APP_SetLogLevel(a.get<AX_S32>("log"));

    constexpr AX_U32 videos[] = {1, 4, 9, 16, 32, 64};
    constexpr AX_U32 MAX_LAYOUT_NUM = sizeof(videos) / sizeof(videos[0]);
    AX_CHAR fbPath[32];
    VO_ATTR_T stAttr;
    CVO *pVO{nullptr};
    VO_CHN_INFO_T stChnInfo[MAX_LAYOUT_NUM];
    AX_POOL pool[MAX_LAYOUT_NUM]{0};
    CFramebufferPaint fbPaint;
    AX_NVR_FB_INIT_PARAM_T fbParam;
    AX_S32 tt = 0;
    AX_S32 run_times = a.get<AX_S32>("times");
    UTIL_RGB_INFO_S stAR24_Info = MAKE_RGB_INFO(8, 16, 8, 8, 8, 0, 8, 24);
    UTIL_RGB_INFO_S stAR15_Info = MAKE_RGB_INFO(5, 10, 5, 5, 5, 0, 1, 15);

    auto drawChildren = [&fbPaint](AX_S32 tt) {
        CFramebufferPaint::RectList rects;
            rects.emplace_back(FB_RECT_T{10, (AX_U32)(10 + tt*10), 200, 150});
            rects.emplace_back(FB_RECT_T{60 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{120 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{180 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{10, (AX_U32)(10 + tt*10), 10, 50});
            rects.emplace_back(FB_RECT_T{60 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{120 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{120 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{120 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{120 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{120 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{980 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{1080 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{1180 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{1280 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{1380 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{1480 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{1580 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{180 + (AX_U32)(tt * 10), 50, 300, 500});
            rects.emplace_back(FB_RECT_T{180 + (AX_U32)(tt * 10), 50, 500, 500});
            rects.emplace_back(FB_RECT_T{180 + (AX_U32)(tt * 10), 50, 400, 500});
            rects.emplace_back(FB_RECT_T{180 + (AX_U32)(tt * 10), 50, 200, 500});
            rects.emplace_back(FB_RECT_T{180 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{180 + (AX_U32)(tt * 10), 50, 400, 500});
            rects.emplace_back(FB_RECT_T{180 + (AX_U32)(tt * 10), 50, 1000, 500});
            rects.emplace_back(FB_RECT_T{180 + (AX_U32)(tt * 10), 50, 1000, 500});
            rects.emplace_back(FB_RECT_T{180 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{180 + (AX_U32)(tt * 10), 50, 200, 500});
            rects.emplace_back(FB_RECT_T{180 + (AX_U32)(tt * 10), 50, 1800, 500});
            rects.emplace_back(FB_RECT_T{180 + (AX_U32)(tt * 10), 50, 1800, 500});
            rects.emplace_back(FB_RECT_T{10, (AX_U32)(10 + tt*10), 200, 150});
            rects.emplace_back(FB_RECT_T{60 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{120 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{180 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{10, (AX_U32)(10 + tt*10), 10, 50});
            rects.emplace_back(FB_RECT_T{60 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{120 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{120 + (AX_U32)(tt * 10), 50, 100, 500});
            rects.emplace_back(FB_RECT_T{120 + (AX_U32)(tt * 10), 50, 100, 500});
            fbPaint.DrawChannelRects(0, rects);
            fbPaint.DrawChannelRects(1, rects);
            fbPaint.DrawChannelRects(2, rects);
            fbPaint.DrawChannelRects(3, rects);
            fbPaint.DrawChannelRects(4, rects);
            fbPaint.DrawChannelRects(5, rects);
            fbPaint.DrawChannelRects(6, rects);
            fbPaint.DrawChannelRects(7, rects);
            fbPaint.DrawChannelRects(8, rects);
            fbPaint.DrawChannelRects(9, rects);
            fbPaint.DrawChannelRects(10, rects);
    };
    /* calculate video channel rectage and create input vb pools */
    for (AX_U32 i = 0; i < MAX_LAYOUT_NUM; ++i) {
        stChnInfo[i] = InitLayout(1920, 1080, videos[i]);
        for (AX_U32 j = 0; j < stChnInfo[i].nCount; ++j) {
            stChnInfo[i].arrFps[j] = 60;
        }

        pool[i] = CreatePool(stChnInfo[i]);
        if (AX_INVALID_POOLID == pool[i]) {
            goto __EXIT__;
        }
    }

    snprintf(fbPath, sizeof(fbPath), "/dev/fb%d", a.get<AX_S32>("index"));
    fbParam.enPixFmt = (a.get<AX_S32>("pixfmt") == 1) ? AX_FORMAT_ARGB1555 : AX_FORMAT_ARGB8888;
    fbParam.pFBPath = fbPath;
    fbParam.nWidth = 1920;
    fbParam.nHeight = 1080;
    // color from u16ColorsTop array.
    fbParam.nColorDraw = (fbParam.enPixFmt == AX_FORMAT_ARGB1555) ? MAKE_RGBA(&stAR15_Info, 0, 192, 0, 255) : MAKE_RGBA(&stAR24_Info, 0, 192, 0, 255);
    fbParam.uKeyColor = 0x10101;
    fbParam.nGroup = a.get<AX_S32>("group");
    fbParam.nTimeIntervalPanMS = 10;
    fbPaint.Init(fbParam);
    fbPaint.Start();

    stAttr.dev = 0;
    stAttr.enIntfType = AX_VO_INTF_HDMI;
    stAttr.enIntfSync = AX_VO_OUTPUT_1080P60;
    stAttr.nLayerDepth = 2;
    stAttr.nBgClr = 0x0;
    stAttr.stChnInfo = stChnInfo[0];
    // stAttr.uiLayer = a.get<AX_S32>("index");
    stAttr.s32FBIndex[1] = a.get<AX_S32>("index");
    pVO = CVO::CreateInstance(stAttr);
    if (!pVO) {
        goto __EXIT__;
    }

    if (!pVO->Start()) {
        goto __EXIT__;
    }

    fbPaint.CreateChannel(0, FB_RECT_T{0, 200, 500, 200});
    fbPaint.CreateChannel(1, FB_RECT_T{500, 200, 500, 200});
    fbPaint.CreateChannel(2, FB_RECT_T{0, 400, 500, 200});
    fbPaint.CreateChannel(3, FB_RECT_T{500, 400, 500, 600});
    fbPaint.CreateChannel(4, FB_RECT_T{100, 200, 800, 600});
    fbPaint.CreateChannel(5, FB_RECT_T{600, 200, 800, 600});
    fbPaint.CreateChannel(6, FB_RECT_T{400, 200, 800, 600});
    fbPaint.CreateChannel(7, FB_RECT_T{600, 200, 800, 600});
    fbPaint.CreateChannel(8, FB_RECT_T{800, 200, 800, 600});

    {
        AX_BOOL bEraseBackground = AX_FALSE;
        fbPaint.CreateChannel(9, FB_RECT_T{300, 200, 800, 600});
        bEraseBackground = AX_TRUE;
        fbPaint.CreateChannel(10, FB_RECT_T{400, 200, 300, 300}, bEraseBackground);
    }
    while (1) {
        if (IS_APP_QUIT()) {
            break;
        }
        if (++tt > run_times) {
            fbPaint.DestroyAllChannel();
            AX_BOOL bEraseBackground = AX_FALSE;
            fbPaint.CreateChannel(9, FB_RECT_T{300, 200, 800, 600});
            bEraseBackground = AX_TRUE;
            fbPaint.CreateChannel(10, FB_RECT_T{400, 200, 300, 300}, bEraseBackground);
            tt = 0;
        }
        drawChildren(tt);
        CElapsedTimer::mSleep(500);
    }

__EXIT__:
    fbPaint.DeInit();

    for (auto &m : pool) {
        DestoryPool(m);
    }

    if (pVO) {
        pVO->Stop();
        pVO->Destory();
    }

    return 0;
}

static AX_POOL CreatePool(const VO_CHN_INFO_T &stChnInfo) {
    AX_POOL_CONFIG_T stPoolCfg;
    memset(&stPoolCfg, 0, sizeof(stPoolCfg));
    stPoolCfg.MetaSize = 4096;
    stPoolCfg.CacheMode = POOL_CACHE_MODE_NONCACHE;
    stPoolCfg.BlkSize = ALIGN_UP(stChnInfo.arrChns[0].stRect.u32Width, 16) * stChnInfo.arrChns[0].stRect.u32Height * 3 / 2;
    stPoolCfg.BlkCnt = stChnInfo.nCount;

    return AX_POOL_CreatePool(&stPoolCfg);
}

static AX_BOOL DestoryPool(AX_POOL &pool) {
    if (AX_INVALID_POOLID == pool) {
        return AX_TRUE;
    }

    AX_S32 ret = AX_POOL_DestroyPool(pool);
    if (0 != ret) {
        LOG_M_E(TAG, "AX_POOL_DestroyPool(pool %d) fail, ret = 0x%x", pool, ret);
        return AX_FALSE;
    }

    pool = AX_INVALID_POOLID;
    return AX_TRUE;
}
