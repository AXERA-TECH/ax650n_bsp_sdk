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
#include <stdlib.h>
#include <string.h>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stack>
#include "ax_venc_api.h"

#ifdef __PRINT_DEBUG_INFO__
#include <stdio.h>
#endif

typedef struct VIDEO_STREAM_INFO_S {
    AX_U8* pu8Addr;   /* the virtual address of stream */
    AX_U32 u32Len;    /* the length of stream */
    AX_U64 u64PTS;    /* PTS */
    AX_U64 u64SeqNum; /* sequence number of input frame */
    AX_U64 u64UserData;
    AX_PAYLOAD_TYPE_E enType; /* the type of payload*/

    VIDEO_STREAM_INFO_S(AX_VOID) {
        pu8Addr = nullptr;
        u32Len = 0;
        u64PTS = 0;
        u64SeqNum = 0;
        u64UserData = 0;
        enType = PT_BUTT;
    }

    VIDEO_STREAM_INFO_S(const AX_VENC_PACK_T& pack) {
        pu8Addr = (AX_U8*)malloc(pack.u32Len);
        memcpy(pu8Addr, pack.pu8Addr, pack.u32Len);
        u32Len = pack.u32Len;
        u64PTS = pack.u64PTS;
        u64SeqNum = pack.u64SeqNum;
        u64UserData = pack.u64UserData;
        enType = pack.enType;
    }

    AX_VOID FreeMem(AX_VOID) {
        free(pu8Addr);
        pu8Addr = nullptr;
    }
} VIDEO_STREAM_INFO_T;


#define MAX_STREAM_CONTAINER_NUM (3)
class CVideoStreamContainer {
public:
    CVideoStreamContainer(AX_U32 nCap, AX_U32 nCnt = 2) : m_nCap(nCap), m_nCnt(nCnt) {
        if (m_nCnt > MAX_STREAM_CONTAINER_NUM) {
            m_nCnt = MAX_STREAM_CONTAINER_NUM;
        }

        if (0 == m_nCap) {
            m_nCap = 30;
        }
    };

    AX_BOOL Push(const AX_VENC_PACK_T& pack, AX_S32 nTimeOut = -1);
    AX_BOOL Pop(VIDEO_STREAM_INFO_T& m, AX_S32 nTimeOut = -1);

    /* be care to flush */
    AX_VOID Flush(AX_VOID);

    AX_VOID Clear(AX_VOID);
    AX_VOID AdjustPTS(AX_U32 nAdjustPTSDiff);

protected:
    AX_U32 GetLeftCnt(AX_VOID);

private:
    std::stack<VIDEO_STREAM_INFO_T> m_stacks[MAX_STREAM_CONTAINER_NUM];
    AX_U32 m_nCap;
    AX_U32 m_nCnt;
    AX_U64 m_wIndex = {0};
    AX_U64 m_rIndex = {0};

    std::mutex m_mtx;
    std::condition_variable m_cvW;
    std::condition_variable m_cvR;

    bool m_bAbortW = {false};
    bool m_bAbortR = {false};

#ifdef __PRINT_DEBUG_INFO__
    AX_U32 m_nPopCnt = {0};
#endif
};

inline AX_U32 CVideoStreamContainer::GetLeftCnt(AX_VOID) {
    return m_nCnt - (AX_U32)(m_wIndex - m_rIndex);
}
