/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "istream.hpp"
#include <algorithm>
#include "AppLogApi.h"

#define TAG "STREAM"

CONST STREAM_INFO_T& CStream0::GetStreamInfo(AX_VOID) CONST {
    return m_stInfo;
}

CONST STREAM_STAT_T& CStream0::QueryStatus(AX_VOID) CONST {
    return m_stStat;
}

AX_BOOL CStream0::RegisterObserver(IStreamObserver* pObs) {
    if (!pObs) {
        LOG_M_E(TAG, "%s: observer is nil", __func__);
        return AX_FALSE;
    }

    std::lock_guard<std::mutex> lck(m_mtxObs);
    auto it = std::find(m_lstObs.begin(), m_lstObs.end(), pObs);
    if (it != m_lstObs.end()) {
        LOG_M_W(TAG, "%s: observer has been registed", __func__);
    } else {
        m_lstObs.push_back(pObs);
        LOG_M_I(TAG, "%s: regist observer %p ok", __func__, pObs);
    }

    return AX_TRUE;
}

AX_BOOL CStream0::UnRegisterObserver(IStreamObserver* pObs) {
    if (!pObs) {
        LOG_M_E(TAG, "%s: observer is nil", __func__);
        return AX_FALSE;
    }

    std::lock_guard<std::mutex> lck(m_mtxObs);
    auto it = std::find(m_lstObs.begin(), m_lstObs.end(), pObs);
    if (it != m_lstObs.end()) {
        m_lstObs.remove(pObs);
        LOG_M_I(TAG, "%s: unregist observer %p ok", __func__, pObs);
        return AX_TRUE;
    } else {
        LOG_M_E(TAG, "%s: observer %p is not registed", __func__, pObs);
        return AX_FALSE;
    }
}