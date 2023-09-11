/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_IVPS_API_H_
#define _AX_IVPS_API_H_
#include "ax_base_type.h"
#include "ax_global_type.h"
#include "ax_sys_api.h"
#include "ax_ivps_type.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /***************************************************************************************************************/
    /*                                                   PIPELINE                                                  */
    /***************************************************************************************************************/
    typedef struct
    {
        AX_IVPS_ASPECT_RATIO_E eMode;
        /* Typical RGB color:
     1. WHITE : 0xFFFFFF
     2. BLACK : 0x000000
     3. RED   : 0xFF0000
     4. BLUE  : 0x00FF00
     5. YELLOW: 0xFF00FF
     31       23      15      7       0
     |--------|---R---|---G---|---B---|
     */
        AX_U32 nBgColor;
        AX_IVPS_ASPECT_RATIO_ALIGN_E eAligns[2]; /* IVPS_ASPECT_RATIO_ALIGN: [0]: HORIZONTAL [1]: VERTICAL */
        AX_IVPS_RECT_T tRect;                    /* should be set in ASPECT_RATIO_MANUAL mode */
    } AX_IVPS_ASPECT_RATIO_T;

    typedef enum
    {
        AX_IVPS_ENGINE_SUBSIDIARY = 0, /* do not create owner workqueue, subsidiary of the previous filter */
        AX_IVPS_ENGINE_TDP,
        AX_IVPS_ENGINE_GDC,
        AX_IVPS_ENGINE_VPP,
        AX_IVPS_ENGINE_VGP,
        AX_IVPS_ENGINE_IVE,
        AX_IVPS_ENGINE_VO,
        AX_IVPS_ENGINE_DSP,
        /* AX_IVPS_ENGINE_DPU, */ /* depth processor unit */
        AX_IVPS_ENGINE_BUTT
    } AX_IVPS_ENGINE_E;

    /*
     * IVPS_PIPELINE_X is the mainstream pipeline we currently support. The mode is as follows:
     *                            => Pipe1Filter0 Pipe1Filter1 (chn 0)
     * Pipe0Filter0 Pipe0Filter1  => Pipe2Filter0 Pipe2Filter1 (chn 1)
     *                            => Pipe3Filter0 Pipe3Filter1 (chn 2)
     * Note: Filter[n]:
     * n: channel filter type(0: RFC/CROP/SCALE/ROTATE; 1:REGION)
     */
    typedef enum
    {
        AX_IVPS_PIPELINE_DEFAULT = 0,
        AX_IVPS_PIPELINE_BUTT
    } AX_IVPS_PIPELINE_E;

    /*
     * AX_IVPS_TDP_CFG_T
     * This configuration is specific to TDP engine.
     * This engine can support many functions,
     * such as mirror, flip, rotation, scale, mosaic, osd and so on.
     * If in eCompressMode, stride and width should be 64 pixels aligned.
     */
    typedef struct
    {
        AX_IVPS_ROTATION_E eRotation;
        AX_BOOL bMirror;
        AX_BOOL bFlip;
    } AX_IVPS_TDP_CFG_T;

    /*
     * AX_IVPS_FILTER_T
     * A pipeline consists of one filter or several filters.
     * Each filter can complete specific functions. You can choose the engine to do this.
     * bInplace means drawing OSD/line/polygon/mosaic on the original frame directly.
     * Only TDP,VPP,VGP can support bInplace.
     * bInplace cannot coexist with rotation, scale, flip or mirror.
     */
    typedef struct
    {
        AX_BOOL bEngage;
        AX_IVPS_ENGINE_E eEngine;

        AX_FRAME_RATE_CTRL_T tFRC;
        AX_BOOL bCrop;
        AX_IVPS_RECT_T tCropRect;
        AX_U32 nDstPicWidth;
        AX_U32 nDstPicHeight;
        AX_U32 nDstPicStride;
        AX_IMG_FORMAT_E eDstPicFormat;
        AX_FRAME_COMPRESS_INFO_T tCompressInfo;
        AX_BOOL bInplace;
        AX_IVPS_ASPECT_RATIO_T tAspectRatio;
        union /* engine specific config data */
        {
            AX_IVPS_TDP_CFG_T tTdpCfg;
            AX_IVPS_GDC_CFG_T tGdcCfg;
        };
        AX_U32 nFRC; /* Reserved */
    } AX_IVPS_FILTER_T;

    /*
     * AX_IVPS_PIPELINE_ATTR_T
     * Dynamic attribute for the group.
     * @nOutChnNum: [1~AX_IVPS_MAX_OUTCHN_NUM] set according to output channel number.
     * @nInDebugFifoDepth: [0~1024] used by ax_ivps_dump.bin.
     * @nOutFifoDepth: [0~4] If user want to get frame from channel, set it to [1~4].
     *  Otherwise, set it to 0.
     */
    typedef struct
    {
        AX_U8 nOutChnNum;
        AX_U16 nInDebugFifoDepth;
        AX_U8 nOutFifoDepth[AX_IVPS_MAX_OUTCHN_NUM];
        AX_IVPS_FILTER_T tFilter[AX_IVPS_MAX_OUTCHN_NUM + 1][AX_IVPS_MAX_FILTER_NUM_PER_OUTCHN];
    } AX_IVPS_PIPELINE_ATTR_T;

    /*
     * AX_IVPS_GRP_ATTR_T
     * Static attribute for the group.
     * @nInFifoDepth: default 2, if the memory is tight, it's better to set it to 1.
     */
    typedef struct
    {
        AX_U8 nInFifoDepth;
        AX_IVPS_PIPELINE_E ePipeline;
    } AX_IVPS_GRP_ATTR_T;

    typedef struct
    {
        AX_FRAME_RATE_CTRL_T tFRC;
        AX_U32 nDstPicWidth;
        AX_U32 nDstPicHeight;
        AX_U32 nDstPicStride;
        AX_IMG_FORMAT_E eDstPicFormat;
        AX_IVPS_ASPECT_RATIO_T tAspectRatio;
        AX_U8 nOutFifoDepth;
        AX_U32 nFRC; /* Reserved */
    } AX_IVPS_CHN_ATTR_T;

    AX_S32 AX_IVPS_Init(AX_VOID);
    AX_S32 AX_IVPS_Deinit(AX_VOID);
    AX_S32 AX_IVPS_CreateGrp(IVPS_GRP IvpsGrp, const AX_IVPS_GRP_ATTR_T *ptGrpAttr);
    AX_S32 AX_IVPS_DestoryGrp(IVPS_GRP IvpsGrp);
    AX_S32 AX_IVPS_SetPipelineAttr(IVPS_GRP IvpsGrp, AX_IVPS_PIPELINE_ATTR_T *ptPipelineAttr);
    AX_S32 AX_IVPS_GetPipelineAttr(IVPS_GRP IvpsGrp, AX_IVPS_PIPELINE_ATTR_T *ptPipelineAttr);
    AX_S32 AX_IVPS_StartGrp(IVPS_GRP IvpsGrp);
    AX_S32 AX_IVPS_StopGrp(IVPS_GRP IvpsGrp);
    AX_S32 AX_IVPS_EnableChn(IVPS_GRP IvpsGrp, IVPS_CHN IvpsChn);
    AX_S32 AX_IVPS_DisableChn(IVPS_GRP IvpsGrp, IVPS_CHN IvpsChn);
    AX_S32 AX_IVPS_SendFrame(IVPS_GRP IvpsGrp, const AX_VIDEO_FRAME_T *ptFrame, AX_S32 nMilliSec);
    AX_S32 AX_IVPS_GetChnFrame(IVPS_GRP IvpsGrp, IVPS_CHN IvpsChn, AX_VIDEO_FRAME_T *ptFrame, AX_S32 nMilliSec);
    AX_S32 AX_IVPS_ReleaseChnFrame(IVPS_GRP IvpsGrp, IVPS_CHN IvpsChn, AX_VIDEO_FRAME_T *ptFrame);
    AX_S32 AX_IVPS_GetGrpFrame(IVPS_GRP IvpsGrp, AX_VIDEO_FRAME_T *ptFrame, AX_S32 nMilliSec);
    AX_S32 AX_IVPS_ReleaseGrpFrame(IVPS_GRP IvpsGrp, AX_VIDEO_FRAME_T *ptFrame);
    AX_S32 AX_IVPS_GetChnFd(IVPS_GRP IvpsGrp, IVPS_CHN IvpsChn);
    AX_S32 AX_IVPS_GetDebugFifoFrame(IVPS_GRP IvpsGrp, AX_VIDEO_FRAME_T *ptFrame);
    AX_S32 AX_IVPS_ReleaseDebugFifoFrame(IVPS_GRP IvpsGrp, AX_VIDEO_FRAME_T *ptFrame);
    AX_S32 AX_IVPS_CloseAllFd(AX_VOID);

    AX_S32 AX_IVPS_SetGrpLDCAttr(IVPS_GRP IvpsGrp, IVPS_FILTER IvpsFilter,
                                 const AX_IVPS_LDC_ATTR_T *ptLDCAttr);
    AX_S32 AX_IVPS_GetGrpLDCAttr(IVPS_GRP IvpsGrp, IVPS_FILTER IvpsFilter,
                                 AX_IVPS_LDC_ATTR_T *ptLDCAttr);
    AX_S32 AX_IVPS_SetChnLDCAttr(IVPS_GRP IvpsGrp, IVPS_CHN IvpsChn, IVPS_FILTER IvpsFilter,
                                 const AX_IVPS_LDC_ATTR_T *ptLDCAttr);
    AX_S32 AX_IVPS_GetChnLDCAttr(IVPS_GRP IvpsGrp, IVPS_CHN IvpsChn, IVPS_FILTER IvpsFilter,
                                 AX_IVPS_LDC_ATTR_T *ptLDCAttr);

    AX_S32 AX_IVPS_SetGrpPoolAttr(IVPS_GRP IvpsGrp, const AX_IVPS_POOL_ATTR_T *ptPoolAttr);
    AX_S32 AX_IVPS_SetChnPoolAttr(IVPS_GRP IvpsGrp, IVPS_CHN IvpsChn, const AX_IVPS_POOL_ATTR_T *ptPoolAttr);

    AX_S32 AX_IVPS_SetGrpCrop(IVPS_GRP IvpsGrp, const AX_IVPS_CROP_INFO_T *ptCropInfo);
    AX_S32 AX_IVPS_GetGrpCrop(IVPS_GRP IvpsGrp, AX_IVPS_CROP_INFO_T *ptCropInfo);
    AX_S32 AX_IVPS_SetChnAttr(IVPS_GRP IvpsGrp, IVPS_CHN IvpsChn, IVPS_FILTER IvpsFilter, const AX_IVPS_CHN_ATTR_T *ptChnAttr);
    AX_S32 AX_IVPS_GetChnAttr(IVPS_GRP IvpsGrp, IVPS_CHN IvpsChn, IVPS_FILTER IvpsFilter, AX_IVPS_CHN_ATTR_T *ptChnAttr);

    AX_S32 AX_IVPS_EnableBackupFrame(IVPS_GRP IvpsGrp, AX_U8 nFifoDepth);
    AX_S32 AX_IVPS_DisableBackupFrame(IVPS_GRP IvpsGrp);
    AX_S32 AX_IVPS_ResetGrp(IVPS_GRP IvpsGrp);

    /***************************************************************************************************************/
    /*                                               REGION                                                        */
    /***************************************************************************************************************/
    typedef AX_S32 IVPS_RGN_HANDLE;

#define AX_IVPS_MAX_RGN_HANDLE_NUM (64)
#define AX_IVPS_INVALID_REGION_HANDLE (IVPS_RGN_HANDLE)(-1)
#define AX_IVPS_REGION_MAX_DISP_NUM (32)

    typedef enum
    {
        AX_IVPS_RGN_TYPE_LINE = 0,
        AX_IVPS_RGN_TYPE_RECT = 1,
        AX_IVPS_RGN_TYPE_POLYGON = 2, /* convex quadrilateral */
        AX_IVPS_RGN_TYPE_MOSAIC = 3,
        AX_IVPS_RGN_TYPE_OSD = 4,
        AX_IVPS_RGN_TYPE_BUTT
    } AX_IVPS_RGN_TYPE_E;

    typedef struct
    {
        AX_S32 nZindex;           /* RW; if drawing OSD, for the same filter, different region handle owns different nZindex */
        AX_BOOL bSingleCanvas;    /* RW; AX_TURE: single canvas; AX_FALSE: double canvas */
        AX_U16 nAlpha;            /*RW; range: [0, 255]; 0: transparent, 255: opaque */
        AX_IMG_FORMAT_E eFormat;
        AX_BITCOLOR_T nBitColor;  /*RW; only for bitmap */
        AX_COLORKEY_T nColorKey;
    } AX_IVPS_RGN_CHN_ATTR_T;

    typedef struct
    {
        AX_POINT_T tPTs[10];      /* RW; line fixed-point coordinates */
        AX_U8 nPointNum;          /* RW; range: [2, 10] */
        AX_U32 nLineWidth;        /* RW; range: [1, 16]  */
        IVPS_RGB nColor;          /* RGB Color: 0xRRGGBB, eg: red: 0xFF0000 */
        AX_U8 nAlpha;             /* RW; range: [0, 255]; 0: transparent, 255: opaque */
    } AX_IVPS_RGN_LINE_T;

    typedef struct
    {
        union
        {
            AX_IVPS_RECT_T tRect;     /* rectangle info */
            AX_IVPS_POINT_T tPTs[10]; /* RW; polygon fixed-point coordinates */
        };
        AX_U8 nPointNum;   /* RW; range: [3, 10] */
        AX_U32 nLineWidth; /* RW; range: [1, 16] */
        IVPS_RGB nColor;   /* RW; range: [0, 0xffffff]; color RGB888; 0xRRGGBB */
        AX_U8 nAlpha;      /* RW; range: [0, 255]; 0: transparent, 255: opaque*/
        AX_BOOL bSolid;    /* if AX_TRUE, fill the rect with the nColor */
        /*
          The style of rectangle is like below
          [            ]

          [            ]
          if bCornerRect is AX_TRUE, then bSolid is always AX_FALSE
      */
        AX_BOOL bCornerRect;
    } AX_IVPS_RGN_POLYGON_T;

    typedef struct
    {
        AX_IVPS_RECT_T tRect;
        AX_IVPS_MOSAIC_BLK_SIZE_E eBklSize;
    } AX_IVPS_RGN_MOSAIC_T;

    typedef AX_OSD_BMP_ATTR_T AX_IVPS_RGN_OSD_T;

    typedef union
    {
        AX_IVPS_RGN_LINE_T tLine;
        AX_IVPS_RGN_POLYGON_T tPolygon;
        AX_IVPS_RGN_MOSAIC_T tMosaic;
        AX_IVPS_RGN_OSD_T tOSD;
    } AX_IVPS_RGN_DISP_U;

    typedef struct
    {
        AX_BOOL bShow;
        AX_IVPS_RGN_TYPE_E eType;
        AX_IVPS_RGN_DISP_U uDisp;
    } AX_IVPS_RGN_DISP_T;

    typedef struct
    {
        AX_U32 nNum;
        AX_IVPS_RGN_CHN_ATTR_T tChnAttr;
        AX_IVPS_RGN_DISP_T arrDisp[AX_IVPS_REGION_MAX_DISP_NUM];
    } AX_IVPS_RGN_DISP_GROUP_T;

    typedef struct
    {
        AX_U16 nThick;
        AX_U16 nAlpha;
        AX_U32 nColor;
        AX_BOOL bSolid;  /* if AX_TRUE, fill the rect with the nColor */
        AX_BOOL bAbsCoo; /* is Absolute Coordinate */
    } AX_IVPS_GDI_ATTR_T;

    typedef struct
    {
        AX_U64 nPhyAddr;
        AX_VOID *pVirAddr;
        AX_U32 nUVOffset; /* Pixels of Y and UV offset. If YUV420 format, nUVOffset = u64PhyAddr[1] - u64PhyAddr[0] */
        AX_U32 nStride;
        AX_U16 nW;
        AX_U16 nH;
        AX_IMG_FORMAT_E eFormat;
    } AX_IVPS_RGN_CANVAS_INFO_T;

    /*
    Create region
    @return : return the region handle created
    */
    IVPS_RGN_HANDLE AX_IVPS_RGN_Create(AX_VOID);
    /*
    Destroy created region
    @param - [IN]  hRegion: specifies the region handle created by AX_IVPS_RGN_Create
    */
    AX_S32 AX_IVPS_RGN_Destroy(IVPS_RGN_HANDLE hRegion);
    /*
    Attach region to IVPS channel
    @param - [IN]  hRegion: specifies the region handle created by AX_IVPS_RGN_Create
    @param - [IN]  IvpsGrp:  specifies the group to attach.
    @param - [IN]  IvpsFilter:  specifies the filter of the group to attach.
    */
    AX_S32 AX_IVPS_RGN_AttachToFilter(IVPS_RGN_HANDLE hRegion, IVPS_GRP IvpsGrp, IVPS_FILTER IvpsFilter);
    /*
    Detach region from IVPS channel
    @param - [IN]  hRegion: specifies the region handle created by AX_IVPS_RGN_Create
    @param - [IN]  IvpsGrp:  specifies the group to detach.
    @param - [IN]  IvpsFilter:  specifies the filter of the group to detach.
    */
    AX_S32 AX_IVPS_RGN_DetachFromFilter(IVPS_RGN_HANDLE hRegion, IVPS_GRP IvpsGrp, IVPS_FILTER IvpsFilter);
    AX_S32 AX_IVPS_RGN_Update(IVPS_RGN_HANDLE hRegion, const AX_IVPS_RGN_DISP_GROUP_T *ptDisp);

    /***************************************************************************************************************/
    /*                                             INDEPENDENT INTF                                                */
    /***************************************************************************************************************/
    /***************************************************************************************************************/
    /*                                                   TDP                                                       */
    /***************************************************************************************************************/
    AX_S32 AX_IVPS_CmmCopyTdp(AX_U64 nSrcPhyAddr, AX_U64 nDstPhyAddr, AX_U64 nMemSize);
    AX_S32 AX_IVPS_FlipAndRotationTdp(const AX_VIDEO_FRAME_T *ptSrc, AX_IVPS_CHN_FLIP_MODE_E eFlipMode,
                                      AX_IVPS_ROTATION_E eRotation, AX_VIDEO_FRAME_T *ptDst);
    AX_S32 AX_IVPS_CscTdp(const AX_VIDEO_FRAME_T *ptSrc, AX_VIDEO_FRAME_T *ptDst);
    AX_S32 AX_IVPS_CropResizeTdp(const AX_VIDEO_FRAME_T *ptSrc, AX_VIDEO_FRAME_T *ptDst,
                                 const AX_IVPS_ASPECT_RATIO_T *ptAspectRatio);
    AX_S32 AX_IVPS_CropResizeV2Tdp(const AX_VIDEO_FRAME_T *ptSrc, const AX_IVPS_RECT_T tBox[], AX_U32 nCropNum,
                                   AX_VIDEO_FRAME_T *ptDst[], const AX_IVPS_ASPECT_RATIO_T *ptAspectRatio);
    AX_S32 AX_IVPS_AlphaBlendingTdp(const AX_VIDEO_FRAME_T *ptSrc, const AX_VIDEO_FRAME_T *ptOverlay,
                                    const AX_IVPS_POINT_T tOffset, AX_U8 nAlpha, AX_VIDEO_FRAME_T *ptDst);
    AX_S32 AX_IVPS_AlphaBlendingV3Tdp(const AX_VIDEO_FRAME_T *ptSrc, const AX_OVERLAY_T *ptOverlay,
                                      AX_VIDEO_FRAME_T *ptDst);
    AX_S32 AX_IVPS_DrawOsdTdp(const AX_VIDEO_FRAME_T *ptFrame, const AX_OSD_BMP_ATTR_T arrBmp[], AX_U32 nNum);
    AX_S32 AX_IVPS_DrawMosaicTdp(const AX_VIDEO_FRAME_T *ptSrc, AX_IVPS_RGN_MOSAIC_T tMosaic[], AX_U32 nNum);
    /***************************************************************************************************************/
    /*                                                   VPP                                                       */
    /***************************************************************************************************************/
    AX_S32 AX_IVPS_CmmCopyVpp(AX_U64 nSrcPhyAddr, AX_U64 nDstPhyAddr, AX_U64 nMemSize);
    AX_S32 AX_IVPS_CropResizeVpp(const AX_VIDEO_FRAME_T *ptSrc, AX_VIDEO_FRAME_T *ptDst,
                                 const AX_IVPS_ASPECT_RATIO_T *ptAspectRatio);
    AX_S32 AX_IVPS_CropResizeV2Vpp(const AX_VIDEO_FRAME_T *ptSrc, const AX_IVPS_RECT_T tBox[], AX_U32 nCropNum,
                                   AX_VIDEO_FRAME_T *ptDst[], const AX_IVPS_ASPECT_RATIO_T *ptAspectRatio);
    AX_S32 AX_IVPS_CropResizeV3Vpp(const AX_VIDEO_FRAME_T *ptSrc, AX_VIDEO_FRAME_T *ptDst[], AX_U32 nNum,
                                   const AX_IVPS_ASPECT_RATIO_T *ptAspectRatio);
    AX_S32 AX_IVPS_CscVpp(const AX_VIDEO_FRAME_T *ptSrc, AX_VIDEO_FRAME_T *ptDst);
    AX_S32 AX_IVPS_DrawMosaicVpp(const AX_VIDEO_FRAME_T *ptSrc, AX_IVPS_RGN_MOSAIC_T tMosaic[], AX_U32 nNum);
    /***************************************************************************************************************/
    /*                                                   VGP                                                       */
    /***************************************************************************************************************/
    AX_S32 AX_IVPS_CmmCopyVgp(AX_U64 nSrcPhyAddr, AX_U64 nDstPhyAddr, AX_U64 nMemSize);
    AX_S32 AX_IVPS_CscVgp(const AX_VIDEO_FRAME_T *ptSrc, AX_VIDEO_FRAME_T *ptDst);
    AX_S32 AX_IVPS_CropResizeVgp(const AX_VIDEO_FRAME_T *ptSrc, AX_VIDEO_FRAME_T *ptDst,
                                 const AX_IVPS_ASPECT_RATIO_T *ptAspectRatio);
    AX_S32 AX_IVPS_CropResizeV2Vgp(const AX_VIDEO_FRAME_T *ptSrc, const AX_IVPS_RECT_T tBox[], AX_U32 nCropNum,
                                   AX_VIDEO_FRAME_T *ptDst[], const AX_IVPS_ASPECT_RATIO_T *ptAspectRatio);
    AX_S32 AX_IVPS_AlphaBlendingVgp(const AX_VIDEO_FRAME_T *ptSrc, const AX_VIDEO_FRAME_T *ptOverlay,
                                    const AX_IVPS_POINT_T tOffset, AX_U8 nAlpha, AX_VIDEO_FRAME_T *ptDst);
    AX_S32 AX_IVPS_AlphaBlendingV2Vgp(const AX_VIDEO_FRAME_T *ptSrc, const AX_VIDEO_FRAME_T *ptOverlay,
                                      const AX_IVPS_POINT_T tOffset, const AX_IVPS_ALPHA_LUT_T *ptSpAlpha,
                                      AX_VIDEO_FRAME_T *ptDst);
    AX_S32 AX_IVPS_AlphaBlendingV3Vgp(const AX_VIDEO_FRAME_T *ptSrc, const AX_OVERLAY_T *ptOverlay,
                                      AX_VIDEO_FRAME_T *ptDst);
    AX_S32 AX_IVPS_DrawMosaicVgp(const AX_VIDEO_FRAME_T *ptSrc, AX_IVPS_RGN_MOSAIC_T tMosaic[], AX_U32 nNum);
    /***************************************************************************************************************/
    /*                                                   CPU                                                       */
    /***************************************************************************************************************/
    AX_S32 AX_IVPS_DrawLine(const AX_IVPS_RGN_CANVAS_INFO_T *ptCanvas, AX_IVPS_GDI_ATTR_T tAttr,
                            const AX_IVPS_POINT_T tPoint[], AX_U32 nPointNum);
    AX_S32 AX_IVPS_DrawPolygon(const AX_IVPS_RGN_CANVAS_INFO_T *ptCanvas, AX_IVPS_GDI_ATTR_T tAttr,
                               const AX_IVPS_POINT_T tPoint[], AX_U32 nPointNum);
    AX_S32 AX_IVPS_DrawRect(const AX_IVPS_RGN_CANVAS_INFO_T *ptCanvas, AX_IVPS_GDI_ATTR_T tAttr,
                            AX_IVPS_RECT_T tRect);
    /***************************************************************************************************************/
    /*                                                   GDC                                                       */
    /***************************************************************************************************************/
    AX_S32 AX_IVPS_Dewarp(const AX_VIDEO_FRAME_T *ptSrc, AX_VIDEO_FRAME_T *ptDst, const AX_IVPS_DEWARP_ATTR_T *ptAttr);

    /***************************************************************************************************************/
    /*                                                  PYRA LITE                                                       */
    /***************************************************************************************************************/
    AX_S32 AX_PyraLite_Gen(const AX_PYRA_FRAME_T *tSrcFrame, AX_PYRA_FRAME_T *tDstFrame, AX_BOOL bMaskFlag);
    AX_S32 AX_PyraLite_Rcn(const AX_PYRA_FRAME_T *tSrcFrame, AX_PYRA_FRAME_T *tDstFrame, AX_BOOL bBottom);

    /*****************************************************************************
     *   Prototype    : AX_IVPS_GdcWorkCreate
     *   Description  : This API is used to returned handle ID of a work.
     *   Parameters   : pGdcHandle     handle ID of a work.
     *   Return Value : 0: Success; Error codes: Failure.
     *   Spec         :
     *****************************************************************************/
    AX_S32 AX_IVPS_GdcWorkCreate(GDC_HANDLE *pGdcHandle);

    /*****************************************************************************
     *   Prototype    : AX_IVPS_GdcWorkAttrSet
     *   Description  : This API is used to set work attributes and Prepare the necessary configuration.
     *   Parameters   : nGdcHandle     A valid handle of work. It is entered by users.
     *                  ptGdcAttr      GDC attributes
     *   Return Value : 0: Success; Error codes: Failure.
     *   Spec         :
     *****************************************************************************/
    AX_S32 AX_IVPS_GdcWorkAttrSet(GDC_HANDLE nGdcHandle, const AX_IVPS_GDC_ATTR_T *ptGdcAttr);

    /*****************************************************************************
     *   Prototype    : AX_IVPS_GdcWorkRun
     *   Description  : This API is used to start the work to process the input frame.
     *   Parameters   : nGdcHandle     A valid handle of work. It is entered by users.
     *                  ptSrc          Input frame infomation. u64PhyAddr[0], u32PicStride[0], u32Width,
     *                                 u32Height and enImgFormat must be set.
     *                  ptDst          Output frame infomation, u64PhyAddr[0] must be set.
     *   Return Value : 0: Success; Error codes: Failure.
     *   Spec         :
     *****************************************************************************/
    AX_S32 AX_IVPS_GdcWorkRun(GDC_HANDLE nGdcHandle, const AX_VIDEO_FRAME_T *ptSrc, AX_VIDEO_FRAME_T *ptDst);

    /*****************************************************************************
     *   Prototype    : AX_IVPS_GdcWorkDestroy
     *   Description  : This API is used to destroy the handle that has been created before.
     *   Parameters   : nGdcHandle      A valid handle of work. It is entered by users.
     *   Return Value : 0: Success; Error codes: Failure.
     *   Spec         :
     *****************************************************************************/
    AX_S32 AX_IVPS_GdcWorkDestroy(GDC_HANDLE nGdcHandle);

    /*****************************************************************************
     *   Prototype    : AX_IVPS_FisheyePointQueryDst2Src
     *   Description  : This API is used to find the source image coordinate point according to the fisheye correction output image coordinate point.
     *   Parameters   : ptSrcPoint               The coordinate points found on the fisheye original map.
     *                  ptDstPoint               Coordinate points on fisheye correction map that need to find mapping relationship.
     *                  nInputW                  Width of input frame.
     *                  nInputH                  Height of input frame.
     *                  nRgnIdx                  Fisheye region index.
     *                  ptFisheyeAttr            Fisheye correction Attribute.
     *   Return Value : 0: Success; Error codes: Failure.
     *   Spec         :
     *****************************************************************************/
    AX_S32 AX_IVPS_FisheyePointQueryDst2Src(AX_IVPS_POINT_T *ptSrcPoint, const AX_IVPS_POINT_T *ptDstPoint,
                                            AX_U16 nInputW, AX_U16 nInputH, AX_U8 nRgnIdx,
                                            const AX_IVPS_FISHEYE_ATTR_T *ptFisheyeAttr);
#ifdef __cplusplus
}
#endif
#endif /* _AX_IVPS_API_H_ */
