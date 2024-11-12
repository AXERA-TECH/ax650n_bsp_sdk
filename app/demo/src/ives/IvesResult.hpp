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
#include "AXSingleton.h"
#include "AXAlgo.hpp"

#define MAX_IVES_MD_RESULT_COUNT (16)
#define MAX_IVES_OD_RESULT_COUNT (1)
#define MAX_IVES_SCD_RESULT_COUNT (1)

typedef struct IVES_RESULT_S {
    AX_U64 nSeqNum;
    AX_U32 nW;
    AX_U32 nH;
    AX_S32 nGrpId;

    AX_U32 nMdCount;
    AX_APP_ALGO_IVES_ITEM_T stMds[MAX_IVES_MD_RESULT_COUNT];
    AX_U32 nOdCount;
    AX_APP_ALGO_IVES_ITEM_T stOds[MAX_IVES_OD_RESULT_COUNT];
    AX_U32 nScdCount;
    AX_APP_ALGO_IVES_ITEM_T stScds[MAX_IVES_SCD_RESULT_COUNT];

    IVES_RESULT_S(AX_VOID) {
        memset(this, 0, sizeof(*this));
    }
} IVES_RESULT_T;

/**
 * @brief
 *
 */
class CIvesResult : public CAXSingleton<CIvesResult> {
    friend class CAXSingleton<CIvesResult>;

public:
    AX_BOOL Set(AX_S32 nGrp, const IVES_RESULT_T& ives) {
        std::lock_guard<std::mutex> lck(m_mtx);
        m_mapRlts[nGrp] = ives;
        return AX_TRUE;
    };

    AX_BOOL Get(AX_S32 nGrp, IVES_RESULT_T& ives) {
        std::lock_guard<std::mutex> lck(m_mtx);
        if (m_mapRlts.end() == m_mapRlts.find(nGrp)) {
            return AX_FALSE;
        }

        ives = m_mapRlts[nGrp];
        return AX_TRUE;
    };

protected:
    CIvesResult(AX_VOID) noexcept = default;
    virtual ~CIvesResult(AX_VOID) = default;

private:
    std::mutex m_mtx;
    std::map<AX_S32, IVES_RESULT_T> m_mapRlts;
};
