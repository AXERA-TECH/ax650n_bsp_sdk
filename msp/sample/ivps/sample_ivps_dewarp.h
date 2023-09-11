/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _SAMPLE_IVPS_DEWARP_H_
#define _SAMPLE_IVPS_DEWARP_H_

typedef enum
{
	SAMPLE_IVPS_DEWARP_NONE,
	SAMPLE_IVPS_DEWARP_GDC,
	SAMPLE_IVPS_DEWARP_SYNC,
} SAMPLE_IVPS_DEWARP_TYPE;

typedef struct
{
	SAMPLE_IVPS_DEWARP_TYPE eSampleType;
	char *pMeshFile;
	AX_U16 nSrcWidth;
	AX_U16 nSrcHeight;
	AX_U32 nRotate;
	int nRepeatNum;
	int nHandle;
	char *pFilePath;
	AX_VIDEO_FRAME_T tFrame;

	AX_IVPS_DEWARP_ATTR_T tDewarpAttr; /* common, perspective */
	AX_IVPS_GDC_ATTR_T tGdcAttr; /* fisheye, usermap */
} SAMPLE_IVPS_DEWARP_T;

typedef struct
{
	char *pMeshFile;
	AX_IVPS_GDC_CFG_T tGdcCfg;
} SAMPLE_IVPS_DEWARP_CFG_T;

AX_S32 IVPS_DewarpSingleThread(const AX_VIDEO_FRAME_T *ptFrame);
AX_S32 SAMPLE_IVPS_Dewarp(const AX_VIDEO_FRAME_T *ptFrame, char *pFilePath,
			  SAMPLE_IVPS_DEWARP_T *pSampleDewarp);

extern SAMPLE_IVPS_DEWARP_T gSampleDewarp;
extern SAMPLE_IVPS_GRP_T gSampleIvpsGrp;
#endif /* _SAMPLE_IVPS_DEWARP_H_ */
