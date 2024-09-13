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

#include "ax_base_type.h"
#include "ax_global_type.h"

typedef enum {
    /* SDK modules must listed at header */
    E_PPL_MOD_TYPE_VIN = AX_ID_VIN,
    E_PPL_MOD_TYPE_IVPS = AX_ID_IVPS,
    E_PPL_MOD_TYPE_VENC = AX_ID_VENC,
    E_PPL_MOD_TYPE_JENC = AX_ID_VENC,
    E_PPL_MOD_TYPE_JDEC = AX_ID_JDEC,
    E_PPL_MOD_TYPE_MJENC = AX_ID_VENC, /* E_PPL_MOD_TYPE_JENC and E_PPL_MOD_TYPE_MJENC applies same SDK module: AX_ID_JENC */
    E_PPL_MOD_TYPE_AVS = AX_ID_AVS,
    /* APP modules must listed after SDK modules */
    E_PPL_MOD_TYPE_SNS_MANAGER = AX_ID_BUTT,
    E_PPL_MOD_TYPE_DETECT,
    E_PPL_MOD_TYPE_COLLECT,
    E_PPL_MOD_TYPE_USER,
    E_PPL_MOD_TYPE_IVES,
    E_PPL_MOD_TYPE_DSP,
    E_PPL_MOD_TYPE_CAPTURE,
    E_PPL_MOD_TYPE_MAX
} PPL_MODULE_TYPE_E;

typedef struct {
    PPL_MODULE_TYPE_E eModType;
    AX_S32 nGroup;
    AX_S32 nChannel;
} IPC_MOD_INFO_T, PPL_MOD_INFO_T;

typedef struct {
    IPC_MOD_INFO_T tSrcModChn;
    IPC_MOD_INFO_T tDstModChn;
} LINK_MOD_INFO_T;

class CBaseLinkage {
public:
    CBaseLinkage(AX_VOID) = default;
    virtual ~CBaseLinkage(AX_VOID) = default;

    AX_S32 Link(const LINK_MOD_INFO_T& tLinkModInfo) const;
    AX_S32 Unlink(const LINK_MOD_INFO_T& tLinkModInfo) const;
    virtual AX_BOOL Setup() = 0;
    virtual AX_BOOL Release() = 0;
};