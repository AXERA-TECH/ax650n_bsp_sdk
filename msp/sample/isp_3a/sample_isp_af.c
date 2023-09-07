/**********************************************************************************
 *
 * Copyright (c) 2019-2020 Beijing AXera Technology Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Beijing AXera Technology Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Beijing AXera Technology Co., Ltd.
 *
 **********************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include "ax_isp_api.h"
#include "ax_sys_api.h"
#include "sample_isp_af.h"
#include <pthread.h>
#include <assert.h>
#include "ax_isp_3a_api.h"
#include "common_cam.h"
#include "common_type.h"

extern AX_CAMERA_T gCams[MAX_CAMERAS];
static AX_ISP_IQ_AF_STAT_PARAM_T af_itp_wbc_param_sdr =
{
    /* nAfEnable */
    1,
    /* eAfStatPos */
    /* 0:AX_ISP_AF_STAT_IFE_LONG_FRAME, 1:AX_ISP_AF_STAT_IFE_SHORT_FRAME, 2:AX_ISP_AF_STAT_IFE_VERY_SHORT_FRAME, 3:AX_ISP_AF_STAT_AFTER_ITP_WBC, 4:AX_ISP_AF_STAT_AFTER_ITP_RLTM, 5:AX_ISP_AF_STAT_POSITION_MAX */
    AX_ISP_AF_STAT_AFTER_ITP_WBC,
    /* tAfBayer2Y */
    {
        /* nCoeffR */
        1024,
        /* nCoeffGb */
        1024,
        /* nCoeffGr */
        1024,
        /* nCoeffB */
        1024,
    },
    /* tAfGamma */
    {
        /* nGammaEnable */
        1,
        /* nGammaLut[33] */
        {
            0, 3390, 4646, 5586, 6366, 7046, 7655, 8211, 8724, 9204, 9656, 10083, 10490, 10879, 11251, 11610, 11956, 12290, 12613, 12927, 13232, 13529, 13818, 14100, 14375, 14644, 14908, 15166, 15419, 15667, 15910, 16149,  /* 0 - 31*/
            16383, /*32 - 32*/
        },
    },
    /* tAfScaler */
    {
        /* nScaleEnable */
        1,
        /* nScaleFactor */
        1,
        /* nScaleWeight[16] */
        {2047, 2047, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 15*/},
    },
    /* tAfFilter */
    {
        /* nFirEnable */
        0,
        /* nViirRefId */
        0,
        /* nH1IirRefId */
        0,
        /* nH2IirRefId */
        0,
    },
    /* tAfCoring */
    {
        /* nCoringThr */
        1,
        /* nCoringGain */
        16,
        /* nCoringLut[16] */
        {1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 4, 5, 7, 9, 13, 14, /*0 - 15*/},
    },
    /* tAfBrightPix */
    {
        /* nBrightPixMaskEnable */
        0,
        /* nBrightPixMaskThr */
        0,
        /* nBrightPixMaskLv */
        0,
        /* nBrightPixCountThr */
        0,
    },
    /* tAfDrc */
    {
        /* nDrcEnable */
        0,
        /* nDrcLut[17] */
        {0, 0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16383, /*0 - 16*/},
    },
    /* tAfRoi */
    {
        /* nRoiOffsetH */
        64,
        /* nRoiOffsetV */
        20,
        /* nRoiRegionNumH */
        18,
        /* nRoiRegionNumV */
        10,
        /* nRoiRegionW */
        224,
        /* nRoiRegionH */
        212,
    },
    /* tAfLumaSuppress */
    {
        /* nSuppressMode */
        1,
        /* tLumaSuppressUserCurve */
        {
            /* nLumaLowStartTh */
            32,
            /* nLumaLowEndTh */
            96,
            /* nLumaLowMinRatio */
            917504,
            /* nLumaHighStartTh */
            192,
            /* nLumaHighEndTh */
            220,
            /* nLumaHighMinRatio */
            917504,
        },
    },
    /* tAfPeakEnhance */
    {
        /* nSquareMode */
        0,
    },
};

static AX_ISP_IQ_AF_STAT_PARAM_T af_itp_wbc_param_hdr =
{
    /* nAfEnable */
    1,
    /* eAfStatPos */
    /* 0:AX_ISP_AF_STAT_IFE_LONG_FRAME, 1:AX_ISP_AF_STAT_IFE_SHORT_FRAME, 2:AX_ISP_AF_STAT_IFE_VERY_SHORT_FRAME, 3:AX_ISP_AF_STAT_AFTER_ITP_WBC, 4:AX_ISP_AF_STAT_AFTER_ITP_RLTM, 5:AX_ISP_AF_STAT_POSITION_MAX */
    AX_ISP_AF_STAT_AFTER_ITP_WBC,
    /* tAfBayer2Y */
    {
        /* nCoeffR */
        1024,
        /* nCoeffGb */
        1024,
        /* nCoeffGr */
        1024,
        /* nCoeffB */
        1024,
    },
    /* tAfGamma */
    {
        /* nGammaEnable */
        1,
        /* nGammaLut[33] */
        {
            0, 3390, 4646, 5586, 6366, 7046, 7655, 8211, 8724, 9204, 9656, 10083, 10490, 10879, 11251, 11610, 11956, 12290, 12613, 12927, 13232, 13529, 13818, 14100, 14375, 14644, 14908, 15166, 15419, 15667, 15910, 16149,  /* 0 - 31*/
            16383, /*32 - 32*/
        },
    },
    /* tAfScaler */
    {
        /* nScaleEnable */
        1,
        /* nScaleFactor */
        1,
        /* nScaleWeight[16] */
        {2047, 2047, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 15*/},
    },
    /* tAfFilter */
    {
        /* nFirEnable */
        0,
        /* nViirRefId */
        0,
        /* nH1IirRefId */
        0,
        /* nH2IirRefId */
        0,
    },
    /* tAfCoring */
    {
        /* nCoringThr */
        1,
        /* nCoringGain */
        16,
        /* nCoringLut[16] */
        {1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 4, 5, 7, 9, 13, 14, /*0 - 15*/},
    },
    /* tAfBrightPix */
    {
        /* nBrightPixMaskEnable */
        0,
        /* nBrightPixMaskThr */
        0,
        /* nBrightPixMaskLv */
        0,
        /* nBrightPixCountThr */
        0,
    },
    /* tAfDrc */
    {
        /* nDrcEnable */
        1,
        /* nDrcLut[17] */
        {0, 0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16383, /*0 - 16*/},
    },
    /* tAfRoi */
    {
        /* nRoiOffsetH */
        64,
        /* nRoiOffsetV */
        20,
        /* nRoiRegionNumH */
        18,
        /* nRoiRegionNumV */
        10,
        /* nRoiRegionW */
        224,
        /* nRoiRegionH */
        212,
    },
    /* tAfLumaSuppress */
    {
        /* nSuppressMode */
        1,
        /* tLumaSuppressUserCurve */
        {
            /* nLumaLowStartTh */
            32,
            /* nLumaLowEndTh */
            96,
            /* nLumaLowMinRatio */
            917504,
            /* nLumaHighStartTh */
            192,
            /* nLumaHighEndTh */
            220,
            /* nLumaHighMinRatio */
            917504,
        },
    },
    /* tAfPeakEnhance */
    {
        /* nSquareMode */
        0,
    },
};

static AX_ISP_IQ_AF_STAT_PARAM_T af_itp_rltm_param =
{
    /* nAfEnable */
    1,
    /* eAfStatPos */
    /* 0:AX_ISP_AF_STAT_IFE_LONG_FRAME, 1:AX_ISP_AF_STAT_IFE_SHORT_FRAME, 2:AX_ISP_AF_STAT_IFE_VERY_SHORT_FRAME, 3:AX_ISP_AF_STAT_AFTER_ITP_WBC, 4:AX_ISP_AF_STAT_AFTER_ITP_RLTM, 5:AX_ISP_AF_STAT_POSITION_MAX */
    AX_ISP_AF_STAT_AFTER_ITP_RLTM,
    /* tAfBayer2Y */
    {
        /* nCoeffR */
        1024,
        /* nCoeffGb */
        1024,
        /* nCoeffGr */
        1024,
        /* nCoeffB */
        1024,
    },
    /* tAfGamma */
    {
        /* nGammaEnable */
        1,
        /* nGammaLut[33] */
        {
            0, 3390, 4646, 5586, 6366, 7046, 7655, 8211, 8724, 9204, 9656, 10083, 10490, 10879, 11251, 11610, 11956, 12290, 12613, 12927, 13232, 13529, 13818, 14100, 14375, 14644, 14908, 15166, 15419, 15667, 15910, 16149,  /* 0 - 31*/
            16383, /*32 - 32*/
        },
    },
    /* tAfScaler */
    {
        /* nScaleEnable */
        1,
        /* nScaleFactor */
        1,
        /* nScaleWeight[16] */
        {2047, 2047, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 15*/},
    },
    /* tAfFilter */
    {
        /* nFirEnable */
        0,
        /* nViirRefId */
        0,
        /* nH1IirRefId */
        0,
        /* nH2IirRefId */
        0,
    },
    /* tAfCoring */
    {
        /* nCoringThr */
        1,
        /* nCoringGain */
        16,
        /* nCoringLut[16] */
        {1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 4, 5, 7, 9, 13, 14, /*0 - 15*/},
    },
    /* tAfBrightPix */
    {
        /* nBrightPixMaskEnable */
        0,
        /* nBrightPixMaskThr */
        0,
        /* nBrightPixMaskLv */
        0,
        /* nBrightPixCountThr */
        0,
    },
    /* tAfDrc */
    {
        /* nDrcEnable */
        1,
        /* nDrcLut[17] */
        {0, 152, 208, 284, 388, 530, 724, 989, 1351, 1846, 2521, 3444, 4705, 6427, 8780, 11994, 16383, /*0 - 16*/},
    },
    /* tAfRoi */
    {
        /* nRoiOffsetH */
        64,
        /* nRoiOffsetV */
        20,
        /* nRoiRegionNumH */
        18,
        /* nRoiRegionNumV */
        10,
        /* nRoiRegionW */
        224,
        /* nRoiRegionH */
        212,
    },
    /* tAfLumaSuppress */
    {
        /* nSuppressMode */
        1,
        /* tLumaSuppressUserCurve */
        {
            /* nLumaLowStartTh */
            32,
            /* nLumaLowEndTh */
            96,
            /* nLumaLowMinRatio */
            917504,
            /* nLumaHighStartTh */
            192,
            /* nLumaHighEndTh */
            220,
            /* nLumaHighMinRatio */
            917504,
        },
    },
    /* tAfPeakEnhance */
    {
        /* nSquareMode */
        0,
    },
};

static AX_ISP_IQ_AF_STAT_PARAM_T af_ife_param =
{
    /* nAfEnable */
    1,
    /* eAfStatPos */
    /* 0:AX_ISP_AF_STAT_IFE_LONG_FRAME, 1:AX_ISP_AF_STAT_IFE_SHORT_FRAME, 2:AX_ISP_AF_STAT_IFE_VERY_SHORT_FRAME, 3:AX_ISP_AF_STAT_AFTER_ITP_WBC, 4:AX_ISP_AF_STAT_AFTER_ITP_RLTM, 5:AX_ISP_AF_STAT_POSITION_MAX */
    AX_ISP_AF_STAT_IFE_LONG_FRAME,
    /* tAfBayer2Y */
    {
        /* nCoeffR */
        1024,
        /* nCoeffGb */
        1024,
        /* nCoeffGr */
        1024,
        /* nCoeffB */
        1024,
    },
    /* tAfGamma */
    {
        /* nGammaEnable */
        1,
        /* nGammaLut[33] */
        {
            0, 3390, 4646, 5586, 6366, 7046, 7655, 8211, 8724, 9204, 9656, 10083, 10490, 10879, 11251, 11610, 11956, 12290, 12613, 12927, 13232, 13529, 13818, 14100, 14375, 14644, 14908, 15166, 15419, 15667, 15910, 16149,  /* 0 - 31*/
            16383, /*32 - 32*/
        },
    },
    /* tAfScaler */
    {
        /* nScaleEnable */
        1,
        /* nScaleFactor */
        1,
        /* nScaleWeight[16] */
        {2047, 2047, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 15*/},
    },
    /* tAfFilter */
    {
        /* nFirEnable */
        0,
        /* nViirRefId */
        0,
        /* nH1IirRefId */
        0,
        /* nH2IirRefId */
        0,
    },
    /* tAfCoring */
    {
        /* nCoringThr */
        1,
        /* nCoringGain */
        16,
        /* nCoringLut[16] */
        {1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 4, 5, 7, 9, 13, 14, /*0 - 15*/},
    },
    /* tAfBrightPix */
    {
        /* nBrightPixMaskEnable */
        0,
        /* nBrightPixMaskThr */
        0,
        /* nBrightPixMaskLv */
        0,
        /* nBrightPixCountThr */
        0,
    },
    /* tAfDrc */
    {
        /* nDrcEnable */
        1,
        /* nDrcLut[17] */
        {0, 0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16383, /*0 - 16*/},
    },
    /* tAfRoi */
    {
        /* nRoiOffsetH */
        64,
        /* nRoiOffsetV */
        20,
        /* nRoiRegionNumH */
        18,
        /* nRoiRegionNumV */
        10,
        /* nRoiRegionW */
        224,
        /* nRoiRegionH */
        212,
    },
    /* tAfLumaSuppress */
    {
        /* nSuppressMode */
        1,
        /* tLumaSuppressUserCurve */
        {
            /* nLumaLowStartTh */
            32,
            /* nLumaLowEndTh */
            96,
            /* nLumaLowMinRatio */
            917504,
            /* nLumaHighStartTh */
            192,
            /* nLumaHighEndTh */
            220,
            /* nLumaHighMinRatio */
            917504,
        },
    },
    /* tAfPeakEnhance */
    {
        /* nSquareMode */
        0,
    },
};


AX_ISP_IQ_AF_IIR_REF_LIST_T af_iir_ref_list_param =
{
    /* nViirRefNum */
    21,
    /* nH1IirRefNum */
    21,
    /* nH2IirRefNum */
    21,
    /* tVIirRefList[32] */
    {
        /* 0 */
        {
            /* nStartFreq */
            104858,
            /* nEndFreq */
            314573,
            /* nIirCoefList[10] */
            {537, 1074, 537, 3063, -1114, 3279, -6557, 3279, 6394, -2626, /*0 - 9*/},
        },
        /* 1 */
        {
            /* nStartFreq */
            314573,
            /* nEndFreq */
            524288,
            /* nIirCoefList[10] */
            {1200, 2399, 1200, 0, -702, 2068, -4136, 2068, 3063, -1114, /*0 - 9*/},
        },
        /* 2 */
        {
            /* nStartFreq */
            524288,
            /* nEndFreq */
            734003,
            /* nIirCoefList[10] */
            {2068, 4137, 2068, -3062, -1114, 1200, -2398, 1200, 0, -702, /*0 - 9*/},
        },
        /* 3 */
        {
            /* nStartFreq */
            41943,
            /* nEndFreq */
            104858,
            /* nIirCoefList[10] */
            {82, 165, 82, 6394, -2626, 3748, -7494, 3748, 7466, -3428, /*0 - 9*/},
        },
        /* 4 */
        {
            /* nStartFreq */
            104858,
            /* nEndFreq */
            209715,
            /* nIirCoefList[10] */
            {276, 553, 276, 4682, -1690, 3279, -6557, 3279, 6394, -2626, /*0 - 9*/},
        },
        /* 5 */
        {
            /* nStartFreq */
            209715,
            /* nEndFreq */
            314573,
            /* nIirCoefList[10] */
            {537, 1074, 537, 3063, -1114, 2617, -5233, 2617, 4682, -1690, /*0 - 9*/},
        },
        /* 6 */
        {
            /* nStartFreq */
            314573,
            /* nEndFreq */
            419430,
            /* nIirCoefList[10] */
            {846, 1692, 846, 1514, -801, 2068, -4136, 2068, 3063, -1114, /*0 - 9*/},
        },
        /* 7 */
        {
            /* nStartFreq */
            419430,
            /* nEndFreq */
            524288,
            /* nIirCoefList[10] */
            {1200, 2399, 1200, 0, -702, 1603, -3205, 1603, 1514, -801, /*0 - 9*/},
        },
        /* 8 */
        {
            /* nStartFreq */
            524288,
            /* nEndFreq */
            629146,
            /* nIirCoefList[10] */
            {1603, 3206, 1603, -1513, -801, 1200, -2398, 1200, 0, -702, /*0 - 9*/},
        },
        /* 9 */
        {
            /* nStartFreq */
            41943,
            /* nEndFreq */
            209715,
            /* nIirCoefList[10] */
            {276, 553, 276, 4682, -1690, 3748, -7494, 3748, 7466, -3428, /*0 - 9*/},
        },
        /* 10 */
        {
            /* nStartFreq */
            20972,
            /* nEndFreq */
            83886,
            /* nIirCoefList[10] */
            {55, 109, 55, 6748, -2870, 3918, -7835, 3918, 7828, -3747, /*0 - 9*/},
        },
        /* 11 */
        {
            /* nStartFreq */
            41943,
            /* nEndFreq */
            83886,
            /* nIirCoefList[10] */
            {55, 109, 55, 6748, -2870, 3748, -7494, 3748, 7466, -3428, /*0 - 9*/},
        },
        /* 12 */
        {
            /* nStartFreq */
            41943,
            /* nEndFreq */
            62915,
            /* nIirCoefList[10] */
            {32, 64, 32, 7105, -3137, 3748, -7494, 3748, 7466, -3428, /*0 - 9*/},
        },
        /* 13 */
        {
            /* nStartFreq */
            20972,
            /* nEndFreq */
            62915,
            /* nIirCoefList[10] */
            {32, 64, 32, 7105, -3137, 3918, -7835, 3918, 7828, -3747, /*0 - 9*/},
        },
        /* 14 */
        {
            /* nStartFreq */
            20972,
            /* nEndFreq */
            52429,
            /* nIirCoefList[10] */
            {23, 45, 23, 7285, -3279, 3918, -7835, 3918, 7828, -3747, /*0 - 9*/},
        },
        /* 15 */
        {
            /* nStartFreq */
            20972,
            /* nEndFreq */
            41943,
            /* nIirCoefList[10] */
            {15, 30, 15, 7466, -3428, 3918, -7835, 3918, 7828, -3747, /*0 - 9*/},
        },
        /* 16 */
        {
            /* nStartFreq */
            10486,
            /* nEndFreq */
            41943,
            /* nIirCoefList[10] */
            {15, 30, 15, 7466, -3428, 4006, -8011, 4006, 8010, -3917, /*0 - 9*/},
        },
        /* 17 */
        {
            /* nStartFreq */
            10486,
            /* nEndFreq */
            83886,
            /* nIirCoefList[10] */
            {55, 109, 55, 6748, -2870, 4006, -8011, 4006, 8010, -3917, /*0 - 9*/},
        },
        /* 18 */
        {
            /* nStartFreq */
            10486,
            /* nEndFreq */
            62915,
            /* nIirCoefList[10] */
            {32, 64, 32, 7105, -3137, 4006, -8011, 4006, 8010, -3917, /*0 - 9*/},
        },
        /* 19 */
        {
            /* nStartFreq */
            10486,
            /* nEndFreq */
            52429,
            /* nIirCoefList[10] */
            {23, 45, 23, 7285, -3279, 4006, -8011, 4006, 8010, -3917, /*0 - 9*/},
        },
        /* 20 */
        {
            /* nStartFreq */
            20972,
            /* nEndFreq */
            104858,
            /* nIirCoefList[10] */
            {82, 165, 82, 6394, -2626, 3918, -7835, 3918, 7828, -3747, /*0 - 9*/},
        },
        /* 21 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 22 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 23 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 24 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 25 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 26 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 27 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 28 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 29 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 30 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 31 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
    },
    /* tH1IirRefList[32] */
    {
        /* 0 */
        {
            /* nStartFreq */
            104858,
            /* nEndFreq */
            314573,
            /* nIirCoefList[10] */
            {537, 1074, 537, 3063, -1114, 3279, -6557, 3279, 6394, -2626, /*0 - 9*/},
        },
        /* 1 */
        {
            /* nStartFreq */
            314573,
            /* nEndFreq */
            524288,
            /* nIirCoefList[10] */
            {1200, 2399, 1200, 0, -702, 2068, -4136, 2068, 3063, -1114, /*0 - 9*/},
        },
        /* 2 */
        {
            /* nStartFreq */
            524288,
            /* nEndFreq */
            734003,
            /* nIirCoefList[10] */
            {2068, 4137, 2068, -3062, -1114, 1200, -2398, 1200, 0, -702, /*0 - 9*/},
        },
        /* 3 */
        {
            /* nStartFreq */
            41943,
            /* nEndFreq */
            104858,
            /* nIirCoefList[10] */
            {82, 165, 82, 6394, -2626, 3748, -7494, 3748, 7466, -3428, /*0 - 9*/},
        },
        /* 4 */
        {
            /* nStartFreq */
            104858,
            /* nEndFreq */
            209715,
            /* nIirCoefList[10] */
            {276, 553, 276, 4682, -1690, 3279, -6557, 3279, 6394, -2626, /*0 - 9*/},
        },
        /* 5 */
        {
            /* nStartFreq */
            209715,
            /* nEndFreq */
            314573,
            /* nIirCoefList[10] */
            {537, 1074, 537, 3063, -1114, 2617, -5233, 2617, 4682, -1690, /*0 - 9*/},
        },
        /* 6 */
        {
            /* nStartFreq */
            314573,
            /* nEndFreq */
            419430,
            /* nIirCoefList[10] */
            {846, 1692, 846, 1514, -801, 2068, -4136, 2068, 3063, -1114, /*0 - 9*/},
        },
        /* 7 */
        {
            /* nStartFreq */
            419430,
            /* nEndFreq */
            524288,
            /* nIirCoefList[10] */
            {1200, 2399, 1200, 0, -702, 1603, -3205, 1603, 1514, -801, /*0 - 9*/},
        },
        /* 8 */
        {
            /* nStartFreq */
            524288,
            /* nEndFreq */
            629146,
            /* nIirCoefList[10] */
            {1603, 3206, 1603, -1513, -801, 1200, -2398, 1200, 0, -702, /*0 - 9*/},
        },
        /* 9 */
        {
            /* nStartFreq */
            41943,
            /* nEndFreq */
            209715,
            /* nIirCoefList[10] */
            {276, 553, 276, 4682, -1690, 3748, -7494, 3748, 7466, -3428, /*0 - 9*/},
        },
        /* 10 */
        {
            /* nStartFreq */
            20972,
            /* nEndFreq */
            83886,
            /* nIirCoefList[10] */
            {55, 109, 55, 6748, -2870, 3918, -7835, 3918, 7828, -3747, /*0 - 9*/},
        },
        /* 11 */
        {
            /* nStartFreq */
            41943,
            /* nEndFreq */
            83886,
            /* nIirCoefList[10] */
            {55, 109, 55, 6748, -2870, 3748, -7494, 3748, 7466, -3428, /*0 - 9*/},
        },
        /* 12 */
        {
            /* nStartFreq */
            41943,
            /* nEndFreq */
            62915,
            /* nIirCoefList[10] */
            {32, 64, 32, 7105, -3137, 3748, -7494, 3748, 7466, -3428, /*0 - 9*/},
        },
        /* 13 */
        {
            /* nStartFreq */
            20972,
            /* nEndFreq */
            62915,
            /* nIirCoefList[10] */
            {32, 64, 32, 7105, -3137, 3918, -7835, 3918, 7828, -3747, /*0 - 9*/},
        },
        /* 14 */
        {
            /* nStartFreq */
            20972,
            /* nEndFreq */
            52429,
            /* nIirCoefList[10] */
            {23, 45, 23, 7285, -3279, 3918, -7835, 3918, 7828, -3747, /*0 - 9*/},
        },
        /* 15 */
        {
            /* nStartFreq */
            20972,
            /* nEndFreq */
            41943,
            /* nIirCoefList[10] */
            {15, 30, 15, 7466, -3428, 3918, -7835, 3918, 7828, -3747, /*0 - 9*/},
        },
        /* 16 */
        {
            /* nStartFreq */
            10486,
            /* nEndFreq */
            41943,
            /* nIirCoefList[10] */
            {15, 30, 15, 7466, -3428, 4006, -8011, 4006, 8010, -3917, /*0 - 9*/},
        },
        /* 17 */
        {
            /* nStartFreq */
            10486,
            /* nEndFreq */
            83886,
            /* nIirCoefList[10] */
            {55, 109, 55, 6748, -2870, 4006, -8011, 4006, 8010, -3917, /*0 - 9*/},
        },
        /* 18 */
        {
            /* nStartFreq */
            10486,
            /* nEndFreq */
            62915,
            /* nIirCoefList[10] */
            {32, 64, 32, 7105, -3137, 4006, -8011, 4006, 8010, -3917, /*0 - 9*/},
        },
        /* 19 */
        {
            /* nStartFreq */
            10486,
            /* nEndFreq */
            52429,
            /* nIirCoefList[10] */
            {23, 45, 23, 7285, -3279, 4006, -8011, 4006, 8010, -3917, /*0 - 9*/},
        },
        /* 20 */
        {
            /* nStartFreq */
            20972,
            /* nEndFreq */
            104858,
            /* nIirCoefList[10] */
            {82, 165, 82, 6394, -2626, 3918, -7835, 3918, 7828, -3747, /*0 - 9*/},
        },
        /* 21 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 22 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 23 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 24 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 25 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 26 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 27 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 28 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 29 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 30 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 31 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
    },
    /* tH2IirRefList[32] */
    {
        /* 0 */
        {
            /* nStartFreq */
            104858,
            /* nEndFreq */
            314573,
            /* nIirCoefList[10] */
            {537, 1074, 537, 3063, -1114, 3279, -6557, 3279, 6394, -2626, /*0 - 9*/},
        },
        /* 1 */
        {
            /* nStartFreq */
            314573,
            /* nEndFreq */
            524288,
            /* nIirCoefList[10] */
            {1200, 2399, 1200, 0, -702, 2068, -4136, 2068, 3063, -1114, /*0 - 9*/},
        },
        /* 2 */
        {
            /* nStartFreq */
            524288,
            /* nEndFreq */
            734003,
            /* nIirCoefList[10] */
            {2068, 4137, 2068, -3062, -1114, 1200, -2398, 1200, 0, -702, /*0 - 9*/},
        },
        /* 3 */
        {
            /* nStartFreq */
            41943,
            /* nEndFreq */
            104858,
            /* nIirCoefList[10] */
            {82, 165, 82, 6394, -2626, 3748, -7494, 3748, 7466, -3428, /*0 - 9*/},
        },
        /* 4 */
        {
            /* nStartFreq */
            104858,
            /* nEndFreq */
            209715,
            /* nIirCoefList[10] */
            {276, 553, 276, 4682, -1690, 3279, -6557, 3279, 6394, -2626, /*0 - 9*/},
        },
        /* 5 */
        {
            /* nStartFreq */
            209715,
            /* nEndFreq */
            314573,
            /* nIirCoefList[10] */
            {537, 1074, 537, 3063, -1114, 2617, -5233, 2617, 4682, -1690, /*0 - 9*/},
        },
        /* 6 */
        {
            /* nStartFreq */
            314573,
            /* nEndFreq */
            419430,
            /* nIirCoefList[10] */
            {846, 1692, 846, 1514, -801, 2068, -4136, 2068, 3063, -1114, /*0 - 9*/},
        },
        /* 7 */
        {
            /* nStartFreq */
            419430,
            /* nEndFreq */
            524288,
            /* nIirCoefList[10] */
            {1200, 2399, 1200, 0, -702, 1603, -3205, 1603, 1514, -801, /*0 - 9*/},
        },
        /* 8 */
        {
            /* nStartFreq */
            524288,
            /* nEndFreq */
            629146,
            /* nIirCoefList[10] */
            {1603, 3206, 1603, -1513, -801, 1200, -2398, 1200, 0, -702, /*0 - 9*/},
        },
        /* 9 */
        {
            /* nStartFreq */
            41943,
            /* nEndFreq */
            209715,
            /* nIirCoefList[10] */
            {276, 553, 276, 4682, -1690, 3748, -7494, 3748, 7466, -3428, /*0 - 9*/},
        },
        /* 10 */
        {
            /* nStartFreq */
            20972,
            /* nEndFreq */
            83886,
            /* nIirCoefList[10] */
            {55, 109, 55, 6748, -2870, 3918, -7835, 3918, 7828, -3747, /*0 - 9*/},
        },
        /* 11 */
        {
            /* nStartFreq */
            41943,
            /* nEndFreq */
            83886,
            /* nIirCoefList[10] */
            {55, 109, 55, 6748, -2870, 3748, -7494, 3748, 7466, -3428, /*0 - 9*/},
        },
        /* 12 */
        {
            /* nStartFreq */
            41943,
            /* nEndFreq */
            62915,
            /* nIirCoefList[10] */
            {32, 64, 32, 7105, -3137, 3748, -7494, 3748, 7466, -3428, /*0 - 9*/},
        },
        /* 13 */
        {
            /* nStartFreq */
            20972,
            /* nEndFreq */
            62915,
            /* nIirCoefList[10] */
            {32, 64, 32, 7105, -3137, 3918, -7835, 3918, 7828, -3747, /*0 - 9*/},
        },
        /* 14 */
        {
            /* nStartFreq */
            20972,
            /* nEndFreq */
            52429,
            /* nIirCoefList[10] */
            {23, 45, 23, 7285, -3279, 3918, -7835, 3918, 7828, -3747, /*0 - 9*/},
        },
        /* 15 */
        {
            /* nStartFreq */
            20972,
            /* nEndFreq */
            41943,
            /* nIirCoefList[10] */
            {15, 30, 15, 7466, -3428, 3918, -7835, 3918, 7828, -3747, /*0 - 9*/},
        },
        /* 16 */
        {
            /* nStartFreq */
            10486,
            /* nEndFreq */
            41943,
            /* nIirCoefList[10] */
            {15, 30, 15, 7466, -3428, 4006, -8011, 4006, 8010, -3917, /*0 - 9*/},
        },
        /* 17 */
        {
            /* nStartFreq */
            10486,
            /* nEndFreq */
            83886,
            /* nIirCoefList[10] */
            {55, 109, 55, 6748, -2870, 4006, -8011, 4006, 8010, -3917, /*0 - 9*/},
        },
        /* 18 */
        {
            /* nStartFreq */
            10486,
            /* nEndFreq */
            62915,
            /* nIirCoefList[10] */
            {32, 64, 32, 7105, -3137, 4006, -8011, 4006, 8010, -3917, /*0 - 9*/},
        },
        /* 19 */
        {
            /* nStartFreq */
            10486,
            /* nEndFreq */
            52429,
            /* nIirCoefList[10] */
            {23, 45, 23, 7285, -3279, 4006, -8011, 4006, 8010, -3917, /*0 - 9*/},
        },
        /* 20 */
        {
            /* nStartFreq */
            20972,
            /* nEndFreq */
            104858,
            /* nIirCoefList[10] */
            {82, 165, 82, 6394, -2626, 3918, -7835, 3918, 7828, -3747, /*0 - 9*/},
        },
        /* 21 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 22 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 23 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 24 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 25 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 26 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 27 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 28 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 29 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 30 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* 31 */
        {
            /* nStartFreq */
            0,
            /* nEndFreq */
            0,
            /* nIirCoefList[10] */
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
    },
};

#define DEF_AF_GAMMA_LUT_NUM       (33)
#define DEF_AF_WEIGHT_LUT_NUM      (16)
#define DEF_AF_VIIR_LUT_NUM        (10)
#define DEF_AF_H1IIR_LUT_NUM       (10)
#define DEF_AF_H2IIR_LUT_NUM       (10)
#define DEF_AF_H1FIR_LUT_NUM       (13)
#define DEF_AF_H2FIR_LUT_NUM       (13)
#define DEF_AF_CORING_LUT_NUM      (16)
#define DEF_AF_DRC_LUT_NUM         (17)
#define DEF_AF_IIR_REF_LIST_SIZE   (32)
#define DEF_AF_IIR_COEF_NUM        (10)
#define DEF_AF_USED(value)         (void)value
extern volatile AX_S32 gLoopExit;
AX_S32 sample_af_init_afparams
(
    AX_U8                         pipe,
    AX_ISP_IQ_AF_STAT_PARAM_T    *pAfInitPara,
    AX_ISP_IQ_AF_STAT_PARAM_T    *pAfStatParam
)
{
    pAfInitPara->nAfEnable = pAfStatParam->nAfEnable;
    pAfInitPara->eAfStatPos = pAfStatParam->eAfStatPos;

    //pAfInitPara->tAfBayer2Y.nYSel = af_param_sdr.tAfBayer2Y.nYSel;
    //pAfInitPara->tAfBayer2Y.nGrgbSel = af_param_sdr.tAfBayer2Y.nGrgbSel;
    pAfInitPara->tAfBayer2Y.nCoeffR = pAfStatParam->tAfBayer2Y.nCoeffR;
    pAfInitPara->tAfBayer2Y.nCoeffGr = pAfStatParam->tAfBayer2Y.nCoeffGr;
    pAfInitPara->tAfBayer2Y.nCoeffGb = pAfStatParam->tAfBayer2Y.nCoeffGb;
    pAfInitPara->tAfBayer2Y.nCoeffB = pAfStatParam->tAfBayer2Y.nCoeffB;
    pAfInitPara->tAfGamma.nGammaEnable = pAfStatParam->tAfGamma.nGammaEnable;

    for (int i = 0; i < DEF_AF_GAMMA_LUT_NUM; i++)
    {
        pAfInitPara->tAfGamma.nGammaLut[i] = pAfStatParam->tAfGamma.nGammaLut[i];
    }

    pAfInitPara->tAfScaler.nScaleEnable = pAfStatParam->tAfScaler.nScaleEnable;
    pAfInitPara->tAfScaler.nScaleRatio = pAfStatParam->tAfScaler.nScaleRatio;
    for (int i = 0; i < DEF_AF_WEIGHT_LUT_NUM; i++)
    {
        pAfInitPara->tAfScaler.nScaleWeight[i] = pAfStatParam->tAfScaler.nScaleWeight[i];
    }

    pAfInitPara->tAfFilter.nFirEnable = pAfStatParam->tAfFilter.nFirEnable;
    pAfInitPara->tAfFilter.nViirRefId = pAfStatParam->tAfFilter.nViirRefId;
    pAfInitPara->tAfFilter.nH1IirRefId = pAfStatParam->tAfFilter.nH1IirRefId;
    pAfInitPara->tAfFilter.nH2IirRefId = pAfStatParam->tAfFilter.nH2IirRefId;

    pAfInitPara->tAfCoring.nCoringThr = pAfStatParam->tAfCoring.nCoringThr;
    pAfInitPara->tAfCoring.nCoringGain = pAfStatParam->tAfCoring.nCoringGain;

    for (int i = 0; i < DEF_AF_CORING_LUT_NUM; ++i)
    {
        pAfInitPara->tAfCoring.nCoringLut[i] = pAfStatParam->tAfCoring.nCoringLut[i];
    }

    pAfInitPara->tAfDrc.nDrcEnable = pAfStatParam->tAfDrc.nDrcEnable;
    for(int i = 0; i < DEF_AF_DRC_LUT_NUM; i++)
    {
        pAfInitPara->tAfDrc.nDrcLut[i] = pAfStatParam->tAfDrc.nDrcLut[i];
    }

    pAfInitPara->tAfRoi.nRoiOffsetH = pAfStatParam->tAfRoi.nRoiOffsetH;
    pAfInitPara->tAfRoi.nRoiOffsetV = pAfStatParam->tAfRoi.nRoiOffsetV;
    pAfInitPara->tAfRoi.nRoiRegionNumH = pAfStatParam->tAfRoi.nRoiRegionNumH;
    pAfInitPara->tAfRoi.nRoiRegionNumV = pAfStatParam->tAfRoi.nRoiRegionNumV;
    pAfInitPara->tAfRoi.nRoiRegionW = pAfStatParam->tAfRoi.nRoiRegionW;
    pAfInitPara->tAfRoi.nRoiRegionH = pAfStatParam->tAfRoi.nRoiRegionH;

    pAfInitPara->tAfLumaSuppress = pAfStatParam->tAfLumaSuppress;
    pAfInitPara->tAfPeakEnhance = pAfStatParam->tAfPeakEnhance;
    pAfInitPara->tAfBrightPix   = pAfStatParam->tAfBrightPix;

    return 0;
}


AX_S32 sample_af_init_afiirref
(
    AX_U8                         pipe,
    AX_ISP_IQ_AF_IIR_REF_LIST_T *pAfRefListPara
)
{
    AX_S32 axRet;
    memcpy(pAfRefListPara, &af_iir_ref_list_param, sizeof(AX_ISP_IQ_AF_IIR_REF_LIST_T));

    axRet = AX_ISP_IQ_SetAFIirRefList(pipe, pAfRefListPara);
    if (0 != axRet)
    {
        printf("AX_ISP_GetAfAttr failed!\n");
        return -1;
    }
    return 0;
}

AX_S32 sample_af_init
(
    AX_U8                         pipe
)
{
    AX_S32 axRet;
    AX_ISP_IQ_AF_STAT_PARAM_T     tITPParams;
    AX_ISP_IQ_AF_STAT_PARAM_T     tIFEParams;
    AX_ISP_IQ_AF_IIR_REF_LIST_T   tAfRefListPara;
    DEF_AF_USED(af_itp_wbc_param_hdr);
    DEF_AF_USED(af_itp_wbc_param_sdr);

    axRet = sample_af_init_afiirref(pipe, &tAfRefListPara);
    if (0 != axRet)
    {
        printf("sample_af_init_afiirref failed!\n");
        return -1;
    }
    axRet = sample_af_init_afparams(pipe, &tITPParams, &af_itp_rltm_param);
    if (0 != axRet)
    {
        printf("sample_af_init_afparams failed!\n");
        return -1;
    }

    axRet = sample_af_init_afparams(pipe, &tIFEParams, &af_ife_param);
    if (0 != axRet)
    {
        printf("sample_af_init_afparams failed!\n");
        return -1;
    }

    /* For testing, the SetAfStatParam interface needs to be called here to enable the AFStat. The user can choose which one to use, or both. */
    axRet = AX_ISP_IQ_SetAf0StatParam(pipe, &tIFEParams);
    if (0 != axRet)
    {
        printf("AX_ISP_IQ_SetAf0StatParam failed!\n");
        return -1;
    }

    axRet = AX_ISP_IQ_SetAf1StatParam(pipe, &tITPParams);
    if (0 != axRet)
    {
        printf("AX_ISP_IQ_SetAf1StatParam failed!\n");
        return -1;
    }

    printf("sample_af_init end\n");

    return 0;
}

AX_S32 sample_af_stats
(
    AX_U8                          pipe,
    AX_ISP_AF_STAT_INFO_T         *pAfStats,
    AX_ISP_IQ_AF_STAT_PARAM_T     *pAfParams,
    AfRes                         *Res
)
{
    AX_S32 axRet = 0;
    AX_U32 regions = 0;
    AX_U64 V_v, H1_v, H2_v, V_y, H1_y, H2_y, V_p, H1_p, H2_p;
    V_v = 0;
    H1_v = 0;
    H2_v = 0;
    V_y = 0;
    H1_y = 0;
    H2_y = 0;
    V_p = 0;
    H1_p = 0;
    H2_p = 0;

    axRet = AX_ISP_IQ_GetAF1Statistics(gCams[pipe].nPipeId, pAfStats);
    if (0 != axRet)
    {
        COMM_ISP_PRT("get pipe: 0 statistics failed!\n");
        return -1;
    }

    regions = (pAfStats->tAfStatInfo.nZoneColSize * pAfStats->tAfStatInfo.nZoneRowSize);

    for (int k = 0; k < regions; k++) {
        V_p += pAfStats->tAfStatInfo.tAfRoiV[k].nPixCount;
        V_y  +=  pAfStats->tAfStatInfo.tAfRoiV[k].nPixSum;
        V_v += pAfStats->tAfStatInfo.tAfRoiV[k].nFocusValue;

        H1_p += pAfStats->tAfStatInfo.tAfRoiH1[k].nPixCount;
        H1_y  +=  pAfStats->tAfStatInfo.tAfRoiH1[k].nPixSum;
        H1_v += pAfStats->tAfStatInfo.tAfRoiH1[k].nFocusValue;

        H2_p += pAfStats->tAfStatInfo.tAfRoiH2[k].nPixCount;
        H2_y  +=  pAfStats->tAfStatInfo.tAfRoiH2[k].nPixSum;
        H2_v += pAfStats->tAfStatInfo.tAfRoiH2[k].nFocusValue;
    }

    // COMM_ISP_PRT("V_p:%llu, V_y:%llu, V_v:%llu\n", V_p, V_y, V_v);
    // COMM_ISP_PRT("H1_p:%llu, H1_y:%llu, H1_v:%llu\n", H1_p, H1_y, H1_v);
    // COMM_ISP_PRT("H2_p:%llu, H2_y:%llu, H2_v:%llu\n", H2_p, H2_y, H2_v);

    Res->V = (V_v + H1_v + H2_v)/(3*regions);
    if (0 == V_p || 0 == H1_p || 0 == H2_p) {
        Res->Y = 1;
    } else {
        Res->Y = (V_y/V_p + H1_y/H1_p + H2_y/H2_p)/3;
    }

    Res->Y>>=3;

    return axRet;
}


AX_S32 sample_af_deinit(AX_U8 pipe)
{
    return 0;
}

void *AfRun(void *args)
{
    AX_U32 i = (AX_U32)(AX_ULONG)args;
    AX_ISP_AF_STAT_INFO_T afStat = {0};
    AX_S32 axRet = 0;
    AX_ISP_IQ_AF_STAT_PARAM_T tParams;
    AfRes Res;
    AX_U32 count =0;
    COMM_ISP_PRT("3A %d is running...\n", i);

    while (!gLoopExit) {
        if (gCams[i].bOpen) {
            axRet = AX_ISP_GetIrqTimeOut(gCams[i].nPipeId, AX_IRQ_TYPE_ITP_AF_DONE, 200);
            if (axRet == 0) {

                axRet = sample_af_stats(gCams[i].nPipeId, &afStat, &tParams, &Res);
                if (0 == axRet) {
                    count ++;
                    if (count % 16 == 0)
                    {
                        COMM_PRT("AF[pipe=%d]:  Y: %llu   FV: %llu\n", gCams[i].nPipeId, Res.Y, Res.V);
                        count = 0;
                    }
                } else {
                    COMM_ISP_PRT("sample_af_stats failed\n");
                }
            }

        }

    }
    return NULL;
}