/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "Vo.hpp"
#include <math.h>
#include <string.h>
#include <unistd.h>
#include "AXPoolManager.hpp"
#include "AppLogApi.h"
#include "GlobalDef.h"
#if defined(__RECORD_VB_TIMESTAMP__)
#include "TimestampHelper.hpp"
#endif
#include "ax_ivps_api.h"
#include "fs.hpp"

using namespace std;
#define VO "DISP"

AX_BOOL CVo::Init(const VO_ATTR_T& stAttr) {
    LOG_M_D(VO, "%s: +++", __func__);

    AX_U32 nHz;
    AX_U32 u32ChnNr;
    AX_VO_RECT_T stArea;
    if (!GetDispInfoFromIntfSync(stAttr.enIntfSync, stArea, nHz)) {
        return AX_FALSE;
    }

    m_nVideoChnCount = stAttr.arrChns.size();
    m_stAttr = stAttr;
    m_stAttr.nW = stArea.u32Width;
    m_stAttr.nH = stArea.u32Height;
    m_stAttr.nHz = nHz;

    if (!InitLayout(m_nVideoChnCount)) {
        return AX_FALSE;
    }

    for (AX_U32 i = 0; i < m_nVideoChnCount; ++i) {
        VO_CHN voChn = GetVideoChn(i);
        m_arrChns[voChn].u32FifoDepth = m_stAttr.arrChns[i].nDepth;
        m_arrChns[voChn].u32Priority = m_stAttr.arrChns[i].nPriority;
    }

#ifdef __DUMMY_VO__
#else
    AX_S32 ret = AX_VO_CreateVideoLayer(&m_stAttr.voLayer);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_CreateVideoLayer(layer %d) fail, ret = 0x%x", m_stAttr.voLayer, ret);
        return AX_FALSE;
    }

    enum { LAYER_CREATED = 0x1, VODEV_ENABLED = 0x2, LAYER_BINDED = 0x4, LAYER_ENABLED = 0x8 };
    AX_U32 nState = LAYER_CREATED;

    try {
        AX_VO_PUB_ATTR_T stPubAttr;
        memset(&stPubAttr, 0, sizeof(stPubAttr));
        stPubAttr.enIntfType = m_stAttr.enIntfType;
        stPubAttr.enIntfSync = m_stAttr.enIntfSync;
        stPubAttr.enMode = m_stAttr.enMode;
        ret = AX_VO_SetPubAttr(m_stAttr.voDev, &stPubAttr);
        if (0 != ret) {
            LOG_M_E(VO, "AX_VO_SetPubAttr(dev %d) fail, ret = 0x%x", m_stAttr.voDev, ret);
            throw 1;
        }

        ret = AX_VO_Enable(m_stAttr.voDev);
        if (0 != ret) {
            LOG_M_E(VO, "AX_VO_Enable(dev %d) fail, ret = 0x%x", m_stAttr.voDev, ret);
            throw 1;
        } else {
            nState |= VODEV_ENABLED;
        }

        if (!CreatePools(m_stAttr.nLayerDepth)) {
            LOG_M_E(VO, "%s: CreatePool() fail", __func__);
            throw 1;
        }

        AX_VO_VIDEO_LAYER_ATTR_T stLayerAttr;
        memset(&stLayerAttr, 0, sizeof(stLayerAttr));
        stLayerAttr.stDispRect.u32Width = stArea.u32Width;
        stLayerAttr.stDispRect.u32Height = stArea.u32Height;
        stLayerAttr.stImageSize.u32Width = stArea.u32Width;
        stLayerAttr.stImageSize.u32Height = stArea.u32Height;
        stLayerAttr.enPixFmt = AX_FORMAT_YUV420_SEMIPLANAR;

        /* if layer bind to dev, enSynMode is ignored */
        stLayerAttr.enSyncMode = AX_VO_LAYER_SYNC_NORMAL;
        stLayerAttr.f32FrmRate = (AX_F32)(nHz);
        stLayerAttr.u32FifoDepth = m_stAttr.nLayerDepth;
        u32ChnNr = m_arrChns.size();
        stLayerAttr.u32BkClr = m_stAttr.nBgClr;
        stLayerAttr.u32PrimaryChnId = LOGO_CHN;
        stLayerAttr.enBgFillMode = AX_VO_BG_FILL_ONCE;
        stLayerAttr.enWBMode = AX_VO_LAYER_WB_POOL;
        stLayerAttr.u32PoolId = m_LayerPool;
        stLayerAttr.u32DispatchMode = AX_VO_LAYER_OUT_TO_LINK;
        stLayerAttr.enPartMode = AX_VO_PART_MODE_MULTI;
        stLayerAttr.enBlendMode = AX_VO_BLEND_MODE_DEFAULT;
        stLayerAttr.u32Toleration = m_stAttr.nTolerance;
        ret = AX_VO_SetVideoLayerAttr(m_stAttr.voLayer, &stLayerAttr);
        if (0 != ret) {
            LOG_M_E(VO, "AX_VO_SetVideoLayerAttr(layer %d) fail, ret = 0x%x", m_stAttr.voLayer, ret);
            throw 1;
        } else {
            LOG_M_C(VO, "layer %d: [(%d, %d) %dx%d], dispatch mode %d, layer depth %d, part mode %d, tolerance %d",
                    m_stAttr.voLayer, stLayerAttr.stDispRect.u32X, stLayerAttr.stDispRect.u32Y, stLayerAttr.stDispRect.u32Width,
                    stLayerAttr.stDispRect.u32Height, stLayerAttr.u32DispatchMode, stLayerAttr.u32FifoDepth,
                    stLayerAttr.enPartMode, stLayerAttr.u32Toleration);
        }

        ret = AX_VO_BindVideoLayer(m_stAttr.voLayer, m_stAttr.voDev);
        if (0 != ret) {
            LOG_M_E(VO, "AX_VO_BindVideoLayer(layer %d dev %d) fail, ret = 0x%x", m_stAttr.voLayer, m_stAttr.voDev, ret);
            throw 1;
        } else {
            nState |= LAYER_BINDED;
        }

        ret = AX_VO_EnableVideoLayer(m_stAttr.voLayer);
        if (0 != ret) {
            LOG_M_E(VO, "AX_VO_EnableVideoLayer(layer %d) fail, ret = 0x%x", m_stAttr.voLayer, ret);
            throw 1;
        } else {
            nState |= LAYER_ENABLED;
        }

        auto DisableChns = [](VO_LAYER layer, VO_CHN i) -> AX_VOID {
            for (VO_CHN j = 0; j < i; ++j) {
                AX_VO_DisableChn(layer, j);
            }
        };
        for (VO_CHN voChn = 0; voChn < u32ChnNr; ++voChn) {
            ret = AX_VO_SetChnAttr(m_stAttr.voLayer, voChn, &m_arrChns[voChn]);
            if (0 != ret) {
                LOG_M_E(VO, "AX_VO_SetChnAttr(layer %d chn %d) fail, ret = 0x%x", m_stAttr.voLayer, voChn, ret);
                DisableChns(m_stAttr.voLayer, voChn);
                throw 1;
            }

            /* set default fps for all chns including logo and idle */
            if (!SetChnFrameRate(voChn, nHz)) {
                DisableChns(m_stAttr.voLayer, voChn);
                throw 1;
            }
        }

    } catch (...) {
        if (LAYER_ENABLED == (nState & LAYER_ENABLED)) {
            AX_VO_DisableVideoLayer(m_stAttr.voLayer);
        }

        if (LAYER_BINDED == (nState & LAYER_BINDED)) {
            AX_VO_UnBindVideoLayer(m_stAttr.voLayer, m_stAttr.voDev);
        }

        if (VODEV_ENABLED == (nState & VODEV_ENABLED)) {
            AX_VO_Disable(m_stAttr.voDev);
        }

        if (LAYER_CREATED == (nState & LAYER_CREATED)) {
            AX_VO_DestroyVideoLayer(m_stAttr.voLayer);
        }

        return AX_FALSE;
    }
#endif

    m_bInited = AX_TRUE;
    LOG_M_D(VO, "%s: ---", __func__);
    return AX_TRUE;
}

AX_BOOL CVo::DeInit(AX_VOID) {
    LOG_M_D(VO, "%s: +++", __func__);

    if (!m_bInited) {
        return AX_TRUE;
    }
#ifdef __DUMMY_VO__
#else
    AX_S32 ret;
    ret = AX_VO_DisableVideoLayer(m_stAttr.voLayer);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_DisableVideoLayer(layer %d) fail, ret = 0x%x", m_stAttr.voLayer, ret);
        return AX_FALSE;
    }

    ret = AX_VO_UnBindVideoLayer(m_stAttr.voLayer, m_stAttr.voDev);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_UnBindVideoLayer(layer %d dev %d) fail, ret = 0x%x", m_stAttr.voLayer, m_stAttr.voDev, ret);
        return AX_FALSE;
    }

    ret = AX_VO_Disable(m_stAttr.voDev);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_Disable(dev %d) fail, ret = 0x%x", m_stAttr.voDev, ret);
        return AX_FALSE;
    }

    ret = AX_VO_DestroyVideoLayer(m_stAttr.voLayer);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_DestroyVideoLayer(layer %d) fail, ret = 0x%x", m_stAttr.voLayer, ret);
        return AX_FALSE;
    }
#endif
    m_bInited = AX_FALSE;
    LOG_M_D(VO, "%s: ---", __func__);
    return AX_TRUE;
}

AX_BOOL CVo::SetChnFrameRate(VO_CHN voChn, AX_U32 nFps) {
    LOG_M_C(VO, "set layer %d voChn %d fps to %d", m_stAttr.voLayer, voChn, nFps);
#ifdef __DUMMY_VO__
#else
    AX_S32 ret = AX_VO_SetChnFrameRate(m_stAttr.voLayer, voChn, (AX_F32)nFps);
    if (0 != ret) {
        LOG_M_E(VO, "AX_VO_SetChnFrameRate(layer %d chn %d fps %d) fail, ret = 0x%x", m_stAttr.voLayer, voChn, nFps, ret);
        return AX_FALSE;
    }
#endif

    return AX_TRUE;
}

AX_BOOL CVo::Start(AX_VOID) {
    LOG_M_D(VO, "%s: +++", __func__);

#ifdef __DUMMY_VO__
#else
    AX_S32 ret;
    const AX_U32 TOTAL_CHN_NUM = m_arrChns.size();
    for (VO_CHN voChn = 0; voChn < TOTAL_CHN_NUM; ++voChn) {
        LOG_M_D(VO, "enable voChn %d: [(%d, %d) %dx%d], depth %d prior %d", voChn, m_arrChns[voChn].stRect.u32X,
                m_arrChns[voChn].stRect.u32Y, m_arrChns[voChn].stRect.u32Width, m_arrChns[voChn].stRect.u32Height,
                m_arrChns[voChn].u32FifoDepth, m_arrChns[voChn].u32Priority);

        ret = AX_VO_EnableChn(m_stAttr.voLayer, voChn);
        if (0 != ret) {
            LOG_M_E(VO, "AX_VO_EnableChn(layer %d chn %d) fail, ret = 0x%x", m_stAttr.voLayer, voChn, ret);

            for (VO_CHN j = 0; j < voChn; ++j) {
                AX_VO_DisableChn(m_stAttr.voLayer, j);
            }
            return AX_FALSE;
        }
    }

    m_bStarted = AX_TRUE;

    ShowNoVideo();

    if (m_rcLogo.bShow) {
        ShowLogo();
    }
#endif

    LOG_M_D(VO, "%s: ---", __func__);
    return AX_TRUE;
}

AX_BOOL CVo::Stop(AX_VOID) {
    LOG_M_D(VO, "%s: +++", __func__);

    AX_S32 ret;
    const AX_U32 TOTAL_CHN_NUM = m_arrChns.size();
    for (VO_CHN voChn = 0; voChn < TOTAL_CHN_NUM; ++voChn) {
        ret = AX_VO_DisableChn(m_stAttr.voLayer, voChn);
        if (0 != ret) {
            LOG_M_E(VO, "AX_VO_DisableChn(layer %d chn %d) fail, ret = 0x%x", m_stAttr.voLayer, voChn, ret);
            return AX_FALSE;
        }
    }

    m_bStarted = AX_FALSE;
    LOG_M_D(VO, "%s: ---", __func__);
    return AX_TRUE;
}

AX_BOOL CVo::InitLayout(AX_U32 nVideoCount) {
    m_arrChns.clear();

    InitResource();

    struct POINT {
        AX_U32 x, y;
    } pt;

    struct COORDINATE {
        AX_U32 x1, y1, x2, y2;
    } area;

    const AX_U32 BORDER = 8;
    AX_VO_CHN_ATTR_T stChnAttr;
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.u32FifoDepth = 1; /* default depth: 1 */
    stChnAttr.u32Priority = 0;
    stChnAttr.bKeepPrevFr = AX_TRUE;

    if (m_rcLogo.bShow) {
        /**
         *  Example for 2x2 videos
         *  -------------------------------------------------------------------
         *                                LOGO
         *  -------------------------------------------------------------------
         *                                  |
         *            Video 1               |               Video 2
         *                                  |
         *  -------------------------------------------------------------------
         *                                  |
         *            Video 3               |               Video 4
         *                                  |
         * --------------------------------------------------------------------
         **/
        area.x1 = 0;
        area.y1 = 0;
        area.x2 = area.x1 + m_rcLogo.nW;
        area.y2 = area.y1 + m_rcLogo.nH;

        stChnAttr.stRect.u32X = area.x1;
        stChnAttr.stRect.u32Y = area.y1;
        stChnAttr.stRect.u32Width = area.x2 - area.x1;
        stChnAttr.stRect.u32Height = area.y2 - area.y1;

        LOG_M_N(VO, "logo chn position: %d %d %d %d [%d x %d]", area.x1, area.y1, area.x2, area.y2, stChnAttr.stRect.u32Width,
                stChnAttr.stRect.u32Height);
        m_arrChns.push_back(stChnAttr);

        pt.x = 0;
        pt.y = area.y2 + BORDER;
    } else {
        pt.x = 0;
        pt.y = 0;
    }

    AX_U32 nCols = ceil(sqrt((float)nVideoCount));
    AX_U32 nRows = ((nVideoCount % nCols) > 0) ? (nVideoCount / nCols + 1) : (nVideoCount / nCols);
    if (2 == nVideoCount) {
        nCols = 2;
        nRows = 2;
    }

    /* border for both row and col */
    const AX_U32 nAreaW = ALIGN_DOWN(((m_stAttr.nW - pt.x - BORDER * (nCols - 1)) / nCols), 8);
    const AX_U32 nAreaH =
        ALIGN_DOWN(((m_stAttr.nH - pt.y - BORDER * (m_rcLogo.bShow ? nRows : (nRows - 1))) / nRows), 4); /* VDEC FB height align to 4 */

    for (AX_U32 i = 0; i < nRows; ++i) {
        for (AX_U32 j = 0; j < nCols; ++j) {
            if (m_arrChns.size() >= MAX_VO_CHN_NUM) {
                break;
            }
            area.x1 = pt.x + j * BORDER + j * nAreaW;
            area.y1 = pt.y + i * BORDER + i * nAreaH;
            area.x2 = area.x1 + nAreaW;
            area.y2 = area.y1 + nAreaH;

            stChnAttr.stRect.u32X = area.x1;
            stChnAttr.stRect.u32Y = area.y1;
            stChnAttr.stRect.u32Width = area.x2 - area.x1;
            stChnAttr.stRect.u32Height = area.y2 - area.y1;

            LOG_M_N(VO, "video chn %d position: %d %d %d %d [%d x %d]", i * nRows + j, area.x1, area.y1, area.x2, area.y2,
                    stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height);
            m_arrChns.push_back(stChnAttr);
        }
    }

    return AX_TRUE;
}

AX_BOOL CVo::GetDispInfoFromIntfSync(AX_VO_INTF_SYNC_E eIntfSync, AX_VO_RECT_T& stArea, AX_U32& nHz) {
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

vector<AX_VO_RECT_T> CVo::GetVideoLayout(AX_VOID) {
    vector<AX_VO_RECT_T> layout;
    AX_U32 nCount = m_arrChns.size();
    layout.reserve(nCount);
    for (AX_U32 voChn = 0; voChn < nCount; ++voChn) {
        if (IsLogoChn(voChn)) {
            continue;
        }

        layout.push_back(m_arrChns[voChn].stRect);
    }

    return layout;
}

AX_BOOL CVo::SendFrame(VO_CHN voChn, CAXFrame& axFrame, AX_S32 nTimeOut /* = -1 */) {
    if (!m_bStarted) {
        return AX_TRUE;
    }

    LOG_M_I(VO, "send2vo vdGrp %d vdChn %d frame %lld pts %lld phy 0x%llx to layer %d chn %d, blkId 0x%x, +++", axFrame.nGrp, axFrame.nChn,
            axFrame.stFrame.stVFrame.stVFrame.u64SeqNum, axFrame.stFrame.stVFrame.stVFrame.u64PTS,
            axFrame.stFrame.stVFrame.stVFrame.u64PhyAddr[0], m_stAttr.voLayer, voChn, axFrame.stFrame.stVFrame.stVFrame.u32BlkId[0]);

#ifdef __DUMMY_VO__
#else
#if defined(__RECORD_VB_TIMESTAMP__)
    if (!IsLogoChn(voChn) && !IsNoVideoChn(voChn)) {
        CTimestampHelper::RecordTimestamp(axFrame.stFrame.stVFrame.stVFrame, axFrame.nGrp, axFrame.nChn, TIMESTAMP_DISP_PRE_SEND);
    }
#endif

    AX_S32 ret = AX_VO_SendFrame(m_stAttr.voLayer, voChn, &axFrame.stFrame.stVFrame.stVFrame, -1);

#if defined(__RECORD_VB_TIMESTAMP__)
    if (!IsLogoChn(voChn) && !IsNoVideoChn(voChn)) {
        CTimestampHelper::RecordTimestamp(axFrame.stFrame.stVFrame.stVFrame, axFrame.nGrp, axFrame.nChn, TIMESTAMP_DISP_POS_SEND);
    }
#endif

    LOG_M_D(VO, "send2vo vdGrp %d vdChn %d frame %lld pts %lld phy 0x%llx to layer %d chn %d, blkId 0x%x, --- ret = 0x%x", axFrame.nGrp,
            axFrame.nChn, axFrame.stFrame.stVFrame.stVFrame.u64SeqNum, axFrame.stFrame.stVFrame.stVFrame.u64PTS,
            axFrame.stFrame.stVFrame.stVFrame.u64PhyAddr[0], m_stAttr.voLayer, voChn, axFrame.stFrame.stVFrame.stVFrame.u32BlkId[0], ret);
    if (0 != ret) {
        LOG_M_W(VO, "AX_VO_SendFrame(layer %d, chn %d, timeout %d): 0x%x", m_stAttr.voLayer, voChn, nTimeOut, ret);
        return AX_FALSE;
    }
#endif

    return AX_TRUE;
}

AX_BOOL CVo::ShowLogo(AX_VOID) {
    CAXFrame axFrame;
    AX_VIDEO_FRAME_T& stVFrame = axFrame.stFrame.stVFrame.stVFrame;
    stVFrame.u32Width = m_rcLogo.nW;
    stVFrame.u32Height = m_rcLogo.nH;
    stVFrame.enImgFormat = m_rcLogo.eImgFormat;
    stVFrame.u32PicStride[0] = stVFrame.u32Width;
    stVFrame.u32FrameSize = axFrame.GetFrameSize();

    AX_BLK blkId = AX_POOL_GetBlock(m_rcLogo.pool, stVFrame.u32FrameSize, NULL);
    if (AX_INVALID_BLOCKID == blkId) {
        LOG_M_E(VO, "%s: AX_POOL_GetBlock(rc logo pool %d blkSize %d) fail", __func__, m_rcLogo.pool, stVFrame.u32FrameSize);
        return AX_FALSE;
    } else {
        stVFrame.u32BlkId[0] = blkId;
    }

    stVFrame.u64PhyAddr[0] = AX_POOL_Handle2PhysAddr(blkId);
    if (0 == stVFrame.u64PhyAddr[0]) {
        LOG_M_E(VO, "%s: AX_POOL_Handle2PhysAddr(blkId 0x%x) fail", __func__, blkId);
        AX_POOL_ReleaseBlock(blkId);
        return AX_FALSE;
    }

    stVFrame.u64VirAddr[0] = (AX_U64)AX_POOL_GetBlockVirAddr(blkId);
    if (0 == stVFrame.u64VirAddr[0]) {
        LOG_M_E(VO, "%s: AX_POOL_GetBlockVirAddr(blkId 0x%x) fail", __func__, blkId);
        AX_POOL_ReleaseBlock(blkId);
        return AX_FALSE;
    }

    if (!axFrame.LoadFile(m_rcLogo.strPath.c_str())) {
        LOG_M_E(VO, "%s: load logo '%s' fail", __func__, m_rcLogo.strPath.c_str());
        AX_POOL_ReleaseBlock(blkId);
        return AX_FALSE;
    }

    AX_BOOL bOpr = CVo::SendFrame(LOGO_CHN, axFrame, -1);

    AX_POOL_ReleaseBlock(blkId);
    return bOpr;
}

AX_BOOL CVo::ShowNoVideo(AX_VOID) {
    if (!m_rcNoVideo.bShow || (AX_INVALID_POOLID == m_rcNoVideo.pool)) {
        return AX_FALSE;
    }

    CAXFrame axSrcFrame;
    AX_VIDEO_FRAME_T& src = axSrcFrame.stFrame.stVFrame.stVFrame;
    src.u32Width = m_rcNoVideo.nW;
    src.u32Height = m_rcNoVideo.nH;
    src.u32PicStride[0] = src.u32Width;
    src.u32PicStride[1] = src.u32Width;
    src.enImgFormat = m_rcNoVideo.eImgFormat;
    src.u32FrameSize = axSrcFrame.GetFrameSize();
    AX_S32 ret = AX_SYS_MemAlloc(&src.u64PhyAddr[0], (AX_VOID**)&src.u64VirAddr[0], src.u32FrameSize, 128, (const AX_S8*)"novideoimg");
    if (0 != ret) {
        LOG_M_E(VO, "%s: AX_SYS_MemAlloc(%d) fail, ret = 0x%x", __func__, src.u32FrameSize, ret);
        return AX_FALSE;
    } else {
        src.u64PhyAddr[1] = src.u64PhyAddr[0] + src.u32PicStride[0] * src.u32Height;
        src.u64VirAddr[1] = src.u64VirAddr[0] + src.u32PicStride[0] * src.u32Height;
    }

    if (!axSrcFrame.LoadFile(m_rcNoVideo.strPath.c_str())) {
        AX_SYS_MemFree(src.u64PhyAddr[0], (AX_VOID*)src.u64VirAddr[0]);
        return AX_FALSE;
    }

    for (AX_U32 i = ((m_rcLogo.bShow ? 1 : 0) + m_nVideoChnCount); i < m_arrChns.size(); ++i) {
        CAXFrame axDstFrame;
        AX_VIDEO_FRAME_T& dst = axDstFrame.stFrame.stVFrame.stVFrame;
        dst.u32Width = m_arrChns[i].stRect.u32Width;
        dst.u32Height = m_arrChns[i].stRect.u32Height;
        dst.u32PicStride[0] = ALIGN_UP(dst.u32Width, 16);
        dst.u32PicStride[1] = dst.u32PicStride[0];
        dst.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
        dst.u32FrameSize = axDstFrame.GetFrameSize();

        AX_BLK blkId = AX_POOL_GetBlock(m_rcNoVideo.pool, dst.u32FrameSize, NULL);
        if (AX_INVALID_BLOCKID == blkId) {
            LOG_M_E(VO, "%s: AX_POOL_GetBlock(rc novideo pool %d blkSize %d) fail", __func__, m_rcNoVideo.pool, dst.u32FrameSize);
            return AX_FALSE;
        } else {
            dst.u32BlkId[0] = blkId;
        }

        dst.u64PhyAddr[0] = AX_POOL_Handle2PhysAddr(blkId);
        if (0 == dst.u64PhyAddr[0]) {
            LOG_M_E(VO, "%s: AX_POOL_Handle2PhysAddr(blkId 0x%x) fail", __func__, blkId);
            AX_POOL_ReleaseBlock(blkId);
            AX_SYS_MemFree(src.u64PhyAddr[0], (AX_VOID*)src.u64VirAddr[0]);
            return AX_FALSE;
        }

        dst.u64VirAddr[0] = (AX_U64)AX_POOL_GetBlockVirAddr(blkId);
        if (0 == dst.u64VirAddr[0]) {
            LOG_M_E(VO, "%s: AX_POOL_GetBlockVirAddr(blkId 0x%x) fail", __func__, blkId);
            AX_POOL_ReleaseBlock(blkId);
            AX_SYS_MemFree(src.u64PhyAddr[0], (AX_VOID*)src.u64VirAddr[0]);
            return AX_FALSE;
        }

        dst.u64PhyAddr[1] = dst.u64PhyAddr[0] + dst.u32PicStride[0] * dst.u32Height;
        dst.u64VirAddr[1] = dst.u64VirAddr[0] + dst.u32PicStride[0] * dst.u32Height;

        AX_IVPS_ASPECT_RATIO_T aspect;
        memset(&aspect, 0, sizeof(aspect));
        aspect.eMode = AX_IVPS_ASPECT_RATIO_STRETCH;
        ret = AX_IVPS_CropResizeTdp(&src, &dst, &aspect);
        if (0 != ret) {
            LOG_M_E(VO, "%s: AX_IVPS_CropResizeTdp() fail, ret = 0x%x", __func__, ret);
            AX_POOL_ReleaseBlock(blkId);
            AX_SYS_MemFree(src.u64PhyAddr[0], (AX_VOID*)src.u64VirAddr[0]);
            return AX_FALSE;
        }

        AX_BOOL bOpr = CVo::SendFrame(i, axDstFrame, -1);
        if (!bOpr) {
            AX_POOL_ReleaseBlock(blkId);
            AX_SYS_MemFree(src.u64PhyAddr[0], (AX_VOID*)src.u64VirAddr[0]);
            return AX_FALSE;
        }

        AX_POOL_ReleaseBlock(blkId);
    }

    AX_SYS_MemFree(src.u64PhyAddr[0], (AX_VOID*)src.u64VirAddr[0]);
    return AX_TRUE;
}

AX_BOOL CVo::CreatePools(AX_U32 nLayerDepth) {
    AX_POOL_CONFIG_T stPoolCfg;
    memset(&stPoolCfg, 0, sizeof(stPoolCfg));
    stPoolCfg.MetaSize = 512;
    stPoolCfg.CacheMode = POOL_CACHE_MODE_NONCACHE;
    strcpy((AX_CHAR*)stPoolCfg.PartitionName, "anonymous");

    if (AX_VO_MODE_OFFLINE == m_stAttr.enMode) {
        /* [0]: video layer pool */
        stPoolCfg.BlkSize = m_stAttr.nW * m_stAttr.nH * 3 / 2;
        stPoolCfg.BlkCnt = nLayerDepth;
        sprintf((AX_CHAR*)stPoolCfg.PoolName, "vo_dev%d_layer_pool", m_stAttr.voDev);
        m_LayerPool = CAXPoolManager::GetInstance()->CreatePool(stPoolCfg);
        if (AX_INVALID_POOLID == m_LayerPool) {
            return AX_FALSE;
        }
    } else {
        m_LayerPool = AX_INVALID_POOLID;
    }

    /* [1]: log stream pool */
    if (m_rcLogo.bShow) {
        stPoolCfg.BlkSize = m_rcLogo.nW * m_rcLogo.nH * 3 / 2;
        stPoolCfg.BlkCnt = 1;
        sprintf((AX_CHAR*)stPoolCfg.PoolName, "vo_dev%d_logo_pool", m_stAttr.voDev);
        m_rcLogo.pool = CAXPoolManager::GetInstance()->CreatePool(stPoolCfg);
        if (AX_INVALID_POOLID == m_rcLogo.pool) {
            return AX_FALSE;
        }
    }

    /* [2]: video stream pool which has no source input */
    if (m_rcNoVideo.bShow) {
        AX_U32 nLeftChns = m_arrChns.size() - (m_rcLogo.bShow ? 1 : 0) - m_nVideoChnCount;
        if (nLeftChns > 0) {
            AX_U32 n = m_arrChns.size() - 1;
            /* if resize by IVPS TDP, image stride should align to 16 */
            constexpr AX_U32 IVPS_STRIDE_ALIGN = 16;
            stPoolCfg.BlkSize = ALIGN_UP(m_arrChns[n].stRect.u32Width, IVPS_STRIDE_ALIGN) * m_arrChns[n].stRect.u32Height * 3 / 2;
            stPoolCfg.BlkCnt = 1 * nLeftChns;
            sprintf((AX_CHAR*)stPoolCfg.PoolName, "vo_dev%d_novideo_pool", m_stAttr.voDev);
            m_rcNoVideo.pool = CAXPoolManager::GetInstance()->CreatePool(stPoolCfg);
            if (AX_INVALID_POOLID == m_rcNoVideo.pool) {
                return AX_FALSE;
            }
        }
    }

    return AX_TRUE;
}

AX_VOID CVo::InitResource(AX_VOID) {
    m_rcLogo.bShow = AX_FALSE;
    m_rcNoVideo.bShow = AX_FALSE;

    if (m_stAttr.bShowLogo) {
        AX_CHAR szName[32];
        sprintf(szName, "logo_%dx", m_stAttr.nW);
        for (auto& v : fs::directory_iterator(m_stAttr.strResDirPath)) {
            /* logo file name: wxh, nv12 format */
            const fs::path& fpath = v.path();
            if (string::npos != fpath.filename().string().find(szName)) {
                AX_U32 sz = fs::file_size(fpath);
                AX_U32 nW = m_stAttr.nW;
                AX_U32 nH = sz * 2 / 3 / nW;
                if (sz > 0 && nH > 0 && (0 == ::access(fpath.string().c_str(), F_OK | R_OK))) {
                    m_rcLogo.bShow = AX_TRUE;
                    m_rcLogo.strPath = fpath.string();
                    m_rcLogo.nW = nW;
                    m_rcLogo.nH = nH;
                    break;
                }
            }
        }
    }

    if (m_stAttr.bShowNoVideo) {
        /*  no video file:
            1. file name must be novideo_1920x1080.nv12.yuv
            2. 1920x1080 nv12
         */
        m_rcNoVideo.strPath = m_stAttr.strResDirPath + "/novideo_1920x1080.nv12.yuv";
        m_rcNoVideo.nW = 1920;
        m_rcNoVideo.nH = 1080;
        if (0 == ::access(m_rcNoVideo.strPath.c_str(), F_OK | R_OK)) {
            m_rcNoVideo.bShow = AX_TRUE;
        }
    }
}
