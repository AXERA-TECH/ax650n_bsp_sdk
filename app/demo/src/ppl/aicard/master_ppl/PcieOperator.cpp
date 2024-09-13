/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "PcieOperator.hpp"
#include "AppLog.hpp"
#include "PrintHelper.hpp"

#define SHARED_PORT_NUMBER (1)

AX_S32 CPcieOperator::Init(AX_BOOL bMaster, AX_U16 nSlaveCount, AX_U16 nChannelNum, AX_U32 nDmaBufferSize, AX_S16 nTraceData, AX_S16 nRetryCount) {
    m_nSlaveCount = nSlaveCount;
    m_nAllChannelCount = nChannelNum;
    return PCIe_Init(bMaster, nSlaveCount, nChannelNum, nDmaBufferSize, nTraceData, nRetryCount);
}

AX_S32 CPcieOperator::DeInit() {
    return PCIe_DeInit();
}

AX_S32 CPcieOperator::Send(PCIE_CMD_TYPE_E nCmdType, AX_S16 nDevice, AX_S16 nChannel, AX_U8* pDataBuf, AX_U32 nSize, AX_S16 nTimeout /*= -1*/) {
    LOG_M_D("PcieOpr", "[MST][Dev:%d][Chn:%d][SEND] PcieOpr => type: %d, size: %d", nDevice, nChannel, nCmdType, nSize);
    AX_S32 nSendSize = PCIe_Send(nCmdType, nDevice, nChannel, pDataBuf, nSize, nTimeout);
    if (nSendSize > 0 && nChannel > 0/* Do not print command channle's fps statistics */) {
        AX_U32 nDataSendChnPerDevice = (m_nAllChannelCount - m_nSlaveCount * SHARED_PORT_NUMBER) / 2/*send + recv*/ / m_nSlaveCount;
        AX_U32 nVideoIndex = nChannel + nDevice * nDataSendChnPerDevice;
        CPrintHelper::GetInstance()->Add(E_PH_MOD_PCIE_SEND, nVideoIndex);
    }

    return nSendSize;
}

AX_S32 CPcieOperator::Recv(PCIE_CMD_TYPE_E nCmdType, AX_S16 nDevice, AX_S16 nChannel, AX_U8* pDataBuf, AX_U32 nSize, AX_S16 nTimeout /*= -1*/) {
#ifndef __MASTER_DEBUG__
    AX_S32 nRecvSize = PCIe_Recv(nCmdType, nDevice, nChannel, pDataBuf, nSize, nTimeout);
    LOG_M_D("PcieOpr", "[MST][Dev:%d][Chn:%d][RECV] PcieOpr => size: %d", nDevice, nChannel, nRecvSize);
    if (nRecvSize > 0 && nChannel > 0/* Do not print command channle's fps statistics */) {
        AX_U32 nDataSendChnPerDevice = (m_nAllChannelCount - m_nSlaveCount * SHARED_PORT_NUMBER) / 2/*send + recv*/ / m_nSlaveCount;
        AX_U32 nVideoIndex = nChannel - nDataSendChnPerDevice  + nDevice * nDataSendChnPerDevice;
        CPrintHelper::GetInstance()->Add(E_PH_MOD_PCIE_RECV, nVideoIndex);
    }

    return nRecvSize;
#else
    DETECT_RESULT_T tResult;
    GenSimulateResult(tResult);

    memcpy(pDataBuf, &tResult, sizeof(DETECT_RESULT_T));

    return sizeof(DETECT_RESULT_T);
#endif
}

#ifdef __MASTER_DEBUG__
static int s_nCmdFeedbackWaitCount = 5000; // freq:1ms * times:5000 = 5s
static int s_nCmdIndex = 0;
AX_S32 CPcieOperator::RecvCmdRet(PCIE_CMD_TYPE_E nCmdType, AX_U32 nChannel, AX_U8* pDataBuf, AX_U32 nSize) {
    if (s_nCmdIndex++ < s_nCmdFeedbackWaitCount) {
        return -1;
    } else {
        s_nCmdIndex = 0;
    }

    AX_U32 nFinish = 1;
    memcpy(pDataBuf, &nFinish, sizeof(AX_U32));

    return sizeof(AX_U32);
}

AX_VOID CPcieOperator::GenSimulateResult(DETECT_RESULT_T& tResult) {
    static AX_U32 s_nSeq = 0;
    static AX_U32 s_nResultCount = 1;

    tResult.nCount = s_nResultCount++ % 5 + 1;
    for (AX_U32 i = 0; i < tResult.nCount; i++) {
        tResult.item[i].eType = (DETECT_TYPE_E)(rand() % DETECT_TYPE_BUTT);
        tResult.item[i].tBox.fX = rand() % (1920 - 200);
        tResult.item[i].tBox.fY = rand() % (1080 - 200);
        tResult.item[i].tBox.fW = 100;
        tResult.item[i].tBox.fH = 100;
    }
    tResult.nGrpId = rand() % 32;
    tResult.nW = 1920;
    tResult.nH = 1080;
    tResult.nSeqNum = s_nSeq++;
}
#endif