/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "streamTransfer.hpp"
#include <algorithm>
#include "AppLogApi.h"

#define TAG "STREAM_TRANS"

AX_BOOL CVideoStreamTransfer::Init(AX_S32 nStream, AX_U32 nFps, AX_U32 nGop) {
    m_pStreamContainer = std::make_unique<CVideoStreamContainer>(nGop, 3);
    if (!m_pStreamContainer) {
        return AX_FALSE;
    }

    m_nStreamID = nStream;
    m_nFps = nFps;
    m_nGop = nGop;

    return AX_TRUE;
}

AX_BOOL CVideoStreamTransfer::DeInit() {
    return AX_TRUE;
}

AX_BOOL CVideoStreamTransfer::SendStream(AX_S32 nStream, CONST AX_VENC_PACK_T& tVencPacked, AX_BOOL bGopStart /*= AX_TRUE*/, AX_S32 nTimeOut /*= INFINITE*/) {
    if (!tVencPacked.pu8Addr || tVencPacked.u32Len == 0) {
        LOG_MM_W(TAG, "Stream data is invalid.");
        return AX_FALSE;
    }

    if (nStream != m_nStreamID) {
        LOG_MM_W(TAG, "Stream ID is not matched(%d != %d).", nStream, m_nStreamID);
        return AX_FALSE;
    }

    if (!m_pStreamContainer) {
        LOG_MM_W(TAG, "Stream container is not prepared.");
        return AX_FALSE;
    }

    if (tVencPacked.u64UserData >= m_nGop) {
        LOG_MM_E(TAG, "Frame GOP index(%d) is larger than configured GOP value(%d), ignore this frame.", tVencPacked.u64UserData, m_nGop);
        return AX_FALSE;
    }

    if (bGopStart) {
        AX_U32 nAdjustPTSDiff = 0;
        if (m_nLastGopIndex != -1 && (AX_U32)m_nLastGopIndex < m_nGop - 1) {
            AX_U32 nStep = 1000000 / m_nFps;
            nAdjustPTSDiff = nStep * (m_nGop - m_nLastGopIndex - 1);
            m_pStreamContainer->AdjustPTS(nAdjustPTSDiff);
        }
        m_pStreamContainer->Flush();
    }

    m_nLastGopIndex = tVencPacked.u64UserData;

    if (!m_pStreamContainer->Push(tVencPacked, nTimeOut)) {
        LOG_MM_E(TAG, "Fill stream data(size: %d) to container failed", tVencPacked.u32Len);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CVideoStreamTransfer::OnRecvStreamPack(AX_S32 nStream, CONST AX_VENC_PACK_T& stPack, AX_BOOL bGopStart /*= AX_TRUE*/) {
    return SendStream(nStream, stPack, bGopStart, -1);
}

AX_BOOL CVideoStreamTransfer::RegisterObserver(IStreamObserver* pObs) {
    if (!pObs) {
        LOG_MM_E(TAG, "observer is nil");
        return AX_FALSE;
    }

    std::lock_guard<std::mutex> lck(m_mtxObs);

    auto it = std::find(m_lstObs.begin(), m_lstObs.end(), pObs);
    if (it != m_lstObs.end()) {
        LOG_MM_W(TAG, "Stream %d's observer %p already registed", m_nStreamID, pObs);
    } else {
        m_lstObs.push_back(pObs);
        LOG_MM_I(TAG, "Regist observer %p to stream %d ok", pObs, m_nStreamID);
    }

    return AX_TRUE;
}

AX_BOOL CVideoStreamTransfer::UnRegisterObserver(IStreamObserver* pObs) {
    if (!pObs) {
        LOG_MM_E(TAG, "observer is nil");
        return AX_FALSE;
    }

    std::lock_guard<std::mutex> lck(m_mtxObs);

    auto it = std::find(m_lstObs.begin(), m_lstObs.end(), pObs);
    if (it != m_lstObs.end()) {
        m_lstObs.remove(pObs);
        LOG_MM_I(TAG, "unregist observer %p from stream %d ok", pObs, m_nStreamID);
        return AX_TRUE;
    }

    LOG_MM_E(TAG, "observer %p is not registed to stream %d", pObs, m_nStreamID);

    return AX_FALSE;
}

AX_BOOL CVideoStreamTransfer::Start() {
    if (!m_threadDispatch.Start([this](AX_VOID* pArg) -> AX_VOID { DispatchThread(pArg); }, nullptr, "StreamDispatch")) {
        LOG_MM_E(TAG, "Create test suite thread fail.");
    }

    return AX_TRUE;
}

AX_BOOL CVideoStreamTransfer::Stop() {
    if (m_threadDispatch.IsRunning()) {
        m_threadDispatch.Stop();
        m_pStreamContainer->Clear();
        m_threadDispatch.Join();
    }

    m_lstObs.clear();

    return AX_TRUE;
}

AX_VOID CVideoStreamTransfer::DispatchThread(AX_VOID*) {
    AX_S32 ret = 0;
    while (m_threadDispatch.IsRunning()) {
        VIDEO_STREAM_INFO_T tVencStream;
        if (!m_pStreamContainer->Pop(tVencStream) || 0 == tVencStream.u32Len) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        if (tVencStream.pu8Addr && tVencStream.u32Len > 0) {
            LOG_MM_D(TAG, "[%d] StreamContainer output (Seq:%lld, Size:%d, PTS:%lld)", m_nStreamID, tVencStream.u64SeqNum, tVencStream.u32Len, tVencStream.u64PTS);
            NotifyAll(&tVencStream);
        } else {
            LOG_MM_E(TAG, "[%d] StreamContainer output data error, addr %p, size %d", m_nStreamID, tVencStream.pu8Addr, tVencStream.u32Len);
        }

        tVencStream.FreeMem();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

AX_BOOL CVideoStreamTransfer::NotifyAll(VIDEO_STREAM_INFO_T* pStream) {
    if (nullptr == pStream) {
        return AX_TRUE;
    }

    AX_BOOL bRet = AX_TRUE;

    std::lock_guard<std::mutex> lock(m_mtxObs);
    for (std::list<IStreamObserver*>::iterator it = m_lstObs.begin(); it != m_lstObs.end(); it++) {
        STREAM_FRAME_T tFrame;
        memset(&tFrame, 0, sizeof(STREAM_FRAME_T));
        tFrame.enPayload = pStream->enType;
        tFrame.nPrivData = 0;
        tFrame.frame.stVideo.nPTS = pStream->u64PTS;
        tFrame.frame.stVideo.enNalu = NALU_TYPE_IDR; /* TODO: Need to get from AX_VENC_PACK_T */
        tFrame.frame.stVideo.pData = (AX_U8*)pStream->pu8Addr;
        tFrame.frame.stVideo.nLen = pStream->u32Len;

        if (!(*it)->OnRecvStreamData(tFrame)) {
            bRet = AX_FALSE;
        }
    }

    return bRet;
}