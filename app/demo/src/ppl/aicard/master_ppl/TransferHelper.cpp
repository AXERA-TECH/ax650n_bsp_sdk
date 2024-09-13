/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "TransferHelper.hpp"
#include <string.h>
#include "AppLog.hpp"
#include "ElapsedTimer.hpp"
#include "DetectResult.hpp"
#include "PrintHelper.hpp"

#define COMMAND_PORT_NUMBER (1) // one shared port id(usually 0) for transfer command for each device

#define TAG "TRANSFER"

using namespace std;

AX_VOID CTransferHelper::SendStreamThread(AX_VOID* pArg) {
    std::tuple<AX_U32, AX_U32>* pParams = (std::tuple<AX_U32, AX_U32>*)pArg;
    AX_U32 nIndex = std::get<0>(*pParams);
    AX_U32 nUniChannel = std::get<1>(*pParams);

    LOG_MM_I(TAG, "[%d][%d] +++", nIndex, nUniChannel);

    TRANSFER_SEND_DATA_T& tSendData = m_vecSendStream[nIndex];
    CAXThread* pThread = m_vecThreadSendStream[nIndex];
    while (pThread->IsRunning()) {
        CAXRingElement* pData = tSendData.pRingBuffer->Get();
        if (!pData) {
            CElapsedTimer::GetInstance()->mSleep(1);
            continue;
        }

        // if (!m_bAiSwitching) { /* Still send to slave side even if AI switching is progressing */
            if (!CPcieAdapter::GetInstance()->SendStream(nUniChannel, PCIE_CMD_H264_DATA_E, pData->pBuf, pData->nSize, m_tIniAttr.nSendTimeout)) {
                LOG_M_E(TAG, "Send stream failed.");
            } else {
                // CPrintHelper::GetInstance()->Add(E_PH_MOD_TRANS_SEND, nChannel);
            }
        // }

        tSendData.pRingBuffer->Pop();
    }

    LOG_MM_I(TAG, "[%d][%d] ---", nIndex, nUniChannel);
}

AX_BOOL CTransferHelper::SendAiAttr(AI_CARD_AI_SWITCH_ATTR_T& tAiAttr) {
    if (m_bAiSwitching) {
        LOG_MM_W(TAG, "Ai switching is in progress.");
        return AX_FALSE;
    }

    if (CPcieAdapter::GetInstance()->SendCtrlCommand(tAiAttr, m_tIniAttr.nSendTimeout)) {
        m_bAiSwitching = AX_TRUE;
        CDetectResult::GetInstance()->Clear();
    }

    return AX_TRUE;
}

AX_BOOL CTransferHelper::HandShake(AX_VOID) {
    if (CPcieAdapter::GetInstance()->SendHandShakePacket()) {
        AX_S32 nFeedback = CPcieAdapter::GetInstance()->RecvHandShakeFeedback();
        if (1 == nFeedback) {
            return AX_TRUE;
        }
    }

    return AX_FALSE;
}

AX_VOID CTransferHelper::RecvDetectResultThread(AX_VOID* pArg) {
    std::tuple<AX_U32, AX_U32>* pParams = (std::tuple<AX_U32, AX_U32>*)pArg;
    AX_U32 nIndex = std::get<0>(*pParams);
    AX_U32 nUniChannel = std::get<1>(*pParams);

    LOG_MM_I(TAG, "[%d][%d] +++", nIndex, nUniChannel);

    while (m_vecThreadRecvDetRet[nIndex]->IsRunning()) {
        if (m_bAiSwitching) {
            CElapsedTimer::GetInstance()->mSleep(1);
            continue;
        }

        DETECT_RESULT_T* pResult = CPcieAdapter::GetInstance()->RecvDetectorResult(nUniChannel, m_tIniAttr.nRecvTimeout);
        if (nullptr == pResult) {
            CElapsedTimer::GetInstance()->mSleep(1);
            continue;
        }

        AX_U32 nVideoIndex = nUniChannel - COMMAND_PORT_NUMBER * m_tIniAttr.nSlaveCount - m_tIniAttr.nMaxVideoCount;
        CDetectResult::GetInstance()->Set(nVideoIndex, *pResult);

        CElapsedTimer::GetInstance()->mSleep(1);
    }

    LOG_MM_I(TAG, "[%d][%d] ---", nIndex, nUniChannel);
}

AX_VOID CTransferHelper::RecvCtrlCmdResultThread(AX_VOID* pArg) {
    LOG_MM_I(TAG, "+++");

    while (m_threadRecvCmdRet.IsRunning()) {
        if (!m_bAiSwitching) {
            CElapsedTimer::GetInstance()->mSleep(1);
            continue;
        }

        PCIE_DATA_T tRecvData;
        memset(&tRecvData, 0, sizeof(PCIE_DATA_T));
        tRecvData.tBody.tCtrl.nCmdType = PCIE_CMD_SWITCH_AI_E;
        tRecvData.tBody.tCtrl.nChannel = PCIE_COMMON_MSG_CHANNEL;

        AX_S32 nRet = CPcieAdapter::GetInstance()->RecvCtrlCmdResult(PCIE_CMD_SWITCH_AI_E, &tRecvData, m_tIniAttr.nRecvTimeout);
        if (-1 == nRet) {
            CElapsedTimer::GetInstance()->mSleep(1);
            continue;
        }

        m_bAiSwitching = AX_FALSE;
    }

    LOG_MM_I(TAG, "---");
}

AX_BOOL CTransferHelper::Init(const TRANSFER_ATTR_T& stAttr) {
    LOG_MM_I(TAG, "+++");

    /* Send and Recv uses different channel id */
    m_tIniAttr = stAttr;
    AX_U32 nVideoCount = stAttr.nMaxVideoCount;
    if (!CPcieAdapter::GetInstance()->Init(stAttr.nSlaveCount, nVideoCount, stAttr.nBuffSize * 1024, stAttr.nTraceData, stAttr.nRetryCount)) {
        return AX_FALSE;
    }

    m_vecSendStream.resize(nVideoCount);
    for (AX_U8 i = 0; i < nVideoCount; i++) {
        TRANSFER_SEND_DATA_T& tData = m_vecSendStream[i];
        tData.pRingBuffer = new CAXRingBuffer(stAttr.nBuffSize * 1024, stAttr.nBuffCount);
        tData.nChannel = i + COMMAND_PORT_NUMBER * stAttr.nSlaveCount;
    }

    for (AX_U8 i = 0; i < nVideoCount; i++) {
        m_vecThreadSendStream.push_back(new CAXThread());
        m_vecThreadRecvDetRet.push_back(new CAXThread());
        m_mapSendThreadParams[i] = make_tuple(i, i + COMMAND_PORT_NUMBER * stAttr.nSlaveCount);
        m_mapRecvThreadParams[i] = make_tuple(i, i + nVideoCount + COMMAND_PORT_NUMBER * stAttr.nSlaveCount);
    }

    if (!HandShake()) {
        return AX_FALSE;
    }

    LOG_MM_I(TAG, "---");
    return AX_TRUE;
}

AX_BOOL CTransferHelper::DeInit(AX_VOID) {
    LOG_MM_I(TAG, "+++");

    for (auto& m : m_vecThreadSendStream) {
        if (m->IsRunning()) {
            LOG_MM_E(TAG, "Send stream thread is still running");
            return AX_FALSE;
        }
        delete(m);
        m = nullptr;
    }

    for (auto& m : m_vecThreadRecvDetRet) {
        if (m->IsRunning()) {
            LOG_MM_E(TAG, "Detector result recv thread is still running");
            return AX_FALSE;
        }
        delete(m);
        m = nullptr;
    }

    if (m_threadRecvCmdRet.IsRunning()) {
        LOG_MM_E(TAG, "Control command result recv thread is still running");
        return AX_FALSE;
    }

    LOG_MM_I(TAG, "---");
    return AX_TRUE;
}

AX_BOOL CTransferHelper::Start(AX_VOID) {
    LOG_MM_I(TAG, "+++");

    for (AX_U8 i = 0; i < m_tIniAttr.nMaxVideoCount; i++) {
        if (!m_vecThreadSendStream[i]->Start([this](AX_VOID* pArg) -> AX_VOID { SendStreamThread(pArg); }, &m_mapSendThreadParams[i], "MstTransSend")) {
            LOG_MM_E(TAG, "[%d] Create stream send thread fail.", i);
            return AX_FALSE;
        }
    }

    for (AX_U8 i = 0; i < m_tIniAttr.nMaxVideoCount; i++) {
        if (!m_vecThreadRecvDetRet[i]->Start([this](AX_VOID* pArg) -> AX_VOID { RecvDetectResultThread(pArg); }, &m_mapRecvThreadParams[i], "MstTransRecv")) {
            LOG_MM_E(TAG, "[%d] Create detector result recv thread fail.", i);
            return AX_FALSE;
        }
    }

    if (!m_threadRecvCmdRet.Start([this](AX_VOID* pArg) -> AX_VOID { RecvCtrlCmdResultThread(pArg); }, nullptr, "MstTransRecvCmd")) {
        LOG_MM_E(TAG, "Create control command result recv thread fail.");
        return AX_FALSE;
    }

    LOG_MM_I(TAG, "---");
    return AX_TRUE;
}

AX_BOOL CTransferHelper::Stop(AX_VOID) {
    LOG_MM_I(TAG, "+++");

    for (auto& m : m_vecThreadRecvDetRet) {
        m->Stop();
    }

    m_threadRecvCmdRet.Stop();

    for (auto& m : m_vecThreadSendStream) {
        m->Stop();
    }

    CPcieAdapter::GetInstance()->DeInit();

    for (auto& m : m_vecThreadRecvDetRet) {
        m->Join();
    }

    m_threadRecvCmdRet.Join();

    for (auto& m : m_vecThreadSendStream) {
        m->Join();
    }

    ClearBuf();

    LOG_MM_I(TAG, "---");
    return AX_TRUE;
}

AX_BOOL CTransferHelper::OnRecvVideoData(AX_S32 nChannel, const AX_U8* pData, AX_U32 nLen, AX_U64 nPTS) {
    CAXRingElement ele((AX_U8*)pData, nLen, nPTS, AX_TRUE);

    if (!m_vecSendStream[nChannel].pRingBuffer->Put(ele)) {
        LOG_M_D(TAG, "[%d] Put video data(len=%d) into ringbuf full", nChannel, nLen);
        return AX_FALSE;
    } else {
        LOG_M_D(TAG, "[%d] Put video data into ringbuf, size=%d", nChannel, nLen);
    }

    return AX_TRUE;
}

AX_BOOL CTransferHelper::OnRecvAudioData(AX_S32 nChannel, const AX_U8* pData, AX_U32 nLen, AX_U64 nPTS) {
    return AX_TRUE;
}

AX_VOID CTransferHelper::ClearBuf(AX_VOID) {
    for (auto& m: m_vecSendStream) {
        m.pRingBuffer->Clear();
    }
}
