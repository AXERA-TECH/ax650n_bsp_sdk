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

#include "IModule.h"
#include "IniWrapper.hpp"
#include "ax_base_type.h"
#include "ax_pool_type.h"

typedef struct _POOL_ATTR_T {
    AX_U32 nMaxWidth{0};
    AX_U32 nMaxHeight{0};
    AX_BOOL bRotatetion{AX_FALSE};
} POOL_ATTR_T;

class CPoolConfig {
public:
    CPoolConfig(POOL_ATTR_T tAttr);
    virtual ~CPoolConfig(AX_VOID) = default;

    AX_BOOL LoadConfig();
    AX_BOOL AddBlocks();
    AX_VOID MergeBlocks();
    AX_BOOL Start();
    AX_BOOL Stop();

private:
    AX_BOOL AddBlock(string strName, AX_U32 nBlkSize, AX_U32 nMetaSize, AX_U32 nBlkCount,
                     AX_POOL_CACHE_MODE_E eCacheMode = POOL_CACHE_MODE_NONCACHE);
    AX_BOOL AddPrivVinBlock(string strName, AX_U32 nBlkSize, AX_U32 nMetaSize, const AX_U32 nBlkCount,
                            AX_POOL_CACHE_MODE_E eCacheMode = POOL_CACHE_MODE_NONCACHE);
    AX_S32 InitPrivatePool();
    AX_VOID MergePrivBlocks();

private:
    CIniWrapper m_iniParser;
    POOL_ATTR_T m_tAttr;
    AX_POOL_FLOORPLAN_T m_tCommPoolFloorPlan;
    AX_POOL_FLOORPLAN_T m_tPrivPoolFloorPlan;
    AX_U32 m_nPoolCount{0};
    AX_U32 m_nPrivPoolCount{0};
};  // namespace class CPoolConfig
