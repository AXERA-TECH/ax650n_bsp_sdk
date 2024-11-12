/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#pragma once

#include "AXSingleton.h"
#include "DetectResult.hpp"
#include "pcie_api.h"

#define PACKED(x) __attribute__((aligned(x), packed))
#define MAX_PCIE_BODY_SIZE (622080)

typedef struct {
    PCIE_CMD_MSG_HEAD_T tCtrl;
    AX_U8  body[MAX_PCIE_BODY_SIZE];
} PCIE_BODY_T;

typedef struct {
    PCIE_BODY_T tBody;
} PCIE_DATA_T;

class CPcieOperator : public CAXSingleton<CPcieOperator> {
    friend class CAXSingleton<CPcieOperator>;

public:
    CPcieOperator(AX_VOID) = default;
    ~CPcieOperator(AX_VOID) = default;

    AX_S32 Init(AX_BOOL bMaster, AX_U16 nSlaveCount, AX_U16 nChannelNum, AX_U32 nDmaBufferSize, AX_S16 nTraceData = 0, AX_S16 nRetryCount = 1);
    AX_S32 DeInit();
    AX_S32 Send(PCIE_CMD_TYPE_E nCmdType, AX_S16 nDevice, AX_S16 nChannel, AX_U8* pDataBuf, AX_U32 nSize, AX_S16 nTimeout = -1);
    AX_S32 Recv(PCIE_CMD_TYPE_E nCmdType, AX_S16 nDevice, AX_S16 nChannel, AX_U8* pDataBuf, AX_U32 nSize, AX_S16 nTimeout = -1);

#ifdef __MASTER_DEBUG__
    AX_S32 RecvCmdRet(PCIE_CMD_TYPE_E nCmdType, AX_U32 nChannel, AX_U8* pDataBuf, AX_U32 nSize);

private:
    AX_VOID GenSimulateResult(DETECT_RESULT_T& tResult);
#endif

private:
    AX_U32 m_nSlaveCount {1};
    AX_U32 m_nAllChannelCount {0};

};