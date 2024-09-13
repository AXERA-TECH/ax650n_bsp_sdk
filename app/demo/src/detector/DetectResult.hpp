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
#include "ax_skel_type.h"
#include "AXAlgo.hpp"

#define MAX_DETECT_RESULT_COUNT (32)

typedef struct DETECT_RESULT_S {
    AX_U64 nSeqNum;
    AX_U32 nW;
    AX_U32 nH;
    AX_S32 nGrpId;
    AX_S32 nSnsId;

    AX_U32 nBodyCount;
    AX_APP_ALGO_HVCFP_ITEM_T stBodys[MAX_DETECT_RESULT_COUNT];
    AX_U32 nVehicleCount;
    AX_APP_ALGO_HVCFP_ITEM_T stVehicles[MAX_DETECT_RESULT_COUNT];
    AX_U32 nCycleCount;
    AX_APP_ALGO_HVCFP_ITEM_T stCycles[MAX_DETECT_RESULT_COUNT];
    AX_U32 nFaceCount;
    AX_APP_ALGO_HVCFP_ITEM_T stFaces[MAX_DETECT_RESULT_COUNT];
    AX_U32 nPlateCount;
    AX_APP_ALGO_HVCFP_ITEM_T stPlates[MAX_DETECT_RESULT_COUNT];

    DETECT_RESULT_S(AX_VOID) {
        memset(this, 0, sizeof(*this));
    }
} DETECT_RESULT_T;

/**
 * @brief
 *
 */
class CDetectResult : public CAXSingleton<CDetectResult> {
    friend class CAXSingleton<CDetectResult>;

public:
    AX_BOOL Set(AX_S32 nGrp, const DETECT_RESULT_T& hvcfp) {
        std::lock_guard<std::mutex> lck(m_mtx);
        m_mapRlts[nGrp] = hvcfp;
        return AX_TRUE;
    };

    AX_BOOL Get(AX_S32 nGrp, DETECT_RESULT_T& hvcfp) {
        std::lock_guard<std::mutex> lck(m_mtx);
        if (m_mapRlts.end() == m_mapRlts.find(nGrp)) {
            return AX_FALSE;
        }

        hvcfp = m_mapRlts[nGrp];
        return AX_TRUE;
    };

protected:
    CDetectResult(AX_VOID) noexcept = default;
    virtual ~CDetectResult(AX_VOID) = default;

private:
    std::mutex m_mtx;
    std::map<AX_S32, DETECT_RESULT_T> m_mapRlts;
};
