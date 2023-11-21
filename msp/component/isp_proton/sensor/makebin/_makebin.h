/**********************************************************************************
 *
 * Copyright (c) 2019-2020 Beijing AXera Technology Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Beijing AXera Technology Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Beijing AXera Technology Co., Ltd.
 *
 **********************************************************************************/


#ifndef _AX_ISP_MAKE_BIN_H_
#define _AX_ISP_MAKE_BIN_H_
#include "ax_base_type.h"
#include "ax_global_type.h"
#include "ax_isp_api.h"

typedef enum _AX_ISP_PARAM_ID {
    AX_ISP_PARAM_NONE           = 0x0,
    AX_ISP_PARAM_VERSION,
    AX_ISP_PARAM_BLC,
    AX_ISP_PARAM_DPC,
    AX_ISP_PARAM_RLTM,
    AX_ISP_PARAM_DEHAZE,
    AX_ISP_PARAM_LSC,
    AX_ISP_PARAM_DEMOSAIC,
    AX_ISP_PARAM_GIC,
    AX_ISP_PARAM_CSC,
    AX_ISP_PARAM_CC,
    AX_ISP_PARAM_GAMMA,
    AX_ISP_PARAM_YNR,
    AX_ISP_PARAM_CNR,
    AX_ISP_PARAM_YCRT,
    AX_ISP_PARAM_SCM,
    AX_ISP_PARAM_SHARPEN,
    AX_ISP_PARAM_YCPROC,
    AX_ISP_PARAM_CCMP,
    AX_ISP_PARAM_MDE,
    AX_ISP_PARAM_AYNR,
    AX_ISP_PARAM_ACNR,
    AX_ISP_PARAM_AINR,
    AX_ISP_PARAM_AE,
    AX_ISP_PARAM_AWB,
    AX_ISP_PARAM_WBC,
    AX_ISP_PARAM_DEPURPLE,
    AX_ISP_PARAM_HDR,
    AX_ISP_PARAM_RAW3DNR,
    AX_ISP_PARAM_3DLUT,
    AX_ISP_PARAM_CA,
    AX_ISP_PARAM_DEPWL,
    AX_ISP_PARAM_AICE,
    AX_ISP_PARAM_SCENE,
    AX_ISP_PARAM_LDC,
    AX_ISP_PARAM_DIS,
    AX_ISP_PARAM_ME,
} AX_ISP_PARAM_ID;

typedef struct _ax_isp_iq_param_t {
    AX_ISP_PARAM_ID param_id;
    AX_VOID *param_buf;
} ax_isp_iq_param_t;

#endif
