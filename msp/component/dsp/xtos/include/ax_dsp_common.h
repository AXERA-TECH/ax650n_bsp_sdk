/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_DSP_COMMON_H__
#define __AX_DSP_COMMON_H__

typedef enum {
    AX_SUCCESS = 0,
    AX_ERR_VDSP_ILLEGAL_PARAM = 0x80000001,
    AX_ERR_VDSP_INVALID_DEVID,
    AX_ERR_VDSP_INVALID_CHNID,
    AX_ERR_VDSP_EXIST,
    AX_ERR_VDSP_UNEXIST,
    AX_ERR_VDSP_NULL_PTR,
    AX_ERR_VDSP_NOT_CONFIG,
    AX_ERR_VDSP_NOT_SURPPORT,
    AX_ERR_VDSP_NOT_PERM,
    AX_ERR_VDSP_NOMEM,
    AX_ERR_VDSP_NOBUF,
    AX_ERR_VDSP_BUF_EMPTY,
    AX_ERR_VDSP_BUF_FULL,
    AX_ERR_VDSP_NOTREADY,
    AX_ERR_VDSP_BADADDR,
    AX_ERR_VDSP_BUSY,
    AX_ERR_VDSP_SYS_TIMEOUT,
    AX_ERR_VDSP_QUERY_TIMEOUT,
    AX_ERR_VDSP_OPEN_FILE,
    AX_ERR_VDSP_READ_FILE,
    AX_ERR_VDSP_INVALID_PARAM,
    AX_FAILURE,
} AX_VDSP_STS_E;

typedef struct {
    AX_U32 u32CMD;
    AX_U32 u32MsgId;
    AX_U32 u32Body[6];
} AX_DSP_MESSAGE_T;  //8 dword

typedef enum {
    AX_DSP_CMD_INIT = 0x0,
    AX_DSP_CMD_EXIT = 0x1,
    AX_DSP_CMD_SLEEP = 0x2,
    AX_DSP_CMD_POWEROFF = 0x3,
    AX_DSP_CMD_LOGLEVEL = 0x4,
    AX_DSP_CMD_FBCDC_CHANGE = 0x5,
    AX_DSP_CMD_TEST_COPY = 0x6,
    AX_DSP_CMD_TEST_GAUSSIBLUR = 0x7,
    AX_DSP_CMD_TEST_ADD = 0x8,
    AX_DSP_CMD_TEST_FBCDC = 0x9,
    AX_DSP_CMD_TEST_FBCDC_2 = 0xA,
    AX_DSP_CMD_TEST_DDRPERF = 0xB,
    AX_DSP_CMD_OPERATOR = 0x20,
    AX_DSP_CMD_OPERATOR_RESIZE,
    AX_DSP_CMD_OPERATOR_CVTCOLOR,
    AX_DSP_CMD_OPERATOR_JOINT_LR,
    AX_DSP_CMD_OPERATOR_SAD,
    AX_DSP_CMD_OPERATOR_KVM_SPLIT,
    AX_DSP_CMD_OPERATOR_KVM_COMBINE,
    AX_DSP_CMD_OPERATOR_MAP,
    AX_DSP_CMD_OPERATOR_NV12COPY,
    AX_DSP_CMD_OPERATOR_NV12Blending,
    AX_DSP_CMD_OPERATOR_COPY,
    AX_DSP_CMD_USER = 0x1000,
    AX_DSP_CMD_BUTT,
} AX_DSP_CMD_E;

#endif  //__AX_DSP_COMMON_H__
