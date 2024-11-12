/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_SYS_API_
#define _AX_SYS_API_
#include "ax_global_type.h"
#include "ax_base_type.h"
#include "ax_pool_type.h"

/* flags */
#define AX_MEM_CACHED (1 << 1)    /* alloc mem is cached */
#define AX_MEM_NONCACHED (1 << 2) /* alloc mem is not cached */

typedef struct {
    AX_U64 PhysAddr;
    AX_U32 SizeKB;
    AX_S8  Name[AX_MAX_PARTITION_NAME_LEN];
} AX_PARTITION_INFO_T;

typedef struct {
    AX_U32 PartitionCnt;/* range:1~AX_MAX_PARTITION_COUNT */
    AX_PARTITION_INFO_T PartitionInfo[AX_MAX_PARTITION_COUNT];
} AX_CMM_PARTITION_INFO_T;

typedef struct {
    AX_U32 TotalSize;
    AX_U32 RemainSize;
    AX_U32 BlockCnt;
    AX_CMM_PARTITION_INFO_T Partition;
} AX_CMM_STATUS_T;

/* error code define */
#define AX_ERR_CMM_ILLEGAL_PARAM    AX_DEF_ERR(AX_ID_SYS, 0x00, AX_ERR_ILLEGAL_PARAM)  //0x800B000A
#define AX_ERR_CMM_NULL_PTR         AX_DEF_ERR(AX_ID_SYS, 0x00, AX_ERR_NULL_PTR)       //0x800B000B
#define AX_ERR_CMM_NOTREADY         AX_DEF_ERR(AX_ID_SYS, 0x00, AX_ERR_SYS_NOTREADY)   //0x800B0010
#define AX_ERR_CMM_NOMEM            AX_DEF_ERR(AX_ID_SYS, 0x00, AX_ERR_NOMEM)          //0x800B0018
#define AX_ERR_CMM_UNKNOWN          AX_DEF_ERR(AX_ID_SYS, 0x00, AX_ERR_UNKNOWN)        //0x800B0029
#define AX_ERR_CMM_MMAP_FAIL        AX_DEF_ERR(AX_ID_SYS, 0x00, 0x80)                  //0x800B0080
#define AX_ERR_CMM_MUNMAP_FAIL      AX_DEF_ERR(AX_ID_SYS, 0x00, 0x81)                  //0x800B0081
#define AX_ERR_CMM_FREE_FAIL        AX_DEF_ERR(AX_ID_SYS, 0x00, 0x82)                  //0x800B0082

#define AX_ERR_PTS_ILLEGAL_PARAM    AX_DEF_ERR(AX_ID_SYS, 0x02, AX_ERR_ILLEGAL_PARAM)  //0x800B020A
#define AX_ERR_PTS_NULL_PTR         AX_DEF_ERR(AX_ID_SYS, 0x02, AX_ERR_NULL_PTR)       //0x800B020B
#define AX_ERR_PTS_NOTREADY         AX_DEF_ERR(AX_ID_SYS, 0x02, AX_ERR_SYS_NOTREADY)   //0x800B0210
#define AX_ERR_PTS_NOT_PERM         AX_DEF_ERR(AX_ID_SYS, 0x02, AX_ERR_NOT_PERM)       //0x800B0215
#define AX_ERR_PTS_UNKNOWN          AX_DEF_ERR(AX_ID_SYS, 0x02, AX_ERR_UNKNOWN)        //0x800B0229
#define AX_ERR_PTS_COPY_TO_USER     AX_DEF_ERR(AX_ID_SYS, 0x02, 0x82)                  //0x800B0282
#define AX_ERR_PTS_COPY_FROM_USER   AX_DEF_ERR(AX_ID_SYS, 0x02, 0x83)                  //0x800B0283

#define AX_ERR_LINK_ILLEGAL_PARAM   AX_DEF_ERR(AX_ID_SYS, 0x03, AX_ERR_ILLEGAL_PARAM)  //0x800B030A
#define AX_ERR_LINK_NULL_PTR        AX_DEF_ERR(AX_ID_SYS, 0x03, AX_ERR_NULL_PTR)       //0x800B030B
#define AX_ERR_LINK_NOTREADY        AX_DEF_ERR(AX_ID_SYS, 0x03, AX_ERR_SYS_NOTREADY)   //0x800B0310
#define AX_ERR_LINK_NOT_SUPPORT     AX_DEF_ERR(AX_ID_SYS, 0x03, AX_ERR_NOT_SUPPORT)    //0x800B0314
#define AX_ERR_LINK_NOT_PERM        AX_DEF_ERR(AX_ID_SYS, 0x03, AX_ERR_NOT_PERM)       //0x800B0315
#define AX_ERR_LINK_UNEXIST         AX_DEF_ERR(AX_ID_SYS, 0x03, AX_ERR_UNEXIST)        //0x800B0317
#define AX_ERR_LINK_UNKNOWN         AX_DEF_ERR(AX_ID_SYS, 0x03, AX_ERR_UNKNOWN)        //0x800B0329
#define AX_ERR_LINK_TABLE_FULL      AX_DEF_ERR(AX_ID_SYS, 0x03, 0x80)                  //0x800B0380
#define AX_ERR_LINK_TABLE_EMPTY     AX_DEF_ERR(AX_ID_SYS, 0x03, 0x81)                  //0x800B0381
#define AX_ERR_LINK_COPY_TO_USER    AX_DEF_ERR(AX_ID_SYS, 0x03, 0x82)                  //0x800B0382
#define AX_ERR_LINK_COPY_FROM_USER  AX_DEF_ERR(AX_ID_SYS, 0x03, 0x83)                  //0x800B0383



#ifdef __cplusplus
extern "C"
{
#endif

AX_S32 AX_SYS_Init(AX_VOID);
AX_S32 AX_SYS_Deinit(AX_VOID);

/* CMM API */
AX_S32 AX_SYS_MemAlloc(AX_U64 *phyaddr, AX_VOID **pviraddr, AX_U32 size, AX_U32 align, const AX_S8 *token);
AX_S32 AX_SYS_MemAllocCached(AX_U64 *phyaddr, AX_VOID **pviraddr, AX_U32 size, AX_U32 align, const AX_S8 *token);
AX_S32 AX_SYS_MemFree(AX_U64 phyaddr, AX_VOID *pviraddr);
AX_VOID *AX_SYS_Mmap(AX_U64 phyaddr, AX_U32 size);
AX_VOID *AX_SYS_MmapCache(AX_U64 phyaddr, AX_U32 size);
AX_VOID *AX_SYS_MmapFast(AX_U64 phyaddr, AX_U32 size);
AX_VOID *AX_SYS_MmapCacheFast(AX_U64 phyaddr, AX_U32 size);
AX_S32 AX_SYS_Munmap(AX_VOID *pviraddr, AX_U32 size);
AX_S32 AX_SYS_MflushCache(AX_U64 phyaddr, AX_VOID *pviraddr, AX_U32 size);
AX_S32 AX_SYS_MinvalidateCache(AX_U64 phyaddr, AX_VOID *pviraddr, AX_U32 size);
AX_S32 AX_SYS_MemGetBlockInfoByPhy(AX_U64 phyaddr, AX_S32 *pmemType, AX_VOID **pviraddr, AX_U32 *pblockSize);
AX_S32 AX_SYS_MemGetBlockInfoByVirt(AX_VOID *pviraddr, AX_U64 *phyaddr, AX_S32 *pmemType);
AX_S32 AX_SYS_MemGetPartitionInfo(AX_CMM_PARTITION_INFO_T *pCmmPartitionInfo);
AX_S32 AX_SYS_MemSetConfig(const AX_MOD_INFO_T *pModInfo, const AX_S8 *pPartitionName);
AX_S32 AX_SYS_MemGetConfig(const AX_MOD_INFO_T *pModInfo, AX_S8 *pPartitionName);
AX_S32 AX_SYS_MemQueryStatus(AX_CMM_STATUS_T *pCmmStatus);

/* LINK API*/
AX_S32 AX_SYS_Link(const AX_MOD_INFO_T *pSrc, const AX_MOD_INFO_T *pDest);
AX_S32 AX_SYS_UnLink(const AX_MOD_INFO_T *pSrc, const AX_MOD_INFO_T *pDest);
AX_S32 AX_SYS_GetLinkByDest(const AX_MOD_INFO_T *pDest, AX_MOD_INFO_T *pSrc);
AX_S32 AX_SYS_GetLinkBySrc(const AX_MOD_INFO_T *pSrc, AX_LINK_DEST_T *pLinkDest);

/* POOL API */
AX_S32 AX_POOL_SetConfig(const AX_POOL_FLOORPLAN_T *pPoolFloorPlan);
AX_S32 AX_POOL_GetConfig (AX_POOL_FLOORPLAN_T *pPoolFloorPlan);
AX_S32 AX_POOL_Init(AX_VOID);
AX_S32 AX_POOL_Exit(AX_VOID);
AX_POOL AX_POOL_CreatePool(AX_POOL_CONFIG_T *pPoolConfig);
AX_S32 AX_POOL_DestroyPool(AX_POOL PoolId);
AX_BLK AX_POOL_GetBlock(AX_POOL PoolId, AX_U64 BlkSize, const AX_S8 *pPartitionName);
AX_S32 AX_POOL_ReleaseBlock(AX_BLK BlockId);
AX_BLK AX_POOL_PhysAddr2Handle(AX_U64 PhysAddr);
AX_U64 AX_POOL_Handle2PhysAddr(AX_BLK BlockId);
AX_U64 AX_POOL_Handle2MetaPhysAddr(AX_BLK BlockId);
AX_POOL AX_POOL_Handle2PoolId(AX_BLK BlockId);
AX_U64 AX_POOL_Handle2BlkSize(AX_BLK BlockId);
AX_S32 AX_POOL_MmapPool(AX_POOL PoolId);
AX_S32 AX_POOL_MunmapPool(AX_POOL PoolId);
AX_VOID *AX_POOL_GetBlockVirAddr(AX_BLK BlockId);
AX_VOID *AX_POOL_GetMetaVirAddr(AX_BLK BlockId);
AX_S32 AX_POOL_IncreaseRefCnt(AX_BLK BlockId);
AX_S32 AX_POOL_DecreaseRefCnt(AX_BLK BlockId);

/* PTS API */
AX_S32 AX_SYS_GetCurPTS(AX_U64 *pu64CurPTS);
AX_S32 AX_SYS_InitPTSBase(AX_U64 u64PTSBase);
AX_S32 AX_SYS_SyncPTS(AX_U64 u64PTSBase);

/* LOG API */
AX_S32 AX_SYS_SetLogLevel(AX_LOG_LEVEL_E target);
AX_S32 AX_SYS_SetLogTarget(AX_LOG_TARGET_E target);
AX_S32 AX_SYS_EnableTimestamp(AX_BOOL enable);

#ifdef __cplusplus
}
#endif

#endif //_AX_SYS_API_
