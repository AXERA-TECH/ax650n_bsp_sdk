/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _SAMPLE_IVPS_VENC_H_
#define _SAMPLE_IVPS_VENC_H_

#include "ax_ivps_api.h"

AX_S32 SAMPLE_Ivps2VencInit(AX_U32 nWidth, AX_U32 nHeight, AX_MOD_INFO_T *pSrcMode, AX_MOD_ID_E dstModId);
AX_S32 SAMPLE_Ivps2VencDeinit(AX_MOD_INFO_T *pSrcMode, AX_MOD_ID_E dstModId);
#endif