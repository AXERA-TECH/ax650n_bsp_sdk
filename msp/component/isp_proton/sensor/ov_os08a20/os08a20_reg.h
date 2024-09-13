/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __OS08A20_REG_H__
#define __OS08A20_REG_H__

#include "ax_base_type.h"


#define OS08A20_SLAVE_ADDR      (0x36)    /**< i2c slave address of the OS08a20 camera sensor */
#define OS08A20_SENSOR_CHIP_ID  (0x530841)
#define OS08A20_ADDR_BYTE       (2)
#define OS08A20_DATA_BYTE       (1)
#define OS08A20_SWAP_BYTES      (1)
#define OS08a20_INCK_24M        (24)

/* Exposure control related registers */
#define     OS08A20_LONG_EXP_LINE_H     (0x3501)  /* bit[7:0], long frame exposure in unit of rows */
#define     OS08A20_LONG_EXP_LINE_L     (0x3502)  /* bit[7:0], long frame exposure in unit of rows */
#define     OS08A20_LONG_AGAIN_H        (0x3508)  /* bit[5:0], real gain[13:8] long frame */
#define     OS08A20_LONG_AGAIN_L        (0x3509)  /* bit[7:0], real gain[7:0] long frame */
#define     OS08A20_LONG_DGAIN_H        (0x350A)  /* bit[3:0], real gain[13:10] long frame */
#define     OS08A20_LONG_DGAIN_L        (0x350B)  /* bit[7:0], real gain[9:2] long frame */

#define     OS08A20_SHORT_EXP_LINE_H    (0x3511)  /* bit[7:0], short frame exposure in unit of rows */
#define     OS08A20_SHORT_EXP_LINE_L    (0x3512)  /* bit[7:0], short frame exposure in unit of rows */
#define     OS08A20_SHORT_AGAIN_H       (0x350C)  /* bit[5:0], real gain[13:8] short frame */
#define     OS08A20_SHORT_AGAIN_L       (0x350D)  /* bit[7:0], real gain[7:0] short frame */
#define     OS08A20_SHORT_DGAIN_H       (0x350E)  /* bit[3:0], real gain[13:10] short frame */
#define     OS08A20_SHORT_DGAIN_L       (0x350F)  /* bit[7:0], real gain[9:2] short frame */

#define     OS08A20_VTS_H               (0x380E)  /* bit[7:0], vts[15:8] */
#define     OS08A20_VTS_L               (0x380F)  /* bit[7:0], vts[7:0] */

/* VTS */
#define OS08A20_VTS_12BIT_8M30_SDR          (0x900)
#define OS08A20_VTS_12BIT_8M25_SDR          (0xad0)

#define OS08A20_VTS_10BIT_8M60_SDR          (0x90a)
#define OS08A20_VTS_10BIT_8M30_SDR          (0x1214)
#define OS08A20_VTS_10BIT_8M25_SDR          (0x15b1)

#define OS08A20_VTS_10BIT_8M30_HDR_2X       (0x8d0)
#define OS08A20_VTS_10BIT_8M25_HDR_2X       (0xae0)

AX_S32 os08a20_reset(ISP_PIPE_ID nPipeId, AX_U32 nResetGpio);
AX_S32 os08a20_sensor_i2c_init(ISP_PIPE_ID nPipeId);
AX_S32 os08a20_sensor_i2c_exit(ISP_PIPE_ID nPipeId);
AX_S32 os08a20_read_register(ISP_PIPE_ID nPipeId, AX_U32 nAddr, AX_U32 *pData);
AX_S32 os08a20_reg_read(ISP_PIPE_ID nPipeId, AX_U32 addr);
AX_S32 os08a20_write_register(ISP_PIPE_ID nPipeId, AX_U32 addr, AX_U32 data);

AX_U32 os08a20_get_hts(ISP_PIPE_ID nPipeId);
AX_F32 os08a20_get_sclk(ISP_PIPE_ID nPipeId);
AX_U32 os08a20_get_vts(ISP_PIPE_ID nPipeId);
AX_U32 os08a20_get_vs_hts(ISP_PIPE_ID nPipeId);

AX_S32 os08a20_write_settings(ISP_PIPE_ID nPipeId, AX_U32 setindex);

AX_S32 os08a20_set_bus_info(ISP_PIPE_ID nPipeId, AX_SNS_COMMBUS_T tSnsBusInfo);
AX_S32 os08a20_get_bus_num(ISP_PIPE_ID nPipeId);

#endif  //end __OS08A20_REG_H__
