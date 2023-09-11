/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __I2C_H__
#define __I2C_H__

#include "ax_base_type.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define I2C_NR_DAT_BYTES_1 (1)
#define I2C_NR_DAT_BYTES_2 (2)
#define I2C_NR_DAT_BYTES_4 (4)

typedef AX_S32 I2cReturnType;

enum I2C_RESULT {
    I2C_RET_SUCCESS = 0,  /**< The operation was successfully completed */
    I2C_RET_FAILURE = -1, /**< Generic error */
    I2C_RET_INVALID_PARM = -2,
};

extern AX_S32 g_i2c_fd;

AX_S32 i2c_init(AX_U8 bus_num, AX_U16 slave_add);

AX_S32 i2c_exit(AX_S32 i2c_fd);

AX_S32 I2cSwapBytes(
    AX_U8 *pData,
    const AX_U8 NrOfDataBytes);

I2cReturnType i2c_read(
    AX_S32 handle,
    const AX_U16 slave_addr,
    AX_U32 reg_addr,
    AX_U8 reg_addr_size,
    AX_U8 *p_data,
    AX_U8 num_data,
    AX_BOOL bSwapBytesEnable);

I2cReturnType i2c_write(
    AX_S32 handle,
    const AX_U16 slave_addr,
    AX_U32 reg_addr,
    AX_U8 reg_addr_size,
    AX_U8 *p_data,
    AX_U8 num_data,
    AX_BOOL bSwapBytesEnable);

#ifdef __cplusplus
}
#endif

#endif /* __I2C_H__ */
