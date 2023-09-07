/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __SAMPLE_CMD_PARAMS_H__
#define __SAMPLE_CMD_PARAMS_H__

#include <stdio.h>

#include "common_venc.h"


#ifdef __cplusplus
extern "C" {
#endif /* End of #ifdef __cplusplus */

AX_S32 SampleCmdLineParse(AX_S32 argc, AX_CHAR **argv, SAMPLE_VENC_CMD_PARA_T *pCml);
AX_VOID SampleSetDefaultParams(SAMPLE_VENC_CMD_PARA_T *pstPara);

#ifdef __cplusplus
}
#endif /* End of #ifdef __cplusplus */

#endif
