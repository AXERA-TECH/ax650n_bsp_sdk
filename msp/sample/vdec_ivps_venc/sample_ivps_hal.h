/**********************************************************************************
 *
 * Copyright (c) 2019-2020 Beijing AXera Technology Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Beijing AXera Technology Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Beijing AXera Technology Co., Ltd.
 *
 **********************************************************************************/
#ifndef _SAMPLE_IVPS_HAL_H_
#define _SAMPLE_IVPS_HAL_H_

#include <signal.h>
#include "ax_sys_api.h"
#include "ax_ivps_api.h"


AX_S32 SampleIVPS_Init(int GrpNum, AX_U32 width, AX_U32 height);
AX_S32 SampleIvpsExit(int GrpNum);


#endif /* _SAMPLE_IVPS_HAL_H_ */
