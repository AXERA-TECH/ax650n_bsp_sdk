/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "ax_base_type.h"
#include "ax_isp_common.h"

#include "isp_sensor_types.h"
#include "isp_sensor_internal.h"

#include "os08a20_reg.h"
#include "os08a20_ae_ctrl.h"
#include "os08a20_ae_params.h"

#define OS08A20_MAX_VTS         (0xFFFF)
#define OS08A20_MAX_RATIO       (16.0f)
#define OS08A20_MIN_RATIO       (1.0f)

#define OS08A20_EXP_OFFSET_SDR           (0.4f) //unit:line
#define OS08A20_EXP_OFFSET_HDR_2STAGGER  (0.2f)

typedef struct _OS08A20_GAIN_TABLE_T_ {
    float gain;
    AX_U8 again_in;
    AX_U8 again_de;
    AX_U8 dgain_in;
    AX_U8 dgain_de;
    AX_U8 dgain_de2;
} OS08A20_GAIN_TABLE_T;


extern SNS_STATE_OBJ *g_szOs08a20Ctx[AX_VIN_MAX_PIPE_NUM];
extern SNSOS08A20_OBJ_T sns_os08a20params[AX_VIN_MAX_PIPE_NUM];
#define SENSOR_GET_CTX(dev, pstCtx) (pstCtx = g_szOs08a20Ctx[dev])

static AX_F32 nAgainTable[SENSOR_MAX_GAIN_STEP];
static AX_F32 nDgainTable[SENSOR_MAX_GAIN_STEP];

/*user config*/
static AX_F32 gFpsGear[] = {2.00, 3.00, 4.00, 5.00, 6.00, 7.00, 8.00, 9.00, 10.00,
                            11.00, 12.00, 13.00, 14.00, 15.00, 16.00, 17.00, 18.00, 19.00, 20.00,
                            21.00, 22.00, 23.00, 24.00, 25.00, 26.00, 27.00, 28.00, 29.00, 30.00,
                            31.00, 32.00, 33.00, 34.00, 35.00, 36.00, 37.00, 38.00, 39.30, 40.00,
                            41.00, 42.00, 43.00, 44.00, 45.00, 46.00, 47.00, 48.00, 49.30, 50.00,
                            51.00, 52.00, 53.00, 54.00, 55.00, 56.00, 57.00, 58.00, 59.30, 60.00
                           };

typedef enum _AX_SNS_AE_REG_IDX_E_ {
    OS08A20_LONG_EXP_LINE_H_IDX = 0,
    OS08A20_LONG_EXP_LINE_L_IDX,
    OS08A20_LONG_AGAIN_H_IDX,
    OS08A20_LONG_AGAIN_L_IDX,
    OS08A20_LONG_DGAIN_H_IDX,
    OS08A20_LONG_DGAIN_L_IDX,

    OS08A20_SHORT_EXP_LINE_H_IDX,
    OS08A20_SHORT_EXP_LINE_L_IDX,
    OS08A20_SHORT_AGAIN_H_IDX,
    OS08A20_SHORT_AGAIN_L_IDX,
    OS08A20_SHORT_DGAIN_H_IDX,
    OS08A20_SHORT_DGAIN_L_IDX,

    OS08A20_VTS_H_IDX,
    OS08A20_VTS_L_IDX,

    OS08A20_REG_MAX_IDX,
} AX_SNS_AE_REG_IDX_E;

static AX_SNS_DRV_DELAY_TABLE_T gOs08a20AeRegsTable[] = {
    /* regs index */          /* regs addr */                /*regs value*/        /*Delay Frame Num*/
    {OS08A20_LONG_EXP_LINE_H_IDX,   OS08A20_LONG_EXP_LINE_H,       0,          0},
    {OS08A20_LONG_EXP_LINE_L_IDX,   OS08A20_LONG_EXP_LINE_L,       0,          0},
    {OS08A20_LONG_AGAIN_H_IDX,      OS08A20_LONG_AGAIN_H,          0,          0},
    {OS08A20_LONG_AGAIN_L_IDX,      OS08A20_LONG_AGAIN_L,          0,          0},
    {OS08A20_LONG_DGAIN_H_IDX,      OS08A20_LONG_DGAIN_H,          0,          0},
    {OS08A20_LONG_DGAIN_L_IDX,      OS08A20_LONG_DGAIN_L,          0,          0},

    {OS08A20_SHORT_EXP_LINE_H_IDX,  OS08A20_SHORT_EXP_LINE_H,      0,          0},
    {OS08A20_SHORT_EXP_LINE_L_IDX,  OS08A20_SHORT_EXP_LINE_L,      0,          0},
    {OS08A20_SHORT_AGAIN_H_IDX,     OS08A20_SHORT_AGAIN_H,         0,          0},
    {OS08A20_SHORT_AGAIN_L_IDX,     OS08A20_SHORT_AGAIN_L,         0,          0},
    {OS08A20_SHORT_DGAIN_H_IDX,     OS08A20_SHORT_DGAIN_H,         0,          0},
    {OS08A20_SHORT_DGAIN_L_IDX,     OS08A20_SHORT_DGAIN_L,         0,          0},

    {OS08A20_VTS_H_IDX,             OS08A20_VTS_H,                 0,          0},
    {OS08A20_VTS_L_IDX,             OS08A20_VTS_L,                 0,          0},
};


const OS08A20_GAIN_TABLE_T os08a20_gain_table[] = {
    /* gain   ag_h  ag_l  dg_h  dg_l */
    {1,      0x00, 0x80, 0x04, 0x00},
    {1.0625, 0x00, 0x88, 0x04, 0x40},
    {1.125,  0x00, 0x90, 0x04, 0x80},
    {1.1875, 0x00, 0x98, 0x04, 0xC0},
    {1.25,   0x00, 0xA0, 0x05, 0x00},
    {1.3125, 0x00, 0xA8, 0x05, 0x40},
    {1.375,  0x00, 0xB0, 0x05, 0x80},
    {1.4375, 0x00, 0xB8, 0x05, 0xC0},
    {1.5,    0x00, 0xC0, 0x06, 0x00},
    {1.5625, 0x00, 0xC8, 0x06, 0x40},
    {1.625,  0x00, 0xD0, 0x06, 0x80},
    {1.6875, 0x00, 0xD8, 0x06, 0xC0},
    {1.75,   0x00, 0xE0, 0x07, 0x00},
    {1.8125, 0x00, 0xE8, 0x07, 0x40},
    {1.875,  0x00, 0xF0, 0x07, 0x80},
    {1.9375, 0x00, 0xF8, 0x07, 0xC0},
    {2,      0x01, 0x00, 0x08, 0x00},
    {2.125,  0x01, 0x10, 0x08, 0x80},
    {2.25,   0x01, 0x20, 0x09, 0x00},
    {2.375,  0x01, 0x30, 0x09, 0x80},
    {2.5,    0x01, 0x40, 0x0A, 0x00},
    {2.625,  0x01, 0x50, 0x0A, 0x80},
    {2.75,   0x01, 0x60, 0x0B, 0x00},
    {2.875,  0x01, 0x70, 0x0B, 0x80},
    {3,      0x01, 0x80, 0x0C, 0x00},
    {3.125,  0x01, 0x90, 0x0C, 0x80},
    {3.25,   0x01, 0xA0, 0x0D, 0x00},
    {3.375,  0x01, 0xB0, 0x0D, 0x80},
    {3.5,    0x01, 0xC0, 0x0E, 0x00},
    {3.625,  0x01, 0xD0, 0x0E, 0x80},
    {3.75,   0x01, 0xE0, 0x0F, 0x00},
    {3.875,  0x01, 0xF0, 0x0F, 0x80},
    {4,      0x02, 0x00, 0x10, 0x00},
    {4.25,   0x02, 0x20, 0x11, 0x00},
    {4.5,    0x02, 0x40, 0x12, 0x00},
    {4.75,   0x02, 0x60, 0x13, 0x00},
    {5,      0x02, 0x80, 0x14, 0x00},
    {5.25,   0x02, 0xA0, 0x15, 0x00},
    {5.5,    0x02, 0xC0, 0x16, 0x00},
    {5.75,   0x02, 0xE0, 0x17, 0x00},
    {6,      0x03, 0x00, 0x18, 0x00},
    {6.25,   0x03, 0x20, 0x19, 0x00},
    {6.5,    0x03, 0x40, 0x1A, 0x00},
    {6.75,   0x03, 0x60, 0x1B, 0x00},
    {7,      0x03, 0x80, 0x1C, 0x00},
    {7.25,   0x03, 0xA0, 0x1D, 0x00},
    {7.5,    0x03, 0xC0, 0x1E, 0x00},
    {7.75,   0x03, 0xE0, 0x1F, 0x00},
    {8,      0x04, 0x00, 0x20, 0x00},
    {8.5,    0x04, 0x40, 0x22, 0x00},
    {9,      0x04, 0x80, 0x24, 0x00},
    {9.5,    0x04, 0xC0, 0x26, 0x00},
    {10,     0x05, 0x00, 0x28, 0x00},
    {10.5,   0x05, 0x40, 0x2A, 0x00},
    {11,     0x05, 0x80, 0x2C, 0x00},
    {11.5,   0x05, 0xC0, 0x2E, 0x00},
    {12,     0x06, 0x00, 0x30, 0x00},
    {12.5,   0x06, 0x40, 0x32, 0x00},
    {13,     0x06, 0x80, 0x34, 0x00},
    {13.5,   0x06, 0xC0, 0x36, 0x00},
    {14,     0x07, 0x00, 0x38, 0x00},
    {14.5,   0x07, 0x40, 0x3A, 0x00},
    {15,     0x07, 0x80, 0x3C, 0x00},
    {15.5,   0x07, 0xC0, 0x3E, 0x00},
    {15.99,  0X07, 0xFF, 0x3F, 0xFF},
};


static AX_F32 os08a20_again2value(float gain, AX_U8 *again_in, AX_U8 *again_de)
{
    AX_U32 i;
    AX_U32 count;
    AX_U32 ret_value = 0;

    if (!again_in || !again_de)
        return -1;

    count = sizeof(os08a20_gain_table) / sizeof(os08a20_gain_table[0]);

    for (i = 0; i < count; i++) {
        if (gain > os08a20_gain_table[i].gain) {
            continue;
        } else {
            if (again_in)
                *again_in = os08a20_gain_table[i].again_in;
            if (again_de)
                *again_de = os08a20_gain_table[i].again_de;
            SNS_DBG("again=%f, again_in=0x%x, again_de=0x%x\n", gain, *again_in, *again_de);
            return os08a20_gain_table[i].gain;
        }
    }

    return -1;
}

static AX_F32 os08a20_dgain2value(float gain, AX_U8 *dgain_in, AX_U8 *dgain_de, AX_U8 *dgain_de2)
{
    AX_U32 i;
    AX_U32 count;
    AX_U32 ret_value = 0;

    if (!dgain_in || !dgain_de)
        return -1;

    count = sizeof(os08a20_gain_table) / sizeof(os08a20_gain_table[0]);

    for (i = 0; i < count; i++) {
        if (gain > os08a20_gain_table[i].gain) {
            continue;
        } else {
            *dgain_in = os08a20_gain_table[i].dgain_in;
            *dgain_de = os08a20_gain_table[i].dgain_de;

            SNS_DBG("dgain=%f, dgain_in=0x%x, dgain_de=0x%x\n", gain, *dgain_in, *dgain_de);

            return os08a20_gain_table[i].gain;
        }
    }

    return -1;
}


static AX_S32 os08a20_get_gain_table(AX_SNS_AE_GAIN_TABLE_T *params)
{
    AX_U32 i;
    AX_S32 ret = 0;
    if (!params)
        return -1;

    params->nAgainTableSize = sizeof(os08a20_gain_table) / sizeof(os08a20_gain_table[0]);
    params->nDgainTableSize = sizeof(os08a20_gain_table) / sizeof(os08a20_gain_table[0]);

    for (i = 0; i < params->nAgainTableSize ; i++) {
        nAgainTable[i] = os08a20_gain_table[i].gain;
        params->pAgainTable = nAgainTable;
    }

    for (i = 0; i < params->nDgainTableSize ; i++) {
        nDgainTable[i] = os08a20_gain_table[i].gain;
        params->pDgainTable = nDgainTable;
    }

    return ret;
}


/****************************************************************************
 * exposure control external function
 ****************************************************************************/
AX_S32 os08a20_cfg_aec_param(ISP_PIPE_ID nPipeId)
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
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMinIntegrationTime[HDR_LONG_FRAME_IDX] = 2;
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMaxIntegrationTime[HDR_LONG_FRAME_IDX] = sns_os08a20params[nPipeId].vts - 8;
        sns_obj->ae_ctrl_param.sns_ae_param.nIntegrationTimeIncrement[HDR_LONG_FRAME_IDX] = 1;
    } else if (IS_2DOL_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMinIntegrationTime[HDR_LONG_FRAME_IDX] = 2;
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMaxIntegrationTime[HDR_LONG_FRAME_IDX] = sns_os08a20params[nPipeId].vts - 8;
        sns_obj->ae_ctrl_param.sns_ae_param.nIntegrationTimeIncrement[HDR_LONG_FRAME_IDX] = 1;

        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMinIntegrationTime[HDR_MEDIUM_FRAME_IDX] = 2;
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

AX_S32 os08a20_sns_update_regidx_table(ISP_PIPE_ID nPipeId, AX_U8 nRegIdx, AX_U8 nRegValue)
{
    AX_S32 i = 0;
    AX_U32 nNum = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    nNum = sizeof(gOs08a20AeRegsTable) / sizeof(gOs08a20AeRegsTable[0]);
    SNS_CHECK_VALUE_RANGE_VALID(nRegIdx, 0, nNum - 1);

    sns_obj->sztRegsInfo[0].sztI2cData[nRegIdx].nData = nRegValue;

    SNS_DBG("Idx = %d, reg addr 0x%x, reg data 0x%x\n",
        nRegIdx, sns_obj->sztRegsInfo[0].sztI2cData[nRegIdx].nRegAddr, nRegValue);

    return SNS_SUCCESS;
}

AX_S32 os08a20_get_sensor_gain_table(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_TABLE_T *params)
{
    AX_S32 result = 0;
    SNS_CHECK_PTR_VALID(params);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    result = os08a20_get_gain_table(params);
    return result;

    return SNS_SUCCESS;
}


AX_S32 os08a20_set_again(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_CFG_T *ptAGain)
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

    gain_value = os08a20_again2value(nGainFromUser, &Gain_in, &Gain_de);
    if (gain_value == -1) {
        SNS_ERR("new gain match failed \n");
    } else {
        sns_obj->ae_ctrl_param.sns_ae_param.fCurAGain[HDR_LONG_FRAME_IDX] = gain_value;
        result = os08a20_sns_update_regidx_table(nPipeId, OS08A20_LONG_AGAIN_H_IDX, (Gain_in & 0x3F));
        result |= os08a20_sns_update_regidx_table(nPipeId, OS08A20_LONG_AGAIN_L_IDX, (Gain_de & 0xFF));
        if (result != 0) {
            SNS_ERR("write hw failed %d \n", result);
            return result;
        }
    }

    /* medium gain seting */
    if (IS_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {

        nGainFromUser = ptAGain->fGain[HDR_MEDIUM_FRAME_IDX];
        nGainFromUser = AXSNS_CLIP3(nGainFromUser, sns_obj->ae_ctrl_param.sns_ae_limit.fMinAgain[HDR_MEDIUM_FRAME_IDX],
                                    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxAgain[HDR_MEDIUM_FRAME_IDX]);

        gain_value = os08a20_again2value(nGainFromUser, &Gain_in, &Gain_de);
        if (gain_value == -1) {
            SNS_ERR(" new gain match failed \n");
        } else {
            sns_obj->ae_ctrl_param.sns_ae_param.fCurAGain[HDR_MEDIUM_FRAME_IDX] = gain_value;
            result = os08a20_sns_update_regidx_table(nPipeId, OS08A20_SHORT_AGAIN_H_IDX, (Gain_in & 0x3F));
            result = os08a20_sns_update_regidx_table(nPipeId, OS08A20_SHORT_AGAIN_L_IDX, (Gain_de & 0xFF));
            if (result != 0) {
                SNS_ERR("%s: write hw failed %d \n", __func__, result);
                return result;
            }
        }
    }

    return SNS_SUCCESS;
}

AX_S32 os08a20_set_dgain(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_CFG_T *ptDGain)
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

    gain_val = os08a20_dgain2value(nGainFromUser, &Gain_in, &Gain_de, &Gain_de2);
    if (gain_val == -1) {
        SNS_ERR("new gain match failed \n");
    } else {
        sns_obj->ae_ctrl_param.sns_ae_param.fCurDGain[HDR_LONG_FRAME_IDX] = gain_val;
        result = os08a20_sns_update_regidx_table(nPipeId, OS08A20_LONG_DGAIN_H_IDX, (Gain_in & 0x3F));
        result = os08a20_sns_update_regidx_table(nPipeId, OS08A20_LONG_DGAIN_L_IDX, (Gain_de & 0xFF));
        if (result != 0) {
            SNS_ERR("write hw failed %d \n", result);
            return result;
        }
    }

    /* medium frame digital gain seting */
    if (IS_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        nGainFromUser = ptDGain->fGain[HDR_MEDIUM_FRAME_IDX];
        nGainFromUser = AXSNS_CLIP3(nGainFromUser, sns_obj->ae_ctrl_param.sns_ae_limit.fMinDgain[HDR_MEDIUM_FRAME_IDX],
                                    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxDgain[HDR_MEDIUM_FRAME_IDX]);

        gain_val = os08a20_dgain2value(nGainFromUser, &Gain_in, &Gain_de, &Gain_de2);
        if (gain_val == -1) {
            SNS_ERR("new gain match failed \n");
        } else {
            sns_obj->ae_ctrl_param.sns_ae_param.fCurDGain[HDR_MEDIUM_FRAME_IDX] = gain_val;
            result = os08a20_sns_update_regidx_table(nPipeId, OS08A20_SHORT_DGAIN_H_IDX, (Gain_in & 0x3F));
            result = os08a20_sns_update_regidx_table(nPipeId, OS08A20_SHORT_DGAIN_L_IDX, (Gain_de & 0xFF));
            if (result != 0) {
                SNS_ERR("write hw failed %d \n", result);
                return result;
            }
        }
    }

    return SNS_SUCCESS;
}


/* Calculate the max int time according to the exposure ratio */
AX_S32 os08a20_get_integration_time_range(ISP_PIPE_ID nPipeId, AX_F32 fHdrRatio,
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

AX_S32 os08a20_set_integration_time(ISP_PIPE_ID nPipeId, AX_SNS_AE_SHUTTER_CFG_T *ptIntTime)
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
    os08a20_sns_update_regidx_table(nPipeId, OS08A20_LONG_EXP_LINE_H_IDX, ex_h);
    os08a20_sns_update_regidx_table(nPipeId, OS08A20_LONG_EXP_LINE_L_IDX, ex_l);

    sns_obj->ae_ctrl_param.sns_ae_param.nCurIntegrationTime[HDR_LONG_FRAME_IDX] = nExpLineFromUser;

    /* short frame shutter seting */
    if (IS_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        nExpLineFromUser = ptIntTime->nIntTime[HDR_MEDIUM_FRAME_IDX];
        nExpLineFromUser = AXSNS_CLIP3(nExpLineFromUser,
                                        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMinIntegrationTime[HDR_MEDIUM_FRAME_IDX],
                                        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.nMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX]);

        ex_ival = nExpLineFromUser;
        ex_l = REG_LOW_8BITS(ex_ival);
        ex_h = REG_HIGH_8BITS(ex_ival);

        os08a20_sns_update_regidx_table(nPipeId, OS08A20_SHORT_EXP_LINE_H_IDX, ex_h);
        os08a20_sns_update_regidx_table(nPipeId, OS08A20_SHORT_EXP_LINE_L_IDX, ex_l);

        sns_obj->ae_ctrl_param.sns_ae_param.nCurIntegrationTime[HDR_MEDIUM_FRAME_IDX] = nExpLineFromUser;
    }

    return SNS_SUCCESS;
}

AX_S32 os08a20_get_hw_exposure_params(ISP_PIPE_ID nPipeId, AX_SNS_EXP_CTRL_PARAM_T *ptAeCtrlParam)
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

AX_U32 os08a20_sns_refresh_all_regs_from_tbl(ISP_PIPE_ID nPipeId)
{
    AX_S32 i = 0;
    AX_U32 nNum = 0;
    AX_U32  nRegValue;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    nNum = sizeof(gOs08a20AeRegsTable) / sizeof(gOs08a20AeRegsTable[0]);

    for (i = 0; i < nNum; i++) {
        nRegValue = os08a20_reg_read(nPipeId, gOs08a20AeRegsTable[i].nRegAddr);
        sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr = gOs08a20AeRegsTable[i].nRegAddr;
        sns_obj->sztRegsInfo[0].sztI2cData[i].nData = nRegValue;

        SNS_DBG(" nRegAddr 0x%x, nRegValue 0x%x\n", gOs08a20AeRegsTable[i].nRegAddr, gOs08a20AeRegsTable[i].nRegValue);
    }

    return SNS_SUCCESS;
}

AX_S32 os08a20_sns_update_init_exposure_reg(ISP_PIPE_ID nPipeId)
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

AX_S32 os08a20_ae_get_sensor_reg_info(ISP_PIPE_ID nPipeId, AX_SNS_REGS_CFG_TABLE_T *ptSnsRegsInfo)
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
        sns_obj->sztRegsInfo[0].nRegNum = sizeof(gOs08a20AeRegsTable) / sizeof(gOs08a20AeRegsTable[0]);
        sns_obj->sztRegsInfo[0].nCfg2ValidDelayMax = 2;

        for (i = 0; i < sns_obj->sztRegsInfo[0].nRegNum; i++) {
            sns_obj->sztRegsInfo[0].sztI2cData[i].bUpdate = AX_TRUE;
            sns_obj->sztRegsInfo[0].sztI2cData[i].nDevAddr = OS08A20_SLAVE_ADDR;
            sns_obj->sztRegsInfo[0].sztI2cData[i].nAddrByteNum = OS08A20_ADDR_BYTE;
            sns_obj->sztRegsInfo[0].sztI2cData[i].nDataByteNum = OS08A20_DATA_BYTE;
            sns_obj->sztRegsInfo[0].sztI2cData[i].nDelayFrmNum = gOs08a20AeRegsTable[i].nDelayFrmNum;
            SNS_DBG("pipe %d, [%2d] nRegAddr 0x%x, nRegValue 0x%x\n", nPipeId, i,
                    gOs08a20AeRegsTable[i].nRegAddr, gOs08a20AeRegsTable[i].nRegValue);
        }

        bUpdateReg = AX_TRUE;
        sns_obj->bSyncInit = AX_TRUE;
        os08a20_sns_update_init_exposure_reg(nPipeId);
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

static AX_U32 os08a20_set_vts(ISP_PIPE_ID nPipeId, AX_U32 vts)
{
    AX_U8 vts_h;
    AX_U8 vts_l;
    AX_S32 result = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    vts_l = vts & 0xFF;
    vts_h = (vts & 0xFF00) >> 8;

    result |= os08a20_sns_update_regidx_table(nPipeId, OS08A20_VTS_H_IDX, vts_h);
    result |= os08a20_sns_update_regidx_table(nPipeId, OS08A20_VTS_L_IDX, vts_l);

    return result;
}

AX_S32 os08a20_set_slow_fps(ISP_PIPE_ID nPipeId, AX_F32 nFps)
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
    result = os08a20_set_vts(nPipeId, sns_os08a20params[nPipeId].vts);
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

AX_S32 os08a20_get_slow_shutter_param(ISP_PIPE_ID nPipeId, AX_SNS_AE_SLOW_SHUTTER_PARAM_T *ptSlowShutterParam)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_S32 framerate = SNS_MAX_FRAME_RATE;
    AX_U32 nfps = 0;
    AX_U32 nVts = 0;

    SNS_CHECK_PTR_VALID(ptSlowShutterParam);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    framerate = sns_obj->sns_mode_obj.fFrameRate;
    if (framerate > SNS_MAX_FRAME_RATE) {
        SNS_ERR(" framerate out of range %d \n", framerate);
        return SNS_ERR_CODE_ILLEGAL_PARAMS;
    }

    if (sns_os08a20params[nPipeId].line_period == 0) {
        SNS_ERR("line_period is zero : %f\n", sns_os08a20params[nPipeId].line_period);
        return SNS_ERR_CODE_ILLEGAL_PARAMS;
    }

    ptSlowShutterParam->nGroupNum = AXSNS_MIN((sizeof(gFpsGear) / sizeof(AX_F32)), framerate);
    //ax_sns_quick_sort_float(gFpsGear, ptSlowShutterParam->nGroupNum);
    ptSlowShutterParam->fMinFps = AXSNS_MAX(gFpsGear[0],
                                            (1 * SNS_1_SECOND_UNIT_US / (sns_os08a20params[nPipeId].line_period * OS08A20_MAX_VTS)));

    for (nfps = 0 ; nfps < ptSlowShutterParam->nGroupNum; nfps++) {
        nVts = 1 * SNS_1_SECOND_UNIT_US / (sns_os08a20params[nPipeId].line_period * gFpsGear[nfps]);
        if ((AX_S32)gFpsGear[nfps] >= framerate) {
            nVts = sns_obj->sns_mode_obj.nVts;
        }
        if (nVts > OS08A20_MAX_VTS) {
            nVts = OS08A20_MAX_VTS;
            SNS_WRN("Beyond minmum fps  %f\n", ptSlowShutterParam->fMinFps);
        }

        if (IS_LINEAR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
            ptSlowShutterParam->tSlowShutterTbl[nfps].nMaxIntTime = (nVts  - 8);
        } else if (IS_2DOL_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
            ptSlowShutterParam->tSlowShutterTbl[nfps].nMaxIntTime = (AX_U32)((nVts  - 8) * OS08A20_MAX_RATIO / (OS08A20_MAX_RATIO + 1));
        }

        ptSlowShutterParam->tSlowShutterTbl[nfps].fRealFps = 1 * SNS_1_SECOND_UNIT_US / (sns_os08a20params[nPipeId].line_period * nVts);
        ptSlowShutterParam->fMaxFps  =  ptSlowShutterParam->tSlowShutterTbl[nfps].fRealFps;

        SNS_DBG("nPipeId = %d, line_period = %.2f, fps = %.2f, nMaxIntTime = %d, vts=0x%x\n",
                nPipeId, sns_os08a20params[nPipeId].line_period,
                ptSlowShutterParam->tSlowShutterTbl[nfps].fRealFps,
                ptSlowShutterParam->tSlowShutterTbl[nfps].nMaxIntTime, nVts);
    }

    return SNS_SUCCESS;
}


AX_S32 os08a20_set_fps(ISP_PIPE_ID nPipeId, AX_F32 fFps)
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
    result = os08a20_set_vts(nPipeId, sns_os08a20params[nPipeId].vts);
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
