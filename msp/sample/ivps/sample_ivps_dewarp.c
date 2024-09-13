/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "sample_ivps_main.h"

/* fisheye top pano config */
SAMPLE_IVPS_DEWARP_T gSampleDewarp = {
    .eSampleType = SAMPLE_IVPS_DEWARP_NONE,
    .tGdcAttr = {
	.eGdcType = AX_IVPS_GDC_FISHEYE,
	.nSrcWidth = 2000,
	.nSrcHeight = 2000,
	.nDstStride = 4096,
	.nDstWidth = 4000,
	.nDstHeight = 2000,
	.eDstFormat = AX_FORMAT_YUV420_SEMIPLANAR,
	.tFisheyeAttr = {
	    .eMountMode = AX_IVPS_FISHEYE_MOUNT_MODE_CEILING,
	    .nHorOffset = 0,
	    .nVerOffset = 0,
	    .nRgnNum = 2,
	    .tFisheyeRgnAttr[0] = {
		.eViewMode = AX_IVPS_FISHEYE_VIEW_MODE_PANORAMA,

		.nPan = 0,
		.nTilt = 180,
		.nHorZoom = 4095,
		.nVerZoom = 4095,
		.nInRadius = 0,
		.nOutRadius = 1000,
		.tOutRect = {.nX = 0, .nY = 1000, .nW = 4000, .nH = 1000},
	    },
	    .tFisheyeRgnAttr[1] = {
		.eViewMode = AX_IVPS_FISHEYE_VIEW_MODE_BYPASS,
		.tOutRect = {.nX = 0, .nY = 0, .nW = 4000, .nH = 1000},
	    },
	},
    },
};

/* map user config */
SAMPLE_IVPS_DEWARP_T gSampleDewarpMapUser = {
    .eSampleType = SAMPLE_IVPS_DEWARP_GDC,
    .tGdcAttr = {
	.eGdcType = AX_IVPS_GDC_MAP_USER,
	.nSrcWidth = 1920,
	.nSrcHeight = 1080,
	.nDstStride = 1920,
	.nDstWidth = 1920,
	.nDstHeight = 1080,
	.eDstFormat = AX_FORMAT_YUV420_SEMIPLANAR,
	.tMapUserAttr = {
	    .nMeshStartX = 0,
	    .nMeshStartY = 0,
	    .nMeshWidth = 64,  /* nMeshWidth * (nMeshNumH - 1) >= nDstWidth */
	    .nMeshHeight = 48, /* nMeshHeight * (nMeshNumV - 1) >= nDstHeight */
	    .nMeshNumH = 33,
	    .nMeshNumV = 33,
	},
    },
};

/* perspective config */
SAMPLE_IVPS_DEWARP_T gSampleDewarpSync = {
    .nSrcWidth = 1920,
    .nSrcHeight = 1080,
    .eSampleType = SAMPLE_IVPS_DEWARP_SYNC,
    .tDewarpAttr = {
	.nDstWidth = 1920,
	.nDstHeight = 1080,
	.nDstStride = 1920,
	.eImgFormat = AX_FORMAT_YUV420_SEMIPLANAR,
	.bPerspective = AX_TRUE,
	/* .tPerspectiveAttr = {1921876, -745623, 262524440, 587421, 2439435, 103340980, 0, 0, 1000000}, */
	.tPerspectiveAttr = {{1000000, 0, 0, 0, 1000000, 0, 0, 0, 1000000}},
    },
};

static AX_S32 DewarpSingleCore(const AX_VIDEO_FRAME_T *ptFrame,
			       AX_IVPS_DEWARP_ATTR_T *ptDewarpAttr,
			       char *pStorePath,
			       char *fileName)
{
	AX_S32 ret = 0;
	AX_VIDEO_FRAME_T tDstFrame = {0};
	AX_BLK BlkId = 0, MeshBlkId = 0;
	AX_U32 nImgSize;

	ptDewarpAttr->eImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
	nImgSize = CalcImgSize(ptDewarpAttr->nDstStride, ptDewarpAttr->nDstWidth,
			       ptDewarpAttr->nDstHeight, ptDewarpAttr->eImgFormat, 16);
	CHECK_RESULT(BufPoolBlockAddrGet(-1, nImgSize, &tDstFrame.u64PhyAddr[0],
					 (AX_VOID **)(&tDstFrame.u64VirAddr[0]), &BlkId));

	tDstFrame.u64PhyAddr[1] = tDstFrame.u64PhyAddr[0] +
				  ptDewarpAttr->nDstStride * ALIGN_UP(ptDewarpAttr->nDstHeight, 16);

	memset((AX_VOID *)((AX_LONG)tDstFrame.u64VirAddr[0]), 0x00, nImgSize);

	AX_IVPS_Dewarp(ptFrame, &tDstFrame, ptDewarpAttr);
	SaveFile(&tDstFrame, 0, 0, pStorePath, fileName);
	ret = AX_POOL_ReleaseBlock(BlkId);
	if (ret)
	{
		ALOGE("IVPS Release BlkId fail, ret = %x", ret);
	}
	ret = AX_POOL_ReleaseBlock(MeshBlkId);
	if (ret)
	{
		ALOGE("IVPS Release Mesh BlkId fail, ret = %x", ret);
	}

	return ret;
}

/*
 * SAMPLE_IVPS_DewarpSync
 * Function: CropResize or Perspective.
 */
static AX_S32 SAMPLE_IVPS_DewarpSync(const AX_VIDEO_FRAME_T *ptFrame, char *pStorePath, SAMPLE_IVPS_DEWARP_T *pSampleDewarp)
{
	AX_S32 i, ret = 0;
	char fileName[64] = {0};

	snprintf(fileName, 64, "Dewarp_s%u_%ux%u_", pSampleDewarp->tDewarpAttr.nDstStride,
		 pSampleDewarp->tDewarpAttr.nDstWidth, pSampleDewarp->tDewarpAttr.nDstHeight);
	ret = DewarpSingleCore(ptFrame, &pSampleDewarp->tDewarpAttr, pStorePath, fileName);
	if (ret)
	{
		ALOGE("DewarpSingleCore fail, ret = %x", ret);
		return ret;
	}
	return ret;
}

static AX_S32 SAMPLE_IVPS_Fisheye(const AX_VIDEO_FRAME_T *ptSrc, char *pStorePath, SAMPLE_IVPS_DEWARP_T *pSampleDewarp)
{
	int ret, handle, nImgSize;
	AX_VIDEO_FRAME_T tDst = {0};
	AX_BLK BlkId;
	AX_IVPS_GDC_ATTR_T *pGdc = &pSampleDewarp->tGdcAttr;
	AX_IVPS_POINT_NICE_T tSrcPoint = {0}, tDstPoint = {0};
	AX_U64 nStartTime, nEndTime;

	nImgSize = CalcImgSize(pGdc->nDstStride, pGdc->nDstWidth, pGdc->nDstHeight, pGdc->eDstFormat, 16);

	ALOGI("Fisheye nImgSize =%d", nImgSize);
	ALOGI("Fisheye u32PicStride[0]=%d", tDst.u32PicStride[0]);

	CHECK_RESULT(BufPoolBlockAddrGet(-1, nImgSize, &tDst.u64PhyAddr[0],
					 (AX_VOID **)&tDst.u64VirAddr[0], &BlkId));
	tDstPoint.fX = 120;
	tDstPoint.fY = 300;
	AX_IVPS_FisheyePointQueryDst2Src(&tSrcPoint, &tDstPoint,
					 ptSrc->u32Width, ptSrc->u32Height, 0,
					 &pGdc->tFisheyeAttr);

	printf("SRC point nX:%f nY:%f\n", tSrcPoint.fX, tSrcPoint.fY);

	AX_IVPS_FisheyePointQuerySrc2Dst(&tDstPoint, &tSrcPoint,
					 ptSrc->u32Width, ptSrc->u32Height, 0,
					 &pGdc->tFisheyeAttr);

	printf("DST point nX:%f nY:%f\n", tDstPoint.fX, tDstPoint.fY);

	return 0;
	ret = AX_IVPS_GdcWorkCreate(&handle);
	if (ret)
	{
		ALOGE("AX_IVPS_GdcWorkCreate fail, ret = %x", ret);
		return -1;
	}

	ret = AX_IVPS_GdcWorkAttrSet(handle, pGdc);
	if (ret)
	{
		ALOGE("AX_IVPS_GdcWorkAttrSet fail, ret = %x", ret);
		return -2;
	}

	nStartTime = GetTickCount();
	ret = AX_IVPS_GdcWorkRun(handle, ptSrc, &tDst);
	if (ret)
	{
		ALOGE("AX_IVPS_GdcWorkRun fail, ret = %x", ret);
		return -3;
	}
	nEndTime = GetTickCount();
	printf("GDC time cost:%lld\n", nEndTime - nStartTime);

	SaveFile(&tDst, 0, 0, pStorePath, "Fisheye");

	ret = AX_POOL_ReleaseBlock(BlkId);
	if (ret)
	{
		ALOGE("Rls BlkId fail, ret=0x%x", ret);
	}

	ret = AX_IVPS_GdcWorkDestroy(handle);
	if (ret)
	{
		ALOGE("AX_IVPS_GdcWorkDestroy fail, ret = %x", ret);
		return -4;
	}
	return 0;
}

static AX_S32 SAMPLE_IVPS_MapUser(const AX_VIDEO_FRAME_T *ptSrc, char *pStorePath, SAMPLE_IVPS_DEWARP_T *pSampleDewarp)
{
	int ret, handle, nImgSize;
	AX_VIDEO_FRAME_T tDst = {0};
	AX_BLK BlkId;
	AX_IVPS_GDC_ATTR_T *pGdc = &pSampleDewarp->tGdcAttr;
	AX_IVPS_POINT_T tSrcPoint = {0}, tDstPoint = {0};
	AX_U64 nStartTime, nEndTime;

	nImgSize = CalcImgSize(pGdc->nDstStride, pGdc->nDstWidth, pGdc->nDstHeight, pGdc->eDstFormat, 16);

	ALOGI("MapUser nImgSize =%d", nImgSize);
	ALOGI("MapUser u32PicStride[0]=%d", tDst.u32PicStride[0]);

	CHECK_RESULT(BufPoolBlockAddrGet(-1, nImgSize, &tDst.u64PhyAddr[0],
					 (AX_VOID **)&tDst.u64VirAddr[0], &BlkId));

	pGdc->tMapUserAttr.pUserMap = malloc(pGdc->tMapUserAttr.nMeshNumV * pGdc->tMapUserAttr.nMeshNumH * 16);

	/* The following shows how to obtain the map table */
	int k;
	for (int j = 0; j < pGdc->tMapUserAttr.nMeshNumV; j++)
	{
		for (int i = 0; i < pGdc->tMapUserAttr.nMeshNumH; i++)
		{
			k = (j * pGdc->tMapUserAttr.nMeshNumH + i) * 2;
			/*
			  pGdc->tMapUserAttr.pUserMap[k] = dx;     Fixed-point decimals, s13.10
			  pGdc->tMapUserAttr.pUserMap[k + 1] = dy; Fixed-point decimals, s13.10
			*/
		}
	}

	ret = AX_IVPS_GdcWorkCreate(&handle);
	if (ret)
	{
		ALOGE("AX_IVPS_GdcWorkCreate fail, ret = %x", ret);
		return -1;
	}

	ret = AX_IVPS_GdcWorkAttrSet(handle, pGdc);
	if (ret)
	{
		ALOGE("AX_IVPS_GdcWorkAttrSet fail, ret = %x", ret);
		return -2;
	}

	nStartTime = GetTickCount();
	ret = AX_IVPS_GdcWorkRun(handle, ptSrc, &tDst);
	if (ret)
	{
		ALOGE("AX_IVPS_GdcWorkRun fail, ret = %x", ret);
		return -3;
	}
	nEndTime = GetTickCount();
	printf("GDC time cost:%lld\n", nEndTime - nStartTime);

	SaveFile(&tDst, 0, 0, pStorePath, "MapUser");

	ret = AX_POOL_ReleaseBlock(BlkId);
	if (ret)
	{
		ALOGE("Rls BlkId fail, ret=0x%x", ret);
	}

	ret = AX_IVPS_GdcWorkDestroy(handle);
	if (ret)
	{
		ALOGE("AX_IVPS_GdcWorkDestroy fail, ret = %x", ret);
		return -4;
	}
	return 0;
}

AX_S32 SAMPLE_IVPS_Dewarp(const AX_VIDEO_FRAME_T *ptFrame, char *pFilePath, SAMPLE_IVPS_DEWARP_T *pSampleDewarp)
{
	int ret;

	switch (pSampleDewarp->eSampleType)
	{
	case SAMPLE_IVPS_DEWARP_GDC:
		if (pSampleDewarp->tGdcAttr.eGdcType == AX_IVPS_GDC_FISHEYE)
		{
			ret = SAMPLE_IVPS_Fisheye(ptFrame, pFilePath, pSampleDewarp);
			if (ret)
			{
				ALOGE("SAMPLE_IVPS_Fisheye failed, ret=0x%x.", ret);
				return -1;
			}
		}
		else
		{
			ret = SAMPLE_IVPS_MapUser(ptFrame, pFilePath, &gSampleDewarpMapUser);
			if (ret)
			{
				ALOGE("SAMPLE_IVPS_MapUser failed, ret=0x%x.", ret);
				return -1;
			}
		}
		break;
	case SAMPLE_IVPS_DEWARP_SYNC:
		ret = SAMPLE_IVPS_DewarpSync(ptFrame, pFilePath, &gSampleDewarpSync);
		if (ret)
		{
			ALOGE("SAMPLE_IVPS_DewarpSync failed, ret=0x%x.", ret);
			return -1;
		}
		break;
	default:
		break;
	}
	return 0;
}