/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _SAMPLE_IVPS_PIPELINE_H_
#define _SAMPLE_IVPS_PIPELINE_H_

#include "ax_ivps_api.h"

typedef AX_S32 IVPS_GRP;

#define DEBUG_SEND_FRAME
#define DEBUG_LINK_FRAME

enum
{
	COLOR_GREEN = 0x000000,
	COLOR_RED = 0x0000FF,
	COLOR_BLUE = 0x00FF00,
	COLOR_PURPLE = 0x00FFFF,
	COLOR_DARK_GREEN = 0xFF0000,
	COLOR_YELLOW = 0xFF00FF,
	COLOR_DARK_BLACK = 0x008080,
	COLOR_GRAY = 0x808080,
	COLOR_WHITE = 0xFF8080,
	COLOR_NUM = 9
};

typedef struct
{
	IVPS_GRP IvpsGrp;
	IVPS_CHN IvpsChn;
	AX_MOD_INFO_T tSrcMod;
	AX_MOD_INFO_T tDstMod;
	AX_S32 nIvpsRepeatNum;
	AX_U32 nPhyAddr;
	AX_U8 *pVirAddr;
} SAMPLE_IVPS_CHN_T;

typedef struct
{
	AX_U16 nW;
	AX_U16 nH;
	AX_U16 nCropX;
	AX_U16 nCropY;
	AX_U16 nCropW;
	AX_U16 nCropH;
	AX_U32 nStride;
	AX_IMG_FORMAT_E eFormat;
	IVPS_GRP IvpsGrp;
} SAMPLE_IVPS_INPUT_INFO_T;

typedef struct
{
	IVPS_CHN nIvpsChn;
	AX_U32 Width;
	AX_U32 Height;
	AX_U32 Stride;
} SAMPLE_IVPS_ATTR_CHN_T;

typedef struct
{
	AX_S32 nEfd;
	IVPS_GRP nIvpsGrp;
	AX_VIDEO_FRAME_T tFrameInput;
	AX_IVPS_GRP_ATTR_T tGrpAttr;
	AX_S32 nIvpsRepeatNum;
	AX_S32 nRegionNum;
	AX_S32 nIvpsStreamNum;
	AX_U64 nPhyAddr[2];
	AX_U8 *pVirAddr[2];
	AX_BLK BlkId0;
	AX_BLK BlkId1;
	char *pFilePath;
	char *pFileName;
	AX_IVPS_PIPELINE_ATTR_T tPipelineAttr;
	AX_POOL user_pool_id;
	AX_POOL_SOURCE_E ePoolSrc;
	pthread_t get_frame_tid;
	AX_BOOL bSaveFile;
	pthread_t change_tid;
	pthread_t region_tid;
	pthread_t send_frame_tid;

	struct
	{
		AX_S32 nFd;
		AX_BOOL bEmpty;
		IVPS_CHN nIvpsChn;
	} arrOutChns[5];
} SAMPLE_IVPS_GRP_T;

typedef struct
{
	AX_S32 nRepeatNum;
	IVPS_GRP nGrpIdx;
	AX_S32 nFilterIdx;
	AX_IVPS_FILTER_T tCurValue;
	AX_IVPS_FILTER_T tMaxValue;
	AX_IVPS_FILTER_T tMinValue;
	AX_S32 nWidthStep;
	AX_S32 nHeightStep;
	AX_S32 nCropX0Step;
	AX_S32 nCropY0Step;
	AX_S32 nCropWStep;
	AX_S32 nCropHStep;
} SAMPLE_IVPS_CHANGE_T;

AX_S32 IVPS_StartGrp(SAMPLE_IVPS_GRP_T *p);
AX_S32 IVPS_StopGrp(const SAMPLE_IVPS_GRP_T *p);

AX_S32 IVPS_StartGrpV2(IVPS_CHN IvpsChn);
AX_S32 IVPS_StopGrpV2(IVPS_CHN IvpsChn);

AX_S32 IVPS_ThreadStart(AX_VOID *src, AX_VOID *dst);
AX_S32 IVPS_ThreadStop(SAMPLE_IVPS_GRP_T *pGrp);

AX_S32 IVPS_FilterAttrThreadStart(SAMPLE_IVPS_GRP_T *pGrp);
AX_S32 IVPS_FilterAttrThreadStop(SAMPLE_IVPS_GRP_T *pGrp);

extern SAMPLE_IVPS_CHANGE_T gSampleChange;
extern SAMPLE_IVPS_GRP_T gSampleIvpsGrp;
extern SAMPLE_IVPS_GRP_T gSampleIvpsGrpExt;
#endif /* _SAMPLE_IVPS_PIPELINE_H_ */
