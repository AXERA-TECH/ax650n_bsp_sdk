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

#include "ax_global_type.h"
#include "ax_sys_api.h"
#include "ax_skel_api.h"
#include "AXSingleton.h"

typedef struct _STAT_OBJECT_NUM_T {
    AX_U32 nBodyNum;
    AX_U32 nVehicleNum;
    AX_U32 nCycleNum;
    AX_U32 nFaceNum;
    AX_U32 nPlateNum;
} STAT_OBJECT_NUM_T;

class CStatMgr : public CAXSingleton<CStatMgr> {
    friend class CAXSingleton<CStatMgr>;

public:
    AX_VOID StatTrackMgr(const AX_SKEL_OBJECT_ITEM_T *pstObjectItems, STAT_OBJECT_NUM_T &stObjectNum);
    AX_VOID StatPushMgr(const AX_SKEL_OBJECT_ITEM_T *pstObjectItems, STAT_OBJECT_NUM_T &stObjectNum);
};
