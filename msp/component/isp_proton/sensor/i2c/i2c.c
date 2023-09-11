/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include "isp_sensor_types.h"

#include "i2c.h"

int i2c_init(AX_U8 bus_num, AX_U16 slave_addr)
{
    int ret;
    int i2c_fd = -1;
    char bus_index[8];
    char device_name[64];

    sprintf(bus_index, "%d", bus_num);
    sprintf(device_name, "/dev/i2c-%s", bus_index);

    SNS_DBG("%s: i2c device is %s \n", __func__, device_name);

    i2c_fd = open(device_name, O_RDWR);
    if (i2c_fd < 0) {
        SNS_ERR("Open /dev/i2c-%d error!\n", bus_num);
        return -1;
    }

    ret = ioctl(i2c_fd, I2C_SLAVE_FORCE, slave_addr);
    if (ret < 0) {
        SNS_ERR("CMD_SET_DEV error!\n");
        return ret;
    }

    return i2c_fd;
}

int i2c_exit(int i2c_fd)
{
    if (i2c_fd >= 0) {
        close(i2c_fd);
        i2c_fd = -1;
        return 0;
    }
    return -1;
}

int I2cSwapBytes(
    AX_U8 *pData,
    const AX_U8 NrOfDataBytes)
{
    int result = I2C_RET_SUCCESS;

    //SNS_DBG("%s: (enter)\n", __func__);

    if (pData == AX_NULL) {
        return (I2C_RET_INVALID_PARM);
    }

    switch (NrOfDataBytes) {
    case 1: {
        // nothing to do
        result = I2C_RET_SUCCESS;
        break;
    }

    case 2: {
        AX_U8 *pSwapData = pData;
        AX_U8 SwapByte = 0;

        // advance to second byte
        pSwapData++;

        // save first byte
        SwapByte = *(pData);

        // copy second byte to first position
        *pData = *pSwapData;

        // restore first byte to second position
        *pSwapData = SwapByte;

        result = I2C_RET_SUCCESS;
        break;
    }

    case 4: {
        AX_U32 *pSwapData = (AX_U32 *)(pData);
        AX_U32 Help = 0UL;

        // get byte 1
        Help = (*pSwapData & 0x000000FF);
        Help <<= 8;
        *pSwapData >>= 8;

        // subjoin byte 2
        Help |= (*pSwapData & 0x000000FF);
        Help <<= 8;
        *pSwapData >>= 8;

        // subjoin byte 3
        Help |= (*pSwapData & 0x000000FF);
        Help <<= 8;
        *pSwapData >>= 8;

        // get byte 4
        *pSwapData &= 0x000000FF;

        // subjoin bytes 1 to 3
        *pSwapData |= Help;

        result = I2C_RET_SUCCESS;
        break;
    }

    default: {
        SNS_ERR("%s: Wrong amount of bytes (%d) for swapping.\n", __func__, NrOfDataBytes);
        break;
    }
    }

    //SNS_DBG("%s: (exit)\n", __func__);

    return (result);
}

#if 0
int i2c_init(AX_U32 bus_num, AX_U32 slave_addr)
{
    char devname[20];
    int handle = 0;
    int ret = 0;

    snprintf(devname, sizeof(devname), "/dev/i2c-%u", bus_num);
    handle = open(devname, O_RDWR, S_IRUSR | S_IWUSR);
    if (handle < 0) {
        SNS_ERR("open /dev/i2c-%u failed\n", bus_num);
        return I2C_RET_FAILURE;
    }

    ret = ioctl(handle, I2C_SLAVE_FORCE, (slave_addr >> 1));
    if (ret < 0) {
        SNS_ERR("I2C_SLAVE_FORCE failed\n");
        close(handle);
        return I2C_RET_FAILURE;
    }

    return handle;
}

I2cReturnType i2c_deinit(int handle)
{
    if (handle >= 0) {
        close(handle);
        SNS_DBG("%s: succuss\n", __func__);
        return I2C_RET_SUCCESS;
    }

    return I2C_RET_SUCCESS;
}
#endif

I2cReturnType i2c_read(
    int handle,
    const AX_U16 slave_addr,
    AX_U32 reg_addr,
    AX_U8 reg_addr_size,
    AX_U8 *p_data,
    AX_U8 num_data,
    AX_BOOL bSwapBytesEnable)
{
    AX_U8 reg_addr_bus[4];
    struct i2c_rdwr_ioctl_data msgset;
    struct i2c_msg msgs[2];
    AX_U32 nmsgs;

    I2cReturnType result = I2C_RET_SUCCESS;
    int ret = 0;

    //SNS_DBG("%s: addr:0x%x, fd:%d\n", __func__, reg_addr, handle);
    memset(msgs, 0x0, sizeof(struct i2c_msg));

    if ((reg_addr_size > 4) || ((p_data == AX_NULL) && (num_data != 0))) {
        SNS_ERR("%s: parameter error!\n", __func__);
        return I2C_RET_INVALID_PARM;
    }

    switch (reg_addr_size) {
    case 4:
        reg_addr_bus[3] = (AX_U8)(reg_addr & 0xff);
        reg_addr >>= 8;
    case 3:
        reg_addr_bus[2] = (AX_U8)(reg_addr & 0xff);
        reg_addr >>= 8;
    case 2:
        reg_addr_bus[1] = (AX_U8)(reg_addr & 0xff);
        reg_addr >>= 8;

    case 1:
        reg_addr_bus[0] = (AX_U8)(reg_addr & 0xff);
        reg_addr >>= 8;
    case 0:
        break;
    default:
        SNS_ERR("%s: reg_addr_size error!\n", __func__);
        return I2C_RET_INVALID_PARM;
    }

    if (reg_addr_size) {
        msgs[0].addr = slave_addr;
        msgs[0].flags = !I2C_M_RD;
        msgs[0].len = reg_addr_size;
        msgs[0].buf = reg_addr_bus;

        msgs[1].addr = slave_addr;
        msgs[1].flags = I2C_M_RD;
        msgs[1].len = num_data;
        msgs[1].buf = p_data;

        nmsgs = 2;
    } else {
        msgs[0].addr = slave_addr;
        msgs[0].flags = I2C_M_RD;
        msgs[0].len = num_data;
        msgs[0].buf = p_data;

        nmsgs = 1;
    }

    msgset.msgs = msgs;
    msgset.nmsgs = nmsgs;

    ret = ioctl(handle, I2C_RDWR, &msgset);
    if (ret < 0) {
        result = I2C_RET_FAILURE;
        SNS_ERR("%s: Failed to read reg: %s.!\n", __func__, strerror(errno));
    }

    if (bSwapBytesEnable)
        I2cSwapBytes(p_data, num_data);

    return (result);
}

I2cReturnType i2c_write(
    int handle,
    const AX_U16 slave_addr,
    AX_U32 reg_addr,
    AX_U8 reg_addr_size,
    AX_U8 *p_data,
    AX_U8 num_data,
    AX_BOOL bSwapBytesEnable)
{
    AX_U8 buf[16];
    struct i2c_rdwr_ioctl_data msgset;
    struct i2c_msg msg[1];
    int i, ret = 0;

    //SNS_DBG("%s: addr:0x%x, fd:%d\n", __func__, reg_addr, handle);
    if ((reg_addr_size > 4) || ((p_data == AX_NULL) && (num_data != 0))) {
        SNS_ERR("%s: parameter error!\n", __func__);
        return I2C_RET_INVALID_PARM;
    }

    if (bSwapBytesEnable)
        I2cSwapBytes(p_data, num_data);

    switch (reg_addr_size) {
    case 4:
        buf[3] = (AX_U8)(reg_addr & 0xff);
        reg_addr >>= 8;
    case 3:
        buf[2] = (AX_U8)(reg_addr & 0xff);
        reg_addr >>= 8;
    case 2:
        buf[1] = (AX_U8)(reg_addr & 0xff);
        reg_addr >>= 8;
    case 1:
        buf[0] = (AX_U8)(reg_addr & 0xff);
        reg_addr >>= 8;
    case 0:
        break;
    default:
        SNS_ERR("%s: reg_addr_size error!\n", __func__);
        return I2C_RET_INVALID_PARM;
    }

    for (i = 0; i < num_data; i++) {
        buf[reg_addr_size + i] = p_data[i];
    }

    msg[0].addr = slave_addr;
    msg[0].flags = !I2C_M_RD;
    msg[0].len = reg_addr_size + num_data;
    msg[0].buf = buf;

    msgset.msgs = msg;
    msgset.nmsgs = 1;

    ret = ioctl(handle, I2C_RDWR, &msgset);
    if (ret < 0) {
        SNS_ERR("%s: Failed to write reg: %s.!\n", __func__, strerror(errno));
    }

    return I2C_RET_SUCCESS;
}
