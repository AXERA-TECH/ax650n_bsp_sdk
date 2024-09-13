/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_DMAXOR_API_H_
#define _AX_DMAXOR_API_H_

#include "ax_base_type.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
    AX_DMAXOR_MEMORY_INIT = 4,
    AX_DMAXOR_XOR_WB,
    AX_DMAXOR_XOR_CALI,
} AX_DMA_XFER_MODE_E;

typedef struct {
    AX_U64 u64XorCali;
    AX_S32 s32Id;
    AX_U32 u32Stat;
} AX_DMAXOR_XFER_STAT_T;

/** @brief transmit information for AX_DMAXOR_MEMORY_INIT */
typedef struct {
    AX_U32 u32InitVal;
    AX_U32 u32Size;
    AX_U64 u64PhyDst;
} AX_DMAXOR_DESC_MEM_T;

/** @brief transmit information for AX_DMAXOR_XOR_WB/AX_DMAXOR_XOR_CALI */
typedef struct {
    AX_U64 u64PhySrcBuf[16];
    AX_U64 u64PhyDst;
    AX_U32 u32Size;
    AX_U8  u8SrcCnt;
} AX_DMAXOR_DESC_XOR_T;

/**
 * @brief XOR transfer msg struct
 *
 * u32DescNum: The number of dma transmission nodes
 * pDescBuf: dma transmission nodes, type is related to eDmaMode, and assigned
 *           (AX_DMAXOR_DESC_MEM_T *) or (AX_DMAXOR_DESC_XOR_T *)
 * pfnCallBack: a call back func for dma transfer done,
 *             The first parameter save the dma transfer result
 *             The second parameter id pCbArg, customized by user
 * pCbArg: The second parameter of pfnCallBack
 * eDmaMode: dma transfer mode(AX_DMA_XFER_MODE_E)
 */
typedef struct {
    AX_U32 u32DescNum;
    AX_VOID *pDescBuf;
    AX_VOID(*pfnCallBack)(AX_DMAXOR_XFER_STAT_T *, AX_VOID *);
    AX_VOID *pCbArg;
    AX_DMA_XFER_MODE_E eDmaMode;
} AX_DMAXOR_MSG_T;

AX_S32 AX_DMAXOR_Open(AX_BOOL bSync);
AX_S32 AX_DMAXOR_Cfg(AX_S32 s32DmaChn, AX_DMAXOR_MSG_T *pDmaMsg);
AX_S32 AX_DMAXOR_Start(AX_S32 s32DmaChn, AX_S32 s32Id);
AX_S32 AX_DMAXOR_Waitdone(AX_S32 s32DmaChn, AX_DMAXOR_XFER_STAT_T *pXferStat, AX_S32 s32Timeout);
AX_S32 AX_DMAXOR_Close(AX_S32 s32DmaChn);

enum {
    AX_DMAXOR_ERRNO_DEFAULT = 0x80,
    AX_DMAXOR_ERRNO_ENOENT = 0x82,
    AX_DMAXOR_ERRNO_EIO = 0x85,
    AX_DMAXOR_ERRNO_EFAULT = 0x8E,
    AX_DMAXOR_ERRNO_EINVAL = 0x96,
};

#define AX_ID_DMAXOR                       0x2
#define AX_ERR_DMAXOR_NOMEM                AX_DEF_ERR(AX_ID_DMA, AX_ID_DMAXOR, AX_ERR_NOMEM)
#define AX_ERR_DMAXOR_TIMEOUT              AX_DEF_ERR(AX_ID_DMA, AX_ID_DMAXOR, AX_ERR_TIMED_OUT)

#define AX_ERR_DMAXOR_EIO                  AX_DEF_ERR(AX_ID_DMA, AX_ID_DMAXOR, AX_DMAXOR_ERRNO_EIO)
#define AX_ERR_DMAXOR_ENOENT               AX_DEF_ERR(AX_ID_DMA, AX_ID_DMAXOR, AX_DMAXOR_ERRNO_ENOENT)
#define AX_ERR_DMAXOR_EFAULT               AX_DEF_ERR(AX_ID_DMA, AX_ID_DMAXOR, AX_DMAXOR_ERRNO_EFAULT)
#define AX_ERR_DMAXOR_EINVAL               AX_DEF_ERR(AX_ID_DMA, AX_ID_DMAXOR, AX_DMAXOR_ERRNO_EINVAL)

#define AX_DMAXOR_XFER_SUCCESS      1

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
