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
#include <functional>
#include <mutex>
#include <unordered_map>
#include "haltype.hpp"

class CLinker {
    struct hash_fn {
        std::size_t operator()(CONST AX_MOD_INFO_T& m) CONST {
            return std::hash<int>()(m.enModId) ^ std::hash<int>()(m.s32GrpId) ^ std::hash<int>()(m.s32ChnId);
        }
    };

    struct hash_equal {
        bool operator()(CONST AX_MOD_INFO_T& a, CONST AX_MOD_INFO_T& b) CONST {
            return ((a.enModId == b.enModId) && (a.s32GrpId == b.s32GrpId) && (a.s32ChnId == b.s32ChnId));
        }
    };

public:
    CLinker(AX_VOID) = default;

    AX_BOOL Link(CONST AX_MOD_INFO_T& src, CONST AX_MOD_INFO_T& dst);
    AX_BOOL Unlink(CONST AX_MOD_INFO_T& src);
    AX_BOOL Unlink(CONST AX_MOD_INFO_T& src, CONST AX_MOD_INFO_T& dst);
    AX_BOOL UnlinkAll(AX_VOID);

private:
    std::unordered_multimap<AX_MOD_INFO_T, AX_MOD_INFO_T, hash_fn, hash_equal> m_map;
    std::mutex m_mtx;
};