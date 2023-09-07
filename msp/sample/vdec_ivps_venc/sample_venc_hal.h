/**********************************************************************************
 *
 * Copyright (c) 2019-2020 Beijing AXera Technology Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Beijing AXera Technology Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Beijing AXera Technology Co., Ltd.
 *
 **********************************************************************************/
#ifndef _SAMPLE_VENC_HAL_H_
#define _SAMPLE_VENC_HAL_H_

#include <signal.h>
#include "ax_sys_api.h"

AX_S32 SAMPLE_VencInit(AX_S32 chnNum, AX_U16 width, AX_U16 height, AX_MOD_ID_E enModId, AX_BOOL bSaveStrm);
AX_S32 SAMPLE_VencDeinit(AX_S32 nChnIdx);

#endif /* _SAMPLE_IVPS_HAL_H_ */
