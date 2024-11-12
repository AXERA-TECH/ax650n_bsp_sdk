/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/fb.h>
#include <math.h>

#include <sys/stat.h>
#include <sys/mman.h>
#include <algorithm>

using namespace std;

#include "AppLogApi.h"
#include "ax_base_type.h"
#include "ax_ivps_api.h"
#include "AXNVRDisplayCtrl.h"
#include "NVRConfigParser.h"
#include "region.hpp"

#define TAG "NVRDISP"

#ifndef ALIGN_UP
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#endif

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, align) ((x) & ~((align)-1))
#endif

std::default_random_engine g_random_engine;
std::uniform_int_distribution<AX_U32> g_random_uniform(0, NVR_MAX_LAYOUT_NUM - 1);

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
}

AX_BOOL CAXNVRDisplayCtrl::Init(const AX_NVR_VO_INFO_T &stAttr) {
    AX_BOOL bRet = AX_FALSE;
    do {
        if (m_pVo != nullptr) {
            LOG_M_W(TAG, "Invalid pointer");
            bRet = AX_TRUE;
            break;
        }

        m_stAttr = stAttr;
        g_random_engine.seed(time(0));

        AX_U32 nHz = 0;
        AX_VO_RECT_T stArea;
        if (!this->getDispInfoFromIntfSync(m_stAttr.stVoAttr.enIntfSync, stArea, nHz)) {
            break;
        }

        // qt fb
        if (m_stAttr.stVoAttr.s32FBIndex[0] != -1) {
            AX_S32 s32Ret = this->initFramebuffer(stArea.u32Width, stArea.u32Height, m_stAttr.stVoAttr.s32FBIndex[0]);
            if (s32Ret != 0) {
                LOG_M_E(TAG, "initialize Frame buffer(dev %d) fail, ret = 0x%x", m_stAttr.stVoAttr.dev, s32Ret);
                break;
            }
        }

        if (m_stAttr.stVoAttr.s32FBIndex[1] != -1) {
            // paint fb
            AX_CHAR fbPath[32];
            snprintf(fbPath, sizeof(fbPath), "/dev/fb%d", m_stAttr.stVoAttr.s32FBIndex[1]);
            UTIL_RGB_INFO_S stAR24_Info = MAKE_RGB_INFO(8, 16, 8, 8, 8, 0, 8, 24);
            UTIL_RGB_INFO_S stAR15_Info = MAKE_RGB_INFO(5, 10, 5, 5, 5, 0, 1, 15);
            AX_NVR_FB_INIT_PARAM_T fbParam;
            fbParam.enPixFmt = AX_FORMAT_ARGB1555;
            fbParam.pFBPath = fbPath;
            fbParam.nWidth = stArea.u32Width;
            fbParam.nHeight = stArea.u32Height;
            fbParam.nColorDraw = (fbParam.enPixFmt == AX_FORMAT_ARGB1555) ?
                MAKE_RGBA(&stAR15_Info, 255, 255, 255, 255) : MAKE_RGBA(&stAR24_Info, 255, 255, 255, 255);
            fbParam.uKeyColor = 0x10101;
            fbParam.nGroup = 4;
            fbParam.nTimeIntervalPanMS = 10;

            m_pFbPaint = new (std::nothrow) CFramebufferPaint;
            if (m_pFbPaint == nullptr) {
                break;
            }
            if (!m_pFbPaint->Init(fbParam)) {
                LOG_M_E(TAG, "initialize Frame buffer paint(dev %d) failed", m_stAttr.stVoAttr.dev);
                break;
            }
            m_pFbPaint->Start();

            LOG_M_W(TAG, "[SUCCESS]Frame buffer paint(dev %d)", m_stAttr.stVoAttr.dev);

        }

        if (m_stAttr.enDispDevType == AX_NVR_DISPDEV_TYPE::PRIMARY) {
            m_stAttr.stVoAttr.nEngineId = 3; /* TDP */
            m_stAttr.stVoAttr.enEngineMode = AX_VO_ENGINE_MODE_FORCE;
            m_stAttr.stVoAttr.enDrawMode = AX_VO_PART_MODE_SINGLE; /* if virutal engine, must set to single mode */

            m_stAttr.stVoAttr.pipChn = NVR_PIP_CHN_NUM;
        }
        else if (m_stAttr.enDispDevType == AX_NVR_DISPDEV_TYPE::SECOND) {
            m_stAttr.stVoAttr.nEngineId = 4; /* TDP */
            m_stAttr.stVoAttr.enEngineMode = AX_VO_ENGINE_MODE_FORCE;
            m_stAttr.stVoAttr.enDrawMode = AX_VO_PART_MODE_SINGLE;
            // The following data comes from the layout of the UI <RoundPatrolMain.ui>:
            const AX_U32 top = 4;
            const AX_U32 left = 4;
            const AX_U32 bottom = 4;
            const AX_U32 right = 4;
            if (!this->initPollingLayout(stArea.u32Width - (top + bottom), stArea.u32Height - (left + right)) ) {
                LOG_M_E(TAG, "initialize polling layout failed.");
                break;
            }
        }

        VO_CHN_INFO_T stChn;
        // stChn.nCount = 1;
        // stChn.arrChns[0].stRect = {0, 0, stArea.u32Width, stArea.u32Height};
        // stChn.arrChns[0].u32FifoDepth = m_stAttr.u32FifoDepth;
        // stChn.arrFps[0] = m_stAttr.fFps;
        m_stAttr.stVoAttr.stChnInfo = stChn;
        m_pVo = CVO::CreateInstance(m_stAttr.stVoAttr);
        if (!m_pVo) {
            break;
        }

        if (!m_pVo->Start()) {
            m_pVo->Destory();
            m_pVo = nullptr;
            break;
        } else if (!m_stAttr.stVoAttr.bLinkVo2Disp) {
            m_pVoRegionObs = new CVOLayerRegionObserver(m_pVo);
        }

        bRet = AX_TRUE;
    } while (0);

    return bRet;
}

AX_VOID CAXNVRDisplayCtrl::DeInit() {



    if (m_pFbPaint != nullptr) {
        m_pFbPaint->DestroyAllChannel();
        m_pFbPaint->Stop();
        m_pFbPaint->DeInit();
        delete m_pFbPaint;
        m_pFbPaint = nullptr;
    }

    if (m_pVo) {
        // if (m_bRoundPatrolStop) {
        //     VO_ATTR_T attr = m_pVo->GetAttr();
        //     AX_VO_BindVideoLayer(attr.voLayer, attr.dev);
        //     for (AX_U32 i = 0; i < attr.stChnInfo.nCount; ++i) {
        //         m_pVo->EnableChn(i, attr.stChnInfo.arrChns[i]);
        //     }
        //     m_bRoundPatrolStop = AX_FALSE;
        // }
        if (m_bRoundPatrolStop) {
            m_pVo->Init(m_stAttr.stVoAttr);
            m_pVo->Start();
            m_bRoundPatrolStop = AX_FALSE;
        }

        if (!m_pVo->Stop()) {
            LOG_M_E(TAG, "VO Stop failed.");
        }
        m_pVo->Destory();
        m_pVo = nullptr;
    }

    if (m_pVoRegionObs) {
        delete m_pVoRegionObs;
        m_pVoRegionObs = nullptr;
    }
}

AX_BOOL CAXNVRDisplayCtrl::GetChannelRect(VO_CHN nChn, AX_NVR_RECT_T &rect) {
    if (nChn < m_vecRect.size()) {
        rect = m_vecRect[nChn];
        return AX_TRUE;
    } else {
        if (nChn == NVR_PIP_CHN_NUM) {
            rect = m_pipRect;
            return AX_TRUE;
        }
    }
    return AX_FALSE;
}

AX_VOID CAXNVRDisplayCtrl::StartPipFBChannel(const AX_NVR_RECT_T &rect) {
    if (m_stAttr.enDispDevType == AX_NVR_DISPDEV_TYPE::PRIMARY) {
        if (m_pFbPaint) {
            m_pipRect = rect;
            m_pFbPaint->CreateChannel(NVR_PIP_CHN_NUM,
                    FB_RECT_T{ALIGN_UP(rect.x, 2), ALIGN_UP(rect.y, 2), ALIGN_DOWN(rect.w, 8), ALIGN_DOWN(rect.h, 2)},
                    AX_TRUE);
        }
    }
}

AX_VOID CAXNVRDisplayCtrl::StopPipFBChannel(AX_VOID) {
    if (m_pFbPaint) {
        m_pFbPaint->DestroyChannel(NVR_PIP_CHN_NUM);
    }
}

AX_BOOL CAXNVRDisplayCtrl::EnablePip(const AX_NVR_RECT_T &rect) {

    if (m_stAttr.enDispDevType == AX_NVR_DISPDEV_TYPE::PRIMARY) {
        AX_VO_CHN_ATTR_T stAttr;
        AX_VO_RECT_T stArea;
        VO_ATTR_T attr = m_pVo->GetAttr();
        AX_BOOL b4K = is4K(attr.enIntfSync, stArea);
        auto vorect = rect;
        m_pipRect = rect;
        if (b4K) {
            vorect = mapRect(vorect, stArea.u32Width, stArea.u32Height);
        }
        stAttr.stRect = {ALIGN_UP(vorect.x, 2), ALIGN_UP(vorect.y, 2), ALIGN_DOWN(vorect.w, 8), ALIGN_DOWN(vorect.h, 2)};
        stAttr.u32FifoDepth = m_stAttr.u32PreviewFifoDepth;
        stAttr.u32Priority = 99;
        stAttr.bKeepPrevFr = AX_TRUE;
        // if (m_pFbPaint) {
        //     m_pFbPaint->CreateChannel(NVR_PIP_CHN_NUM,
        //             FB_RECT_T{stAttr.stRect.u32X, stAttr.stRect.u32Y, stAttr.stRect.u32Width, stAttr.stRect.u32Height},
        //             AX_TRUE);
        // }
        return m_pVo->EnableChn(NVR_PIP_CHN_NUM, stAttr);
    }

    return AX_TRUE;
}

AX_BOOL CAXNVRDisplayCtrl::DisablePip(AX_VOID) {
    if (m_stAttr.enDispDevType == AX_NVR_DISPDEV_TYPE::PRIMARY) {
        // if (m_pFbPaint) {
        //     m_pFbPaint->DestroyChannel(NVR_PIP_CHN_NUM);
        // }
        return m_pVo->DisableChn(NVR_PIP_CHN_NUM);
    }
    return AX_TRUE;
}

AX_VOID CAXNVRDisplayCtrl::StartFBChannels(AX_VOID) {
    this->createFbChannels();
}

AX_VOID CAXNVRDisplayCtrl::StopFBChannels(AX_VOID) {
    this->destoryFbChannels();
}

AX_BOOL CAXNVRDisplayCtrl::UpdateChnResolution(VO_CHN nChn, AX_U32 nWidth, AX_U32 nHeight) {
    if (m_pVo && m_pVo->UpdateChnResolution(nChn, nWidth, nHeight)) {
        return AX_TRUE;
    }

    return AX_FALSE;
}

AX_BOOL CAXNVRDisplayCtrl::UpdateView(const ax_nvr_channel_vector vecViewChns,
                    const vector<AX_NVR_RECT_T> vecVoRect,
                    AX_NVR_VIEW_TYPE enViewType,
                    AX_NVR_VIEW_CHANGE_TYPE enChangeType) {
    AX_BOOL bRet = AX_FALSE;
    LOG_M_D(TAG, "[%s][%d][%d] +++", __func__, __LINE__, m_stAttr.enDispDevType);
    std::lock_guard<std::mutex> lock(mutex_);

    do {
        if (vecViewChns.size() != vecVoRect.size()) {
            LOG_M_E(TAG, "[%s][%d][%d] vecViewChans.size=%d vecVoRect.size=%d", __func__, __LINE__, m_stAttr.enDispDevType, vecViewChns.size(), vecVoRect.size());
            break;
        }

        m_vecRect = vecVoRect;

        if (nullptr == m_pVo) {
            LOG_M_E(TAG, "[%s][%d][%d] VO not created and initialized", __func__, __LINE__, m_stAttr.enDispDevType);
            break;
        }

        AX_BOOL bbRet = AX_TRUE;
        if (AX_NVR_DISPDEV_TYPE::PRIMARY == m_stAttr.enDispDevType) {
            if (AX_NVR_VIEW_CHANGE_TYPE::HIDE == enChangeType) {
                // this->destoryFbChannels();
                VO_ATTR_T attr = m_pVo->GetAttr();
                for (AX_U32 i = 0; i < attr.stChnInfo.nCount; ++i) {
                    m_pVo->ClearChn(i);
                }
            }
            else if (AX_NVR_VIEW_CHANGE_TYPE::SHOW == enChangeType || AX_NVR_VIEW_CHANGE_TYPE::UPDATE == enChangeType) {
                {
                    // this->destoryFbChannels();
                    VO_ATTR_T attr = m_pVo->GetAttr();
                    for (AX_U32 i = 0; i < attr.stChnInfo.nCount; ++i) {
                        if (!m_pVo->HideChn(i)) {
                            bbRet = AX_FALSE;
                            break;
                        }
                    }

                    if (!bbRet) {
                        break;
                    }
                }

                // 0. Update all Channels
                {

                    VO_ATTR_T attr = m_pVo->GetAttr();
                    attr.stChnInfo.nCount = min(vecViewChns.size(), vecVoRect.size());

                    AX_VO_RECT_T stArea;
                    AX_BOOL b4K = is4K(attr.enIntfSync, stArea);
                    for (unsigned int i = 0; i < attr.stChnInfo.nCount; ++i) {
                        auto &viewchn = get<0>(vecViewChns[i]);
                        auto vorect = vecVoRect[i];
                        if (b4K) {
                           vorect = mapRect(vorect, stArea.u32Width, stArea.u32Height);
                        }

                        VO_CHN nVoChn = viewchn%attr.stChnInfo.nCount;
                        attr.stChnInfo.arrChns[nVoChn].stRect = {ALIGN_UP(vorect.x, 2), ALIGN_UP(vorect.y, 2), ALIGN_DOWN(vorect.w, 8), ALIGN_DOWN(vorect.h, 2)};
                        attr.stChnInfo.arrCropRect[nVoChn] = {0, 0, 0, 0};

                        LOG_M_D(TAG, "[%d] %d,%d %dx%d", nVoChn,
                                        attr.stChnInfo.arrChns[nVoChn].stRect.u32X,
                                        attr.stChnInfo.arrChns[nVoChn].stRect.u32Y,
                                        attr.stChnInfo.arrChns[nVoChn].stRect.u32Width,
                                        attr.stChnInfo.arrChns[nVoChn].stRect.u32Height);

                        if (AX_NVR_VIEW_TYPE::PLAYBACK == enViewType) {
                            attr.stChnInfo.arrChns[nVoChn].u32FifoDepth = m_stAttr.u32PlaybakFifoDepth;
                        } else {
                            attr.stChnInfo.arrChns[nVoChn].u32FifoDepth = m_stAttr.u32PreviewFifoDepth;
                        }

                        attr.stChnInfo.arrChns[nVoChn].u32Priority = m_stAttr.u32Priority;
                    }
                    if (!m_pVo->UpdateChnInfo(attr.stChnInfo)) {
                        bbRet = AX_FALSE;
                        break;
                    }

                }

                // Step 1. Show channels
                {
                    VO_ATTR_T attr = m_pVo->GetAttr();
                    for (AX_U32 i = 0; i < attr.stChnInfo.nCount; ++i) {
                        if (!m_pVo->ShowChn(i)) {
                            bbRet = AX_FALSE;
                            break;
                        }
                    }
                    if (!bbRet) {
                        break;
                    }
                }
            }
            // MIN/MAX does not change the actual layout count of VO, only UpdateRect, AX_VO_HideChn and AX_VO_ShowChn
            else if (AX_NVR_VIEW_CHANGE_TYPE::MIN == enChangeType || enChangeType == AX_NVR_VIEW_CHANGE_TYPE::MAX) {

                VO_CHN nVoChn = 0; // vo channel while AX_NVR_VIEW_CHANGE_TYPE::MAX state

                // Step 0. Update all channels
                {
                    VO_ATTR_T attr = m_pVo->GetAttr();
                    AX_VO_RECT_T stArea;
                    AX_BOOL b4K = is4K(attr.enIntfSync, stArea);
                    for (unsigned int i = 0; i < vecViewChns.size() && i < vecVoRect.size(); ++i) {
                        auto &viewchn = get<0>(vecViewChns[i]);
                        auto vorect = vecVoRect[i];
                        if (b4K) {
                           vorect = mapRect(vorect, stArea.u32Width, stArea.u32Height);
                        }
                        nVoChn = viewchn%attr.stChnInfo.nCount;
                        attr.stChnInfo.arrChns[nVoChn].stRect = {ALIGN_UP(vorect.x, 2), ALIGN_UP(vorect.y, 2), ALIGN_DOWN(vorect.w, 8), ALIGN_DOWN(vorect.h, 2)};

                        if (AX_NVR_VIEW_TYPE::PLAYBACK == enViewType) {
                            attr.stChnInfo.arrChns[nVoChn].u32FifoDepth = m_stAttr.u32PlaybakFifoDepth;
                        } else {
                            attr.stChnInfo.arrChns[nVoChn].u32FifoDepth = m_stAttr.u32PreviewFifoDepth;
                        }

                        attr.stChnInfo.arrChns[nVoChn].u32Priority = m_stAttr.u32Priority;
                    }

                    if (!m_pVo->UpdateChnInfo(attr.stChnInfo)) {
                        bbRet = AX_FALSE;
                        break;
                    }
                }

                // Step 1. hide all channels
                {
                    VO_ATTR_T attr = m_pVo->GetAttr();
                    for (AX_U32 i = 0; i < attr.stChnInfo.nCount; ++i) {
                        if (!m_pVo->HideChn(i)) {
                            bbRet = AX_FALSE;
                            break;
                        }
                    }
                    if (!bbRet) {
                        break;
                    }
                }

                // Step 2. Update all channels
                {
                    VO_ATTR_T attr = m_pVo->GetAttr();
                    for (AX_U32 i = 0; i < attr.stChnInfo.nCount; ++i) {
                        if (AX_NVR_VIEW_CHANGE_TYPE::MIN == enChangeType) {
                            if (!m_pVo->ShowChn(i)) {
                                bbRet = AX_FALSE;
                                break;
                            }
                        } else if (AX_NVR_VIEW_CHANGE_TYPE::MAX == enChangeType) {
                            if (i == nVoChn) {
                                if (!m_pVo->ShowChn(i)) {
                                    bbRet = AX_FALSE;
                                    break;
                                }
                            }
                        }
                    }
                    if (!bbRet) {
                        break;
                    }
                }
            }
        }
        bRet = bbRet;
    } while (0);

    LOG_M_D(TAG, "[%s][%d][%d] +++", __func__, __LINE__, m_stAttr.enDispDevType);
    return bRet;
}

AX_BOOL CAXNVRDisplayCtrl::UpdateViewRound(AX_NVR_VIEW_CHANGE_TYPE enChangeType) {

    AX_VO_RECT_T stArea;

    // 4K
    AX_BOOL b4K = AX_FALSE;
    {
        VO_ATTR_T attr = m_pVo->GetAttr();
        b4K = is4K(attr.enIntfSync, stArea);
    }

    if (AX_NVR_VIEW_CHANGE_TYPE::SHOW == enChangeType) {
        if (m_bRoundPatrolStop) {
            m_pVo->Init(m_stAttr.stVoAttr);
            m_pVo->Start();

            m_bRoundPatrolStop = AX_FALSE;
        }

        VO_CHN_INFO_T stChnInfo;
        AX_NVR_VO_SPLITE_TYPE enSplitType = m_szLayoutVideos[m_nCurrentLayoutIndex];
        vector<AX_NVR_RECT_T> vecVoRectIn = m_mapPollingLayout[enSplitType];
        m_vecRect = vecVoRectIn;
        stChnInfo.nCount = vecVoRectIn.size();
        for (int i = 0; i < (int)vecVoRectIn.size(); ++i) {
            auto vorect = vecVoRectIn[i];
            if (b4K) {
                vorect = mapRect(vorect, stArea.u32Width, stArea.u32Height);
            }
            stChnInfo.arrChns[i].stRect = {ALIGN_UP(vorect.x, 2), ALIGN_UP(vorect.y, 2), ALIGN_DOWN(vorect.w, 8), ALIGN_DOWN(vorect.h, 2)};
            stChnInfo.arrChns[i].u32FifoDepth = m_stAttr.u32PreviewFifoDepth;
            stChnInfo.arrChns[i].u32Priority = m_stAttr.u32Priority;
        }

        if (!m_pVo->UpdateChnInfo(stChnInfo)) {
            return AX_FALSE;
        }
    }
    else if (AX_NVR_VIEW_CHANGE_TYPE::HIDE == enChangeType) {
        if (!m_bRoundPatrolStop) {
            m_pVo->Stop();
            m_pVo->DeInit();

            m_nCurrentLayoutIndex = 0;
            m_bRoundPatrolStop = AX_TRUE;
        }
    }
    else if (AX_NVR_VIEW_CHANGE_TYPE::UPDATE == enChangeType) {
        if (m_bRoundPatrolStop) {
            m_pVo->Init(m_stAttr.stVoAttr);
            m_pVo->Start();

            m_bRoundPatrolStop = AX_FALSE;
        }

        m_nCurrentLayoutIndex = getNextLayout();

        VO_CHN_INFO_T stChnInfo;
        AX_NVR_VO_SPLITE_TYPE enSplitType = m_szLayoutVideos[m_nCurrentLayoutIndex];
        vector<AX_NVR_RECT_T> vecVoRectIn = m_mapPollingLayout[enSplitType];
        m_vecRect = vecVoRectIn;
        stChnInfo.nCount = vecVoRectIn.size();
        for (int i = 0; i < (int)vecVoRectIn.size(); ++i) {
            auto vorect = vecVoRectIn[i];
            if (b4K) {
                vorect = mapRect(vorect, stArea.u32Width, stArea.u32Height);
            }
            stChnInfo.arrChns[i].stRect = {ALIGN_UP(vorect.x, 2), ALIGN_UP(vorect.y, 2), ALIGN_DOWN(vorect.w, 8), ALIGN_DOWN(vorect.h, 2)};
            stChnInfo.arrChns[i].u32FifoDepth = m_stAttr.u32PreviewFifoDepth;
            stChnInfo.arrChns[i].u32Priority = m_stAttr.u32Priority;
        }

        if (!m_pVo->UpdateChnInfo(stChnInfo)) {
            return AX_FALSE;
        }
    }
    else {
        // no other action while round patrol.
    }

    return AX_TRUE;
}

#ifdef USE_COLORBAR_FB0_FB1
typedef struct COLOR_RGB24 {
    AX_U32 u32Value: 24;
} __attribute__((__packed__)) COLOR_RGB24_S;

typedef struct COLOR_YUV {
    AX_U8 u8Y;
    AX_U8 u8U;
    AX_U8 u8V;
} COLOR_YUV_S;

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

/* This function takes 8-bit color values */
static inline AX_U32 SHIFT_COLOR8(const UTIL_COLOR_COMPONENT_S *pstComp,
                                  AX_U32 u32Value)
{
    u32Value &= 0xff;
    /* Fill the low bits with the high bits. */
    u32Value = (u32Value << 8) | u32Value;
    /* Shift down to remove unwanted low bits */
    u32Value = u32Value >> AX_U32(16 - pstComp->u32Length);
    /* Shift back up to where the u32Value should be */
    return u32Value << pstComp->u32Offset;
}

#define MAKE_RGBA(stRGB, u8R, u8G, u8B, u8A) \
    (SHIFT_COLOR8(&(stRGB)->stRed, (u8R)) | \
     SHIFT_COLOR8(&(stRGB)->stGreen, (u8G)) | \
     SHIFT_COLOR8(&(stRGB)->stBlue, (u8B)) | \
     SHIFT_COLOR8(&(stRGB)->stAlpha, (u8A)))

#define MAKE_RGB24(stRGB, u8R, u8G, u8B) \
        { .u32Value = MAKE_RGBA(stRGB, u8R, u8G, u8B, 0) }

#define MAKE_YUV_601_Y(u8R, u8G, u8B) \
    ((( 66 * (u8R) + 129 * (u8G) +  25 * (u8B) + 128) >> 8) + 16)
#define MAKE_YUV_601_U(u8R, u8G, u8B) \
    (((-38 * (u8R) -  74 * (u8G) + 112 * (u8B) + 128) >> 8) + 128)
#define MAKE_YUV_601_V(u8R, u8G, u8B) \
    (((112 * (u8R) -  94 * (u8G) -  18 * (u8B) + 128) >> 8) + 128)

#define MAKE_YUV_601(u8R, u8G, u8B) \
    { .u8Y = MAKE_YUV_601_Y(u8R, u8G, u8B), \
      .u8U = MAKE_YUV_601_U(u8R, u8G, u8B), \
      .u8V = MAKE_YUV_601_V(u8R, u8G, u8B) }

#define MAKE_RGB_INFO(u32RedLen, u32RedOffs, u32GreenLen, u32GreenOffs, u32BlueLen, u32BlueOffs, u32AlphaLen, u32AlphaOffs) \
    { .stRed = { (u32RedLen), (u32RedOffs) }, \
      .stGreen = { (u32GreenLen), (u32GreenOffs) }, \
      .stBlue = { (u32BlueLen), (u32BlueOffs) }, \
      .stAlpha = { (u32AlphaLen), (u32AlphaOffs) } }

static AX_VOID Fill_Smpte_RGB32(const UTIL_RGB_INFO_S *stRGB, AX_U8 *pMem,
                                AX_U32 u32Width, AX_U32 u32Height,
                                AX_U32 u32Stride)
{
    const AX_U32 u32ColorsTop[] = {
        MAKE_RGBA(stRGB, 192U, 192U, 192U, 255U),   /* grey */
        MAKE_RGBA(stRGB, 192U, 192U, 0U, 255U), /* yellow */
        MAKE_RGBA(stRGB, 0U, 192U, 192U, 255U), /* cyan */
        MAKE_RGBA(stRGB, 0U, 192U, 0U, 255U),       /* green */
        MAKE_RGBA(stRGB, 192U, 0U, 192U, 255U), /* magenta */
        MAKE_RGBA(stRGB, 192U, 0U, 0U, 255U),       /* red */
        MAKE_RGBA(stRGB, 0U, 0U, 192U, 255U),       /* blue */
    };
    const AX_U32 u32ColorsMiddle[] = {
        MAKE_RGBA(stRGB, 0U, 0U, 192U, 127U),       /* blue */
        MAKE_RGBA(stRGB, 19U, 19U, 19U, 127U),  /* black */
        MAKE_RGBA(stRGB, 192U, 0U, 192U, 127U), /* magenta */
        MAKE_RGBA(stRGB, 19U, 19U, 19U, 127U),  /* black */
        MAKE_RGBA(stRGB, 0U, 192U, 192U, 127U), /* cyan */
        MAKE_RGBA(stRGB, 19U, 19U, 19U, 127U),  /* black */
        MAKE_RGBA(stRGB, 192U, 192U, 192U, 127U),   /* grey */
    };
    const AX_U32 u32ColorsBottom[] = {
        MAKE_RGBA(stRGB, 0U, 33U, 76U, 255U),       /* in-phase */
        MAKE_RGBA(stRGB, 255U, 255U, 255U, 255U),   /* super white */
        MAKE_RGBA(stRGB, 50U, 0U, 106U, 255U),  /* quadrature */
        MAKE_RGBA(stRGB, 19U, 19U, 19U, 255U),  /* black */
        MAKE_RGBA(stRGB, 9U, 9U, 9U, 255U),     /* 3.5% */
        MAKE_RGBA(stRGB, 19U, 19U, 19U, 255),  /* 7.5% */
        MAKE_RGBA(stRGB, 29U, 29U, 29U, 255U),  /* 11.5% */
        MAKE_RGBA(stRGB, 19U, 19U, 19U, 255U),  /* black */
    };
    AX_U32 u32X;
    AX_U32 u32Y;

    for (u32Y = 0; u32Y < u32Height * 6 / 9; ++u32Y) {
        for (u32X = 0; u32X < u32Width; ++u32X)
            ((AX_U32 *)pMem)[u32X] = u32ColorsTop[u32X * 7 / u32Width];
        pMem += u32Stride;
    }

    for (; u32Y < u32Height * 7 / 9; ++u32Y) {
        for (u32X = 0; u32X < u32Width; ++u32X)
            ((AX_U32 *)pMem)[u32X] = u32ColorsMiddle[u32X * 7 / u32Width];
        pMem += u32Stride;
    }

    for (; u32Y < u32Height; ++u32Y) {
        for (u32X = 0; u32X < u32Width * 5 / 7; ++u32X)
            ((AX_U32 *)pMem)[u32X] =
                u32ColorsBottom[u32X * 4 / (u32Width * 5 / 7)];
        for (; u32X < u32Width * 6 / 7; ++u32X)
            ((AX_U32 *)pMem)[u32X] =
                u32ColorsBottom[(u32X - u32Width * 5 / 7) * 3
                                                          / (u32Width / 7) + 4];
        for (; u32X < u32Width; ++u32X)
            ((AX_U32 *)pMem)[u32X] = u32ColorsBottom[7];
        pMem += u32Stride;
    }
}


static AX_VOID Fill_Smpte_RGB16(const UTIL_RGB_INFO_S *stRGB, AX_U8 *pMem,
                                AX_U32 u32Width, AX_U32 u32Height,
                                AX_U32 u32Stride)
{
    const AX_U32 u16ColorsTop[] = {
        MAKE_RGBA(stRGB, 192U, 192U, 192U, 255U),   /* grey */
        MAKE_RGBA(stRGB, 192U, 192U, 0U, 255U), /* yellow */
        MAKE_RGBA(stRGB, 0U, 192U, 192U, 255U), /* cyan */
        MAKE_RGBA(stRGB, 0U, 192U, 0U, 255U),       /* green */
        MAKE_RGBA(stRGB, 192U, 0U, 192U, 255U), /* magenta */
        MAKE_RGBA(stRGB, 192U, 0U, 0U, 255U),       /* red */
        MAKE_RGBA(stRGB, 0U, 0U, 192U, 255U),       /* blue */
    };
    const AX_U32 u16ColorsMiddle[] = {
        MAKE_RGBA(stRGB, 0U, 0U, 192U, 127U),       /* blue */
        MAKE_RGBA(stRGB, 19U, 19U, 19U, 127U),  /* black */
        MAKE_RGBA(stRGB, 192U, 0U, 192U, 127U), /* magenta */
        MAKE_RGBA(stRGB, 19U, 19U, 19U, 127U),  /* black */
        MAKE_RGBA(stRGB, 0U, 192U, 192U, 127U), /* cyan */
        MAKE_RGBA(stRGB, 19U, 19U, 19U, 127U),  /* black */
        MAKE_RGBA(stRGB, 192U, 192U, 192U, 127U),   /* grey */
    };
    const AX_U32 u16ColorsBottom[] = {
        MAKE_RGBA(stRGB, 0U, 33U, 76U, 255U),       /* in-phase */
        MAKE_RGBA(stRGB, 255U, 255U, 255U, 255U),   /* super white */
        MAKE_RGBA(stRGB, 50U, 0U, 106U, 255U),  /* quadrature */
        MAKE_RGBA(stRGB, 19U, 19U, 19U, 255U),  /* black */
        MAKE_RGBA(stRGB, 9U, 9U, 9U, 255U),     /* 3.5% */
        MAKE_RGBA(stRGB, 19U, 19U, 19U, 255U),  /* 7.5% */
        MAKE_RGBA(stRGB, 29U, 29U, 29U, 255U),  /* 11.5% */
        MAKE_RGBA(stRGB, 19U, 19U, 19U, 255U),  /* black */
    };
    AX_U32 u32X;
    AX_U32 u32Y;

    for (u32Y = 0; u32Y < u32Height * 6 / 9; ++u32Y) {
        for (u32X = 0; u32X < u32Width; ++u32X)
            ((AX_U16 *)pMem)[u32X] = u16ColorsTop[u32X * 7 / u32Width];
        pMem += u32Stride;
    }

    for (; u32Y < u32Height * 7 / 9; ++u32Y) {
        for (u32X = 0; u32X < u32Width; ++u32X)
            ((AX_U16 *)pMem)[u32X] = u16ColorsMiddle[u32X * 7 / u32Width];
        pMem += u32Stride;
    }

    for (; u32Y < u32Height; ++u32Y) {
        for (u32X = 0; u32X < u32Width * 5 / 7; ++u32X)
            ((AX_U16 *)pMem)[u32X] =
                u16ColorsBottom[u32X * 4 / (u32Width * 5 / 7)];
        for (; u32X < u32Width * 6 / 7; ++u32X)
            ((AX_U16 *)pMem)[u32X] =
                u16ColorsBottom[(u32X - u32Width * 5 / 7) * 3
                                                          / (u32Width / 7) + 4];
        for (; u32X < u32Width; ++u32X)
            ((AX_U16 *)pMem)[u32X] = u16ColorsBottom[7];
        pMem += u32Stride;
    }
}

static AX_VOID SAMPLE_Fill_Color(AX_IMG_FORMAT_E enPixFmt, AX_U32 u32Width, AX_U32 u32Height, AX_U32 u32Stride, AX_U8 *u8Mem)
{
    UTIL_RGB_INFO_S stAR12_Info = MAKE_RGB_INFO(4, 8, 4, 4, 4, 0, 4, 12);
    UTIL_RGB_INFO_S stAR15_Info = MAKE_RGB_INFO(5, 10, 5, 5, 5, 0, 1, 15);
    UTIL_RGB_INFO_S stRG16_Info = MAKE_RGB_INFO(5, 11, 6, 5, 5, 0, 0, 0);
    UTIL_RGB_INFO_S stRG24_Info = MAKE_RGB_INFO(8, 16, 8, 8, 8, 0, 0, 0);
    UTIL_RGB_INFO_S stAR24_Info = MAKE_RGB_INFO(8, 16, 8, 8, 8, 0, 8, 24);

    // if (enPixFmt == AX_FORMAT_YUV420_SEMIPLANAR) {
    //     Fill_Smpte_YUV(u32Width, u32Height, u32Stride,
    //                    u8Mem, u8Mem + u32Stride * u32Height, u8Mem + u32Stride * u32Height + 1);
    // } else if (enPixFmt == AX_FORMAT_RGB565) {
    //     Fill_Smpte_RGB16(&stRG16_Info, u8Mem, u32Width, u32Height, u32Stride);
    // } else if (enPixFmt == AX_FORMAT_RGB888) {
    //     Fill_Smpte_RGB24(&stRG24_Info, u8Mem, u32Width, u32Height, u32Stride);
    // } else if (enPixFmt == AX_FORMAT_ARGB1555) {
    //     Fill_Smpte_RGB16(&stAR15_Info, u8Mem, u32Width, u32Height, u32Stride);
    // } else if (enPixFmt == AX_FORMAT_ARGB4444) {
    //     Fill_Smpte_RGB16(&stAR12_Info, u8Mem, u32Width, u32Height, u32Stride);
    // } else
    if (enPixFmt == AX_FORMAT_RGB565) {
        Fill_Smpte_RGB16(&stRG16_Info, u8Mem, u32Width, u32Height, u32Stride);
    }
    else if (enPixFmt == AX_FORMAT_ARGB8888) {
        Fill_Smpte_RGB32(&stAR24_Info, u8Mem, u32Width, u32Height, u32Stride);
    } else {
        printf("%s unsupported fomat, fmt: %d\n", __func__, enPixFmt);
    }
}
#endif

AX_S32 CAXNVRDisplayCtrl::initFramebuffer(AX_U32 u32Width, AX_U32 u32Height, AX_U32 u32FbIndex) {

    AX_S32 s32Fd, s32Ret = 0;

    do {
        AX_CHAR fbPath[32];
        struct fb_var_screeninfo stVar;
        // struct fb_bitfield stR32 = {24, 8, 0};
        // struct fb_bitfield stG32 = {16, 8, 0};
        // struct fb_bitfield stB32 = {8, 8, 0};
        // struct fb_bitfield stA32 = {0, 8, 0};
        struct fb_bitfield r = {16, 8, 0};
        struct fb_bitfield g = {8, 8, 0};
        struct fb_bitfield b = {0, 8, 0};
        struct fb_bitfield a = {24, 8, 0};

        /* 1.Open framebuffer device */

        snprintf(fbPath, sizeof(fbPath), "/dev/fb%d", u32FbIndex);
        s32Fd = open(fbPath, O_RDWR);
        if (s32Fd < 0) {
            LOG_M_E(TAG, "open %s failed, err:%s", fbPath, strerror(errno));
            return s32Fd;
        }
        // LOG_M_W(TAG, "open %s success", fbPath);

        /* 2.Get the variable screen info */
        s32Ret = ioctl(s32Fd, FBIOGET_VSCREENINFO, &stVar);
        if (s32Ret < 0) {
            LOG_M_E(TAG, "get variable screen info from fb%d failed", u32FbIndex);
            break;
        }

        /* 3.Modify the variable screen info, the screen size: u32Width*u32Height, the
        * virtual screen size: u32Width*(u32Height*2), the pixel format: ARGB8888
        */
        stVar.xres          = u32Width;
        stVar.xres_virtual  = u32Width;
        stVar.yres          = u32Height;
        stVar.yres_virtual  = u32Height*2;
        stVar.transp        = a;
        stVar.red           = r;
        stVar.green         = g;
        stVar.blue          = b;
        stVar.bits_per_pixel = 32;

        /* 4.Set the variable screeninfo */
        s32Ret = ioctl(s32Fd, FBIOPUT_VSCREENINFO, &stVar);
        if (s32Ret < 0) {
            LOG_M_E(TAG, "put variable screen info to fb%d failed", u32FbIndex);
            break;
        }

        AX_FB_COLORKEY_T stColorKey;
        stColorKey.u16Enable = 1;
        stColorKey.u16Inv = 0;
        stColorKey.u32KeyLow = 0x10101;
        stColorKey.u32KeyHigh = 0x10101;
        s32Ret = ioctl(s32Fd, AX_FBIOPUT_COLORKEY, &stColorKey);
        if (s32Ret < 0) {
            LOG_M_E(TAG, "put colorkey to fb%d failed", u32FbIndex);
            break;
        }

#ifdef USE_COLORBAR_FB0_FB1
        AX_U8 *pShowScreen;
        struct fb_fix_screeninfo stFix;
        AX_S32 i, j = 0;
        AX_U32 *u32Pixel;


        /* 5.Get the fix screen info */
        s32Ret = ioctl(s32Fd, FBIOGET_FSCREENINFO, &stFix);
        if (s32Ret < 0) {
            // SAMPLE_PRT("get fix screen info from fb%d failed\n", u32FbIndex);
            break;
        }

        /* 6.Map the physical video memory for user use */
        pShowScreen = (AX_U8*)mmap(NULL, stFix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, s32Fd, 0);
        if (pShowScreen == (AX_U8 *) - 1) {
            // SAMPLE_PRT("map fb%d failed\n", u32FbIndex);
            break;
        }
        if (u32FbIndex == 1) {
            SAMPLE_Fill_Color(AX_FORMAT_ARGB8888, u32Width, u32Height * 2, u32Width * 4, pShowScreen);
        }
        else if (u32FbIndex == 0) {
            SAMPLE_Fill_Color(AX_FORMAT_RGB565, u32Width, u32Height * 2, u32Width * 4, pShowScreen);
        }

        for (i = u32Height / 4; i < u32Height * 3 / 4; i++) {
            u32Pixel = (AX_U32 *)(pShowScreen + i * u32Width * 4);
            for (j = u32Width / 3; j < u32Width * 2 / 3; j++) {
                u32Pixel[j] &= ~(0xff << 24);
            }
        }

        munmap(pShowScreen, stFix.smem_len);
#endif
    } while(0);

    close(s32Fd);
    return s32Ret;
}

const vector<AX_NVR_RECT_T> CAXNVRDisplayCtrl::calcLayouts(AX_NVR_VO_SPLITE_TYPE enSplitType,
                                                        int nWidth, int nHeight,
                                                        int nLeftMargin, int nTopMargin,
                                                        int nRightMargin, int nBottomMargin) const {

    int cols_ = 0;
    int rows_ = 0;
    switch (enSplitType)
    {
    case AX_NVR_VO_SPLITE_TYPE::ONE:
    case AX_NVR_VO_SPLITE_TYPE::MAX:
        cols_ = 1;
        rows_ = 1;
        break;
    case AX_NVR_VO_SPLITE_TYPE::FOUR:
        cols_ = 2;
        rows_ = 2;
        break;
    case AX_NVR_VO_SPLITE_TYPE::SIXTEEN:
    case AX_NVR_VO_SPLITE_TYPE::EIGHT:
        cols_ = 4;
        rows_ = 4;
        break;
    case AX_NVR_VO_SPLITE_TYPE::THIRTYSIX:
        cols_ = 6;
        rows_ = 6;
        break;
    case AX_NVR_VO_SPLITE_TYPE::SIXTYFOUR:
        cols_ = 8;
        rows_ = 8;
        break;
    default:
        cols_ = 1;
        rows_ = 1;
        break;
    }

    int verticalSpacing = 4;
    int horizontalSpacing = 4;
    int rect_w_tmp = (nWidth - (cols_ - 1) * horizontalSpacing) / cols_;
    int rect_h_tmp = (nHeight - (rows_ - 1) * verticalSpacing) / rows_;
    int rect_w = ALIGN_DOWN(rect_w_tmp, 8);
    int rect_h = ALIGN_DOWN(rect_h_tmp, 2);
    int w = rect_w * cols_ + (cols_ - 1) * horizontalSpacing;
    int h = rect_h * rows_ + (rows_ - 1) * verticalSpacing;
    int left_right = nWidth - w;
    int top_bottom = nHeight - h;
    int left = left_right / 2;
    left = left - left % 2;
    int right = left_right - left;
    int top = top_bottom / 2;
    top = top - top % 2;
    int bottom = top_bottom - top;

    vector<AX_NVR_RECT_T> vecVoRect;
    int pt_x = 0;
    int pt_y = 0;
    if (enSplitType == AX_NVR_VO_SPLITE_TYPE::EIGHT) {
        int index = 0;
        vector<int> channels = {1,2,4,5,6,8,9,10};
        for (AX_U32 i = 0; i < (AX_U32)rows_; ++i) {
            for (AX_U32 j = 0; j < (AX_U32)cols_; ++j) {
                if (std::find(channels.begin(), channels.end(), index) != channels.end()) {
                    index++;
                    continue;
                }

                AX_NVR_RECT_T nvr_rect;
                nvr_rect.x = pt_x + nLeftMargin + left + j * horizontalSpacing + j * rect_w;
                nvr_rect.y = pt_y + nTopMargin + top + i * verticalSpacing + i * rect_h;
                if (index == 0) {
                    nvr_rect.w = rect_w * 3 + horizontalSpacing * 2;
                    nvr_rect.h = rect_h * 3 + verticalSpacing * 2;
                }
                else {
                    nvr_rect.w = rect_w;
                    nvr_rect.h = rect_h;
                }
                vecVoRect.emplace_back(nvr_rect);
                // qDebug() << nvr_rect.x << nvr_rect.y << nvr_rect.w << nvr_rect.h;
                index++;
            }
        }
    } else {
        for (AX_U32 i = 0; i < (AX_U32)rows_; ++i) {
            for (AX_U32 j = 0; j < (AX_U32)cols_; ++j) {
                AX_NVR_RECT_T nvr_rect;
                nvr_rect.x = pt_x + nLeftMargin + left + j * horizontalSpacing + j * rect_w;
                nvr_rect.y = pt_y + nTopMargin + top + i * verticalSpacing + i * rect_h;
                nvr_rect.w = rect_w;
                nvr_rect.h = rect_h;
                // qDebug() << nvr_rect.x << nvr_rect.y << nvr_rect.w << nvr_rect.h;
                vecVoRect.emplace_back(nvr_rect);
            }
        }
    }
    return vecVoRect;
}

AX_BOOL CAXNVRDisplayCtrl::initPollingLayout(AX_U32 u32Width, AX_U32 u32Height) {

#if 0
    struct POINT {
        AX_U32 x, y;
    } pt = {0, 0};

    struct COORDINATE {
        AX_U32 x1, y1, x2, y2;
    } area;

    constexpr AX_U32 BORDER = 8;

    /* calculate video channel rectage */
    for (AX_U32 i = 0; i < NVR_MAX_LAYOUT_NUM; ++i) {
        AX_U32 nVideoCount = m_szLayoutVideos[i];
        AX_U32 nCols = ceil(sqrt((float)nVideoCount));
        AX_U32 nRows = ((nVideoCount % nCols) > 0) ? (nVideoCount / nCols + 1) : (nVideoCount / nCols);
        if (2 == nVideoCount) {
            nCols = 2;
            nRows = 2;
        }
        /* border for both row and col */
        const AX_U32 nAreaW = ALIGN_DOWN(((u32Width - pt.x - BORDER * (nCols - 1)) / nCols), 8);
        const AX_U32 nAreaH = ALIGN_DOWN(((u32Height - pt.y - BORDER * (nRows - 1)) / nRows), 2);

        vector<AX_NVR_RECT_T> vecVoRect;
        for (AX_U32 i = 0; i < nRows; ++i) {
            for (AX_U32 j = 0; j < nCols; ++j) {
                area.x1 = pt.x + j * BORDER + j * nAreaW;
                area.y1 = pt.y + i * BORDER + i * nAreaH;
                area.x2 = area.x1 + nAreaW;
                area.y2 = area.y1 + nAreaH;

                AX_NVR_RECT_T nvr_rect;
                nvr_rect.x = area.x1;
                nvr_rect.y = area.y1;
                nvr_rect.w = area.x2 - area.x1;
                nvr_rect.h = area.y2 - area.y1;
                vecVoRect.emplace_back(nvr_rect);
            }
        }

        m_mapPollingLayout[nVideoCount] = vecVoRect;
    }
#endif
    for (auto enType: m_szLayoutVideos) {
        m_mapPollingLayout[enType] = this->calcLayouts(enType, u32Width, u32Height, 4, 4, 4, 4);
    }
#if 0
    for (auto it : m_mapPollingLayout) {
        printf("count=%d \n", (int)it.first);
        for (auto rect: it.second) {
            printf("[%d]rect=%d %d %d %d \n", (int)it.first, rect.x, rect.y, rect.w, rect.h);
        }
    }
#endif
    return AX_TRUE;
}

AX_BOOL CAXNVRDisplayCtrl::is4K(AX_VO_INTF_SYNC_E eIntfSync, AX_VO_RECT_T& stArea) {
    AX_BOOL b4k = AX_FALSE;
    switch (eIntfSync) {
        case AX_VO_OUTPUT_1080P25:
        case AX_VO_OUTPUT_1080P30:
        case AX_VO_OUTPUT_1080P50:
        case AX_VO_OUTPUT_1080P60:
            stArea.u32Width = 1920;
            stArea.u32Height = 1080;
            b4k = AX_FALSE;
            break;
        case AX_VO_OUTPUT_3840x2160_25:
        case AX_VO_OUTPUT_3840x2160_30:
        case AX_VO_OUTPUT_3840x2160_50:
        case AX_VO_OUTPUT_3840x2160_60:
            stArea.u32Width = 3840;
            stArea.u32Height = 2160;
            b4k = AX_TRUE;
            break;
        case AX_VO_OUTPUT_4096x2160_25:
        case AX_VO_OUTPUT_4096x2160_30:
        case AX_VO_OUTPUT_4096x2160_50:
        case AX_VO_OUTPUT_4096x2160_60:
            stArea.u32Width = 4096;
            stArea.u32Height = 2160;
            b4k = AX_TRUE;
            break;
        default:
            LOG_M_E(TAG, "%s: UnSupport device %d", __func__, eIntfSync);
            return AX_FALSE;
    }
    return b4k;
}

AX_BOOL CAXNVRDisplayCtrl::getDispInfoFromIntfSync(AX_VO_INTF_SYNC_E eIntfSync, AX_VO_RECT_T& stArea, AX_U32& nHz) {
    stArea.u32X = 0;
    stArea.u32Y = 0;
    stArea.u32Width = 1920;
    stArea.u32Height = 1080;
    switch (eIntfSync) {
        case AX_VO_OUTPUT_1080P25: nHz = 25; break;
        case AX_VO_OUTPUT_1080P30: nHz = 30; break;
        case AX_VO_OUTPUT_1080P50: nHz = 50; break;
        case AX_VO_OUTPUT_1080P60: nHz = 60; break;
        case AX_VO_OUTPUT_3840x2160_25: nHz = 25; break;
        case AX_VO_OUTPUT_3840x2160_30: nHz = 30; break;
        case AX_VO_OUTPUT_3840x2160_50: nHz = 50; break;
        case AX_VO_OUTPUT_3840x2160_60: nHz = 60; break;
        case AX_VO_OUTPUT_4096x2160_25: nHz = 25; break;
        case AX_VO_OUTPUT_4096x2160_30: nHz = 30; break;
        case AX_VO_OUTPUT_4096x2160_50: nHz = 50; break;
        case AX_VO_OUTPUT_4096x2160_60: nHz = 60; break;
        default:
            LOG_M_E(TAG, "%s: UnSupport device %d", __func__, eIntfSync);
            return AX_FALSE;
    }

    return AX_TRUE;
}

AX_VOID CAXNVRDisplayCtrl::createFbChannels(AX_VOID) {
    if (m_pFbPaint == nullptr) return;

    VO_ATTR_T attr = m_pVo->GetAttr();
    AX_VO_RECT_T stArea;
    AX_BOOL b4K = is4K(attr.enIntfSync, stArea);
    if (!b4K) {
        for (unsigned int nVoChn = 0; nVoChn < attr.stChnInfo.nCount; ++nVoChn) {
            // printf("createFbChannels (%d, %d, %dx%d)\n", attr.stChnInfo.arrChns[nVoChn].stRect.u32X,
            //             attr.stChnInfo.arrChns[nVoChn].stRect.u32Y,
            //             attr.stChnInfo.arrChns[nVoChn].stRect.u32Width,
            //             attr.stChnInfo.arrChns[nVoChn].stRect.u32Height);

            m_pFbPaint->CreateChannel(nVoChn, FB_RECT_T{attr.stChnInfo.arrChns[nVoChn].stRect.u32X,
                                                        attr.stChnInfo.arrChns[nVoChn].stRect.u32Y,
                                                        attr.stChnInfo.arrChns[nVoChn].stRect.u32Width,
                                                        attr.stChnInfo.arrChns[nVoChn].stRect.u32Height});
        }
    }
    else {
        for (unsigned int nVoChn = 0; nVoChn < m_vecRect.size(); ++nVoChn) {
            // printf("createFbChannels (%d, %d, %dx%d)\n", m_vecRect[nVoChn].x, m_vecRect[nVoChn].y, m_vecRect[nVoChn].w, m_vecRect[nVoChn].h);
            m_pFbPaint->CreateChannel(nVoChn, FB_RECT_T{m_vecRect[nVoChn].x,
                                                        m_vecRect[nVoChn].y,
                                                        m_vecRect[nVoChn].w,
                                                        m_vecRect[nVoChn].h});
        }
    }

    // m_pFbPaint->Start();
}

AX_VOID CAXNVRDisplayCtrl::destoryFbChannels(AX_VOID) {
    if (m_pFbPaint == nullptr) return;
    VO_ATTR_T attr = m_pVo->GetAttr();
    for (unsigned int nVoChn = 0; nVoChn < attr.stChnInfo.nCount; ++nVoChn) {
        m_pFbPaint->DestroyChannel(nVoChn);
    }
    // m_pFbPaint->Stop();
}

AX_NVR_RECT_T CAXNVRDisplayCtrl::mapRect(AX_NVR_RECT_T &rect1920x1080, int dst_width, int dst_height) {

    int x1 = rect1920x1080.x * dst_width / 1920;
    int y1 = rect1920x1080.y * dst_height / 1080;

    int x2 = (rect1920x1080.x + rect1920x1080.w) * dst_width / 1920;
    int y2 = (rect1920x1080.y + rect1920x1080.h) * dst_height / 1080;

    int width = rect1920x1080.w * dst_width / 1920;
    int height = rect1920x1080.h * dst_height / 1080;

    AX_NVR_RECT_T rect3840x2160;
    rect3840x2160.x = x1;
    rect3840x2160.y = y1;
    rect3840x2160.w = width;
    rect3840x2160.h = height;

    return rect3840x2160;
}

AX_S32 CAXNVRDisplayCtrl::getNextLayout() {
    AX_NVR_RPATROL_CONFIG_T tConfig = CNVRConfigParser::GetInstance()->GetRoundPatrolConfig();

    AX_S32 nNext = m_nCurrentLayoutIndex;
    switch (tConfig.nStrategy) {
        case 0: { /* ascending */
            nNext = (nNext + 1) % NVR_MAX_LAYOUT_NUM;
            break;
        }
        case 1: { /* round */
            nNext += 1 * m_nRoundPatrolDirection;
            if (nNext >= (AX_S32)NVR_MAX_LAYOUT_NUM) {
                nNext = (AX_S32)NVR_MAX_LAYOUT_NUM - 2;
                m_nRoundPatrolDirection = -1;
            } else if (nNext < 0) {
                nNext = 1;
                m_nRoundPatrolDirection = 1;
            }
            break;
        }
        case 2: { /* random */
            while ((nNext = g_random_uniform(g_random_engine)) == m_nCurrentLayoutIndex) {
            }
            break;
        }
        default: break;
    }

    return nNext;
}