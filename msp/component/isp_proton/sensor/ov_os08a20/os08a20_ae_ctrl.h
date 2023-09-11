/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __OS08A20_AE_CTRL_H__
#define __OS08A20_AE_CTRL_H__

#include "ax_base_type.h"
#include "ax_sensor_struct.h"


AX_S32 os08a20_cfg_aec_param(ISP_PIPE_ID nPipeId);

AX_S32 os08a20_get_sensor_gain_table(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_TABLE_T *params);
AX_S32 os08a20_set_again(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_CFG_T *ptAGain);
AX_S32 os08a20_set_dgain(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_CFG_T *ptDGain);

AX_S32 os08a20_get_hw_exposure_params(ISP_PIPE_ID nPipeId, AX_SNS_EXP_CTRL_PARAM_T *ptAeCtrlParam);
AX_S32 os08a20_get_integration_time_range(ISP_PIPE_ID nPipeId, AX_F32 fHdrRatio, AX_SNS_AE_INT_TIME_RANGE_T *ptIntTimeRange);
AX_S32 os08a20_set_integration_time(ISP_PIPE_ID nPipeId, AX_SNS_AE_SHUTTER_CFG_T *ptIntTime);

AX_S32 os08a20_set_slow_fps(ISP_PIPE_ID nPipeId, AX_F32 fFps);

AX_S32 os08a20_set_fps(ISP_PIPE_ID nPipeId, AX_F32 fFps);

AX_U32 os08a20_sns_update_regs_table(ISP_PIPE_ID nPipeId, AX_U32 nRegsAddr, AX_U8 nRegsValue);
AX_S32 os08a20_ae_get_sensor_reg_info(ISP_PIPE_ID nPipeId, AX_SNS_REGS_CFG_TABLE_T *ptSnsRegsInfo);
AX_U32 os08a20_sns_refresh_all_regs_from_tbl(ISP_PIPE_ID nPipeId);
AX_S32 os08a20_get_slow_shutter_param(ISP_PIPE_ID nPipeId, AX_SNS_AE_SLOW_SHUTTER_PARAM_T *ptSlowShutterParam);


#endif  //end __OS08A20_AE_CTRL_H__
