/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "BaseLinkage.h"
#include "AppLogApi.h"
#include <string.h>
#include "ax_sys_api.h"

#define BASE_LINK "BASE_LINK"

AX_S32 CBaseLinkage::Link(const LINK_MOD_INFO_T& tLinkModInfo) const {
    AX_MOD_INFO_T tPreMode;
    memset(&tPreMode, 0, sizeof(AX_MOD_INFO_T));
    AX_MOD_INFO_T tCurMode;
    memset(&tCurMode, 0, sizeof(AX_MOD_INFO_T));

    tPreMode.enModId = (AX_MOD_ID_E)tLinkModInfo.tSrcModChn.eModType;
    tPreMode.s32GrpId = tLinkModInfo.tSrcModChn.nGroup;
    tPreMode.s32ChnId = tLinkModInfo.tSrcModChn.nChannel;
    tCurMode.enModId = (AX_MOD_ID_E)tLinkModInfo.tDstModChn.eModType;
    tCurMode.s32GrpId = tLinkModInfo.tDstModChn.nGroup;
    tCurMode.s32ChnId = tLinkModInfo.tDstModChn.nChannel;

    LOG_M_I(BASE_LINK, "Linkage setup: (%d, %d, %d) => (%d, %d, %d)", tPreMode.enModId, tPreMode.s32GrpId, tPreMode.s32ChnId, tCurMode.enModId, tCurMode.s32GrpId, tCurMode.s32ChnId);

    return AX_SYS_Link(&tPreMode, &tCurMode);
}

AX_S32 CBaseLinkage::Unlink(const LINK_MOD_INFO_T& tLinkModInfo) const {
    AX_MOD_INFO_T tPreMode;
    memset(&tPreMode, 0, sizeof(AX_MOD_INFO_T));
    AX_MOD_INFO_T tCurMode;
    memset(&tCurMode, 0, sizeof(AX_MOD_INFO_T));

    tPreMode.enModId = (AX_MOD_ID_E)tLinkModInfo.tSrcModChn.eModType;
    tPreMode.s32GrpId = tLinkModInfo.tSrcModChn.nGroup;
    tPreMode.s32ChnId = tLinkModInfo.tSrcModChn.nChannel;
    tCurMode.enModId = (AX_MOD_ID_E)tLinkModInfo.tDstModChn.eModType;
    tCurMode.s32GrpId = tLinkModInfo.tDstModChn.nGroup;
    tCurMode.s32ChnId = tLinkModInfo.tDstModChn.nChannel;

    LOG_M_I(BASE_LINK, "Linkage release: (%d, %d, %d) => (%d, %d, %d)", tPreMode.enModId, tPreMode.s32GrpId, tPreMode.s32ChnId, tCurMode.enModId, tCurMode.s32GrpId, tCurMode.s32ChnId);

    return AX_SYS_UnLink(&tPreMode, &tCurMode);
}