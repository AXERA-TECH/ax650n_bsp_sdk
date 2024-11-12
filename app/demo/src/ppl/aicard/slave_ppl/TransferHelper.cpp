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
#include "CommonDef.h"

#define CTRL_COMMAND_CHANNEL (0)
#define TRANSFER "TRANSFER"
using namespace std;

/**
 * Stream & Ctrl command recieve thread
 */
AX_VOID CTransferHelper::RecvThread(AX_VOID* pArg) {
    std::tuple<AX_U32, AX_U32>* pParams = (std::tuple<AX_U32, AX_U32>*)pArg;
    AX_U32 nIndex = std::get<0>(*pParams);
    AX_U32 nChannel = std::get<1>(*pParams);

    LOG_MM_I(TRANSFER, "[%d][%d] +++", nIndex, nChannel);

    while (m_vecThreadRecv[nIndex]->IsRunning()) {
        AI_CARD_SLV_PCIE_RECV_DATA_T* pResult = nullptr;
        if (0 == nChannel) {
            pResult = CPcieAdapter::GetInstance()->RecvCtrlCommand(m_tInitAttr.nRecvTimeout);
        } else {
            pResult = CPcieAdapter::GetInstance()->RecvStream(nChannel, m_tInitAttr.nRecvTimeout);
        }

        if (nullptr == pResult) {
            CElapsedTimer::GetInstance()->mSleep(1);
            continue;
        }

        AX_U8* pDataStart = nullptr;
        AX_U32 nDataSize = 0;
        AICARD_SLAVE_RECV_DATA_TYPE_E eDataType = RECV_DATA_TYPE_MAX;
        pDataStart = pResult->data + sizeof(PCIE_CMD_MSG_HEAD_T);
        nDataSize = pResult->nSize - sizeof(PCIE_CMD_MSG_HEAD_T);
        if (0 == nChannel) {
            eDataType = RECV_DATA_TYPE_CTRL;
        } else {
            eDataType = RECV_DATA_TYPE_STREAM;
        }

        CAXRingElement ele(pDataStart, nDataSize, 0, AX_TRUE);
        m_mapRecvData[nChannel].eDataType = eDataType;
        m_mapRecvData[nChannel].nChannel = nChannel;
        if (!m_mapRecvData[nChannel].pRingBuffer->Put(ele)) {
            LOG_M_D(TRANSFER, "[%d] Put video data(len=%d) into ringbuf full", nChannel, nDataSize);
        }

        CElapsedTimer::GetInstance()->mSleep(1);
    }

    LOG_MM_I(TRANSFER, "[%d][%d] ---", nIndex, nChannel);
}

/**
 * Detector results send thread
 */
AX_VOID CTransferHelper::SendThread(AX_VOID* pArg) {
    std::tuple<AX_U32, AX_U32>* pParams = (std::tuple<AX_U32, AX_U32>*)pArg;
    AX_U32 nIndex = std::get<0>(*pParams);
    AX_U32 nChannel = std::get<1>(*pParams);

    LOG_MM_I(TRANSFER, "[%d][%d] +++", nIndex, nChannel);

    DETECT_RESULT_T fhvp;
    while (m_vecThreadSend[nIndex]->IsRunning()) {
        if (CDetectResult::GetInstance()->Pop(nIndex - 1, fhvp)) {
            CPcieAdapter::GetInstance()->SendDetectResult(nChannel, fhvp, m_tInitAttr.nSendTimeout);
            CElapsedTimer::GetInstance()->mSleep(1);
            continue;
        }

        CElapsedTimer::GetInstance()->mSleep(1);
    }

    LOG_MM_I(TRANSFER, "[%d][%d] ---", nIndex, nChannel);
}

/**
 * Stream & Ctrl command dispatch (to vdec or NPU) thread
 */
AX_VOID CTransferHelper::DispatchThread(AX_VOID* pArg) {
    std::tuple<AX_U32, AX_U32>* pParams = (std::tuple<AX_U32, AX_U32>*)pArg;
    AX_U32 nIndex = std::get<0>(*pParams);
    AX_U32 nChannel = std::get<1>(*pParams);

    LOG_MM_I(TRANSFER, "[%d][%d] +++", nIndex, nChannel);

    while (m_vecThreadDispatch[nIndex]->IsRunning()) {
        if (m_mapRecvData[nChannel].pRingBuffer->IsEmpty()) {
            CElapsedTimer::GetInstance()->mSleep(1);
            continue;
        }

        CAXRingElement* pData = m_mapRecvData[nChannel].pRingBuffer->Get();
        if (nullptr == pData) {
            continue;
        }

        if (RECV_DATA_TYPE_STREAM == m_mapRecvData[nChannel].eDataType) {
            if (m_pVdecObserver) {
                /* Stream channels start with 1 */
                LOG_MM_D(TRANSFER, "[%d] Transfer stream to vdec, size=%d.", nChannel - SHARED_PORT_NUMBER, pData->nSize);
                m_pVdecObserver->OnRecvVideoData(nChannel - SHARED_PORT_NUMBER, pData->pBuf, pData->nSize, pData->nPts);
            }
        } else if (RECV_DATA_TYPE_CTRL == m_mapRecvData[nChannel].eDataType) {
            for (auto& obs : m_vecCtrlObservers) {
                LOG_MM_D(TRANSFER, "[%d] Transfer ctrl command to detector, size=%d.", nChannel, pData->nSize);
                obs->OnRecvData(E_OBS_TARGET_TYPE_AICARD_TRANSFER, 0, nChannel, pData->pBuf);
            }
        }

        m_mapRecvData[nChannel].pRingBuffer->Pop();

        CElapsedTimer::GetInstance()->mSleep(1);
    }

    LOG_MM_I(TRANSFER, "[%d][%d] ---", nIndex, nChannel);
}

AX_BOOL CTransferHelper::Init(const TRANSFER_ATTR_T& stAttr) {
    LOG_MM_I(TRANSFER, "+++");

    m_tInitAttr = stAttr;
    AX_U32 nDataChannelCount = stAttr.nMaxVideoCount * 2/* send + recv */;

    if (!CPcieAdapter::GetInstance()->Init(nDataChannelCount + SHARED_PORT_NUMBER, stAttr.nBuffSize * 1024, stAttr.bEnableSimulateDetRets, stAttr.nTraceData, stAttr.nRetryCount)) {
        return AX_FALSE;
    }

    for (AX_U8 i = 0; i < stAttr.nMaxVideoCount + SHARED_PORT_NUMBER; i++) {
        // TODO: Not distinguish from stream and control command now
        m_mapRecvData[i].pRingBuffer = new CAXRingBuffer(stAttr.nBuffSize * 1024, stAttr.nBuffCount);

        m_vecThreadRecv.push_back(new CAXThread());
        m_vecThreadDispatch.push_back(new CAXThread());
        m_vecThreadSend.push_back(new CAXThread());

        m_mapRecvThreadParams[i] = make_tuple(i, i);
        m_mapDispThreadParams[i] = make_tuple(i, i);
        m_mapSendThreadParams[i] = make_tuple(i, (i < SHARED_PORT_NUMBER ? i : i + stAttr.nMaxVideoCount));
    }

    if (!HandShake()) {
        return AX_FALSE;
    }

    LOG_MM_I(TRANSFER, "---");
    return AX_TRUE;
}

AX_BOOL CTransferHelper::DeInit(AX_VOID) {
    LOG_MM_I(TRANSFER, "+++");

    for (auto& m : m_vecThreadSend) {
        if (m->IsRunning()) {
            LOG_MM_E(TRANSFER, "Send thread is still running.");
            return AX_FALSE;
        }
        delete(m);
        m = nullptr;
    }

    for (auto& m : m_vecThreadRecv) {
        if (m->IsRunning()) {
            LOG_MM_E(TRANSFER, "Recv thread is still running.");
            return AX_FALSE;
        }
        delete(m);
        m = nullptr;
    }

    for (auto& m : m_vecThreadDispatch) {
        if (m->IsRunning()) {
            LOG_MM_E(TRANSFER, "Dispatch thread is still running.");
            return AX_FALSE;
        }
        delete(m);
        m = nullptr;
    }

    LOG_MM_I(TRANSFER, "---");
    return AX_TRUE;
}

AX_BOOL CTransferHelper::Start(AX_VOID) {
    LOG_MM_I(TRANSFER, "+++");

    AX_U32 nVideoCount = m_tInitAttr.nMaxVideoCount;
    for (AX_U8 i = 0; i < nVideoCount + SHARED_PORT_NUMBER; i++) {
        if (!m_vecThreadDispatch[i]->Start([this](AX_VOID* pArg) -> AX_VOID { DispatchThread(pArg); }, &m_mapDispThreadParams[i], "ACTransDisp")) {
            LOG_MM_E(TRANSFER, "[%d] Create dispatch thread failed.", i);
            return AX_FALSE;
        }
    }

    for (AX_U8 i = 0; i < nVideoCount + SHARED_PORT_NUMBER; i++) {
        if (!m_vecThreadRecv[i]->Start([this](AX_VOID* pArg) -> AX_VOID { RecvThread(pArg); }, &m_mapRecvThreadParams[i], "ACTransRecv")) {
            LOG_MM_E(TRANSFER, "Create receive thread failed.");
            return AX_FALSE;
        }
    }

    for (AX_U8 i = 0; i < nVideoCount + SHARED_PORT_NUMBER; i++) {
        if (!m_vecThreadSend[i]->Start([this](AX_VOID* pArg) -> AX_VOID { SendThread(pArg); }, &m_mapSendThreadParams[i], "ACTransSend")) {
            LOG_MM_E(TRANSFER, "Create send thread failed.");
            return AX_FALSE;
        }
    }

    LOG_MM_I(TRANSFER, "---");
    return AX_TRUE;
}

AX_BOOL CTransferHelper::Stop(AX_VOID) {
    LOG_MM_I(TRANSFER, "+++");

    for (auto& m : m_vecThreadSend) {
        m->Stop();
    }

    for (auto& m : m_vecThreadDispatch) {
        m->Stop();
    }

    for (auto& m : m_vecThreadRecv) {
        m->Stop();
    }

    CPcieAdapter::GetInstance()->DeInit();

    for (auto& m : m_vecThreadSend) {
        m->Join();
    }

    for (auto& m : m_vecThreadDispatch) {
        m->Join();
    }

    for (auto& m : m_vecThreadRecv) {
        m->Join();
    }

    LOG_MM_I(TRANSFER, "---");
    return AX_TRUE;
}

AX_BOOL CTransferHelper::RegStreamObserver(IStreamObserver* pObs) {
    m_pVdecObserver = pObs;
    return AX_TRUE;
}

AX_BOOL CTransferHelper::RegCommandObserver(IObserver* pObs) {
    m_vecCtrlObservers.push_back(pObs);
    return AX_TRUE;
}

AX_BOOL CTransferHelper::HandShake(AX_VOID) {
    if (CPcieAdapter::GetInstance()->RecvHandShakePacket()) {
        return CPcieAdapter::GetInstance()->SendHandShakeFeedback();
    }

    return AX_FALSE;
}
