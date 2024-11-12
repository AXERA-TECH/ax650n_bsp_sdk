/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "vo.hpp"
#include <string.h>
#include <exception>
#include "AXException.hpp"
#include "AppLogApi.h"
#include "ElapsedTimer.hpp"
#include "ax_ivps_api.h"
#include "ax_sys_api.h"
#include "AXNVRFrameworkDefine.h"

#define VO "VO"
#define IS_GRAPHIC_LAYER_ENABLED(layer) ((GRAPHIC_LAYER)-1 != layer)

using namespace ::std;

#if 0
static AX_U64 GetTickCount() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
}
#endif

CVO::CVO(AX_VOID) {
    memset(m_bChnEnable, AX_FALSE, sizeof(m_bChnEnable));
}

CVO* CVO::CreateInstance(VO_ATTR_T& stAttr) {
    CVO* obj = new (std::nothrow) CVO;
    if (obj) {
        if (obj->Init(stAttr)) {
            return obj;
        } else {
            delete obj;
            obj = nullptr;
        }
    }

    return nullptr;
}

AX_BOOL CVO::Destory(AX_VOID) {
    if (!DeInit()) {
        return AX_FALSE;
    }

    delete this;
    return AX_TRUE;
}

AX_BOOL CVO::Init(VO_ATTR_T& stAttr) {
    LOG_M_D(VO, "%s: +++", __func__);

    m_stVoCallbackFun.pfnVsyncEventCallback = NULL;
    m_stVoCallbackFun.pPrivateData = NULL;

    if (!CheckAttr(stAttr)) {
        return AX_FALSE;
    }

    AX_U32 nHz;
    AX_VO_RECT_T stArea;
    if (!GetDispInfoFromIntfSync(stAttr.enIntfSync, stArea, nHz)) {
        return AX_FALSE;
    }

    m_stAttr = stAttr;

    if (AX_VO_PART_MODE_SINGLE == m_stAttr.enDrawMode || AX_VO_MODE_OFFLINE == m_stAttr.enVoMode) {
        m_LayerPool = CreateLayerPool(stAttr.dev, stArea.u32Width * stArea.u32Width * 3 / 2, m_stAttr.nLayerDepth);
        if (AX_INVALID_POOLID == m_LayerPool) {
            return AX_FALSE;
        }
    }

    AX_VO_PUB_ATTR_T stPubAttr;
    memset(&stPubAttr, 0, sizeof(stPubAttr));
    stPubAttr.enIntfType = m_stAttr.enIntfType;
    stPubAttr.enIntfSync = m_stAttr.enIntfSync;
    stPubAttr.stHdmiAttr.bEnableHdmi = m_stAttr.bForceDisplay;
    stPubAttr.enMode = m_stAttr.enVoMode;
    AX_S32 ret = AX_VO_SetPubAttr(m_stAttr.dev, &stPubAttr);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_SetPubAttr(dev %d) fail, ret = 0x%x", m_stAttr.dev, ret);
        return AX_FALSE;
    }

    enum { DEV_ENABLED = 0x1, VOLAYER_CREATED = 0x2, VOLAYER_BINDED = 0x4, UILAYER_BINDED = 0x8 };
    AX_U32 nState = 0;

    do {
        ret = AX_VO_Enable(m_stAttr.dev);
        if (0 != ret) {
            LOG_M_E(VO, "AX_VO_Enable(dev %d) fail, ret = 0x%x", m_stAttr.dev, ret);
            break;
        } else {
            nState |= DEV_ENABLED;
        }

        if (m_stAttr.s32FBIndex[1] != -1) {
            ret = AX_VO_BindGraphicLayer(m_stAttr.s32FBIndex[1], m_stAttr.dev);
            if (0 != ret) {
                LOG_M_E(VO, "AX_VO_BindGraphicLayer(layer %d dev %d) fail, ret = 0x%x", m_stAttr.s32FBIndex[1], m_stAttr.dev, ret);
                break;
            } else {
                nState |= UILAYER_BINDED;
            }
        }

        if (m_stAttr.s32FBIndex[0] != -1) {
            ret = AX_VO_BindGraphicLayer(m_stAttr.s32FBIndex[0], m_stAttr.dev);
            if (0 != ret) {
                LOG_M_E(VO, "AX_VO_BindGraphicLayer(layer %d dev %d) fail, ret = 0x%x", m_stAttr.s32FBIndex[0], m_stAttr.dev, ret);
                break;
            } else {
                nState |= UILAYER_BINDED;
            }
        }

        ret = AX_VO_CreateVideoLayer(&m_stAttr.voLayer);
        if (0 != ret) {
            LOG_M_E(VO, "AX_VO_CreateVideoLayer(layer %d) fail, ret = 0x%x", m_stAttr.voLayer, ret);
            break;
        } else {
            nState |= VOLAYER_CREATED;
        }

        AX_VO_VIDEO_LAYER_ATTR_T stLayerAttr;
        memset(&stLayerAttr, 0, sizeof(stLayerAttr));
        stLayerAttr.stDispRect.u32Width = stArea.u32Width;
        stLayerAttr.stDispRect.u32Height = stArea.u32Height;
        stLayerAttr.stImageSize.u32Width = stArea.u32Width;
        stLayerAttr.stImageSize.u32Height = stArea.u32Height;
        stLayerAttr.enPixFmt = AX_FORMAT_YUV420_SEMIPLANAR;
        stLayerAttr.enSyncMode = m_stAttr.enSyncMode;
        stLayerAttr.f32FrmRate = (AX_F32)(nHz);
        stLayerAttr.u32FifoDepth = m_stAttr.nLayerDepth;
        stLayerAttr.u32BkClr = m_stAttr.nBgClr;
        stLayerAttr.enBgFillMode = AX_VO_BG_FILL_ONCE;
        stLayerAttr.enWBMode = AX_VO_LAYER_WB_POOL;
        stLayerAttr.u32PoolId = m_LayerPool;
        stLayerAttr.u32DispatchMode = m_stAttr.bLinkVo2Disp ? AX_VO_LAYER_OUT_TO_LINK : AX_VO_LAYER_OUT_TO_FIFO;
        stLayerAttr.enPartMode = m_stAttr.enDrawMode;
        stLayerAttr.enBlendMode = AX_VO_BLEND_MODE_DEFAULT;
        stLayerAttr.enEngineMode = m_stAttr.enEngineMode;
        stLayerAttr.u32EngineId = m_stAttr.nEngineId;
        stLayerAttr.bDisplayPreProcess = m_stAttr.bLinkVo2Disp ? AX_FALSE : AX_TRUE;
        ret = AX_VO_SetVideoLayerAttr(m_stAttr.voLayer, &stLayerAttr);
        if (0 != ret) {
            LOG_M_E(VO, "AX_VO_SetVideoLayerAttr(layer %d) fail, ret = 0x%x", m_stAttr.voLayer, ret);
            break;
        } else {
            LOG_M_I(VO, "video layer %d: [(%d, %d) %dx%d], dispatch mode %d, layer depth %d, part mode %d, tolerance %d", m_stAttr.voLayer,
                    stLayerAttr.stDispRect.u32X, stLayerAttr.stDispRect.u32Y, stLayerAttr.stDispRect.u32Width,
                    stLayerAttr.stDispRect.u32Height, stLayerAttr.u32DispatchMode, stLayerAttr.u32FifoDepth, stLayerAttr.enPartMode,
                    stLayerAttr.u32Toleration);
        }

        ret = AX_VO_BindVideoLayer(m_stAttr.voLayer, m_stAttr.dev);
        if (0 != ret) {
            LOG_M_E(VO, "AX_VO_BindVideoLayer(layer %d dev %d) fail, ret = 0x%x", m_stAttr.voLayer, m_stAttr.dev, ret);
            break;
        } else {
            nState |= VOLAYER_BINDED;
        }

        stAttr = m_stAttr;
        if (!stAttr.bLinkVo2Disp) {
            m_vecRegionInfo.resize(NVR_PIP_CHN_NUM + (0 == m_stAttr.voLayer ? 1 /* PIP channel */ : 0));
        }

        m_bInited = AX_TRUE;

        LOG_M_D(VO, "%s: ---", __func__);
        return AX_TRUE;

    } while (0);

    if (VOLAYER_BINDED == (nState & VOLAYER_BINDED)) {
        ret = AX_VO_UnBindVideoLayer(m_stAttr.voLayer, m_stAttr.dev);
        if (0 != ret) {
            LOG_M_E(VO, "AX_VO_UnBindVideoLayer(layer %d dev %d) fail, ret = 0x%x", m_stAttr.voLayer, m_stAttr.dev, ret);
        }
    }

    if (UILAYER_BINDED == (nState & UILAYER_BINDED)) {
        if (m_stAttr.s32FBIndex[0] != -1) {
            ret = AX_VO_UnBindGraphicLayer(m_stAttr.s32FBIndex[0], m_stAttr.dev);
            if (0 != ret) {
                LOG_M_E(VO, "AX_VO_UnBindGraphicLayer(layer %d dev %d) fail, ret = 0x%x", m_stAttr.s32FBIndex[0], m_stAttr.dev, ret);
            }
        }

        if (m_stAttr.s32FBIndex[1] != -1) {
            ret = AX_VO_UnBindGraphicLayer(m_stAttr.s32FBIndex[1], m_stAttr.dev);
            if (0 != ret) {
                LOG_M_E(VO, "AX_VO_UnBindGraphicLayer(layer %d dev %d) fail, ret = 0x%x", m_stAttr.s32FBIndex[1], m_stAttr.dev, ret);
            }
        }
    }

    if (VOLAYER_CREATED == (nState & VOLAYER_CREATED)) {
        ret = AX_VO_DestroyVideoLayer(m_stAttr.voLayer);
        if (0 != ret) {
            LOG_M_E(VO, "AX_VO_DestroyVideoLayer(layer %d) fail, ret = 0x%x", m_stAttr.voLayer, ret);
        }
    }

    if (DEV_ENABLED == (nState & DEV_ENABLED)) {
        ret = AX_VO_Disable(m_stAttr.dev);
        if (0 != ret) {
            LOG_M_E(VO, "AX_VO_Disable(dev %d) fail, ret = 0x%x", m_stAttr.dev, ret);
        }
    }

    return AX_FALSE;
}

AX_BOOL CVO::DeInit(AX_VOID) {
    if (!m_bInited) {
        return AX_TRUE;
    }

#ifdef __DEBUG_STOP__
    LOG_M_C(VO, "%s: +++", __func__);
#else
    LOG_M_D(VO, "%s: +++", __func__);
#endif

    AX_S32 ret;
    ret = AX_VO_UnBindVideoLayer(m_stAttr.voLayer, m_stAttr.dev);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_UnBindVideoLayer(layer %d dev %d) fail, ret = 0x%x", m_stAttr.voLayer, m_stAttr.dev, ret);
        return AX_FALSE;
    }

    if (m_stAttr.s32FBIndex[0] != -1) {
        ret = AX_VO_UnBindGraphicLayer(m_stAttr.s32FBIndex[0], m_stAttr.dev);
        if (0 != ret) {
            LOG_M_E(VO, "AX_VO_UnBindGraphicLayer(layer %d dev %d) fail, ret = 0x%x", m_stAttr.s32FBIndex[0], m_stAttr.dev, ret);
        }
    }

    if (m_stAttr.s32FBIndex[1] != -1) {
        ret = AX_VO_UnBindGraphicLayer(m_stAttr.s32FBIndex[1], m_stAttr.dev);
        if (0 != ret) {
            LOG_M_E(VO, "AX_VO_UnBindGraphicLayer(layer %d dev %d) fail, ret = 0x%x", m_stAttr.s32FBIndex[1], m_stAttr.dev, ret);
        }
    }

    ret = AX_VO_DestroyVideoLayer(m_stAttr.voLayer);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_DestroyVideoLayer(layer %d) fail, ret = 0x%x", m_stAttr.voLayer, ret);
        return AX_FALSE;
    } else {
        m_stAttr.voLayer = (VO_LAYER)-1;
    }

    ret = AX_VO_Disable(m_stAttr.dev);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_Disable(dev %d) fail, ret = 0x%x", m_stAttr.dev, ret);
        return AX_FALSE;
    }

    if (AX_INVALID_POOLID != m_LayerPool) {
        if (!DestoryLayerPool(m_LayerPool)) {
            return AX_FALSE;
        }
    }

#ifdef __DEBUG_STOP__
    LOG_M_C(VO, "%s: ---", __func__);
#else
    LOG_M_D(VO, "%s: ---", __func__);
#endif
    return AX_TRUE;
}

AX_BOOL CVO::Start(AX_VOID) {
    if (m_bStarted) {
        LOG_M_E(VO, "%s: already started", __func__);
        return AX_FALSE;
    }

    LOG_M_D(VO, "%s: +++", __func__);

    if (!EnableVideoLayer()) {
        return AX_FALSE;
    }

    if (!BatchUpdate(AX_TRUE, m_stAttr.stChnInfo)) {
        DisableVideoLayer();
        return AX_FALSE;
    }

    if (!m_stAttr.bLinkVo2Disp) {
        AX_CHAR szName[16];
        sprintf(szName, "NvrVoLayer_%d", m_stAttr.dev);
        if (!m_thread.Start([this](AX_VOID* pArg) -> AX_VOID { LayerFrameGetThread(pArg); }, nullptr, szName)) {
            LOG_M_E(VO, "[Dev_%d] Start VO layer frame get thread fail", m_stAttr.dev);
            return AX_FALSE;
        }
    }

    m_bStarted = AX_TRUE;

    LOG_M_D(VO, "%s: ---", __func__);
    return AX_TRUE;
}

AX_BOOL CVO::Stop(AX_VOID) {
    if (!m_bStarted) {
        return AX_TRUE;
    }

#ifdef __DEBUG_STOP__
    LOG_M_C(VO, "%s: +++", __func__);
#else
    LOG_M_D(VO, "%s: +++", __func__);
#endif

    if (!m_stAttr.bLinkVo2Disp) {
        if (m_thread.IsRunning()) {
            m_thread.Stop();
            m_thread.Join();
        }
    }

    if (!BatchUpdate(AX_FALSE, m_stAttr.stChnInfo)) {
        return AX_FALSE;
    }

    if (!DisableVideoLayer()) {
        return AX_FALSE;
    }

    m_bStarted = AX_FALSE;

#ifdef __DEBUG_STOP__
    LOG_M_C(VO, "%s: ---", __func__);
#else
    LOG_M_D(VO, "%s: ---", __func__);
#endif
    return AX_TRUE;
}

AX_BOOL CVO::BatchUpdate(AX_BOOL bEnable, CONST VO_CHN_INFO_T& stChnInfo) {
    if (0 == stChnInfo.nCount) {
        return AX_TRUE;
    }

    AX_S32 ret = AX_VO_BatchBegin(m_stAttr.voLayer);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_BatchBegin(layer %d) fail, ret = 0x%x", m_stAttr.voLayer, ret);
        return AX_FALSE;
    }

    auto DisableChns = [this](AX_U32 nNumChns) -> AX_VOID {
        for (VO_CHN j = 0; j < nNumChns; ++j) {
            DisableChn(j);
        }

        AX_VO_BatchEnd(m_stAttr.voLayer);
    };

    for (VO_CHN i = 0; i < stChnInfo.nCount; ++i) {
        if (bEnable) {
            if (!SetChnFps(i, stChnInfo.arrFps[i])) {
                DisableChns(i);
                return AX_FALSE;
            }

            if (!EnableChn(i, stChnInfo.arrChns[i])) {
                DisableChns(i);
                return AX_FALSE;
            }
        } else {
            if (!DisableChn(i)) {
                return AX_FALSE;
            }
        }
    }

    ret = AX_VO_BatchEnd(m_stAttr.voLayer);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_BatchEnd(layer %d) fail, ret = 0x%x", m_stAttr.voLayer, ret);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CVO::UpdateChnInfo(CONST VO_CHN_INFO_T& stChnInfo) {
    if (!CheckChnInfo(stChnInfo)) {
        return AX_FALSE;
    }

    if (!m_bStarted) {
        m_stAttr.stChnInfo = stChnInfo;
        if (!m_stAttr.bLinkVo2Disp) {
            std::unique_lock<std::mutex> lck(m_mtxRegions);
            m_vecRegionInfo.resize(NVR_PIP_CHN_NUM + (0 == m_stAttr.voLayer ? 1 /* PIP channel */ : 0));
        }
        return AX_TRUE;
    }

    /* disable all current chns */
    if (m_stAttr.stChnInfo.nCount > 0) {
        if (!BatchUpdate(AX_FALSE, m_stAttr.stChnInfo)) {
            return AX_FALSE;
        }
    }

    if (!BatchUpdate(AX_TRUE, stChnInfo)) {
        return AX_FALSE;
    }

    m_stAttr.stChnInfo = stChnInfo;
    if (!m_stAttr.bLinkVo2Disp) {
        std::unique_lock<std::mutex> lck(m_mtxRegions);
        m_vecRegionInfo.resize(NVR_PIP_CHN_NUM + (0 == m_stAttr.voLayer ? 1 /* PIP channel */ : 0));
    }

    // PRINT_ELAPSED_USEC("switch video layout %02d", m_stAttr.stChnInfo.nCount);
    return AX_TRUE;
}

AX_BOOL CVO::UpdateChnCropRect(VO_CHN voChn, CONST AX_IVPS_RECT_T& stCropRect) {
    m_stAttr.stChnInfo.arrCropRect[voChn].u32X = stCropRect.nX;
    m_stAttr.stChnInfo.arrCropRect[voChn].u32Y = stCropRect.nY;
    m_stAttr.stChnInfo.arrCropRect[voChn].u32Width = stCropRect.nW;
    m_stAttr.stChnInfo.arrCropRect[voChn].u32Height = stCropRect.nH;

    return AX_TRUE;
}

AX_BOOL CVO::UpdateChnResolution(VO_CHN voChn, AX_U32 nWidth, AX_U32 nHeight) {
    m_stAttr.arrImageW[voChn] = nWidth;
    m_stAttr.arrImageH[voChn] = nHeight;

    return AX_TRUE;
}

AX_S32 CVO::CrtcOff() {
    return AX_VO_CrtcOff(m_stAttr.dev);
}

AX_S32 CVO::CrtcOn() {
    return AX_VO_CrtcOn(m_stAttr.dev);
}

AX_S32 CVO::RegCallbackFunc(AX_VSYNC_CALLBACK_FUNC_T& stVoCallbackFun) {
    m_stVoCallbackFun = stVoCallbackFun;
    return AX_VO_VSYNC_RegCallbackFunc(m_stAttr.dev, &m_stVoCallbackFun);
}

AX_S32 CVO::UnRegCallbackFunc() {
    return AX_VO_VSYNC_UnRegCallbackFunc(m_stAttr.dev, &m_stVoCallbackFun);
}

AX_BOOL CVO::SetChnFps(VO_CHN voChn, AX_F32 fps) {
    AX_S32 ret = AX_VO_SetChnFrameRate(m_stAttr.voLayer, voChn, fps);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_SetChnFrameRate(layer %d, chn %d, fps %f) fail, ret = 0x%x", m_stAttr.voLayer, voChn, fps, ret);
        return AX_FALSE;
    }

    LOG_M_N(VO, "set fps of layer %d chn %d to %f", m_stAttr.voLayer, voChn, fps);
    return AX_TRUE;
}

AX_BOOL CVO::SendFrame(VO_CHN voChn, CAXFrame& axFrame, AX_S32 nTimeOut /* = INFINITE*/) {
    LOG_M_D(VO, "%s: layer %d chn %d frame %lld pts %lld phy 0x%llx blkId 0x%x +++", __func__, m_stAttr.voLayer, voChn,
            axFrame.stFrame.stVFrame.stVFrame.u64SeqNum, axFrame.stFrame.stVFrame.stVFrame.u64PTS,
            axFrame.stFrame.stVFrame.stVFrame.u64PhyAddr[0], axFrame.stFrame.stVFrame.stVFrame.u32BlkId[0]);
#ifdef TEST_LATENCY
    static AX_U64 u64BeforeVOSendStreamLatency = 0;
    static AX_U64 u64BeforeVOSendStreamCount = 0;
    AX_U64 u64BeforeCurPts = 0;
    AX_SYS_GetCurPTS(&u64BeforeCurPts);
    if (u64BeforeVOSendStreamCount < 1200) {
        u64BeforeVOSendStreamLatency += u64BeforeCurPts - axFrame.stFrame.stVFrame.stVFrame.u64PrivateData;
        u64BeforeVOSendStreamCount++;
    }
    if (1200 == u64BeforeVOSendStreamCount) {
        LOG_M_W(VO, "===============before vo send frame done: avg latency: %llu us", u64BeforeVOSendStreamLatency / 1200);
        u64BeforeVOSendStreamLatency = 0;
        u64BeforeVOSendStreamCount = 0;
    }
#endif
    AX_S32 ret = AX_VO_SendFrame(m_stAttr.voLayer, voChn, &axFrame.stFrame.stVFrame.stVFrame, nTimeOut);
#ifdef TEST_LATENCY
    static AX_U64 u64VOSendStreamLatency = 0;
    static AX_U64 u64VOSendStreamCount = 0;
    AX_U64 u64CurPts = 0;
    AX_SYS_GetCurPTS(&u64CurPts);
    if (u64VOSendStreamCount < 1200) {
        u64VOSendStreamLatency += u64CurPts - axFrame.stFrame.stVFrame.stVFrame.u64PrivateData;
        u64VOSendStreamCount++;
    }
    if (1200 == u64VOSendStreamCount) {
        LOG_M_W(VO, "===============vo send frame done: avg latency: %llu us\n", u64VOSendStreamLatency / 1200);
        u64VOSendStreamLatency = 0;
        u64VOSendStreamCount = 0;
    }
#endif
    LOG_M_D(VO, "%s: layer %d chn %d frame %lld pts %lld phy 0x%llx blkId 0x%x ---, ret = 0x%x", __func__, m_stAttr.voLayer, voChn,
            axFrame.stFrame.stVFrame.stVFrame.u64SeqNum, axFrame.stFrame.stVFrame.stVFrame.u64PTS,
            axFrame.stFrame.stVFrame.stVFrame.u64PhyAddr[0], axFrame.stFrame.stVFrame.stVFrame.u32BlkId[0], ret);

    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_SendFrame(layer %d, chn %d, timeout %d) fail, ret = 0x%x", m_stAttr.voLayer, voChn, nTimeOut, ret);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CVO::EnableChn(VO_CHN voChn, CONST AX_VO_CHN_ATTR_T& stChnAttr) {
    if (m_bChnEnable[voChn]) {
        LOG_M_E(VO, "chn %d of layer %d is enabled, please disable first", voChn, m_stAttr.voLayer);
        return AX_FALSE;
    }

    AX_S32 ret = AX_VO_SetChnAttr(m_stAttr.voLayer, voChn, &stChnAttr);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_SetChnAttr(layer %d chn %d) fail, ret = 0x%x", m_stAttr.voLayer, voChn, ret);
        return AX_FALSE;
    }

    ret = AX_VO_EnableChn(m_stAttr.voLayer, voChn);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_EnableChn(layer %d chn %d) fail, ret = 0x%x", m_stAttr.voLayer, voChn, ret);
        return AX_FALSE;
    }

    m_bChnEnable[voChn] = AX_TRUE;

    m_stAttr.stChnInfo.arrChns[voChn] = stChnAttr;

    LOG_M_I(VO, "enable chn %d of layer %d [(%d, %d) %dx%d], depth %d prior %d", voChn, m_stAttr.voLayer, stChnAttr.stRect.u32X,
            stChnAttr.stRect.u32Y, stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height, stChnAttr.u32FifoDepth, stChnAttr.u32Priority);
    return AX_TRUE;
}

AX_BOOL CVO::DisableChn(VO_CHN voChn) {
    if (!m_bChnEnable[voChn]) {
        return AX_TRUE;
    }

    AX_S32 ret = AX_VO_DisableChn(m_stAttr.voLayer, voChn);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_DisableChn(layer %d chn %d) fail, ret = 0x%x", m_stAttr.voLayer, voChn, ret);
        return AX_FALSE;
    }

    m_bChnEnable[voChn] = AX_FALSE;

    LOG_M_I(VO, "disable chn %d of layer %d", voChn, m_stAttr.voLayer);
    return AX_TRUE;
}

AX_BOOL CVO::PauseChn(VO_CHN voChn) {
    AX_S32 ret = AX_VO_PauseChn(m_stAttr.voLayer, voChn);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_PauseChn(layer %d chn %d) fail, ret = 0x%x", m_stAttr.voLayer, voChn, ret);
        return AX_FALSE;
    }

    LOG_M_I(VO, "pause chn %d of layer %d", voChn, m_stAttr.voLayer);
    return AX_TRUE;
}

AX_BOOL CVO::RefreshChn(VO_CHN voChn) {
    LOG_M_I(VO, "%s: layer %d chn %d +++", __func__, m_stAttr.voLayer, voChn);
    AX_S32 ret = AX_VO_RefreshChn(m_stAttr.voLayer, voChn);
    LOG_M_I(VO, "%s: layer %d chn %d ---, ret = 0x%x", __func__, m_stAttr.voLayer, voChn, ret);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_RefreshChn(layer %d chn %d) fail, ret = 0x%x", m_stAttr.voLayer, voChn, ret);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CVO::StepChn(VO_CHN voChn) {
    AX_S32 ret = AX_VO_StepChn(m_stAttr.voLayer, voChn);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_StepChn(layer %d chn %d) fail, ret = 0x%x", m_stAttr.voLayer, voChn, ret);
        return AX_FALSE;
    }

    LOG_M_I(VO, "step chn %d of layer %d", voChn, m_stAttr.voLayer);
    return AX_TRUE;
}

AX_BOOL CVO::ResumeChn(VO_CHN voChn) {
    AX_S32 ret = AX_VO_ResumeChn(m_stAttr.voLayer, voChn);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_ResumeChn(layer %d chn %d) fail, ret = 0x%x", m_stAttr.voLayer, voChn, ret);
        return AX_FALSE;
    }

    LOG_M_I(VO, "resume chn %d of layer %d", voChn, m_stAttr.voLayer);
    return AX_TRUE;
}

AX_BOOL CVO::ShowChn(VO_CHN voChn) {
    AX_S32 ret = AX_VO_ShowChn(m_stAttr.voLayer, voChn);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_ShowChn(layer %d chn %d) fail, ret = 0x%x", m_stAttr.voLayer, voChn, ret);
        return AX_FALSE;
    }

    m_stAttr.stChnInfo.arrHidden[voChn] = AX_FALSE;
    return AX_TRUE;
}

AX_BOOL CVO::HideChn(VO_CHN voChn) {
    AX_S32 ret = AX_VO_HideChn(m_stAttr.voLayer, voChn);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_HideChn(layer %d chn %d) fail, ret = 0x%x", m_stAttr.voLayer, voChn, ret);
        return AX_FALSE;
    }

    do {
        std::unique_lock<std::mutex> lck(m_mtxRegions);
        m_stAttr.stChnInfo.arrHidden[voChn] = AX_TRUE;
        if (!m_stAttr.bLinkVo2Disp && voChn < m_vecRegionInfo.size()) {
            m_vecRegionInfo[voChn].bValid = AX_FALSE;
            m_vecRegionInfo[voChn].nChn = -1;
        }
    } while (0);

    LOG_M_I(VO, "hide chn %d of layer %d", voChn, m_stAttr.voLayer);
    return AX_TRUE;
}

AX_BOOL CVO::IsHidden(VO_CHN voChn) {
    if (voChn >= MAX_VO_CHN_NUM) {
        return AX_FALSE;
    }
    return m_stAttr.stChnInfo.arrHidden[voChn];
}

AX_BOOL CVO::ClearChn(VO_CHN voChn, AX_BOOL bClrAll /* = AX_TRUE */) {
    AX_S32 ret = AX_VO_ClearChnBuf(m_stAttr.voLayer, voChn, bClrAll);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_ClearChnBuf(layer %d chn %d ClrAll %d) fail, ret = 0x%x", m_stAttr.voLayer, voChn, bClrAll, ret);
        return AX_FALSE;
    }

    LOG_M_I(VO, "clear chn %d of layer %d", voChn, m_stAttr.voLayer);
    return AX_TRUE;
}

AX_BOOL CVO::GetChnPTS(VO_CHN voChn, AX_U64& pts) {
    AX_S32 ret = AX_VO_GetChnPTS(m_stAttr.voLayer, voChn, &pts);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_GetChnPTS(layer %d chn %d) fail, ret = 0x%x", m_stAttr.voLayer, voChn, ret);
        return AX_FALSE;
    }

    LOG_M_I(VO, "layer %d chn %d pts %lld", m_stAttr.voLayer, voChn, pts);
    return AX_TRUE;
}

AX_BOOL CVO::GetDispInfoFromIntfSync(AX_VO_INTF_SYNC_E eIntfSync, AX_VO_RECT_T& stArea, AX_U32& nHz) {
    stArea.u32X = 0;
    stArea.u32Y = 0;
    switch (eIntfSync) {
        case AX_VO_OUTPUT_1080P25:
            stArea.u32Width = 1920;
            stArea.u32Height = 1080;
            nHz = 25;
            break;
        case AX_VO_OUTPUT_1080P30:
            stArea.u32Width = 1920;
            stArea.u32Height = 1080;
            nHz = 30;
            break;
        case AX_VO_OUTPUT_1080P50:
            stArea.u32Width = 1920;
            stArea.u32Height = 1080;
            nHz = 50;
            break;
        case AX_VO_OUTPUT_1080P60:
            stArea.u32Width = 1920;
            stArea.u32Height = 1080;
            nHz = 60;
            break;
        case AX_VO_OUTPUT_3840x2160_25:
            stArea.u32Width = 3840;
            stArea.u32Height = 2160;
            nHz = 25;
            break;
        case AX_VO_OUTPUT_3840x2160_30:
            stArea.u32Width = 3840;
            stArea.u32Height = 2160;
            nHz = 30;
            break;
        case AX_VO_OUTPUT_3840x2160_50:
            stArea.u32Width = 3840;
            stArea.u32Height = 2160;
            nHz = 50;
            break;
        case AX_VO_OUTPUT_3840x2160_60:
            stArea.u32Width = 3840;
            stArea.u32Height = 2160;
            nHz = 60;
            break;
        case AX_VO_OUTPUT_4096x2160_25:
            stArea.u32Width = 4096;
            stArea.u32Height = 2160;
            nHz = 25;
            break;
        case AX_VO_OUTPUT_4096x2160_30:
            stArea.u32Width = 4096;
            stArea.u32Height = 2160;
            nHz = 30;
            break;
        case AX_VO_OUTPUT_4096x2160_50:
            stArea.u32Width = 4096;
            stArea.u32Height = 2160;
            nHz = 50;
            break;
        case AX_VO_OUTPUT_4096x2160_60:
            stArea.u32Width = 4096;
            stArea.u32Height = 2160;
            nHz = 60;
            break;
        default:
            LOG_M_E(VO, "%s: UnSupport device %d", __func__, eIntfSync);
            return AX_FALSE;
    }

    return AX_TRUE;
}

AX_POOL CVO::CreateLayerPool(VO_DEV dev, AX_U32 nBlkSize, AX_U32 nBlkCnt) {
    AX_POOL_CONFIG_T stPoolCfg;
    memset(&stPoolCfg, 0, sizeof(stPoolCfg));
    stPoolCfg.MetaSize = 4096;
    stPoolCfg.CacheMode = POOL_CACHE_MODE_NONCACHE;
    stPoolCfg.BlkSize = nBlkSize;
    stPoolCfg.BlkCnt = nBlkCnt;
    sprintf((AX_CHAR*)stPoolCfg.PoolName, "vo_dev%d_layer_pool", dev);
    AX_POOL pool = AX_POOL_CreatePool(&stPoolCfg);
    if (AX_INVALID_POOLID == pool) {
        LOG_M_E(VO, "AX_POOL_CreatePool(blkSize %d, blkCnt %d) fail", nBlkSize, nBlkCnt);
    }

    return pool;
}

AX_BOOL CVO::DestoryLayerPool(AX_POOL& pool) {
    if (AX_INVALID_POOLID == pool) {
        LOG_M_E(VO, "Invalid pool id");
        return AX_FALSE;
    }

    AX_S32 ret = AX_POOL_DestroyPool(pool);
    if (0 != ret) {
        LOG_M_E(VO, "AX_POOL_DestroyPool(pool %d) fail, ret = 0x%x", pool, ret);
        return AX_FALSE;
    }

    pool = AX_INVALID_POOLID;
    return AX_TRUE;
}

AX_BOOL CVO::EnableVideoLayer(AX_VOID) {
    AX_S32 ret = AX_VO_EnableVideoLayer(m_stAttr.voLayer);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_EnableVideoLayer(layer %d) fail, ret = 0x%x", m_stAttr.voLayer, ret);
        return AX_FALSE;
    }

    LOG_M_I(VO, "enable layer %d", m_stAttr.voLayer);
    return AX_TRUE;
}

AX_BOOL CVO::DisableVideoLayer(AX_VOID) {
    AX_S32 ret = AX_VO_DisableVideoLayer(m_stAttr.voLayer);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_DisableVideoLayer(layer %d) fail, ret = 0x%x", m_stAttr.voLayer, ret);
        return AX_FALSE;
    }

    LOG_M_I(VO, "disable layer %d", m_stAttr.voLayer);
    return AX_TRUE;
}

AX_BOOL CVO::CheckAttr(VO_ATTR_T& stAttr) {
    /*  allow to set 0 chn, and update chns later
        if (!CheckChnInfo(stAttr.stChnInfo)) {
            return AX_FALSE;
        }
    */

    if (3 /* TDP */ == m_stAttr.nEngineId) {
        LOG_M_W(VO, "set force mode to engine %d", m_stAttr.nEngineId);
        stAttr.enEngineMode = AX_VO_ENGINE_MODE_FORCE;
    }

    return AX_TRUE;
}

AX_BOOL CVO::CheckChnInfo(CONST VO_CHN_INFO_T& stChnInfo) {
    if (!IN_RANGE(1, stChnInfo.nCount, MAX_VO_CHN_NUM)) {
        LOG_M_E(VO, "Invalid video channel count %d", stChnInfo.nCount);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_VOID CVO::LayerFrameGetThread(AX_VOID* pArg) {
    LOG_MM_I(VO, "[%d] +++", m_stAttr.voLayer);

    AX_VIDEO_FRAME_T stVFrame;
    AX_S32 ret;
    VO_LAYER layer = m_stAttr.voLayer;
    CONSTEXPR AX_S32 TIMEOUT = 1000;

    // AX_U64 elapsed = 0;
    // AX_U32 cnt = 0;
    while (1) {
        if (!m_thread.IsRunning()) {
            break;
        }

        ret = AX_VO_GetLayerFrame(layer, &stVFrame, TIMEOUT);
        if (0 != ret) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        } else {
            LOG_MM_D(VO, "AX_VO_GetLayerFrame(Layer: %d, Seq: %lld)", layer, stVFrame.u64SeqNum);
        }

        PaintRegions(&stVFrame);

        ret = AX_VO_SendFrame2Disp(layer, &stVFrame, 0);
        if (0 != ret) {
            LOG_M_E(VO, "AX_VO_SendFrame2Disp(frame %lld, dev %d, layer %d) fail, ret = 0x%x", stVFrame.u64SeqNum, m_stAttr.dev, layer,
                    ret);
        } else {
            LOG_MM_D(VO, "AX_VO_SendFrame2Disp(Layer: %d, Seq: %lld)", layer, stVFrame.u64SeqNum);
        }

        ret = AX_VO_ReleaseLayerFrame(layer, &stVFrame);
        if (0 != ret) {
            LOG_M_E(VO, "AX_VO_ReleaseLayerFrame(frame %lld, dev %d, layer %d) fail, ret = 0x%x", stVFrame.u64SeqNum, m_stAttr.dev, layer,
                    ret);
        } else {
            LOG_MM_D(VO, "AX_VO_ReleaseLayerFrame(Layer: %d, Seq: %lld)", layer, stVFrame.u64SeqNum);
        }
    }

    LOG_MM_I(VO, "[%d] ---", m_stAttr.voLayer);
}

AX_VOID CVO::PaintRegions(AX_VIDEO_FRAME_T* pLayerFrame) {
    std::unique_lock<std::mutex> lck(m_mtxRegions);

    if (nullptr == pLayerFrame) {
        return;
    }

    if (0 == pLayerFrame->u32FrameSize) {
        pLayerFrame->u32FrameSize = pLayerFrame->u32PicStride[0] * pLayerFrame->u32Height * 3 / 2;
    }

#if 0
    AX_VOID* pVirtualAddr = AX_SYS_Mmap(pLayerFrame->u64PhyAddr[0], pLayerFrame->u32FrameSize);
#else
    AX_VOID* pVirtualAddr = AX_POOL_GetBlockVirAddr(pLayerFrame->u32BlkId[0]);
#endif
    if (nullptr == pVirtualAddr) {
        LOG_MM_E(VO, "AX_SYS_Mmap failed for layer frame phy addr: %p", pLayerFrame->u64PhyAddr[0]);
        return;
    }

    AX_IVPS_RGN_CANVAS_INFO_T tCanvas;
    memset(&tCanvas, 0, sizeof(AX_IVPS_RGN_CANVAS_INFO_T));
    tCanvas.eFormat = pLayerFrame->enImgFormat;
    tCanvas.nW = pLayerFrame->u32Width;
    tCanvas.nH = pLayerFrame->u32Height;
    tCanvas.nStride = pLayerFrame->u32PicStride[0];
    tCanvas.nPhyAddr = pLayerFrame->u64PhyAddr[0];
    tCanvas.pVirAddr = pVirtualAddr;

    AX_IVPS_GDI_ATTR_T tGDI;
    memset(&tGDI, 0, sizeof(AX_IVPS_GDI_ATTR_T));
    tGDI.bSolid = AX_FALSE;
    tGDI.nColor = 0xFF8080; /* WHITE */
    tGDI.nThick = 1;

    AX_S32 nRet = AX_SUCCESS;
    AX_IVPS_RECT_T tIvpsRect;
    for (auto& m : m_vecRegionInfo) {
        if (m.bValid) {
            for (auto& rect : m.vecRects) {
                tIvpsRect.nX = rect.nX;
                tIvpsRect.nY = rect.nY;
                tIvpsRect.nW = rect.nW;
                tIvpsRect.nH = rect.nH;

                nRet = AX_IVPS_DrawRect(&tCanvas, tGDI, tIvpsRect);
                if (AX_SUCCESS != nRet) {
                    LOG_MM_E(VO, "AX_IVPS_DrawRect failed, ret=0x%08X", nRet);
                    continue;
                }
            }

            for (auto& line : m.vecLines) {
                AX_IVPS_POINT_T arrPoint[2] = {0};
                arrPoint[0].nX = line.ptStart.nX;
                arrPoint[0].nY = line.ptStart.nY;
                arrPoint[1].nX = line.ptEnd.nX;
                arrPoint[1].nY = line.ptEnd.nY;

                nRet = AX_IVPS_DrawLine(&tCanvas, tGDI, arrPoint, 2);
            }
        }
    }

#if 0
    nRet = AX_SYS_Munmap(pVirtualAddr, pLayerFrame->u32FrameSize);
    if (AX_SUCCESS != nRet) {
        LOG_MM_E(VO, "AX_SYS_Munmap failed for layer frame phy addr: %p, ret=0x%08X", pLayerFrame->u64PhyAddr[0], nRet);
        return;
    }
#endif
}

AX_BOOL CVO::CoordinateConvert(VO_CHN voChn, const DETECT_RESULT_T& tResult) {
    std::unique_lock<std::mutex> lck(m_mtxRegions);

    if (m_stAttr.stChnInfo.arrHidden[voChn]) {
        return AX_FALSE;
    }

    AX_U32 nRegionChn = voChn;
    if (voChn == m_stAttr.pipChn) {
        nRegionChn = NVR_PIP_CHN_NUM;
    }

    m_vecRegionInfo[nRegionChn].Clear();

    AX_U32 nImageW = 0;
    AX_U32 nImageH = 0;
    AX_VO_RECT_T stChnCropRect = m_stAttr.stChnInfo.arrCropRect[voChn];
    if (stChnCropRect.u32Width > 0 && stChnCropRect.u32Height > 0) {
        nImageW = m_stAttr.arrImageW[voChn];
        nImageH = m_stAttr.arrImageH[voChn];

        if (0 == nImageW || 0 == nImageH) {
            return AX_FALSE;
        }
    }

    AX_U32 nDetectSrcW = m_stAttr.nDetectW;
    AX_U32 nDetectSrcH = m_stAttr.nDetectH;

    AX_VO_RECT_T& stChnRect = m_stAttr.stChnInfo.arrChns[voChn].stRect;
    AX_F32 fXRatio = (AX_F32)stChnRect.u32Width / nDetectSrcW;
    AX_F32 fYRatio = (AX_F32)stChnRect.u32Height / nDetectSrcH;

    VO_RECT_ATTR_T tRect{0, 0, 0, 0};
    for (AX_U32 i = 0; i < tResult.nCount; ++i) {
        auto& item = tResult.item[i];

        /* Rect mapping from detect rect to channel rect */
        tRect.nX = (AX_U32)(stChnRect.u32X + item.tBox.fX * fXRatio);
        tRect.nY = (AX_U32)(stChnRect.u32Y + item.tBox.fY * fYRatio);
        tRect.nW = (AX_U32)(item.tBox.fW * fXRatio);
        tRect.nH = (AX_U32)(item.tBox.fH * fYRatio);

        if (voChn == m_stAttr.pipChn) {
            m_vecRegionInfo[nRegionChn].vecRects.emplace_back(tRect.nX, tRect.nY, tRect.nW, tRect.nH);
            m_vecRegionInfo[nRegionChn].bValid = AX_TRUE;
            m_vecRegionInfo[nRegionChn].nChn = voChn;
        } else {
            AX_VO_RECT_T stChnCropRect = m_stAttr.stChnInfo.arrCropRect[voChn];
            if (stChnCropRect.u32Width > 0 && stChnCropRect.u32Height > 0) {
                /* Rect mapping from crop rect to channel rect */
                AX_F32 fXRatio_2 = (AX_F32)stChnRect.u32Width / nImageW;
                AX_F32 fYRatio_2 = (AX_F32)stChnRect.u32Height / nImageH;
                stChnCropRect.u32X = stChnCropRect.u32X * fXRatio_2;
                stChnCropRect.u32Y = stChnCropRect.u32Y * fYRatio_2;
                stChnCropRect.u32X += stChnRect.u32X;
                stChnCropRect.u32Y += stChnRect.u32Y;
                stChnCropRect.u32Width = stChnCropRect.u32Width * stChnRect.u32Width / nImageW;
                stChnCropRect.u32Height = stChnCropRect.u32Height * stChnRect.u32Height / nImageH;

                if (tRect.nX >= stChnCropRect.u32X + stChnCropRect.u32Width || tRect.nY >= stChnCropRect.u32Y + stChnCropRect.u32Height) {
                    /* 去除右侧及下侧框 */
                    continue;
                }

                if (tRect.nX + tRect.nW <= stChnCropRect.u32X || tRect.nY + tRect.nH <= stChnCropRect.u32Y) {
                    /* 去除左侧及上侧框 */
                    continue;
                }

                if (tRect.nX < stChnCropRect.u32X && tRect.nY < stChnCropRect.u32Y &&
                    tRect.nX + tRect.nW > stChnCropRect.u32X + stChnCropRect.u32Width &&
                    tRect.nY + tRect.nH > stChnCropRect.u32Y + stChnCropRect.u32Height) {
                    /* 去除全包围框 */
                    continue;
                }

                if (tRect.nX < stChnCropRect.u32X) {
                    /* 收缩左边界 */
                    tRect.nW -= stChnCropRect.u32X - tRect.nX;
                    tRect.nX = stChnCropRect.u32X;
                }

                if (tRect.nX + tRect.nW > stChnCropRect.u32X + stChnCropRect.u32Width) {
                    /* 收缩右边界 */
                    tRect.nW = stChnCropRect.u32X + stChnCropRect.u32Width - tRect.nX;
                }

                if (tRect.nY < stChnCropRect.u32Y) {
                    /* 收缩上边界 */
                    tRect.nH -= stChnCropRect.u32Y - tRect.nY;
                    tRect.nY = stChnCropRect.u32Y;
                }

                if (tRect.nY + tRect.nH > stChnCropRect.u32Y + stChnCropRect.u32Height) {
                    /* 收缩下边界 */
                    tRect.nH = stChnCropRect.u32Y + stChnCropRect.u32Height - tRect.nY;
                }

                tRect.nX = stChnRect.u32X + (tRect.nX - stChnCropRect.u32X) * stChnRect.u32Width / stChnCropRect.u32Width;
                tRect.nY = stChnRect.u32Y + (tRect.nY - stChnCropRect.u32Y) * stChnRect.u32Height / stChnCropRect.u32Height;
                tRect.nW = tRect.nW * stChnRect.u32Width / stChnCropRect.u32Width;
                tRect.nH = tRect.nH * stChnRect.u32Height / stChnCropRect.u32Height;

                if (tRect.nX < stChnRect.u32X) {
                    /* 收缩左边界 */
                    tRect.nW -= stChnRect.u32X - tRect.nX;
                    tRect.nX = stChnRect.u32X;
                }

                if (tRect.nX + tRect.nW >= stChnRect.u32X + stChnRect.u32Width) {
                    /* 收缩右边界 */
                    tRect.nW = stChnRect.u32X + stChnRect.u32Width - tRect.nX - 2;
                }

                if (tRect.nY < stChnRect.u32Y) {
                    /* 收缩上边界 */
                    tRect.nH -= stChnRect.u32Y - tRect.nY;
                    tRect.nY = stChnRect.u32Y;
                }

                if (tRect.nY + tRect.nH >= stChnRect.u32Y + stChnRect.u32Height) {
                    /* 收缩下边界 */
                    tRect.nH = stChnRect.u32Y + stChnRect.u32Height - tRect.nY - 2;
                }
            }

            vector<VO_RECT_ATTR_T> vecConvertedRect;
            vector<VO_LINE_ATTR_T> vecConvertedLines;
            if (GenRegionsForPipRect(tRect, vecConvertedRect, vecConvertedLines)) {
                for (auto& m : vecConvertedRect) {
                    m_vecRegionInfo[nRegionChn].vecRects.emplace_back(m);
                }

                for (auto& m : vecConvertedLines) {
                    m_vecRegionInfo[nRegionChn].vecLines.emplace_back(m);
                }

                m_vecRegionInfo[nRegionChn].bValid = AX_TRUE;
                m_vecRegionInfo[nRegionChn].nChn = voChn;
            }
        }
    }

    return AX_TRUE;
}

AX_VOID CVO::SetPipRect(const AX_VO_RECT_T& tRect) {
    std::unique_lock<std::mutex> lck(m_mtxRegions);
    if (tRect.u32Width != 0 && tRect.u32Height != 0) {
        m_tPipRect.nX = tRect.u32X - 10; /* PIP channel border width */
        m_tPipRect.nY = tRect.u32Y - 10; /* PIP channel border width */
        m_tPipRect.nW = tRect.u32Width + 20;
        m_tPipRect.nH = tRect.u32Height + 20;
    } else {
        m_tPipRect.nX = 0;
        m_tPipRect.nY = 0;
        m_tPipRect.nW = 0;
        m_tPipRect.nH = 0;
    }
}

AX_BOOL CVO::GenRegionsForPipRect(VO_RECT_ATTR_T& tRect, std::vector<VO_RECT_ATTR_T>& vecOutConvertedRect,
                                  std::vector<VO_LINE_ATTR_T>& vecOutConvertedLines) {
    if (m_tPipRect.nW == 0 || m_tPipRect.nH == 0) {
        /* PIP 未使能，返回原RECT */
        vecOutConvertedRect.emplace_back(tRect);
        return AX_TRUE;
    }

    VO_POINT_ATTR_T ptPipLeftTop = {m_tPipRect.nX, m_tPipRect.nY};
    VO_POINT_ATTR_T ptPipRightTop = {m_tPipRect.nX + m_tPipRect.nW, m_tPipRect.nY};
    VO_POINT_ATTR_T ptPipLeftBottom = {m_tPipRect.nX, m_tPipRect.nY + m_tPipRect.nH};
    VO_POINT_ATTR_T ptPipRightBottom = {m_tPipRect.nX + m_tPipRect.nW, m_tPipRect.nY + m_tPipRect.nH};

    VO_POINT_ATTR_T ptLeftTop = {tRect.nX, tRect.nY};
    VO_POINT_ATTR_T ptRightTop = {tRect.nX + tRect.nW, tRect.nY};
    VO_POINT_ATTR_T ptLeftBottom = {tRect.nX, tRect.nY + tRect.nH};
    VO_POINT_ATTR_T ptRightBottom = {tRect.nX + tRect.nW, tRect.nY + tRect.nH};
    if (PointInRect(ptLeftTop, m_tPipRect)) {
        if (ptRightBottom.nX > ptPipRightBottom.nX && ptRightBottom.nY > ptPipRightBottom.nY) {
            vecOutConvertedLines.emplace_back(ptPipRightTop.nX, ptRightTop.nY, ptRightTop.nX, ptRightTop.nY);
            vecOutConvertedLines.emplace_back(ptRightTop.nX, ptRightTop.nY, ptRightBottom.nX, ptRightBottom.nY);
            vecOutConvertedLines.emplace_back(ptLeftBottom.nX, ptPipLeftBottom.nY, ptLeftBottom.nX, ptLeftBottom.nY);
            vecOutConvertedLines.emplace_back(ptLeftBottom.nX, ptLeftBottom.nY, ptRightBottom.nX, ptRightBottom.nY);
        } else if (ptRightBottom.nY > ptPipRightBottom.nY) {
            vecOutConvertedRect.emplace_back(ptLeftTop.nX, ptPipLeftBottom.nY, tRect.nW, ptLeftBottom.nY - ptPipLeftBottom.nY);
        } else if (ptRightBottom.nX > ptPipRightBottom.nX) {
            vecOutConvertedRect.emplace_back(ptPipRightTop.nX, ptLeftTop.nY, ptRightTop.nX - ptPipRightTop.nX, tRect.nH);
        } else {
            return AX_FALSE;
        }
    } else if (PointInRect(ptRightTop, m_tPipRect)) {
        if (ptLeftBottom.nX < ptPipLeftBottom.nX && ptLeftBottom.nY > ptPipLeftBottom.nY) {
            vecOutConvertedLines.emplace_back(ptLeftTop.nX, ptLeftTop.nY, ptPipLeftTop.nX, ptLeftTop.nY);
            vecOutConvertedLines.emplace_back(ptLeftTop.nX, ptLeftTop.nY, ptLeftBottom.nX, ptLeftBottom.nY);
            vecOutConvertedLines.emplace_back(ptLeftBottom.nX, ptLeftBottom.nY, ptRightBottom.nX, ptRightBottom.nY);
            vecOutConvertedLines.emplace_back(ptRightBottom.nX, ptPipRightBottom.nY, ptRightBottom.nX, ptRightBottom.nY);
        } else if (ptLeftBottom.nY > ptPipLeftBottom.nY) {
            vecOutConvertedRect.emplace_back(ptLeftTop.nX, ptPipLeftBottom.nY, tRect.nW, ptLeftBottom.nY - ptPipLeftBottom.nY);
        } else if (ptLeftBottom.nX < ptPipLeftBottom.nX) {
            vecOutConvertedRect.emplace_back(ptLeftTop.nX, ptLeftTop.nY, ptPipLeftBottom.nX - ptLeftBottom.nX, tRect.nH);
        } else {
            return AX_FALSE;
        }
    } else if (PointInRect(ptLeftBottom, m_tPipRect)) {
        if (ptRightTop.nX > ptPipRightTop.nX && ptRightTop.nY < ptPipRightBottom.nY) {
            vecOutConvertedLines.emplace_back(ptLeftTop.nX, ptLeftTop.nY, ptLeftBottom.nX, ptPipLeftTop.nY);
            vecOutConvertedLines.emplace_back(ptLeftTop.nX, ptLeftTop.nY, ptRightTop.nX, ptRightTop.nY);
            vecOutConvertedLines.emplace_back(ptRightTop.nX, ptRightTop.nY, ptRightBottom.nX, ptRightBottom.nY);
            vecOutConvertedLines.emplace_back(ptPipRightBottom.nX, ptRightBottom.nY, ptRightBottom.nX, ptRightBottom.nY);
        } else if (ptRightTop.nY < ptPipRightBottom.nY) {
            vecOutConvertedRect.emplace_back(ptLeftTop.nX, ptLeftTop.nY, tRect.nW, ptPipLeftTop.nY - ptLeftTop.nY);
        } else if (ptRightTop.nX > ptPipRightTop.nX) {
            vecOutConvertedRect.emplace_back(ptPipRightTop.nX, ptLeftTop.nY, ptRightTop.nX - ptPipRightTop.nX, tRect.nH);
        } else {
            return AX_FALSE;
        }
    } else if (PointInRect(ptRightBottom, m_tPipRect)) {
        if (ptLeftTop.nX < ptPipLeftTop.nX && ptLeftTop.nY < ptPipLeftTop.nY) {
            vecOutConvertedLines.emplace_back(ptLeftTop.nX, ptLeftTop.nY, ptLeftBottom.nX, ptLeftBottom.nY);
            vecOutConvertedLines.emplace_back(ptLeftBottom.nX, ptLeftBottom.nY, ptPipLeftBottom.nX, ptRightBottom.nY);
            vecOutConvertedLines.emplace_back(ptLeftTop.nX, ptLeftTop.nY, ptRightTop.nX, ptRightTop.nY);
            vecOutConvertedLines.emplace_back(ptRightTop.nX, ptRightTop.nY, ptRightBottom.nX, ptPipRightTop.nY);
        } else if (ptLeftTop.nY < ptPipLeftTop.nY) {
            vecOutConvertedRect.emplace_back(ptLeftTop.nX, ptLeftTop.nY, tRect.nW, ptPipLeftTop.nY - ptLeftTop.nY);
        } else if (ptLeftTop.nX < ptPipLeftTop.nX) {
            vecOutConvertedRect.emplace_back(ptLeftTop.nX, ptLeftTop.nY, ptPipLeftBottom.nX - ptLeftBottom.nX, tRect.nH);
        } else {
            return AX_FALSE;
        }
    } else if (PointInRect(ptPipLeftTop, tRect) && PointInRect(ptPipLeftBottom, tRect) && PointInRect(ptPipRightTop, tRect) &&
               PointInRect(ptPipRightBottom, tRect)) {
        /* 完全覆盖PIP通道 */
        vecOutConvertedRect.emplace_back(tRect);
    } else if (ptLeftTop.nX < ptPipLeftTop.nX && ptRightTop.nX > ptPipRightTop.nX) {
        /* 十字交叉 */
        if (ptLeftTop.nY > ptPipLeftTop.nY && ptLeftBottom.nY < ptPipLeftBottom.nY) {
            vecOutConvertedLines.emplace_back(ptLeftTop.nX, ptLeftTop.nY, ptLeftBottom.nX, ptLeftBottom.nY);
            vecOutConvertedLines.emplace_back(ptLeftTop.nX, ptLeftTop.nY, ptPipLeftTop.nX, ptRightTop.nY);
            vecOutConvertedLines.emplace_back(ptPipRightTop.nX, ptLeftTop.nY, ptRightTop.nX, ptRightTop.nY);
            vecOutConvertedLines.emplace_back(ptRightTop.nX, ptRightTop.nY, ptRightBottom.nX, ptRightBottom.nY);
            vecOutConvertedLines.emplace_back(ptLeftBottom.nX, ptLeftBottom.nY, ptPipLeftBottom.nX, ptRightBottom.nY);
            vecOutConvertedLines.emplace_back(ptPipRightBottom.nX, ptLeftBottom.nY, ptRightBottom.nX, ptRightBottom.nY);
        } else if (ptLeftTop.nY < ptPipLeftTop.nY && ptLeftBottom.nY < ptPipLeftBottom.nY) {
            vecOutConvertedLines.emplace_back(ptLeftTop.nX, ptLeftTop.nY, ptLeftBottom.nX, ptLeftBottom.nY);
            vecOutConvertedLines.emplace_back(ptLeftBottom.nX, ptLeftBottom.nY, ptPipLeftBottom.nX, ptRightBottom.nY);
            vecOutConvertedLines.emplace_back(ptLeftTop.nX, ptLeftTop.nY, ptRightTop.nX, ptRightTop.nY);
            vecOutConvertedLines.emplace_back(ptRightTop.nX, ptRightTop.nY, ptRightBottom.nX, ptRightBottom.nY);
            vecOutConvertedLines.emplace_back(ptPipRightBottom.nX, ptLeftBottom.nY, ptRightBottom.nX, ptRightBottom.nY);
        } else if (ptLeftTop.nY > ptPipLeftTop.nY && ptLeftBottom.nY > ptPipLeftBottom.nY) {
            vecOutConvertedLines.emplace_back(ptLeftTop.nX, ptLeftTop.nY, ptLeftBottom.nX, ptLeftBottom.nY);
            vecOutConvertedLines.emplace_back(ptLeftBottom.nX, ptLeftBottom.nY, ptPipLeftBottom.nX, ptRightBottom.nY);
            vecOutConvertedLines.emplace_back(ptLeftTop.nX, ptLeftTop.nY, ptPipLeftTop.nX, ptRightTop.nY);
            vecOutConvertedLines.emplace_back(ptPipRightTop.nX, ptLeftTop.nY, ptRightTop.nX, ptRightTop.nY);
            vecOutConvertedLines.emplace_back(ptRightTop.nX, ptRightTop.nY, ptRightBottom.nX, ptRightBottom.nY);
        }
    } else if (ptLeftTop.nY < ptPipLeftTop.nY && ptLeftBottom.nY > ptPipLeftBottom.nY) {
        /* 十字交叉 */
        if (ptLeftTop.nX > ptPipLeftTop.nX && ptRightTop.nX < ptPipRightTop.nX) {
            vecOutConvertedLines.emplace_back(ptLeftTop.nX, ptLeftTop.nY, ptLeftBottom.nX, ptPipLeftTop.nY);
            vecOutConvertedLines.emplace_back(ptLeftTop.nX, ptLeftTop.nY, ptRightTop.nX, ptRightTop.nY);
            vecOutConvertedLines.emplace_back(ptRightTop.nX, ptRightTop.nY, ptRightBottom.nX, ptPipRightTop.nY);
            vecOutConvertedLines.emplace_back(ptLeftTop.nX, ptPipLeftBottom.nY, ptLeftBottom.nX, ptLeftBottom.nY);
            vecOutConvertedLines.emplace_back(ptLeftBottom.nX, ptLeftBottom.nY, ptRightBottom.nX, ptRightBottom.nY);
            vecOutConvertedLines.emplace_back(ptRightTop.nX, ptPipRightBottom.nY, ptRightBottom.nX, ptRightBottom.nY);
        } else if (ptLeftTop.nX < ptPipLeftTop.nX && ptRightTop.nX < ptPipRightTop.nX) {
            vecOutConvertedLines.emplace_back(ptLeftTop.nX, ptLeftTop.nY, ptLeftBottom.nX, ptLeftBottom.nY);
            vecOutConvertedLines.emplace_back(ptLeftBottom.nX, ptLeftBottom.nY, ptRightBottom.nX, ptRightBottom.nY);
            vecOutConvertedLines.emplace_back(ptLeftTop.nX, ptLeftTop.nY, ptRightTop.nX, ptRightTop.nY);
            vecOutConvertedLines.emplace_back(ptRightTop.nX, ptRightTop.nY, ptRightBottom.nX, ptPipRightTop.nY);
            vecOutConvertedLines.emplace_back(ptRightTop.nX, ptPipRightBottom.nY, ptRightBottom.nX, ptRightBottom.nY);
        } else if (ptLeftTop.nX > ptPipLeftTop.nX && ptRightTop.nX > ptPipRightTop.nX) {
            vecOutConvertedLines.emplace_back(ptLeftTop.nX, ptLeftTop.nY, ptLeftBottom.nX, ptPipLeftTop.nY);
            vecOutConvertedLines.emplace_back(ptLeftTop.nX, ptPipLeftBottom.nY, ptLeftBottom.nX, ptRightBottom.nY);
            vecOutConvertedLines.emplace_back(ptLeftTop.nX, ptLeftTop.nY, ptRightTop.nX, ptRightTop.nY);
            vecOutConvertedLines.emplace_back(ptRightTop.nX, ptRightTop.nY, ptRightBottom.nX, ptRightBottom.nY);
            vecOutConvertedLines.emplace_back(ptLeftBottom.nX, ptLeftBottom.nY, ptRightBottom.nX, ptRightBottom.nY);
        }
    } else {
        /* 无重叠 */
        vecOutConvertedRect.emplace_back(tRect);
    }

    return AX_TRUE;
}

AX_BOOL CVO::PointInRect(VO_POINT_ATTR_T& tPoint, VO_RECT_ATTR_T& tRect) {
    return (tPoint.nX >= tRect.nX && tPoint.nX <= tRect.nX + tRect.nW && tPoint.nY >= tRect.nY && tPoint.nY <= tRect.nY + tRect.nH)
               ? AX_TRUE
               : AX_FALSE;
}

AX_VOID CVO::ClearRegions(VO_CHN voChn) {
    std::unique_lock<std::mutex> lck(m_mtxRegions);

    AX_U32 nRegionChn = voChn;
    if (voChn == m_stAttr.pipChn) {
        nRegionChn = NVR_PIP_CHN_NUM;
    }

    m_vecRegionInfo[nRegionChn].Clear();
}
