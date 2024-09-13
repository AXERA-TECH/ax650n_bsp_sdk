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
#include <string.h>
#include <map>
#include <mutex>
#include <vector>
#include "AXSingleton.h"
#include "ax_pool_type.h"

#define DEFAULT_META_SIZE (4096)

class CAXPoolManager : public CAXSingleton<CAXPoolManager> {
    friend class CAXSingleton<CAXPoolManager>;

public:
    /* Add block to common pool */
    AX_BOOL AddBlockToFloorPlan(AX_U32 nBlkSize, AX_U32 nBlkCount);
    AX_VOID ClearFloorPlan(AX_VOID);

    /* Create and destory common pool */
    AX_BOOL CreateFloorPlan(AX_U32 nMetaSize = DEFAULT_META_SIZE, AX_POOL_CACHE_MODE_E eCacheMode = POOL_CACHE_MODE_NONCACHE,
                            AX_BOOL bMergeMode = AX_FALSE);

    /**
     * @brief Create a private pool and push back m_arrPools
     * @note
     *   1. Recommend to destory pools at last by DestoryAllPools
     *   2. DestoryPool is not recommended to avoid pool fragment
     */
    AX_POOL CreatePool(const AX_POOL_CONFIG_T& stPoolCfg);
    AX_BOOL DestoryPool(AX_POOL& pool);

    /**
     * @brief Destory all pools including private and common pool
     */
    AX_BOOL DestoryAllPools(AX_VOID);

private:
    CAXPoolManager(AX_VOID) noexcept = default;
    virtual ~CAXPoolManager(AX_VOID) = default;

private:
    std::map<AX_U32 /* blkSize */, AX_POOL_CONFIG_T> m_mapFloorPool;
    std::vector<AX_POOL> m_arrPools;
    std::mutex m_mtx;
};