/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _SAMPLE_VIN_HAL_H_
#define _SAMPLE_VIN_HAL_H_

#include "common_sys.h"
#include "common_vin.h"
#include "common_nt.h"
#include "common_cam.h"

typedef enum {
    SAMPLE_VIN_HAL_CASE_NONE  = -1,
    SAMPLE_VIN_HAL_CASE_SINGLE_DUMMY  = 0,
    SAMPLE_VIN_HAL_CASE_SINGLE_OS08A20  = 1,
    SAMPLE_VIN_HAL_CASE_BUTT
} SAMPLE_VIN_HAL_CASE_E;

AX_S32 SAMPLE_VIN_Init(SAMPLE_VIN_HAL_CASE_E eVinCase, COMMON_VIN_MODE_E eSysMode, AX_SNS_HDR_MODE_E eHdrMode,
                       AX_BOOL bAiispEnable, COMMON_SYS_ARGS_T *pPrivPool);
AX_S32 SAMPLE_VIN_DeInit(AX_VOID);

AX_S32 SAMPLE_VIN_Open(AX_VOID);
AX_S32 SAMPLE_VIN_Close(AX_VOID);

AX_S32 SAMPLE_VIN_Start(AX_VOID);
AX_S32 SAMPLE_VIN_Stop(AX_VOID);

#endif /* _SAMPLE_VIN_HAL_H_ */
