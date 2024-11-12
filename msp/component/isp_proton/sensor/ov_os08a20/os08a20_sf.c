/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "ax_base_type.h"
#include "ax_isp_common.h"

#include "isp_sensor_types.h"
#include "isp_sensor_internal.h"
#include "os08a20_settings.h"

#include "os08a20_reg.h"

/* default param */
#include "os08a20_sf.h"
#include "os08a20_ae_params.h"

#define OS08A20_MAX_VTS         (0xFFFF)
#define OS08A20_MAX_RATIO       (16.0f)
#define OS08A20_MIN_RATIO       (1.0f)

#define OS08A20_EXP_OFFSET_SDR           (0.4f) //unit:line
#define OS08A20_EXP_OFFSET_HDR_2STAGGER  (0.2f)

typedef struct _SNSOS08A20_SF_OBJ_T_ {
    AX_U32 hts;
    AX_U32 vs_hts;
    AX_U32 vts;
    AX_F32 sclk;
    AX_F32 line_period;
} SNSOS08A20_SF_OBJ_T;

typedef struct _OS08A20_SF_GAIN_TABLE_T_ {
    float gain;
    AX_U8 again_in;
    AX_U8 again_de;
    AX_U8 dgain_in;
    AX_U8 dgain_de;
    AX_U8 dgain_de2;
} OS08A20_SF_GAIN_TABLE_T;

/****************************************************************************
 * golbal variables  and macro definition
 ****************************************************************************/
extern SNS_STATE_OBJ *g_szOs08a20Ctx[AX_VIN_MAX_PIPE_NUM];
extern SNSOS08A20_OBJ_T sns_os08a20params[AX_VIN_MAX_PIPE_NUM];
#define SENSOR_GET_CTX(dev, pstCtx) (pstCtx = g_szOs08a20Ctx[dev])
#define SENSOR_SET_CTX(dev, pstCtx) (g_szOs08a20Ctx[dev] = pstCtx)
#define SENSOR_RESET_CTX(dev) (g_szOs08a20Ctx[dev] = NULL)

static AX_F32 nSf_AgainTable[SENSOR_MAX_GAIN_STEP];
static AX_F32 nSf_DgainTable[SENSOR_MAX_GAIN_STEP];

typedef enum _AX_SNS_SF_AE_REG_IDX_E_ {
    OS08A20_SHORT_EXP_LINE_H_IDX = 0,
    OS08A20_SHORT_EXP_LINE_L_IDX,
    OS08A20_SHORT_AGAIN_H_IDX,
    OS08A20_SHORT_AGAIN_L_IDX,
    OS08A20_SHORT_DGAIN_H_IDX,
    OS08A20_SHORT_DGAIN_L_IDX,

    OS08A20_VTS_H_IDX,
    OS08A20_VTS_L_IDX,

    OS08A20_REG_MAX_IDX,
} AX_SNS_SF_AE_REG_IDX_E;

static AX_SNS_DRV_DELAY_TABLE_T gOs08a20SfAeRegsTable[] = {
    /* regs index */          /* regs addr */                /*regs value*/        /*Delay Frame Num*/
    {OS08A20_SHORT_EXP_LINE_H_IDX, OS08A20_SHORT_EXP_LINE_H,      0,          0},
    {OS08A20_SHORT_EXP_LINE_L_IDX, OS08A20_SHORT_EXP_LINE_L,      0,          0},
    {OS08A20_SHORT_AGAIN_H_IDX,    OS08A20_SHORT_AGAIN_H,         0,          0},
    {OS08A20_SHORT_AGAIN_L_IDX,    OS08A20_SHORT_AGAIN_L,         0,          0},
    {OS08A20_SHORT_DGAIN_H_IDX,    OS08A20_SHORT_DGAIN_H,         0,          0},
    {OS08A20_SHORT_DGAIN_L_IDX,    OS08A20_SHORT_DGAIN_L,         0,          0},

    {OS08A20_VTS_H_IDX,           OS08A20_VTS_H,                 0,          0},
    {OS08A20_VTS_L_IDX,           OS08A20_VTS_L,                 0,          0},
};


const OS08A20_SF_GAIN_TABLE_T os08a20_sf_gain_table[] = {
    {1, 0x00, 0x80, 0x04, 0x0},
    {1.0625, 0x00, 0x88, 0x05, 0x40},
    {1.125, 0x00, 0x90, 0x05, 0x81},
    {1.1875, 0x00, 0x98, 0x05, 0xC1},
    {1.25, 0x00, 0xA0, 0x05, 0x2},
    {1.3125, 0x00, 0xA8, 0x05, 0x42},
    {1.375, 0x00, 0xB0, 0x05, 0x83},
    {1.4375, 0x00, 0xB8, 0x05, 0xC4},
    {1.5, 0x00, 0xC0, 0x06, 0x4},
    {1.5625, 0x00, 0xC8, 0x06, 0x45},
    {1.625, 0x00, 0xD0, 0x06, 0x85},
    {1.6875, 0x00, 0xD8, 0x06, 0xC6},
    {1.75, 0x00, 0xE0, 0x07, 0x7},
    {1.8125, 0x00, 0xE8, 0x07, 0x67},
    {1.875, 0x00, 0xF0, 0x07, 0x88},
    {1.9375, 0x00, 0xF8, 0x07, 0xC8},
    {2, 0x01, 0x00, 0x08, 0x0},
    {2.125, 0x01, 0x10, 0x09, 0x81},
    {2.25, 0x01, 0x20, 0x09, 0x2},
    {2.375, 0x01, 0x30, 0x09, 0x83},
    {2.5, 0x01, 0x40, 0x0A, 0x4},
    {2.625, 0x01, 0x50, 0x0A, 0x85},
    {2.75, 0x01, 0x60, 0x0B, 0x7},
    {2.875, 0x01, 0x70, 0x0B, 0x88},
    {3, 0x01, 0x80, 0x0C, 0x0},
    {3.125, 0x01, 0x90, 0x0D, 0x81},
    {3.25, 0x01, 0xA0, 0x0D, 0x2},
    {3.375, 0x01, 0xB0, 0x0D, 0x83},
    {3.5, 0x01, 0xC0, 0x0E, 0x4},
    {3.625, 0x01, 0xD0, 0x0E, 0x85},
    {3.75, 0x01, 0xE0, 0x0F, 0x7},
    {3.875, 0x01, 0xF0, 0x0F, 0x88},
    {4, 0x02, 0x00, 0x10, 0x0},
    {4.25, 0x02, 0x20, 0x11, 0x2},
    {4.5, 0x02, 0x40, 0x12, 0x4},
    {4.75, 0x02, 0x60, 0x13, 0x7},
    {5, 0x02, 0x80, 0x14, 0x0},
    {5.25, 0x02, 0xA0, 0x15, 0x2},
    {5.5, 0x02, 0xC0, 0x16, 0x4},
    {5.75, 0x02, 0xE0, 0x17, 0x7},
    {6, 0x03, 0x00, 0x18, 0x0},
    {6.25, 0x03, 0x20, 0x19, 0x2},
    {6.5, 0x03, 0x40, 0x1A, 0x4},
    {6.75, 0x03, 0x60, 0x1B, 0x7},
    {7, 0x03, 0x80, 0x1C, 0x0},
    {7.25, 0x03, 0xA0, 0x1D, 0x2},
    {7.5, 0x03, 0xC0, 0x1E, 0x4},
    {7.75, 0x03, 0xE0, 0x1F, 0x7},
    {8, 0x04, 0x00, 0x20, 0x0},
    {8.5, 0x04, 0x40, 0x22, 0x4},
    {9, 0x04, 0x80, 0x24, 0x0},
    {9.5, 0x04, 0xC0, 0x26, 0x4},
    {10, 0x05, 0x00, 0x28, 0x0},
    {10.5, 0x05, 0x40, 0x2A, 0x4},
    {11, 0x05, 0x80, 0x2C, 0x0},
    {11.5, 0x05, 0xC0, 0x2E, 0x4},
    {12, 0x06, 0x00, 0x30, 0x0},
    {12.5, 0x06, 0x40, 0x32, 0x4},
    {13, 0x06, 0x80, 0x34, 0x0},
    {13.5, 0x06, 0xC0, 0x36, 0x4},
    {14, 0x07, 0x00, 0x38, 0x0},
    {14.5, 0x07, 0x40, 0x3A, 0x4},
    {15, 0x07, 0x80, 0x3C, 0x0},
    {15.5, 0x07, 0xC0, 0x3E, 0x4},
    {15.99, 0X07, 0xff, 0x3F, 0xFF},
};

static AX_F32 os08a20_again2value_sf(float gain, AX_U8 *again_in, AX_U8 *again_de)
{
    AX_U32 i;
    AX_U32 count;
    AX_U32 ret_value = 0;

    if (!again_in || !again_de)
        return -1;

    count = sizeof(os08a20_sf_gain_table) / sizeof(os08a20_sf_gain_table[0]);

    for (i = 0; i < count; i++) {
        if (gain > os08a20_sf_gain_table[i].gain) {
            continue;
        } else {
            if (again_in)
                *again_in = os08a20_sf_gain_table[i].again_in;
            if (again_de)
                *again_de = os08a20_sf_gain_table[i].again_de;
            SNS_DBG("again=%f, again_in=0x%x, again_de=0x%x\n", gain, *again_in, *again_de);
            return os08a20_sf_gain_table[i].gain;
        }
    }

    return -1;
}

static AX_F32 os08a20_dgain2value_sf(float gain, AX_U8 *dgain_in, AX_U8 *dgain_de, AX_U8 *dgain_de2)
{
    AX_U32 i;
    AX_U32 count;
    AX_U32 ret_value = 0;

    if (!dgain_in || !dgain_de)
        return -1;

    count = sizeof(os08a20_sf_gain_table) / sizeof(os08a20_sf_gain_table[0]);

    for (i = 0; i < count; i++) {
        if (gain > os08a20_sf_gain_table[i].gain) {
            continue;
        } else {
            *dgain_in = os08a20_sf_gain_table[i].dgain_in;
            *dgain_de = os08a20_sf_gain_table[i].dgain_de;

            SNS_DBG("dgain=%f, dgain_in=0x%x, dgain_de=0x%x\n", gain, *dgain_in, *dgain_de);

            return os08a20_sf_gain_table[i].gain;
        }
    }

    return -1;
}


static AX_S32 os08a20_get_gain_table_sf(AX_SNS_AE_GAIN_TABLE_T *params)
{
    AX_U32 i;
    AX_S32 ret = 0;
    if (!params)
        return -1;

    params->nAgainTableSize = sizeof(os08a20_sf_gain_table) / sizeof(os08a20_sf_gain_table[0]);
    params->nDgainTableSize = sizeof(os08a20_sf_gain_table) / sizeof(os08a20_sf_gain_table[0]);

    for (i = 0; i < params->nAgainTableSize ; i++) {
        nSf_AgainTable[i] = os08a20_sf_gain_table[i].gain;
        params->pAgainTable = nSf_AgainTable;
    }

    for (i = 0; i < params->nDgainTableSize ; i++) {
        nSf_DgainTable[i] = os08a20_sf_gain_table[i].gain;
        params->pDgainTable = nSf_DgainTable;
    }

    return ret;
}


/****************************************************************************
 * exposure control external function
 ****************************************************************************/
AX_S32 os08a20_cfg_aec_param_sf(ISP_PIPE_ID nPipeId)
{
    AX_F32 vs_line_period = 0;

    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    sns_os08a20params[nPipeId].hts = os08a20_get_hts(nPipeId);
    sns_os08a20params[nPipeId].vs_hts = os08a20_get_vs_hts(nPipeId);
    sns_os08a20params[nPipeId].vts = os08a20_get_vts(nPipeId);
    sns_os08a20params[nPipeId].sclk = os08a20_get_sclk(nPipeId);

    if (IS_LINEAR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        sns_os08a20params[nPipeId].line_period = (float)sns_os08a20params[nPipeId].hts / AXSNS_DIV_0_TO_1(sns_os08a20params[nPipeId].sclk)
                * SNS_1_SECOND_UNIT_US;
    } else if (IS_2DOL_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        sns_os08a20params[nPipeId].line_period = (float)(2 * sns_os08a20params[nPipeId].hts) / AXSNS_DIV_0_TO_1(sns_os08a20params[nPipeId].sclk)
                * SNS_1_SECOND_UNIT_US;
    } else {
        // wrong hdr mode
    }

    sns_obj->ae_ctrl_param.fTimePerLine = sns_os08a20params[nPipeId].line_period;

    /* sensor again  limit*/
    sns_obj->ae_ctrl_param.sns_ae_limit.fMinAgain[HDR_LONG_FRAME_IDX] = 1.0f;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxAgain[HDR_LONG_FRAME_IDX] = 15.5;
    sns_obj->ae_ctrl_param.sns_ae_param.fAGainIncrement[HDR_LONG_FRAME_IDX] = (AX_F32)1 / 16;

    /* sensor dgain  limit*/
    sns_obj->ae_ctrl_param.sns_ae_limit.fMinDgain[HDR_LONG_FRAME_IDX] = 1.0f;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxDgain[HDR_LONG_FRAME_IDX] = 15.99;
    sns_obj->ae_ctrl_param.sns_ae_param.fDGainIncrement[HDR_LONG_FRAME_IDX] = (AX_F32)1 / 1024;

    /* sensor medium again limit*/
    sns_obj->ae_ctrl_param.sns_ae_limit.fMinAgain[HDR_MEDIUM_FRAME_IDX] = 1.0f;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxAgain[HDR_MEDIUM_FRAME_IDX] = 15.5;
    sns_obj->ae_ctrl_param.sns_ae_param.fAGainIncrement[HDR_MEDIUM_FRAME_IDX] = (AX_F32)1 / 16;

    /* sensor medium dgain limit*/
    sns_obj->ae_ctrl_param.sns_ae_limit.fMinDgain[HDR_MEDIUM_FRAME_IDX] = 1.0f;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxDgain[HDR_MEDIUM_FRAME_IDX] = 15.99;
    sns_obj->ae_ctrl_param.sns_ae_param.fDGainIncrement[HDR_MEDIUM_FRAME_IDX] = (AX_F32)1 / 1024;

    sns_obj->ae_ctrl_param.sns_ae_limit.fMinRatio = OS08A20_MIN_RATIO;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxRatio = OS08A20_MAX_RATIO;

    if (IS_LINEAR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMinIntegrationTime[HDR_LONG_FRAME_IDX] = 8;
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMaxIntegrationTime[HDR_LONG_FRAME_IDX] = sns_os08a20params[nPipeId].vts - 8;
        sns_obj->ae_ctrl_param.sns_ae_param.nIntegrationTimeIncrement[HDR_LONG_FRAME_IDX] = 1;
    } else if (IS_2DOL_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMinIntegrationTime[HDR_LONG_FRAME_IDX] = 4;
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMaxIntegrationTime[HDR_LONG_FRAME_IDX] = sns_os08a20params[nPipeId].vts - 8;
        sns_obj->ae_ctrl_param.sns_ae_param.nIntegrationTimeIncrement[HDR_LONG_FRAME_IDX] = 1;

        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMinIntegrationTime[HDR_MEDIUM_FRAME_IDX] = 4;
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX] = sns_os08a20params[nPipeId].vts - 8;
        sns_obj->ae_ctrl_param.sns_ae_param.nIntegrationTimeIncrement[HDR_MEDIUM_FRAME_IDX] = 1;
    } else {
        // wrong hdr mode
    }

    sns_obj->ae_ctrl_param.sns_ae_param.fCurFps = sns_obj->sns_mode_obj.fFrameRate;

    sns_obj->ae_ctrl_param.eSnsHcgLcgMode = AX_LCG_NOTSUPPORT_MODE;

    if (sns_obj->sns_mode_obj.eHDRMode == AX_SNS_LINEAR_MODE) {
        sns_obj->ae_ctrl_param.sns_ae_param.nIntegrationTimeOffset[HDR_LONG_FRAME_IDX] = OS08A20_EXP_OFFSET_SDR;
    } else if (sns_obj->sns_mode_obj.eHDRMode == AX_SNS_HDR_2X_MODE) {
        sns_obj->ae_ctrl_param.sns_ae_param.nIntegrationTimeOffset[HDR_LONG_FRAME_IDX] = OS08A20_EXP_OFFSET_HDR_2STAGGER;
        sns_obj->ae_ctrl_param.sns_ae_param.nIntegrationTimeOffset[HDR_MEDIUM_FRAME_IDX] = OS08A20_EXP_OFFSET_HDR_2STAGGER;
    } else {
        sns_obj->ae_ctrl_param.sns_ae_param.nIntegrationTimeOffset[HDR_LONG_FRAME_IDX] = OS08A20_EXP_OFFSET_SDR;
    }

    SNS_DBG("inttime min = %d, max = %d, line_period=%f, vts = %d \n",
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMinIntegrationTime[HDR_LONG_FRAME_IDX],
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMaxIntegrationTime[HDR_LONG_FRAME_IDX],
            sns_os08a20params[nPipeId].line_period, sns_os08a20params[nPipeId].vts);

    return SNS_SUCCESS;
}

AX_S32 os08a20_sns_update_regidx_table_sf(ISP_PIPE_ID nPipeId, AX_U8 nRegIdx, AX_U8 nRegValue)
{
    AX_S32 i = 0;
    AX_U32 nNum = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    nNum = sizeof(gOs08a20SfAeRegsTable) / sizeof(gOs08a20SfAeRegsTable[0]);
    SNS_CHECK_VALUE_RANGE_VALID(nRegIdx, 0, nNum - 1);

    sns_obj->sztRegsInfo[0].sztI2cData[nRegIdx].nData = nRegValue;

    SNS_DBG("Idx = %d, reg addr 0x%x, reg data 0x%x\n",
        nRegIdx, sns_obj->sztRegsInfo[0].sztI2cData[nRegIdx].nRegAddr, nRegValue);

    return SNS_SUCCESS;
}

AX_S32 os08a20_get_sensor_gain_table_sf(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_TABLE_T *params)
{
    AX_S32 result = 0;
    SNS_CHECK_PTR_VALID(params);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    result = os08a20_get_gain_table_sf(params);
    return result;

    return SNS_SUCCESS;
}


AX_S32 os08a20_set_again_sf(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_CFG_T *ptAGain)
{
    AX_U8 Gain_in;
    AX_U8 Gain_de;
    AX_S32 result = 0;
    AX_F32 gain_value = 0;
    AX_F32 nGainFromUser = 0;

    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);
    SNS_CHECK_PTR_VALID(ptAGain);

    /* long gain seting */
    nGainFromUser = ptAGain->fGain[HDR_LONG_FRAME_IDX];
    nGainFromUser = AXSNS_CLIP3(nGainFromUser, sns_obj->ae_ctrl_param.sns_ae_limit.fMinAgain[HDR_LONG_FRAME_IDX],
                                sns_obj->ae_ctrl_param.sns_ae_limit.fMaxAgain[HDR_LONG_FRAME_IDX]);

    gain_value = os08a20_again2value_sf(nGainFromUser, &Gain_in, &Gain_de);
    if (gain_value == -1) {
        SNS_ERR("new gain match failed \n");
    } else {
        sns_obj->ae_ctrl_param.sns_ae_param.fCurAGain[HDR_LONG_FRAME_IDX] = gain_value;
        result = os08a20_sns_update_regidx_table_sf(nPipeId, OS08A20_SHORT_AGAIN_H_IDX, (Gain_in & 0x3F));
        result |= os08a20_sns_update_regidx_table_sf(nPipeId, OS08A20_SHORT_AGAIN_L_IDX, (Gain_de & 0xFF));
        if (result != 0) {
            SNS_ERR("write hw failed %d \n", result);
            return result;
        }
    }

    return SNS_SUCCESS;
}

AX_S32 os08a20_set_dgain_sf(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_CFG_T *ptDGain)
{
    AX_U8 Gain_in;
    AX_U8 Gain_de;
    AX_U8 Gain_de2;
    AX_S32 result = 0;
    AX_F32 gain_val = 0;
    AX_F32 nGainFromUser = 0;

    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);
    SNS_CHECK_PTR_VALID(ptDGain);

    /* long frame digital gain seting */
    nGainFromUser = ptDGain->fGain[HDR_LONG_FRAME_IDX];
    nGainFromUser = AXSNS_CLIP3(nGainFromUser, sns_obj->ae_ctrl_param.sns_ae_limit.fMinDgain[HDR_LONG_FRAME_IDX],
                                sns_obj->ae_ctrl_param.sns_ae_limit.fMaxDgain[HDR_LONG_FRAME_IDX]);

    gain_val = os08a20_dgain2value_sf(nGainFromUser, &Gain_in, &Gain_de, &Gain_de2);
    if (gain_val == -1) {
        SNS_ERR("new gain match failed \n");
    } else {
        sns_obj->ae_ctrl_param.sns_ae_param.fCurDGain[HDR_LONG_FRAME_IDX] = gain_val;
        result = os08a20_sns_update_regidx_table_sf(nPipeId, OS08A20_SHORT_DGAIN_H_IDX, (Gain_in & 0x3F));
        result = os08a20_sns_update_regidx_table_sf(nPipeId, OS08A20_SHORT_DGAIN_L_IDX, (Gain_de & 0xFF));
        if (result != 0) {
            SNS_ERR("write hw failed %d \n", result);
            return result;
        }
    }

    return SNS_SUCCESS;
}


/* Calculate the max int time according to the exposure ratio */
AX_S32 os08a20_get_integration_time_range_sf(ISP_PIPE_ID nPipeId, AX_F32 fHdrRatio,
        AX_SNS_AE_INT_TIME_RANGE_T *ptIntTimeRange)
{
    AX_F32 ratio = 0.0f;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);
    SNS_CHECK_PTR_VALID(ptIntTimeRange);

    if (fabs(fHdrRatio) < EPS) {
        SNS_ERR("hdr ratio is error \n");
        return SNS_ERR_CODE_ILLEGAL_PARAMS;
    }

    if (AX_SNS_HDR_2X_MODE == sns_obj->sns_mode_obj.eHDRMode) {

        ratio = fHdrRatio;
        ratio = AXSNS_CLIP3(ratio, sns_obj->ae_ctrl_param.sns_ae_limit.fMinRatio, sns_obj->ae_ctrl_param.sns_ae_limit.fMaxRatio);
        if (fabs(ratio) <= EPS) {
            SNS_ERR("hdr ratio is error \n");
        }

        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMaxIntegrationTime[HDR_LONG_FRAME_IDX] =
            (sns_os08a20params[nPipeId].vts - 4) * ((ratio) / (1 + ratio));

        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX] =
            (sns_os08a20params[nPipeId].vts - 4) -
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMaxIntegrationTime[HDR_LONG_FRAME_IDX];

        ptIntTimeRange->nMaxIntegrationTime[HDR_LONG_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMaxIntegrationTime[HDR_LONG_FRAME_IDX];
        ptIntTimeRange->nMinIntegrationTime[HDR_LONG_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMinIntegrationTime[HDR_LONG_FRAME_IDX];
        ptIntTimeRange->nMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX];
        ptIntTimeRange->nMinIntegrationTime[HDR_MEDIUM_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMinIntegrationTime[HDR_MEDIUM_FRAME_IDX];

    } else if (AX_SNS_LINEAR_MODE == sns_obj->sns_mode_obj.eHDRMode) {

        ptIntTimeRange->nMaxIntegrationTime[HDR_LONG_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMaxIntegrationTime[HDR_LONG_FRAME_IDX];
        ptIntTimeRange->nMinIntegrationTime[HDR_LONG_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMinIntegrationTime[HDR_LONG_FRAME_IDX];

    } else {
        // do nothing
    }

    return SNS_SUCCESS;
}

AX_S32 os08a20_set_integration_time_sf(ISP_PIPE_ID nPipeId, AX_SNS_AE_SHUTTER_CFG_T *ptIntTime)
{
    AX_U8 ex_h;
    AX_U8 ex_l;
    AX_U32 ex_ival = 0;
    AX_S32 result = 0;
    AX_U32 nExpLineFromUser = 0;

    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);
    SNS_CHECK_PTR_VALID(ptIntTime);

    SNS_DBG("Exptime:%d-%d-%d-%d, Hdrratio:%f-%f-%f-%f\n",
            ptIntTime->nIntTime[0], ptIntTime->nIntTime[1], ptIntTime->nIntTime[2], ptIntTime->nIntTime[3],
            ptIntTime->fHdrRatio[0], ptIntTime->fHdrRatio[1], ptIntTime->fHdrRatio[2], ptIntTime->fHdrRatio[3]);

    /* long frame shutter seting */
    nExpLineFromUser = ptIntTime->nIntTime[HDR_LONG_FRAME_IDX];
    nExpLineFromUser = AXSNS_CLIP3(nExpLineFromUser,
                                    sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMinIntegrationTime[HDR_LONG_FRAME_IDX],
                                    sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMaxIntegrationTime[HDR_LONG_FRAME_IDX]);

    ex_ival = nExpLineFromUser;
    ex_l = REG_LOW_8BITS(ex_ival);
    ex_h = REG_HIGH_8BITS(ex_ival);
    os08a20_sns_update_regidx_table_sf(nPipeId, OS08A20_SHORT_EXP_LINE_H_IDX, ex_h);
    os08a20_sns_update_regidx_table_sf(nPipeId, OS08A20_SHORT_EXP_LINE_L_IDX, ex_l);

    sns_obj->ae_ctrl_param.sns_ae_param.nCurIntegrationTime[HDR_LONG_FRAME_IDX] = nExpLineFromUser;

    return SNS_SUCCESS;
}

AX_S32 os08a20_get_hw_exposure_params_sf(ISP_PIPE_ID nPipeId, AX_SNS_EXP_CTRL_PARAM_T *ptAeCtrlParam)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_PTR_VALID(ptAeCtrlParam);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    memcpy(ptAeCtrlParam, &sns_obj->ae_ctrl_param, sizeof(AX_SNS_EXP_CTRL_PARAM_T));
    memcpy(&ptAeCtrlParam->sns_dev_attr, &sns_obj->sns_attr_param, sizeof(AX_SNS_ATTR_T));

    return SNS_SUCCESS;
}

AX_U32 os08a20_sns_refresh_all_regs_from_tbl_sf(ISP_PIPE_ID nPipeId)
{
    AX_S32 i = 0;
    AX_U32 nNum = 0;
    AX_U32  nRegValue;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    nNum = sizeof(gOs08a20SfAeRegsTable) / sizeof(gOs08a20SfAeRegsTable[0]);

    for (i = 0; i < nNum; i++) {
        nRegValue = os08a20_reg_read(nPipeId, gOs08a20SfAeRegsTable[i].nRegAddr);
        sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr = gOs08a20SfAeRegsTable[i].nRegAddr;
        sns_obj->sztRegsInfo[0].sztI2cData[i].nData = nRegValue;

        SNS_DBG(" nRegAddr 0x%x, nRegValue 0x%x\n", gOs08a20SfAeRegsTable[i].nRegAddr, gOs08a20SfAeRegsTable[i].nRegValue);
    }

    return SNS_SUCCESS;
}

AX_S32 os08a20_sns_update_init_exposure_reg_sf(ISP_PIPE_ID nPipeId)
{
    int i = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    for (i = 0; i < sns_obj->sztRegsInfo[0].nRegNum; i++) {
        os08a20_write_register(nPipeId, sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr, sns_obj->sztRegsInfo[0].sztI2cData[i].nData);
        SNS_DBG("Idx = %d, reg addr 0x%x, reg data 0x%x\n",
            i, sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr, sns_obj->sztRegsInfo[0].sztI2cData[i].nData);
    }

    return SNS_SUCCESS;
}

AX_S32 os08a20_ae_get_sensor_reg_info_sf(ISP_PIPE_ID nPipeId, AX_SNS_REGS_CFG_TABLE_T *ptSnsRegsInfo)
{
    AX_S32 i = 0;
    AX_S32 nRet = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_BOOL bUpdateReg = AX_FALSE;

    SNS_CHECK_PTR_VALID(ptSnsRegsInfo);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if ((AX_FALSE == sns_obj->bSyncInit) || (AX_FALSE == ptSnsRegsInfo->bConfig)) {
        /* sync config */
        SNS_DBG(" bSyncInit %d, bConfig %d\n", sns_obj->bSyncInit, ptSnsRegsInfo->bConfig);
        sns_obj->sztRegsInfo[0].eSnsType = ISP_SNS_CONNECT_I2C_TYPE;
        sns_obj->sztRegsInfo[0].tComBus.I2cDev = os08a20_get_bus_num(nPipeId);
        sns_obj->sztRegsInfo[0].nRegNum = sizeof(gOs08a20SfAeRegsTable) / sizeof(gOs08a20SfAeRegsTable[0]);
        sns_obj->sztRegsInfo[0].nCfg2ValidDelayMax = 2;

        for (i = 0; i < sns_obj->sztRegsInfo[0].nRegNum; i++) {
            sns_obj->sztRegsInfo[0].sztI2cData[i].bUpdate = AX_TRUE;
            sns_obj->sztRegsInfo[0].sztI2cData[i].nDevAddr = OS08A20_SLAVE_ADDR;
            sns_obj->sztRegsInfo[0].sztI2cData[i].nAddrByteNum = OS08A20_ADDR_BYTE;
            sns_obj->sztRegsInfo[0].sztI2cData[i].nDataByteNum = OS08A20_DATA_BYTE;
            sns_obj->sztRegsInfo[0].sztI2cData[i].nDelayFrmNum = gOs08a20SfAeRegsTable[i].nDelayFrmNum;
            SNS_DBG("pipe %d, [%2d] nRegAddr 0x%x, nRegValue 0x%x\n", nPipeId, i,
                    gOs08a20SfAeRegsTable[i].nRegAddr, gOs08a20SfAeRegsTable[i].nRegValue);
        }

        bUpdateReg = AX_TRUE;
        sns_obj->bSyncInit = AX_TRUE;
        os08a20_sns_update_init_exposure_reg_sf(nPipeId);
    } else {
        for (i = 0; i < sns_obj->sztRegsInfo[0].nRegNum; i++) {
            if (sns_obj->sztRegsInfo[0].sztI2cData[i].nData == sns_obj->sztRegsInfo[1].sztI2cData[i].nData) {
                sns_obj->sztRegsInfo[0].sztI2cData[i].bUpdate = AX_FALSE;
            } else {
                sns_obj->sztRegsInfo[0].sztI2cData[i].bUpdate = AX_TRUE;
                bUpdateReg = AX_TRUE;
                SNS_DBG("pipe %d, [%2d] nRegAddr 0x%x, nRegValue 0x%x\n", nPipeId, i,
                        sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr, sns_obj->sztRegsInfo[0].sztI2cData[i].nData);
            }
        }
    }

    if (AX_TRUE == bUpdateReg) {
        sns_obj->sztRegsInfo[0].bConfig = AX_FALSE;
    } else {
        sns_obj->sztRegsInfo[0].bConfig = AX_TRUE;
    }

    memcpy(ptSnsRegsInfo, &sns_obj->sztRegsInfo[0], sizeof(AX_SNS_REGS_CFG_TABLE_T));
    /* Save the current register table */
    memcpy(&sns_obj->sztRegsInfo[1], &sns_obj->sztRegsInfo[0], sizeof(AX_SNS_REGS_CFG_TABLE_T));

    return nRet;
}

static AX_U32 os08a20_set_vts_sf(ISP_PIPE_ID nPipeId, AX_U32 vts)
{
    AX_U8 vts_h;
    AX_U8 vts_l;
    AX_S32 result = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    vts_l = vts & 0xFF;
    vts_h = (vts & 0xFF00) >> 8;

    result |= os08a20_sns_update_regidx_table_sf(nPipeId, OS08A20_VTS_H_IDX, vts_h);
    result |= os08a20_sns_update_regidx_table_sf(nPipeId, OS08A20_VTS_L_IDX, vts_l);

    return result;
}

AX_S32 os08a20_set_slow_fps_sf(ISP_PIPE_ID nPipeId, AX_F32 nFps)
{
    AX_S32 result = 0;
    AX_S32 framerate = 30;
    AX_U32 vts = 0;

    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    framerate = sns_obj->sns_mode_obj.fFrameRate;
    vts = sns_obj->sns_mode_obj.nVts;
    if (nFps >= framerate) {
        sns_os08a20params[nPipeId].vts = vts;
    } else {
        sns_os08a20params[nPipeId].vts = 1 * SNS_1_SECOND_UNIT_US / (sns_os08a20params[nPipeId].line_period * nFps);
    }

    if (sns_os08a20params[nPipeId].vts > OS08A20_MAX_VTS){
        sns_os08a20params[nPipeId].vts = OS08A20_MAX_VTS;
        nFps = 1 * SNS_1_SECOND_UNIT_US / (sns_os08a20params[nPipeId].line_period *sns_os08a20params[nPipeId].vts);
        SNS_ERR("Beyond minmum fps  %f\n",nFps);
    }
    result = os08a20_set_vts_sf(nPipeId, sns_os08a20params[nPipeId].vts);
    if (result != 0) {
        SNS_ERR("%s: write vts failed %d \n", __func__, result);
        return result;
    }

    sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMaxIntegrationTime[HDR_LONG_FRAME_IDX] = sns_os08a20params[nPipeId].vts - 8;
    sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX] = sns_os08a20params[nPipeId].vts - 8;
    sns_obj->ae_ctrl_param.sns_ae_param.fCurFps = 1 * SNS_1_SECOND_UNIT_US / (sns_os08a20params[nPipeId].line_period *sns_os08a20params[nPipeId].vts);

    SNS_DBG("nPipeId = %d, fps(from alg) = %f, current vts = 0x%x\n", nPipeId, nFps, sns_os08a20params[nPipeId].vts);

    return SNS_SUCCESS;
}

AX_S32 os08a20_set_fps_sf(ISP_PIPE_ID nPipeId, AX_F32 fFps)
{
    AX_S32 result = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if (AX_SNS_HDR_2X_MODE == sns_obj->sns_attr_param.eSnsMode) {

        if (IS_SNS_FPS_EQUAL(fFps, 30.0)) {
            sns_os08a20params[nPipeId].vts = OS08A20_VTS_10BIT_8M30_HDR_2X;
        } else if (IS_SNS_FPS_EQUAL(fFps, 25.0)) {
            sns_os08a20params[nPipeId].vts = OS08A20_VTS_10BIT_8M25_HDR_2X;
        }

    } else if (AX_SNS_LINEAR_MODE == sns_obj->sns_attr_param.eSnsMode) {

        if ( (AX_RT_RAW12 == sns_obj->sns_attr_param.eRawType) && (IS_SNS_FPS_EQUAL(fFps, 30.0)) ) {
            sns_os08a20params[nPipeId].vts = OS08A20_VTS_12BIT_8M30_SDR;
        } else if ( (AX_RT_RAW12 == sns_obj->sns_attr_param.eRawType) && (IS_SNS_FPS_EQUAL(fFps, 25.0)) ) {
            sns_os08a20params[nPipeId].vts = OS08A20_VTS_12BIT_8M25_SDR;
        } else if ( (AX_RT_RAW10 == sns_obj->sns_attr_param.eRawType) && (IS_SNS_FPS_EQUAL(fFps, 30.0)) ) {
            sns_os08a20params[nPipeId].vts = OS08A20_VTS_10BIT_8M30_SDR;
        } else if ( (AX_RT_RAW10 == sns_obj->sns_attr_param.eRawType) && (IS_SNS_FPS_EQUAL(fFps, 25.0)) ) {
            sns_os08a20params[nPipeId].vts = OS08A20_VTS_10BIT_8M25_SDR;
        }
    }

    /* set sensor vts */
    result = os08a20_set_vts_sf(nPipeId, sns_os08a20params[nPipeId].vts);
    if (result != 0) {
        SNS_ERR("%s: write vts failed %d \n", __func__, result);
        return result;
    }

    /* change fps */
    // sns_obj->sns_attr_param.fFrameRate = fFps;
    sns_obj->sns_mode_obj.nVts = sns_os08a20params[nPipeId].vts;
    sns_obj->sns_mode_obj.fFrameRate = fFps;

    /* update exposure limit parameters */
    sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMaxIntegrationTime[HDR_LONG_FRAME_IDX] = sns_os08a20params[nPipeId].vts - 8;
    sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX] = sns_os08a20params[nPipeId].vts - 8;
    sns_obj->ae_ctrl_param.sns_ae_param.fCurFps = fFps;

    SNS_DBG("nPipeId = %d, fps(from alg) = %f, current vts = 0x%x\n", nPipeId, fFps, sns_os08a20params[nPipeId].vts);

    return SNS_SUCCESS;
}

/****************************************************************************
 * Internal function definition
 ****************************************************************************/
static AX_S32 sensor_ctx_init_sf(ISP_PIPE_ID nPipeId)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_S32 ret = 0;

    SNS_DBG("os08a20 sensor_ctx_init_sf. ret = %d\n", ret);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    if (AX_NULL == sns_obj) {
        sns_obj = (SNS_STATE_OBJ *)calloc(1, sizeof(SNS_STATE_OBJ));
        if (AX_NULL == sns_obj) {
            SNS_ERR("malloc g_szOs08a20Ctx failed\r\n");
            return SNS_ERR_CODE_NOT_MEM;
        }
    }

    memset(sns_obj, 0, sizeof(SNS_STATE_OBJ));

    SENSOR_SET_CTX(nPipeId, sns_obj);

    return SNS_SUCCESS;
}

static AX_VOID sensor_ctx_exit_sf(ISP_PIPE_ID nPipeId)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SENSOR_GET_CTX(nPipeId, sns_obj);
    free(sns_obj);
    SENSOR_RESET_CTX(nPipeId);
}

/****************************************************************************
 * sensor control function
 ****************************************************************************/
static AX_S32 os08a20_get_chipid_sf(ISP_PIPE_ID nPipeId, AX_S32 *pSnsId)
{
    AX_U32 sensor_id = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    sensor_id |= os08a20_reg_read(nPipeId, 0x300A) << 16;
    sensor_id |= os08a20_reg_read(nPipeId, 0x300B) << 8;
    sensor_id |= os08a20_reg_read(nPipeId, 0x300C);

    SNS_DBG("%s: sensor os08a20 id: 0x%x\n", __func__, sensor_id);

    if (sensor_id != OS08A20_SENSOR_CHIP_ID) {
        SNS_ERR("%s: Failed to read sensor os08a20 id 0x%x\n", __func__, sensor_id);
        return SNS_ERR_CODE_ILLEGAL_PARAMS;
    }

    *pSnsId = sensor_id;

    return SNS_SUCCESS;
}

static void os08a20_init_sf(ISP_PIPE_ID nPipeId)
{
    AX_S32 nRet = 0;
    AX_S32 nImagemode = 0;
    AX_S32 nSnsId = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    if (nPipeId < 0 || (nPipeId >= AX_VIN_MAX_PIPE_NUM)) {
        return;
    }

    /* 1. contex init */
    SENSOR_GET_CTX(nPipeId, sns_obj);
    if (AX_NULL == sns_obj) {
        /* contex init */
        nRet = sensor_ctx_init_sf(nPipeId);
        if (0 != nRet) {
            SNS_ERR("sensor_ctx_init_sf failed!\n");
            return;
        } else {
            SENSOR_GET_CTX(nPipeId, sns_obj);
        }
    }

    /* 2. i2c init */
    os08a20_sensor_i2c_init(nPipeId);

    nRet = os08a20_get_chipid_sf(nPipeId, &nSnsId);
    if (nRet != SNS_SUCCESS) {
        SNS_ERR("can't find os08a20 sensor id.\r\n");
    } else {
        SNS_DBG("os08a20 check chip id success.\r\n");
    }

    // /* 3. config settings  */
    nImagemode = sns_obj->eImgMode;
    os08a20_write_settings(nPipeId, nImagemode);

    /* 4. refresh ae param */
    os08a20_cfg_aec_param_sf(nPipeId);

    /* 5. refresh ae regs table */
    os08a20_sns_refresh_all_regs_from_tbl_sf(nPipeId);
    sns_obj->bSyncInit = AX_FALSE;
    sns_obj->sns_mode_obj.nVts = os08a20_get_vts(nPipeId);

    return;
}

static void os08a20_exit_sf(ISP_PIPE_ID nPipeId)
{
    if (nPipeId < 0 || (nPipeId >= AX_VIN_MAX_PIPE_NUM)) {
        return;
    }

    os08a20_sensor_i2c_exit(nPipeId);
    sensor_ctx_exit_sf(nPipeId);

    return;
}

static AX_S32 os08a20_sensor_streaming_ctrl_sf(ISP_PIPE_ID nPipeId, AX_U32 on)
{
    AX_S32 result = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    if (1 == on) {
        result = os08a20_write_register(nPipeId, 0x0100, 0x01); // stream on
        SNS_DBG("sensor stream on!\n");
    } else {
        result = os08a20_write_register(nPipeId, 0x0100, 0x00); // stream off
        SNS_DBG("sensor stream off!\n");
    }
    if (0 != result) {
        return -1;
    }

    return SNS_SUCCESS;
}

static AX_S32 os08a20_sensor_set_mode_sf(ISP_PIPE_ID nPipeId, AX_SNS_ATTR_T *sns_mode)
{
    AX_S32 width;
    AX_S32 height;
    AX_S32 hdrmode;
    AX_RAW_TYPE_E eRawType;
    AX_F32 framerate = 30;
    AX_S32 sns_setting_index = 0;
    AX_S32 nRet = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_PTR_VALID(sns_mode);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    if (AX_NULL == sns_obj) {
        /* contex init */
        nRet = sensor_ctx_init_sf(nPipeId);
        if (0 != nRet) {
            SNS_ERR("sensor_ctx_init failed!\n");
            return SNS_ERR_CODE_INIT_FAILD;
        } else {
            SENSOR_GET_CTX(nPipeId, sns_obj);
        }
    }

    width = sns_mode->nWidth;
    height = sns_mode->nHeight;
    framerate = sns_mode->fFrameRate;
    eRawType = sns_mode->eRawType;
    hdrmode = sns_mode->eSnsMode;

    if (width == 3840 && height == 2160 && hdrmode == 1 && eRawType == AX_RT_RAW10 && IS_SNS_FPS_EQUAL(framerate, 30.0)) {
        sns_setting_index = e_OS08A20_4lane_3840x2160_Linear_10bit_30fps_1440Mbps;
    } else if (width == 3840 && height == 2160 && hdrmode == 1 && eRawType == AX_RT_RAW10 && IS_SNS_FPS_EQUAL(framerate, 25.0)) {
        sns_setting_index = e_OS08A20_4lane_3840x2160_Linear_10bit_25fps_1440Mbps;
    } else if (width == 3840 && height == 2160 && hdrmode == 1 && eRawType == AX_RT_RAW12 && IS_SNS_FPS_EQUAL(framerate, 25.0)) {
        sns_setting_index = e_OS08A20_4lane_3840x2160_Linear_12bit_25fps_1280Mbps;
    } else if (width == 3840 && height == 2160 && hdrmode == 1 && eRawType == AX_RT_RAW12 && IS_SNS_FPS_EQUAL(framerate, 30.0)) {
        sns_setting_index = e_OS08A20_4lane_3840x2160_Linear_12bit_30fps_1280Mbps;
    } else if (width == 3840 && height == 2160 && hdrmode == 1 && eRawType == AX_RT_RAW10 && IS_SNS_FPS_EQUAL(framerate, 50.0)) {
        sns_setting_index = e_OS08A20_4lane_3840x2160_Linear_10bit_50fps_1440Mbps;
    } else if (width == 3840 && height == 2160 && hdrmode == 1 && eRawType == AX_RT_RAW10 && IS_SNS_FPS_EQUAL(framerate, 60.0)) {
        sns_setting_index = e_OS08A20_4lane_3840x2160_Linear_10bit_60fps_1440Mbps;
    } else if (width == 3840 && height == 2160 && hdrmode == 2 && eRawType == AX_RT_RAW10 && IS_SNS_FPS_EQUAL(framerate, 30.0)) {
        sns_setting_index = e_OS08A20_4lane_3840x2160_hdr_10bit_30fps_1280Mbps;
    } else if (width == 3840 && height == 2160 && hdrmode == 2 && eRawType == AX_RT_RAW10 && IS_SNS_FPS_EQUAL(framerate, 25.0)) {
        sns_setting_index = e_OS08A20_4lane_3840x2160_hdr_10bit_25fps_1280Mbps;
    } else {
        SNS_ERR("%s it's not supported. [%dx%d mode=%d fps=%f] \n",
                __func__, width, height, hdrmode, framerate);
        return -1;
    }

    /* optional, Not Recommended. if nSettingIndex > 0 will take effect */
    if (sns_mode->nSettingIndex > 0) {
        sns_setting_index = sns_mode->nSettingIndex;
    }

    sns_obj->eImgMode = sns_setting_index;
    sns_obj->sns_mode_obj.eHDRMode = hdrmode;
    sns_obj->sns_mode_obj.nWidth = width;
    sns_obj->sns_mode_obj.nHeight = height;
    sns_obj->sns_mode_obj.fFrameRate = framerate;
    memcpy(&sns_obj->sns_attr_param, sns_mode, sizeof(AX_SNS_ATTR_T));

    return SNS_SUCCESS;
}

static AX_S32 os08a20_sensor_get_mode_sf(ISP_PIPE_ID nPipeId, AX_SNS_ATTR_T *pSnsMode)
{
    AX_S32 nRet = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_PTR_VALID(pSnsMode);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    if (AX_NULL == sns_obj) {
        /* contex init */
        nRet = sensor_ctx_init_sf(nPipeId);
        if (0 != nRet) {
            SNS_ERR("sensor_ctx_init_sf failed!\n");
            return -1;
        } else {
            SENSOR_GET_CTX(nPipeId, sns_obj);
        }
    }

    memcpy(pSnsMode, &sns_obj->sns_attr_param, sizeof(AX_SNS_ATTR_T));

    return SNS_SUCCESS;
}

/****************************************************************************
 * get module default parameters function
 ****************************************************************************/
static AX_S32 os08a20_get_isp_default_params_sf(ISP_PIPE_ID nPipeId, AX_SENSOR_DEFAULT_PARAM_T *ptDftParam)
{
    AX_S32 nRet = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_SNS_HDR_MODE_E nHdrmode;

    SNS_CHECK_PTR_VALID(ptDftParam);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);


    SENSOR_GET_CTX(nPipeId, sns_obj);
    if (AX_NULL == sns_obj) {
        /* contex init */
        nRet = sensor_ctx_init_sf(nPipeId);
        if (0 != nRet) {
            SNS_ERR("sensor_ctx_init_sf failed!\n");
            return SNS_ERR_CODE_INIT_FAILD;
        } else {
            SENSOR_GET_CTX(nPipeId, sns_obj);
        }
    }

    memset(ptDftParam, 0, sizeof(AX_SENSOR_DEFAULT_PARAM_T));

    nHdrmode = sns_obj->sns_mode_obj.eHDRMode;

    SNS_DBG(" current hdr mode %d \n", nHdrmode);

    ptDftParam->ptDpc           = &dpc_param_sdr;
    ptDftParam->ptBlc           = &blc_param_sdr;
    ptDftParam->ptFpn           = &fpn_param_sdr;
    ptDftParam->ptCsc           = &csc_param_sdr;
    ptDftParam->ptDemosaic      = &demosaic_param_sdr;
    ptDftParam->ptGic           = &gic_param_sdr;
    ptDftParam->ptGamma         = &gamma_param_sdr;
    ptDftParam->ptCc            = &cc_param_sdr;
    ptDftParam->ptMde           = &mde_param_sdr;
    ptDftParam->ptAYnr          = &aynr_param_sdr;
    ptDftParam->ptACnr          = &acnr_param_sdr;
    ptDftParam->ptRltm          = &rltm_param_sdr;
    ptDftParam->ptRaw3dnr       = &raw3dnr_param_sdr;
    ptDftParam->ptDepurple      = &depurple_param_sdr;
    ptDftParam->ptLsc           = &lsc_param_sdr;
    ptDftParam->ptSharpen       = &sharpen_param_sdr;
    ptDftParam->ptScm           = &scm_param_sdr;
    ptDftParam->ptYnr           = &ynr_param_sdr;
    ptDftParam->ptCnr           = &cnr_param_sdr;
    ptDftParam->ptCcmp          = &ccmp_param_sdr;
    ptDftParam->ptYcproc        = &ycproc_param_sdr;
    ptDftParam->ptYcrt          = &ycrt_param_sdr;
    ptDftParam->ptCa            = &ca_param_sdr;
    ptDftParam->ptAinr          = &ainr_param_sdr;
    ptDftParam->ptAwbDftParam   = &awb_param_sdr;
    ptDftParam->ptAeDftParam    = &ae_param_sdr;
    ptDftParam->ptAice          = &aice_param_sdr;
    ptDftParam->pt3Dlut         = &isp_3dlut_param_sdr;
    ptDftParam->ptScene        = &scene_param_sdr;
    ptDftParam->ptDehaze        = &dehaze_param_sdr;

    return SNS_SUCCESS;
}

static AX_S32 os08a20_get_isp_black_level_sf(ISP_PIPE_ID nPipeId, AX_SNS_BLACK_LEVEL_T *ptBlackLevel)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_PTR_VALID(ptBlackLevel);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    /* black level of linear mode */
    if (AX_SNS_LINEAR_MODE == sns_obj->sns_mode_obj.eHDRMode) {
        if (sns_obj->sns_attr_param.eRawType == AX_RT_RAW10) {
            ptBlackLevel->nBlackLevel[0] = 1024;  /*08a sdr 10bit 64 => u8.6 1024*/
            ptBlackLevel->nBlackLevel[1] = 1024;
            ptBlackLevel->nBlackLevel[2] = 1024;
            ptBlackLevel->nBlackLevel[3] = 1024;
        } else if (sns_obj->sns_attr_param.eRawType == AX_RT_RAW12) {
            ptBlackLevel->nBlackLevel[0] = 256;  /*08a sdr 12bit 64 => u8.6 1024*/
            ptBlackLevel->nBlackLevel[1] = 256;
            ptBlackLevel->nBlackLevel[2] = 256;
            ptBlackLevel->nBlackLevel[3] = 256;
        }
    } else {
        ptBlackLevel->nBlackLevel[0] = 1024;  /*08a hdr 10bit 64 => u8.6 1024*/
        ptBlackLevel->nBlackLevel[1] = 1024;
        ptBlackLevel->nBlackLevel[2] = 1024;
        ptBlackLevel->nBlackLevel[3] = 1024;
    }

    return SNS_SUCCESS;
}

static AX_S32 os08a20_testpattern_ctrl_sf(ISP_PIPE_ID nPipeId, AX_U32 on)
{
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    return SNS_SUCCESS;
}

/* short frame sensor object */
AX_SYS_API_PUBLIC AX_SENSOR_REGISTER_FUNC_T gSnsos08a20ObjSf = {
    /* sensor ctrl */
    .pfn_sensor_chipid                      = os08a20_get_chipid_sf,
    .pfn_sensor_init                        = os08a20_init_sf,
    .pfn_sensor_exit                        = os08a20_exit_sf,
    .pfn_sensor_reset                       = os08a20_reset,
    .pfn_sensor_streaming_ctrl              = os08a20_sensor_streaming_ctrl_sf,
    .pfn_sensor_testpattern                 = os08a20_testpattern_ctrl_sf,

    .pfn_sensor_set_mode                    = os08a20_sensor_set_mode_sf,
    .pfn_sensor_get_mode                    = os08a20_sensor_get_mode_sf,

    .pfn_sensor_set_fps                     = os08a20_set_fps_sf,

    /* communication : register read/write */
    .pfn_sensor_set_bus_info                = os08a20_set_bus_info,
    .pfn_sensor_write_register              = os08a20_write_register,
    .pfn_sensor_read_register               = os08a20_read_register,

    /* default param */
    .pfn_sensor_get_default_params          = os08a20_get_isp_default_params_sf,
    .pfn_sensor_get_isp_black_level         = os08a20_get_isp_black_level_sf,

    /* ae ctrl */
    .pfn_sensor_get_hw_exposure_params      = os08a20_get_hw_exposure_params_sf,
    .pfn_sensor_get_gain_table              = os08a20_get_sensor_gain_table_sf,
    .pfn_sensor_set_again                   = os08a20_set_again_sf,
    .pfn_sensor_set_dgain                   = os08a20_set_dgain_sf,
    .pfn_sensor_hcglcg_ctrl                 = AX_NULL,

    .pfn_sensor_set_integration_time        = os08a20_set_integration_time_sf,
    .pfn_sensor_get_integration_time_range  = os08a20_get_integration_time_range_sf,
    .pfn_sensor_set_slow_fps                = os08a20_set_slow_fps_sf,
    .pfn_sensor_get_slow_shutter_param      = AX_NULL,
    .pfn_sensor_get_sns_reg_info            = os08a20_ae_get_sensor_reg_info_sf,
    .pfn_sensor_set_wbgain                  = AX_NULL,
};