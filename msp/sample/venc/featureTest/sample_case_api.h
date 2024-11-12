/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __SAMPLE_CASE_API_H__
#define __SAMPLE_CASE_API_H__

#include "common_venc.h"

#ifdef __cplusplus
extern "C" {
#endif

AX_VOID SampleJpegStartEncOnce(SAMPLE_VENC_CMD_PARA_T *pStCmdPara);
AX_VOID SampleJpegStopEncOnce();
AX_S32 SampleSetChnAttr(SAMPLE_VENC_CMD_PARA_T *pCml);
AX_S32 SampleSetJpegParam(SAMPLE_VENC_CMD_PARA_T *pCml);
AX_S32 SampleSetDynProfileLevel(SAMPLE_VENC_CMD_PARA_T *pCml);
AX_S32 SampleSetDynRcMode(SAMPLE_VENC_CMD_PARA_T *pCml);

#ifdef __cplusplus
}
#endif

#endif