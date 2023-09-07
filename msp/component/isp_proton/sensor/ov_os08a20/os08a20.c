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

#include "ax_sensor_struct.h"
#include "ax_base_type.h"
#include "ax_isp_common.h"
#include "isp_sensor_internal.h"
#include "isp_sensor_types.h"

#include "os08a20_settings.h"
#include "os08a20_reg.h"
#include "os08a20_ae_ctrl.h"
#include "os08a20_ae_params.h"

/* default param */
#include "os08a20_sdr.h"
#include "os08a20_hdr_2x.h"

#include "ax_module_version.h"

/****************************************************************************
 * golbal variables  and macro definition
 ****************************************************************************/

SNS_STATE_OBJ *g_szOs08a20Ctx[AX_VIN_MAX_PIPE_NUM] = {NULL};
SNSOS08A20_OBJ_T sns_os08a20params[AX_VIN_MAX_PIPE_NUM];

#define SENSOR_GET_CTX(dev, pstCtx) (pstCtx = g_szOs08a20Ctx[dev])
#define SENSOR_SET_CTX(dev, pstCtx) (g_szOs08a20Ctx[dev] = pstCtx)
#define SENSOR_RESET_CTX(dev) (g_szOs08a20Ctx[dev] = NULL)

const char axera_sns_os08a20_version[] = AXERA_MODULE_VERSION;

/****************************************************************************
 * Internal function definition
 ****************************************************************************/
static AX_S32 sensor_ctx_init(ISP_PIPE_ID nPipeId)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_S32 ret = 0;

    SNS_DBG("os08a20 sensor_ctx_init. ret = %d\n", ret);
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

static AX_VOID sensor_ctx_exit(ISP_PIPE_ID nPipeId)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SENSOR_GET_CTX(nPipeId, sns_obj);
    free(sns_obj);
    SENSOR_RESET_CTX(nPipeId);
}

/****************************************************************************
 * sensor control function
 ****************************************************************************/
static AX_S32 os08a20_get_chipid(ISP_PIPE_ID nPipeId, AX_S32 *pSnsId)
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

static void os08a20_init(ISP_PIPE_ID nPipeId)
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
        nRet = sensor_ctx_init(nPipeId);
        if (0 != nRet) {
            SNS_ERR("sensor_ctx_init failed!\n");
            return;
        } else {
            SENSOR_GET_CTX(nPipeId, sns_obj);
        }
    }

    /* 2. i2c init */
    os08a20_sensor_i2c_init(nPipeId);

    nRet = os08a20_get_chipid(nPipeId, &nSnsId);
    if (nRet != SNS_SUCCESS) {
        SNS_ERR("can't find os08a20 sensor id.\r\n");
    } else {
        SNS_DBG("os08a20 check chip id success.\r\n");
    }

    /* 3. config settings  */
    nImagemode = sns_obj->eImgMode;
    os08a20_write_settings(nPipeId, nImagemode);

    /* 4. refresh ae param */
    os08a20_cfg_aec_param(nPipeId);

    /* 5. refresh ae regs table */
    os08a20_sns_refresh_all_regs_from_tbl(nPipeId);
    sns_obj->bSyncInit = AX_FALSE;
    sns_obj->sns_mode_obj.nVts = os08a20_get_vts(nPipeId);

    return;
}

static void os08a20_exit(ISP_PIPE_ID nPipeId)
{
    if (nPipeId < 0 || (nPipeId >= AX_VIN_MAX_PIPE_NUM)) {
        return;
    }

    os08a20_sensor_i2c_exit(nPipeId);
    sensor_ctx_exit(nPipeId);

    return;
}

static AX_S32 os08a20_sensor_streaming_ctrl(ISP_PIPE_ID nPipeId, AX_U32 on)
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

static AX_S32 os08a20_sensor_set_mode(ISP_PIPE_ID nPipeId, AX_SNS_ATTR_T *sns_mode)
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
        nRet = sensor_ctx_init(nPipeId);
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

static AX_S32 os08a20_sensor_get_mode(ISP_PIPE_ID nPipeId, AX_SNS_ATTR_T *pSnsMode)
{
    AX_S32 nRet = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_PTR_VALID(pSnsMode);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    if (AX_NULL == sns_obj) {
        /* contex init */
        nRet = sensor_ctx_init(nPipeId);
        if (0 != nRet) {
            SNS_ERR("sensor_ctx_init failed!\n");
            return -1;
        } else {
            SENSOR_GET_CTX(nPipeId, sns_obj);
        }
    }

    memcpy(pSnsMode, &sns_obj->sns_attr_param, sizeof(AX_SNS_ATTR_T));

    return SNS_SUCCESS;
}

static AX_S32 os08a20_testpattern_ctrl(ISP_PIPE_ID nPipeId, AX_U32 on)
{
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SNS_DBG("test pattern enable: %d\n", on);
    if (1 == on) {
        /* enable test-pattern */
        os08a20_write_register(nPipeId, 0x5081, 0x80);
    } else {
        /* disable test-pattern */
        os08a20_write_register(nPipeId, 0x5081, 0x00);
    }

    return SNS_SUCCESS;
}

/****************************************************************************
 * get module default parameters function
 ****************************************************************************/
static AX_S32 os08a20_get_isp_default_params(ISP_PIPE_ID nPipeId, AX_SENSOR_DEFAULT_PARAM_T *ptDftParam)
{
    AX_S32 nRet = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_SNS_HDR_MODE_E nHdrmode;

    SNS_CHECK_PTR_VALID(ptDftParam);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);


    SENSOR_GET_CTX(nPipeId, sns_obj);
    if (AX_NULL == sns_obj) {
        /* contex init */
        nRet = sensor_ctx_init(nPipeId);
        if (0 != nRet) {
            SNS_ERR("sensor_ctx_init failed!\n");
            return SNS_ERR_CODE_INIT_FAILD;
        } else {
            SENSOR_GET_CTX(nPipeId, sns_obj);
        }
    }

    memset(ptDftParam, 0, sizeof(AX_SENSOR_DEFAULT_PARAM_T));

    nHdrmode = sns_obj->sns_mode_obj.eHDRMode;

    SNS_DBG(" current hdr mode %d \n", nHdrmode);

    switch (nHdrmode) {
    case AX_SNS_LINEAR_MODE:
        /* TODO: Users configure their own default parameters */
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
        ptDftParam->ptScene         = &scene_param_sdr;
        ptDftParam->ptDehaze        = &dehaze_param_sdr;
        ptDftParam->ptLdc           = &ldc_param_sdr;
        break;

    case AX_SNS_HDR_2X_MODE:
        /* TODO: Users configure their own default parameters */
        ptDftParam->ptHdr           = &hdr_param_hdr_2x;
        ptDftParam->ptDpc           = &dpc_param_hdr_2x;
        ptDftParam->ptBlc           = &blc_param_hdr_2x;
        ptDftParam->ptFpn           = &fpn_param_hdr_2x;
        ptDftParam->ptCsc           = &csc_param_hdr_2x;
        ptDftParam->ptDemosaic      = &demosaic_param_hdr_2x;
        ptDftParam->ptGic           = &gic_param_hdr_2x;
        ptDftParam->ptGamma         = &gamma_param_hdr_2x;
        ptDftParam->ptCc            = &cc_param_hdr_2x;
        ptDftParam->ptMde           = &mde_param_hdr_2x;
        ptDftParam->ptAYnr          = &aynr_param_hdr_2x;
        ptDftParam->ptACnr          = &acnr_param_hdr_2x;
        ptDftParam->ptRltm          = &rltm_param_hdr_2x;
        ptDftParam->ptRaw3dnr       = &raw3dnr_param_hdr_2x;
        ptDftParam->ptDepurple      = &depurple_param_hdr_2x;
        ptDftParam->ptLsc           = &lsc_param_hdr_2x;
        ptDftParam->ptSharpen       = &sharpen_param_hdr_2x;
        ptDftParam->ptScm           = &scm_param_hdr_2x;
        ptDftParam->ptYnr           = &ynr_param_hdr_2x;
        ptDftParam->ptCnr           = &cnr_param_hdr_2x;
        ptDftParam->ptCcmp          = &ccmp_param_hdr_2x;
        ptDftParam->ptYcproc        = &ycproc_param_hdr_2x;
        ptDftParam->ptYcrt          = &ycrt_param_hdr_2x;
        ptDftParam->ptCa            = &ca_param_hdr_2x;
        ptDftParam->ptAinr          = &ainr_param_hdr_2x;
        ptDftParam->ptAwbDftParam   = &awb_param_hdr_2x;
        ptDftParam->ptAeDftParam    = &ae_param_hdr_2x;
        ptDftParam->ptAice          = &aice_param_hdr_2x;
        ptDftParam->pt3Dlut         = &isp_3dlut_param_hdr_2x;
        ptDftParam->ptScene         = &scene_param_hdr_2x;
        ptDftParam->ptDehaze        = &dehaze_param_hdr_2x;
        ptDftParam->ptLdc           = &ldc_param_hdr_2x;
        break;

    case AX_SNS_HDR_3X_MODE:
        /* TODO: Users configure their own default parameters */
        break;

    case AX_SNS_HDR_4X_MODE:
        /* TODO: Users configure their own default parameters */
        break;
    default:
        SNS_ERR(" hdr mode %d error\n", nHdrmode);
        break;
    }

    return SNS_SUCCESS;
}

static AX_S32 os08a20_get_isp_black_level(ISP_PIPE_ID nPipeId, AX_SNS_BLACK_LEVEL_T *ptBlackLevel)
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

AX_SYS_API_PUBLIC AX_SENSOR_REGISTER_FUNC_T gSnsos08a20Obj = {
    /* sensor ctrl */
    .pfn_sensor_chipid                      = os08a20_get_chipid,
    .pfn_sensor_init                        = os08a20_init,
    .pfn_sensor_exit                        = os08a20_exit,
    .pfn_sensor_reset                       = os08a20_reset,
    .pfn_sensor_streaming_ctrl              = os08a20_sensor_streaming_ctrl,
    .pfn_sensor_testpattern                 = os08a20_testpattern_ctrl,

    .pfn_sensor_set_mode                    = os08a20_sensor_set_mode,
    .pfn_sensor_get_mode                    = os08a20_sensor_get_mode,

    .pfn_sensor_set_fps                     = os08a20_set_fps,

    /* communication : register read/write */
    .pfn_sensor_set_bus_info                = os08a20_set_bus_info,
    .pfn_sensor_write_register              = os08a20_write_register,
    .pfn_sensor_read_register               = os08a20_read_register,

    /* default param */
    .pfn_sensor_get_default_params          = os08a20_get_isp_default_params,
    .pfn_sensor_get_isp_black_level         = os08a20_get_isp_black_level,

    /* ae ctrl */
    .pfn_sensor_get_hw_exposure_params      = os08a20_get_hw_exposure_params,
    .pfn_sensor_get_gain_table              = os08a20_get_sensor_gain_table,
    .pfn_sensor_set_again                   = os08a20_set_again,
    .pfn_sensor_set_dgain                   = os08a20_set_dgain,
    .pfn_sensor_hcglcg_ctrl                 = AX_NULL,

    .pfn_sensor_set_integration_time        = os08a20_set_integration_time,
    .pfn_sensor_get_integration_time_range  = os08a20_get_integration_time_range,
    .pfn_sensor_set_slow_fps                = os08a20_set_slow_fps,
    .pfn_sensor_get_slow_shutter_param      = os08a20_get_slow_shutter_param,
    .pfn_sensor_get_sns_reg_info            = os08a20_ae_get_sensor_reg_info,
    .pfn_sensor_set_wbgain                  = AX_NULL,
};
