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
#include "ax_sys_api.h"
#include "ax_ivps_api.h"
#include "AXRtspWrapper.h"

typedef struct
{
	AX_S32 nEfd;
	IVPS_GRP nIvpsGrp;
	AX_VIDEO_FRAME_T tFrameInput;
	AX_IVPS_GRP_ATTR_T tGrpAttr;
	AX_S32 nIvpsRepeatNum;
	AX_S32 nIvpsStreamNum;
	AX_U64 nPhyAddr[2];
	AX_U8 *pVirAddr[2];
	AX_BLK BlkId0;
	AX_BLK BlkId1;
	char *pFilePath;
	char *pFileName;
	AX_IVPS_PIPELINE_ATTR_T tPipelineAttr;

	struct
	{
		AX_S32 nFd;
		AX_BOOL bEmpty;
		IVPS_CHN nIvpsChn;
	} arrOutChns[5];
	AX_RTSP_HANDLE pRtspObj;
	AX_U8 nFrameDump; /* 1: venc dump */
} SAMPLE_GRP_T;
extern SAMPLE_GRP_T gSampleGrp;

AX_S32 SAMPLE_IVPS_StartGrp(SAMPLE_GRP_T *p);
AX_S32 SAMPLE_IVPS_StopGrp(const SAMPLE_GRP_T *p);
AX_S32 SAMPLE_IVPS_ThreadStart(SAMPLE_GRP_T *src);

#endif /* _SAMPLE_IVPS_HAL_H_ */
