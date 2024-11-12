/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "AXPoolManager.hpp"
#include <algorithm>
#include "AppLog.hpp"
#include "ax_sys_api.h"
using namespace std;

AX_BOOL CAXPoolManager::AddBlockToFloorPlan(AX_U32 nBlkSize, AX_U32 nBlkCount) {
    if (0 == nBlkSize || 0 == nBlkCount) {
        LOG_E("%s: nil parameter (blkSize %d blkCnt %d)", __func__, nBlkSize, nBlkCount);
        return AX_FALSE;
    }

    lock_guard<std::mutex> lck(m_mtx);
    if (m_mapFloorPool.size() >= AX_MAX_COMM_POOLS) {
        LOG_E("%s: too many pools %d of floor plan", __func__, (AX_U32)m_mapFloorPool.size());
        return AX_FALSE;
    }

    if (m_mapFloorPool.end() != m_mapFloorPool.find(nBlkSize)) {
        m_mapFloorPool[nBlkSize].BlkCnt += nBlkCount;
    } else {
        AX_POOL_CONFIG_T stPoolCfg;
        memset(&stPoolCfg, 0, sizeof(stPoolCfg));
        stPoolCfg.BlkSize = nBlkSize;
        stPoolCfg.BlkCnt = nBlkCount;
        m_mapFloorPool[nBlkSize] = stPoolCfg;
    }

    return AX_TRUE;
}

AX_VOID CAXPoolManager::ClearFloorPlan(AX_VOID) {
    lock_guard<std::mutex> lck(m_mtx);
    m_mapFloorPool.clear();
}

AX_BOOL CAXPoolManager::CreateFloorPlan(AX_U32 nMetaSize, AX_POOL_CACHE_MODE_E eCacheMode, AX_BOOL bMergeMode) {
    AX_POOL_FLOORPLAN_T plan;
    memset(&plan, 0, sizeof(plan));

    {
        lock_guard<std::mutex> lck(m_mtx);
        if (0 == m_mapFloorPool.size()) {
            LOG_E("%s: no floor pool blk", __func__);
            return AX_FALSE;
        }

        AX_U32 i{0};
        for (const auto &kv : m_mapFloorPool) {
            plan.CommPool[i] = kv.second;
            plan.CommPool[i].MetaSize = nMetaSize;
            plan.CommPool[i].IsMergeMode = bMergeMode;
            plan.CommPool[i].CacheMode = eCacheMode;
            ++i;
        }
    }

    /*
        AX_POOL_Exit will destory all pools, if CreatePool() -> CreateFloorPlan() then private pool will be destoryed.
        AX_POOL_Exit is recommended to invoke after AX_SYS_Init()
    */
    // AX_POOL_Exit();

    AX_S32 ret = AX_POOL_SetConfig(&plan);
    if (0 != ret) {
        LOG_E("%s: AX_POOL_SetConfig() fail, ret = 0x%x", __func__, ret);
        return AX_FALSE;
    }

    ret = AX_POOL_Init();
    if (0 != ret) {
        LOG_E("%s: AX_POOL_Init() fail, ret = 0x%x", __func__, ret);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_POOL CAXPoolManager::CreatePool(const AX_POOL_CONFIG_T &stPoolCfg) {
    AX_POOL pool = AX_POOL_CreatePool((AX_POOL_CONFIG_T *)&stPoolCfg);
    if (AX_INVALID_POOLID == pool) {
        LOG_E("%s: AX_POOL_CreatePool(blkSize %d, blkCnt %d, metaSize %d) fail", __func__, stPoolCfg.BlkSize, stPoolCfg.BlkCnt,
              stPoolCfg.MetaSize);
        return AX_FALSE;
    }

    lock_guard<std::mutex> lck(m_mtx);
    m_arrPools.push_back(pool);
    return pool;
}

AX_BOOL CAXPoolManager::DestoryPool(AX_POOL &pool) {
    if (AX_INVALID_POOLID == pool) {
        return AX_FALSE;
    }

    AX_S32 ret = AX_POOL_DestroyPool(pool);
    if (0 != ret) {
        LOG_E("%s: AX_POOL_DestroyPool(pool %d) fail, ret = 0x%x\n", __func__, ret, (AX_U32)pool);
        return AX_FALSE;
    }

    lock_guard<std::mutex> lck(m_mtx);
    auto it = find(m_arrPools.begin(), m_arrPools.end(), pool);
    if (it != m_arrPools.end()) {
        *it = AX_INVALID_POOLID;
    }

    pool = AX_INVALID_POOLID;
    return AX_TRUE;
}

AX_BOOL CAXPoolManager::DestoryAllPools(AX_VOID) {
    lock_guard<std::mutex> lck(m_mtx);

    if (0 == m_arrPools.size()) {
        return AX_TRUE;
    }

    /* destory by reverse sequence (FILO) */
    reverse(m_arrPools.begin(), m_arrPools.end());

     AX_S32 ret;
    for (auto &m : m_arrPools) {
        if (AX_INVALID_POOLID != m) {
            ret = AX_POOL_DestroyPool(m);
            if (0 != ret) {
                LOG_E("%s: AX_POOL_DestroyPool(pool %d) fail, ret = 0x%x\n", __func__, m, ret);
                return AX_FALSE;
            }

            m = AX_INVALID_POOLID;
        }
    }

    m_arrPools.clear();

    ret = AX_POOL_Exit();
    if (0 != ret) {
        LOG_E("%s: AX_POOL_Exit() fail, ret = 0x%x", __func__, ret);
        return AX_FALSE;
    }

    return AX_TRUE;
}