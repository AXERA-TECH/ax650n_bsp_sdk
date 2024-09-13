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
#include "ElapsedTimer.hpp"

AX_S32 CPcieOperator::Init(AX_BOOL bMaster, AX_U16 nSlaveCount, AX_U16 nChannelNum, AX_U32 nDmaBufferSize, AX_BOOL bSimulateDetRets /*= AX_FALSE*/, AX_S16 nTraceData, AX_S16 nRetryCount) {
    m_bSimulateDetRets = bSimulateDetRets;
    return PCIe_Init(bMaster, nSlaveCount, nChannelNum, nDmaBufferSize, nTraceData, nRetryCount);
}

AX_S32 CPcieOperator::DeInit() {
    return PCIe_DeInit();
}

AX_S32 CPcieOperator::Send(PCIE_CMD_TYPE_E nCmdType, AX_S16 nChannel, AX_U8* pDataBuf, AX_U32 nSize, AX_S16 nTimeout /*= -1*/) {
    LOG_M_D("PcieOpr", "[SLV][%d][SEND] size: %d", nChannel, nSize);
    AX_S32 nSendSize = PCIe_Send(nCmdType, 0, nChannel, pDataBuf, nSize, nTimeout);
    if (nSendSize > 0) {
        CPrintHelper::GetInstance()->Add(E_PH_MOD_PCIE_SEND, nChannel);
    }

    return nSendSize;
}

AX_S32 CPcieOperator::Recv(PCIE_CMD_TYPE_E nCmdType, AX_S16 nChannel, AX_U8* pDataBuf, AX_U32 nSize, AX_S16 nTimeout /*= -1*/) {
    AX_S32 nRecvSize = PCIe_Recv(nCmdType, 0, nChannel, pDataBuf, nSize, nTimeout);
    LOG_M_D("PcieOpr", "[SLV][%d][RECV] size: %d", nChannel, nRecvSize);
    if (nRecvSize > 0) {
        CPrintHelper::GetInstance()->Add(E_PH_MOD_PCIE_RECV, nChannel);

        if (m_bSimulateDetRets && (PCIE_CMD_H264_DATA_E == nCmdType || PCIE_CMD_H265_DATA_E == nCmdType)) {
            CElapsedTimer::GetInstance()->mSleep(2);
            DETECT_RESULT_T tResult;
            GenSimulateResult(nChannel, tResult);
            CDetectResult::GetInstance()->Set(nChannel - 1, tResult);
        }
    }

    return nRecvSize;
}

AX_VOID CPcieOperator::GenSimulateResult(AX_S16 nChannel, DETECT_RESULT_T& tResult) {
    static AX_U32 s_nSeq = 0;
    static AX_U32 s_nResultCount = 1;

    tResult.nCount = s_nResultCount++ % 10 + 1;
    for (AX_U32 i = 0; i < tResult.nCount; i++) {
        tResult.item[i].eType = (DETECT_TYPE_E)(rand() % DETECT_TYPE_BUTT);
        tResult.item[i].tBox.fX = rand() % (1920 - 200);
        tResult.item[i].tBox.fY = rand() % (1080 - 200);
        tResult.item[i].tBox.fW = 100;
        tResult.item[i].tBox.fH = 100;
    }
    tResult.nGrpId = (nChannel - 1);
    tResult.nW = 1920;
    tResult.nH = 1080;
    tResult.nSeqNum = s_nSeq++;
}