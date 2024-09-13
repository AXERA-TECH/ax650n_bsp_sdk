/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "TimestampHelper.hpp"
#include <stdio.h>
#include "AppLogApi.h"
#include "ax_sys_api.h"
#include "make_unique.hpp"
#define TAG "TIMESTAMP"

/* SDK VB debug internal APIs */
extern "C" AX_S32 AX_POOL_RecordTimeStamp(AX_BLK BlockId, AX_U8 TsIndex);
extern "C" AX_S32 AX_POOL_RecordTimeStampEx(AX_BLK BlockId, AX_U8 TsIndex, AX_U64 TimeStamp);
extern "C" AX_S32 AX_POOL_ClearTimeStamp(AX_BLK BlockId);

extern "C" AX_S32 AX_SYS_AllocPTSBuffer(AX_MOD_INFO_T *pModInfo, AX_U32 BufNum);
extern "C" AX_S32 AX_SYS_FreePTSBuffer(AX_MOD_INFO_T *pModInfo);
extern "C" AX_S32 AX_SYS_RecordPTS(AX_MOD_INFO_T *pModInfo, AX_U64 FrameId, AX_U64 SeqNum, AX_BLK BlockId, AX_U32 Index, AX_U64 TimeStamp);

AX_U32 CTimestampHelper::m_nBufNum = {0};

CTimestampHelper::CTimestampHelper(AX_U32 nCapacity /* = 100 */) : m_nCapacity((nCapacity > 0xFF) ? 0xFF : nCapacity) {
    m_mtx = std::make_unique<std::mutex>();
}

AX_VOID CTimestampHelper::Reset(AX_VOID) {
    std::lock_guard<std::mutex> lck(*m_mtx);

    m_nIndex = 0;
    m_nSeqId = 0;
    m_map.clear();
}

AX_U64 CTimestampHelper::MakePrivateData(AX_S32 nGrpId, AX_U64 &nTimeStamp) {
    std::lock_guard<std::mutex> lck(*m_mtx);

    if (++m_nIndex > m_nCapacity) {
        m_nIndex = 1;
    }

    AX_U64 nPrivData = MAKE_FRAME_PRIVATE_DATA(++m_nSeqId, m_nIndex, nGrpId, DUMMY_CHN_ID);

    AX_SYS_GetCurPTS(&nTimeStamp);
    m_map[m_nIndex] = {nPrivData, nTimeStamp};

    return nPrivData;
}

AX_U64 CTimestampHelper::LoadTimeStamp(AX_U64 nPrivData) {
    std::lock_guard<std::mutex> lck(*m_mtx);

    AX_U32 nIndex = FRAME_PRIVATE_APPID(nPrivData);
    auto it = m_map.find(nIndex);
    return (it != m_map.end()) ? m_map[nIndex].timeStamp : INVALID_TIMESTAMP;
}

AX_VOID CTimestampHelper::ClearTimestamps(const AX_VIDEO_FRAME_T &stFrame) {
    AX_POOL_ClearTimeStamp(stFrame.u32BlkId[0]);
}

AX_VOID CTimestampHelper::RecordTimestamp(const AX_VIDEO_FRAME_T &stFrame, AX_S32 nGrpId, AX_S32 nChnId, AX_U32 nIndex,
                                          AX_U64 nTimestamp /* = 0 */) {
    if (0 == m_nBufNum) {
        if (nTimestamp > 0) {
            AX_POOL_RecordTimeStampEx(stFrame.u32BlkId[0], nIndex, nTimestamp);
        } else {
            AX_POOL_RecordTimeStamp(stFrame.u32BlkId[0], nIndex);
        }
    } else {
        if (nGrpId < 0 || nChnId < 0) {
            LOG_M_E(TAG, "%s: invalid grp %d or chn %d", __func__, nGrpId, nChnId);
            return;
        }

        AX_MOD_INFO_T mod = {AX_ID_USER, nGrpId, nChnId};
        AX_SYS_RecordPTS(&mod, stFrame.u64PrivateData, stFrame.u64SeqNum, stFrame.u32BlkId[0], nIndex, nTimestamp);
    }
}

AX_BOOL CTimestampHelper::AllocTimestampBuf(const AX_MOD_INFO_T &m, AX_U32 nBufNum) {
    /* all mods share the same buf num */
    m_nBufNum = nBufNum;

    if (m_nBufNum > 0) {
        AX_S32 ret = AX_SYS_AllocPTSBuffer((AX_MOD_INFO_T *)&m, nBufNum);
        if (0 != ret) {
            LOG_M_E(TAG, "vdGrp %d vdChn %d AX_SYS_AllocPTSBuffer %d buf fail, ret = 0x%x", m.s32GrpId, m.s32ChnId, nBufNum, ret);
            return AX_FALSE;
        }
    }

    return AX_TRUE;
}

AX_VOID CTimestampHelper::FreeTimestampBuf(const AX_MOD_INFO_T &m) {
    if (m_nBufNum > 0) {
        AX_SYS_FreePTSBuffer((AX_MOD_INFO_T *)&m);
    }
}