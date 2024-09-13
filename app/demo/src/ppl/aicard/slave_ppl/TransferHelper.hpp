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
#include "IObserver.h"
#include "AXRingBuffer.h"
#include "PcieAdapter.hpp"
#include <vector>
#include "IStreamHandler.hpp"


typedef enum {
    RECV_DATA_TYPE_STREAM,
    RECV_DATA_TYPE_CTRL,
    RECV_DATA_TYPE_MAX
} AICARD_SLAVE_RECV_DATA_TYPE_E;

typedef struct _TRANSFER_ATTR {
    AX_U8 nMaxVideoCount {0};
    AX_U8 nBuffCount {2};
    AX_S16 nBuffSize {600};
    AX_S16 nSendTimeout {-1};
    AX_S16 nRecvTimeout {-1};
    AX_BOOL bEnableSimulateDetRets {AX_FALSE};
    AX_S16 nTraceData {0};
    AX_S16 nRetryCount {1};
} TRANSFER_ATTR_T;

typedef struct _TRANSFER_RECV_DATA {
    AICARD_SLAVE_RECV_DATA_TYPE_E eDataType;
    AX_U8 nChannel;
    CAXRingBuffer* pRingBuffer {nullptr};
    _TRANSFER_RECV_DATA() {
        memset(this, 0, sizeof(_TRANSFER_RECV_DATA));
    }
    ~_TRANSFER_RECV_DATA() {
        if (pRingBuffer) {
            delete pRingBuffer;
            pRingBuffer = nullptr;
        }
    }
} TRANSFER_RECV_DATA_T;

class CTransferHelper {
public:
    CTransferHelper(AX_VOID) = default;
    virtual ~CTransferHelper(AX_VOID) = default;

    AX_BOOL Init(const TRANSFER_ATTR_T& stAttr);
    AX_BOOL DeInit(AX_VOID);

    AX_BOOL Start(AX_VOID);
    AX_BOOL Stop(AX_VOID);

    AX_BOOL RegStreamObserver(IStreamObserver* pObs);
    AX_BOOL RegCommandObserver(IObserver* pObs);

protected:
    AX_VOID RecvThread(AX_VOID* pArg);
    AX_BOOL HandShake(AX_VOID);
    AX_VOID SendThread(AX_VOID* pArg);
    AX_VOID DispatchThread(AX_VOID* pArg);

protected:
    TRANSFER_ATTR_T m_tInitAttr;
    std::map<AX_U32, TRANSFER_RECV_DATA_T> m_mapRecvData;
    std::vector<CAXThread*> m_vecThreadRecv;
    std::vector<CAXThread*> m_vecThreadSend;
    std::vector<CAXThread*> m_vecThreadDispatch;

    std::map<AX_U32, std::tuple<AX_U32, AX_U32>> m_mapRecvThreadParams;
    std::map<AX_U32, std::tuple<AX_U32, AX_U32>> m_mapSendThreadParams;
    std::map<AX_U32, std::tuple<AX_U32, AX_U32>> m_mapDispThreadParams;

    IStreamObserver* m_pVdecObserver;
    //IObserver* m_pDetectorObserver;
    std::vector<IObserver*> m_vecCtrlObservers;
};