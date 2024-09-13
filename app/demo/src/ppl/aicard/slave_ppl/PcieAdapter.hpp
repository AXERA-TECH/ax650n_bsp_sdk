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

#include "ax_global_type.h"
#include "AXThread.hpp"
#include "AXSingleton.h"
#include "DetectResult.hpp"
#include "PcieOperator.hpp"
#include <vector>

#define PCIE_COMMON_MSG_CHANNEL (0)

typedef struct {
    AX_U32 nPPL {0};
    AX_U32 nVNPU {0};
    AX_BOOL bTrackEnable {AX_FALSE};
} AI_CARD_AI_SWITCH_DET_CHN_PARAM_T;

typedef struct {
    AX_CHAR szModelPath[128] {0};
    AX_S32 nChannelNum {0};
    AI_CARD_AI_SWITCH_DET_CHN_PARAM_T arrChnParam[3];
} AI_CARD_AI_SWITCH_ATTR_T;

typedef struct {
    AX_U8 nChannel;
    AX_U8 nCmdType;
    AX_U8 data[MAX_PCIE_BODY_SIZE];
    AX_U32 nSize;
} AI_CARD_SLV_PCIE_RECV_DATA_T;

class CPcieAdapter : public CAXSingleton<CPcieAdapter> {
    friend class CAXSingleton<CPcieAdapter>;
public:
    CPcieAdapter(AX_VOID) = default;
    virtual ~CPcieAdapter(AX_VOID) = default;

    AX_BOOL Init(AX_U32 nChannelCount, AX_U32 nBufferSize, AX_BOOL bSimulateDetRets = AX_FALSE, AX_S16 nTraceData = 0, AX_S16 nRetryCount = 1);
    AX_BOOL DeInit();

    AX_BOOL RecvHandShakePacket();
    AX_BOOL SendHandShakeFeedback();

    AI_CARD_SLV_PCIE_RECV_DATA_T* RecvStream(AX_U32 nChannel, AX_S16 nTimeout = -1);
    AI_CARD_SLV_PCIE_RECV_DATA_T* RecvCtrlCommand(AX_S16 nTimeout = -1);

    AX_BOOL SendDetectResult(AX_U32 nChannel, const DETECT_RESULT_T& tResult, AX_S16 nTimeout = -1);
    AX_BOOL SendCtrlResult(AX_S32 nResult, AX_S16 nTimeout = -1);

private:
    AX_BOOL GenProtocalHandShakeFeedback(AX_S32 nResult, PCIE_DATA_T &tOutPacket);
    AX_BOOL GenProtocalDetectResultData(AX_U32 nChannel, const DETECT_RESULT_T& tResult);
    AX_BOOL GenProtocalCtrlResultData(AX_S32 nResult);
    AX_U8   CheckSum8(AX_U8* pData, AX_U32 nSize);
    AX_BOOL CheckData(AX_U8* pData);

private:
    std::vector<PCIE_DATA_T> m_vecDetectResultData;
    PCIE_DATA_T m_tCtrlResultData;

    std::vector<AI_CARD_SLV_PCIE_RECV_DATA_T> m_vecRecvStreamData;

    AX_BOOL m_bAiSwitching {AX_FALSE};
};