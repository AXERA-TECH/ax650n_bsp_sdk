/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _SAMPLE_IVES_UTIL_H_
#define _SAMPLE_IVES_UTIL_H_

#include "ax_ives_api.h"

AX_U32  SAMPLE_CALC_IMAGE_SIZE(AX_U32 u32Width, AX_U32 u32Height, AX_IMG_FORMAT_E eImgType, AX_U32 u32Stride);
AX_IVES_IMAGE_T *SAMPLE_LOAD_IMAGE(const AX_CHAR *pImgFile, AX_U32 u32Width, AX_U32 u32Height, AX_IMG_FORMAT_E eImgType);
AX_VOID SAMPLE_FREE_IMAGE(AX_IVES_IMAGE_T *pstImg);

AX_U64 SAMPLE_GET_TICK_COUNT(AX_VOID);


#endif /* _SAMPLE_IVES_UTIL_H_ */