/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "OsdOptionHelper.h"
#include "CommonUtils.hpp"

AX_BOOL COsdOptionHelper::InitOnce() {
    return AX_TRUE;
}

AX_BOOL COsdOptionHelper::GetOsdRect(AX_S32 Handle, std::vector<AX_IVPS_RGN_POLYGON_T>& vecRgn) {
    std::lock_guard<std::mutex> lck(m_mutex);

    auto it = m_mapOsdRect.find(Handle);
    if (m_mapOsdRect.end() == it) {
        return AX_FALSE;
    }

    vecRgn = it->second;

    return AX_TRUE;
}

AX_BOOL COsdOptionHelper::GetOsdPolygon(AX_S32 Handle, std::vector<AX_IVPS_RGN_POLYGON_T>& vecRgn) {
    std::lock_guard<std::mutex> lck(m_mutex);

    auto it = m_mapOsdPolygon.find(Handle);
    if (m_mapOsdPolygon.end() == it) {
        return AX_FALSE;
    }

    vecRgn = it->second;

    return AX_TRUE;
}

AX_BOOL COsdOptionHelper::SetOsdRect(AX_S32 Handle, const std::vector<AX_IVPS_RGN_POLYGON_T>& vecRgn) {
    std::lock_guard<std::mutex> lck(m_mutex);

    if (AX_IVPS_INVALID_REGION_HANDLE == Handle) {
        return AX_FALSE;
    }

    m_mapOsdRect[Handle].clear();
    m_mapOsdRect[Handle] = vecRgn;

    return AX_TRUE;
}

AX_BOOL COsdOptionHelper::SetOsdPolygon(AX_S32 Handle, const std::vector<AX_IVPS_RGN_POLYGON_T>& vecRgn) {
    std::lock_guard<std::mutex> lck(m_mutex);

    if (AX_IVPS_INVALID_REGION_HANDLE == Handle) {
        return AX_FALSE;
    }

    m_mapOsdPolygon[Handle].clear();
    m_mapOsdPolygon[Handle] = vecRgn;

    return AX_TRUE;
}
