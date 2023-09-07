/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _SAMPLE_VENC_HAL_H_
#define _SAMPLE_VENC_HAL_H_

#include "common_sys.h"

AX_S32 SAMPLE_Link2VencInit(AX_MOD_INFO_T *ptSrcMod, AX_U32 u32Width, AX_U32 u32Height, AX_BOOL bVencMode);
AX_S32 SAMPLE_Link2VencDeinit(AX_MOD_INFO_T *ptSrcMod, AX_BOOL bVencMode);

#endif /* _SAMPLE_IVPS_HAL_H_ */