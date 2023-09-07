/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _SAMPLE_VO_HAL_H_
#define _SAMPLE_VO_HAL_H_

#include "common_sys.h"
#include "ax_vo_api.h"

AX_VOID PrintVoReso(AX_VOID);
AX_S32 ParseVoPubAttr(AX_CHAR *pStr);
AX_S32 SampleGetVoModeInfo(AX_MOD_INFO_T *pstModeInfo);
AX_S32 VoInit(AX_MOD_INFO_T *pstModeInfo, AX_VO_SIZE_T *pstImgSize);
AX_VOID VoDeInit(AX_VOID);

#endif /*_SAMPLE_VO_HAL_H_*/