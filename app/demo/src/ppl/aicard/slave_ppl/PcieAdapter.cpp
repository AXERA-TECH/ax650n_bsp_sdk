/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "PcieAdapter.hpp"
#include "AppLog.hpp"

#define TAG "PCIE_ADAPTER"
#define SHARED_PORT_NUMBER (1)

AX_BOOL CPcieAdapter::Init(AX_U32 nChannelCount, AX_U32 nBufferSize, AX_BOOL bSimulateDetRets /*= AX_FALSE*/, AX_S16 nTraceData, AX_S16 nRetryCount) {
    m_vecRecvStreamData.resize(nChannelCount);
    m_vecDetectResultData.resize(nChannelCount);

    return CPcieOperator::GetInstance()->Init(AX_FALSE, 1, nChannelCount, nBufferSize, bSimulateDetRets, nTraceData, nRetryCount) == 0 ? AX_TRUE : AX_FALSE;
}

AX_BOOL CPcieAdapter::DeInit() {
    return CPcieOperator::GetInstance()->DeInit() == 0 ? AX_TRUE : AX_FALSE;
}

AX_BOOL CPcieAdapter::RecvHandShakePacket() {
    AX_S32 nSize = CPcieOperator::GetInstance()->Recv(PCIE_CMD_HAND_SHAKE, PCIE_COMMON_MSG_CHANNEL, m_vecRecvStreamData[PCIE_COMMON_MSG_CHANNEL].data, MAX_PCIE_BODY_SIZE, -1);
    if (nSize < 0) {
        LOG_M_E(TAG, "Recieve handshake packet failed, ret=%d", nSize);
        return AX_FALSE;
    }

    LOG_M_C(TAG, "Recv handshake packet successfully.");

    return AX_TRUE;
}

AX_BOOL CPcieAdapter::SendHandShakeFeedback() {
    PCIE_DATA_T tFeedbackPacket;
    memset(&tFeedbackPacket, 0, sizeof(PCIE_DATA_T));

    if (!GenProtocalHandShakeFeedback(1, tFeedbackPacket)) {
        return AX_FALSE;
    }

    AX_S32 nSize = CPcieOperator::GetInstance()->Send((PCIE_CMD_TYPE_E)tFeedbackPacket.tBody.tCtrl.nCmdType,
                                                 tFeedbackPacket.tBody.tCtrl.nChannel,
                                                 (AX_U8*)&tFeedbackPacket.tBody,
                                                 tFeedbackPacket.tBody.tCtrl.nDataLen,
                                                 -1);
    if (nSize < 0) {
        LOG_MM_E(TAG, "Send handshake feedback failed.");
        return AX_FALSE;
    } else {
        LOG_M_C(TAG, "Send handshake feedback packet successfully.");
    }

    return AX_TRUE;
}

AI_CARD_SLV_PCIE_RECV_DATA_T* CPcieAdapter::RecvStream(AX_U32 nChannel, AX_S16 nTimeout /*= -1*/) {
    AX_S32 nSize = CPcieOperator::GetInstance()->Recv(PCIE_CMD_H264_DATA_E, nChannel, m_vecRecvStreamData[nChannel].data, MAX_PCIE_BODY_SIZE, nTimeout);
    if (nSize <= 0) {
        if (-2 != nSize) {
            LOG_M_E(TAG, "[%d] Recieve stream failed, ret=%d", nChannel, nSize);
        }
        return nullptr;
    }

    m_vecRecvStreamData[nChannel].nChannel = nChannel;
    m_vecRecvStreamData[nChannel].nCmdType = PCIE_CMD_H264_DATA_E;
    m_vecRecvStreamData[nChannel].nSize = nSize;

    return &m_vecRecvStreamData[nChannel];
}

AI_CARD_SLV_PCIE_RECV_DATA_T* CPcieAdapter::RecvCtrlCommand(AX_S16 nTimeout /*= -1*/) {
    AX_S32 nSize = CPcieOperator::GetInstance()->Recv(PCIE_CMD_SWITCH_AI_E, 0, m_vecRecvStreamData[0].data, MAX_PCIE_BODY_SIZE, nTimeout);
    if (nSize < 0) {
        if (-2 != nSize) {
            LOG_M_E(TAG, "Recieve control command failed, ret=%d", nSize);
        }
        return nullptr;
    }

    m_vecRecvStreamData[0].nChannel = PCIE_COMMON_MSG_CHANNEL;
    m_vecRecvStreamData[0].nCmdType = PCIE_CMD_SWITCH_AI_E;
    m_vecRecvStreamData[0].nSize = nSize;

    return &m_vecRecvStreamData[0];
}

AX_BOOL CPcieAdapter::SendDetectResult(AX_U32 nChannel, const DETECT_RESULT_T& tResult, AX_S16 nTimeout /*= -1*/) {
    if (!GenProtocalDetectResultData(nChannel, tResult)) {
        return AX_FALSE;
    }

    PCIE_DATA_T& tChnResult = m_vecDetectResultData[nChannel];
    AX_S32 nSize = CPcieOperator::GetInstance()->Send((PCIE_CMD_TYPE_E)tChnResult.tBody.tCtrl.nCmdType,
                                                 tChnResult.tBody.tCtrl.nChannel,
                                                 (AX_U8*)&tChnResult.tBody,
                                                 tChnResult.tBody.tCtrl.nDataLen,
                                                 nTimeout);

    if (nSize < 0) {
        LOG_MM_E(TAG, "[%d] Send detect result failed, ret=%d.", nChannel, nSize);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CPcieAdapter::SendCtrlResult(AX_S32 nResult, AX_S16 nTimeout /*= -1*/) {
    if (!GenProtocalCtrlResultData(nResult)) {
        return AX_FALSE;
    }

    AX_S32 nSize = CPcieOperator::GetInstance()->Send((PCIE_CMD_TYPE_E)m_tCtrlResultData.tBody.tCtrl.nCmdType,
                                                 m_tCtrlResultData.tBody.tCtrl.nChannel,
                                                 (AX_U8*)&m_tCtrlResultData.tBody,
                                                 m_tCtrlResultData.tBody.tCtrl.nDataLen,
                                                 nTimeout);
    if (nSize < 0) {
        LOG_MM_E(TAG, "Send control command result failed, ret=%d.", nSize);
        return AX_FALSE;
    } else {
        LOG_B(&m_tCtrlResultData.tBody, m_tCtrlResultData.tBody.tCtrl.nDataLen + 16, APP_LOG_ASYN_SEND);
    }

    return AX_TRUE;
}

AX_BOOL CPcieAdapter::GenProtocalDetectResultData(AX_U32 nChannel, const DETECT_RESULT_T& tResult) {
    memset(&m_vecDetectResultData[nChannel], 0, sizeof(PCIE_DATA_T));

    m_vecDetectResultData[nChannel].tBody.tCtrl.nSn = 0;
    m_vecDetectResultData[nChannel].tBody.tCtrl.nCmdType = PCIE_CMD_AI_RESULT_E;
    m_vecDetectResultData[nChannel].tBody.tCtrl.nChannel = nChannel;
    m_vecDetectResultData[nChannel].tBody.tCtrl.nDataLen = sizeof(DETECT_RESULT_T);
    memcpy(&m_vecDetectResultData[nChannel].tBody.body, &tResult, sizeof(DETECT_RESULT_T));

    return AX_TRUE;
}

AX_BOOL CPcieAdapter::GenProtocalCtrlResultData(AX_S32 nResult) {
    memset(&m_tCtrlResultData, 0, sizeof(PCIE_DATA_T));

    m_tCtrlResultData.tBody.tCtrl.nSn = 0;
    m_tCtrlResultData.tBody.tCtrl.nCmdType = PCIE_CMD_SWITCH_AI_E;
    m_tCtrlResultData.tBody.tCtrl.nChannel = PCIE_COMMON_MSG_CHANNEL;
    m_tCtrlResultData.tBody.tCtrl.nDataLen = sizeof(AX_S32);
    memcpy(&m_tCtrlResultData.tBody.body, &nResult, sizeof(AX_S32));

    return AX_TRUE;
}

AX_BOOL CPcieAdapter::GenProtocalHandShakeFeedback(AX_S32 nResult, PCIE_DATA_T &tOutPacket) {
    memset(&tOutPacket, 0, sizeof(PCIE_DATA_T));

    tOutPacket.tBody.tCtrl.nSn = 0;
    tOutPacket.tBody.tCtrl.nCmdType = PCIE_CMD_HAND_SHAKE;
    tOutPacket.tBody.tCtrl.nChannel = PCIE_COMMON_MSG_CHANNEL;
    tOutPacket.tBody.tCtrl.nDataLen = sizeof(AX_S32);
    memcpy(&tOutPacket.tBody.body, &nResult, sizeof(AX_S32));

    return AX_TRUE;
}


AX_U8 CPcieAdapter::CheckSum8(AX_U8* pData, AX_U32 nSize) {
    AX_U16 nSum = 0;
    for (AX_U32 i = 0; i < nSize; i++){
        nSum += pData[i];
    }

    return (AX_U8)(nSum & 0x00FF);
}

AX_BOOL CPcieAdapter::CheckData(AX_U8* pData) {
    if (nullptr == pData) {
        return AX_FALSE;
    }

    PCIE_DATA_T* pRecvData = (PCIE_DATA_T*)pData;
    if (pRecvData->tBody.tCtrl.nCheckSum != CheckSum8((AX_U8 *)&pRecvData->tBody.tCtrl, sizeof(pRecvData->tBody.tCtrl) - 2)) {
        return AX_FALSE;
    }

    return AX_TRUE;
}