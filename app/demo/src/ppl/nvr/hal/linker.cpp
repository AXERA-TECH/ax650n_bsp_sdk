/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "linker.hpp"
#include "AppLogApi.h"
#include "ax_sys_api.h"

#define TAG "LINK"

AX_BOOL CLinker::Link(CONST AX_MOD_INFO_T& src, CONST AX_MOD_INFO_T& dst) {
    std::lock_guard<std::mutex> lck(m_mtx);

    hash_equal cmp;
    auto range = m_map.equal_range(src);
    for (auto it = range.first; it != range.second; ++it) {
        if (cmp(it->second, dst)) {
            LOG_M_W(TAG, "{%2d, %2d, %2d} is already linked to {%2d, %2d, %2d}", dst.enModId, dst.s32GrpId, dst.s32ChnId, src.enModId,
                    src.s32GrpId, src.s32ChnId);
            return AX_TRUE;
        }
    }

    AX_S32 ret = AX_SYS_Link(&src, &dst);
    if (0 != ret) {
        LOG_M_E(TAG, "AX_SYS_Link({%2d, %2d, %2d} -> {%2d, %2d, %2d} fail, ret = 0x%x", dst.enModId, dst.s32GrpId, dst.s32ChnId,
                src.enModId, src.s32GrpId, src.s32ChnId, ret);
        return AX_FALSE;
    }

    m_map.insert(std::make_pair(src, dst));
    LOG_M_I(TAG, "{%2d, %2d, %2d} -> {%2d, %2d, %2d} is linked", dst.enModId, dst.s32GrpId, dst.s32ChnId, src.enModId, src.s32GrpId,
            src.s32ChnId);
    return AX_TRUE;
}

AX_BOOL CLinker::Unlink(CONST AX_MOD_INFO_T& src, CONST AX_MOD_INFO_T& dst) {
    std::lock_guard<std::mutex> lck(m_mtx);

    hash_equal cmp;
    AX_S32 ret;
    auto range = m_map.equal_range(src);
    for (auto it = range.first; it != range.second; ++it) {
        if (cmp(it->second, dst)) {
            ret = AX_SYS_UnLink(&src, &dst);
            if (0 != ret) {
                LOG_M_E(TAG, "AX_SYS_UnLink({%2d, %2d, %2d} -> {%2d, %2d, %2d} fail, ret = 0x%x", dst.enModId, dst.s32GrpId, dst.s32ChnId,
                        src.enModId, src.s32GrpId, src.s32ChnId, ret);
                return AX_FALSE;
            }

            m_map.erase(it);
            LOG_M_I(TAG, "{%2d, %2d, %2d} -> {%2d, %2d, %2d} is unlinked", dst.enModId, dst.s32GrpId, dst.s32ChnId, src.enModId,
                    src.s32GrpId, src.s32ChnId);
            return AX_TRUE;
        }
    }

    LOG_M_E(TAG, "{%2d, %2d, %2d} -> {%2d, %2d, %2d} is not linked", dst.enModId, dst.s32GrpId, dst.s32ChnId, src.enModId, src.s32GrpId,
            src.s32ChnId);
    return AX_FALSE;
}

AX_BOOL CLinker::Unlink(CONST AX_MOD_INFO_T& src) {
    std::lock_guard<std::mutex> lck(m_mtx);

    if (0 == m_map.count(src)) {
        LOG_M_W(TAG, "no modules are linked to {%2d, %2d, %2d}", src.enModId, src.s32GrpId, src.s32ChnId);
        return AX_TRUE;
    }

    AX_BOOL bOprRes = {AX_TRUE};
    AX_S32 ret;
    auto range = m_map.equal_range(src);
    auto it = range.first;
    while (it != range.second) {
        CONST AX_MOD_INFO_T& dst = it->second;
        ret = AX_SYS_UnLink(&src, &dst);
        if (0 != ret) {
            LOG_M_E(TAG, "AX_SYS_UnLink({%2d, %2d, %2d} -> {%2d, %2d, %2d} fail, ret = 0x%x", dst.enModId, dst.s32GrpId, dst.s32ChnId,
                    src.enModId, src.s32GrpId, src.s32ChnId, ret);

            bOprRes = AX_FALSE;
            ++it;
        } else {
            LOG_M_I(TAG, "{%2d, %2d, %2d} -> {%2d, %2d, %2d} is unlinked", dst.enModId, dst.s32GrpId, dst.s32ChnId, src.enModId,
                    src.s32GrpId, src.s32ChnId);

            it = m_map.erase(it);
        }
    }

    return bOprRes;
}

AX_BOOL CLinker::UnlinkAll(AX_VOID) {
    std::lock_guard<std::mutex> lck(m_mtx);

    AX_BOOL bOprRes = {AX_TRUE};
    AX_S32 ret;
    auto it = m_map.begin();
    auto end = m_map.end();
    while (it != end) {
        CONST AX_MOD_INFO_T& src = it->first;
        CONST AX_MOD_INFO_T& dst = it->second;
        ret = AX_SYS_UnLink(&src, &dst);
        if (0 != ret) {
            LOG_M_E(TAG, "AX_SYS_UnLink({%2d, %2d, %2d} -> {%2d, %2d, %2d} fail, ret = 0x%x", dst.enModId, dst.s32GrpId, dst.s32ChnId,
                    src.enModId, src.s32GrpId, src.s32ChnId, ret);

            bOprRes = AX_FALSE;
            ++it;
        } else {
            LOG_M_I(TAG, "{%2d, %2d, %2d} -> {%2d, %2d, %2d} is unlinked", dst.enModId, dst.s32GrpId, dst.s32ChnId, src.enModId,
                    src.s32GrpId, src.s32ChnId);

            it = m_map.erase(it);
        }
    }

    return bOprRes;
}
