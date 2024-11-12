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
#include <tuple>

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

class CPcieAdapter : public CAXSingleton<CPcieAdapter> {
    friend class CAXSingleton<CPcieAdapter>;
public:
    CPcieAdapter(AX_VOID) = default;
    virtual ~CPcieAdapter(AX_VOID) = default;

    AX_BOOL Init(AX_U32 nSlaveCount, AX_U32 nVideoCount, AX_U32 nBufferSize, AX_S16 nTraceData = 0, AX_S16 nRetryCount = 1);
    AX_BOOL DeInit();

    AX_BOOL SendHandShakePacket();
    AX_S32  RecvHandShakeFeedback();

    AX_BOOL SendStream(AX_U8 nUniChannel, AX_U8 nType, AX_VOID* pData, AX_U32 nSize, AX_S16 nTimeout = -1);
    AX_BOOL SendCtrlCommand(AI_CARD_AI_SWITCH_ATTR_T& tSwitchAttr, AX_S16 nTimeout = -1);
    DETECT_RESULT_T* RecvDetectorResult(AX_U32 nUniChannel, AX_S16 nTimeout = -1);
    AX_S32  RecvCtrlCmdResult(PCIE_CMD_TYPE_E eType, PCIE_DATA_T* pRecvData, AX_S16 nTimeout = -1);

private:
    std::tuple<AX_S16, AX_S16> GetDevInfo(AX_S32 nUniChannel);
    AX_BOOL GenProtocalHandShake(PCIE_DATA_T &tOutPacket);
    AX_BOOL GenProtocalStreamData(AX_U8 nChannel, AX_U8 nType, AX_VOID* pData, AX_U32 nSize);
    AX_BOOL GenProtocalCtrlData(AI_CARD_AI_SWITCH_ATTR_T& tSwitchAttr);
    AX_U8   CheckSum8(AX_U8* pData, AX_U32 nSize);
    AX_BOOL CheckData(AX_U8* pData);

private:
    AX_U16 m_nSlaveCount {0};
    std::map<AX_S16, std::tuple<AX_S16, AX_S16>> m_mapChn2DevInfo;
    std::vector<PCIE_DATA_T> m_vecSendData;
    std::vector<PCIE_DATA_T> m_vecRecvData;
    PCIE_DATA_T m_tCtrlData;
};