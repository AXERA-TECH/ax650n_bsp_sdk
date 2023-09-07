/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_ISP_IQ_API_H__
#define __AX_ISP_IQ_API_H__

#include "ax_base_type.h"
#include "ax_global_type.h"
#include "ax_isp_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define AX_ISP_AUTO_TABLE_MAX_NUM           (16)
#define AX_ISP_GAIN_GRP_NUM                 (16)
#define AX_ISP_EXPOSE_TIME_GRP_NUM          (10)

/************************************************************************************
 *  BLC IQ Param: SBL + GBL
 ************************************************************************************/
#define AX_ISP_BLC_GBL_IIR_SIZE             (8)

typedef struct {
    AX_U32 nSblRValue;    /* Accuracy: U8.12 Range: [0, 0xFFFFF] */
    AX_U32 nSblGrValue;   /* Accuracy: U8.12 Range: [0, 0xFFFFF] */
    AX_U32 nSblGbValue;   /* Accuracy: U8.12 Range: [0, 0xFFFFF] */
    AX_U32 nSblBValue;    /* Accuracy: U8.12 Range: [0, 0xFFFFF] */
} AX_ISP_IQ_BLC_MANUAL_T;

typedef struct {
    AX_U8 nGainGrpNum;                    /* Gain dimension num. Accuracy: U8.0 Range: [0, AX_ISP_GAIN_GRP_NUM] */
    AX_U8 nExposeTimeGrpNum;              /* ExposeTime dimension num. Accuracy: U8.0 Range: [0, AX_ISP_EXPOSE_TIME_GRP_NUM] */
    AX_U32 nGain[AX_ISP_GAIN_GRP_NUM];    /* Again value for sbl tunning. Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF] */
    AX_U32 nExposeTime[AX_ISP_EXPOSE_TIME_GRP_NUM];    /* ExposeTime value for sbl tunning. Accuracy: U32 Range: [0x0, 0xFFFFFFFF] */
    AX_U32 nAutoSblRValue[AX_ISP_GAIN_GRP_NUM][AX_ISP_EXPOSE_TIME_GRP_NUM];    /* offline sbl tunning value for R channel.  Accuracy: U8.12 Range: [0, 0xFFFFF] */
    AX_U32 nAutoSblGrValue[AX_ISP_GAIN_GRP_NUM][AX_ISP_EXPOSE_TIME_GRP_NUM];   /* offline sbl tunning value for Gr channel. Accuracy: U8.12 Range: [0, 0xFFFFF] */
    AX_U32 nAutoSblGbValue[AX_ISP_GAIN_GRP_NUM][AX_ISP_EXPOSE_TIME_GRP_NUM];   /* offline sbl tunning value for Gb channel. Accuracy: U8.12 Range: [0, 0xFFFFF] */
    AX_U32 nAutoSblBValue[AX_ISP_GAIN_GRP_NUM][AX_ISP_EXPOSE_TIME_GRP_NUM];    /* offline sbl tunning value for B channel.  Accuracy: U8.12 Range: [0, 0xFFFFF] */
} AX_ISP_IQ_BLC_AUTO_TABLE_T;

typedef struct {
    AX_ISP_IQ_BLC_AUTO_TABLE_T      tHcgAutoTable;
    AX_ISP_IQ_BLC_AUTO_TABLE_T      tLcgAutoTable;
} AX_ISP_IQ_BLC_AUTO_T;

typedef struct {
    AX_U8                           nBlcEnable;     /* sbl correction enable */
    AX_U8                           nAutoMode;      /* BLC Automode enable 0: manual, 1: auto */
    AX_ISP_IQ_BLC_MANUAL_T          tManualParam[AX_HDR_CHN_NUM];/* RGB pattern(2x2) used [0] for SDR or (0, 1, 2, 3) for HDR:(L, S, VS, VVS), RGBIR pattern(4x4)(only support Normal) used full [0-3] */
    AX_ISP_IQ_BLC_AUTO_T            tAutoParam;
} AX_ISP_IQ_BLC_PARAM_T;

typedef struct {
    AX_U8                           nGblEnable; /* GBL enable */
    AX_U8                           nGblMode;   /* GBL smooth mode sel  Accuracy: U2.0 Range: [0, 3] */
    AX_U8                           nBlcDetSel; /* 0: detect before sbl correction, 1: detect after sbl and darkshading correction */
    AX_U8                           nGblIirRate[AX_ISP_BLC_GBL_IIR_SIZE];   /* gbl time smooth ratio lut. Accuracy: U1.6  Range: [0, 0x7F] */
    AX_U16                          nGblIirTh[AX_ISP_BLC_GBL_IIR_SIZE];     /* gbl time smooth threshold lut. Accuracy: U4.12 Range: [0, 0xFFFF] */
} AX_ISP_IQ_GBL_PARAM_T;

/************************************************************************************
 *  DEPURPLE IQ Param
 ************************************************************************************/
#define AX_ISP_DEPURPLE_DET_COLOR_NUM (4)
#define AX_ISP_DEPURPLE_DET_COLOR_CH (3)
#define AX_ISP_DEPURPLE_CMP_LUMA_LUT_SIZE (8)
#define AX_ISP_DEPURPLE_CMP_HUE_LUT_SIZE (16)
#define AX_ISP_DEPURPLE_CMP_SAT_LUT_SIZE (6)

typedef struct {
    AX_U8 nDetMode;        /* Accuracy: U1 Range: [0x0, 0x1] */
    AX_U16 nDetEdgeSlope;  /* Accuracy: U1.8 Range: [0, 0x100] */
    AX_S16 nDetEdgeOffset; /* Accuracy: S1.8 Range: [-256, 0] */
    AX_S16 nDetColorCenter[AX_ISP_DEPURPLE_DET_COLOR_NUM][AX_ISP_DEPURPLE_DET_COLOR_CH]; /* Accuracy: S8.4 Range: [-4096, 4095] */
    AX_U16 nDetColorRadius[AX_ISP_DEPURPLE_DET_COLOR_NUM][AX_ISP_DEPURPLE_DET_COLOR_CH]; /* Accuracy: U8.4 Range: [0x0, 0xFFF] */
    AX_U8  nDetStrength;                                   /* Accuracy: U3.5 Range: [0x0, 0x60] */
    AX_U8  nDetGetcompcolorGrad[AX_ISP_DEPURPLE_DET_COLOR_NUM][AX_ISP_DEPURPLE_DET_COLOR_CH]; /* Accuracy: U4 Range: [0x0, 0xF] */
} AX_ISP_IQ_DEPURPLE_DET_PARAM_T;

typedef struct {
    AX_U8 nCompTargetLuma[AX_ISP_DEPURPLE_CMP_LUMA_LUT_SIZE]; /* Accuracy: U1.7 Range: [0x0, 0x80] */
    AX_U8 nCompTargetHue[AX_ISP_DEPURPLE_CMP_HUE_LUT_SIZE];   /* Accuracy: U1.7 Range: [0x0, 0x80] */
    AX_U8 nCompTargetSat[AX_ISP_DEPURPLE_CMP_SAT_LUT_SIZE];   /* Accuracy: U1.7 Range: [0x0, 0x80] */
    AX_U8 nCompUvLevel;                                       /* Accuracy: U1.7 Range: [0x0, 0x80] */
} AX_ISP_IQ_DEPURPLE_CMP_PARAM_T;

typedef struct {
    AX_U8  nParamGrpNum;                       /* Accuracy: U8.0 Range: [0x0, 0xF]  */
    AX_U32 nRefVal[AX_ISP_AUTO_TABLE_MAX_NUM]; /* Gain: Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U16 nAutoCompStrength[AX_ISP_AUTO_TABLE_MAX_NUM]; /* Accuracy: U1.7 Range: [0x0, 0x80] */
} AX_ISP_IQ_DEPURPLE_AUTO_PARAM_T;

typedef struct {
    AX_U16 nManualCompStrength; /* Accuracy: U1.7 Range: [0x0, 0x80] */
} AX_ISP_IQ_DEPURPLE_MANUAL_PARAM_T;

typedef struct {
    AX_U8 nEnable;       /* Accuracy: U1.0 Range: [0x0, 0x1] */
    AX_U8 nAutoMode;     /* Accuracy: U1.0 Range: [0x0, 0x1] */
    AX_U8 nRefMode;      /* Accuracy: U4.0 Range: [0x0, 0x1] */
    AX_U8 nDepurpleMode; /* Accuracy: U8.0 Range: [0x0, 0x1] */
    AX_ISP_IQ_DEPURPLE_DET_PARAM_T tDetParam;
    AX_ISP_IQ_DEPURPLE_CMP_PARAM_T tCmpParam;
    AX_ISP_IQ_DEPURPLE_MANUAL_PARAM_T tManualParam;
    AX_ISP_IQ_DEPURPLE_AUTO_PARAM_T tAutoParam;
} AX_ISP_IQ_DEPURPLE_PARAM_T;

/************************************************************************************
 *  HDR IQ Param
 ************************************************************************************/
#define AX_ISP_HDR_FRAME_SIZE (3)
#define AX_ISP_HDR_DGST_THRE_SIZE (2)
#define AX_ISP_HDR_DGST_LIMIT_SIZE (2)
#define AX_ISP_HDR_MASK_RATIO_SIZE (2)
#define AX_ISP_HDR_NP_LUT_SIZE (33)
#define AX_ISP_HDR_INV_NP_LUT_SIZE (19)
#define AX_ISP_HDR_EXP_LUT_SIZE (257)
#define AX_ISP_HDR_FUS_PROT_SIZE (2)

typedef struct {
    AX_U16 nFusionProtectThreshold[AX_ISP_HDR_FUS_PROT_SIZE]; /* Accuracy: U8.6 Range: [0x0, 0x3FFF] */
} AX_ISP_IQ_HDR_FUSION_PARAM_T;

typedef struct {
    AX_U16 nCoarseMotMaskRatio[AX_ISP_HDR_FRAME_SIZE][AX_ISP_HDR_MASK_RATIO_SIZE]; /* coarse motion mask ratio, Accuracy: U1.8 Range: [0x0, 0x100] */
    AX_U16 nMotMaskStrength[AX_ISP_HDR_FRAME_SIZE]; /* Accuracy: U4.12 Range: [0x0, 0xFFFF] */
    AX_U16 nMotIirRatio[AX_ISP_HDR_FRAME_SIZE][AX_ISP_HDR_MASK_RATIO_SIZE];        /* Accuracy: U1.8 Range: [0x0, 0x100] */
} AX_ISP_IQ_HDR_MOT_DET_PARAM_T;

typedef struct {
    AX_U16 nCoarseExpMaskRatio[AX_ISP_HDR_FRAME_SIZE][AX_ISP_HDR_MASK_RATIO_SIZE]; /* Accuracy: U1.8 Range: [0x0, 0x100] */
    AX_U16 nExpIirRatio[AX_ISP_HDR_FRAME_SIZE][AX_ISP_HDR_MASK_RATIO_SIZE]; /* Accuracy: U1.8 Range: [0x0, 0x100] */
    AX_U16 nExpYRatio[AX_ISP_HDR_FRAME_SIZE];                               /* Accuracy: U1.8 Range: [0x0, 0x100] */
    AX_U16 nExpWeightLut[AX_ISP_HDR_FRAME_SIZE][AX_ISP_HDR_EXP_LUT_SIZE];   /* Accuracy: U1.15 Range: [0x0, 0x8000] */
    AX_U16 nExpWeightGain[AX_ISP_HDR_FRAME_SIZE];                           /* Accuracy: U1.8 Range: [0x0, 0x100] */
} AX_ISP_IQ_HDR_EXP_MASK_PARAM_T;

typedef struct {
    AX_U8 nDeghostEnable;                               /* Accuracy: U1.0 Range: [0x0, 0x1] */
    AX_U16 nDgstStrenThre[AX_ISP_HDR_DGST_THRE_SIZE];   /* Accuracy: U1.10 Range: [0x0, 0x400] */
    AX_U16 nDgstStrenLimit[AX_ISP_HDR_DGST_LIMIT_SIZE]; /* Accuracy: U1.8 Range: [0x0, 0x100] */
    AX_U8 nDgstBaseFid;                                 /* Accuracy: U2.0 Range: [0x0, 0x2] */
    AX_U8 nDgstManualGainEn;                            /* Accuracy: U1.0 Range: [0x0, 0x1] */
    AX_U16 nDgstManualGain[AX_ISP_HDR_FRAME_SIZE];      /* Accuracy: U8.8 Range: [0x0, 0xFFFF] */
} AX_ISP_IQ_HDR_DGST_PARAM_T;

typedef struct {
    AX_U16 nCoarseMotMaskNoiseLvl[AX_ISP_HDR_FRAME_SIZE]; /* manual coarse motion mask noise level for motion detect, Accuracy: U1.11 Range: [0x0, 0x800] */
    AX_U16 nCoarseMotMaskSen[AX_ISP_HDR_FRAME_SIZE]; /* manual coarse motion mask sensitivity for motion detect, Accuracy: U1.11 Range: [0x0, 0x800] */
    AX_U16 nCoarseExpMaskSen[AX_ISP_HDR_FRAME_SIZE]; /* manual coarse exposure mask sensitivity for motion exposure mask, Accuracy: U1.11 Range: [0x0, 0x800] */
} AX_ISP_IQ_HDR_MANUAL_PARAM_T;

typedef struct {
    AX_U8  nParamGrpNum;                       /* Accuracy: U8.0 Range: [0x0, 0xF]  */
    AX_U32 nRefVal[AX_ISP_AUTO_TABLE_MAX_NUM]; /* Gain: Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U16 nCoarseMotMaskNoiseLvl[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_HDR_FRAME_SIZE]; /* auto coarse motion mask noise level, Accuracy: U1.11 Range: [0x0, 0x800] */
    AX_U16 nCoarseMotMaskSen[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_HDR_FRAME_SIZE]; /* auto coarse motion mask sensitivity, Accuracy: U1.11 Range: [0x0, 0x800] */
    AX_U16 nCoarseExpMaskSen[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_HDR_FRAME_SIZE]; /* auto coarse exposure mask sensitivity, Accuracy: U1.11 Range: [0x0, 0x800] */
} AX_ISP_IQ_HDR_AUTO_PARAM_T;

typedef struct {
    AX_U8 nEnable;    /* Accuracy: U1.0 Range: [0x0, 0x1] */
    AX_U8 nAutoMode;  /* Accuracy: U1.0 Range: [0x0, 0x1] */
    AX_U8 nRefMode;   /* Accuracy: U1.0 Range: [0x0, 0x1] */
    AX_U8 nDebugMode; /* Accuracy: U3.0 Range: [0x0, 0x7] */
    AX_U16 nNoiseLutScale; /* Accuracy: U4.12 Range: [0x0, 0xFFFF] */
    AX_ISP_IQ_HDR_MOT_DET_PARAM_T tMotDetParam;
    AX_ISP_IQ_HDR_EXP_MASK_PARAM_T tExpMaskParam;
    AX_ISP_IQ_HDR_DGST_PARAM_T tDgstParam;
    AX_ISP_IQ_HDR_FUSION_PARAM_T tFusionParam;
    AX_ISP_IQ_HDR_MANUAL_PARAM_T tHdrManualParam;
    AX_ISP_IQ_HDR_AUTO_PARAM_T tHdrAutoParam;
} AX_ISP_IQ_HDR_PARAM_T;

/************************************************************************************
 *  FPN IQ Param
 ************************************************************************************/
#define AX_ISP_FPN_MAX_PATH_SIZE           (128)
#define AX_ISP_FPN_MAX_CALI_FRM_NUM        (8)

typedef enum {
    AX_ISP_IQ_FPN_MODE0,         /* sbl + fpn */
    AX_ISP_IQ_FPN_MODE1,         /* sbl + dark shading + fpn */
    AX_ISP_IQ_FPN_MODE_MAX
} AX_ISP_IQ_FPN_MODE_E;

typedef struct {
    AX_CHAR         szFpnFrmName[AX_ISP_FPN_MAX_PATH_SIZE];  /* fpn calibration file path + name */
    AX_U8           nLcgHcgMode;    /* Accuracy: u1.0  0: hcg 1: lcg 2: not support, Range: [0x, 0x1] */
    AX_U16          nAgain;         /* Accuracy: u8.8 Range: [0x0, 0xFFFF] */
    AX_U16          nIntTime;       /* Accuracy: u16.0 Range: [0x0, 0xFFFF] */
    AX_U16          nTemperature;   /* Accuracy: u16.0 Range: [0x, 0xFFFF] */
    AX_U32          nFrameSize;
    AX_U8           nFpnCorrGain;   /* Accuracy: u1.7 Range:[0x0, 0xFF]*/
    AX_U8           nFpnOffset;     /* Accuracy: u4.4 Range:[0x0, 0xFF]*/
} AX_ISP_IQ_FPN_FRAME_INFO_T;

typedef struct {
    AX_ISP_IQ_FPN_FRAME_INFO_T tFpnCaliFrame;
} AX_ISP_IQ_FPN_MANUAL_T;

typedef struct {
    AX_U32                     nCaliFrameNum;
    AX_ISP_IQ_FPN_FRAME_INFO_T tFpnCaliFrame[AX_ISP_FPN_MAX_CALI_FRM_NUM];
} AX_ISP_IQ_FPN_AUTO_TABLE_T;

typedef struct {
    AX_ISP_IQ_FPN_AUTO_TABLE_T      tHcgFpnCaliFrame;
    AX_ISP_IQ_FPN_AUTO_TABLE_T      tLcgFpnCaliFrame;
} AX_ISP_IQ_FPN_AUTO_T;

typedef struct {
    AX_BOOL                         nFpnEnable;         /* fpn enable */
    AX_U8                           nAutoMode;          /* 0: manual, 1: auto */
    AX_ISP_IQ_FPN_MODE_E            eFpnMode;           /* fpn mode, Accuracy: U1.0 Range: [0x0, 0x1] */
    AX_ISP_IQ_FPN_MANUAL_T          tManualParam;
    AX_ISP_IQ_FPN_AUTO_T            tAutoParam;
} AX_ISP_IQ_FPN_PARAM_T;

/************************************************************************************
 *  DarkSharding IQ Param
 ************************************************************************************/
#define AX_ISP_BLC_MESH_SIZE_H              (72)
#define AX_ISP_BLC_MESH_SIZE_V              (72)

typedef struct {
    AX_U16 nMeshLut[AX_ISP_BLC_MESH_SIZE_H][AX_ISP_BLC_MESH_SIZE_V];    /* mesh lut for the specific Again and ExposeTime. Accuracy: U8.8 Range: [0, 0xFFFF] */
} AX_ISP_IQ_DS_MESH_LUT_T;

typedef struct {
    AX_U8  nGainGrpNum;                               /* Gain dimension num. Accuracy: U8.0 Range: [0, AX_ISP_GAIN_GRP_NUM] */
    AX_U8  nExposeTimeGrpNum;                         /* ExposeTime dimension num. Accuracy: U8.0 Range: [0, AX_ISP_EXPOSE_TIME_GRP_NUM] */
    AX_U32 nGain[AX_ISP_GAIN_GRP_NUM];                /* Again value for sbl tunning. Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF] */
    AX_U32 nExposeTime[AX_ISP_EXPOSE_TIME_GRP_NUM];   /* ExposeTime value for sbl tunning. Accuracy: U32 Range: [0x0, 0xFFFFFFFF] */
    AX_ISP_IQ_DS_MESH_LUT_T tMeshTab[AX_ISP_GAIN_GRP_NUM][AX_ISP_EXPOSE_TIME_GRP_NUM];    /* Dark Shading mesh table */
} AX_ISP_IQ_DS_AUTO_TABLE_T;

typedef struct {
    AX_ISP_IQ_DS_AUTO_TABLE_T       tHcgAutoTable;
    AX_ISP_IQ_DS_AUTO_TABLE_T       tLcgAutoTable;
} AX_ISP_IQ_DS_AUTO_T;

typedef struct {
    AX_ISP_IQ_DS_MESH_LUT_T         tMeshTab;       /* Dark Shading mesh table */
} AX_ISP_IQ_DS_MANUAL_T;

typedef struct {
    AX_U8  nDsEnable;                               /* Dark Shading enable */
    AX_U8  nAutoMode;                               /* 0: manual, 1: auto */
    AX_ISP_IQ_DS_MANUAL_T           tManualParam;
    AX_ISP_IQ_DS_AUTO_T             tAutoParam;
} AX_ISP_IQ_DS_PARAM_T;

/************************************************************************************
 *  DPC IQ Param
 ************************************************************************************/
#define AX_ISP_DPC_NOISE_SDPC_RATIO_NUM     (2)
#define AX_ISP_DPC_NOISE_PARAM_NUM          (4)
#define AX_ISP_DPC_SDPC_BUFFER_MAX          (8192)
#define AX_ISP_DPC_DYNAMIC_STATIC_PDAF_NUM  (2)
#define AX_ISP_AUTO_TABLE_MAX_NUM           (16)
#define AX_ISP_DPC_MARGIN_LIMIT_NUM         (2)
#define AX_ISP_DPC_MARGIN_NUM               (2)

typedef struct{
    AX_U16 nUpperLimit;
    AX_U16 nLowerLimit;
}AX_ISP_IQ_DPC_MARGIN_T;

typedef struct {
    AX_U16 nNoiseRatio[AX_ISP_DPC_NOISE_SDPC_RATIO_NUM];           /* Accuracy: U8.6 Range: [0, 0x27FF]*/
    AX_U8  nDpType;                                                /* Defective Pixel Type. Accuracy: U1.0 Range: [0, 1] */
    AX_U8  nNonChwiseEn;                                           /* Non Chwise Enable. Accuracy: U1.0 Range: [0, 1] */
    AX_U8  nChwiseStr;                                             /* Accuracy: U1.4 Range: [0, 0x1F] */
    AX_U8  nDetCoarseStr;                                          /* Accuracy: U4.4 Range: [0, 0xFF] */
    AX_U8  nDetFineStr;                                            /* Accuracy: U0.6 Range: [0, 0x3F] */
    AX_U16 nDynamicDpcStr;                                         /* Accuracy: U4.8 Range: [0, 0xFFF] */
    AX_U8  nEdgeStr;                                               /* Accuracy: U1.7 Range: [0, 0xFF] */
    AX_U8  nHotColdTypeStr;                                        /* Accuracy: U1.7 Range: [0, 0x80] */
    AX_U8  nSupWinkStr;                                            /* Accuracy: U4.4 Range: [0, 0xFF] */
    AX_ISP_IQ_DPC_MARGIN_T nDynamicDpClrLimOffset;                 /* Accuracy: U8.6 Range: [0, 0x3FFF] */
    AX_ISP_IQ_DPC_MARGIN_T nStaticDpClrLimOffset;                  /* Accuracy: U8.6 Range: [0, 0x3FFF] */
    AX_ISP_IQ_DPC_MARGIN_T nNormalPixDpClrLimOffset;               /* Accuracy: U8.6 Range: [0, 0x3FFF] */
    AX_U8  nDynamicDpClrLimStr;                                    /* Accuracy: U1.7 Range: [0, 0x80] */
    AX_U8  nStaticDpClrLimStr;                                     /* Accuracy: U1.7 Range: [0, 0x80] */
    AX_U8  nNormalPixDpClrLimStr;                                  /* Accuracy: U1.7 Range: [0, 0x80] */
} AX_ISP_IQ_DPC_MANUAL_T;

typedef struct {
    AX_U8  nParamGrpNum;                                        /* Accuracy: U8.0 Range:[0, AX_ISP_AUTO_TABLE_MAX_NUM] */
    AX_U32 nRefVal[AX_ISP_AUTO_TABLE_MAX_NUM];                  /* Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U16 nNoiseRatio[AX_ISP_DPC_NOISE_SDPC_RATIO_NUM][AX_ISP_AUTO_TABLE_MAX_NUM];              /* Accuracy: U8.6 Range: [0, 0x27FF]*/
    AX_U8  nDpType[AX_ISP_AUTO_TABLE_MAX_NUM];                  /* Defective Pixel Type. Accuracy: U1.0 Range: [0, 1] */
    AX_U8  nNonChwiseEn[AX_ISP_AUTO_TABLE_MAX_NUM];             /* Non Chwise Enable. Accuracy: U1.0 Range: [0, 1] */
    AX_U8  nChwiseStr[AX_ISP_AUTO_TABLE_MAX_NUM];               /* Accuracy: U1.4 Range: [0, 0x1F] */
    AX_U8  nDetCoarseStr[AX_ISP_AUTO_TABLE_MAX_NUM];            /* Accuracy: U4.4 Range: [0, 0xFF] */
    AX_U8  nDetFineStr[AX_ISP_AUTO_TABLE_MAX_NUM];              /* Accuracy: U0.6 Range: [0, 0x3F] */
    AX_U16 nDynamicDpcStr[AX_ISP_AUTO_TABLE_MAX_NUM];           /* Accuracy: U4.8 Range: [0, 0xFFF] */
    AX_U8  nEdgeStr[AX_ISP_AUTO_TABLE_MAX_NUM];                 /* Accuracy: U1.7 Range: [0, 0xFF] */
    AX_U8  nHotColdTypeStr[AX_ISP_AUTO_TABLE_MAX_NUM];          /* Accuracy: U1.7 Range: [0, 0x80] */
    AX_U8  nSupWinkStr[AX_ISP_AUTO_TABLE_MAX_NUM];              /* Accuracy: U4.4 Range: [0, 0xFF] */
    AX_ISP_IQ_DPC_MARGIN_T nDynamicDpClrLimOffset[AX_ISP_AUTO_TABLE_MAX_NUM];        /* Accuracy: U8.6 Range: [0, 0x3FFF] */
    AX_ISP_IQ_DPC_MARGIN_T nStaticDpClrLimOffset[AX_ISP_AUTO_TABLE_MAX_NUM];         /* Accuracy: U8.6 Range: [0, 0x3FFF] */
    AX_ISP_IQ_DPC_MARGIN_T nNormalPixDpClrLimOffset[AX_ISP_AUTO_TABLE_MAX_NUM];      /* Accuracy: U8.6 Range: [0, 0x3FFF] */
    AX_U8  nDynamicDpClrLimStr[AX_ISP_AUTO_TABLE_MAX_NUM];      /* Accuracy: U1.7 Range: [0, 0x80] */
    AX_U8  nStaticDpClrLimStr[AX_ISP_AUTO_TABLE_MAX_NUM];       /* Accuracy: U1.7 Range: [0, 0x80] */
    AX_U8  nNormalPixDpClrLimStr[AX_ISP_AUTO_TABLE_MAX_NUM];    /* Accuracy: U1.7 Range: [0, 0x80] */
} AX_ISP_IQ_DPC_AUTO_T;

typedef struct {
    AX_U32 nSdpcLength;                                     /* Accuracy: U32 Range: [0, AX_ISP_DPC_SBPC_BUFFER_MAX] */
    AX_U32 nSdpcBuffer[AX_ISP_DPC_SDPC_BUFFER_MAX];         /* Accuracy: U32 Range: [0, 0xFFFFFFFF] */
    AX_S32 nShotNoiseCoeffsA[AX_ISP_DPC_NOISE_PARAM_NUM];   /* Accuracy: S0.31 Range: [-0x7FFFFFFF, 0x7FFFFFFF] */
    AX_S32 nShotNoiseCoeffsB[AX_ISP_DPC_NOISE_PARAM_NUM];   /* Accuracy: S0.31 Range: [-0x7FFFFFFF, 0x7FFFFFFF] */
    AX_S32 nReadNoiseCoeffsA[AX_ISP_DPC_NOISE_PARAM_NUM];   /* Accuracy: S0.31 Range: [-0x7FFFFFFF, 0x7FFFFFFF] */
    AX_S32 nReadNoiseCoeffsB[AX_ISP_DPC_NOISE_PARAM_NUM];   /* Accuracy: S0.31 Range: [-0x7FFFFFFF, 0x7FFFFFFF] */
    AX_S32 nReadNoiseCoeffsC[AX_ISP_DPC_NOISE_PARAM_NUM];   /* Accuracy: S0.31 Range: [-0x7FFFFFFF, 0x7FFFFFFF] */
} AX_ISP_DPC_NOISE_SBPC_T;

typedef struct {
    AX_ISP_DPC_NOISE_SBPC_T         tHcgTable;
    AX_ISP_DPC_NOISE_SBPC_T         tLcgTable;
} AX_ISP_DPC_TABLE_T;

typedef struct {
    AX_U8  nDpcEnable;                                      /* Accuracy: U1.0 Range: [0, 1]   */
    AX_U8  nStaticDpcEnable;                                /* Accuracy: U1.0 Range: [0, 1] */
    AX_U8  nDynamicDpcEnable;                               /* Accuracy: U1.0 Range: [0, 1] */
    AX_U32 nPreDetLevelSlope;                               /* Predet Level Slope.Accuracy: U0.8 Range: [0, 0xFF] */
    AX_U32 nPreDetLevelOffset;                              /* Predet Level Offset.Accuracy: U4.6 Range: [0, 0x3FF] */
    AX_U8  nColorLimitEnable;                               /* Color Limit Enable. Accuracy: U1.0 Range: [0, 1] */
    AX_U8  nAutoMode;                                       /* Accuracy: U1.0 Range: [0, 1] */
    AX_U8  nRefMode;                                        /* Accuracy: U1.0 Range: [0, 1] */
    AX_ISP_DPC_TABLE_T              tDpcParam;
    AX_ISP_IQ_DPC_MANUAL_T          tManualParam;
    AX_ISP_IQ_DPC_AUTO_T            tAutoParam;
} AX_ISP_IQ_DPC_PARAM_T;

/************************************************************************************
 *  LSC IQ Param
 ************************************************************************************/
#define AX_ISP_LSC_MESH_SIZE_V              (52)
#define AX_ISP_LSC_MESH_SIZE_H              (98)
#define AX_ISP_LSC_MESH_POINTS              (AX_ISP_LSC_MESH_SIZE_V * AX_ISP_LSC_MESH_SIZE_H)
#define AX_ISP_LSC_AUTO_COLOR_TEMP_MAX_NUM  (10)

typedef struct {
    AX_U8                        nLumaRatio;                                    /* Accuacy: U8 Range: [0, 100] */
    AX_U8                        nColorRatio;                                   /* Accuacy: U8 Range: [0, 100] */
    AX_U32                       nLumaMeshLut[AX_ISP_LSC_MESH_SIZE_V][AX_ISP_LSC_MESH_SIZE_H];  /* Accuacy U4.14 Range: [0x4000, 0x3FFFF] */
    AX_U32                       nRRMeshLut[AX_ISP_LSC_MESH_SIZE_V][AX_ISP_LSC_MESH_SIZE_H];    /* Accuacy U4.14 Range: [0x4000, 0x3FFFF] */
    AX_U32                       nGRMeshLut[AX_ISP_LSC_MESH_SIZE_V][AX_ISP_LSC_MESH_SIZE_H];    /* Accuacy U4.14 Range: [0x4000, 0x3FFFF] */
    AX_U32                       nGBMeshLut[AX_ISP_LSC_MESH_SIZE_V][AX_ISP_LSC_MESH_SIZE_H];    /* Accuacy U4.14 Range: [0x4000, 0x3FFFF] */
    AX_U32                       nBBMeshLut[AX_ISP_LSC_MESH_SIZE_V][AX_ISP_LSC_MESH_SIZE_H];    /* Accuacy U4.14 Range: [0x4000, 0x3FFFF] */
} AX_ISP_IQ_LSC_MANUAL_T;

typedef struct{
    AX_U8                        nParamGrpNum;                                /* Luma Grp Num; Accuacy: U8 Range: [0, AX_ISP_AUTO_TABLE_MAX_NUM] */
    AX_U32                       nRefValStart[AX_ISP_AUTO_TABLE_MAX_NUM];     /* Ref Gain Start: Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF];Ref Lux Start: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U32                       nRefValEnd[AX_ISP_AUTO_TABLE_MAX_NUM];       /* Ref Gain End: Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF];Ref Lux End: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U8                        nLumaRatio[AX_ISP_AUTO_TABLE_MAX_NUM];       /* Luma Ratio; Accuacy: U8 Range: [0, 100] */
    AX_U32                       nLumaMeshLut[AX_ISP_LSC_MESH_SIZE_V][AX_ISP_LSC_MESH_SIZE_H];    /* Accuacy U4.14 Range: [0x4000, 0x3FFFF] */
} AX_ISP_IQ_LSC_LUMA_PARAM_T;

typedef struct{
    AX_U8                        nColTempNum;      /*Calib Color Temp Num; Accuracy: U8 Range: [0, AX_ISP_LSC_AUTO_COLOR_TEMP_MAX_NUM] */
    AX_U32                       nRefColorTempStart[AX_ISP_LSC_AUTO_COLOR_TEMP_MAX_NUM];    /* Ref CCT Start; Accuracy: U32.0 Range: [0, 100000]*/
    AX_U32                       nRefColorTempEnd[AX_ISP_LSC_AUTO_COLOR_TEMP_MAX_NUM];      /* Ref CCT End; Accuracy: U32.0 Range: [0, 100000]*/
    AX_U32                       nColorTemp[AX_ISP_LSC_AUTO_COLOR_TEMP_MAX_NUM];            /* Calib CCT; Accuracy: U32.0 Range: [0, 100000] */
    AX_U8                        nColorRatio[AX_ISP_LSC_AUTO_COLOR_TEMP_MAX_NUM];           /* Color Ratio; Accuacy: U8 Range: [0, 100] */
    AX_U32                       nRRMeshLut[AX_ISP_LSC_AUTO_COLOR_TEMP_MAX_NUM][AX_ISP_LSC_MESH_SIZE_V][AX_ISP_LSC_MESH_SIZE_H]; /* Accuacy U4.14 Range: [0x4000, 0x3FFFF] */
    AX_U32                       nGRMeshLut[AX_ISP_LSC_AUTO_COLOR_TEMP_MAX_NUM][AX_ISP_LSC_MESH_SIZE_V][AX_ISP_LSC_MESH_SIZE_H]; /* Accuacy U4.14 Range: [0x4000, 0x3FFFF] */
    AX_U32                       nGBMeshLut[AX_ISP_LSC_AUTO_COLOR_TEMP_MAX_NUM][AX_ISP_LSC_MESH_SIZE_V][AX_ISP_LSC_MESH_SIZE_H]; /* Accuacy U4.14 Range: [0x4000, 0x3FFFF] */
    AX_U32                       nBBMeshLut[AX_ISP_LSC_AUTO_COLOR_TEMP_MAX_NUM][AX_ISP_LSC_MESH_SIZE_V][AX_ISP_LSC_MESH_SIZE_H]; /* Accuacy U4.14 Range: [0x4000, 0x3FFFF] */
} AX_ISP_IQ_LSC_CT_PARAM_T;

typedef struct {
    AX_U8                        nDampRatio;        /* Damp Ratio; Accuacy: U8 Range: [0, 100] */
    AX_U8                        nToleranceRatio;   /* Tolerance Ratio; Accuacy: U8 Range: [0, 100] */
    AX_ISP_IQ_LSC_LUMA_PARAM_T   tLumaParam;        /* Luma Params */
    AX_ISP_IQ_LSC_CT_PARAM_T     tColTempParam;     /* Color Temp Params */
} AX_ISP_IQ_LSC_AUTO_T;

typedef struct {
    AX_U8                        nLscEn;            /* Acuracy: U8 Range: [0, 1] */
    AX_U8                        nRefMode;          /* choose ref mode, Accuracy: U8 Range: [0, 1], 0: use lux as ref, 1: use gain as ref */
    AX_U8                        nMeshMode;         /* mesh mode, Accuracy: U8 Range: [0, 1], 1: mirror mode, 0: normal mode */
    AX_U8                        nAutoMode;         /* for ref auto or manual adjust mode, Accuracy: U8 Range: [0, 1], 0: manual, 1:auto, default:1 */
    AX_U8                        nMeshRows;         /* status of valid mesh rows number; Accuracy: U8 Range: [0, 52] */
    AX_U8                        nMeshCols;         /* status of valid mesh columns number; Accuracy:U8 Range: [0, 98] */
    AX_ISP_IQ_LSC_MANUAL_T       tManualParam;
    AX_ISP_IQ_LSC_AUTO_T         tAutoParam;
} AX_ISP_IQ_LSC_PARAM_T;

#define AX_ISP_RLTM_SCURVE_MAX_LEN              (1025)
#define AX_ISP_RLTM_HISTOGRAM_WEIGHT_MAX_LEN    (63)
#define AX_ISP_RLTM_HISTOGRAM_WEIGHT_NUM        (16)
#define AX_ISP_RLTM_HIST_REGION_NUM             (4)
#define AX_ISP_RLTM_LUMA_WEIGHT_NUM             (5)

/************************************************************************************
 *  RLTM IQ Param
 ************************************************************************************/
typedef struct {
    AX_U16 nAlpha;          /* Accuracy: U1.15 Range: [0, 32768], default:512 */
    AX_U8  nReset;          /* Accuracy: U1 Range: [0, 1] */
    AX_U8  nStopUpdating;   /* Accuracy: U1 Range: [0, 1] */
} AX_ISP_IQ_RLTM_TEMPO_FILTER_T;

typedef struct {
    AX_U8 nLumaWeight[AX_ISP_RLTM_LUMA_WEIGHT_NUM]; /* luma weight. (R, Gr, Gb, B, Max). Accuracy: U1.7 Range: [0, 128], default (0, 0, 0, 0, 128)*/
} AX_ISP_IQ_RLTM_LUMAWT_T;

typedef struct {
    AX_U16 nHistogramWeight[AX_ISP_RLTM_HISTOGRAM_WEIGHT_MAX_LEN]; /* histogram bin weights. Accuracy: U16 Range: [0, 65535], default 1*/
} AX_ISP_IQ_RLTM_HISTWT_T;

typedef struct {
    AX_U16 nTop;        /* Accuracy: U16 Range: [0, 8192] */
    AX_U16 nBottom;     /* Accuracy: U16 Range: [0, 8192] */
    AX_U16 nLeft;       /* Accuracy: U16 Range: [0, 8192] */
    AX_U16 nRight;      /* Accuracy: U16 Range: [0, 8192] */
} AX_ISP_IQ_RLTM_ROI_T;

typedef struct {
    AX_U8 nMode;                /* rltm base&advance mode. Accuracy:U8 Range: [0, 1] */
    AX_U8 nRegionNum;           /* valide region number. Accuracy:U8 Range: [0, 4] */
    AX_U8 nHistWtNum;           /* hist weight number. Accuracy:U8 Range: [1, 16] */
    AX_ISP_IQ_RLTM_ROI_T    tRoi;
    AX_ISP_IQ_RLTM_HISTWT_T tHistWt;
    AX_U8 nFlagHistId[AX_ISP_RLTM_HISTOGRAM_WEIGHT_NUM][AX_ISP_RLTM_HISTOGRAM_WEIGHT_NUM]; /* Read Only. Accuracy: U8 Range: [0, 1] */
} AX_ISP_IQ_RLTM_MUL_HISTWT_T;

typedef struct {
    AX_U8  nLocalFactor;        /* Accuracy: U1.7 Range: [0, 128], default 90 */
    AX_U8  nHighlightSup;       /* highlight suppress. Accuracy: U5.3 Range: [0, 255], default 50 */
    AX_U16 nKMax;               /* limit brightness. Accuracy: U8.8 Range: [256, 65535], default 1024 */
    AX_U8  nPreGamma;           /* for gamma lut. Accuracy: U3.5 Range: [32, 255], default 32 */
    AX_U8  nPostGamma;          /* for invgamma lut. Accuracy: U3.5 Range: [32, 255], default 64 */
    AX_U8  nDynamicRangeUpper;  /* for dynamic range upper, not dependence on effect_strength. Accuracy: U1.7 Range: [90, 128], default 128 */
    AX_U8  nDynamicRangeLower;  /* for dynamic range lower, not dependence on effect_strength. Accuracy: U1.7 Range: [0, 40], default 0 */
    AX_U16 nExtraDgain;         /* for invgamma lut. Accuracy: U4.4 Range: [16, 255], default 16 */
    AX_U16 nWinSize;            /* for hist. Accuracy: U16.0 Range: [128, 256, 512, 1024, 2048], default 512 */
    AX_U8  nRltmStrength;       /* Accuracy: U1.7 Range: [0, 128], default 64 */
    AX_U8  nLog10Offset;        /* log10 offset. Accuracy: U3.5 Range: [0, 211], default 0 */
    AX_U8  nContrastStrength;   /* contrast strength. Accuracy: U1.7 Range: [0, 255], default 42 */
    AX_U16 nBaseGain;           /* base gain. Accuracy: U10.6 Range: [1, 65535], default 64 */
    AX_U8  nDitherMode;         /* 0: no-dither, 1: before pre-gamma, 2: after pre-gamma 0. Accuracy: U2.0 Range: [0, 2], default 0 */
    AX_U16 nDitherScale;       /* for dither strength. Accuracy: U10.6 Range: [0, 65535], default 64 */
    AX_U8  nGtmSwEn;            /* gtm software switch. Accuracy: U1.0 Range: [0, 1], default 0 */
    AX_U16 nGtmSwDgain;         /* gtm dgain for software gtm curve. Accuracy: U8.8 Range: [256, 65535], default 256 */
    AX_U8  nHistWtBrightLow[AX_ISP_RLTM_HIST_REGION_NUM];   /* Hist weight Lower limit of brightness range. Actual nums deps nRegionNum. Accuracy: U8 Range: [0, 62] */
    AX_U8  nHistWtBrightHigh[AX_ISP_RLTM_HIST_REGION_NUM];  /* Hist weight Upper limit of brightness range. Actual nums deps nRegionNum. Accuracy: U8 Range: [0, 62] */
    AX_U8  nHistWtThreshold[AX_ISP_RLTM_HIST_REGION_NUM];   /* Hist weight threshod. Actual nums deps nRegionNum. Accuracy: U1.7 Range: [0, 129] */
    AX_U16 nSCurveList[AX_ISP_RLTM_SCURVE_MAX_LEN];         /* s curve lut. Accuracy: U1.15 Range: [0, 32768] */
    AX_ISP_IQ_RLTM_HISTWT_T tHistWt[AX_ISP_RLTM_HISTOGRAM_WEIGHT_NUM];
} AX_ISP_IQ_RLTM_MANUAL_T;

typedef struct {
    AX_U8  nParamGrpNum;                                 /* Accuracy: U8 Range: [0, AX_ISP_AUTO_TABLE_MAX_NUM]  */
    AX_U32 nRefVal[AX_ISP_AUTO_TABLE_MAX_NUM];           /* Gain: Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U8  nLocalFactor[AX_ISP_AUTO_TABLE_MAX_NUM];      /* Accuracy: U1.7 Range: [0, 128], default 90 */
    AX_U8  nHighlightSup[AX_ISP_AUTO_TABLE_MAX_NUM];     /* highlight suppress. Accuracy: U5.3 Range: [0, 255], default 50 */
    AX_U16 nKMax[AX_ISP_AUTO_TABLE_MAX_NUM];             /* limit brightness. Accuracy: U8.8 Range: [256, 65535], default 1024 */
    AX_U8  nPreGamma[AX_ISP_AUTO_TABLE_MAX_NUM];         /* for gamma lut. Accuracy: U3.5 Range: [32, 255], default 32 */
    AX_U8  nPostGamma[AX_ISP_AUTO_TABLE_MAX_NUM];        /* for invgamma lut. Accuracy: U3.5 Range: [32, 255], default 64 */
    AX_U8  nDynamicRangeUpper[AX_ISP_AUTO_TABLE_MAX_NUM];/* for dynamic range upper, not dependence on effect_strength. Accuracy: U1.7 Range: [90, 128], default 128 */
    AX_U8  nDynamicRangeLower[AX_ISP_AUTO_TABLE_MAX_NUM];/* for dynamic range lower, not dependence on effect_strength. Accuracy: U1.7 Range: [0, 40], default 0 */
    AX_U16 nExtraDgain[AX_ISP_AUTO_TABLE_MAX_NUM];       /* for invgamma lut. Accuracy: U4.4 Range: [16, 255], default 16 */
    AX_U16 nWinSize[AX_ISP_AUTO_TABLE_MAX_NUM];          /* for hist. Accuracy: U16.0 Range: [128, 256, 512, 1024, 2048], default 512 */
    AX_U8  nRltmStrength[AX_ISP_AUTO_TABLE_MAX_NUM];     /* Accuracy: U1.7 Range: [0, 128], default 64 */
    AX_U8  nLog10Offset[AX_ISP_AUTO_TABLE_MAX_NUM];      /* log10 offset. Accuracy: U3.5 Range: [0, 211], default 0 */
    AX_U8  nContrastStrength[AX_ISP_AUTO_TABLE_MAX_NUM]; /* contrast strength. Accuracy: U1.7 Range: [0, 255], default 42 */
    AX_U16 nBaseGain[AX_ISP_AUTO_TABLE_MAX_NUM];         /* base gain. Accuracy: U10.6 Range: [1, 65535], default 64 */
    AX_U8  nDitherMode[AX_ISP_AUTO_TABLE_MAX_NUM];       /* 0: no-dither, 1: before pre-gamma, 2: after pre-gamma 0. Accuracy: U2.0 Range: [0, 2], default 0 */
    AX_U16 nDitherScale[AX_ISP_AUTO_TABLE_MAX_NUM];     /* for dither strength. Accuracy: U10.6 Range: [0, 65535], default 64 */
    AX_U8  nGtmSwEn[AX_ISP_AUTO_TABLE_MAX_NUM];          /* gtm software switch. Accuracy: U1.0 Range: [0, 1], default 0 */
    AX_U16 nGtmSwDgain[AX_ISP_AUTO_TABLE_MAX_NUM];       /* gtm dgain for software gtm curve. Accuracy: U8.8 Range: [256, 65535], default 256 */
    AX_U8  nHistWtBrightLow[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RLTM_HIST_REGION_NUM];    /* Hist weight Lower limit of brightness range. Actual nums deps nRegionNum. Accuracy: U8 Range: [0, 62] */
    AX_U8  nHistWtBrightHigh[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RLTM_HIST_REGION_NUM];   /* Hist weight Upper limit of brightness range. Actual nums deps nRegionNum. Accuracy: U8 Range: [0, 62] */
    AX_U8  nHistWtThreshold[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RLTM_HIST_REGION_NUM];    /* Hist weight threshod. Actual nums deps nRegionNum. Accuracy: U1.7 Range: [0, 129] */
    AX_U16 nSCurveList[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RLTM_SCURVE_MAX_LEN];          /* s curve lut. Accuracy: U1.15 Range: [0, 32768] */
    AX_ISP_IQ_RLTM_HISTWT_T tHistWt[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RLTM_HISTOGRAM_WEIGHT_NUM];
} AX_ISP_IQ_RLTM_AUTO_T;

typedef struct {
    AX_U8  nRltmEn;             /* rltm en -- module control */
    AX_U8  nMultiCamSyncMode;   /* 0：INDEPEND MODE; 1: MASTER SLAVE MODE; 2: OVERLAP MODE */
    AX_U8  nAutoMode;           /* for ref auto or manual adjust mode, [0,1], 0: manual, 1:auto, default:1 */
    AX_U8  nRefMode;            /* choose ref mode, [0,1], 0:use lux as ref, 1:use gain as ref */
    AX_ISP_IQ_RLTM_TEMPO_FILTER_T tTempoFilter;
    AX_ISP_IQ_RLTM_LUMAWT_T       tLumaWt;
    AX_ISP_IQ_RLTM_MUL_HISTWT_T   tMultiHistWt;
    AX_ISP_IQ_RLTM_MANUAL_T       tManualParam;
    AX_ISP_IQ_RLTM_AUTO_T         tAutoParam;
} AX_ISP_IQ_RLTM_PARAM_T;

/************************************************************************************
 *  Demosaic IQ Param
 ************************************************************************************/
#define AX_ISP_DEMOSAIC_GAMMA_LUT_SIZE      (8)

typedef struct {
    AX_U8   nDemosaicEn;                 /* Demosaic module enable.  Range: [0, 1], 0: Disable, 1: Enable */
    AX_U8   nEdgeDirectEstStrength;      /* Edge direction estimation strength.  Accuracy:U2.6  Range:[0x0,0xFF] */
    AX_U8   nFcCorEnable;                /* False color correction enable.  default:1  Range:[0,1] */
    AX_U8   nFcCorUvLevel;               /* False color correction uv level.  Accuracy: U1.4 Range:[0x0,0x1F] */
    AX_U16  nFcCorLevel;                 /* False color correction weight range. Accuracy:U1.8  Range:[0x0,0x1FF] */
} AX_ISP_IQ_DEMOSAIC_PARAM_T;


/************************************************************************************
 *  GIC IQ Param
 ************************************************************************************/
#define AX_ISP_GIC_STRENGTH_LUT_SIZE        (9)
typedef struct {
    AX_U8   nGicEnable;                                      /* Green imbalance correction enable. range:[0,1]  0: Disable  1: Enable */
    AX_U8   nGicStrengthLut[AX_ISP_GIC_STRENGTH_LUT_SIZE];   /* Green imbalance correction strength lut. Accuracy:U1.7  Range: [0x0,0x80] */
} AX_ISP_IQ_GIC_PARAM_T;

/************************************************************************************
 *  CC IQ Param
 ************************************************************************************/
#define AX_ISP_CC_LUMA_RATIO_SIZE          (2)
#define AX_ISP_CC_AUTO_COLOR_TEMP_MAX_NUM  (12)
#define AX_ISP_CC_AUTO_LUX_GAIN_MAX_NUM    (5)
#define AX_ISP_CC_SPC_MAX_NUM              (12)
#define AX_ISP_CC_ANGLE_SIZE               (16)
#define AX_ISP_CC_CCM_V_SIZE               (3)
#define AX_ISP_CC_CCM_H_SIZE               (2)
#define AX_ISP_CC_CCM_SIZE                 (AX_ISP_CC_CCM_V_SIZE * AX_ISP_CC_CCM_H_SIZE)

typedef struct {
    AX_U16                 nCcmCtrlLevel;                           /* Accuracy: U1.8 Range: [0, 256] */
    AX_S8                  nCcmSat;                                 /* Accuracy: S1.6 Range: [-64, 64] */
    AX_S16                 nCcmHue;                                 /* Accuracy: S5.6 Range: [-1920, 1920] */
    AX_S16                 nCcmMatrix[AX_ISP_CC_CCM_SIZE];         /* Accuracy: S3.8 Range: [-2047, 2047] */
    AX_U16                 nXcmCtrlLevel[AX_ISP_CC_ANGLE_SIZE];    /* Accuracy: U1.8 Range: [0, 256] */
    AX_S8                  nXcmSat[AX_ISP_CC_ANGLE_SIZE];          /* Accuracy: S1.6 Range: [-32, 32] */
    AX_S16                 nXcmHue[AX_ISP_CC_ANGLE_SIZE];          /* Accuracy: S5.6 Range: [-640, 640] */
} AX_ISP_IQ_CC_MANUAL_T;

typedef struct {
    AX_U8                  nLightSource;        /* index for which light source is selected, Accuracy: U4 Range: [0, AX_ISP_CC_SPC_MAX_NUM] */
    AX_U8                  nLightSourceNum;     /* Accuracy: U8 Range: [0, AX_ISP_CC_SPC_MAX_NUM] */
    AX_U16                 nCcmCtrlLevel[AX_ISP_CC_SPC_MAX_NUM];                          /* Accuracy: U1.8 Range: [0, 256] */
    AX_S8                  nCcmSat[AX_ISP_CC_SPC_MAX_NUM];                                /* Accuracy: S1.6 Range: [-64, 64] */
    AX_S16                 nCcmHue[AX_ISP_CC_SPC_MAX_NUM];                                /* Accuracy: S5.6 Range: [-1920, 1920] */
    AX_S16                 nCcmMatrix[AX_ISP_CC_SPC_MAX_NUM][AX_ISP_CC_CCM_SIZE];         /* Accuracy: S3.8 Range: [-2047, 2047] */
    AX_U16                 nXcmCtrlLevel[AX_ISP_CC_SPC_MAX_NUM][AX_ISP_CC_ANGLE_SIZE];    /* Accuracy: U1.8 Range: [0, 256] */
    AX_S8                  nXcmSat[AX_ISP_CC_SPC_MAX_NUM][AX_ISP_CC_ANGLE_SIZE];          /* Accuracy: S1.6 Range: [-32, 32] */
    AX_S16                 nXcmHue[AX_ISP_CC_SPC_MAX_NUM][AX_ISP_CC_ANGLE_SIZE];          /* Accuracy: S5.6 Range: [-640, 640] */
} AX_ISP_IQ_CC_LIGHT_SOURCE_AUTO_T;

typedef struct {
    AX_U8                  nCtNum;                /* color temp ref num,  Accuracy: U8 Range: [0, AX_ISP_CC_AUTO_COLOR_TEMP_MAX_NUM] */
    AX_U32                 nRefValCt[AX_ISP_CC_AUTO_COLOR_TEMP_MAX_NUM];                         /* color temp, Accuracy: U32.0 Range: [0, 100000] */
    AX_U8                  nLuxGainNum;           /* lux/gain ref num,  Accuracy: U8 Range: [0, AX_ISP_CC_AUTO_LUX_GAIN_MAX_NUM] */
    AX_U32                 nRefValLuxGain[AX_ISP_CC_AUTO_COLOR_TEMP_MAX_NUM][AX_ISP_CC_AUTO_LUX_GAIN_MAX_NUM];/* Gain: Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U16                 nCcmCtrlLevel[AX_ISP_CC_AUTO_COLOR_TEMP_MAX_NUM][AX_ISP_CC_AUTO_LUX_GAIN_MAX_NUM];                          /* Accuracy: U1.8 Range: [0, 256] */
    AX_S8                  nCcmSat[AX_ISP_CC_AUTO_COLOR_TEMP_MAX_NUM][AX_ISP_CC_AUTO_LUX_GAIN_MAX_NUM];                                /* Accuracy: S1.6 Range: [-64, 64] */
    AX_S16                 nCcmHue[AX_ISP_CC_AUTO_COLOR_TEMP_MAX_NUM][AX_ISP_CC_AUTO_LUX_GAIN_MAX_NUM];                                /* Accuracy: S5.6 Range: [-1920, 1920] */
    AX_S16                 nCcmMatrix[AX_ISP_CC_AUTO_COLOR_TEMP_MAX_NUM][AX_ISP_CC_AUTO_LUX_GAIN_MAX_NUM][AX_ISP_CC_CCM_SIZE];         /* Accuracy: S3.8 Range: [-2047, 2047] */
    AX_U16                 nXcmCtrlLevel[AX_ISP_CC_AUTO_COLOR_TEMP_MAX_NUM][AX_ISP_CC_AUTO_LUX_GAIN_MAX_NUM][AX_ISP_CC_ANGLE_SIZE];    /* Accuracy: U1.8 Range: [0, 256] */
    AX_S8                  nXcmSat[AX_ISP_CC_AUTO_COLOR_TEMP_MAX_NUM][AX_ISP_CC_AUTO_LUX_GAIN_MAX_NUM][AX_ISP_CC_ANGLE_SIZE];          /* Accuracy: S1.6 Range: [-32, 32] */
    AX_S16                 nXcmHue[AX_ISP_CC_AUTO_COLOR_TEMP_MAX_NUM][AX_ISP_CC_AUTO_LUX_GAIN_MAX_NUM][AX_ISP_CC_ANGLE_SIZE];          /* Accuracy: S5.6 Range: [-640, 640] */
} AX_ISP_IQ_CC_COLOR_TEMP_AUTO_T;

typedef struct {
    AX_U16                  nLightSourceRatio;   /* Accuracy: U1.8 Range:[0,256], 0: use tAutoParam0, 256: use tAutoParam1,  1-255:blending with tAutoParam0 and tAutoParam1*/
    AX_ISP_IQ_CC_COLOR_TEMP_AUTO_T   tColorTempAuto;      /* interpolation by color temp and lux/gain */
    AX_ISP_IQ_CC_LIGHT_SOURCE_AUTO_T tLightSourceAuto;    /* special light source param */
} AX_ISP_IQ_CC_AUTO_T;

typedef struct {
    AX_U8                  nCcEn;               /* cc enable,  Range: [0, 1], 0: Disable, 1: Enable */
    AX_U8                  nCcMode;             /* color control mode, Range: [0, 1], 0: basic mode 1: advanced mode */
    AX_U8                  nAutoMode;           /* for ref auto or manual adjust mode, Range:[0, 1], 0: manual, 1:auto, default:1 */
    AX_U8                  nRefMode;            /* choose ref mode, Range: [0, 1], 0:use color temp and lux as ref, 1:use color temp and gain as ref */
    AX_U8                  nLumaRatio[AX_ISP_CC_LUMA_RATIO_SIZE];    /* Accuracy: U1.7  Range: [0, 128] */
    AX_ISP_IQ_CC_MANUAL_T  tManualParam;
    AX_ISP_IQ_CC_AUTO_T    tAutoParam;
} AX_ISP_IQ_CC_PARAM_T;

/************************************************************************************
 *  3dlut IQ Param
 ************************************************************************************/
#define AX_ISP_3DLUT_LUT2D_RADIUS_NUM (16)
#define AX_ISP_3DLUT_LUT2D_THETA_NUM (24)
#define AX_ISP_3DLUT_LUT2D_REF_SEG_NUM_MAX (12)
#define AX_ISP_3DLUT_LUT2D_CCT_SEG_NUM_MAX (16)

typedef enum {
    AX_ISP_3DLUT_LUT_SIZE_TYPE_17      = 17,
    AX_ISP_3DLUT_LUT_SIZE_TYPE_21      = 21,
    AX_ISP_3DLUT_LUT_SIZE_TYPE_27      = 27,
} AX_ISP_IQ_3DLUT_LUT_SIZE_TYPE_E;

typedef struct
{
    AX_U32 nHueTbl[AX_ISP_3DLUT_LUT2D_RADIUS_NUM][AX_ISP_3DLUT_LUT2D_THETA_NUM]; /* Accuracy: U9.7 Range: [0, 0xB400] */
    AX_U32 nSatTbl[AX_ISP_3DLUT_LUT2D_RADIUS_NUM][AX_ISP_3DLUT_LUT2D_THETA_NUM]; /* Accuracy: U1.16 Range: [0, 0x10000] */
} AX_ISP_IQ_3DLUT_LUT2D_ANCHOR_TABLE_T;

typedef struct
{
    AX_ISP_IQ_3DLUT_LUT2D_ANCHOR_TABLE_T    tAnchorTbl;
} AX_ISP_IQ_3DLUT_MANUAL_T;

typedef struct
{
    AX_U32 nCctStart; /* Accuracy: U14.0 Range: [0, 0x3FFF] */
    AX_U32 nCctEnd;   /* Accuracy: U14.0 Range: [0, 0x3FFF] */
    AX_ISP_IQ_3DLUT_LUT2D_ANCHOR_TABLE_T    tAnchorTbl;
} AX_ISP_IQ_3DLUT_LUT2D_CCT_TABLE_T;

typedef struct
{
    AX_U32 nRefStartVal;        /* <gain value/lux value>. Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U32 nRefEndVal;          /* <gain value/lux value>. Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U32 nCctListNum;         /* Accuracy: U8.0 Range: [1, AX_ISP_3DLUT_LUT2D_CCT_SEG_NUM_MAX] */
    AX_ISP_IQ_3DLUT_LUT2D_CCT_TABLE_T   tCctTbl[AX_ISP_3DLUT_LUT2D_CCT_SEG_NUM_MAX];
} AX_ISP_IQ_3DLUT_LUT2D_REF_TABLE_T;

typedef struct
{
    AX_U32 nRefListNum;       /* Accuracy: U8.0 Range: [1, AX_ISP_3DLUT_LUT2D_REF_SEG_NUM_MAX] */
    AX_ISP_IQ_3DLUT_LUT2D_REF_TABLE_T tRefTbl[AX_ISP_3DLUT_LUT2D_REF_SEG_NUM_MAX];
} AX_ISP_IQ_3DLUT_AUTO_T;

typedef struct {
    AX_U8  n3DlutEn;                                /* Accuracy: U1.0 Range: [0, 1] */
    AX_U8  nAutoMode;                               /* 0:manual , 1:auto. Accuracy: U1.0 Range: [0, 1] */
    AX_U8  nRefMode;                                /* 0:use lux as ref, 1:use gain as ref, 1:gain is default. Accuracy: U1.0 Range: [0, 1] */
    AX_U8  nConvergeSpeed;                          /* Accuracy: U8.0 Range: [0, 0xFF] */
    AX_U32 nGainTrigger;                            /* Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U32 nLuxTrigger;                             /* Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U32 nCctTrigger;                             /* Accuracy: U14.0 Range: [0, 0x3FFF] */
    AX_ISP_IQ_3DLUT_LUT_SIZE_TYPE_E eLutSizeType;   /* Choose Lut Size Type, only support 3 params:{17, 21, 27}*/
    AX_ISP_IQ_3DLUT_MANUAL_T        tManualParam;
    AX_ISP_IQ_3DLUT_AUTO_T          tAutoParam;
} AX_ISP_IQ_3DLUT_PARAM_T;

/************************************************************************************
 *  Gamma IQ Param
 ************************************************************************************/
#define AX_ISP_GAMMA_LUT_SIZE               (129)
#define DEF_ISP_GAMMA_CURVE_MAX_NUM          (3)
typedef enum {
    AX_ISP_GAM_USER_GAMMA = 0,
    AX_ISP_GAM_PRESET_GAMMA,
} AX_ISP_GAM_MODE_E;

typedef enum {
    AX_ISP_GAM_LINEAR       = 0,
    AX_ISP_GAM_BT709        = 1,
    AX_ISP_GAM_SRGB         = 2,
    AX_ISP_GAM_AX_GAM0      = 10,
    AX_ISP_GAM_AX_GAM1      = 11,
    AX_ISP_GAM_AX_GAM2      = 12,
} AX_ISP_GAM_PRESET_GAMMA_TYPE_E;

typedef enum {
    AX_ISP_GAM_LUT_LINEAR = 0,
    AX_ISP_GAM_EXPONENTIAL,
} AX_ISP_LUT_MODE_E;

typedef struct {
    AX_U16                          nLut[AX_ISP_GAMMA_LUT_SIZE];                    /* Accuracy: U8.6 Range: [0, 0x3FFF] */
} AX_ISP_IQ_GAMMA_LUT_T;

typedef struct {
    AX_ISP_IQ_GAMMA_LUT_T           tGammaLut;                                      /* Gamma lut */
} AX_ISP_IQ_GAMMA_LUT_USER_MANUAL_T;

typedef struct {
    AX_U8                               nParamGrpNum;                               /* Accuracy: U8.0 Range: [0, AX_ISP_AUTO_TABLE_MAX_NUM] */
    AX_U32                              nRefValStart[DEF_ISP_GAMMA_CURVE_MAX_NUM];  /* Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U32                              nRefValEnd[DEF_ISP_GAMMA_CURVE_MAX_NUM];    /* Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_ISP_IQ_GAMMA_LUT_T               tGammaLut[DEF_ISP_GAMMA_CURVE_MAX_NUM];     /* Gamma lut */
}AX_ISP_IQ_GAMMA_LUT_USER_AUTO_T;

typedef struct {
    AX_U8                               nGammaEn;                                   /* Accuracy: U1.0 Range: [0, 1] */
    AX_U8                               nAutoMode;                                  /* Accuracy: U1.0 Range: [0, 1] */
    AX_U8                               nRefMode;                                   /* Accuracy: U1.0 Range: [0, 1] */
    AX_ISP_GAM_MODE_E                   eGammaMode;                                 /* Choice of custom gamma and preset gamma. Accuracy: U1.0 Range: [0, 1] */
    AX_ISP_GAM_PRESET_GAMMA_TYPE_E      ePresetGammaType;                           /* Optional type of preset gamma. Accuracy: U8.0 Range: [0, 255] */
    AX_ISP_LUT_MODE_E                   eLutMode;                                   /* Interpolation method of LUT table, 0: linear, 1: exponential, default:0. Accuracy: U1.0 Range: [0, 1] */
    AX_ISP_IQ_GAMMA_LUT_USER_MANUAL_T   tManualParam;                               /* When setting param, only eGammaMode=AX_ISP_GAM_USER_GAMMA is valid */
    AX_ISP_IQ_GAMMA_LUT_USER_AUTO_T     tAutoParam;                                 /* The AutoTable Gamma Lut must match eLutMode */
} AX_ISP_IQ_GAMMA_PARAM_T;

/************************************************************************************
 *  CA IQ Param
 ************************************************************************************/
#define AX_ISP_CA_COLOR_TEMP_AUTO_CT_MAX_NUM         (12)
#define AX_ISP_CA_COLOR_TEMP_AUTO_LG_MAX_NUM         (5)
#define AX_ISP_CA_CMTX_NUM                           (2)
#define AX_ISP_CA_CMTX_ARRAY_NUM       (AX_ISP_CA_CMTX_NUM * AX_ISP_CA_CMTX_NUM)

typedef struct {
    AX_U16                  nCtrlLevel;                                                             /* Accuracy: U1.8 Range: [0, 256] Default: 256 */
    AX_S16                  nSat;                                                                   /* Accuracy: S6.4 Range: [-800, 800] Default: 0 */
    AX_S16                  nHue;                                                                   /* Accuracy: S6.4 Range: [-480, 480] Default: 0 */
    AX_S16                  nCmtx[AX_ISP_CA_CMTX_NUM][AX_ISP_CA_CMTX_NUM];                          /* Accuracy: S2.8 Range: [-1024, 1023] */
} AX_ISP_IQ_CA_MANUAL_T;

typedef struct {
    AX_U8                   nParamGrpNumCt;                                                         /* Accuracy: U8.0 Range: [1, 12] */
    AX_U8                   nParamGrpNumLG;                                                         /* Accuracy: U8.0 Range: [1, 5] */
    AX_U32                  nRefValCt[AX_ISP_CA_COLOR_TEMP_AUTO_CT_MAX_NUM];                        /* Accuracy: U32.0 Range: [0x0, 0xFFFFFFFF] */
    AX_U32                  nRefValLG[AX_ISP_CA_COLOR_TEMP_AUTO_CT_MAX_NUM][AX_ISP_CA_COLOR_TEMP_AUTO_LG_MAX_NUM];                           /* Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF] */
    AX_S16                  nCmtx[AX_ISP_CA_COLOR_TEMP_AUTO_CT_MAX_NUM][AX_ISP_CA_COLOR_TEMP_AUTO_LG_MAX_NUM][AX_ISP_CA_CMTX_ARRAY_NUM];     /* Accuracy: S2.8 Range: [-1024, 1023] */
} AX_ISP_IQ_CA_AUTO_T;

typedef struct {
    AX_U8                   nCppEn;                                                                 /* Accuracy: U1.0 Range: [0, 1] */
    AX_U8                   nAutoMode;                                                              /* Accuracy: U1.0 Range: [0, 1] */
    AX_U8                   nRefMode;                                                               /* Accuracy: U1.0 Range: [0, 1] */
    AX_ISP_IQ_CA_MANUAL_T   tManualParam;
    AX_ISP_IQ_CA_AUTO_T     tAutoParam;
} AX_ISP_IQ_CA_PARAM_T;

/************************************************************************************
 *  CSC IQ Param
 ************************************************************************************/
#define AX_ISP_YUV_CSC_MATRIX_SIZE           (3)

typedef enum {
    AX_ISP_CSC_USER = 0,
    AX_ISP_CSC_BT601,
    AX_ISP_CSC_BT709,
    AX_ISP_CSC_BT2020,
    AX_ISP_CSC_BUTT,
} AX_ISP_CSC_COlOR_SPACE_MODE_E;

typedef enum {
    AX_ISP_NV12_SEL = 0,
    AX_ISP_NV21_SEL = 1,
} AX_ISP_UV_SEQ_SEL_E;

typedef struct  {
    AX_S16 nMatrix[AX_ISP_YUV_CSC_MATRIX_SIZE][AX_ISP_YUV_CSC_MATRIX_SIZE]; /* color matrix. Accuracy: S2.8 Range: [-1024, 1023] */
    AX_ISP_CSC_COlOR_SPACE_MODE_E  eColorSpaceMode; /* color space select. 0:rgb2yuv_matrix(customized), 1:BT601, 2:BT709, 3:BT2020(1~3 is nMatrix, can not be modified). Accuracy: U4.0 Range: [0, 3] */
    AX_ISP_UV_SEQ_SEL_E  eUvSeqSel;                 /* U/V sequence select. Accuracy: U1.0 Range: [0, 1] */
} AX_ISP_IQ_CSC_PARAM_T;

/************************************************************************************
 *  Sharpen IQ Param
 ************************************************************************************/
#define AX_ISP_SHP_HPF_NUM                 (3)
#define AX_ISP_SHP_GAIN_SIZE               (2)
#define AX_ISP_SHP_LIMIT_SIZE              (2)

#define AX_ISP_SHP_COLOR_TARGET_NUM        (3)
#define AX_ISP_SHP_COLOR_TARGET_CHN_NUM    (3)
#define AX_ISP_SHP_COLOR_TARGET_LUT0_NUM   (3)
#define AX_ISP_SHP_COLOR_TARGET_LUT0_SIZE  (17)
#define AX_ISP_SHP_COLOR_TARGET_LUT0_TABLE (AX_ISP_SHP_COLOR_TARGET_LUT0_NUM * AX_ISP_SHP_COLOR_TARGET_LUT0_SIZE)

typedef struct {
    AX_S16  nShpMotionStren;                                                            /* sharpen strength for motion area, Accuracy: S2.8 Range: [-1024, 1023] */
    AX_S16  nShpStillStren;                                                             /* sharpen strength for still area, Accuracy: S2.8 Range: [-1024, 1023] */
    AX_S16  nShpCenter[AX_ISP_SHP_COLOR_TARGET_CHN_NUM][AX_ISP_SHP_COLOR_TARGET_NUM];   /* color target mask center [0]:Y, [1]:U, [2]:V. Accuracy: S8.4 Range: [-4096,4095] */
    AX_U16  nShpRadius[AX_ISP_SHP_COLOR_TARGET_CHN_NUM][AX_ISP_SHP_COLOR_TARGET_NUM];   /* color target mask radius [0]:Y, [1]:U, [2]:V. Accuracy: U8.4 Range: [0,4095] */
    AX_U8   nShpSmooth[AX_ISP_SHP_COLOR_TARGET_CHN_NUM][AX_ISP_SHP_COLOR_TARGET_NUM];   /* color target mask transition gradient [0]:Y, [1]:U, [2]:V. Accuracy: U8 Range: [0, 7] */
    AX_S16  nShpLevelLut0[AX_ISP_SHP_COLOR_TARGET_LUT0_TABLE];                          /* color target strength. Accuracy: S4.8 Range: [-4096,4095] */
    AX_U8   nShpCoring[AX_ISP_SHP_HPF_NUM];                                             /* sharp coring. Accuracy: U4.4 Range: [0, 255] */
    AX_U8   nShpEdSlope[AX_ISP_SHP_HPF_NUM];                                            /* sharpen edge slop. Accuracy: U1.7 Range: [0, 128] */
    AX_S8   nShpEdOffset[AX_ISP_SHP_HPF_NUM];                                           /* sharpen edge offset. Accuracy: S1.6 Range: [-128, 127] */
    AX_U8   nShpEdLlimit[AX_ISP_SHP_HPF_NUM];                                           /* sharpen edge llimit. Accuracy: U0.8 Range: [0, 255] */
    AX_U8   nShpGain[AX_ISP_SHP_HPF_NUM][AX_ISP_SHP_GAIN_SIZE];                         /* sharpen gain. Accuracy: U4.4 Range: [0, 255] */
    AX_S16  nShpLimit[AX_ISP_SHP_HPF_NUM][AX_ISP_SHP_LIMIT_SIZE];                       /* sharpen limit. Accuracy: S8.2 Range: [-1024, 1023] */
    AX_S8   nShpOsLimit[AX_ISP_SHP_HPF_NUM][AX_ISP_SHP_LIMIT_SIZE];                     /* sharpen over shot limit. Accuracy: S5.2 Range: [-128, 127] */
    AX_U8   nShpOsGain[AX_ISP_SHP_HPF_NUM];                                             /* sharpen over shot gain. Accuracy: U1.3 Range: [0, 8] */
    AX_U8   nShpHpfBsigma[AX_ISP_SHP_HPF_NUM];                                          /* sharpen filter base frequency. Accuracy: U3.5 Range: high[0, 48], med[0, 80], low[0, 96] */
    AX_U8   nShpHpfDsigma[AX_ISP_SHP_HPF_NUM];                                          /* sharpen filter delta frequency. Accuracy: U3.5 Range: [0, 255] */
    AX_U8   nShpHpfScale[AX_ISP_SHP_HPF_NUM];                                           /* sharpen filter scale. Accuracy: U1.7 Range: [0, 128] */
} AX_ISP_IQ_SHARPEN_MANUAL_T;

typedef struct {
    AX_U8   nParamGrpNum;                                                                       /* Accuracy: U8.0 Range: [0, AX_ISP_AUTO_TABLE_MAX_NUM] */
    AX_U32  nRefVal[AX_ISP_AUTO_TABLE_MAX_NUM];                                                 /* Gain: Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_S16  nShpMotionStren[AX_ISP_AUTO_TABLE_MAX_NUM];                                         /* sharpen strength for motion area, Accuracy: S2.8 Range: [-1024, 1023] */
    AX_S16  nShpStillStren[AX_ISP_AUTO_TABLE_MAX_NUM];                                          /* sharpen strength for still area, Accuracy: S2.8 Range: [-1024, 1023] */
    AX_S16  tShpCenter[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_SHP_COLOR_TARGET_CHN_NUM][AX_ISP_SHP_COLOR_TARGET_NUM];
    AX_U16  tShpRadius[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_SHP_COLOR_TARGET_CHN_NUM][AX_ISP_SHP_COLOR_TARGET_NUM];
    AX_U8   tShpSmooth[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_SHP_COLOR_TARGET_CHN_NUM][AX_ISP_SHP_COLOR_TARGET_NUM];
    AX_S16  nShpLevelLut0[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_SHP_COLOR_TARGET_LUT0_TABLE];       /* color target strength. Accuracy: S4.8 Range: [-4096,4095] */
    AX_U8   nShpCoring[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_SHP_HPF_NUM];                          /* sharp coring. Accuracy: U4.4 Range: [0, 255] */
    AX_U8   nShpEdSlope[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_SHP_HPF_NUM];                         /* sharpen edge slop. Accuracy: U1.7 Range: [0, 128] */
    AX_S8   nShpEdOffset[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_SHP_HPF_NUM];                        /* sharpen edge offset. Accuracy: S1.6 Range: [-128, 127] */
    AX_U8   nShpEdLlimit[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_SHP_HPF_NUM];                        /* sharpen edge llimit. Accuracy: U0.8 Range: [0, 255] */
    AX_U8   nShpGain[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_SHP_HPF_NUM][AX_ISP_SHP_GAIN_SIZE];      /* sharpen gain. Accuracy: U4.4 Range: [0, 255] */
    AX_S16  nShpLimit[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_SHP_HPF_NUM][AX_ISP_SHP_LIMIT_SIZE];    /* sharpen limit. Accuracy: S8.2 Range: [-1024, 1023] */
    AX_S8   nShpOsLimit[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_SHP_HPF_NUM][AX_ISP_SHP_LIMIT_SIZE];  /* sharpen over shot limit. Accuracy: S5.2 Range: [-128, 127] */
    AX_U8   nShpOsGain[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_SHP_HPF_NUM];                          /* sharpen over shot gain. Accuracy: U1.3 Range: [0, 8] */
    AX_U8   nShpHpfBsigma[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_SHP_HPF_NUM];                       /* sharpen filter base frequency. Accuracy: U3.5 Range: high[0, 48], med[0, 80], low[0, 96] */
    AX_U8   nShpHpfDsigma[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_SHP_HPF_NUM];                       /* sharpen filter delta frequency. Accuracy: U3.5 Range: [0, 255] */
    AX_U8   nShpHpfScale[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_SHP_HPF_NUM];                        /* sharpen filter scale. Accuracy: U1.7 Range: [0, 128] */
} AX_ISP_IQ_SHARPEN_AUTO_T;

typedef struct {
    AX_U8                       nShpEn;                                         /* sharpen on-off. Accuracy: U1 Range: [0, 1] */
    AX_U8                       nShpMotionEn;                                   /* enable separate sharpen control for motion area and still area, Accuracy: U1 Range: [0, 1] */
    AX_U8                       nColorTargetEn[AX_ISP_SHP_COLOR_TARGET_NUM];    /* color target on-ffset Accuracy: U1 Range: [0: 1] */
    AX_U8                       nAutoMode;                                      /* for lux auto or manual adjust mode. Accuracy: U1 Range: [0, 1] */
    AX_U8                       nRefMode;                                       /* choose ref mode. Accuracy: U1 Range: [0, 1] */
    AX_U8                       nIoFlag[AX_ISP_SHP_COLOR_TARGET_NUM];           /* shp color mask in/out flag. Accuracy: U1 Range: [0, 1] */
    AX_ISP_IQ_SHARPEN_MANUAL_T  tManualParam;
    AX_ISP_IQ_SHARPEN_AUTO_T    tAutoParam;
} AX_ISP_IQ_SHARPEN_PARAM_T;

/************************************************************************************
 *  MDE IQ Param : Mutiscale Detail Enhance
 ************************************************************************************/
#define AX_ISP_MDE_LAYER_NUM                (4)
#define AX_ISP_MDE_CON_LUMA_LUT             (129)
#define AX_ISP_MDE_CON_LEVEL_LUT            (129)

#define AX_ISP_MDE_COLOR_TARGET_NUM         (8)
#define AX_ISP_MDE_COLOR_TARGET_CHN_NUM     (3)
#define AX_ISP_MDE_COLOR_TARGET_LUT0_NUM    (9)
#define AX_ISP_MDE_COLOR_TARGET_LUT1_NUM    (5)

typedef struct {
    AX_S16  tMdeCenter[AX_ISP_MDE_COLOR_TARGET_CHN_NUM][AX_ISP_MDE_COLOR_TARGET_NUM];   /* mde color target mask center [0]:Y, [1]:U, [2]:V. Accuracy: S8.4 Range: [-4096,4095]*/
    AX_U16  tMdeRadius[AX_ISP_MDE_COLOR_TARGET_CHN_NUM][AX_ISP_MDE_COLOR_TARGET_NUM];   /* mde color target mask radius [0]:Y, [1]:U, [2]:V. Accuracy: U8.4 Range: [0,4095] */
    AX_U8   tMdeSmooth[AX_ISP_MDE_COLOR_TARGET_CHN_NUM][AX_ISP_MDE_COLOR_TARGET_NUM];   /* mde color target mask transition gradient [0]:Y, [1]:U, [2]:V. Accuracy: U8 Range: [0, 7] */
    AX_S16  nMdeStrength0[AX_ISP_MDE_LAYER_NUM][AX_ISP_MDE_COLOR_TARGET_LUT0_NUM];      /* mde color target strength. Accuracy: S4.8 Range: [-4096,4095] */
    AX_S16  nMdeStrength1[AX_ISP_MDE_LAYER_NUM][AX_ISP_MDE_COLOR_TARGET_LUT0_NUM];      /* mde color target strength. Accuracy: S4.8 Range: [-4096,4095] */
    AX_S16  nMdeStrength2[AX_ISP_MDE_LAYER_NUM][AX_ISP_MDE_COLOR_TARGET_LUT0_NUM];      /* mde color target strength. Accuracy: S4.8 Range: [-4096,4095] */
    AX_S16  nMdeStrength3[AX_ISP_MDE_LAYER_NUM][AX_ISP_MDE_COLOR_TARGET_LUT1_NUM];      /* mde color target strength. Accuracy: S4.8 Range: [-4096,4095] */
    AX_S16  nMdeStrength4[AX_ISP_MDE_LAYER_NUM][AX_ISP_MDE_COLOR_TARGET_LUT1_NUM];      /* mde color target strength. Accuracy: S4.8 Range: [-4096,4095] */
    AX_S16  nMdeStrength5[AX_ISP_MDE_LAYER_NUM][AX_ISP_MDE_COLOR_TARGET_LUT1_NUM];      /* mde color target strength. Accuracy: S4.8 Range: [-4096,4095] */
    AX_S16  nMdeStrength6[AX_ISP_MDE_LAYER_NUM];                                        /* mde color target strength. Accuracy: S4.8 Range: [-4096,4095] */
    AX_S16  nMdeStrength7[AX_ISP_MDE_LAYER_NUM];                                        /* mde color target strength. Accuracy: S4.8 Range: [-4096,4095] */
    AX_U16  nMdeLumaLut[AX_ISP_MDE_LAYER_NUM][AX_ISP_MDE_CON_LUMA_LUT];                 /* contrast mde lut controlled by luma. Accuracy: U4.8 Range: [0, 4095] */
    AX_U16  nMdeLevelLut[AX_ISP_MDE_LAYER_NUM][AX_ISP_MDE_CON_LEVEL_LUT];               /* contrast mde lut. Accuracy: U8.4 Range: [0, 4095] */
    AX_U16  nMdeGain[AX_ISP_MDE_LAYER_NUM];                                             /* contrast mde gain. Accuracy: U4.8 Range: [0, 4095] */
} AX_ISP_IQ_MDE_MANUAL_T;

typedef struct {
    AX_U8   nParamGrpNum;                                                                                           /* Accuracy: U8.0 Range: [0, AX_ISP_AUTO_TABLE_MAX_NUM] */
    AX_U32  nRefVal[AX_ISP_AUTO_TABLE_MAX_NUM];                                                                     /* Gain: Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_S16  tMdeCenter[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_MDE_COLOR_TARGET_CHN_NUM][AX_ISP_MDE_COLOR_TARGET_NUM];    /* mde color target mask center [0]:Y, [1]:U, [2]:V. Accuracy: S8.4 Range: [-4096,4095]*/
    AX_U16  tMdeRadius[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_MDE_COLOR_TARGET_CHN_NUM][AX_ISP_MDE_COLOR_TARGET_NUM];    /* mde color target mask radius [0]:Y, [1]:U, [2]:V. Accuracy: U8.4 Range: [0,4095] */
    AX_U8   tMdeSmooth[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_MDE_COLOR_TARGET_CHN_NUM][AX_ISP_MDE_COLOR_TARGET_NUM];    /* mde color target mask transition gradient [0]:Y, [1]:U, [2]:V. Accuracy: U8 Range: [0, 7] */
    AX_S16  nMdeStrength0[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_MDE_LAYER_NUM][AX_ISP_MDE_COLOR_TARGET_LUT0_NUM];       /* mde color target strength. Accuracy: S4.8 Range: [-4096,4095] */
    AX_S16  nMdeStrength1[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_MDE_LAYER_NUM][AX_ISP_MDE_COLOR_TARGET_LUT0_NUM];       /* mde color target strength. Accuracy: S4.8 Range: [-4096,4095] */
    AX_S16  nMdeStrength2[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_MDE_LAYER_NUM][AX_ISP_MDE_COLOR_TARGET_LUT0_NUM];       /* mde color target strength. Accuracy: S4.8 Range: [-4096,4095] */
    AX_S16  nMdeStrength3[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_MDE_LAYER_NUM][AX_ISP_MDE_COLOR_TARGET_LUT1_NUM];       /* mde color target strength. Accuracy: S4.8 Range: [-4096,4095] */
    AX_S16  nMdeStrength4[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_MDE_LAYER_NUM][AX_ISP_MDE_COLOR_TARGET_LUT1_NUM];       /* mde color target strength. Accuracy: S4.8 Range: [-4096,4095] */
    AX_S16  nMdeStrength5[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_MDE_LAYER_NUM][AX_ISP_MDE_COLOR_TARGET_LUT1_NUM];       /* mde color target strength. Accuracy: S4.8 Range: [-4096,4095] */
    AX_S16  nMdeStrength6[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_MDE_LAYER_NUM];                                         /* mde color target strength. Accuracy: S4.8 Range: [-4096,4095] */
    AX_S16  nMdeStrength7[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_MDE_LAYER_NUM];                                         /* mde color target strength. Accuracy: S4.8 Range: [-4096,4095] */
    AX_U16  nMdeLumaLut[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_MDE_LAYER_NUM][AX_ISP_MDE_CON_LUMA_LUT];                  /* contrast mde lut controlled by luma. Accuracy: U4.8 Range: [0, 4095] */
    AX_U16  nMdeLevelLut[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_MDE_LAYER_NUM][AX_ISP_MDE_CON_LEVEL_LUT];                /* contrast mde lut. Accuracy: U8.4 Range: [0, 4095] */
    AX_U16  nMdeGain[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_MDE_LAYER_NUM];                                              /*contrast mde gain. Accuracy: U4.8 Range: [0, 4095] */
} AX_ISP_IQ_MDE_AUTO_T;

typedef struct {
    AX_U8                   nColorTargetEn[AX_ISP_MDE_COLOR_TARGET_NUM];    /* color target on-ffset Accuracy: U1 Range: [0: 1] */
    AX_U8                   nAutoMode;                                      /* for lux auto or manual adjust mode. Accuracy: U1 Range: [0, 1] */
    AX_U8                   nRefMode;                                       /* choose ref mode. Accuracy: U1 Range: [0, 1] */
    AX_U8                   nIoFlag[AX_ISP_MDE_COLOR_TARGET_NUM];           /* mde color mask in/out flag. Accuracy: U1 Range: [0, 1] */
    AX_ISP_IQ_MDE_MANUAL_T  tManualParam;
    AX_ISP_IQ_MDE_AUTO_T    tAutoParam;
} AX_ISP_IQ_MDE_PARAM_T;

/************************************************************************************
 *  YNR IQ Param: Luma Noise Reduction : YNR + DBPC
 ************************************************************************************/
#define AX_ISP_YNR_CMASK_CENTER_SIZE    (3)
#define AX_ISP_YNR_CMASK_RADIUS_SIZE    (3)
#define AX_ISP_YNR_CMASK_SMOOTH_SIZE    (3)
#define AX_ISP_YNR_LEVEL_SIZE           (2)
#define AX_ISP_YNR_INV_LUT_SIZE         (4)
#define AX_ISP_DBPC_ED_LUT_SIZE         (4)

typedef struct {
    AX_S16  nYnrCenter[AX_ISP_YNR_CMASK_CENTER_SIZE];   /* ynr cmask center [0]:Y, [1]:U, [2]:V. Accuracy: S8.4 Range: [-4096, 4095] */
    AX_U16  nYnrRadius[AX_ISP_YNR_CMASK_RADIUS_SIZE];   /* ynr cmask radius [0]:Y, [1]:U, [2]:V. Accuracy: U8.4 Range: [0, 4095] */
    AX_U8   nYnrSmooth[AX_ISP_YNR_CMASK_SMOOTH_SIZE];   /* ynr cmask smooth [0]:Y, [1]:U, [2]:V. Accuracy: U8 Range: [0, 7] */
    AX_U8   nYnrLevel[AX_ISP_YNR_LEVEL_SIZE];           /* ynr level [0]:(mask)ratio=1, [1]:(mask)ratio=0. Accuracy: U0.8 Range: [0, 255] */
    AX_U16  nYnrInvNrLut[AX_ISP_YNR_INV_LUT_SIZE];      /* ynr 1/noise lut. Accuracy: U1.10 Range: [0, 2047] */
    AX_U8   nDbpcEdSlope;                               /* dbpc edge level coefficient. Accuracy: U4.4 Range: [0, 255] */
    AX_U16  nDbpcEdOffsetLut[AX_ISP_DBPC_ED_LUT_SIZE];  /* dbpc edge control offset. Accuracy: U8.4 Range: [0, 4095] */
} AX_ISP_IQ_YNR_MANUAL_T;

typedef struct {
    AX_U8   nParamGrpNum;                                                           /* Accuracy: U8.0 Range: [0, AX_ISP_AUTO_TABLE_MAX_NUM] */
    AX_U32  nRefVal[AX_ISP_AUTO_TABLE_MAX_NUM];                                     /* Gain: Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_S16  nYnrCenter[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_YNR_CMASK_CENTER_SIZE];    /* ynr cmask center [0]:Y, [1]:U, [2]:V. Accuracy: S8.4 Range: [-4096, 4095] */
    AX_U16  nYnrRadius[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_YNR_CMASK_RADIUS_SIZE];    /* ynr cmask radius [0]:Y, [1]:U, [2]:V. Accuracy: U8.4 Range: [0, 4095] */
    AX_U8   nYnrSmooth[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_YNR_CMASK_SMOOTH_SIZE];    /* ynr cmask smooth [0]:Y, [1]:U, [2]:V. Accuracy: U8 Range: [0, 7] */
    AX_U8   nYnrLevel[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_YNR_LEVEL_SIZE];            /* ynr level [0]:(mask)ratio=0, [1]:(mask)ratio=1. Accuracy: U0.8 Range: [0, 255] */
    AX_U16  nYnrInvNrLut[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_YNR_INV_LUT_SIZE];       /* ynr 1/noise lut. Accuracy: U1.10 Range: [0, 2047] */
    AX_U8   nDbpcEdSlope[AX_ISP_AUTO_TABLE_MAX_NUM];                                /* dbpc edge level coefficient. Accuracy: U4.4 Range: [0, 255] */
    AX_U16  nDbpcEdOffsetLut[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_DBPC_ED_LUT_SIZE];   /* dbpc edge control offset. Accuracy: U8.4 Range: [0, 4095] */
} AX_ISP_IQ_YNR_AUTO_T;

typedef struct {
    AX_U8                       nYnrEn;         /* ynr on-off. Accuracy: U1 Range: [0, 1] */
    AX_U8                       nDbpcEn;        /* dbpc on-off. Accuracy: U1 Range: [0, 1] */
    AX_U8                       nColorTargetEn; /* color target on-ffset Accuracy: U1 Range: [0: 1] */
    AX_U8                       nAutoMode;      /* for lux auto or manual adjust mode, [0,1], 0: manual, 1:auto, default:1. Accuracy: U1 Range: [0, 1] */
    AX_U8                       nRefMode;       /* choose ref mode, [0,1], 0:use lux as ref, 1:use gain as ref. Accuracy: U1 Range: [0, 1] */
    AX_U8                       nIoFlag;        /* ynr color mask in/out flag. Accuracy: U1 Range: [0, 1] */
    AX_ISP_IQ_YNR_MANUAL_T      tManualParam;
    AX_ISP_IQ_YNR_AUTO_T        tAutoParam;
} AX_ISP_IQ_YNR_PARAM_T;

/************************************************************************************
 *  AYNR IQ Param: Luma Noise Reduction : ADVANCED YNR
 ************************************************************************************/
#define AX_ISP_AYNR_LAYER_NUM           (4)
#define AX_ISP_AYNR_FILTER_COEF_H       (5)
#define AX_ISP_AYNR_FILTER_COEF_V       (5)
#define AX_ISP_AYNR_FILTER_STRENGTH     (2)
#define AX_ISP_AYNR_FILTER_COEF_TABLE   (AX_ISP_AYNR_FILTER_COEF_H * AX_ISP_AYNR_FILTER_COEF_V)

typedef struct {
    AX_S16  nAYnrFilter0Coef[AX_ISP_AYNR_FILTER_COEF_TABLE];    /* luma noise reduction filter 0 coef. Accuracy: S1.8 Range: [-256, 256] */
    AX_S16  nAYnrFilter1Coef[AX_ISP_AYNR_FILTER_COEF_TABLE];    /* luma noise reduction filter 1 coef. Accuracy: S1.8 Range: [-256, 256] */
    AX_S16  nAYnrFilter2Coef[AX_ISP_AYNR_FILTER_COEF_TABLE];    /* luma noise reduction filter 2 coef. Accuracy: S1.8 Range: [-256, 256] */
    AX_S16  nAYnrFilter3Coef[AX_ISP_AYNR_FILTER_COEF_TABLE];    /* luma noise reduction filter 3 coef. Accuracy: S1.8 Range: [-256, 256] */
    AX_S16  nAYnrFilter4Coef[AX_ISP_AYNR_FILTER_COEF_TABLE];    /* luma noise reduction filter 4 coef. Accuracy: S1.8 Range: [-256, 256] */
} AX_ISP_IQ_AYNR_FILTER_COEF_T;

typedef struct {
    AX_U16  nAYnrFilter0Strength;                               /* luma noise reduction filter 0 strength. Accuracy: U1.8 Range: [0, 256] */
    AX_U16  nAYnrFilter1Strength[AX_ISP_AYNR_FILTER_STRENGTH];  /* luma noise reduction filter 1 strength. Accuracy: U1.8 Range: [0, 256] */
    AX_U16  nAYnrFilter2Strength[AX_ISP_AYNR_FILTER_STRENGTH];  /* luma noise reduction filter 2 strength. Accuracy: U1.8 Range: [0, 256] */
    AX_U16  nAYnrFilter3Strength[AX_ISP_AYNR_FILTER_STRENGTH];  /* luma noise reduction filter 3 strength. Accuracy: U1.8 Range: [0, 256] */
    AX_U16  nAYnrFilter4Strength[AX_ISP_AYNR_FILTER_STRENGTH];  /* luma noise reduction filter 4 strength. Accuracy: U1.8 Range: [0, 256] */
} AX_ISP_IQ_AYNR_FILTER_STRENGTH_T;

typedef struct {
    AX_U16                              nAYnrEdDeteThre[AX_ISP_AYNR_LAYER_NUM];     /* luma noise reduction edge detection threshold. Accuracy: U1.10 Range: [0, 2047] */
    AX_U16                              nAYnrEdDeteSlope[AX_ISP_AYNR_LAYER_NUM];    /* luma noise reduction edge detection slope. Accuracy: U10.6 Range: [0, 265535] */
    AX_U16                              nAYnrIddDetStren[AX_ISP_AYNR_LAYER_NUM];    /* luma noise reduction isolated dot detection strength. Accuracy: U1.8 Range: [0, 256] */
    AX_ISP_IQ_AYNR_FILTER_COEF_T        tAYnrFilterCoef[AX_ISP_AYNR_LAYER_NUM];     /* manual setting mode struct */
    AX_ISP_IQ_AYNR_FILTER_STRENGTH_T    tAYnrFilterStrength[AX_ISP_AYNR_LAYER_NUM]; /* easy setting mode struct */
    AX_U16                              nAYnrStrength[AX_ISP_AYNR_LAYER_NUM];       /* luma nosie reduction strength control. Accuracy: U1.8 Range: [0, 256] */
} AX_ISP_IQ_AYNR_MANUAL_T;

typedef struct {
    AX_U8                               nParamGrpNum;                                                           /* Accuracy: U8.0 Range: [0, AX_ISP_AUTO_TABLE_MAX_NUM] */
    AX_U32                              nRefVal[AX_ISP_AUTO_TABLE_MAX_NUM];                                     /* Gain: Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U16                              nAYnrEdDeteThre[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_AYNR_LAYER_NUM];      /* luma noise reduction edge detection threshold. Accuracy: U1.10 Range: [0, 2047] */
    AX_U16                              nAYnrEdDeteSlope[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_AYNR_LAYER_NUM];     /* luma noise reduction edge detection slope. Accuracy: U10.6 Range: [0, 265535] */
    AX_U16                              nAYnrIddDetStren[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_AYNR_LAYER_NUM];     /* luma noise reduction isolated dot detection strength. Accuracy: U1.8 Range: [0, 256] */
    AX_ISP_IQ_AYNR_FILTER_COEF_T        tAYnrFilterCoef[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_AYNR_LAYER_NUM];      /* manual setting mode struct */
    AX_ISP_IQ_AYNR_FILTER_STRENGTH_T    tAYnrFilterStrength[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_AYNR_LAYER_NUM];  /* easy setting mode struct */
    AX_U16                              nAYnrStrength[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_AYNR_LAYER_NUM];        /* luma nosie reduction strength control. Accuracy: U1.8 Range: [0, 256] */
} AX_ISP_IQ_AYNR_AUTO_T;

typedef struct {
    AX_U8                       nAutoMode;                          /* for lux auto or manual adjust mode, [0,1], 0: manual, 1:auto, default:1. Accuracy: U1 Range: [0, 1] */
    AX_U8                       nRefMode;                           /* choose ref mode, [0,1], 0:use lux as ref, 1:use gain as ref. Accuracy: U1 Range: [0, 1] */
    AX_U8                       nFilterMode[AX_ISP_AYNR_LAYER_NUM]; /* luma noise reduction filter setting mode, [0]: use easy setting mode, [1]: use manual setting mode */
    AX_ISP_IQ_AYNR_MANUAL_T     tManualParam;
    AX_ISP_IQ_AYNR_AUTO_T       tAutoParam;
} AX_ISP_IQ_AYNR_PARAM_T;

/************************************************************************************
 *  CNR IQ Param: Chroma Noise Reduction : CNR
 ************************************************************************************/
#define AX_ISP_CNR_INV_LUT_SIZE         (4)

typedef struct {
    AX_U8   nCnrLevel;                              /* cnr level. Accuracy: U1.4 Range: [0, 16] */
    AX_U16  nCnrInvNrLut[AX_ISP_CNR_INV_LUT_SIZE];  /* cnr 1/noise lut. Accuracy: U1.10 Range: [0, 2047] */
} AX_ISP_IQ_CNR_MANUAL_T;

typedef struct {
    AX_U8   nParamGrpNum;                                                       /* Accuracy: U8.0 Range: [0, AX_ISP_AUTO_TABLE_MAX_NUM] */
    AX_U32  nRefVal[AX_ISP_AUTO_TABLE_MAX_NUM];                                 /* Gain: Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U8   nCnrLevel[AX_ISP_AUTO_TABLE_MAX_NUM];
    AX_U16  nCnrInvNrLut[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_CNR_INV_LUT_SIZE];   /* cnr 1/noise lut. Accuracy: U1.10 Range: [0, 2047] */
} AX_ISP_IQ_CNR_AUTO_T;

typedef struct {
    AX_U8                       nCnrEn;     /* cnr on-off. Accuracy: U1 Range: [0, 1] */
    AX_U8                       nAutoMode;  /* for lux auto or manual adjust mode, [0,1], 0: manual, 1:auto, default:1. Accuracy: U1 Range: [0, 1] */
    AX_U8                       nRefMode;   /* choose ref mode, [0,1], 0:use lux as ref, 1:use gain as ref. Accuracy: U1 Range: [0, 1] */
    AX_ISP_IQ_CNR_MANUAL_T      tManualParam;
    AX_ISP_IQ_CNR_AUTO_T        tAutoParam;
} AX_ISP_IQ_CNR_PARAM_T;

/************************************************************************************
 *  ACNR IQ Param: Chroma Noise Reduction : ADVANCED CNR
 ************************************************************************************/
#define AX_ISP_ACNR_LAYER_NUM           (4)
#define AX_ISP_ACNR_NOISE_LUT_SIZE      (9)
#define AX_ISP_ACNR_NOISE_LUT_TABLE     (AX_ISP_ACNR_LAYER_NUM * AX_ISP_ACNR_NOISE_LUT_SIZE)

typedef struct {
    AX_U16      nACnrCoringThre[AX_ISP_ACNR_LAYER_NUM];     /* chroma noise reduction coring threshold. Accuracy: U8.4 Range: [0, 4095] */
    AX_U8       nACnrCoringSlope[AX_ISP_ACNR_LAYER_NUM];    /* chroma noise reduction coring slope. Accuracy: U3 Range: [0, 7] */
    AX_U16      nACnrEdgeThre[AX_ISP_ACNR_LAYER_NUM];       /* chroma nosie reduction strength on edge. Accuracy: U1.10 Range: [0, 2047] */
    AX_U16      nACnrEdgeSlope[AX_ISP_ACNR_LAYER_NUM];      /* chroma nosie reduction strength on edge. Accuracy: U10.6 Range: [0, 65535] */
    AX_U16      nACnrNvLut[AX_ISP_ACNR_NOISE_LUT_TABLE];    /* chroma nosie reduction noise level lut. Accuracy: U6.8 Range: [0, 16383] */
    AX_U16      nACnrStrength[AX_ISP_ACNR_LAYER_NUM];       /* chroma nosie reduction strength control. Accuracy: U1.8 Range: [0, 256] */
} AX_ISP_IQ_ACNR_MANUAL_T;

typedef struct {
    AX_U8       nParamGrpNum;                                                       /* Accuracy: U8.0 Range: [0, AX_ISP_AUTO_TABLE_MAX_NUM] */
    AX_U32      nRefVal[AX_ISP_AUTO_TABLE_MAX_NUM];                                 /* Gain: Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U16      nACnrCoringThre[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_ACNR_LAYER_NUM];  /* chroma noise reduction coring threshold. Accuracy: U8.4 Range: [0, 4095] */
    AX_U8       nACnrCoringSlope[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_ACNR_LAYER_NUM]; /* chroma noise reduction coring slope. Accuracy: U3 Range: [0, 7] */
    AX_U16      nACnrEdgeThre[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_ACNR_LAYER_NUM];    /* chroma nosie reduction strength on edge. Accuracy: U1.10 Range: [0, 2047] */
    AX_U16      nACnrEdgeSlope[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_ACNR_LAYER_NUM];   /* chroma nosie reduction strength on edge. Accuracy: U10.6 Range: [0, 65535] */
    AX_U16      nACnrNvLut[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_ACNR_NOISE_LUT_TABLE]; /* chroma nosie reduction noise level lut. Accuracy: U6.8 Range: [0, 16383] */
    AX_U16      nACnrStrength[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_ACNR_LAYER_NUM];    /* chroma nosie reduction strength control. Accuracy: U1.8 Range: [0, 256] */
} AX_ISP_IQ_ACNR_AUTO_T;

typedef struct {
    AX_U8                           nAutoMode;  /* for lux auto or manual adjust mode, [0,1], 0: manual, 1:auto, default:1. Accuracy: U1 Range: [0, 1] */
    AX_U8                           nRefMode;   /* choose ref mode, [0,1], 0:use lux as ref, 1:use gain as ref. Accuracy: U1 Range: [0, 1] */
    AX_ISP_IQ_ACNR_MANUAL_T         tManualParam;
    AX_ISP_IQ_ACNR_AUTO_T           tAutoParam;
} AX_ISP_IQ_ACNR_PARAM_T;

/************************************************************************************
 *  SCM IQ Param: Special Color Mapping
 ************************************************************************************/
#define AX_ISP_SCM_COLOR_SIZE          (2)
#define AX_ISP_SCM_MASK_CENTER_UV_SIZE (2)
#define AX_ISP_SCM_MASK_SIZE           (3)

typedef struct {
    AX_S16  nScmColor[AX_ISP_SCM_COLOR_SIZE];              /* target color. Accuracy: S7.4 Range: [-2048, 2047] */
    AX_U16  nScmCenterY;                                   /* color mask center Y. Accuracy: U8.4 Range: [0, 4095] */
    AX_S16  nScmCenterUv[AX_ISP_SCM_MASK_CENTER_UV_SIZE];  /* scm color mask center [0]:U, [1]:V. Accuracy: S7.4 Range: [-2048, 2047] */
    AX_U16  nScmRadius[AX_ISP_SCM_MASK_SIZE];              /* scm color mask radius [0]:Y, [1]:U, [2]:V. Accuracy: U7.4 Range: [0, 2047] */
    AX_U8   nScmSmooth[AX_ISP_SCM_MASK_SIZE];              /* scm color mask transition gradient. Accuracy: U4 Range: [0, 7] */
} AX_ISP_IQ_SCM_MANUAL_T;

typedef struct {
    AX_U8   nParamGrpNum;                                                               /* Accuracy: U8.0 Range: [0, AX_ISP_AUTO_TABLE_MAX_NUM] */
    AX_U32  nRefVal[AX_ISP_AUTO_TABLE_MAX_NUM];                                         /* Gain: Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_S16  nScmColor[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_SCM_COLOR_SIZE];              /* target color. Accuracy: S7.4 Range: [-2048, 2047] */
    AX_U16  nScmCenterY[AX_ISP_AUTO_TABLE_MAX_NUM];                                    /* color mask center Y. Accuracy: U8.4 Range: [0, 4095] */
    AX_S16  nScmCenterUv[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_SCM_MASK_CENTER_UV_SIZE];  /* scm color mask center [0]:U, [1]:V. Accuracy: S7.4 Range: [-2048, 2047] */
    AX_U16  nScmRadius[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_SCM_MASK_SIZE];               /* scm color mask radius [0]:Y, [1]:U, [2]:V. Accuracy: U7.4 Range: [0, 2047] */
    AX_U8   nScmSmooth[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_SCM_MASK_SIZE];              /* scm color mask transition gradient. Accuracy: U4 Range: [0, 7] */
} AX_ISP_IQ_SCM_AUTO_T;

typedef struct {
    AX_U8                   nScmEn;        /* scm on-off. Accuracy: U1 Range: [0, 1] */
    AX_U8                   nAutoMode;      /* for lux auto or manual adjust mode. Accuracy: U1 Range: [0, 1] */
    AX_U8                   nRefMode;       /* choose ref mode. Accuracy: U1 Range: [0, 1] */
    AX_U8                   nScmIoFlag;    /* scm color mask in/out flag. Accuracy: U1 Range: [0, 1] */
    AX_ISP_IQ_SCM_MANUAL_T  tManualParam;
    AX_ISP_IQ_SCM_AUTO_T    tAutoParam;
} AX_ISP_IQ_SCM_PARAM_T;

/************************************************************************************
 *  YCPROC IQ Param: COLOR PROCESS
 ************************************************************************************/

typedef struct {
    AX_U8                               nYCprocEn;      /* ycproc on-off. Accuracy: U1 Range: [0, 1] */
    AX_U16                              nBrightness;    /* adjust brightness. Accuracy: U4.8 Range: [0, 4095] */
    AX_S16                              nContrast;      /* adjust contrast. Accuracy: S4.8 Range: [-4096, 4095] */
    AX_U16                              nSaturation;    /* adjust saturation. Accuracy: U4.12 Range: [0, 65535] */
    AX_S16                              nHue;           /* adjust hue. Accuracy: S0.15 Range: [-32768, 32767] */
} AX_ISP_IQ_YCPROC_PARAM_T;

/************************************************************************************
 *  CCMP IQ Param: CHROMA COMP
 ************************************************************************************/
#define AX_ISP_CCMP_Y_SIZE      (29)
#define AX_ISP_CCMP_SAT_SIZE    (23)

typedef struct {
    AX_U16      nChromaCompY[AX_ISP_CCMP_Y_SIZE];       /* ccmp y lut. Accuracy: U1.9 Range: [0, 512] */
    AX_U16      nChromaCompSat[AX_ISP_CCMP_SAT_SIZE];   /* ccmp sat lut. Accuracy: U1.9 Range: [0, 512] */
} AX_ISP_IQ_CCMP_MANUAL_T;

typedef struct {
    AX_U8       nParamGrpNum;                                                       /* Accuracy: U8.0 Range: [0, AX_ISP_AUTO_TABLE_MAX_NUM] */
    AX_U32      nRefVal[AX_ISP_AUTO_TABLE_MAX_NUM];                                 /* Gain: Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U16      nChromaCompY[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_CCMP_Y_SIZE];        /* ccmp y lut. Accuracy: U1.9 Range: [0, 512] */
    AX_U16      nChromaCompSat[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_CCMP_SAT_SIZE];    /* ccmp sat lut. Accuracy: U1.9 Range: [0, 512] */
} AX_ISP_IQ_CCMP_AUTO_T;

typedef struct {
    AX_U8                       nChromaCompEn;  /* ccmp enable. Accuracy: U1 Range: [0, 1] */
    AX_U8                       nAutoMode;      /* for lux auto or manual adjust mode. Accuracy: U1 Range: [0, 1] */
    AX_U8                       nRefMode;       /* choose ref mode. Accuracy: U1 Range: [0, 1] */
    AX_ISP_IQ_CCMP_MANUAL_T     tManualParam;
    AX_ISP_IQ_CCMP_AUTO_T       tAutoParam;
} AX_ISP_IQ_CCMP_PARAM_T;

/************************************************************************************
 *  YCRT IQ Param
 ************************************************************************************/
#define AX_ISP_YCRT_SIZE         (2)

typedef struct {
    AX_U8       nYcrtEn;                            /* ycrt on-off. Accuracy: U1 Range: [0, 1] */
    AX_U8       nSignalRangeMode;                   /* full range0->jpeg Default:[0.0, 255.0], limit range1->Video Default:[16.0, 255.0], custom range2->Customer[0.0, 255.0], Accuracy: U2 Range: [0, 2] */
    AX_U16      nYrtInputRange[AX_ISP_YCRT_SIZE];   /* y-range input. Accuracy: U8.4 Range: [0, 4095] */
    AX_U16      nYrtOutputRange[AX_ISP_YCRT_SIZE];  /* y-range output. Accuracy: U8.4 Range: [0, 4095] */
    AX_U16      nCrtInputRange[AX_ISP_YCRT_SIZE];   /* uv-range input. Accuracy: U8.4 Range: [0, 4095] */
    AX_U16      nCrtOutputRange[AX_ISP_YCRT_SIZE];  /* uv-range output. Accuracy: U8.4 Range: [0, 4095] */
    AX_U16      nClipLevelY[AX_ISP_YCRT_SIZE];      /* yclip. Accuracy: U8.4 Range: [0, 4095] */
    AX_S16      nClipLevelUV[AX_ISP_YCRT_SIZE];     /* cclip. Accuracy: S7.4 Range: [-4096, 4095] */
} AX_ISP_IQ_YCRT_PARAM_T;


/************************************************************************************
 *  AE Stat Config
 ************************************************************************************/
#define AX_AE_GIRD_NUM              (2)
#define AX_AE_HIST_WEIGHT_BLK_ROW   (16)
#define AX_AE_HIST_WEIGHT_BLK_COL   (16)


typedef enum {
    AX_ISP_AE_STAT_ITP_PSTHDR_LSC       = 0,    /* after lsc output ae stat data */
    AX_ISP_AE_STAT_ITP_PSTHDR_WBC       = 1,    /* after wbc output ae stat data */
    AX_ISP_AE_STAT_ITP_PSTHDR_RLTM      = 2,    /* after rltm output ae stat data */
    AX_ISP_AE_STAT_ITP_PSTHDR_MAX       = 3,
} AX_ISP_AE_STAT_PSTHDR_POS_E;

typedef enum {
    AX_ISP_AE_STAT_ITP_PREHDR           = 0,    /* before hdr output ae stat data */
    AX_ISP_AE_STAT_ITP_PREHDR_MAX       = 1,
} AX_ISP_AE_STAT_PREHDR_POS_E;

typedef struct {
    AX_ISP_AE_STAT_PREHDR_POS_E ePreHdrPos;     /* preHdr stat position */
    AX_ISP_AE_STAT_PSTHDR_POS_E ePstHdrPos;     /* pstHdr stat position */
} AX_ISP_IQ_AE_STAT_POS_T;

typedef struct {
    AX_U16 nRoiOffsetH;       /* horiOffset, must be even, Accuracy: U16.0, Range: [0, 0xffff] */
    AX_U16 nRoiOffsetV;       /* vertOffset, must be even, Accuracy: U16.0, Range: [0, 0xffff]*/
    AX_U16 nRoiRegionNumH;    /* must be even, Accuracy: U7.0, Range: Roi0[1, 72], Roi1[1, 8] */
    AX_U16 nRoiRegionNumV;    /* must be even, Accuracy: U10.0, Range: Roi0[1, 54], Roi1[1, 512] */
    AX_U16 nRoiRegionW;       /* regionW, must be even, Accuracy: U10.0, Range: [0, 512], nRoiOffsetH + (nRoiRegionNumH * nRoiRegionW) <= hsize */
    AX_U16 nRoiRegionH;       /* regionH, must be even, Accuracy: U10.0, Range: [0, 512], nRoiOffsetV + (nRoiRegionNumV *nRoiRegionH) <= vsize */
} AX_ISP_IQ_AE_STAT_ROI_T;

typedef struct {
    AX_U16 nRThr;           /* AE RThr (YThr for nGridMode=0). Accuracy: U8.2 Range: [0, 1023] */
    AX_U16 nBThr;           /* AE BThr. Accuracy: U8.2 Range: [0, 1023] */
    AX_U16 nGrThr;          /* AE GrThr. Accuracy: U8.2 Range: [0, 1023] */
    AX_U16 nGbThr;          /* AE GbThr. Accuracy: U8.2 Range: [0, 1023] */
} AX_ISP_IQ_AE_STAT_THR_T;

typedef struct {
    AX_U16 nRoiOffsetH;       /* horiOffset, must be even, Accuracy: U16.0, Range: [0, 0xffff] */
    AX_U16 nRoiOffsetV;       /* vertOffset, must be even, Accuracy: U16.0, Range: [0, 0xffff] */
    AX_U16 nRoiWidth;         /* RoiWidth, Accuracy: U13.0, Range: [0, 0xffff], nRoiOffsetH + (nRoiWidth * 16) <= hsize */
    AX_U16 nRoiHeight;        /* RoiHeight, Accuracy: U10.0, Range: [0, 0xffff], nRoiOffsetV + (nRoiHeight * 16) <= vsize  */
} AX_ISP_IQ_AE_HIST_ROI_T;

typedef struct {
    AX_U8  nEnable[2];                                  /* nEnable[0]: ae stat before hdr enable, nEnable[1]: ae stat after hdr enable, Accuracy: U1.0 Range: [0, 1]*/
    AX_U8  nSkipNum;                                    /* 3A STAT Skip Num. 0:no frame skip, 1:1/2 frame skip for 3a stat, 2:2/3 frame skip for 3a stat, Accuracy: U1.0 Range: [0, 2]  */
    AX_ISP_IQ_AE_STAT_POS_T tAEStatPos;                 /* ae stat position */
    AX_U8  nGridMode[AX_AE_GIRD_NUM];                   /* Grid Mode. 0: Y(1ch), 1: RGGB(4ch) Accuracy: U1.0 Range: [0, 1] */
    AX_U16 nGridYcoeff[4];                              /* Grid Ycoeff(R, Gr, Gb, B).  Accuracy: U0.12 Range: [0, 4095], sum <= 4096 */
    AX_U8  nHistMode;                                   /* Hist Mode. 0: Y(1ch), 1: YRGB(4ch), 2:RGGB(4ch) Accuracy: U2.0 Range: [0, 2] */
    AX_U16 nHistYcoeff[4];                              /* Hist Ycoeff(R, Gr, Gb, B).  Accuracy: U0.12 Range: [0, 4095], sum <= 4096 */
    AX_U8  nHistLinearBinNum;                           /* Hist Bin Num. 0: 256, 1: 512, 2:1024, Accuracy: U2.0 Range: [0, 2], 1,2 is only available when nHistMode=1 */
    AX_U8  nHistWeight[AX_AE_HIST_WEIGHT_BLK_ROW * AX_AE_HIST_WEIGHT_BLK_COL];      /* Hist Weight, Accuracy: U8.0, Range: [0, 255], 16 x 16 block */
    AX_ISP_IQ_AE_STAT_ROI_T tGridRoi[AX_AE_GIRD_NUM];
    AX_ISP_IQ_AE_STAT_THR_T tSatThr[AX_AE_GIRD_NUM];
    AX_ISP_IQ_AE_HIST_ROI_T tHistRoi;
} AX_ISP_IQ_AE_STAT_PARAM_T;


/************************************************************************************
 *  AE Stat Info
 ************************************************************************************/
/* AE Grid & Hist */
#define AX_AE_PREHDR_NUM            (3)
#define AX_AE_GRID0_ROW             (54)
#define AX_AE_GRID0_COL             (72)
#define AX_AE_GRID1_ROW             (256)
#define AX_AE_GRID1_COL             (8)
#define AX_AE_GRID_CHN              (4)
#define AX_AE_HIST_LOG_BIN          (64)
#define AX_AE_HIST_LINEAR_BIN       (1024)
#define AX_AE_HIST_CHN              (4)

typedef enum {
    AX_HIST_STAT_FRAME_CHN_L,          /* long frame */
    AX_HIST_STAT_FRAME_CHN_S,          /* middle frame */
    AX_HIST_STAT_FRAME_CHN_VS,         /* short frame */
    AX_HIST_STAT_FRAME_CHN_MAX
} AX_HIST_STAT_FRAME_CHN_E;

typedef struct {
    AX_U32 nBin[AX_AE_HIST_CHN];
} AX_AE_HIST_BIN_T;

typedef struct {
    AX_U8 nValid;
    AX_AE_HIST_BIN_T nLinearHist[AX_AE_HIST_LINEAR_BIN];
    AX_AE_HIST_BIN_T nLogHist[AX_AE_HIST_LOG_BIN];
    AX_AE_HIST_BIN_T nNormLinearHist[AX_AE_HIST_LINEAR_BIN];
    AX_U16 nNormHistWIspgainValidBinNum;
    AX_AE_HIST_BIN_T nNormLinearHistWIspgain[AX_AE_HIST_LINEAR_BIN];
} AX_AE_HIST_STAT_T;

typedef struct {
    AX_U32 nGridSum[AX_AE_GRID_CHN];
    AX_U16 nGridNum[AX_AE_GRID_CHN];
} AX_AE_GRID_STATS;

typedef struct {
    AX_U8   nValid;
    AX_U8   nChnNum;
    AX_U8   nZoneRowSize;
    AX_U8   nZoneColSize;
    AX_U16  nOffsetH;
    AX_U16  nOffsetV;
    AX_U16  nGridWidth;
    AX_U16  nGridHeight;
    AX_AE_GRID_STATS  tGridStats[AX_AE_GRID0_ROW * AX_AE_GRID0_COL];
} AX_AE_GRID_STAT0_T;

typedef struct {
    AX_U8   nValid;
    AX_U8   nChnNum;
    AX_U16  nZoneRowSize;
    AX_U16  nZoneColSize;
    AX_U16  nOffsetH;
    AX_U16  nOffsetV;
    AX_U16  nGridWidth;
    AX_U16  nGridHeight;
    AX_AE_GRID_STATS  tGridStats[AX_AE_GRID1_ROW * AX_AE_GRID1_COL];
} AX_AE_GRID_STAT1_T;

typedef struct {
    AX_U8                   nEnable;
    AX_AE_GRID_STAT0_T      tAeGrid0Stat;    /* for AE */
    AX_AE_GRID_STAT1_T      tAeGrid1Stat;    /* for Flicker */
    AX_AE_HIST_STAT_T       tAeHistStat;     /* hist */
} AX_AE_GRID_HIST_STAT_T;

typedef struct {
    AX_U64 nSeqNum;                                         /* frame seq num */
    AX_U64 nTimestamp;                                      /* frame timestamp */
    AX_U64 nUserData;                                       /* user data */
    AX_U32 nSkipNum;                                        /* Algorithm running interval */
    AX_AE_GRID_HIST_STAT_T tAeStatInfo0[AX_AE_PREHDR_NUM];  /* before hdr */
    AX_AE_GRID_HIST_STAT_T tAeStatInfo1;                    /* after hdr */
} AX_ISP_AE_STAT_INFO_T;


/************************************************************************************
 *  WB Gain Info
 ************************************************************************************/
typedef struct {
    AX_U16 nRGain;      /* WBC RGain.  Accuracy: U4.8 Range: [0, 0xFFF] */
    AX_U16 nGrGain;     /* WBC GrGain. Accuracy: U4.8 Range: [0, 0xFFF] */
    AX_U16 nGbGain;     /* WBC GbGain. Accuracy: U4.8 Range: [0, 0xFFF] */
    AX_U16 nBGain;      /* WBC BGain.  Accuracy: U4.8 Range: [0, 0xFFF] */
} AX_ISP_IQ_WB_GAIN_PARAM_T;


/************************************************************************************
 *  AWB Stat Config
 ************************************************************************************/
#define AX_AWB_GRID_ROW             (54)
#define AX_AWB_GRID_COL             (72)
#define AX_AWB_GRID_CHN             (4)
#define AX_AWB_GRID_LUMA_CHN        (4)
#define AX_AWB_GRID_LUMA_THR_NUM    (3)
#define AX_AWB_WIN_NUM              (4)
#define AX_AWB_WIN_CHN              (3)
#define AX_AWB_WIN_LUMA_CHN         (3)
#define AX_AWB_WIN_LUMA_CHN_THR_NUM (4)


typedef struct {
    AX_U16 nRoiOffsetH;       /* horiOffset, must be even, Accuracy: U16.0, Range: [0, 0xffff] */
    AX_U16 nRoiOffsetV;       /* vertOffset, must be even, Accuracy: U16.0, Range: [0, 0xffff]*/
    AX_U16 nRoiRegionNumH;    /* Accuracy: U7.0, Range: [1, 72] */
    AX_U16 nRoiRegionNumV;    /* Accuracy: U6.0, Range: [1, 54] */
    AX_U16 nRoiRegionW;       /* regionW, must be even, Accuracy: U10.0, Range: [16, 512], nRoiOffsetH + (nRoiRegionNumH * nRoiRegionW) <= hsize */
    AX_U16 nRoiRegionH;       /* regionH, must be even, Accuracy: U10.0, Range: [16, 512], nRoiOffsetV + (nRoiRegionNumV * nRoiRegionH) <= vsize */
} AX_ISP_IQ_AWB_STAT_ROI_T;

typedef struct {
    AX_U32 nRThr;           /* AWB RThr. Accuracy: U16.6 Range: [0, 0x3fffff] */
    AX_U32 nBThr;           /* AWB BThr. Accuracy: U16.6 Range: [0, 0x3fffff] */
    AX_U32 nGrThr;          /* AWB GrThr. Accuracy: U16.6 Range: [0, 0x3fffff] */
    AX_U32 nGbThr;          /* AWB GbThr. Accuracy: U16.6 Range: [0, 0x3fffff] */
    AX_U32 nYThr;           /* AWB YThr.  Accuracy: U16.6 Range: [0, 0x3fffff] */
} AX_ISP_IQ_AWB_STAT_THR_T;

typedef struct {
    AX_ISP_IQ_AWB_STAT_THR_T tLowThr;
    AX_ISP_IQ_AWB_STAT_THR_T tHighThr;
    AX_U32 tLumaThr[AX_AWB_GRID_LUMA_THR_NUM];  /* thr for luma. Accuracy: U16.2 Range: [0, 0x3ffff] */
} AX_ISP_IQ_AWB_GRID_THR_T;


typedef struct {
  AX_U16 nRoiOffsetH;        /* ROI Offset(H) : U16 Range: [0, 0xffff] */
  AX_U16 nRoiOffsetV;        /* ROI Offset(V) : U16 Range: [0, 0xffff] */
  AX_U16 nRoiSizeW;          /* ROI Size(W) : U16 Range: [0, 0xffff] */
  AX_U16 nRoiSizeH;          /* ROI Size(H) : U16 Range: [0, 0xffff] */
} AX_ISP_IQ_WIN_ROI_T;

typedef struct {
  AX_U16 nWbGain[4];          /* NearGray WbGain Accuracy: U4.8 Range: [0, 4095] */
  AX_U32 nWClip;              /* NearGray WhiteClip Accuracy: U16.6 Range: [0, 2^22 -1] */
  AX_S16 nSlope[2][2];        /* NearGray BoundarySlope Accuracy: S8.4 Range: [-4096, 4095] */
  AX_S32 nOffset[2][2];       /* NearGray BoundaryOffset Accuracy: S16.2 Range: [-0x3ffff, 0x3ffff] */
} AX_ISP_IQ_WIN_NEAR_GRAY_T;

typedef struct {
    AX_U8  nEnable;                                     /* AWB Enable. Accuracy: U1.0 Range: [0, 1] */
    AX_U8  nSkipNum;                                    /* 3A STAT Skip Num. 0:no frame skip, 1:1/2 frame skip for 3a stat, 2:2/3 frame skip for 3a stat, Accuracy: U1.0 Range: [0, 2]  */
    AX_U8  nGridMode;                                   /* AWB GridMode. Accuracy: U2.0 Range: [0, 3], 0:RGB, 1:RGGB, 2: RGGB*Luma4ch, 3:RGGB*Luma2ch */
    AX_U16 nGridYcoeff[AX_AWB_GRID_CHN];                /* AWB Grid Ycoeff(R, Gr, Gb, B).  Accuracy: U0.12 Range: [0, 4095], sum <= 4096 */
    AX_ISP_IQ_AWB_STAT_ROI_T tGridRoi;
    AX_ISP_IQ_AWB_STAT_THR_T tSatThr;
    AX_ISP_IQ_AWB_GRID_THR_T tGridThr;
    AX_ISP_IQ_WIN_ROI_T tWinRoi[AX_AWB_WIN_NUM];
    AX_U16 nWinYcoeff[AX_AWB_GRID_CHN];                 /* AWB Win Ycoeff(R, Gr, Gb, B).  Accuracy: U0.12 Range: [0, 4095], sum <= 4096 */
    AX_U32 nWinLumaThr[AX_AWB_WIN_LUMA_CHN_THR_NUM];    /* window luma thr. Accuracy: U16.2 Range: [0, 0x3ffff] */
    AX_ISP_IQ_WIN_NEAR_GRAY_T tWinNG;
} AX_ISP_IQ_AWB_STAT_PARAM_T;


/************************************************************************************
 *  AWB Stat Info
 ************************************************************************************/
typedef struct {
    AX_U32 nUnSatGridSum[AX_AWB_GRID_LUMA_CHN][AX_AWB_GRID_CHN];
    AX_U32 nSatGridSum[AX_AWB_GRID_CHN];
    AX_U32 nUnSatGridNum[AX_AWB_GRID_LUMA_CHN][AX_AWB_GRID_CHN];
    AX_U32 nSatGridNum[AX_AWB_GRID_CHN];
} AX_AWB_GRID_STATS;

typedef struct {
    AX_U32 nWinSum[AX_AWB_WIN_LUMA_CHN][AX_AWB_WIN_CHN];
    AX_U16 nWinNum[AX_AWB_WIN_LUMA_CHN];
    AX_U32 nNGWinSum[AX_AWB_WIN_LUMA_CHN][AX_AWB_WIN_CHN];
    AX_U16 nSatGridNum[AX_AWB_WIN_LUMA_CHN];
} AX_AWB_WIN_STATS;

typedef struct {
    AX_U8   nValid;
    AX_U8   nZoneRowSize;
    AX_U8   nZoneColSize;
    AX_U16  nOffsetH;
    AX_U16  nOffsetV;
    AX_U16  nGridWidth;
    AX_U16  nGridHeight;
    AX_AWB_GRID_STATS tAwbGridStats[AX_AWB_GRID_ROW * AX_AWB_GRID_COL];
} AX_AWB_GRID_STATS_INFO_T;

typedef struct {
    AX_U8  nValid;
    AX_U8  nZoneRowSize;
    AX_U8  nZoneColSize;
    AX_AWB_WIN_STATS tAwbWinStats[AX_AWB_WIN_NUM];
} AX_AWB_WIN_STATS_INFO_T;

typedef struct {
    AX_U64 nSeqNum;             /* frame seq num */
    AX_U64 nTimestamp;          /* frame timestamp */
    AX_U64 nUserData;           /* user data */
    AX_U32 nSkipNum;            /* Algorithm running interval */
    AX_AWB_GRID_STATS_INFO_T tAwbGridStats;
    AX_AWB_WIN_STATS_INFO_T  tAwbWinStats;
} AX_ISP_AWB_STAT_INFO_T;


/************************************************************************************
 *  AF Stat Config
 ************************************************************************************/
#define AX_ISP_AF_GAMMA_LUT_NUM             (33)
#define AX_ISP_AF_WEIGHT_LUT_NUM            (16)
#define AX_ISP_AF_CORING_LUT_NUM            (16)
#define AX_ISP_AF_DRC_LUT_NUM               (17)
#define AX_ISP_AF_IIR_COEF_NUM              (10)
#define AX_ISP_AF_FIR_COEF_NUM              (13)
#define AX_ISP_AF_IIR_REF_LIST_SIZE         (32)

typedef enum {
    AX_ISP_AF_STAT_IFE_LONG_FRAME       = 0,        /* IFE long frame */
    AX_ISP_AF_STAT_IFE_SHORT_FRAME      = 1,        /* IFE short frame */
    AX_ISP_AF_STAT_IFE_VERY_SHORT_FRAME = 2,        /* IFE very short frame */
    AX_ISP_AF_STAT_AFTER_ITP_WBC        = 3,        /* after wbc */
    AX_ISP_AF_STAT_AFTER_ITP_RLTM       = 4,        /* after rltm */
    AX_ISP_AF_STAT_POSITION_MAX
} AX_ISP_IQ_AF_STAT_POS_E;

typedef struct {
    AX_U16 nCoeffR;   /* Accuracy: U0.12, Range: [0, 4095] */
    AX_U16 nCoeffGb;  /* Accuracy: U0.12, Range: [0, 4095] */
    AX_U16 nCoeffGr;  /* Accuracy: U0.12, Range: [0, 4095] */
    AX_U16 nCoeffB;   /* Accuracy: U0.12, Range: [0, 4095], nCoeffR + nCoeffGr + nCoeffGb + nCoeffB <= 4096 */
} AX_ISP_IQ_AF_BAYER2Y_PARAM_T;

typedef struct {
    AX_U8  nGammaEnable;    /* Accuracy: U1.0, Range: [0, 1], 0:Disable Gamma,   1:Enable.  */
    AX_U16 nGammaLut[AX_ISP_AF_GAMMA_LUT_NUM];  /* Accuracy: U8.6, Range: [0, 16383] */
} AX_ISP_IQ_AF_GAMMA_PARAM_T;

typedef struct {
    AX_U8 nScaleEnable;   /* Accuracy: U1.0, Range: [0, 1], 0:Disable Downsample,   1:Enable Downsample.  */
    AX_U8 nScaleRatio;   /* Accuracy: U3.0, Range: [1, 3], Downsample Ratio.   */
    AX_U16 nScaleWeight[AX_ISP_AF_WEIGHT_LUT_NUM];   /* Accuracy: U0.12, Range: [0, 4095] */
} AX_ISP_IQ_AF_DOWNSCALE_PARAM_T;

typedef struct {
    AX_U32 nCoringThr;      /* Accuracy: U8.10, Range:[0, 0x3ffff], suggest 18 numbers: {2^0, 2^1, ..., 2^17} */
    AX_U16 nCoringGain;     /* Accuracy: U5.7, Range:[0, 4095] */
    AX_U8  nCoringLut[AX_ISP_AF_CORING_LUT_NUM];   /* Accuracy: U5.0, Range[0, 31], nCoringLut[i] <= nCoringLut[i+1] */
} AX_ISP_IQ_AF_CORING_PARAM_T;

typedef struct {
    AX_U8   nBrightPixMaskEnable;  /* Accuracy: U1.0, Range: [0, 1], 0:Disable,   1:Enable.  */
    AX_U16  nBrightPixMaskThr;     /* Accuracy: U8.6, Range: [0, 16383] */
    AX_U8   nBrightPixMaskLv;      /* Accuracy: U2.0, Range: [0, 3], 0:strongest - 3:weakest  */
    AX_U16  nBrightPixCountThr;    /* Accuracy: U8.6, Range: [0, 16383] */
} AX_ISP_IQ_AF_BRIGHTPIX_PARAM_T;

typedef struct {
    AX_U16 nRoiOffsetH;       /* Accuracy: U16.0, Range: [32, 0xffff], horiOffset, must be even */
    AX_U16 nRoiOffsetV;       /* Accuracy: U16.0, Range: [16, 0xffff], vertOffset, must be even */
    AX_U16 nRoiRegionNumH;    /* Accuracy: U6.0, Range: [1, 20], horiRegionNum,(nRoiRegionNumH * nRoiRegionW) % 4 == 0 */
    AX_U16 nRoiRegionNumV;    /* Accuracy: U6.0, Range: [1, 64], vertRegionNum, nRoiRegionNumH * nRoiRegionNumV <= 180 */
    AX_U16 nRoiRegionW;       /* Accuracy: U11.0, Range: [16, 512], nRoiOffsetH + nRoiRegionNumH * nRoiRegionW <= hsize */
    AX_U16 nRoiRegionH;       /* Accuracy: U11.0, Range: [8, 512], nRoiOffsetV + nRoiRegionNumV * nRoiRegionH <= vsize */
} AX_ISP_IQ_AF_ROI_PARAM_T;

typedef struct {
    AX_U8 nFirEnable;         /* Accuracy: U1.0 Range: [0, 1] 0:Disable FIR,  1:Enable FIR */
    AX_U8 nViirRefId;         /* Accuracy: U6.0, Range:[0, 31] */
    AX_U8 nH1IirRefId;        /* Accuracy: U6.0, Range:[0, 31] */
    AX_U8 nH2IirRefId;        /* Accuracy: U6.0, Range:[0, 31]*/
} AX_ISP_IQ_AF_FLT_PARAM_T;

typedef struct {
    AX_U8  nDrcEnable;                      /* Accuracy: U1.0 Range: [0, 1] 0:Disable Drc,  1:Enable Drc */
    AX_U16 nDrcLut[AX_ISP_AF_DRC_LUT_NUM];  /* Accuracy: U8.6, Range: [0, 16383] */
} AX_ISP_IQ_AF_DRC_PARAM_T;

/* Luma Depend Curve for Spotlight or Noise Suppress */
typedef struct {
    AX_U8  nLumaLowStartTh;                 /* Accuracy: U8,    Range:[0, 255]  nLumaLowStartTh <= nLumaLowEndTh <= nLumaHighStartTh <= nLumaHighEndTh */
    AX_U8  nLumaLowEndTh;                   /* Accuracy: U8,    Range:[0, 255]  nLumaLowStartTh <= nLumaLowEndTh <= nLumaHighStartTh <= nLumaHighEndTh */
    AX_U32  nLumaLowMinRatio;               /* Accuracy: U1.20, Range:[1, 0x200000] */
    AX_U8  nLumaHighStartTh;                /* Accuracy: U8,    Range:[0, 255] nLumaLowStartTh <= nLumaLowEndTh <= nLumaHighStartTh <= nLumaHighEndTh*/
    AX_U8  nLumaHighEndTh;                  /* Accuracy: U8,    Range:[0, 255] nLumaLowStartTh <= nLumaLowEndTh <= nLumaHighStartTh <= nLumaHighEndTh */
    AX_U32  nLumaHighMinRatio;              /* Accuracy: U1.20, Range:[1, 0x200000] */
}AX_ISP_IQ_AF_LUMA_SUPPRESS_USER_CURVE_T;

/* Spotlight or Noise Suppress */
typedef struct {
    AX_U8                                    nSuppressMode;            /* Accuracy: U1.0  Range:[0,   1] 0:UserDefine, 1:Ax Default */
    AX_ISP_IQ_AF_LUMA_SUPPRESS_USER_CURVE_T  tLumaSuppressUserCurve;
}AX_ISP_IQ_AF_LUMA_SUPPRESS_PARAM_T;

typedef struct {
    AX_U8 nSquareMode;        /* Accuracy: U1.0 Range:[0, 1] 0: Linear mode, 1: Square mode*/
} AX_ISP_IQ_AF_PEAK_ENHANCE_PARAM_T;

typedef struct {
    AX_U8  nAfEnable;                                               /* AF Enable. Accuracy: U1.0 Range: [0, 1] */
    AX_ISP_IQ_AF_STAT_POS_E                        eAfStatPos;      /* af stat position. 0: IFE long frame, 1: IFE short frame, 2: IFE very short frame, 3: after itp_wbc, 4: after itp_rltm, Accuracy: U3.0 Range: [0, 4] */
    AX_ISP_IQ_AF_BAYER2Y_PARAM_T                   tAfBayer2Y;
    AX_ISP_IQ_AF_GAMMA_PARAM_T                     tAfGamma;
    AX_ISP_IQ_AF_DOWNSCALE_PARAM_T                 tAfScaler;
    AX_ISP_IQ_AF_FLT_PARAM_T                       tAfFilter;
    AX_ISP_IQ_AF_CORING_PARAM_T                    tAfCoring;
    AX_ISP_IQ_AF_BRIGHTPIX_PARAM_T                 tAfBrightPix;
    AX_ISP_IQ_AF_DRC_PARAM_T                       tAfDrc;
    AX_ISP_IQ_AF_ROI_PARAM_T                       tAfRoi;
    AX_ISP_IQ_AF_LUMA_SUPPRESS_PARAM_T             tAfLumaSuppress;
    AX_ISP_IQ_AF_PEAK_ENHANCE_PARAM_T              tAfPeakEnhance;
} AX_ISP_IQ_AF_STAT_PARAM_T;

/* Bandpass Filter for Reference, with the Coefficients and Bandpass Info. */
typedef struct {
    AX_U32 nStartFreq;   /* Accuracy:U1.20 Range:[1, 0x200000] */
    AX_U32 nEndFreq;     /* Accuracy:U1.20 Range:[1, 0x200000] */
    AX_S32 nIirCoefList[AX_ISP_AF_IIR_COEF_NUM]; /* Accuracy: S2.12, Range:[-0xffff, 0xffff]. */
} AX_ISP_IQ_AF_IIR_REF_T;

/* Frequently Used Bandpass Filter List for Reference.  */
typedef struct {
    AX_U8 nViirRefNum;  /* Accuracy: U7.0, Range:[1, 32] */
    AX_U8 nH1IirRefNum; /* Accuracy: U7.0, Range:[1, 32] */
    AX_U8 nH2IirRefNum; /* Accuracy: U7.0, Range:[1, 32] */
    AX_ISP_IQ_AF_IIR_REF_T tVIirRefList[AX_ISP_AF_IIR_REF_LIST_SIZE];
    AX_ISP_IQ_AF_IIR_REF_T tH1IirRefList[AX_ISP_AF_IIR_REF_LIST_SIZE];
    AX_ISP_IQ_AF_IIR_REF_T tH2IirRefList[AX_ISP_AF_IIR_REF_LIST_SIZE];
} AX_ISP_IQ_AF_IIR_REF_LIST_T;


/************************************************************************************
 *  AF Stat Info
 ************************************************************************************/
#define AX_AF_ROI_NUM_MAX               (180)
typedef struct {
    AX_U32 nPixCount;
    AX_U32 nPixSum;
    AX_U64 nFocusValue;
    AX_U64 nFocusValueLumaSuppress;
    AX_U32 nBrightPixCount;
} AX_ISP_AF_GRID_STATS;

typedef struct {
    AX_U8  nValid;
    AX_U8  nZoneRowSize;
    AX_U8  nZoneColSize;
    AX_U16 nOffsetH;
    AX_U16 nOffsetV;
    AX_U16 nGridWidth;
    AX_U16 nGridHeight;
    AX_ISP_IQ_AF_STAT_POS_E     eAfStatPos;
    AX_ISP_AF_GRID_STATS        tAfRoiV[AX_AF_ROI_NUM_MAX];
    AX_ISP_AF_GRID_STATS        tAfRoiH1[AX_AF_ROI_NUM_MAX];
    AX_ISP_AF_GRID_STATS        tAfRoiH2[AX_AF_ROI_NUM_MAX];
} AX_ISP_AF_STATS;

typedef struct {
    AX_U64 nSeqNum;
    AX_ISP_AF_STATS tAfStatInfo;
} AX_ISP_AF_STAT_INFO_T;

/************************************************************************************
 *  RAW3DNR IQ Param
 ************************************************************************************/
#define AX_ISP_RAW3DNR_RGBY_CH_NUM (4)
#define AX_ISP_RAW3DNR_DELTA_LUT_SIZE (16)
#define AX_ISP_RAW3DNR_VALUE_LUT_SIZE (AX_ISP_RAW3DNR_DELTA_LUT_SIZE +1)
#define AX_ISP_RAW3DNR_MTD_HARD_THRES_NUM (3)
#define AX_ISP_RAW3DNR_MTD_CURVE_LUT_SIZE (2)
#define AX_ISP_RAW3DNR_MTD_MOTION_LUT_SIZE (15)
#define AX_ISP_RAW3DNR_MTD_KRNL_WT_NUM (3)
#define AX_ISP_RAW3DNR_READ_NOISE_COEF_NUM (3)
#define AX_ISP_RAW3DNR_SHOT_NOISE_COEF_NUM (2)
#define AX_ISP_RAW3DNR_MTD_NOISE_COEF_LUT_SIZE (17)
#define AX_ISP_RAW3DNR_MTD_TF_COEF_NUM (32)
#define AX_ISP_RAW3DNR_SF_NOISE_COEF_LUT_SIZE (16)
#define AX_ISP_RAW3DNR_SF_MOTION_STR_LUT_SIZE (6)
#define AX_ISP_RAW3DNR_SF_KRNL_WT0_NUM (3)
#define AX_ISP_RAW3DNR_SF_KRNL_WT1_NUM (2)
#define AX_ISP_RAW3DNR_SF_MESH_SIZE (11)

typedef enum {
    AX_RAW3DNR_FIRST_3D = 0,
    AX_RAW3DNR_FIRST_2D = 1,
    AX_RAW3DNR_ONLY_2D = 2,
} AX_ISP_RAW3DNR_WORK_MODE_E;

typedef enum {
    AX_RAW3DNR_SOFT_THRES = 0,
    AX_RAW3DNR_HARD_THRES = 1,
} AX_ISP_RAW3DNR_MTD_MODE_E;

typedef struct {
    AX_U16 nSfMeshStr[AX_ISP_RAW3DNR_SF_MESH_SIZE][AX_ISP_RAW3DNR_SF_MESH_SIZE];  /* sf_mesh_str. Default: all zero Accuracy: U4.8 Range: [0x0, 0xfff] */
} AX_ISP_IQ_RAW3DNR_SF_MESH_T;

typedef struct  {
    AX_ISP_RAW3DNR_WORK_MODE_E eWorkMode;                             /* GLB_non-auto: mode. Default: 0 Accuracy: U2.0 Range: [0x0, 0x2] */
    AX_U8 nSfMeshEnable;                                              /* GLB_non-auto: sf_mesh_enable. Default: 0 Accuracy: U1.0 Range: [0x0, 0x1] */
    AX_U8 nRefMaskUpdMode;                                            /* MTD_non-auto: mask_update_mode. Default: 1 Accuracy: U1.0 Range: [0x0, 0x1] */
    AX_S32 nReadNoiseCoeffsHcg[AX_ISP_RAW3DNR_READ_NOISE_COEF_NUM];   /* Calib_non-auto: read_noise_coeffs HCG. Default: (0, 0, 0) Accuracy: S0.31 Range: [-0x7fffffff, 0x7fffffff] */
    AX_S32 nReadNoiseCoeffsLcg[AX_ISP_RAW3DNR_READ_NOISE_COEF_NUM];   /* Calib_non-auto: read_noise_coeffs LCG. Default: (0, 0, 0) Accuracy: S0.31 Range: [-0x7fffffff, 0x7fffffff] */
    AX_S32 nShotNoiseCoeffsHcg[AX_ISP_RAW3DNR_SHOT_NOISE_COEF_NUM];   /* Calib_non-auto: shot_noise_coeffs HCG. Default: (0, 0) Accuracy: S0.31 Range: [-0x7fffffff, 0x7fffffff] */
    AX_S32 nShotNoiseCoeffsLcg[AX_ISP_RAW3DNR_SHOT_NOISE_COEF_NUM];   /* Calib_non-auto: shot_noise_coeffs LCG. Default: (0, 0) Accuracy: S0.31 Range: [-0x7fffffff, 0x7fffffff] */
} AX_ISP_IQ_RAW3DNR_GLB_T;

typedef struct  {
    AX_U8 nAiMotionMaskEnable;                                       /* ai montion mask enable. Default: 0 Accuracy: U1.0 Range: [0x0, 0x1] */
    AX_ISP_RAW3DNR_MTD_MODE_E eMtdMode;                              /* motion_det_mode. Default: 0 Accuracy: U1.0 Range: [0x0, 0x1] */
    AX_U16 nMtdHardThres[AX_ISP_RAW3DNR_MTD_HARD_THRES_NUM];         /* motion_det_hard_thres. Default: (2, 0.75, 0.5) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nMtdLumaStrValue[AX_ISP_RAW3DNR_VALUE_LUT_SIZE];          /* motion_det_str_luma_value. Default: (1, 1, 1, 1, ... , 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U8 nMtdLumaStrDelta[AX_ISP_RAW3DNR_DELTA_LUT_SIZE];           /* motion_det_str_luma_delta. Default: (0, 1, 2, 3, 4, 5, 6, ... , 15) Accuracy: U4.0 Range: [0x0, 0xf] */
    AX_U16 nMtdStrTf;                                                /* motion_det_str_tf. Default: 0 Accuracy: U1.8 Range: [0x0, 0x100] */
    AX_U16 nMtdLf0KernelWt[AX_ISP_RAW3DNR_MTD_KRNL_WT_NUM];          /* motion_det_lf0_kernel_wt. Default: (1/3, 1/3, 1/3) Accuracy: U1.8 Range: [0x0, 0x100] */
    AX_U16 nMtdLf1KernelWt[AX_ISP_RAW3DNR_MTD_KRNL_WT_NUM];          /* motion_det_lf1_kernel_wt. Default: (1/3, 1/3, 1/3) Accuracy: U1.8 Range: [0x0, 0x100] */
    AX_U16 nMtdLf0Wt[AX_ISP_RAW3DNR_RGBY_CH_NUM];                    /* motion_det_lf0_wt. Default: (1, 1, 1, 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nMtdLf0Amp[AX_ISP_RAW3DNR_RGBY_CH_NUM];                   /* motion_det_lf0_amp. Default: (1, 1, 1, 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nMtdLf1Wt;                                                /* motion_det_lf1_wt. Default: 1 Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nMtdLf1Amp;                                               /* motion_det_lf1_amp. Default: 1 Accuracy: U4.8 Range: [0x0, 0xfff] */
} AX_ISP_IQ_RAW3DNR_MTD_MANUAL_T;

typedef struct  {
    AX_U8 nAiMotionMaskEnable[AX_ISP_AUTO_TABLE_MAX_NUM];                               /* ai montion mask enable. Default: 0 Accuracy: U1.0 Range: [0x0, 0x1] */
    AX_ISP_RAW3DNR_MTD_MODE_E eMtdMode[AX_ISP_AUTO_TABLE_MAX_NUM];                      /* motion_det_mode. Default: 0 Accuracy: U1.0 Range: [0x0, 0x1] */
    AX_U16 nMtdHardThres[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_MTD_HARD_THRES_NUM]; /* motion_det_hard_thres. Default: (2, 0.75, 0.5) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nMtdLumaStrValue[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_VALUE_LUT_SIZE];  /* motion_det_str_luma_value. Default: (1, 1, 1, 1, ... , 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U8 nMtdLumaStrDelta[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_DELTA_LUT_SIZE];   /* motion_det_str_luma_delta. Default: (0, 1, 2, 3, 4, 5, 6, ... , 15) Accuracy: U4.0 Range: [0x0, 0xf] */
    AX_U16 nMtdStrTf[AX_ISP_AUTO_TABLE_MAX_NUM];                                        /* motion_det_str_tf. Default: 0 Accuracy: U1.8 Range: [0x0, 0x100] */
    AX_U16 nMtdLf0KernelWt[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_MTD_KRNL_WT_NUM];  /* motion_det_lf0_kernel_wt. Default: (1/3, 1/3, 1/3) Accuracy: U1.8 Range: [0x0, 0x100] */
    AX_U16 nMtdLf1KernelWt[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_MTD_KRNL_WT_NUM];  /* motion_det_lf1_kernel_wt. Default: (1/3, 1/3, 1/3) Accuracy: U1.8 Range: [0x0, 0x100] */
    AX_U16 nMtdLf0Wt[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_RGBY_CH_NUM];            /* motion_det_lf0_wt. Default: (1, 1, 1, 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nMtdLf0Amp[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_RGBY_CH_NUM];           /* motion_det_lf0_amp. Default: (1, 1, 1, 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nMtdLf1Wt[AX_ISP_AUTO_TABLE_MAX_NUM];                                        /* motion_det_lf1_wt. Default: 1 Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nMtdLf1Amp[AX_ISP_AUTO_TABLE_MAX_NUM];                                       /* motion_det_lf1_amp. Default: 1 Accuracy: U4.8 Range: [0x0, 0xfff] */
} AX_ISP_IQ_RAW3DNR_MTD_AUTO_T;

typedef struct  {
    AX_U8  nMtdSoftMode;                                             /* motion_det_motion_lut_custom_en. Default: 0 Accuracy: U1 Range: [0x0, 0x1] */
    AX_U8  nMtdLimit;                                                /* motion_det_motion_limit, Default: 31 Accuracy: U1 Range: [0x0, 0x1F] */
    AX_U16 nMtdMotionCurve[AX_ISP_RAW3DNR_MTD_CURVE_LUT_SIZE];       /* motion_det_motion_curve. Default: (0, 0) Accuracy: U8.8 Range: [0x0, 0xffff] */
    AX_U16 nMtdMotionLut[AX_ISP_RAW3DNR_MTD_MOTION_LUT_SIZE];        /* motion_det_motion_lut. Default: 256 Accuracy: U1.8 Range: [0x0, 0x100] */
    AX_U16 nMtdNoiseCoeffsL[AX_ISP_RAW3DNR_MTD_NOISE_COEF_LUT_SIZE]; /* motion_det_noise_coeffs_L. Default: (1, 1, 1, ..., 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nMtdNoiseCoeffsS[AX_ISP_RAW3DNR_MTD_NOISE_COEF_LUT_SIZE]; /* motion_det_noise_coeffs_S. Default: (1, 1, 1, ..., 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nMtdNoiseCoeffsVs[AX_ISP_RAW3DNR_MTD_NOISE_COEF_LUT_SIZE];/* motion_det_noise_coeffs_VS. Default: (1, 1, 1, ..., 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
} AX_ISP_IQ_RAW3DNR_TD_MANUAL_T;

typedef struct  {
    AX_U8  nMtdSoftMode[AX_ISP_AUTO_TABLE_MAX_NUM];                                             /* motion_det_motion_lut_custom_en. Default: 0 Accuracy: U1 Range: [0x0, 0x1] */
    AX_U8  nMtdLimit[AX_ISP_AUTO_TABLE_MAX_NUM];                                                /* motion_det_motion_limit, Default: 31 Accuracy: U1 Range: [0x0, 0x1F] */
    AX_U16 nMtdMotionCurve[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_MTD_CURVE_LUT_SIZE];       /* motion_det_motion_curve. Default: (0, 0) Accuracy: U8.8 Range: [0x0, 0xffff] */
    AX_U16 nMtdMotionLut[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_MTD_MOTION_LUT_SIZE];        /* motion_det_motion_lut. Default: 256 Accuracy: U1.8 Range: [0x0, 0x100] */
    AX_U16 nMtdNoiseCoeffsL[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_MTD_NOISE_COEF_LUT_SIZE]; /* motion_det_noise_coeffs_L. Default: (1, 1, 1, ..., 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nMtdNoiseCoeffsS[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_MTD_NOISE_COEF_LUT_SIZE]; /* motion_det_noise_coeffs_S. Default: (1, 1, 1, ..., 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nMtdNoiseCoeffsVs[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_MTD_NOISE_COEF_LUT_SIZE];/* motion_det_noise_coeffs_VS. Default: (1, 1, 1, ..., 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
} AX_ISP_IQ_RAW3DNR_TD_AUTO_T;

typedef struct  {
    AX_U16 nSfHighlightProtect;                                       /* sf_highlight_protect. Default: 250 Accuracy: U8.8 Range: [0x0, 0xffff] */
    AX_U8  nSfDetailPreserve;                                         /* sf_detail_preserve. Default: 0 Accuracy: U1.6 Range: [0x0, 0x40] */
    AX_U8  nSfLumaThresDelta[AX_ISP_RAW3DNR_DELTA_LUT_SIZE];          /* sf_luma_thres_delta. Default: (0, 1, 2, 3, 4, 5, 6, ... , 15) Accuracy: U4.0 Range: [0x0, 0xf] */
    AX_U16 nSfLumaThresValue[AX_ISP_RAW3DNR_VALUE_LUT_SIZE];          /* sf_luma_thres_value. Default: (1, 1, 1, 1, ... , 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nSfNoiseCoeffsL[AX_ISP_RAW3DNR_SF_NOISE_COEF_LUT_SIZE];    /* sf_noise_coeffs_L. Default: (1, 1, 1, ..., 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nSfNoiseCoeffsS[AX_ISP_RAW3DNR_SF_NOISE_COEF_LUT_SIZE];    /* sf_noise_coeffs_S. Default: (1, 1, 1, ..., 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nSfNoiseCoeffsVs[AX_ISP_RAW3DNR_SF_NOISE_COEF_LUT_SIZE];   /* sf_noise_coeffs_VS. Default: (1, 1, 1, ..., 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nSfMotionStrL[AX_ISP_RAW3DNR_SF_MOTION_STR_LUT_SIZE];      /* sf_motion_str_L. Default: (1, 1, 1, ..., 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nSfMotionStrS[AX_ISP_RAW3DNR_SF_MOTION_STR_LUT_SIZE];      /* sf_motion_str_S. Default: (1, 1, 1, ..., 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nSfMotionStrVs[AX_ISP_RAW3DNR_SF_MOTION_STR_LUT_SIZE];     /* sf_motion_str_VS. Default: (1, 1, 1, ..., 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U8  nSfKernelWt0[AX_ISP_RAW3DNR_SF_KRNL_WT0_NUM];              /* sf_kernel_wt0. Default: (1, 1, 1) Accuracy: U1.7 Range: [0x0, 0x80] */
    AX_U8  nSfKernelWt1[AX_ISP_RAW3DNR_SF_KRNL_WT1_NUM];              /* sf_kernel_wt1. Default: (1, 1) Accuracy: U1.7 Range: [0x0, 0x80] */
    AX_U8  nSfSmoothStr;                                              /* sf_smooth_str. Default: 6 Accuracy: U4.4 Range: [0x0, 0xff] */
    AX_U8  nDetailBackfillDelta[AX_ISP_RAW3DNR_DELTA_LUT_SIZE];       /* detail_backfill_delta. Default: (0, 1, 2, 3, 4, 5, 6, ... , 15) Accuracy: U4.0 Range: [0x0, 0xf] */
    AX_U8  nDetailBackfillResValue[AX_ISP_RAW3DNR_VALUE_LUT_SIZE];    /* detail_backfill_res_value. Default: (0, 0, ..., 0) Accuracy: U1.7 Range: [0x0, 0x80] */
    AX_U8  nDetailBackfillRefValue[AX_ISP_RAW3DNR_VALUE_LUT_SIZE];    /* detail_backfill_ref_value. Default: (1, 1, ..., 1) Accuracy: U1.7 Range: [0x0, 0x80] */
    AX_U8  nDetailBackfillRatioRes[AX_ISP_RAW3DNR_RGBY_CH_NUM];       /* detail_backfill_ratio_res. Default: (1, 1, 1, 1) Accuracy: U1.7 Range: [0x0, 0x80] */
    AX_U8  nDetailBackfillRatioRef[AX_ISP_RAW3DNR_RGBY_CH_NUM];       /* detail_backfill_ratio_ref. Default: (1, 1, 1, 1) Accuracy: U1.7 Range: [0x0, 0x80] */
    AX_ISP_IQ_RAW3DNR_SF_MESH_T tSfMeshStr;
} AX_ISP_IQ_RAW3DNR_SD_MANUAL_T;

typedef struct  {
    AX_U16 nSfHighlightProtect[AX_ISP_AUTO_TABLE_MAX_NUM];                                      /* sf_highlight_protect. Default: 250 Accuracy: U8.8 Range: [0x0, 0xffff] */
    AX_U8  nSfDetailPreserve[AX_ISP_AUTO_TABLE_MAX_NUM];                                        /* sf_detail_preserve. Default: 0 Accuracy: U1.6 Range: [0x0, 0x40] */
    AX_U8  nSfLumaThresDelta[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_DELTA_LUT_SIZE];         /* sf_luma_thres_delta. Default: (0, 1, 2, 3, 4, 5, 6, ... , 15) Accuracy: U4.0 Range: [0x0, 0xf] */
    AX_U16 nSfLumaThresValue[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_VALUE_LUT_SIZE];         /* sf_luma_thres_value. Default: (1, 1, 1, 1, ... , 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nSfNoiseCoeffsL[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_SF_NOISE_COEF_LUT_SIZE];   /* sf_noise_coeffs_L. Default: (1, 1, 1, ..., 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nSfNoiseCoeffsS[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_SF_NOISE_COEF_LUT_SIZE];   /* sf_noise_coeffs_S. Default: (1, 1, 1, ..., 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nSfNoiseCoeffsVs[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_SF_NOISE_COEF_LUT_SIZE];  /* sf_noise_coeffs_VS. Default: (1, 1, 1, ..., 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nSfMotionStrL[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_SF_MOTION_STR_LUT_SIZE];     /* sf_motion_str_L. Default: (1, 1, 1, ..., 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nSfMotionStrS[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_SF_MOTION_STR_LUT_SIZE];     /* sf_motion_str_S. Default: (1, 1, 1, ..., 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U16 nSfMotionStrVs[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_SF_MOTION_STR_LUT_SIZE];    /* sf_motion_str_VS. Default: (1, 1, 1, ..., 1) Accuracy: U4.8 Range: [0x0, 0xfff] */
    AX_U8  nSfKernelWt0[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_SF_KRNL_WT0_NUM];             /* sf_kernel_wt0. Default: (1, 1, 1) Accuracy: U1.7 Range: [0x0, 0x80] */
    AX_U8  nSfKernelWt1[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_SF_KRNL_WT1_NUM];             /* sf_kernel_wt1. Default: (1, 1) Accuracy: U1.7 Range: [0x0, 0x80] */
    AX_U8  nSfSmoothStr[AX_ISP_AUTO_TABLE_MAX_NUM];                                             /* sf_smooth_str. Default: 6 Accuracy: U4.4 Range: [0x0, 0xff] */
    AX_U8  nDetailBackfillDelta[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_DELTA_LUT_SIZE];      /* detail_backfill_delta. Default: (0, 1, 2, 3, 4, 5, 6, ... , 15) Accuracy: U4.0 Range: [0x0, 0xf] */
    AX_U8  nDetailBackfillResValue[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_VALUE_LUT_SIZE];   /* detail_backfill_res_value. Default: (0, 0, ..., 0) Accuracy: U1.7 Range: [0x0, 0x80] */
    AX_U8  nDetailBackfillRefValue[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_VALUE_LUT_SIZE];   /* detail_backfill_ref_value. Default: (1, 1, ..., 1) Accuracy: U1.7 Range: [0x0, 0x80] */
    AX_U8  nDetailBackfillRatioRes[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_RGBY_CH_NUM];      /* detail_backfill_ratio_res. Default: (1, 1, 1, 1) Accuracy: U1.7 Range: [0x0, 0x80] */
    AX_U8  nDetailBackfillRatioRef[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_RAW3DNR_RGBY_CH_NUM];      /* detail_backfill_ratio_ref. Default: (1, 1, 1, 1) Accuracy: U1.7 Range: [0x0, 0x80] */
    AX_ISP_IQ_RAW3DNR_SF_MESH_T tSfMeshStr[AX_ISP_AUTO_TABLE_MAX_NUM];
} AX_ISP_IQ_RAW3DNR_SD_AUTO_T;

typedef struct  {
    AX_U8 nFpnRatio;                            /* Calib_auto: fpn_ratio. Default: 0.1 Accuracy: U0.8 Range: [0x0, 0xff] */
    AX_ISP_IQ_RAW3DNR_MTD_MANUAL_T tMtd;        /* motion detection auto params */
    AX_ISP_IQ_RAW3DNR_TD_MANUAL_T tTd;          /* temporal denoise auto params */
    AX_ISP_IQ_RAW3DNR_SD_MANUAL_T tSd;          /* spatial denoise auto params */
} AX_ISP_IQ_RAW3DNR_MANUAL_T;

typedef struct  {
    AX_U8 nParamGrpNum;                         /* Accuracy: U8.0 Range: [0, AX_ISP_AUTO_TABLE_MAX_NUM] */
    AX_U32 nRefVal[AX_ISP_AUTO_TABLE_MAX_NUM];  /* Gain: Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U8 nFpnRatio[AX_ISP_AUTO_TABLE_MAX_NUM]; /* Calib_auto: fpn_ratio. Default: 0.1 Accuracy: U0.8 Range: [0x0, 0xff] */
    AX_ISP_IQ_RAW3DNR_MTD_AUTO_T tMtd;          /* motion detection auto params */
    AX_ISP_IQ_RAW3DNR_TD_AUTO_T tTd;            /* temporal denoise auto params */
    AX_ISP_IQ_RAW3DNR_SD_AUTO_T tSd;            /* spatial denoise auto params */
} AX_ISP_IQ_RAW3DNR_AUTO_T;

typedef struct  {
    AX_U8 nRaw3dnrEn;                           /* RAW 3DNR enable */
    AX_U8 nAutoMode;                            /* for AE auto or manual adjust mode, [0,1], 0: manual, 1:auto, default:1 */
    AX_U8 nRefMode;                             /* choose ref mode, [0,1], 0:use lux as ref, 1:use gain as ref */
    AX_ISP_IQ_RAW3DNR_GLB_T tGlb;               /* all non-auto params  */
    AX_ISP_IQ_RAW3DNR_MANUAL_T tManual;
    AX_ISP_IQ_RAW3DNR_AUTO_T tAuto;
} AX_ISP_IQ_RAW3DNR_PARAM_T;

/************************************************************************************
 *  DE-PWL IQ Param
 ************************************************************************************/
#define AX_ISP_DEPWL_LUT_NUM                (513)

typedef struct {
    AX_U32 nLut[AX_ISP_DEPWL_LUT_NUM]; /* Lut Accuracy: U16.6 Range: [0, 0x3FFFFF] */
} AX_ISP_IQ_DEPWL_LUT_T;

typedef struct {
    AX_U8 nUserModeEn;              /* User config de-pwl switch, [0, 1], default: 0, 1: user config mode enable, 0: inner running*/
    AX_ISP_IQ_DEPWL_LUT_T tDePwlLut;/* De-PWL Lut structure*/
} AX_ISP_IQ_DEPWL_PARAM_T;


/************************************************************************************
 *  AINR IQ Param
 ************************************************************************************/
#define AINR_MAX_PATH_SIZE                (256)
#define AINR_ISO_MODEL_MAX_NUM            (16)
#define AINR_MODEL_MAX_NUM                (AINR_ISO_MODEL_MAX_NUM * 2)
#define AINR_DUMMY_MODEL_MAX_NUM          (8)

#define AINR_BIAS_SIZE (4)
#define AINR_COLOR_CH_NUM (4)
#define AINR_STRENGTH_LUT_ROW (8)
#define AINR_STRENGTH_LUT_COLUMN (2)
#define AINR_LUMA_STRENGTH_LUT_ROW (16)
#define AINR_NON_SENS_AUTO_TABLE_SIZE (4)
#define AINR_SENS_AUTO_TABLE_SIZE (8)
#define AINR_FREQ_AFFINE_AUTO_TABLE_SIZE (8)
#define AINR_FREQ_LUT_TABLE_SIZE (8)

typedef struct {
    AX_U16 nTNRStrLut[AINR_STRENGTH_LUT_ROW][AINR_STRENGTH_LUT_COLUMN]; /* Tradition NR Strength Accuracy: U1.8 Range: [0, 255] */
    AX_U16 nShpStrLut[AINR_STRENGTH_LUT_ROW][AINR_STRENGTH_LUT_COLUMN]; /* Accuracy: U1.8 Range: [0, 255] */
    AX_U16 nTemporalFilterStrLut[AINR_STRENGTH_LUT_ROW][AINR_STRENGTH_LUT_COLUMN]; /* Temporal NR Filter Strength Accuracy: U1.8 Range: [0, 255] */
    AX_U16 nTemporalFilterStrLut2[AINR_STRENGTH_LUT_ROW][AINR_STRENGTH_LUT_COLUMN]; /* Temporal NR Filter Strength Accuracy: U1.8 Range: [0, 255] */
} AX_ISP_IQ_NON_SENS_MANUAL_PARAM_T;

typedef struct {
    AX_U16 nLumaStrLut[AINR_LUMA_STRENGTH_LUT_ROW][AINR_STRENGTH_LUT_COLUMN]; /* Accuracy: U1.8 Range: [0, 255] */
    AX_U16 nMotionNRStrLut[AINR_STRENGTH_LUT_ROW][AINR_STRENGTH_LUT_COLUMN];       /* Motion NR StrengthLut Accuracy:U1.8 Range: [0, 255] */
} AX_ISP_IQ_SENS_MANUAL_PARAM_T;

typedef struct {
    AX_U8 nLowFreqNRAffine[AINR_COLOR_CH_NUM];                           /* Accuracy: U1.7 Range: [0, 128] */
    AX_U8 nMidFreqNRAffine[AINR_COLOR_CH_NUM];                        /* Accuracy:U1.7 Range: [0, 128] */
    AX_U8 nHighFreqNRAffine[AINR_COLOR_CH_NUM];                          /* Accuracy:U1.7 Range:	[0, 128] */
} AX_ISP_IQ_FREQ_AFFINE_MANUAL_PARAM_T;

typedef struct {
    AX_U16 nLowFreqNRStrLut[AINR_STRENGTH_LUT_ROW][AINR_STRENGTH_LUT_COLUMN];                      /* Accuracy: U1.7 Range: [0, 255] */
    AX_U16 nMidFreqNRStrLut[AINR_STRENGTH_LUT_ROW][AINR_STRENGTH_LUT_COLUMN];                   /* Accuracy:U1.7 Range: [0, 255] */
    AX_U16 nHighFreqNRStrLut[AINR_STRENGTH_LUT_ROW][AINR_STRENGTH_LUT_COLUMN];                     /* Accuracy:U1.7 Range: [0, 255] */
} AX_ISP_IQ_FREQ_LUT_MANUAL_PARAM_T;

typedef struct {
    AX_CHAR szModelPath[AINR_MAX_PATH_SIZE];  /* model path, absolute path(with model name) */
    AX_CHAR szModelName[AINR_MAX_PATH_SIZE];  /* model name, only name */
    AX_CHAR szTemporalBaseNrName[AINR_MAX_PATH_SIZE];
    AX_CHAR szSpatialBaseNrName[AINR_MAX_PATH_SIZE];
    AX_S16 nBiasIn[AINR_BIAS_SIZE];                                 /* Accuracy: S7.8 Range: [-32768, 32767] */
    AX_S16 nBiasOut[AINR_BIAS_SIZE];                                /* Accuracy: S7.8 Range: [-32768, 32767] */
    AX_ISP_IQ_NON_SENS_MANUAL_PARAM_T tNonSensParam;
    AX_ISP_IQ_SENS_MANUAL_PARAM_T tSensParam;
    AX_ISP_IQ_FREQ_AFFINE_MANUAL_PARAM_T tFreqAffineParam;
    AX_ISP_IQ_FREQ_LUT_MANUAL_PARAM_T   tFreqLutParam;
} AX_ISP_IQ_AINR_MANUAL_T;

typedef struct {
    AX_U8  nParamGrpNum;                       /* Accuracy: U8.0 Range: [0x0, 0x4] */
    AX_U32 nRefValStart[AINR_NON_SENS_AUTO_TABLE_SIZE];     /* Gain start: Accuracy: U22.10 Range: [100, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U32 nRefValEnd[AINR_NON_SENS_AUTO_TABLE_SIZE];     /* Gain end: Accuracy: U22.10 Range: [100, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U16 nTNRStrLut[AINR_NON_SENS_AUTO_TABLE_SIZE][AINR_STRENGTH_LUT_ROW][AINR_STRENGTH_LUT_COLUMN];            /* Accuracy: U1.8 Range: [0, 255] */
    AX_U16 nShpStrLut[AINR_NON_SENS_AUTO_TABLE_SIZE][AINR_STRENGTH_LUT_ROW][AINR_STRENGTH_LUT_COLUMN];            /* Accuracy: U1.8 Range: [0, 255] */
    AX_U16 nTemporalFilterStrLut[AINR_NON_SENS_AUTO_TABLE_SIZE][AINR_STRENGTH_LUT_ROW][AINR_STRENGTH_LUT_COLUMN]; /* Temporal NR Filter Strength Accuracy: U1.8 Range: [0, 255] */
    AX_U16 nTemporalFilterStrLut2[AINR_NON_SENS_AUTO_TABLE_SIZE][AINR_STRENGTH_LUT_ROW][AINR_STRENGTH_LUT_COLUMN]; /* Temporal NR Filter Strength Accuracy: U1.8 Range: [0, 255] */
} AX_ISP_IQ_NON_SENS_AUTO_PARAM_T;

typedef struct {
    AX_U8  nParamGrpNum;                       /* Accuracy: U8.0 Range: [0x0, 0x8] */
    AX_U32 nRefValStart[AINR_SENS_AUTO_TABLE_SIZE];     /* Gain start: Accuracy: U22.10 Range: [100, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U32 nRefValEnd[AINR_SENS_AUTO_TABLE_SIZE];     /* Gain end: Accuracy: U22.10 Range: [100, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U16 nLumaStrLut[AINR_SENS_AUTO_TABLE_SIZE][AINR_LUMA_STRENGTH_LUT_ROW][AINR_STRENGTH_LUT_COLUMN]; /* Accuracy: U1.8 Range: [0, 255] */
    AX_U16 nMotionNRStrLut[AINR_SENS_AUTO_TABLE_SIZE][AINR_STRENGTH_LUT_ROW][AINR_STRENGTH_LUT_COLUMN];       /* Motion NR StrengthLut Accuracy:U1.8 Range: [0, 255] */
} AX_ISP_IQ_SENS_AUTO_PARAM_T;

typedef struct {
    AX_U8  nParamGrpNum;                       /* Accuracy: U8.0 Range: [0x0, 0x8] */
    AX_U32 nRefVal[AINR_FREQ_AFFINE_AUTO_TABLE_SIZE];     /* Gain: Accuracy: U22.10 Range: [100, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U8 nLowFreqNRAffine[AINR_FREQ_AFFINE_AUTO_TABLE_SIZE][AINR_COLOR_CH_NUM];                           /* Accuracy: U1.7 Range: [0, 128] */
    AX_U8 nMidFreqNRAffine[AINR_FREQ_AFFINE_AUTO_TABLE_SIZE][AINR_COLOR_CH_NUM];                        /* Accuracy:U1.7 Range: [0, 128] */
    AX_U8 nHighFreqNRAffine[AINR_FREQ_AFFINE_AUTO_TABLE_SIZE][AINR_COLOR_CH_NUM];                          /* Accuracy:U1.7 Range: [0, 128] */
} AX_ISP_IQ_FREQ_AFFINE_AUTO_PARAM_T;

typedef struct {
    AX_U8  nParamGrpNum;                       /* Accuracy: U8.0 Range: [0x0, 0x8] */
    AX_U32 nRefValStart[AINR_FREQ_LUT_TABLE_SIZE];     /* Gain start: Accuracy: U22.10 Range: [100, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U32 nRefValEnd[AINR_FREQ_LUT_TABLE_SIZE];     /* Gain end: Accuracy: U22.10 Range: [100, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U16 nLowFreqNRStrLut[AINR_FREQ_LUT_TABLE_SIZE][AINR_STRENGTH_LUT_ROW][AINR_STRENGTH_LUT_COLUMN];                      /* Accuracy: U1.7 Range: [0, 255] */
    AX_U16 nMidFreqNRStrLut[AINR_FREQ_LUT_TABLE_SIZE][AINR_STRENGTH_LUT_ROW][AINR_STRENGTH_LUT_COLUMN];                   /* Accuracy:U1.7 Range: [0, 255] */
    AX_U16 nHighFreqNRStrLut[AINR_FREQ_LUT_TABLE_SIZE][AINR_STRENGTH_LUT_ROW][AINR_STRENGTH_LUT_COLUMN];                     /* Accuracy:U1.7 Range: [0, 255] */
} AX_ISP_IQ_FREQ_LUT_AUTO_PARAM_T;

typedef struct {
    AX_CHAR szModelPath[AINR_MAX_PATH_SIZE];  /* model path, absolute path(with model name) */
    AX_CHAR szModelName[AINR_MAX_PATH_SIZE];  /* model name, only name */
    AX_CHAR szTemporalBaseNrName[AINR_MAX_PATH_SIZE];
    AX_CHAR szSpatialBaseNrName[AINR_MAX_PATH_SIZE];
    AX_S16 nBiasIn[AINR_BIAS_SIZE];                                 /* Accuracy: S7.8 Range: [-32768, 32767] */
    AX_S16 nBiasOut[AINR_BIAS_SIZE];                                /* Accuracy: S7.8 Range: [-32768, 32767] */
    AX_U8 nHcgMode;         /* model param, based on the real param of model. Accuracy: U2 Range: [0, 3] 0:LCG,1:HCG,2:LCG NOT SURPPORT*/
    AX_U32 nIsoThresholdMin; /* Accuracy: U32 Range: [1, 0xFFFFFFFF] <= */
    AX_U32 nIsoThresholdMax; /* Accuracy: U32 Range: [1, 0xFFFFFFFF] > */
} AX_ISP_IQ_AINR_AUTO_META_PARAM_T;

typedef struct {
    AX_ISP_IQ_NON_SENS_AUTO_PARAM_T tNonSensParam;
    AX_ISP_IQ_SENS_AUTO_PARAM_T tSensParam;
    AX_ISP_IQ_FREQ_AFFINE_AUTO_PARAM_T tFreqAffineParam;
    AX_ISP_IQ_FREQ_LUT_AUTO_PARAM_T tFreqLutParam;
} AX_ISP_IQ_AINR_AUTO_PARAM_TABLE_T;

typedef struct {
    AX_ISP_IQ_AINR_AUTO_META_PARAM_T tMeta;
    AX_ISP_IQ_AINR_AUTO_PARAM_TABLE_T tParam;
} AX_ISP_IQ_AINR_AUTO_PARAM_T;

typedef struct {
    AX_U8  nAutoModelNum;        /* total number of models. Accuracy: U8.0 Range: [0, AINR_MODEL_MAX_NUM] */
    AX_ISP_IQ_AINR_AUTO_PARAM_T tAutoModelTable[AINR_MODEL_MAX_NUM];
} AX_ISP_IQ_AINR_AUTO_T;

typedef struct {
    AX_ISP_IQ_AINR_AUTO_META_PARAM_T tMeta;
} AX_ISP_IQ_AINR_DUMMY_AUTO_T;

typedef struct {
    AX_U8  nModelNum;        /* total number of models. Accuracy: U8.0 Range: [0, AINR_DUMMY_MODEL_MAX_NUM] */
    AX_ISP_IQ_AINR_DUMMY_AUTO_T tModelTable[AINR_DUMMY_MODEL_MAX_NUM];
} AX_ISP_IQ_AINR_DUMMY_T;

typedef struct {
    AX_U8 nEnable;   /* Accuracy: U1.0 Range: [0x0, 0x1] */
    AX_U8 nAutoMode; /* for NR auto or manual adjust mode, 0: manual, 1:auto, default:1. Accuracy: U1 Range: [0, 1] */
    AX_U8 bUpdateTable;  /* for NR model table switch enable mode, 0: disable, 1:enable, default:0. Accuracy: U1 Range: [0, 1] */
    AX_U8 nRefMode;  /* Accuracy: U1.0 Range: [0x0, 0x1], 0:use lux as ref, 1:use gain as ref*/
    AX_U8 nHdrMode;  /* for NR model hdr mode get/set, 1: sdr, 2:hdr_2x, 3:hdr_3x, default:1. Accuracy: U2 Range: [1, 2, 3] */
    AX_ISP_IQ_AINR_MANUAL_T tManualParam;
    AX_ISP_IQ_AINR_AUTO_T tAutoParam;
    AX_ISP_IQ_AINR_DUMMY_T tDummyParam;
} AX_ISP_IQ_AINR_PARAM_T;

typedef struct {
    AX_U8   nTemporalBaseNrValidNum;
    AX_CHAR szTemporalBaseNrList[AINR_MODEL_MAX_NUM][AINR_MAX_PATH_SIZE];
    AX_U8   nSpatialBaseNrValidNum;
    AX_CHAR szSpatialBaseNrList[AINR_MODEL_MAX_NUM][AINR_MAX_PATH_SIZE];
} AX_ISP_IQ_AINR_BASE_NR_LIST;

/************************************************************************************
 *  AINR IQ Capability
 ************************************************************************************/
typedef struct {
    AX_CHAR szModelName[AINR_MAX_PATH_SIZE];    /* model name, only name */
    AX_BOOL bSuppBias;                          /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppTNRStrLut;                     /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppShpStrLut;                     /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppTemporalFilterStrLut;          /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppTemporalFilterStrLut2;         /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppLumaStrLut;                    /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppMotionNRStrLut;                /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppLowFreqNRAffine;               /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppMidFreqNRAffine;               /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppHighFreqNRAffine;              /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppLowFreqNRLut;                  /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppMidFreqNRLut;                  /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppHighFreqNRLut;                 /* Accuracy: U1.0 Range: [0, 1] */
    AX_ISP_IQ_AINR_BASE_NR_LIST tNrList;        /* supported weight list */
} AX_ISP_IQ_AINR_CAPABILITY_T;

 typedef struct {
    AX_U8                           nValidNum;
    AX_ISP_IQ_AINR_CAPABILITY_T     tModelCapList[AINR_MODEL_MAX_NUM];
} AX_ISP_IQ_AINR_CAP_TABLE_T;

/************************************************************************************
 *  AICE IQ Param
 ************************************************************************************/
#define AICE_MAX_PATH_SIZE                (256)
#define AICE_ISO_MODEL_MAX_NUM            (16)
#define AICE_MODEL_MAX_NUM                (AICE_ISO_MODEL_MAX_NUM * 2)
#define AICE_DUMMY_MODEL_MAX_NUM          (8)
#define AICE_BIAS_SIZE (4)
#define AICE_STRENGTH_LUT_COLUMN (2)
#define AICE_LUMA_STRENGTH_LUT_ROW (16)
#define AICE_LUMA_LUT_AUTO_TABLE_SIZE (8)
#define AICE_FREQ_AFFINE_AUTO_TABLE_SIZE (8)

typedef struct {
    AX_CHAR szModelPath[AICE_MAX_PATH_SIZE];  /* model path, absolute path(with model name) */
    AX_CHAR szModelName[AICE_MAX_PATH_SIZE];  /* model name, only name */
    AX_CHAR szTemporalBaseNrName[AICE_MAX_PATH_SIZE];
    AX_CHAR szSpatialBaseNrName[AICE_MAX_PATH_SIZE];
    AX_S16 nBiasIn[AICE_BIAS_SIZE];                                 /* Accuracy: S7.8 Range: [-32768, 32767] */
    AX_S16 nBiasOut[AICE_BIAS_SIZE];                                /* Accuracy: S7.8 Range: [-32768, 32767] */
} AX_ISP_IQ_AICE_MANUAL_META_T;

typedef struct {
    AX_ISP_IQ_NON_SENS_MANUAL_PARAM_T tNonSensParam;
    AX_ISP_IQ_SENS_MANUAL_PARAM_T tSensParam;
    AX_ISP_IQ_FREQ_AFFINE_MANUAL_PARAM_T tFreqAffineParam;
    AX_ISP_IQ_FREQ_LUT_MANUAL_PARAM_T   tFreqLutParam;
} AX_ISP_IQ_AICE_MANUAL_NR_T;

typedef struct {
    AX_U16 nLumaStrLut[AICE_LUMA_STRENGTH_LUT_ROW][AICE_STRENGTH_LUT_COLUMN]; /* LUMA Strength Accuracy: U1.8 Range: [0, 255] */
    AX_U8 nLowFreqAffine;   /* Accuracy: U4.4 Range: [0, 128] */
    AX_U8 nMidFreqAffine;   /* Accuracy: U4.4 Range: [0, 128] */
    AX_U8 nHighFreqAffine;  /* Accuracy: U4.4 Range: [0, 128] */
} AX_ISP_IQ_AICE_MANUAL_CE_T;

typedef struct {
    AX_ISP_IQ_AICE_MANUAL_NR_T tNr;
    AX_ISP_IQ_AICE_MANUAL_CE_T tCe;
} AX_ISP_IQ_AICE_MANUAL_PARAM_T;

typedef struct {
    AX_ISP_IQ_AICE_MANUAL_META_T   tMeta;
    AX_ISP_IQ_AICE_MANUAL_PARAM_T tParams;
} AX_ISP_IQ_AICE_MANUAL_T;

typedef struct {
    AX_CHAR szModelPath[AICE_MAX_PATH_SIZE];  /* model path, absolute path(with model name) */
    AX_CHAR szModelName[AICE_MAX_PATH_SIZE];  /* model name, only name */
    AX_U8 nHcgMode;         /* model param, based on the real param of model. Accuracy: U2 Range: [0, 3] 0:LCG,1:HCG,2:LCG NOT SURPPORT*/
    AX_U32 nIsoThresholdMin; /* Accuracy: U32 Range: [1, 0xFFFFFFFF] <= */
    AX_U32 nIsoThresholdMax; /* Accuracy: U32 Range: [1, 0xFFFFFFFF] > */
    AX_CHAR szTemporalBaseNrName[AICE_MAX_PATH_SIZE];
    AX_CHAR szSpatialBaseNrName[AICE_MAX_PATH_SIZE];
    AX_S16 nBiasIn[AICE_BIAS_SIZE];                                 /* Accuracy: S7.8 Range: [-32768, 32767] */
    AX_S16 nBiasOut[AICE_BIAS_SIZE];                                /* Accuracy: S7.8 Range: [-32768, 32767] */
} AX_ISP_IQ_AICE_AUTO_META_T;

typedef struct {
    AX_U8  nParamGrpNum;                       /* Accuracy: U8.0 Range: [0x0, 0x8] */
    AX_U32 nRefValStart[AICE_LUMA_LUT_AUTO_TABLE_SIZE];     /* Gain start: Accuracy: U22.10 Range: [100, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U32 nRefValEnd[AICE_LUMA_LUT_AUTO_TABLE_SIZE];     /* Gain end: Accuracy: U22.10 Range: [100, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U16 nLumaStrLut[AICE_LUMA_LUT_AUTO_TABLE_SIZE][AICE_LUMA_STRENGTH_LUT_ROW][AICE_STRENGTH_LUT_COLUMN]; /* Accuracy: U1.8 Range: [0, 255] */
} AX_ISP_IQ_AICE_LUMA_LUT_AUTO_PARAM_T;

typedef struct {
    AX_U8  nParamGrpNum;                       /* Accuracy: U8.0 Range: [0x0, 0x8] */
    AX_U32 nRefVal[AICE_FREQ_AFFINE_AUTO_TABLE_SIZE];     /* Gain: Accuracy: U22.10 Range: [100, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U8 nLowFreqAffine[AICE_FREQ_AFFINE_AUTO_TABLE_SIZE];                           /* Accuracy: U1.7 Range: [0, 128] */
    AX_U8 nMidFreqAffine[AICE_FREQ_AFFINE_AUTO_TABLE_SIZE];                        /* Accuracy:U1.7 Range: [0, 128] */
    AX_U8 nHighFreqAffine[AICE_FREQ_AFFINE_AUTO_TABLE_SIZE];                          /* Accuracy:U1.7 Range: [0, 128] */
} AX_ISP_IQ_AICE_FREQ_AFFINE_AUTO_PARAM_T;

typedef struct {
    AX_ISP_IQ_NON_SENS_AUTO_PARAM_T tNonSensParam;
    AX_ISP_IQ_SENS_AUTO_PARAM_T tSensParam;
    AX_ISP_IQ_FREQ_AFFINE_AUTO_PARAM_T tFreqAffineParam;
    AX_ISP_IQ_FREQ_LUT_AUTO_PARAM_T tFreqLutParam;
} AX_ISP_IQ_AICE_AUTO_PARAM_NR_T;

typedef struct {
    AX_ISP_IQ_AICE_LUMA_LUT_AUTO_PARAM_T tLumaLut;
    AX_ISP_IQ_AICE_FREQ_AFFINE_AUTO_PARAM_T tFreqAffine;
} AX_ISP_IQ_AICE_AUTO_PARAM_CE_T;

typedef struct {
    AX_ISP_IQ_AICE_AUTO_PARAM_NR_T tNr;
    AX_ISP_IQ_AICE_AUTO_PARAM_CE_T tCe;
} AX_ISP_IQ_AICE_AUTO_PARAM_T;

typedef struct {
    AX_ISP_IQ_AICE_AUTO_META_T tMeta;
    AX_ISP_IQ_AICE_AUTO_PARAM_T tParams;
} AX_ISP_IQ_AICE_AUTO_TABLE_T;

typedef struct {
    AX_U8  nAutoModelNum;        /* total number of models. Accuracy: U8.0 Range: [0, AICE_MODEL_MAX_NUM] */
    AX_ISP_IQ_AICE_AUTO_TABLE_T tAutoModelTable[AICE_MODEL_MAX_NUM];
} AX_ISP_IQ_AICE_AUTO_T;

typedef struct {
    AX_ISP_IQ_AICE_AUTO_META_T tMeta;
} AX_ISP_IQ_AICE_DUMMY_AUTO_T;

typedef struct {
    AX_U8  nModelNum;        /* total number of models. Accuracy: U8.0 Range: [0, AICE_DUMMY_MODEL_MAX_NUM] */
    AX_ISP_IQ_AICE_DUMMY_AUTO_T tModelTable[AICE_DUMMY_MODEL_MAX_NUM];
} AX_ISP_IQ_AICE_DUMMY_T;

typedef struct {
    AX_U8 nEnable;   /* Accuracy: U1.0 Range: [0x0, 0x1] */
    AX_U8 nAutoMode; /* for NR auto or manual adjust mode, 0: manual, 1:auto, default:1. Accuracy: U1 Range: [0, 1] */
    AX_U8 bUpdateTable;  /* for NR model table switch enable mode, 0: disable, 1:enable, default:0. Accuracy: U1 Range: [0, 1] */
    AX_U8 nRefMode;  /* Accuracy: U1.0 Range: [0x0, 0x1], 0:use lux as ref, 1:use gain as ref*/
    AX_ISP_IQ_AICE_MANUAL_T tManualParam;
    AX_ISP_IQ_AICE_AUTO_T tAutoParam;
    AX_ISP_IQ_AICE_DUMMY_T tDummyParam;
} AX_ISP_IQ_AICE_PARAM_T;

typedef struct {
    AX_U8   nTemporalBaseNrValidNum;
    AX_CHAR szTemporalBaseNrList[AICE_MODEL_MAX_NUM][AICE_MAX_PATH_SIZE];
    AX_U8   nSpatialBaseNrValidNum;
    AX_CHAR szSpatialBaseNrList[AICE_MODEL_MAX_NUM][AICE_MAX_PATH_SIZE];
} AX_ISP_IQ_AICE_BASE_NR_LIST;

typedef struct {
    AX_CHAR szModelName[AICE_MAX_PATH_SIZE];    /* model name, only name */
    AX_BOOL bSuppBias;                          /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppCeLumaStrLut;                  /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppCeLowFreqAffine;               /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppCeMidFreqAffine;               /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppCeHighFreqAffine;              /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppNrTNRStrLut;                   /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppNrShpStrLut;                   /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppNrTemporalFilterStrLut;        /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppNrTemporalFilterStrLut2;       /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppNrLumaStrLut;                  /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppNrMotionNRStrLut;              /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppNrLowFreqNRAffine;             /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppNrMidFreqNRAffine;             /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppNrHighFreqNRAffine;            /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppNrLowFreqNRLut;                /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppNrMidFreqNRLut;                /* Accuracy: U1.0 Range: [0, 1] */
    AX_BOOL bSuppNrHighFreqNRLut;               /* Accuracy: U1.0 Range: [0, 1] */
    AX_ISP_IQ_AICE_BASE_NR_LIST tNrList;        /* supported weight list */
} AX_ISP_IQ_AICE_CAPABILITY_T;

 typedef struct {
    AX_U8                           nValidNum;
    AX_ISP_IQ_AICE_CAPABILITY_T     tModelCapList[AINR_MODEL_MAX_NUM];
} AX_ISP_IQ_AICE_CAP_TABLE_T;

/************************************************************************************
 *  Dehaze IQ Param (API Param)
 ************************************************************************************/
#define AX_ISP_DEHAZE_LUMA_LUT_SIZE          (9)
#define AX_ISP_DEHAZE_SPATIAL_LUT_SIZE       (9)
#define AX_ISP_DEHAZE_RGB_CHN_NUM            (3)

typedef enum {
    AX_ISP_DEHAZE_STR_MANUAL = 0,
    AX_ISP_DEHAZE_STR_LUMA = 1,
    AX_ISP_DEHAZE_STR_SPATIAL = 2,
    AX_ISP_DEHAZE_STR_LUMA_AND_SPATIAL = 3,
} AX_ISP_DEHAZE_TUNE_MODE_E;

typedef struct {
    AX_ISP_DEHAZE_TUNE_MODE_E eTuningMode;        /* tuning_mode, default 0. Accuracy: U2.0 Range: [0, 3] */
    AX_U8  nGrayMode;                             /* gray_mode, default 0. Accuracy: U1.0 Range: [0, 1] */
    AX_U16 nGrayRatio[AX_ISP_DEHAZE_RGB_CHN_NUM]; /* gray_ratio, default [0, 0, 0]. Accuracy: U1.8 Range: [0, 0x100] */
    AX_U16 nAirReflect;                           /* air_reflect, default 0xcc00. Accuracy: U8.8 Range: [0, 0xff00] */
    AX_U16 nStrengthLimit;                        /* strength_limit, default 26. Accuracy: U1.8 Range: [1, 256] */
} AX_ISP_IQ_DEHAZE_GLB_T;

typedef struct {
    AX_U8  nSpatialStrengthLut[AX_ISP_DEHAZE_SPATIAL_LUT_SIZE][AX_ISP_DEHAZE_SPATIAL_LUT_SIZE]; /* spatial_strength_lut, default 128. Accuracy: U1.7 Range: [0, 255] */
} AX_ISP_IQ_DEHAZE_SPATIAL_T;

typedef struct {
    AX_U8  nEffectStrength;                                     /* manual effect_strength, default 102. Accuracy: U1.7 Range: [0, 128] */
    AX_U8  nGrayDcRatio;                                        /* global dark channel strength, default 64. Accuracy: U1.7 Range: [0, 128] */
    AX_U16 nLumaStrengthIndex[AX_ISP_DEHAZE_LUMA_LUT_SIZE];     /* luma_strength_index, default [0, 0, 0, 0, 0, 0, 0, 0, 0]. Accuracy: U8.4 Range: [0, 0xfff] */
    AX_U16 nLumaStrengthValue[AX_ISP_DEHAZE_LUMA_LUT_SIZE];     /* luma_strength_value, default [16, 16, 16, 16, 16, 16, 16, 16, 16]. Accuracy: U8.4 Range: [0, 0xfff] */
    AX_ISP_IQ_DEHAZE_SPATIAL_T tSpatialStrengthLut;
} AX_ISP_IQ_DEHAZE_MANUAL_T;

typedef struct {
    AX_U8  nParamGrpNum;                                  /* Accuracy: U8.0 Range: [0, AX_ISP_AUTO_TABLE_MAX_NUM] */
    AX_U32 nRefVal[AX_ISP_AUTO_TABLE_MAX_NUM];            /* Gain: Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF]; Lux: Accuracy: U22.10 Range: [0, 0xFFFFFFFF] */
    AX_U8  nEffectStrength[AX_ISP_AUTO_TABLE_MAX_NUM];                                  /* manual effect_strength, default 102. Accuracy: U1.7 Range: [0, 128] */
    AX_U8  nGrayDcRatio[AX_ISP_AUTO_TABLE_MAX_NUM];                                     /* global dark channel strength, default 64. Accuracy: U1.7 Range: [0, 128] */
    AX_U16 nLumaStrengthIndex[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_DEHAZE_LUMA_LUT_SIZE];  /* luma_strength_index, default [0, 0, 0, 0, 0, 0, 0, 0, 0]. Accuracy: U8.4 Range: [0, 0xfff] */
    AX_U16 nLumaStrengthValue[AX_ISP_AUTO_TABLE_MAX_NUM][AX_ISP_DEHAZE_LUMA_LUT_SIZE];  /* luma_strength_value, default [16, 16, 16, 16, 16, 16, 16, 16, 16]. Accuracy: U8.4 Range: [0, 0xfff] */
    AX_ISP_IQ_DEHAZE_SPATIAL_T tSpatialStrengthLut[AX_ISP_AUTO_TABLE_MAX_NUM];
} AX_ISP_IQ_DEHAZE_AUTO_T;

typedef struct {
    AX_U8  nDehazeEn;   /* dehaze enable */
    AX_U8  nAutoMode;   /* for lux auto or manual adjust mode, [0,1], 0: manual, 1:auto, default:1 */
    AX_U8  nRefMode;    /* choose ref mode, [0,1], 0:use lux as ref, 1:use gain as ref */
    AX_ISP_IQ_DEHAZE_GLB_T          tGlbParam;
    AX_ISP_IQ_DEHAZE_MANUAL_T       tManualParam;
    AX_ISP_IQ_DEHAZE_AUTO_T         tAutoParam;
} AX_ISP_IQ_DEHAZE_PARAM_T;

/************************************************************************************
 *  SCENE IQ Param
 ************************************************************************************/
#define AX_ISP_AUTO_SCENE_MAX_NUM (4)

typedef enum {
    AX_AI_CLOSE     = 0,                                                        /* AI-ISP closed */
    AX_AI_SDR_NR    = 1,                                                        /* SDR AI3DNR */
    AX_AI_SDR_2DNR  = 2,                                                        /* SDR AI2DNR */
    AX_AI_HDR_NR    = 3,                                                        /* HDR AI3DNR */
    AX_AI_HDR_CE    = 4,                                                        /* HDR AICE */
} AX_ISP_AI_WORK_MODE_E;

typedef struct {
    AX_U8                          nTnrWorkMode;                                /* tnr work mode, Accuracy: U8.0 Range: [0, AX_RAW3DNR_ONLY_2D] */
    AX_U8                          nAiWorkMode;                                 /* ai work mode, Accuracy: U8.0 Range: [0, AX_AI_HDR_CE] */
} AX_ISP_IQ_SCENE_MANUAL_T;

typedef struct {
    AX_U8   nSceneNum;                                                          /* Accuracy: U8.0 Range: [0, AX_ISP_AUTO_SCENE_MAX_NUM] */
    AX_U32  nDelta;                                                             /* Accuracy: U22.10 Range: [0, 2147483647] <= */
    AX_U32  nRefValStart[AX_ISP_AUTO_SCENE_MAX_NUM];                            /* Accuracy: U22.10 Range: [0, 4294967295] <= */
    AX_U32  nRefValEnd[AX_ISP_AUTO_SCENE_MAX_NUM];                              /* Accuracy: U22.10 Range: [0, 4294967295] <= */
    AX_U8   nTnrWorkMode[AX_ISP_AUTO_SCENE_MAX_NUM];                            /* tnr work mode, mapping reference AX_ISP_RAW3DNR_WORK_MODE_E, Accuracy: U8.0 Range: [0, AX_RAW3DNR_ONLY_2D] */
    AX_U8   nAiWorkMode[AX_ISP_AUTO_SCENE_MAX_NUM];                             /* ai work mode, mapping reference AX_ISP_AI_WORK_MODE_E, Accuracy: U8.0 Range: [0, AX_AI_HDR_CE] */
} AX_ISP_IQ_SCENE_AUTO_T;

typedef struct {
    AX_U8                       nAutoMode;                                      /* for auto or manual adjust mode. Accuracy: U1.0 Range: [0, 1] */
    AX_ISP_IQ_SCENE_MANUAL_T   tManualParam;
    AX_ISP_IQ_SCENE_AUTO_T     tAutoParam;
} AX_ISP_IQ_SCENE_PARAM_T;

/************************************************************************************
 *  LDC IQ Param
 ************************************************************************************/
#define AX_ISP_LDC_V2_MATRIX_V_SIZE     (3)
#define AX_ISP_LDC_V2_MATRIX_H_SIZE     (3)
#define AX_ISP_LDC_V2_COEFF_MAX_NUM     (8)

typedef enum
{
    AX_ISP_IQ_LDC_TYPE_V1,     /* lens distortion correction version 1 */
    AX_ISP_IQ_LDC_TYPE_V2,     /* lens distortion correction version 2 */
} AX_ISP_IQ_LDC_TYPE_E;

typedef struct
{
     AX_BOOL bAspect;          /* whether aspect ration is keep. Accuracy: U1.0 Range: [0, 1]*/
     AX_S16  nXRatio;          /* field angle ration of horizontal, valid when bAspect = 0. Accuracy: S16.0 Range: [0, 100] */
     AX_S16  nYRatio;          /* field angle ration of vertical, valid when bAspect = 0. Accuracy: S16.0 Range: [0, 100] */
     AX_S16  nXYRatio;         /* field angle ration of all,valid when bAspect = 1. Accuracy: S16.0 Range: [0, 100] */
     AX_S16  nCenterXOffset;   /* horizontal offset of the image distortion center relative to image center. Accuracy: S16.0 Range: [-511, 511] */
     AX_S16  nCenterYOffset;   /* vertical offset of the image distortion center relative to image center. Accuracy: S16.0 Range: [-511, 511] */
     AX_S16  nDistortionRatio; /* LDC distortion ratio. [-10000, 0): pincushion distortion; (0, 10000]: barrel distortion. Accuracy: S16.0 Range: [-10000, 10000] */
     AX_S8   nSpreadCoef;      /* LDC spread coefficient. Accuracy: S8.0 Range: [-18, 18] */
} AX_ISP_IQ_LDC_V1_PARAM_T;

typedef struct
{
     AX_U32  nMatrix[AX_ISP_LDC_V2_MATRIX_V_SIZE][AX_ISP_LDC_V2_MATRIX_H_SIZE];  /* Camera Internal Parameter Matrix, {{nXFocus, 0, nXCenter}, {0, nYFocus, nYCenter}, {0, 0, 1}}; Accuracy: have 2 decimal numbers, real value = nMatrix / 100; Range: [0, 0xFFFFFFFF] */
     AX_S64  nDistortionCoeff[AX_ISP_LDC_V2_COEFF_MAX_NUM];                      /* Distortion Coefficients = (k1, k2, p1, p2, k3, k4, k5, k6) Accuracy: have 6 decimal numbers, real value = nDistortionCoeff / 1000000; Range: [-0x7FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF] */
} AX_ISP_IQ_LDC_V2_PARAM_T;

typedef struct
{
    AX_U8 nLdcEnable;                       /* LDC enable, Accuracy: U8.0 Range: [0, 1] */
    AX_U8 nType;                            /* LDC type, Accuracy: U8.0 Range: [AX_ISP_IQ_LDC_TYPE_V1, AX_ISP_IQ_LDC_TYPE_V2] */
    AX_ISP_IQ_LDC_V1_PARAM_T tLdcV1Param;   /* LDC V1 Param */
    AX_ISP_IQ_LDC_V2_PARAM_T tLdcV2Param;   /* LDC V2 Param */
} AX_ISP_IQ_LDC_PARAM_T;

/************************************************************************************
 *  ISP IQ API
 ************************************************************************************/
AX_S32 AX_ISP_IQ_SetBlcParam(AX_U8 nPipeId, AX_ISP_IQ_BLC_PARAM_T *pIspBlcParam);
AX_S32 AX_ISP_IQ_GetBlcParam(AX_U8 nPipeId, AX_ISP_IQ_BLC_PARAM_T *pIspBlcParam);

AX_S32 AX_ISP_IQ_SetFpnParam(AX_U8 nPipeId, AX_ISP_IQ_FPN_PARAM_T *pIspFpnParam);
AX_S32 AX_ISP_IQ_GetFpnParam(AX_U8 nPipeId, AX_ISP_IQ_FPN_PARAM_T *pIspFpnParam);

AX_S32 AX_ISP_IQ_SetGblParam(AX_U8 nPipeId, AX_ISP_IQ_GBL_PARAM_T *pIspGblParam);
AX_S32 AX_ISP_IQ_GetGblParam(AX_U8 nPipeId, AX_ISP_IQ_GBL_PARAM_T *pIspGblParam);

AX_S32 AX_ISP_IQ_SetDarkShadingParam(AX_U8 nPipeId, AX_ISP_IQ_DS_PARAM_T *pIspDsParam);
AX_S32 AX_ISP_IQ_GetDarkShadingParam(AX_U8 nPipeId, AX_ISP_IQ_DS_PARAM_T *pIspDsParam);

AX_S32 AX_ISP_IQ_SetDpcParam(AX_U8 nPipeId, AX_ISP_IQ_DPC_PARAM_T *pIspDpcParam);
AX_S32 AX_ISP_IQ_GetDpcParam(AX_U8 nPipeId, AX_ISP_IQ_DPC_PARAM_T *pIspDpcParam);

AX_S32 AX_ISP_IQ_SetLscParam(AX_U8 nPipeId, AX_ISP_IQ_LSC_PARAM_T *pIspLscParam);
AX_S32 AX_ISP_IQ_GetLscParam(AX_U8 nPipeId, AX_ISP_IQ_LSC_PARAM_T *pIspLscParam);

AX_S32 AX_ISP_IQ_SetWbGainParam(AX_U8 nPipeId, AX_ISP_IQ_WB_GAIN_PARAM_T *pIspWbGainParam);
AX_S32 AX_ISP_IQ_GetWbGainParam(AX_U8 nPipeId, AX_ISP_IQ_WB_GAIN_PARAM_T *pIspWbGainParam);

AX_S32 AX_ISP_IQ_SetRltmParam(AX_U8 nPipeId, AX_ISP_IQ_RLTM_PARAM_T *pIspRltmParam);
AX_S32 AX_ISP_IQ_GetRltmParam(AX_U8 nPipeId, AX_ISP_IQ_RLTM_PARAM_T *pIspRltmParam);

AX_S32 AX_ISP_IQ_SetDemosaicParam(AX_U8 nPipeId, AX_ISP_IQ_DEMOSAIC_PARAM_T *pIspDemosaicParam);
AX_S32 AX_ISP_IQ_GetDemosaicParam(AX_U8 nPipeId, AX_ISP_IQ_DEMOSAIC_PARAM_T *pIspDemosaicParam);

AX_S32 AX_ISP_IQ_SetGicParam(AX_U8 nPipeId, AX_ISP_IQ_GIC_PARAM_T *pIspGicParam);
AX_S32 AX_ISP_IQ_GetGicParam(AX_U8 nPipeId, AX_ISP_IQ_GIC_PARAM_T *pIspGicParam);

AX_S32 AX_ISP_IQ_SetCcParam(AX_U8 nPipeId, AX_ISP_IQ_CC_PARAM_T *pIspCcParam);
AX_S32 AX_ISP_IQ_GetCcParam(AX_U8 nPipeId, AX_ISP_IQ_CC_PARAM_T *pIspCcParam);

AX_S32 AX_ISP_IQ_SetGammaParam(AX_U8 nPipeId, AX_ISP_IQ_GAMMA_PARAM_T *pIspGammaParam);
AX_S32 AX_ISP_IQ_GetGammaParam(AX_U8 nPipeId, AX_ISP_IQ_GAMMA_PARAM_T *pIspGammaParam);

AX_S32 AX_ISP_IQ_SetCscParam(AX_U8 nPipeId, AX_ISP_IQ_CSC_PARAM_T *pIspCscParam);
AX_S32 AX_ISP_IQ_GetCscParam(AX_U8 nPipeId, AX_ISP_IQ_CSC_PARAM_T *pIspCscParam);

AX_S32 AX_ISP_IQ_SetCaParam(AX_U8 nPipeId, AX_ISP_IQ_CA_PARAM_T *pIspCaParam);
AX_S32 AX_ISP_IQ_GetCaParam(AX_U8 nPipeId, AX_ISP_IQ_CA_PARAM_T *pIspCaParam);

AX_S32 AX_ISP_IQ_SetShpParam(AX_U8 nPipeId, AX_ISP_IQ_SHARPEN_PARAM_T *pIspShpParam);
AX_S32 AX_ISP_IQ_GetShpParam(AX_U8 nPipeId, AX_ISP_IQ_SHARPEN_PARAM_T *pIspShpParam);

AX_S32 AX_ISP_IQ_SetMdeParam(AX_U8 nPipeId, AX_ISP_IQ_MDE_PARAM_T *pIspMdeParam);
AX_S32 AX_ISP_IQ_GetMdeParam(AX_U8 nPipeId, AX_ISP_IQ_MDE_PARAM_T *pIspMdeParam);

AX_S32 AX_ISP_IQ_SetYnrParam(AX_U8 nPipeId, AX_ISP_IQ_YNR_PARAM_T *pIspYnrParam);
AX_S32 AX_ISP_IQ_GetYnrParam(AX_U8 nPipeId, AX_ISP_IQ_YNR_PARAM_T *pIspYnrParam);

AX_S32 AX_ISP_IQ_SetAYnrParam(AX_U8 nPipeId, AX_ISP_IQ_AYNR_PARAM_T *pIspAYnrParam);
AX_S32 AX_ISP_IQ_GetAYnrParam(AX_U8 nPipeId, AX_ISP_IQ_AYNR_PARAM_T *pIspAYnrParam);

AX_S32 AX_ISP_IQ_SetCnrParam(AX_U8 nPipeId, AX_ISP_IQ_CNR_PARAM_T *pIspCnrParam);
AX_S32 AX_ISP_IQ_GetCnrParam(AX_U8 nPipeId, AX_ISP_IQ_CNR_PARAM_T *pIspCnrParam);

AX_S32 AX_ISP_IQ_SetACnrParam(AX_U8 nPipeId, AX_ISP_IQ_ACNR_PARAM_T *pIspACnrParam);
AX_S32 AX_ISP_IQ_GetACnrParam(AX_U8 nPipeId, AX_ISP_IQ_ACNR_PARAM_T *pIspACnrParam);

AX_S32 AX_ISP_IQ_SetScmParam(AX_U8 nPipeId, AX_ISP_IQ_SCM_PARAM_T *pIspScmParam);
AX_S32 AX_ISP_IQ_GetScmParam(AX_U8 nPipeId, AX_ISP_IQ_SCM_PARAM_T *pIspScmParam);

AX_S32 AX_ISP_IQ_SetYcprocParam(AX_U8 nPipeId, AX_ISP_IQ_YCPROC_PARAM_T *pIspYcprocParam);
AX_S32 AX_ISP_IQ_GetYcprocParam(AX_U8 nPipeId, AX_ISP_IQ_YCPROC_PARAM_T *pIspYcprocParam);

AX_S32 AX_ISP_IQ_SetCcmpParam(AX_U8 nPipeId, AX_ISP_IQ_CCMP_PARAM_T *pIspCcmpParam);
AX_S32 AX_ISP_IQ_GetCcmpParam(AX_U8 nPipeId, AX_ISP_IQ_CCMP_PARAM_T *pIspCcmpParam);

AX_S32 AX_ISP_IQ_SetYcrtParam(AX_U8 nPipeId, AX_ISP_IQ_YCRT_PARAM_T *pIspYcrtParam);
AX_S32 AX_ISP_IQ_GetYcrtParam(AX_U8 nPipeId, AX_ISP_IQ_YCRT_PARAM_T *pIspYcrtParam);

AX_S32 AX_ISP_IQ_SetAeStatParam(AX_U8 nPipeId, const AX_ISP_IQ_AE_STAT_PARAM_T *pAeStatParam);
AX_S32 AX_ISP_IQ_GetAeStatParam(AX_U8 nPipeId, AX_ISP_IQ_AE_STAT_PARAM_T *pAeStatParam);

AX_S32 AX_ISP_IQ_SetAwb0StatParam(AX_U8 nPipeId, const AX_ISP_IQ_AWB_STAT_PARAM_T *pAwbStatParam);
AX_S32 AX_ISP_IQ_GetAwb0StatParam(AX_U8 nPipeId, AX_ISP_IQ_AWB_STAT_PARAM_T *pAwbStatParam);
AX_S32 AX_ISP_IQ_SetAwb1StatParam(AX_U8 nPipeId, const AX_ISP_IQ_AWB_STAT_PARAM_T *pAwbStatParam);
AX_S32 AX_ISP_IQ_GetAwb1StatParam(AX_U8 nPipeId, AX_ISP_IQ_AWB_STAT_PARAM_T *pAwbStatParam);

AX_S32 AX_ISP_IQ_SetAf0StatParam(AX_U8 nPipeId, const AX_ISP_IQ_AF_STAT_PARAM_T *pAfStatParam);
AX_S32 AX_ISP_IQ_GetAf0StatParam(AX_U8 nPipeId, AX_ISP_IQ_AF_STAT_PARAM_T *pAfStatParam);
AX_S32 AX_ISP_IQ_SetAf1StatParam(AX_U8 nPipeId, const AX_ISP_IQ_AF_STAT_PARAM_T *pAfStatParam);
AX_S32 AX_ISP_IQ_GetAf1StatParam(AX_U8 nPipeId, AX_ISP_IQ_AF_STAT_PARAM_T *pAfStatParam);

AX_S32 AX_ISP_IQ_SetAFIirRefList(AX_U8 nPipeId, const AX_ISP_IQ_AF_IIR_REF_LIST_T *pIirRefList);
AX_S32 AX_ISP_IQ_GetAFIirRefList(AX_U8 nPipeId, AX_ISP_IQ_AF_IIR_REF_LIST_T *pIirRefList);

AX_S32 AX_ISP_IQ_GetAEStatistics(AX_U8 nPipeId, AX_ISP_AE_STAT_INFO_T *pAeStat);
AX_S32 AX_ISP_IQ_GetAWB0Statistics(AX_U8 nPipeId, AX_ISP_AWB_STAT_INFO_T *pAwbStat);
AX_S32 AX_ISP_IQ_GetAWB1Statistics(AX_U8 nPipeId, AX_ISP_AWB_STAT_INFO_T *pAwbStat);
AX_S32 AX_ISP_IQ_GetAF0Statistics(AX_U8 nPipeId, AX_ISP_AF_STAT_INFO_T *pAfStat);
AX_S32 AX_ISP_IQ_GetAF1Statistics(AX_U8 nPipeId, AX_ISP_AF_STAT_INFO_T *pAfStat);

AX_S32 AX_ISP_IQ_SetDepurpleParam(AX_U8 nPipeId, AX_ISP_IQ_DEPURPLE_PARAM_T *pIspDepurpleParam);
AX_S32 AX_ISP_IQ_GetDepurpleParam(AX_U8 nPipeId, AX_ISP_IQ_DEPURPLE_PARAM_T *pIspDepurpleParam);

AX_S32 AX_ISP_IQ_SetHdrParam(AX_U8 nPipeId, AX_ISP_IQ_HDR_PARAM_T *pIspHdrParam);
AX_S32 AX_ISP_IQ_GetHdrParam(AX_U8 nPipeId, AX_ISP_IQ_HDR_PARAM_T *pIspHdrParam);

AX_S32 AX_ISP_IQ_SetRaw3dnrParam(AX_U8 nPipeId, AX_ISP_IQ_RAW3DNR_PARAM_T *pIspRaw3dnrParam);
AX_S32 AX_ISP_IQ_GetRaw3dnrParam(AX_U8 nPipeId, AX_ISP_IQ_RAW3DNR_PARAM_T *pIspRaw3dnrParam);

AX_S32 AX_ISP_IQ_Set3DlutParam(AX_U8 nPipeId, AX_ISP_IQ_3DLUT_PARAM_T *pIsp3dlutParam);
AX_S32 AX_ISP_IQ_Get3DlutParam(AX_U8 nPipeId, AX_ISP_IQ_3DLUT_PARAM_T *pIsp3dlutParam);

AX_S32 AX_ISP_IQ_SetAinrParam(AX_U8 nPipeId, AX_ISP_IQ_AINR_PARAM_T *pIspAinrParam);
AX_S32 AX_ISP_IQ_GetAinrParam(AX_U8 nPipeId, AX_ISP_IQ_AINR_PARAM_T *pIspAinrParam);

AX_S32 AX_ISP_IQ_GetAinrCapability(AX_U8 nPipeId, AX_ISP_IQ_AINR_CAP_TABLE_T *pIspAiNrCapability);

AX_S32 AX_ISP_IQ_SetDePwlParam(AX_U8 nPipeId, const AX_ISP_IQ_DEPWL_PARAM_T *pIspDePwlParam);
AX_S32 AX_ISP_IQ_GetDePwlParam(AX_U8 nPipeId, AX_ISP_IQ_DEPWL_PARAM_T *pIspDePwlParam);

AX_S32 AX_ISP_IQ_SetAiceParam(AX_U8 nPipeId, AX_ISP_IQ_AICE_PARAM_T *pIspAiceParam);
AX_S32 AX_ISP_IQ_GetAiceParam(AX_U8 nPipeId, AX_ISP_IQ_AICE_PARAM_T *pIspAiceParam);
AX_S32 AX_ISP_IQ_GetAiceCapability(AX_U8 nPipeId, AX_ISP_IQ_AICE_CAP_TABLE_T *pIspAiceCapability);

AX_S32 AX_ISP_IQ_SetDehazeParam(AX_U8 nPipeId, AX_ISP_IQ_DEHAZE_PARAM_T *pIspDehazeParam);
AX_S32 AX_ISP_IQ_GetDehazeParam(AX_U8 nPipeId, AX_ISP_IQ_DEHAZE_PARAM_T *pIspDehazeParam);

AX_S32 AX_ISP_IQ_SetSceneParam(AX_U8 nPipeId, AX_ISP_IQ_SCENE_PARAM_T *pIspSceneParam);
AX_S32 AX_ISP_IQ_GetSceneParam(AX_U8 nPipeId, AX_ISP_IQ_SCENE_PARAM_T *pIspSceneParam);

AX_S32 AX_ISP_IQ_SetLdcParam(AX_U8 nPipeId, AX_ISP_IQ_LDC_PARAM_T *pIspLDCParam);
AX_S32 AX_ISP_IQ_GetLdcParam(AX_U8 nPipeId, AX_ISP_IQ_LDC_PARAM_T *pIspLDCParam);

#ifdef __cplusplus
}
#endif
#endif  //_AX_ISP_IQ_API_H_
