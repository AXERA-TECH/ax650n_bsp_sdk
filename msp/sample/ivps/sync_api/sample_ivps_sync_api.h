/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _SAMPLE_IVPS_SYNC_API_H_
#define _SAMPLE_IVPS_SYNC_API_H_

typedef enum
{
        IVPS_ENGINE_ID_TDP = 0,
        IVPS_ENGINE_ID_VPP,
        IVPS_ENGINE_ID_VGP,
        IVPS_ENGINE_ID_GDC,
        IVPS_ENGINE_ID_BUTT
} IVPS_ENGINE_ID_E;

typedef struct
{
        /* cmmcopy */
        IVPS_ENGINE_ID_E eEngineId;
	AX_U32 nMemSize;
        /* csc */
        /* flip and rotation */
        /* alphablending */
        /* crop resize */
        /* crop resize2 */
        /* crop resize3 */
} SAMPLE_IVPS_SYNC_API_T;

typedef struct
{
	AX_U32 nOsdNum;
} SAMPLE_IVPS_OSD_T;

typedef struct
{
	AX_U32 nCoverNum;
} SAMPLE_IVPS_COVER_T;

typedef struct
{
        SAMPLE_IVPS_OSD_T tOsd;
	SAMPLE_IVPS_COVER_T tCover;
} SAMPLE_IVPS_REGION_T;

extern SAMPLE_IVPS_SYNC_API_T gSampleIvpsSyncApi;
extern SAMPLE_IVPS_REGION_T gSampleIvpsRegion;

/* CPU */
AX_S32 SAMPLE_FillCanvas(AX_IVPS_RGN_CANVAS_INFO_T *ptCanvas);
AX_S32 SAMPLE_DrawCover(const AX_VIDEO_FRAME_T *pstVFrame, const SAMPLE_IVPS_COVER_T *ptIvpsCover, char *strFilePath);

/* NPU */
AX_S32 SAMPLE_CropResizeNpu(AX_VIDEO_FRAME_T *ptSrcFrame, char *strFilePath);

/* CmmCopy */
AX_S32 SAMPLE_IVPS_CmmCopy(IVPS_ENGINE_ID_E eEngineId, const AX_VIDEO_FRAME_T *ptSrcFrame, char *strFilePath);

/* PyraLite */
AX_S32 SAMPLE_Pyra_Gen(const AX_VIDEO_FRAME_T *ptSrcFrame, char *strFilePath, AX_BOOL bPyraMode);
AX_S32 SAMPLE_Pyra_Rcn(const AX_VIDEO_FRAME_T *ptGauFrame, AX_VIDEO_FRAME_T *ptLapFrame, char *strFilePath, AX_BOOL bPyraMode);
/* Csc */
AX_S32 SAMPLE_IVPS_Csc(IVPS_ENGINE_ID_E eEngineId, const AX_VIDEO_FRAME_T *ptSrcFrame, char *strFilePath);

/* Flip and Rotation */
AX_S32 SAMPLE_IVPS_FlipAndRotation(const AX_VIDEO_FRAME_T *ptSrcFrame,
                                   AX_S32 nFlipCode, AX_S32 nRotation, char *strFilePath);

/* AlphaBlending */
AX_S32 SAMPLE_IVPS_AlphaBlending(IVPS_ENGINE_ID_E eEngineId, const AX_VIDEO_FRAME_T *ptSrcFrame,
                                 AX_VIDEO_FRAME_T *ptOverlay, AX_U8 nAlpha, char *strFilePath);
/* AlphaBlendingV2 */
AX_S32 SAMPLE_IVPS_AlphaBlendingV2(const AX_VIDEO_FRAME_T *ptSrcFrame, AX_VIDEO_FRAME_T *ptOverlay,
                                   const AX_IVPS_ALPHA_LUT_T *ptSpAlpha, char *strFilePath);
/* AlphaBlendingV3 */
AX_S32 SAMPLE_IVPS_AlphaBlendingV3(IVPS_ENGINE_ID_E eEngineId, const AX_VIDEO_FRAME_T *ptSrcFrame,
                                 AX_VIDEO_FRAME_T *ptOverlay, AX_U8 nAlpha, char *strFilePath);
/* CropResize */
AX_S32 SAMPLE_IVPS_CropResize(IVPS_ENGINE_ID_E eEngineId, const AX_VIDEO_FRAME_T *ptSrcFrame, char *strFilePath);

/* CropResize V2 */
AX_S32 SAMPLE_IVPS_CropResizeV2(IVPS_ENGINE_ID_E eEngineId, const AX_VIDEO_FRAME_T *ptSrcFrame, AX_IVPS_RECT_T tBox[], AX_U32 nNum,
                                AX_S32 nDstStride[], AX_S32 nDstWidth[], AX_S32 nDstHeight[], char *strFilePath);

/* CropResize V3 */
AX_S32 SAMPLE_IVPS_CropResizeV3(const AX_VIDEO_FRAME_T *ptSrcFrame,
                                AX_S32 nDstStride[], AX_S32 nDstWidth[],
                                AX_S32 nDstHeight[], AX_U32 nNum, char *strFilePath);

/* Mosaic */
AX_S32 SAMPLE_IVPS_DrawMosaic(IVPS_ENGINE_ID_E eEngineId, const AX_VIDEO_FRAME_T *ptSrcFrame, AX_IVPS_RGN_MOSAIC_T tMosaic[], AX_U32 nNum, char *strFilePath);

#endif /* _SAMPLE_IVPS_SYNC_API_H_ */
