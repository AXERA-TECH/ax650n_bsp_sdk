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
#include "ax_base_type.h"
#include "ax_isp_common.h"

#include "i2c.h"
#include "isp_sensor_types.h"
#include "isp_sensor_internal.h"

#include "os08a20_reg.h"
#include "os08a20_settings.h"


extern SNS_STATE_OBJ *g_szOs08a20Ctx[AX_VIN_MAX_PIPE_NUM];
#define SENSOR_GET_CTX(dev, pstCtx) (pstCtx = g_szOs08a20Ctx[dev])

static AX_SNS_COMMBUS_T gOs08a20BusInfo[AX_VIN_MAX_PIPE_NUM] = { {1}, {2},};


AX_S32 os08a20_set_bus_info(ISP_PIPE_ID nPipeId, AX_SNS_COMMBUS_T tSnsBusInfo)
{
    gOs08a20BusInfo[nPipeId].I2cDev = tSnsBusInfo.I2cDev;

    return SNS_SUCCESS;
}

AX_S32 os08a20_get_bus_num(ISP_PIPE_ID nPipeId)
{
    return gOs08a20BusInfo[nPipeId].I2cDev;
}


AX_S32 os08a20_sensor_i2c_init(ISP_PIPE_ID nPipeId)
{
    AX_S32 ret = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_S32 snsId = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    sns_obj->sns_i2c_obj.sns_i2c_fd = -1;
    sns_obj->sns_i2c_obj.slave_addr = OS08A20_SLAVE_ADDR;
    sns_obj->sns_i2c_obj.address_byte = OS08A20_ADDR_BYTE;
    sns_obj->sns_i2c_obj.data_byte = OS08A20_DATA_BYTE;
    sns_obj->sns_i2c_obj.swap_byte = OS08A20_SWAP_BYTES;

    sns_obj->sns_i2c_obj.sns_i2c_bnum = os08a20_get_bus_num(nPipeId);

    sns_obj->sns_i2c_obj.sns_i2c_fd = i2c_init(sns_obj->sns_i2c_obj.sns_i2c_bnum,
                                      sns_obj->sns_i2c_obj.slave_addr);

    SNS_DBG("os08a20 i2c init finish, i2c bus %d \n", sns_obj->sns_i2c_obj.sns_i2c_bnum);

    return SNS_SUCCESS;
}

AX_S32 os08a20_sensor_i2c_exit(ISP_PIPE_ID nPipeId)
{
    AX_S32 ret = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    ret = i2c_exit(sns_obj->sns_i2c_obj.sns_i2c_fd);

    return ret;
}

AX_S32 os08a20_read_register(ISP_PIPE_ID nPipeId, AX_U32 nAddr, AX_U32 *pData)
{
    AX_U8 data;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_S32 nRet = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if (-1 == sns_obj->sns_i2c_obj.sns_i2c_fd)
        return -1;

    nRet = i2c_read(sns_obj->sns_i2c_obj.sns_i2c_fd,
             sns_obj->sns_i2c_obj.slave_addr,
             nAddr,
             sns_obj->sns_i2c_obj.address_byte,
             (AX_U8 *)(pData),
             sns_obj->sns_i2c_obj.data_byte,
             sns_obj->sns_i2c_obj.swap_byte);

    return nRet;
}

AX_S32 os08a20_reg_read(ISP_PIPE_ID nPipeId, AX_U32 addr)
{
    AX_U8 data;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_S32 nRet = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if (-1 == sns_obj->sns_i2c_obj.sns_i2c_fd)
        return -1;

    i2c_read(sns_obj->sns_i2c_obj.sns_i2c_fd,
             sns_obj->sns_i2c_obj.slave_addr,
             addr,
             sns_obj->sns_i2c_obj.address_byte,
             (AX_U8 *)(&data),
             sns_obj->sns_i2c_obj.data_byte,
             sns_obj->sns_i2c_obj.swap_byte);

    return data;
}

AX_S32 os08a20_write_register(ISP_PIPE_ID nPipeId, AX_U32 addr, AX_U32 data)
{
    AX_S32 nRet = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if (-1 == sns_obj->sns_i2c_obj.sns_i2c_fd) {
        return -1;
    }

    nRet = i2c_write(sns_obj->sns_i2c_obj.sns_i2c_fd, sns_obj->sns_i2c_obj.slave_addr, addr,
                     sns_obj->sns_i2c_obj.address_byte,
                     (AX_U8 *)(&data), sns_obj->sns_i2c_obj.data_byte, sns_obj->sns_i2c_obj.swap_byte);

    return nRet;
}


AX_S32 os08a20_hw_reset(unsigned int gpio_num, unsigned int gpio_out_val)
{
    FILE *fp = AX_NULL;
    char file_name[50];
    char buf[10];

    sprintf(file_name, "/sys/class/gpio/gpio%d", gpio_num);
    if (0 != access(file_name, F_OK)) {
        sprintf(file_name, "/sys/class/gpio/export");
        fp = fopen(file_name, "w");
        if (fp == AX_NULL) {
            SNS_ERR("Cannot open %s.\n", file_name);
            return -1;
        }
        fprintf(fp, "%d", gpio_num);
        fclose(fp);

        sprintf(file_name, "/sys/class/gpio/gpio%d/direction", gpio_num);
        fp = fopen(file_name, "w");
        if (fp == AX_NULL) {
            SNS_ERR("Cannot open %s.\n", file_name);
            return -1;
        }
        fprintf(fp, "out");
        fclose(fp);
    }

    sprintf(file_name, "/sys/class/gpio/gpio%d/value", gpio_num);
    fp = fopen(file_name, "w");
    if (fp == AX_NULL) {
        SNS_ERR("Cannot open %s.\n", file_name);
        return -1;
    }
    if (gpio_out_val) {
        strcpy(buf, "1");
    } else {
        strcpy(buf, "0");
    }
    fprintf(fp, "%s", buf);
    fclose(fp);

    return 0;
}

AX_S32 os08a20_group_prepare(ISP_PIPE_ID nPipeId)
{
    AX_S32 result = 0;

    result |= os08a20_write_register(nPipeId, 0x320D, 0x00);
    result |= os08a20_write_register(nPipeId, 0x3208, 0x00);
    result |= os08a20_write_register(nPipeId, 0x0808, 0x00);
    result |= os08a20_write_register(nPipeId, 0x3800, 0x11);
    result |= os08a20_write_register(nPipeId, 0x3911, 0x22);
    result |= os08a20_write_register(nPipeId, 0x3208, 0x10);

    return result;
}


AX_S32 os08a20_reset(ISP_PIPE_ID nPipeId, AX_U32 nResetGpio)
{
    AX_S32 result = 0;
    if (nPipeId < 0 || (nPipeId >= AX_VIN_MAX_PIPE_NUM))
        return -1;

    result |= os08a20_hw_reset(nResetGpio, 0);
    usleep(5 * 1000);
    result |= os08a20_hw_reset(nResetGpio, 1);
    usleep(10 * 1000);

    return result;
}

AX_U32 os08a20_get_hts(ISP_PIPE_ID nPipeId)
{
    AX_U8 hts_h;
    AX_U8 hts_l;
    AX_U32 hts;

    if (nPipeId < 0 || (nPipeId >= AX_VIN_MAX_PIPE_NUM))
        return -1;

    hts_h = os08a20_reg_read(nPipeId, 0x380C);
    hts_l = os08a20_reg_read(nPipeId, 0x380D);

    hts = (AX_U32)(((hts_h & 0xF) << 8) | (AX_U32)(hts_l << 0));

    return hts;
}

AX_U32 os08a20_get_vs_hts(ISP_PIPE_ID nPipeId)
{
    AX_U8 hts_h;
    AX_U8 hts_l;
    AX_U32 hts;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    hts_h = os08a20_reg_read(nPipeId, 0x380C);
    hts_l = os08a20_reg_read(nPipeId, 0x380D);

    hts = (AX_U32)(((hts_h & 0xF) << 8) | (AX_U32)(hts_l << 0));

    return hts;
}

AX_U32 os08a20_set_hts(ISP_PIPE_ID nPipeId, AX_U32 hts)
{
    AX_U8 hts_h;
    AX_U8 hts_l;
    AX_S32 result = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    hts_l = hts & 0xFF;
    hts_h = (hts & 0xFF00) >> 8;

    result |= os08a20_write_register(nPipeId, 0x380C, hts_h);
    result |= os08a20_write_register(nPipeId, 0x380D, hts_l);

    return result;
}

AX_U32 os08a20_get_vts(ISP_PIPE_ID nPipeId)
{
    AX_U8 vts_h;
    AX_U8 vts_l;
    AX_U32 vts;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    vts_h = os08a20_reg_read(nPipeId, OS08A20_VTS_H);
    vts_l = os08a20_reg_read(nPipeId, OS08A20_VTS_L);

    vts = (AX_U32)(((vts_h & 0xFF) << 8) | (AX_U32)(vts_l << 0));

    return vts;
}


AX_F32 os08a20_get_sclk(ISP_PIPE_ID nPipeId)
{
    AX_U8 pre_div0;
    AX_U8 pre_div;
    AX_U16 multiplier;
    AX_U8 post_div;
    //AX_U8 sram_div;
    AX_U8 st_div;
    AX_U8 t_div;
    float inck;
    float sclk;

    inck = OS08a20_INCK_24M;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    pre_div0 = (os08a20_reg_read(nPipeId, 0x0322) & 0x1) + 1;

    pre_div = os08a20_reg_read(nPipeId, 0x0323) & 0x7;
    if (pre_div == 0) {
        pre_div = 1;
    } else if (pre_div == 1) {
        pre_div = 1.5;
    } else if (pre_div == 2) {
        pre_div = 2;
    } else if (pre_div == 3) {
        pre_div = 2.5;
    } else if (pre_div == 4) {
        pre_div = 3;
    } else if (pre_div == 5) {
        pre_div = 4;
    } else if (pre_div == 6) {
        pre_div = 6;
    } else if (pre_div == 7) {
        pre_div = 8;
    } else {
    }

    multiplier = (os08a20_reg_read(nPipeId, 0x0324) & 0x3) << 8;
    multiplier = multiplier | ((os08a20_reg_read(nPipeId, 0x0325)) & 0xFF);

    post_div = (os08a20_reg_read(nPipeId, 0x032f) & 0xF) + 1;
    //sram_div = (os08a20_reg_read(nPipeId, 0x0327) & 0xF) + 1;
    st_div = (os08a20_reg_read(nPipeId, 0x0328) & 0xF) + 1;

    t_div = os08a20_reg_read(nPipeId, 0x032a) & 0x7;
    if (t_div == 0) {
        t_div = 1;
    } else if (t_div == 1) {
        t_div = 1.5;
    } else if (t_div == 2) {
        t_div = 2;
    } else if (t_div == 3) {
        t_div = 2.5;
    } else if (t_div == 4) {
        t_div = 3;
    } else if (t_div == 5) {
        t_div = 3.5;
    } else if (t_div == 6) {
        t_div = 4;
    } else if (t_div == 7) {
        t_div = 5;
    } else {
    }
    sclk = (((((((float)(inck * 1000 * 1000) / pre_div0) / pre_div) * multiplier) / post_div) / st_div) / t_div);
    /*SNS_DBG("%s pre_div0=0x%x, pre_div=0x%x, multiplier=0x%x, post_div=0x%x, sram_div=0x%x, st_div=0x%x, t_div=0x%x \n", \
          __func__, pre_div0, pre_div, multiplier, post_div, sram_div, st_div, t_div); */

    return sclk;
}

AX_S32 os08a20_write_settings(ISP_PIPE_ID nPipeId, AX_U32 setindex)
{
    AX_S32 i, errnum = 0;
    AX_U8 rBuf[1];
    AX_S32 reg_count = 0;
    const camera_i2c_reg_array *default_setting = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SNS_DBG("os08a20 setitng index: %d\n", setindex);

    switch (setindex) {
    case e_OS08A20_4lane_3840x2160_Linear_10bit_30fps_1440Mbps:
        default_setting = &OS08A20_4lane_3840x2160_Linear_10bit_30fps_1440Mbps[0];
        reg_count = sizeof(OS08A20_4lane_3840x2160_Linear_10bit_30fps_1440Mbps) / sizeof(camera_i2c_reg_array);
        break;

    case e_OS08A20_4lane_3840x2160_Linear_10bit_25fps_1440Mbps:
        default_setting = &OS08A20_4lane_3840x2160_Linear_10bit_25fps_1440Mbps[0];
        reg_count = sizeof(OS08A20_4lane_3840x2160_Linear_10bit_25fps_1440Mbps) / sizeof(camera_i2c_reg_array);
        break;

    case e_OS08A20_4lane_3840x2160_Linear_12bit_25fps_1280Mbps:
        default_setting = &OS08A20_4lane_3840x2160_Linear_12bit_25fps_1280Mbps[0];
        reg_count = sizeof(OS08A20_4lane_3840x2160_Linear_12bit_25fps_1280Mbps) / sizeof(camera_i2c_reg_array);
        break;

    case e_OS08A20_4lane_3840x2160_Linear_12bit_30fps_1280Mbps:
        default_setting = &OS08A20_4lane_3840x2160_Linear_12bit_30fps_1280Mbps[0];
        reg_count = sizeof(OS08A20_4lane_3840x2160_Linear_12bit_30fps_1280Mbps) / sizeof(camera_i2c_reg_array);
        break;

    case e_OS08A20_4lane_3840x2160_hdr_10bit_25fps_1280Mbps:
        default_setting = &OS08A20_4lane_3840x2160_hdr_10bit_25fps_1280Mbps[0];
        reg_count = sizeof(OS08A20_4lane_3840x2160_hdr_10bit_25fps_1280Mbps) / sizeof(camera_i2c_reg_array);
        break;

    case e_OS08A20_4lane_3840x2160_hdr_10bit_30fps_1280Mbps:
        default_setting = &OS08A20_4lane_3840x2160_hdr_10bit_30fps_1280Mbps[0];
        reg_count = sizeof(OS08A20_4lane_3840x2160_hdr_10bit_30fps_1280Mbps) / sizeof(camera_i2c_reg_array);
        break;

    case e_OS08A20_4lane_3840x2160_Linear_10bit_60fps_1440Mbps:
        default_setting = &OS08A20_4lane_3840x2160_Linear_10bit_60fps_1440Mbps[0];
        reg_count = sizeof(OS08A20_4lane_3840x2160_Linear_10bit_60fps_1440Mbps) / sizeof(camera_i2c_reg_array);
        break;

    case e_OS08A20_4lane_3840x2160_Linear_10bit_50fps_1440Mbps:
        default_setting = &OS08A20_4lane_3840x2160_Linear_10bit_50fps_1440Mbps[0];
        reg_count = sizeof(OS08A20_4lane_3840x2160_Linear_10bit_50fps_1440Mbps) / sizeof(camera_i2c_reg_array);
        break;

    default:
        SNS_ERR("it's not supported. pipe=%d, setting mode=%d] \n", nPipeId, setindex);
        return SNS_ERR_CODE_ILLEGAL_PARAMS;
    }


    SNS_DBG("os08a20 setitng index: %d, reg_count %d\n", setindex, reg_count);
    for (i = 0; i < reg_count; i++) {
        os08a20_write_register(nPipeId, (default_setting + i)->addr, ((default_setting + i)->value));
#ifdef SENSOR_DEBUG
        usleep(2 * 1000);
        rBuf[0] = os08a20_reg_read(nPipeId, (default_setting + i)->addr);
        SNS_DBG(" addr: 0x%04x write:0x%02x read:0x%02x \r\n",
                (default_setting + i)->addr, (default_setting + i)->value, rBuf[0]);
#endif
    }

    return 0;
}
