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
#include "AXRingBuffer.h"
#include "IStreamHandler.hpp"
#include "PcieAdapter.hpp"
#include <vector>


typedef struct _TRANSFER_ATTR {
    AX_S16 nSlaveCount {1};
    AX_U8 nMaxVideoCount {0};
    AX_U8 nBuffCount {2};
    AX_S16 nBuffSize {600};
    AX_S16 nSendTimeout {-1};
    AX_S16 nRecvTimeout {-1};
    AX_S16 nTraceData {0};
    AX_S16 nRetryCount {1};
} TRANSFER_ATTR_T;

typedef struct _TRANSFER_SEND_DATA {
    CAXRingBuffer* pRingBuffer {nullptr};;
    AX_U8 nChannel;
    AX_U8 nFPS;
    _TRANSFER_SEND_DATA() {
        memset(this, 0, sizeof(_TRANSFER_SEND_DATA));
    }
    ~_TRANSFER_SEND_DATA() {
        if (pRingBuffer) {
            delete pRingBuffer;
            pRingBuffer = nullptr;
        }
    }
} TRANSFER_SEND_DATA_T;

typedef struct _TRANSFER_RECV_DATA {
    CAXRingBuffer* pRingBuffer {nullptr}; // DETECT_RESULT_T involved
    AX_U8 nChannel;
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

class CTransferHelper : public IStreamObserver  {
public:
    CTransferHelper(AX_VOID) = default;
    virtual ~CTransferHelper(AX_VOID) = default;

    AX_BOOL Init(const TRANSFER_ATTR_T& stAttr);
    AX_BOOL DeInit(AX_VOID);

    AX_BOOL Start(AX_VOID);
    AX_BOOL Stop(AX_VOID);

    AX_BOOL SendAiAttr(AI_CARD_AI_SWITCH_ATTR_T& tAiAttr);

    /* stream data callback */
    AX_BOOL OnRecvVideoData(AX_S32 nChannel, const AX_U8 *pData, AX_U32 nLen, AX_U64 nPTS) override;
    AX_BOOL OnRecvAudioData(AX_S32 nChannel, const AX_U8 *pData, AX_U32 nLen, AX_U64 nPTS) override;

protected:
    AX_BOOL HandShake(AX_VOID);
    AX_VOID SendStreamThread(AX_VOID* pArg);
    AX_VOID RecvDetectResultThread(AX_VOID* pArg);
    AX_VOID RecvCtrlCmdResultThread(AX_VOID* pArg);

    AX_VOID ClearBuf(AX_VOID);

protected:
    TRANSFER_ATTR_T m_tIniAttr;
    AX_BOOL m_bAiSwitching {AX_FALSE};
    std::vector<TRANSFER_SEND_DATA_T> m_vecSendStream;
    std::vector<CAXThread*> m_vecThreadSendStream;
    std::vector<CAXThread*> m_vecThreadRecvDetRet;
    std::map<AX_U32, std::tuple<AX_U32, AX_U32>> m_mapSendThreadParams;
    std::map<AX_U32, std::tuple<AX_U32, AX_U32>> m_mapRecvThreadParams;
    CAXThread m_threadRecvCmdRet;
};