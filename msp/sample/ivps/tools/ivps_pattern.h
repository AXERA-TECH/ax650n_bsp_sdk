/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _IVPS_PATTERN_
#define _IVPS_PATTERN_

/*-------------------------------------------------------------------------*/
/**
  @brief    Create Smpte pattern
  @param    enPixFmt format of the pattern
  @param    u32Width width of the pattern
  @param    u32Height height of the pattern
  @param    u32Stride stride of the pattern
  @param    u8Mem point to the buffer filled with pattern data
  @return   int 0 if Ok, anything else otherwise
 */
/*--------------------------------------------------------------------------*/
AX_S32 PatternSmpteFill(AX_IMG_FORMAT_E enPixFmt, AX_U32 u32Width, AX_U32 u32Height, AX_U32 u32Stride, AX_U8 *u8Mem);

#endif /* _IVPS_PATTERN_ */
