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

using namespace std;

AX_BOOL CPcieAdapter::Init(AX_U32 nSlaveCount, AX_U32 nVideoCount, AX_U32 nBufferSize, AX_S16 nTraceData, AX_S16 nRetryCount) {
    m_nSlaveCount = nSlaveCount;

    AX_U32 nAllChannelCount = nVideoCount * 2/* Send + Recv */ + nSlaveCount * SHARED_PORT_NUMBER;
    m_vecSendData.resize(nAllChannelCount); /* Only parts used */
    m_vecRecvData.resize(nAllChannelCount); /* Only parts used */

    AX_U32 nAllShardChnNum = SHARED_PORT_NUMBER * nSlaveCount;
    AX_U32 nDataChnCnt = nAllChannelCount - nAllShardChnNum;
    AX_U32 nChnNumPerDev = nDataChnCnt % nSlaveCount ? nDataChnCnt / nSlaveCount + 1 : nDataChnCnt / nSlaveCount;
    AX_U32 nAllPortCnt = nChnNumPerDev * nSlaveCount + nAllShardChnNum;
    AX_S32 nAllSendCount = nDataChnCnt / 2;

    for (AX_U32 i = 0; i < nAllPortCnt; i++) {
        AX_U32 nDevIndex = 0;
        AX_U32 nChannelIndex = 0;
        if (i < nAllShardChnNum) {
            /* command channels */
            nDevIndex = i / SHARED_PORT_NUMBER;
            nChannelIndex = i % SHARED_PORT_NUMBER;
        } else if (i < nAllShardChnNum + nAllSendCount) {
            /* send channels */
            nDevIndex = (i - nAllShardChnNum) / (nChnNumPerDev / 2);
            nChannelIndex = (i - nAllShardChnNum) % (nChnNumPerDev / 2) + SHARED_PORT_NUMBER;
        } else {
            /* recv channels */
            nDevIndex = (i - nAllShardChnNum - nAllSendCount) / (nChnNumPerDev / 2);
            nChannelIndex = (i - nAllShardChnNum - nAllSendCount) % (nChnNumPerDev / 2) + SHARED_PORT_NUMBER + nChnNumPerDev / 2;
        }

        LOG_MM_I(TAG, "[Port %d] => [Device %d][Channel %d]", i, nDevIndex, nChannelIndex);

        m_mapChn2DevInfo[i] = make_tuple(nDevIndex, nChannelIndex);
    }

    return CPcieOperator::GetInstance()->Init(AX_TRUE, nSlaveCount, nAllPortCnt, nBufferSize, nTraceData, nRetryCount) == 0 ? AX_TRUE : AX_FALSE;
}

AX_BOOL CPcieAdapter::DeInit() {
    return CPcieOperator::GetInstance()->DeInit() == 0 ? AX_TRUE : AX_FALSE;
}

AX_BOOL CPcieAdapter::SendStream(AX_U8 nUniChannel, AX_U8 nType, AX_VOID* pData, AX_U32 nSize, AX_S16 nTimeout /*= -1*/) {
    auto devInfo = GetDevInfo(nUniChannel);

    if (!GenProtocalStreamData(nUniChannel, nType, pData, nSize)) {
        return AX_FALSE;
    }

    PCIE_BODY_T& tBody = m_vecSendData[nUniChannel].tBody;
    CPcieOperator::GetInstance()->Send((PCIE_CMD_TYPE_E)tBody.tCtrl.nCmdType, std::get<0>(devInfo), std::get<1>(devInfo), (AX_U8*)&tBody, tBody.tCtrl.nDataLen, nTimeout);

    return AX_TRUE;
}

DETECT_RESULT_T* CPcieAdapter::RecvDetectorResult(AX_U32 nUniChannel, AX_S16 nTimeout /*= -1*/) {
    auto devInfo = GetDevInfo(nUniChannel);
    AX_S16 nDeviceID = std::get<0>(devInfo);
    AX_S16 nChannelID = std::get<1>(devInfo);

    memset(&m_vecRecvData[nUniChannel], 0, sizeof(PCIE_DATA_T));

    AX_S32 nSize = CPcieOperator::GetInstance()->Recv(PCIE_CMD_AI_RESULT_E, nDeviceID, nChannelID, (AX_U8*)&m_vecRecvData[nUniChannel].tBody, MAX_PCIE_BODY_SIZE, nTimeout);
    if (nSize < 0) {
        if (-2 != nSize) { /* -2: Timeout */
            LOG_M_E(TAG, "Recieve detect result failed, ret=%d", nSize);
        }
        return nullptr;
    }

    if (nSize < (AX_S32)(sizeof(DETECT_RESULT_T) + sizeof(PCIE_CMD_MSG_HEAD_T))) {
        return nullptr;
    }

    AX_U8* pDetectResult = (AX_U8*)&m_vecRecvData[nUniChannel].tBody.body;
    return (DETECT_RESULT_T *)pDetectResult;
}

AX_BOOL CPcieAdapter::SendCtrlCommand(AI_CARD_AI_SWITCH_ATTR_T& tSwitchAttr, AX_S16 nTimeout /*= -1*/) {
    if (!GenProtocalCtrlData(tSwitchAttr)) {
        return AX_FALSE;
    }

    if (PCIE_CMD_SWITCH_AI_E == m_tCtrlData.tBody.tCtrl.nCmdType) {
        LOG_M_C(TAG, "AI switching started: (%s, %d).", tSwitchAttr.szModelPath, tSwitchAttr.nChannelNum);
        for (AX_U8 i = 0; i < tSwitchAttr.nChannelNum; i++) {
            LOG_M_C(TAG, "[Chn:%d]: [%d, %d, %d]"
                                , i
                                , tSwitchAttr.arrChnParam[i].nPPL
                                , tSwitchAttr.arrChnParam[i].bTrackEnable
                                , tSwitchAttr.arrChnParam[i].nVNPU);
        }
    }

    for (AX_U32 i = 0; i < m_nSlaveCount; i++) {
        AX_S32 nSize = CPcieOperator::GetInstance()->Send((PCIE_CMD_TYPE_E)m_tCtrlData.tBody.tCtrl.nCmdType, i, m_tCtrlData.tBody.tCtrl.nChannel, (AX_U8*)&m_tCtrlData.tBody, m_tCtrlData.tBody.tCtrl.nDataLen, nTimeout);
        if (nSize < 0) {
            LOG_MM_E(TAG, "[%d][%d] Send data failed, ret=%d.", i, m_tCtrlData.tBody.tCtrl.nChannel, nSize);
            return AX_FALSE;
        }

        if ((AX_U32)nSize != m_tCtrlData.tBody.tCtrl.nDataLen + sizeof(PCIE_CMD_MSG_HEAD_T)) {
            LOG_MM_E(TAG, "Send data failed, data size: %d, send size: %d", m_tCtrlData.tBody.tCtrl.nDataLen, nSize);
            return AX_FALSE;
        }
    }

    return AX_TRUE;
}

AX_S32 CPcieAdapter::RecvCtrlCmdResult(PCIE_CMD_TYPE_E eType, PCIE_DATA_T* pRecvData, AX_S16 nTimeout /*= -1*/) {
#ifndef __MASTER_DEBUG__
    AX_S32 nResultTotal = 0;
    for (AX_U32 i = 0; i < m_nSlaveCount; i++) {
        AX_S32 nSize = CPcieOperator::GetInstance()->Recv(eType, i, PCIE_COMMON_MSG_CHANNEL, (AX_U8 *)pRecvData, sizeof(AX_S32), nTimeout);
        if (nSize < 0) {
            if (-2 != nSize) { /* -2: Timeout */
                LOG_M_E(TAG, "Recieve control command result failed, ret=%d", nSize);
            }
            return -1;
        }

        if (PCIE_CMD_SWITCH_AI_E == eType) {
            AX_S32 nResult = *(AX_S32*)pRecvData->tBody.body;
            if (nResult) {
                LOG_M_C(TAG, "[Device %d] AI switch successfully.", i);
            } else {
                LOG_M_C(TAG, "[Device %d] AI switch failed, ret=%d.", i, nResult);
            }

            nResultTotal += nResult;
        }
    }

    return nResultTotal == m_nSlaveCount ? 1 : 0;
#else
    AX_S32 nSize = CPcieOperator::GetInstance()->RecvCmdRet(eType, PCIE_COMMON_MSG_CHANNEL, (AX_U8 *)&nResult, sizeof(AX_U32));
    if (nSize <= 0) {
        return AX_FALSE;
    }

    if (PCIE_CMD_SWITCH_AI_E == eType && 1 == nResult) {
        LOG_M_C(TAG, "AI switch finished.");
        return AX_TRUE;
    }
#endif
    return AX_FALSE;
}

AX_BOOL CPcieAdapter::SendHandShakePacket() {
    PCIE_DATA_T tHandShakePacket;
    memset(&tHandShakePacket, 0, sizeof(PCIE_DATA_T));

    if (!GenProtocalHandShake(tHandShakePacket)) {
        return AX_FALSE;
    }

    for (AX_U32 i = 0; i < m_nSlaveCount; i++) {
        AX_S32 nSize = CPcieOperator::GetInstance()->Send((PCIE_CMD_TYPE_E)tHandShakePacket.tBody.tCtrl.nCmdType, i, tHandShakePacket.tBody.tCtrl.nChannel, (AX_U8*)&tHandShakePacket.tBody, tHandShakePacket.tBody.tCtrl.nDataLen);
        if (-1 == nSize) {
            LOG_MM_E(TAG, "Send handshake packet failed.");
            return AX_FALSE;
        }

        if ((AX_U32)nSize != m_tCtrlData.tBody.tCtrl.nDataLen + sizeof(PCIE_CMD_MSG_HEAD_T)) {
            LOG_MM_E(TAG, "Send handshake packet failed, data size: %d, send size: %d", m_tCtrlData.tBody.tCtrl.nDataLen, nSize);
            return AX_FALSE;
        }
    }

    LOG_M_C(TAG, "Send handshake packet successfully.");

    return AX_TRUE;
}

AX_S32 CPcieAdapter::RecvHandShakeFeedback() {
    AX_S32 nFeedback = 0;
    for (AX_U32 i = 0; i < m_nSlaveCount; i++) {
        memset(&m_tCtrlData, 0, sizeof(PCIE_DATA_T));

        AX_S32 nSize = CPcieOperator::GetInstance()->Recv(PCIE_CMD_HAND_SHAKE, i, PCIE_COMMON_MSG_CHANNEL, (AX_U8*)&m_tCtrlData.tBody, MAX_PCIE_BODY_SIZE, -1);
        if (nSize < 0) {
            LOG_M_E(TAG, "Recieve handshake feedback failed, ret=%d", nSize);
            return 0;
        }

        if (nSize < (AX_S32)(sizeof(AX_S32) + sizeof(PCIE_CMD_MSG_HEAD_T))) {
            return 0;
        }

        nFeedback = *(AX_S32*)&m_tCtrlData.tBody.body;
        LOG_M_C(TAG, "[Device %d] Recv handshake feedback: %d successfully.", i, nFeedback);
    }

    return nFeedback;
}

AX_BOOL CPcieAdapter::GenProtocalStreamData(AX_U8 nUniChannel, AX_U8 nType, AX_VOID* pData, AX_U32 nSize) {
    memset(&m_vecSendData[nUniChannel], 0, sizeof(PCIE_DATA_T));

    m_vecSendData[nUniChannel].tBody.tCtrl.nCmdType = (PCIE_CMD_TYPE_E)nType;
    m_vecSendData[nUniChannel].tBody.tCtrl.nChannel = 0; /* must filled with physical channel outside */
    m_vecSendData[nUniChannel].tBody.tCtrl.nSn = 0;
    m_vecSendData[nUniChannel].tBody.tCtrl.nDataLen = nSize;
    memcpy(&m_vecSendData[nUniChannel].tBody.body, pData, nSize);

    return AX_TRUE;
}

AX_BOOL CPcieAdapter::GenProtocalCtrlData(AI_CARD_AI_SWITCH_ATTR_T& tSwitchAttr) {
    memset(&m_tCtrlData, 0, sizeof(PCIE_DATA_T));

    m_tCtrlData.tBody.tCtrl.nCmdType = PCIE_CMD_SWITCH_AI_E;
    m_tCtrlData.tBody.tCtrl.nChannel = PCIE_COMMON_MSG_CHANNEL;
    m_tCtrlData.tBody.tCtrl.nSn = 0;
    m_tCtrlData.tBody.tCtrl.nDataLen = sizeof(AI_CARD_AI_SWITCH_ATTR_T);
    memcpy(&m_tCtrlData.tBody.body, &tSwitchAttr, sizeof(AI_CARD_AI_SWITCH_ATTR_T));

    return AX_TRUE;
}

AX_BOOL CPcieAdapter::GenProtocalHandShake(PCIE_DATA_T &tOutPacket) {
    memset(&tOutPacket, 0, sizeof(PCIE_DATA_T));

    tOutPacket.tBody.tCtrl.nCmdType = PCIE_CMD_HAND_SHAKE;
    tOutPacket.tBody.tCtrl.nChannel = PCIE_COMMON_MSG_CHANNEL;
    tOutPacket.tBody.tCtrl.nSn = 0;
    tOutPacket.tBody.tCtrl.nDataLen = 0;

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

tuple<AX_S16, AX_S16> CPcieAdapter::GetDevInfo(AX_S32 nUniChannel) {
    map<AX_S16, tuple<AX_S16, AX_S16>>::iterator itFinder = m_mapChn2DevInfo.find(nUniChannel);
    if (itFinder != m_mapChn2DevInfo.end()) {
        return itFinder->second;
    }

    return {};
}