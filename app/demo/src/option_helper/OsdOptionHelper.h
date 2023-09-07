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

#include <map>
#include <vector>
#include <mutex>
#include "AXSingleton.h"
#include "AXAlgo.hpp"
#include "ax_ivps_api.h"

#define APP_OSD_RECT(Handle, vecRgn) \
        COsdOptionHelper::GetInstance()->GetOsdRect(Handle, vecRgn)
#define APP_OSD_POLYGON(Handle, vecRgn) \
            COsdOptionHelper::GetInstance()->GetOsdPolygon(Handle, vecRgn)

#define SET_APP_OSD_RECT(Handle, vecRgn) \
        COsdOptionHelper::GetInstance()->SetOsdRect(Handle, vecRgn)
#define SET_APP_OSD_POLYGON(Handle, vecRgn) \
            COsdOptionHelper::GetInstance()->SetOsdPolygon(Handle, vecRgn)

/**
 * Load configuration
 */
class COsdOptionHelper final : public CAXSingleton<COsdOptionHelper> {
    friend class CAXSingleton<COsdOptionHelper>;

public:
    AX_BOOL GetOsdRect(AX_S32 Handle, std::vector<AX_IVPS_RGN_POLYGON_T>& vecRgn);
    AX_BOOL GetOsdPolygon(AX_S32 Handle, std::vector<AX_IVPS_RGN_POLYGON_T>& vecRgn);

    AX_BOOL SetOsdRect(AX_S32 Handle, const std::vector<AX_IVPS_RGN_POLYGON_T>& vecRgn);
    AX_BOOL SetOsdPolygon(AX_S32 Handle, const std::vector<AX_IVPS_RGN_POLYGON_T>& vecRgn);

private:
    COsdOptionHelper(AX_VOID) = default;
    ~COsdOptionHelper(AX_VOID) = default;

    AX_BOOL InitOnce() override;

private:
    std::mutex m_mutex;
    std::map<AX_U8, std::vector<AX_IVPS_RGN_POLYGON_T>> m_mapOsdRect;
    std::map<AX_U8, std::vector<AX_IVPS_RGN_POLYGON_T>> m_mapOsdPolygon;
};
