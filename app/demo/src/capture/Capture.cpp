/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "Capture.hpp"
#include "AppLog.hpp"
#include "ElapsedTimer.hpp"

#include "ax_venc_api.h"
#include <algorithm>

#define CAPTURE "capture"

namespace {
using AXLockGuard = std::unique_lock<std::mutex>;
constexpr AX_U8 kReceiveWaitTimeoutSeconds = 2;
constexpr AX_S8 JPEG_ENCODE_ONCE_NAME[] = "JENC_ONCE";

static AX_S32 MallocJpegOutBuffer(AX_JPEG_ENCODE_ONCE_PARAMS_T* pStJpegEncodeOnceParam, AX_U32 frameSize) {
    AX_U64 phyBuff = 0;
    AX_VOID* virBuff = NULL;
    AX_S32 s32Ret = AX_SUCCESS;

    s32Ret = AX_SYS_MemAlloc(&phyBuff, &virBuff, frameSize, 0, (AX_S8 *)JPEG_ENCODE_ONCE_NAME);
    if (s32Ret) {
        LOG_MM_E(CAPTURE, "alloc mem err, size(%d).\n", frameSize);
        return -1;
    }

    pStJpegEncodeOnceParam->u32Len = frameSize;
    pStJpegEncodeOnceParam->ulPhyAddr = phyBuff;
    pStJpegEncodeOnceParam->pu8Addr = (AX_U8 *)virBuff;

    return AX_SUCCESS;
}

}  // namespace


AX_BOOL CCapture::SendFrame(AX_U32 nGrp, CAXFrame* axFrame) {
    if (!m_bCapture || m_nCaptureGrp != nGrp) {
        axFrame->FreeMem();
        return AX_TRUE;
    }

    {
        AXLockGuard lck(m_mutexStat);
        m_bCapture = AX_FALSE;
        m_pAXFrame = axFrame;
    }

    AXLockGuard lck(m_mutexCapture);
    m_cvCapture.notify_one();
    return AX_TRUE;
}

AX_BOOL CCapture::CapturePicture(AX_U8 nGrp, AX_U8 nChn, AX_U32 nQpLevel, AX_VOID **ppCallbackData) {
    LOG_MM_I(CAPTURE, "+++");
    if (m_bCapture) {
        LOG_MM_W(CAPTURE, "[%d] Capture is running, please wait, and try again", nChn);
        return AX_FALSE;
    }

   if (m_pAXFrame) {
        LOG_M_W(CAPTURE, "[%d] Capture frame should be free", m_nCaptureChn);
        return AX_FALSE;
    }

    {
        AXLockGuard lck(m_mutexStat);
        m_bCapture = AX_TRUE;
        m_nCaptureChn = nChn;
        m_nCaptureGrp = nGrp;
        m_pAXFrame = nullptr;
    }

    AXLockGuard lck(m_mutexCapture);
    if (m_cvCapture.wait_for(lck, std::chrono::seconds(kReceiveWaitTimeoutSeconds)) == std::cv_status::timeout) {
        LOG_MM_E(CAPTURE, "[%d] Capture frame timeout group:%d", m_nCaptureChn, m_nCaptureGrp);
        ResetCaptureStatus();
        return AX_FALSE;
    }

    AX_JPEG_ENCODE_ONCE_PARAMS_T stJpegEncodeOnceParam;
    memset(&stJpegEncodeOnceParam, 0, sizeof(stJpegEncodeOnceParam));
    const AX_VIDEO_FRAME_INFO_T& stVFrame = m_pAXFrame->stFrame.stVFrame;
    for (AX_U8 i = 0; i < 3; ++i) {
        stJpegEncodeOnceParam.u64PhyAddr[i] = stVFrame.stVFrame.u64PhyAddr[i];
        stJpegEncodeOnceParam.u32PicStride[i] = stVFrame.stVFrame.u32PicStride[i];
        stJpegEncodeOnceParam.u32HeaderSize[i] = stVFrame.stVFrame.u32HeaderSize[i];
    }

    stJpegEncodeOnceParam.stJpegParam.u32Qfactor = nQpLevel;
    stJpegEncodeOnceParam.u32Width = stVFrame.stVFrame.u32Width;
    stJpegEncodeOnceParam.u32Height = stVFrame.stVFrame.u32Height;
    stJpegEncodeOnceParam.enImgFormat = stVFrame.stVFrame.enImgFormat;
    stJpegEncodeOnceParam.stCompressInfo = stVFrame.stVFrame.stCompressInfo;

    using AXSnapshotDataCallback = std::function<AX_VOID(AX_VOID* data, AX_U32 size)>;
    std::pair<AX_U32, AXSnapshotDataCallback>* pSnapshotData = (std::pair<AX_U32, AXSnapshotDataCallback>*)ppCallbackData;
    AX_S32 s32Ret = MallocJpegOutBuffer(&stJpegEncodeOnceParam, stVFrame.stVFrame.u32FrameSize);
    if (AX_SUCCESS == s32Ret) {
        if (AX_SUCCESS == AX_VENC_JpegEncodeOneFrame(&stJpegEncodeOnceParam)) {
            const AX_U32 uDataSize = stJpegEncodeOnceParam.u32Len;
            pSnapshotData->second(stJpegEncodeOnceParam.pu8Addr, uDataSize);
        }

        AX_SYS_MemFree(stJpegEncodeOnceParam.ulPhyAddr, stJpegEncodeOnceParam.pu8Addr);
    }

    ResetCaptureStatus();

    LOG_MM_I(CAPTURE, "---");

    return AX_TRUE;
}

AX_BOOL CCapture::Init(AX_VOID) {
    return AX_TRUE;
}

AX_BOOL CCapture::DeInit(AX_VOID) {
    LOG_MM_I(CAPTURE, "+++");

    ResetCaptureStatus();

    LOG_MM_I(CAPTURE, "---");
    return AX_TRUE;
}

AX_VOID CCapture::RegObserver(IObserver *pObserver) {
    if (nullptr == pObserver) {
        return;
    }

    OBS_TRANS_ATTR_T tTransAttr;
    if (pObserver->OnRegisterObserver(E_OBS_TARGET_TYPE_CAPTURE, 0, 0, &tTransAttr)) {
        m_vecObserver.emplace_back(pObserver);
    }
}

AX_VOID CCapture::UnregObserver(IObserver *pObserver) {
    if (nullptr == pObserver) {
        return;
    }

    auto pos = std::find(m_vecObserver.begin(), m_vecObserver.end(), pObserver);
    if (pos != m_vecObserver.end()) {
        m_vecObserver.erase(pos);
    }
}

AX_VOID CCapture::NotifyAll(AX_U32 nGrp, AX_U32 nType, AX_VOID *pStream) {
    if (nullptr == pStream) {
        return;
    }

    for (auto& item : m_vecObserver) {
        item->OnRecvData(E_OBS_TARGET_TYPE_CAPTURE, nGrp, nType, pStream);
    }
}

AX_VOID CCapture::ResetCaptureStatus(AX_VOID) {
    AXLockGuard lck(m_mutexStat);
    m_bCapture = AX_FALSE;
    m_nCaptureChn = 0;
    m_nCaptureGrp = 0;
    if (m_pAXFrame) {
        m_pAXFrame->FreeMem();
        m_pAXFrame = nullptr;
    }
}
