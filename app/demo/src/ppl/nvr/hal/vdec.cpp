/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "vdec.hpp"
#include <algorithm>
#include <chrono>
#include <exception>
#include <map>
#include "AXFrame.hpp"
#include "AppLogApi.h"
#include "ax_buffer_tool.h"

#define AX_SHIFT_LEFT_ALIGN(a) (1 << (a))

/* VDEC stride align 256 */
#define VDEC_STRIDE_ALIGN AX_SHIFT_LEFT_ALIGN(8)

#define TAG "VDEC"
#define CHECK_VDEC_GRP(vdGrp) ((vdGrp) >= 0 && (vdGrp) < MAX_VDEC_GRP_NUM)
#define CHECK_VDEC_CHN(vdChn) ((vdChn) >= 0 && (vdChn) < MAX_VDEC_CHN_NUM)

#ifndef ALIGN_UP
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#endif

class CVdecManager {
public:
    CVdecManager(AX_VOID) noexcept {
        for (AX_U32 i = 0; i < MAX_VDEC_GRP_NUM; ++i) {
            m_mapIds[i] = AX_FALSE;
        }
    }

    AX_VDEC_GRP Request(AX_VDEC_GRP vdGrp = MAX_VDEC_GRP_NUM) {
        std::lock_guard<std::mutex> lck(m_mtx);

        if (vdGrp >= 0 && vdGrp < MAX_VDEC_GRP_NUM) {
            if (m_mapIds[vdGrp]) {
                LOG_M_W(TAG, "vdGrp %d is already used", vdGrp);
            } else {
                m_mapIds[vdGrp] = AX_TRUE;
                return vdGrp;
            }
        }

        for (auto&& kv : m_mapIds) {
            if (!kv.second) {
                kv.second = AX_TRUE;
                return kv.first;
            }
        }

        return INVALID_VDEC_GRP;
    }

    AX_BOOL Giveback(AX_VDEC_GRP& vdGrp) {
        if (!CHECK_VDEC_GRP(vdGrp)) {
            return AX_FALSE;
        }

        std::lock_guard<std::mutex> lck(m_mtx);
        m_mapIds[vdGrp] = AX_FALSE;
        vdGrp = INVALID_VDEC_GRP;

        return AX_TRUE;
    }

private:
    std::map<AX_VDEC_GRP, AX_BOOL> m_mapIds;
    std::mutex m_mtx;
};

static CVdecManager theVdecManager;

AX_U32 CVDEC::GetBlkSize(AX_U32 nWidth, AX_U32 nHeight, AX_PAYLOAD_TYPE_E eType,
                         CONST AX_FRAME_COMPRESS_INFO_T* pstCompressInfo /* = nullptr*/,
                         AX_IMG_FORMAT_E ePxlFmt /* = AX_FORMAT_YUV420_SEMIPLANAR*/) {
    return AX_VDEC_GetPicBufferSize(nWidth, nHeight, ePxlFmt, const_cast<AX_FRAME_COMPRESS_INFO_T*>(pstCompressInfo), eType);
}

CVDEC* CVDEC::CreateInstance(CONST VDEC_ATTR_T& stAttr) {
    CVDEC* obj = new (std::nothrow) CVDEC;
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

AX_BOOL CVDEC::Destory(AX_VOID) {
    if (!DeInit()) {
        return AX_FALSE;
    }

    delete this;
    return AX_TRUE;
}

AX_BOOL CVDEC::Init(CONST VDEC_ATTR_T& stAttr) {
    if (!CheckAttr(stAttr)) {
        return AX_FALSE;
    }

    DeInit();

    m_vdGrp = theVdecManager.Request(stAttr.vdGrp);
    if (INVALID_VDEC_GRP == m_vdGrp) {
        LOG_M_E(TAG, "%s: no free vdGrp ID", __func__);
        return AX_FALSE;
    }

    AX_VDEC_GRP_ATTR_T stGrpAttr;
    memset(&stGrpAttr, 0, sizeof(stGrpAttr));
    stGrpAttr.enCodecType = stAttr.enCodecType;
    stGrpAttr.enInputMode = stAttr.enInputMode;
    stGrpAttr.u32MaxPicWidth = ALIGN_UP(stAttr.nWidth, 16);   /* H264 MB 16x16 */
    stGrpAttr.u32MaxPicHeight = ALIGN_UP(stAttr.nHeight, 16); /* H264 MB 16x16 */
    stGrpAttr.u32StreamBufSize = stAttr.nMaxStreamBufSize;
    if (0 == stGrpAttr.u32StreamBufSize) {
        stGrpAttr.u32StreamBufSize = stAttr.nWidth * stAttr.nHeight * 2;
        LOG_M_W(TAG, "vdGrp %d input stream buffer is set to %d", m_vdGrp, stGrpAttr.u32StreamBufSize);
    }

    stGrpAttr.bSdkAutoFramePool = stAttr.bPrivatePool;
    stGrpAttr.bSkipSdkStreamPool = AX_FALSE;

    LOG_M_I(TAG, "vdGrp %d: codec %d, %dx%d, input mode %d, stream buf size 0x%x, private pool %d, playback mode %d", m_vdGrp,
            stGrpAttr.enCodecType, stGrpAttr.u32MaxPicWidth, stGrpAttr.u32MaxPicHeight, stGrpAttr.enInputMode, stGrpAttr.u32StreamBufSize,
            stGrpAttr.bSdkAutoFramePool, stAttr.enDecodeMode);

    AX_S32 ret;

    do {
        ret = AX_VDEC_CreateGrp(m_vdGrp, &stGrpAttr);
        if (0 != ret) {
            LOG_M_E(TAG, "AX_VDEC_CreateGrp(vdGrp %d) fail, ret = 0x%x", m_vdGrp, ret);
            theVdecManager.Giveback(m_vdGrp);
            break;
        }

        AX_VDEC_GRP_PARAM_T stGrpParam;
        memset(&stGrpParam, 0, sizeof(stGrpParam));
        /*
            tick: 6298
            In order to decrease VB num, config VDEC to decode order (no reorder)
            Make sure video stream has no B frames
        */
        stGrpParam.stVdecVideoParam.enVdecMode = stAttr.enIPBMode;
        stGrpParam.stVdecVideoParam.enOutputOrder = (VIDEO_DEC_MODE_I == stAttr.enIPBMode) ? AX_VDEC_OUTPUT_ORDER_DISP : AX_VDEC_OUTPUT_ORDER_DEC;
        stGrpParam.f32SrcFrmRate = stAttr.nFps;
        ret = AX_VDEC_SetGrpParam(m_vdGrp, &stGrpParam);
        if (0 != ret) {
            LOG_M_E(TAG, "AX_VDEC_SetGrpParam(vdGrp %d) fail, ret = 0x%x", m_vdGrp, ret);
            break;
        }

        ret = AX_VDEC_SetDisplayMode(m_vdGrp, stAttr.enDecodeMode);
        if (0 != ret) {
            LOG_M_E(TAG, "AX_VDEC_SetGrpParam(vdGrp %d) fail, ret = 0x%x", m_vdGrp, ret);
            break;
        }

        for (AX_VDEC_CHN vdChn = 0; vdChn < MAX_VDEC_CHN_NUM; ++vdChn) {
            if (stAttr.stChnAttr[vdChn].bEnable) {
                CONST AX_VDEC_CHN_ATTR_T& stChnAttr = stAttr.stChnAttr[vdChn].stAttr;

                LOG_M_I(TAG, "vdChn %d: %dx%d stride %d padding %d, fifo depth %d, mode %d, vb(size %d, cnt %d), compress %d lv %d", vdChn,
                        stChnAttr.u32PicWidth, stChnAttr.u32PicHeight, stChnAttr.u32FrameStride, stChnAttr.u32FramePadding,
                        stChnAttr.u32OutputFifoDepth, stChnAttr.enOutputMode, stChnAttr.u32FrameBufSize, stChnAttr.u32FrameBufCnt,
                        stChnAttr.stCompressInfo.enCompressMode, stChnAttr.stCompressInfo.u32CompressLevel);

                ret = AX_VDEC_SetChnAttr(m_vdGrp, vdChn, &stChnAttr);
                if (0 != ret) {
                    LOG_M_E(TAG, "AX_VDEC_SetChnAttr(vdGrp %d, vdChn %d) fail, ret = 0x%x", m_vdGrp, vdChn, ret);
                    goto __FAIL__;
                }

                ret = AX_VDEC_EnableChn(m_vdGrp, vdChn);
                if (0 != ret) {
                    LOG_M_E(TAG, "AX_VDEC_EnableChn(vdGrp %d, vdChn %d) fail, ret = 0x%x", m_vdGrp, vdChn, ret);
                    goto __FAIL__;
                }
            } else {
                ret = AX_VDEC_DisableChn(m_vdGrp, vdChn);
                if (0 != ret) {
                    LOG_M_E(TAG, "AX_VDEC_DisableChn(vdGrp %d, vdChn %d) fail, ret = 0x%x", m_vdGrp, vdChn, ret);
                    goto __FAIL__;
                }
            }
        }

        m_stAttr = stAttr;
        return AX_TRUE;

    } while (0);

__FAIL__:
    DeInit();
    return AX_FALSE;
}

AX_BOOL CVDEC::DeInit(AX_VOID) {
    if (!CHECK_VDEC_GRP(m_vdGrp)) {
        return AX_TRUE;
    }

#ifdef __DEBUG_STOP__
    LOG_M_C(TAG, "%s: vdGrp %d +++", __func__, m_vdGrp);
#endif

    AX_S32 ret;
    for (AX_VDEC_CHN vdChn = 0; vdChn < MAX_VDEC_CHN_NUM; ++vdChn) {
        if (m_stAttr.stChnAttr[vdChn].bEnable) {
            ret = AX_VDEC_DisableChn(m_vdGrp, vdChn);
            if (0 != ret) {
                LOG_M_E(TAG, "AX_VDEC_DisableChn(vdGrp %d, vdChn %d) fail, ret = 0x%x", m_vdGrp, vdChn, ret);
                return AX_FALSE;
            }
        }
    }

    LOG_M_D(TAG, "AX_VDEC_DestroyGrp(vdGrp %d) +++", m_vdGrp);
    ret = AX_VDEC_DestroyGrp(m_vdGrp);
    LOG_M_D(TAG, "AX_VDEC_DestroyGrp(vdGrp %d) ---", m_vdGrp);
    if (0 != ret) {
        LOG_M_E(TAG, "AX_VDEC_DestroyGrp(vdGrp %d) fail, ret = 0x%x", m_vdGrp, ret);
        return AX_FALSE;
    }

#ifdef __DEBUG_STOP__
    LOG_M_C(TAG, "%s: vdGrp %d ---", __func__, m_vdGrp);
#endif

    theVdecManager.Giveback(m_vdGrp);
    return AX_TRUE;
}

AX_BOOL CVDEC::CheckAttr(CONST VDEC_ATTR_T& stAttr) {
    if (0 == stAttr.nWidth || 0 == stAttr.nHeight) {
        LOG_M_E(TAG, "invalid vdec output width %d or height %d", stAttr.nWidth, stAttr.nHeight);
        return AX_FALSE;
    }

    if (0 == std::count_if(std::begin(stAttr.stChnAttr), std::end(stAttr.stChnAttr), [](CONST VDEC_CHN_ATTR_T& m) { return m.bEnable; })) {
        LOG_M_E(TAG, "at least 1 output channel should be enabled");
        return AX_FALSE;
    }

    for (AX_U32 i = 0; i < MAX_VDEC_CHN_NUM; ++i) {
        if (stAttr.stChnAttr[i].bEnable && !stAttr.stChnAttr[i].bLinked && 0 == stAttr.stChnAttr[i].stAttr.u32OutputFifoDepth) {
            LOG_M_E(TAG, "output channel %d is unlinked, but out fifo depth is 0", i);
            return AX_FALSE;
        }
    }

    return AX_TRUE;
}

AX_BOOL CVDEC::Start(AX_VOID) {
    if (m_bStarted) {
        LOG_M_W(TAG, "vdGrp %d is already started", m_vdGrp);
        return AX_TRUE;
    }

    LOG_M_I(TAG, "%s: vdGrp %d +++", __func__, m_vdGrp);

    AX_VDEC_RECV_PIC_PARAM_T stRecvParam;
    memset(&stRecvParam, 0, sizeof(stRecvParam));
    stRecvParam.s32RecvPicNum = -1;
    AX_S32 ret = AX_VDEC_StartRecvStream(m_vdGrp, &stRecvParam);
    if (0 != ret) {
        LOG_M_E(TAG, "AX_VDEC_StartRecvStream(vdGrp %d) fail, ret = 0x%x", m_vdGrp);
        return AX_FALSE;
    }

    {
        std::lock_guard<std::mutex> lck(m_mtxObs);
        for (AX_VDEC_CHN vdChn = 0; vdChn < MAX_VDEC_CHN_NUM; ++vdChn) {
            if (m_lstObs[vdChn].size() > 0) {
                CDecodeTask::GetInstance()->RegisterObservers(m_vdGrp, vdChn, m_lstObs[vdChn]);
            }
        }
    }

    m_nLastSendCode = 0;
    m_bStarted = AX_TRUE;

    LOG_M_I(TAG, "%s: vdGrp %d ---", __func__, m_vdGrp);
    return AX_TRUE;
}

AX_BOOL CVDEC::Stop(AX_VOID) {
    if (!m_bStarted) {
        LOG_M_W(TAG, "%s: vdGrp %d is not started yet", __func__, m_vdGrp);
        return AX_TRUE;
    }

#ifdef __DEBUG_STOP__
    LOG_M_C(TAG, "%s: vdGrp %d +++", __func__, m_vdGrp);
#else
    LOG_M_I(TAG, "%s: vdGrp %d +++", __func__, m_vdGrp);
#endif

    AX_S32 ret;

#if 0
    AX_VDEC_STREAM_T eof;
    memset(&eof, 0, sizeof(eof));
    eof.bEndOfFrame = AX_TRUE;
    eof.bEndOfStream = AX_TRUE;
    ret = AX_VDEC_SendStream(m_vdGrp, &eof, -1);
    if (0 != ret) {
        LOG_M_E(TAG, "%s: AX_VDEC_SendStream(vdGrp %d EOF) fail, ret = 0x%x", __func__, m_vdGrp, ret);
        return AX_FALSE;
    }
#endif

    ret = AX_VDEC_StopRecvStream(m_vdGrp);
    if (0 != ret) {
        LOG_M_E(TAG, "%s: AX_VDEC_StopRecvStream(vdGrp %d) fail, ret = 0x%x", __func__, m_vdGrp, ret);
        return AX_FALSE;
    }

    m_bStarted = AX_FALSE;
    m_nLastSendCode = 0;

    if (!ResetGrp(NVR_VDEC_RESET_GRP_TRY_COUNT)) {
        return AX_FALSE;
    }

    {
        /* fixme: this may cause abondon some frames */
        std::lock_guard<std::mutex> lck(m_mtxObs);
        for (AX_VDEC_CHN vdChn = 0; vdChn < MAX_VDEC_CHN_NUM; ++vdChn) {
            if (m_lstObs[vdChn].size() > 0) {
                CDecodeTask::GetInstance()->UnRegisterObservers(m_vdGrp, vdChn, m_lstObs[vdChn]);
            }
        }
    }

#ifdef __DEBUG_STOP__
    LOG_M_C(TAG, "%s: vdGrp %d ---", __func__, m_vdGrp);
#else
    LOG_M_I(TAG, "%s: vdGrp %d ---", __func__, m_vdGrp);
#endif
    return AX_TRUE;
}

AX_BOOL CVDEC::ResetGrp(AX_S32 nTryCount /* = NVR_VDEC_RESET_GRP_TRY_COUNT*/) {
    AX_S32 ret = AX_SUCCESS;
    AX_S32 nIndex = 0;
    while (nIndex++ < nTryCount) {
        ret = AX_VDEC_ResetGrp(m_vdGrp);
        if (0 != ret) {
            if (AX_ERR_VDEC_BUSY == ret) {
                LOG_M_W(TAG, "[%d] vdGrp %d is busy, try again to reset", nIndex, m_vdGrp);
            } else {
                LOG_M_E(TAG, "[%d] Try AX_VDEC_ResetGrp(vdGrp %d) fail, ret = 0x%x", nIndex, m_vdGrp, ret);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            continue;
        } else {
            break;
        }
    }

    if (AX_SUCCESS != ret) {
        LOG_M_E(TAG, "AX_VDEC_ResetGrp(vdGrp %d) failed after try %d times, ret = 0x%x", m_vdGrp, nTryCount, ret);
        return AX_FALSE;
    }

    LOG_M_N(TAG, "vdGrp %d is reset", m_vdGrp);

    return AX_TRUE;
}

AX_BOOL CVDEC::SendStream(CONST AX_U8* pStream, AX_U32 nLen, AX_U64 u64PTS, AX_U64 nPrivData /* = 0 */,
                          AX_BOOL bSkipDisplay /* = AX_FALSE */, AX_U64 u64UserData, AX_S32 nTimeOut /* = INFINITE */) {
    if (!m_bStarted) {
        // LOG_M_E(TAG, "%s: vdGrp %d is stopped", __func__, m_vdGrp);
        return AX_FALSE;
    }

    AX_VDEC_STREAM_T stStream;
    memset(&stStream, 0, sizeof(stStream));
    stStream.u64PTS = u64PTS;
    stStream.pu8Addr = const_cast<AX_U8*>(pStream);
    stStream.u32StreamPackLen = nLen;
    stStream.u64PrivateData = nPrivData;
    stStream.u64UserData = u64UserData;

    if (AX_VDEC_INPUT_MODE_FRAME == m_stAttr.enInputMode) {
        stStream.bEndOfFrame = AX_TRUE;
    }

    if (AX_ERR_VDEC_INVALID_GRPID == m_nLastSendCode || AX_ERR_VDEC_INVALID_CHNID == m_nLastSendCode ||
        AX_ERR_VDEC_NEED_REALLOC_BUF == m_nLastSendCode || AX_ERR_VDEC_UNKNOWN == m_nLastSendCode) {
        /* fatal error of VDEC to abandon to send */
        LOG_M_W(TAG, "last nalu send to vdGrp %d return 0x%x, abandon %d bytes, pts %lld", m_vdGrp, m_nLastSendCode, nLen, u64PTS);
        return AX_FALSE;
    }

    if (AX_VDEC_DISPLAY_MODE_PLAYBACK == m_stAttr.enDecodeMode) {
        AX_VDEC_GRP_STATUS_T status;
        while (m_bStarted) {
            if (0 != AX_VDEC_QueryStatus(m_vdGrp, &status)) {
                break;
            }

            if (status.u32LeftStreamFrames <= 30) {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (!m_bStarted) {
            return AX_FALSE;
        }
    }

    LOG_MM_D(TAG, "AX_VDEC_SendStream(vdGrp %d, len %d, pts %lld, timeout %d) +++", m_vdGrp, nLen, u64PTS, nTimeOut);
    m_nLastSendCode = AX_VDEC_SendStream(m_vdGrp, &stStream, nTimeOut);
    LOG_MM_D(TAG, "AX_VDEC_SendStream(vdGrp %d, len %d, pts %lld, timeout %d) ---", m_vdGrp, nLen, u64PTS, nTimeOut);
    if (0 != m_nLastSendCode) {
        if ((AX_ERR_VDEC_BUF_FULL == m_nLastSendCode) || (AX_ERR_VDEC_QUEUE_FULL == m_nLastSendCode)) {
            LOG_M_W(TAG, "vdGrp %d input buffer is full, abandon %d bytes, pts %lld", m_vdGrp, nLen, u64PTS);
        } else {
            LOG_M_E(TAG, "AX_VDEC_SendStream(vdGrp %d, len %d, pts %lld, timeout %d) fail, ret = 0x%x", m_vdGrp, nLen, u64PTS, nTimeOut,
                    m_nLastSendCode);
        }

        return AX_FALSE;
    }

#if TEST_LATENCY
    AX_U64 u64StartPts = 0;
    AX_U64 u64SeqNUm = 0;
    AX_SYS_GetCurPTS(&u64StartPts);
    u64SeqNUm = ((FRAME_INFO_T*)stStream.u64PrivateData)->u64SeqNum;
    ((FRAME_INFO_T*)stStream.u64PrivateData)->u64SendStreamPts = u64StartPts;
    LOG_M_E(TAG, "%s Group:%d, SendStream pts:%llu, SeqNum:%llu", __func__, m_vdGrp, u64StartPts, u64SeqNUm);
#endif
    return AX_TRUE;
}

AX_BOOL CVDEC::OnRecvStreamData(CONST STREAM_FRAME_T& stFrame) {
#ifdef TEST_LATENCY
    static AX_U64 u64BeforeVdecRecvStreamLatency = 0;
    static AX_U64 u64BeforeVdecRecvStreamCount = 0;
    AX_U64 u64ChnStartCurPts = 0;
    AX_SYS_GetCurPTS(&u64ChnStartCurPts);
    if (u64BeforeVdecRecvStreamCount < 1200) {
        AX_U64 u64FramePts = 0;
        memcpy(&u64FramePts, (void*)stFrame.nPrivData, 8);
        u64BeforeVdecRecvStreamLatency += u64ChnStartCurPts - u64FramePts;
        u64BeforeVdecRecvStreamCount++;
    }
    if (1200 == u64BeforeVdecRecvStreamCount) {
        LOG_M_W(TAG, "===============before vdec send stream data done: avg latency: %llu us", u64BeforeVdecRecvStreamLatency / 1200);
        u64BeforeVdecRecvStreamLatency = 0;
        u64BeforeVdecRecvStreamCount = 0;
    }
#endif
    if (PT_H264 == stFrame.enPayload || PT_H265 == stFrame.enPayload) {
        if (!SendStream(stFrame.frame.stVideo.pData, stFrame.frame.stVideo.nLen, stFrame.frame.stVideo.nPTS, stFrame.nPrivData,
                        stFrame.frame.stVideo.bSkipDisplay, stFrame.frame.stVideo.u64UserData, m_stAttr.nTimeOut)) {
            return AX_FALSE;
        }
    }
#ifdef TEST_LATENCY
    static AX_U64 u64TestCount = 0;
    static AX_U64 U64GetFrameLatency = 0;
    static AX_U64 u64ChnEndCurPts = 0;
    static AX_U64 U64PtsLatency = 0;
    static AX_U64 u64VdecRecvStreamLatency = 0;
    static AX_U64 u64VdecRecvStreamCount = 0;

    AX_U64 U64LastChnEndCurpts = u64ChnEndCurPts;
    if (0 != AX_SYS_GetCurPTS(&u64ChnEndCurPts)) {
        LOG_M_W(TAG, "AX_SYS_GetCurPTS failed");
    }

    if (u64TestCount < 1200) {
        U64GetFrameLatency += u64ChnEndCurPts - u64ChnStartCurPts;
        u64TestCount++;
        U64PtsLatency += u64ChnEndCurPts - U64LastChnEndCurpts;

        AX_U64 u64FramePts = 0;
        memcpy(&u64FramePts, (void*)stFrame.nPrivData, 8);
        u64VdecRecvStreamLatency += u64ChnEndCurPts - u64FramePts;
        u64VdecRecvStreamCount++;
    }

    if (1200 == u64TestCount) {
        LOG_M_W(TAG, "===============SendStream: latency: %llu us", U64GetFrameLatency / 1200);
        LOG_M_W(TAG, "===============two times SendStream U64PtsLatency: %llu us", U64PtsLatency / 1200);
        LOG_M_W(TAG, "===============vdec send stream data done: avg latency: %llu us", u64VdecRecvStreamLatency / 1200);

        U64GetFrameLatency = 0;
        u64TestCount = 0;
        U64PtsLatency = 0;
        u64VdecRecvStreamLatency = 0;
        u64VdecRecvStreamCount = 0;
    }
#endif
    return AX_TRUE;
}

AX_BOOL CVDEC::OnRecvStreamInfo(CONST STREAM_INFO_T& /* stInfo */) {
    return AX_TRUE;
}

AX_VOID CVDEC::OnNotifyConnStatus(CONST AX_CHAR* /* pUrl */, CONNECT_STATUS_E /* enStatus */) {
}

AX_BOOL CVDEC::RegisterObserver(AX_S32 vdChn, IObserver* pObs) {
    if (!CHECK_VDEC_CHN(vdChn)) {
        LOG_M_E(TAG, "%s: invalid vdChn %d", __func__, vdChn);
        return AX_FALSE;
    }

    if (!pObs) {
        LOG_M_E(TAG, "%s: observer is nil", __func__);
        return AX_FALSE;
    }

    if (!m_stAttr.stChnAttr[vdChn].bEnable) {
        LOG_MM_E(TAG, "vdGrp %d vdChn %d is not enabled", m_vdGrp, vdChn);
        return AX_FALSE;
    }

    std::lock_guard<std::mutex> lck(m_mtxObs);

    auto it = std::find(m_lstObs[vdChn].begin(), m_lstObs[vdChn].end(), pObs);
    if (it != m_lstObs[vdChn].end()) {
        LOG_M_W(TAG, "%s: vdGrp %d vdChn %d observer %p already registed", __func__, m_vdGrp, vdChn, pObs);
    } else {
        if (m_bStarted) {
            CDecodeTask::GetInstance()->RegisterObserver(m_vdGrp, vdChn, pObs);
        }

        m_lstObs[vdChn].push_back(pObs);
        LOG_M_I(TAG, "%s: regist observer %p to vdGrp %d vdChn %d ok", __func__, pObs, m_vdGrp, vdChn);
    }

    return AX_TRUE;
}

AX_BOOL CVDEC::UnRegisterObserver(AX_S32 vdChn, IObserver* pObs) {
    if (!CHECK_VDEC_CHN(vdChn)) {
        LOG_M_E(TAG, "%s: invalid vdChn %d", __func__, vdChn);
        return AX_FALSE;
    }

    if (!pObs) {
        LOG_M_E(TAG, "%s: observer is nil", __func__);
        return AX_FALSE;
    }

    std::lock_guard<std::mutex> lck(m_mtxObs);

    auto it = std::find(m_lstObs[vdChn].begin(), m_lstObs[vdChn].end(), pObs);
    if (it != m_lstObs[vdChn].end()) {
        if (m_bStarted) {
            CDecodeTask::GetInstance()->UnRegisterObserver(m_vdGrp, vdChn, pObs);
        }

        m_lstObs[vdChn].remove(pObs);
        LOG_M_I(TAG, "%s: unregist observer %p from vdGrp %d vdChn %d ok", __func__, pObs, m_vdGrp, vdChn);
        return AX_TRUE;
    }

    LOG_M_E(TAG, "%s: observer %p is not registed to vdGrp %d vdChn %d", __func__, pObs, m_vdGrp, vdChn);
    return AX_FALSE;
}

AX_BOOL CVDEC::AttachPool(AX_S32 vdChn, AX_POOL pool) {
    if (AX_INVALID_POOLID == pool) {
        LOG_M_E(TAG, "%s: invalid pool id %d", pool);
        return AX_FALSE;
    }

    if (!CHECK_VDEC_CHN(vdChn)) {
        LOG_M_E(TAG, "%s: invalid vdChn %d", __func__, vdChn);
        return AX_FALSE;
    }

    if (m_stAttr.bPrivatePool) {
        LOG_M_E(TAG, "%s: vdGrp %d user pool is not allowed to attach for private pool mode", m_vdGrp);
        return AX_FALSE;
    }

    AX_S32 ret = AX_VDEC_AttachPool(m_vdGrp, vdChn, pool);
    if (0 != ret) {
        LOG_M_E(TAG, "AX_VDEC_AttachPool(vdGrp %d, vdChn %d) pool %d fail, ret = 0x%x", m_vdGrp, vdChn, pool, ret);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CVDEC::DetachPool(AX_S32 vdChn) {
    if (!CHECK_VDEC_CHN(vdChn)) {
        LOG_M_E(TAG, "%s: invalid vdChn %d", __func__, vdChn);
        return AX_FALSE;
    }

    AX_S32 ret = AX_VDEC_DetachPool(m_vdGrp, vdChn);
    if (0 != ret) {
        LOG_M_E(TAG, "AX_VDEC_DetachPool(vdGrp %d, vdChn %d) fail, ret = 0x%x", m_vdGrp, vdChn, ret);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CVDEC::GetChnAttr(AX_S32 vdChn, AX_VDEC_CHN_ATTR_T& stChnAttr) {
    if (!CHECK_VDEC_CHN(vdChn)) {
        LOG_M_E(TAG, "%s: invalid vdChn %d", __func__, vdChn);
        return AX_FALSE;
    }

    AX_S32 ret = AX_VDEC_GetChnAttr(m_vdGrp, vdChn, &stChnAttr);
    if (0 != ret) {
        LOG_M_E(TAG, "AX_VDEC_GetChnAttr(vdGrp %d, vdChn %d) fail, ret = 0x%x", m_vdGrp, vdChn, ret);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CVDEC::SetChnAttr(AX_S32 vdChn, CONST AX_VDEC_CHN_ATTR_T& stChnAttr) {
    if (!CHECK_VDEC_CHN(vdChn)) {
        LOG_M_E(TAG, "%s: invalid vdChn %d", __func__, vdChn);
        return AX_FALSE;
    }

    AX_S32 ret = AX_VDEC_SetChnAttr(m_vdGrp, vdChn, &stChnAttr);
    if (0 != ret) {
        LOG_M_E(TAG, "AX_VDEC_SetChnAttr(vdGrp %d, vdChn %d) fail, ret = 0x%x", m_vdGrp, vdChn, ret);
        return AX_FALSE;
    } else {
        LOG_M_I(TAG, "AX_VDEC_SetChnAttr(vdGrp %d, vdChn %d, %dx%d) OK", m_vdGrp, vdChn, stChnAttr.u32PicWidth, stChnAttr.u32PicHeight);
    }

    m_stAttr.stChnAttr[vdChn].stAttr = stChnAttr;
    return AX_TRUE;
}

AX_VOID CDecodeTask::DecodingThread(AX_VOID* /* pArg */) {
    LOG_M_I(TAG, "%s: +++", __func__);

    AX_S32 ret;
    AX_VDEC_GRP_SET_INFO_T stGrpSet;
    CONSTEXPR AX_S32 TIMEOUT = -1;
    while (1) {
#ifdef TEST_LATENCY
        static AX_U64 u64CodeLatency = 0;
        static AX_U64 u64Count = 0;
        AX_U64 U64StartPts = 0;
        AX_SYS_GetCurPTS(&U64StartPts);
#endif
        WaitTask();

        if (!m_thread.IsRunning()) {
            break;
        }

        LOG_M_D(TAG, "AX_VDEC_SelectGrp() +++");
#ifdef TEST_LATENCY
        AX_U64 u64ChnStartCurPts = 0;
        if (0 != AX_SYS_GetCurPTS(&u64ChnStartCurPts)) {
            LOG_M_W(TAG, "===============AX_SYS_GetCurPTS failed");
        }
#endif
        ret = AX_VDEC_SelectGrp(&stGrpSet, 1000);
#ifdef TEST_LATENCY
        static AX_U64 u64TestCount = 0;
        static AX_U64 U64GetFrameLatency = 0;
        AX_U64 u64ChnEndCurPts = 0;
        if (0 != AX_SYS_GetCurPTS(&u64ChnEndCurPts)) {
            LOG_M_W(TAG, "===============AX_SYS_GetCurPTS failed");
        }

        if (u64TestCount < 1200) {
            U64GetFrameLatency += u64ChnEndCurPts - u64ChnStartCurPts;
            u64TestCount++;
        }

        if (1200 == u64TestCount) {
            LOG_M_W(TAG, "===============AX_VDEC_SelectGrp: latency: %llu us", U64GetFrameLatency / 1200);
            U64GetFrameLatency = 0;
            u64TestCount = 0;
        }
#endif
        LOG_M_D(TAG, "AX_VDEC_SelectGrp() ---");
        if (0 != ret) {
            if (AX_ERR_VDEC_FLOW_END == ret) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            } else {
                if (AX_ERR_VDEC_TIMED_OUT != ret) {
                    LOG_M_E(TAG, "AX_VDEC_SelectGrp() fail, ret = 0x%x", ret);
                } else {
                    LOG_M_I(TAG, "AX_VDEC_SelectGrp() timeout");
                }
            }
            continue;
        }

        if (0 == stGrpSet.u32GrpCount) {
            THROW_AX_EXCEPTION("AX_VDEC_SelectGrp() success, but stGrpSet.u32GrpCount returned is %d", 0);
        }

        for (AX_U32 i = 0; i < stGrpSet.u32GrpCount; ++i) {
            for (AX_U32 j = 0; j < stGrpSet.stChnSet[i].u32ChnCount; ++j) {
                AX_VDEC_GRP vdGrp = stGrpSet.stChnSet[i].VdGrp;
                AX_VDEC_CHN vdChn = stGrpSet.stChnSet[i].VdChn[j];
                if (!CHECK_VDEC_GRP(vdGrp) || !CHECK_VDEC_CHN(vdChn)) {
                    THROW_AX_EXCEPTION("AX_VDEC_SelectGrp() success, but return invalid vdGrp %d or vdChn %d", vdGrp, vdChn);
                }

                AX_VIDEO_FRAME_INFO_T stVFrame;
#ifdef TEST_LATENCY
                AX_U64 u64TestCurPts = 0;
                AX_SYS_GetCurPTS(&u64TestCurPts);
                if (u64Count < 1200) {
                    u64CodeLatency += u64TestCurPts - U64StartPts;
                    u64Count++;
                }

                if (1200 == u64Count) {
                    LOG_M_W(TAG, "===============Before AX_VDEC_GetChnFrame: CODE latency: %llu us", u64CodeLatency / 1200);
                    u64CodeLatency = 0;
                    u64Count = 0;
                }
#endif
                ret = AX_VDEC_GetChnFrame(vdGrp, vdChn, &stVFrame, TIMEOUT);
                if (0 != ret) {
                    if (AX_ERR_VDEC_FLOW_END == ret) {
                        LOG_M_I(TAG, "%s: vdGrp %d vdChn %d received flow end", __func__, vdGrp, vdChn);
                        RemoveObservers(vdGrp, vdChn);
                    } else if (AX_ERR_VDEC_STRM_ERROR == ret) {
                        LOG_M_W(TAG, "AX_VDEC_GetChnFrame(vdGrp %d, vdChn %d): stream is undecodeable", vdGrp, vdChn);
                    } else if (AX_ERR_VDEC_UNEXIST == ret) {
                        LOG_MM_D(TAG, "vdGrp %d vdChn %d maybe under reseting", vdGrp, vdChn);
                    } else {
                        LOG_M_E(TAG, "AX_VDEC_GetChnFrame(vdGrp %d, vdChn %d, timeout %d) fail, ret = 0x%x", vdGrp, vdChn, TIMEOUT, ret);
                    }
                    continue;
                }
#ifdef TEST_LATENCY
                AX_U64 u64EndPts = 0;
                AX_U64 u64SeqNUm = 0;
                AX_SYS_GetCurPTS(&u64EndPts);
                AX_U64 diff = u64EndPts - ((FRAME_INFO_T*)stVFrame.stVFrame.u64PrivateData)->u64SendStreamPts;
                u64SeqNUm = ((FRAME_INFO_T*)stVFrame.stVFrame.u64PrivateData)->u64SeqNum;
                if (diff > 8000) {
                    LOG_M_E(TAG, "%s Group%d, VDEC FRAME GET DIFF IS GREATE THAN 8000 us, diff is %llu", __func__, vdGrp, diff);
                    LOG_M_E(TAG, "%s Group:%d, GetChnFrame pts:%llu, SeqNum:%llu", __func__, vdGrp, u64EndPts, u64SeqNUm);
                }

                static AX_U64 u64VdecGetFrameLatency = 0;
                static AX_U64 u64VdecGetFrameCount = 0;
                AX_U64 u64CurPts = 0;
                AX_SYS_GetCurPTS(&u64CurPts);
                if (u64VdecGetFrameCount < 1200) {
                    AX_U64 u64FramePts = 0;
                    memcpy(&u64FramePts, (void*)(stVFrame.stVFrame.u64PrivateData), 8);
                    u64VdecGetFrameLatency += u64CurPts - u64FramePts;
                    u64VdecGetFrameCount++;
                }
                if (1200 == u64VdecGetFrameCount) {
                    LOG_M_W(TAG, "===============AX_VDEC_GetChnFrame done: avg latency: %llu us", u64VdecGetFrameLatency / 1200);
                    u64VdecGetFrameLatency = 0;
                    u64VdecGetFrameCount = 0;
                }
#endif
                /* SDK only return 0, needs to release */
                if (AX_INVALID_BLOCKID == stVFrame.stVFrame.u32BlkId[0]) {
                    THROW_AX_EXCEPTION("AX_VDEC_GetChnFrame(vdGrp %d, vdChn %d) recv invalid frame blk id", vdGrp, vdChn);
                }

                if (0 == stVFrame.stVFrame.u32Width || 0 == stVFrame.stVFrame.u32Height) {
                    LOG_M_W(TAG, "AX_VDEC_GetChnFrame(vdGrp %d, vdChn %d) recv invalid frame %dx%d, pxl fmt %d, blk 0x%x", vdGrp, vdChn,
                            stVFrame.stVFrame.u32Width, stVFrame.stVFrame.u32Height, stVFrame.stVFrame.enImgFormat,
                            stVFrame.stVFrame.u32BlkId[0]);
                    /* if valid blk id, should release */
                    ret = AX_VDEC_ReleaseChnFrame(vdGrp, vdChn, &stVFrame);
                    if (0 != ret) {
                        LOG_M_E(TAG, "AX_VDEC_ReleaseChnFrame(vdGrp %d, vdChn %d, blk 0x%x) fail, ret = 0x%x", vdGrp, vdChn,
                                stVFrame.stVFrame.u32BlkId[0], ret);
                        return;
                    }

                    continue;
                }

                LOG_M_N(TAG, "decoded vdGrp %d vdChn %d frame %lld pts %lld phy 0x%llx %dx%d stride %d blkId 0x%x", vdGrp, vdChn,
                        stVFrame.stVFrame.u64SeqNum, stVFrame.stVFrame.u64PTS, stVFrame.stVFrame.u64PhyAddr[0], stVFrame.stVFrame.u32Width,
                        stVFrame.stVFrame.u32Height, stVFrame.stVFrame.u32PicStride[0], stVFrame.stVFrame.u32BlkId[0]);

                OnRecvFrame(vdGrp, vdChn, stVFrame);

                ret = AX_VDEC_ReleaseChnFrame(vdGrp, vdChn, &stVFrame);
                if (0 != ret) {
                    LOG_M_E(TAG, "AX_VDEC_ReleaseChnFrame(vdGrp %d, vdChn %d, blk 0x%x) fail, ret = 0x%x", vdGrp, vdChn,
                            stVFrame.stVFrame.u32BlkId[0], ret);
                    return;
                }
            }
        }
    } /* end while (1) */

    LOG_M_I(TAG, "%s: ---", __func__);
}

AX_BOOL CDecodeTask::Start(AX_VOID) {
    if (m_bStarted) {
        LOG_M_W(TAG, "decoding thread is already started");
        return AX_TRUE;
    }

    LOG_M_I(TAG, "start decoding thread +++");

    if (!m_thread.Start([this](AX_VOID* pArg) -> AX_VOID { DecodingThread(pArg); }, nullptr, "AppDecode", SCHED_FIFO, 99)) {
        LOG_M_E(TAG, "start decoding thread fail");
        return AX_FALSE;
    }

    m_bStarted = AX_TRUE;

    LOG_M_I(TAG, "start decoding thread ---");
    return AX_TRUE;
}

AX_BOOL CDecodeTask::Stop(AX_VOID) {
    if (!m_bStarted) {
        LOG_M_W(TAG, "decoding thread is not started yet");
        return AX_TRUE;
    }

#ifdef __DEBUG_STOP__
    LOG_M_C(TAG, "stop decoding thread +++");
#else
    LOG_M_I(TAG, "stop decoding thread +++");
#endif

    m_thread.Stop();
    Wakeup();
    m_thread.Join();
    m_bStarted = AX_FALSE;

#ifdef __DEBUG_STOP__
    LOG_M_C(TAG, "stop decoding thread ---");
#else
    LOG_M_I(TAG, "stop decoding thread ---");
#endif
    return AX_TRUE;
}

AX_VOID CDecodeTask::RegisterObserver(AX_VDEC_GRP vdGrp, AX_VDEC_CHN vdChn, IObserver* pObs) {
    {
        std::lock_guard<std::mutex> lck(m_mtxObs);

        auto key = std::make_pair(vdGrp, vdChn);
        auto range = m_mapObs.equal_range(key);
        for (auto it = range.first; it != range.second; ++it) {
            if (pObs == it->second) {
                LOG_M_W(TAG, "observer %p is already registed to vdGrp %d vdChn %d", pObs, vdGrp, vdChn);
                return;
            }
        }

        m_mapObs.emplace(key, pObs);
        LOG_M_I(TAG, "observer %p is registed to vdGrp %d vdChn %d", pObs, vdGrp, vdChn);
    }

    Wakeup();
}

AX_VOID CDecodeTask::UnRegisterObserver(AX_VDEC_GRP vdGrp, AX_VDEC_CHN vdChn, IObserver* pObs) {
    std::lock_guard<std::mutex> lck(m_mtxObs);

    auto range = m_mapObs.equal_range(std::make_pair(vdGrp, vdChn));
    for (auto it = range.first; it != range.second; ++it) {
        if (pObs == it->second) {
            m_mapObs.erase(it);
            LOG_M_I(TAG, "observer %p is unregisted from vdGrp %d vdChn", pObs, vdGrp, vdChn);
            break;
        }
    }
}

AX_VOID CDecodeTask::RegisterObservers(AX_VDEC_GRP vdGrp, AX_VDEC_CHN vdChn, CONST std::list<IObserver*>& lstObs) {
    AX_BOOL bRegisted = {AX_FALSE};

    {
        std::lock_guard<std::mutex> lck(m_mtxObs);

        auto key = std::make_pair(vdGrp, vdChn);
        for (auto&& m : lstObs) {
            if (!m) {
                continue;
            }

            AX_BOOL bFound = {AX_FALSE};
            auto range = m_mapObs.equal_range(key);
            for (auto it = range.first; it != range.second; ++it) {
                if (m == it->second) {
                    bFound = AX_TRUE;
                    LOG_M_W(TAG, "observer %p is already registed to vdGrp %d vdChn %d", m, vdGrp, vdChn);
                    break;
                }
            }

            if (!bFound) {
                m_mapObs.emplace(key, m);
                bRegisted = AX_TRUE;
                LOG_M_I(TAG, "observer %p is registed to vdGrp %d vdChn %d", m, vdGrp, vdChn);
            }
        }
    }

    if (bRegisted) {
        Wakeup();
    }
}

AX_VOID CDecodeTask::UnRegisterObservers(AX_VDEC_GRP vdGrp, AX_VDEC_CHN vdChn, CONST std::list<IObserver*>& lstObs) {
    std::lock_guard<std::mutex> lck(m_mtxObs);

    for (auto&& m : lstObs) {
        if (!m) {
            continue;
        }

        auto range = m_mapObs.equal_range(std::make_pair(vdGrp, vdChn));
        for (auto it = range.first; it != range.second; ++it) {
            if (m == it->second) {
                m_mapObs.erase(it);
                LOG_M_I(TAG, "observer %p is unregisted from vdGrp %d vdChn", m, vdGrp, vdChn);
                break;
            }
        }
    }
}

AX_VOID CDecodeTask::RemoveObservers(AX_VDEC_GRP vdGrp, AX_VDEC_CHN vdChn) {
    /* remove all observers registed to {vdGrp vdChn} */
    std::lock_guard<std::mutex> lck(m_mtxObs);
    m_mapObs.erase({vdGrp, vdChn});
}

AX_VOID CDecodeTask::WaitTask(AX_VOID) {
    std::unique_lock<std::mutex> lck(m_mtxObs);
    while (0 == m_mapObs.size() && m_thread.IsRunning()) {
        m_cv.wait(lck);
    }
}

AX_VOID CDecodeTask::OnRecvFrame(AX_VDEC_GRP vdGrp, AX_VDEC_CHN vdChn, CONST AX_VIDEO_FRAME_INFO_T& stVFrame) {
    CAXFrame axFrame;
    axFrame.nGrp = vdGrp;
    axFrame.nChn = vdChn;
    axFrame.stFrame.stVFrame = stVFrame;

    std::lock_guard<std::mutex> lck(m_mtxObs);

    auto range = m_mapObs.equal_range(std::make_pair(vdGrp, vdChn));
    for (auto it = range.first; it != range.second; ++it) {
        (AX_VOID) it->second->OnRecvData(E_OBS_TARGET_TYPE_VDEC, axFrame.nGrp, axFrame.nChn, &axFrame);
    }
}
