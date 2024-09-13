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

#include "GlobalDef.h"
#include "AppLogApi.h"
#include "AXSingleton.h"
#include "AXThread.hpp"
#include "OptionHelper.h"
#include "ElapsedTimer.hpp"

#define PRINT_INTERVAL   (10)
#define MAX_CHANNEL_NUM  (64)

typedef enum _PRINT_HELPER_MOD_E
{
    E_PH_MOD_PCIE_RECV = 0,
    E_PH_MOD_PCIE_SEND,
    E_PH_MOD_VDEC,
    E_PH_MOD_MAX
} PRINT_HELPER_MOD_E;

typedef struct _PCIE_PRINT_INFO_T
{
    AX_U32 nPcieTargetFPS[MAX_CHANNEL_NUM];
    AX_U32 nPcieReceivedPackets[MAX_CHANNEL_NUM];
    AX_U32 nPciePeriodPackets[MAX_CHANNEL_NUM];
    AX_CHAR szName[64];

    _PCIE_PRINT_INFO_T(const AX_CHAR* szModName) {
        memset(this, 0, sizeof(_PCIE_PRINT_INFO_T));
        strcpy(szName, szModName);
    }

    AX_VOID Init(AX_U32 nChn, AX_U32 nTargetFPS) {
        nPcieTargetFPS[nChn] = nTargetFPS;
    }

    AX_VOID Add(AX_U32 nChn) {
        if (nChn < 0 || nChn >= MAX_CHANNEL_NUM) {
            return;
        }

        nPciePeriodPackets[nChn]++;
        nPcieReceivedPackets[nChn]++;
    }

    AX_VOID ClearAll() {
        memset(this, 0, sizeof(_PCIE_PRINT_INFO_T));
    }

    AX_VOID Print() {
        for (AX_U32 i = 0; i < MAX_CHANNEL_NUM; i++) {
            if (nPciePeriodPackets[i] > 0) {
                LOG_M_C(szName, "[%d] fps %5.2f, recv %d",
                    i,
                    nPciePeriodPackets[i] * 1.0 / PRINT_INTERVAL,
                    nPcieReceivedPackets[i]);

                nPciePeriodPackets[i] = 0;
            }
        }
    }

} PCIE_PRINT_INFO_T;


/**
 * (frame)statistics info printer
 */
class CPrintHelper final : public CAXSingleton<CPrintHelper> {
    friend class CAXSingleton<CPrintHelper>;

public:
    AX_VOID Start();
    AX_VOID Stop();
    AX_VOID Add(PRINT_HELPER_MOD_E eModType, AX_U32 nChn);

private:
    CPrintHelper(AX_VOID) = default;
    ~CPrintHelper(AX_VOID) = default;

protected:
    AX_VOID PrintThreadFunc(CPrintHelper* pCaller);

private:
    AX_BOOL m_bEnableStart {AX_FALSE};
    AX_U64  m_nTickStart {0};
    AX_U64  m_nTickEnd {0};

    std::thread m_hPrintThread;
    AX_BOOL m_bPrintThreadWorking {AX_FALSE};

    /* statistics info (pending) */
    PCIE_PRINT_INFO_T m_tPcieSend {"PCIE_SEND"};
    PCIE_PRINT_INFO_T m_tPcieRecv {"PCIE_RECV"};
};
