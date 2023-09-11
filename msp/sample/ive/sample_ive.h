/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __SAMPLE_IVE_H__
#define __SAMPLE_IVE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <float.h>

#include "sample_ive_common.h"

AX_VOID SAMPLE_IVE_DMA_TEST(AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList);

AX_VOID SAMPLE_IVE_DMA_TEST_HandleSig(AX_VOID);

AX_VOID SAMPLE_IVE_DualPicCalc_TEST(AX_U32 u32Mode, AX_S32 as32Type[], AX_CHAR **pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList);

AX_VOID SAMPLE_IVE_DualPicCalc_TEST_HandleSig(AX_VOID);

AX_VOID SAMPLE_IVE_EdgeDetection_TEST(AX_U32 u32Mode, AX_S32 as32Type[], AX_CHAR **pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList);

AX_VOID SAMPLE_IVE_EdgeDetection_TEST_HandleSig(AX_VOID);

AX_VOID SAMPLE_IVE_CCL_TEST(AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR **pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList);

AX_VOID SAMPLE_IVE_CCL_TEST_HandleSig(AX_VOID);

AX_VOID SAMPLE_IVE_ED_TEST(AX_U32 u32Mode, AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList);

AX_VOID SAMPLE_IVE_ED_TEST_HandleSig(AX_VOID);

AX_VOID SAMPLE_IVE_Filter_TEST(AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList);

AX_VOID SAMPLE_IVE_Filter_TEST_HandleSig(AX_VOID);

AX_VOID SAMPLE_IVE_Hist_TEST(AX_U32 u32Mode, AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList);

AX_VOID SAMPLE_IVE_Hist_TEST_HandleSig(AX_VOID);

AX_VOID SAMPLE_IVE_Integ_TEST(AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList);

AX_VOID SAMPLE_IVE_Integ_TEST_HandleSig(AX_VOID);

AX_VOID SAMPLE_IVE_MagAng_TEST(AX_S32 as32Type[], AX_CHAR **pchSrcPath, AX_CHAR **pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc);

AX_VOID SAMPLE_IVE_MagAng_TEST_HandleSig(AX_VOID);

AX_VOID SAMPLE_IVE_Sobel_TEST(AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList);

AX_VOID SAMPLE_IVE_Sobel_TEST_HandleSig(AX_VOID);

AX_VOID SAMPLE_IVE_GMM_TEST(AX_U32 u32Mode, AX_S32 as32Type[], AX_CHAR **pchSrcPath, AX_CHAR **pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList);

AX_VOID SAMPLE_IVE_GMM_TEST_HandleSig(AX_VOID);

AX_VOID SAMPLE_IVE_Thresh_TEST(AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList);

AX_VOID SAMPLE_IVE_Thresh_TEST_HandleSig(AX_VOID);

AX_VOID SAMPLE_IVE_16To8Bit_TEST(AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList);

AX_VOID SAMPLE_IVE_16To8Bit_TEST_HandleSig(AX_VOID);

AX_VOID SAMPLE_IVE_CropResize_TEST(AX_U32 u32Engine, AX_U32 u32Mode, AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList);

AX_VOID SAMPLE_IVE_CropResize_TEST_HandleSig(AX_VOID);

AX_VOID SAMPLE_IVE_CSC_TEST(AX_U32 u32Engine, AX_S32 as32Type[], AX_CHAR *pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc);

AX_VOID SAMPLE_IVE_CSC_TEST_HandleSig(AX_VOID);

AX_VOID SAMPLE_IVE_CropResize2_TEST(AX_U32 u32Engine, AX_U32 u32Mode, AX_S32 as32Type[], AX_CHAR **pchSrcPath, AX_CHAR *pchDstPath, AX_U32 u32WidthSrc, AX_U32 u32HeightSrc, AX_CHAR *pchParamsList);

AX_VOID SAMPLE_IVE_CropResize2_TEST_HandleSig(AX_VOID);

AX_VOID SAMPLE_IVE_MatMul_TEST(AX_CHAR *pchParamsList);

AX_VOID SAMPLE_IVE_MatMul_TEST_HandleSig(AX_BOOL bInitEngine);

#endif