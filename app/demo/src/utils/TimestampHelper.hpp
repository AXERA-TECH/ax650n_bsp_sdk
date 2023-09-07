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
#include <memory>
#include <mutex>
#include <unordered_map>
#include "ax_global_type.h"

#define PRIVATE_DATA_MAGIC_ID (0x7E)

#define MAGIC_BIT (56)
#define APPID_BIT (48)
#define GRPID_BIT (40)
#define CHNID_BIT (32)
#define SEQID_BIT (00)

#define DUMMY_CHN_ID (0xFF) /* CHN ID always = 0xFF */
/*************************************************************************************************************
|   MAGIC(8)  |    APP ID   |   GRP ID   |    CHN ID   |                     SEQ ID (32)                     |
|----8bits----|----8bits----|---8bits----|----8bits----|---8bits----|----8bits----|---8bits----|----8bits----|
*************************************************************************************************************/
#define MAKE_FRAME_PRIVATE_DATA(seq, app, grp, chn)                                                                                 \
    (((AX_U64)PRIVATE_DATA_MAGIC_ID << MAGIC_BIT) | ((((AX_U64)app) & 0xFF) << APPID_BIT) | ((((AX_U64)grp) & 0xFF) << GRPID_BIT) | \
     ((((AX_U64)chn) & 0xFF) << CHNID_BIT) | (((seq)&0xFFFFFFF)))

#define FRAME_PRIVATE_MAGIC(data) (AX_U8)((((AX_U64)data) >> MAGIC_BIT) & 0xFFFF)
#define FRAME_PRIVATE_APPID(data) (AX_U8)((((AX_U64)data) >> APPID_BIT) & 0xFF)
#define FRAME_PRIVATE_GRPID(data) (AX_U8)((((AX_U64)data) >> GRPID_BIT) & 0xFF)
#define FRAME_PRIVATE_CHNID(data) (AX_U8)((((AX_U64)data) >> CHNID_BIT) & 0xFF)
#define FRAME_PRIVATE_SEQID(data) (AX_U32)(((AX_U64)data) & 0xFFFFFFFF)

#define CHECK_VALID_PRIVATE_FRAME_DATA(data)      \
    ({                                            \
        AX_BOOL ret = AX_FALSE;                   \
        AX_U16 magic = FRAME_PRIVATE_MAGIC(data); \
        if (PRIVATE_DATA_MAGIC_ID == magic) {     \
            ret = AX_TRUE;                        \
        }                                         \
        ret;                                      \
    })

typedef enum {
    TIMESTAMP_VDEC_SEND = 0,
    TIMESTAMP_VDEC_RECV = 1,
    TIMESTAMP_DISP_PUSH_FIFO = 2,
    TIMESTAMP_DISP_PRE_SEND = 3,
    TIMESTAMP_DISP_POS_SEND = 4,
    TIMESTAMP_SKEL_PUSH_FIFO = 5,
    TIMESTAMP_SKEL_PRE_SEND = 6,
    TIMESTAMP_SKEL_POS_SEND = 7,
    TIMESTAMP_VB_RELEASE = 9 /* last is used for VB release */
} TIMESTAMP_TYPE_E;

#define INVALID_TIMESTAMP (0)

class CTimestampHelper {
public:
    CTimestampHelper(AX_U32 nCapacity = 100);
    AX_VOID Reset(AX_VOID);

    AX_U64 MakePrivateData(AX_S32 nGrpId, AX_U64 &nTimeStamp);
    AX_U64 LoadTimeStamp(AX_U64 nPrivData);

    static AX_VOID ClearTimestamps(const AX_VIDEO_FRAME_T &stVFrame);
    static AX_VOID RecordTimestamp(const AX_VIDEO_FRAME_T &stVFrame, AX_S32 nGrpId, AX_S32 nChnId, AX_U32 nIndex,
                                   AX_U64 nTimestamp = 0 /* 0: SDK<SYS> set current timestamp */);

    static AX_BOOL AllocTimestampBuf(const AX_MOD_INFO_T &m, AX_U32 nBufNum);
    static AX_VOID FreeTimestampBuf(const AX_MOD_INFO_T &m);

private:
    typedef struct {
        AX_U64 frameData;
        AX_U64 timeStamp;
    } FRAME_TIMESTAMP_T;

    std::unordered_map<AX_U32 /* index */, FRAME_TIMESTAMP_T> m_map;
    AX_U32 m_nIndex = {0};
    AX_U32 m_nCapacity;
    AX_U32 m_nSeqId = {0};
    static AX_U32 m_nBufNum;
    std::unique_ptr<std::mutex> m_mtx;
};