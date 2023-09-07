/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_VDSP_DEF_H__
#define __AX_VDSP_DEF_H__

#include <xtensa/tie/xt_ivpn.h>
#include <xtensa/xt_profiling.h>
#include <xtensa/tie/xt_timer.h>

#define IVP

#if defined(__XTENSA__) || defined(GCC)
#define ALIGN(x)  __attribute__((aligned(x)))
#ifndef ALIGN16
#define ALIGN16  __attribute__((aligned(16)))
#endif
#define ALIGN32  __attribute__((aligned(32)))
#define ALIGN64  __attribute__((aligned(64)))
#else
#define ALIGN(x)  _declspec(align(x))
#ifndef ALIGN16
#define ALIGN16  _declspec(align(16))
#endif
#define ALIGN32  _declspec(align(32))
#define ALIGN64  _declspec(align(64))
#define __restrict
#endif

#ifdef IVP
#define _LOCAL_DRAM0_  __attribute__((section(".dram0.data")))
#define _LOCAL_DRAM1_  __attribute__((section(".dram1.data")))
#define _TILEMGR_IRAM0_

#else
#define _LOCAL_DRAM0_
#define _LOCAL_DRAM1_
#define _TILEMGR_IRAM0_
#endif

#ifdef IVP
#define IVP_SIMD_WIDTH  XCHAL_IVPN_SIMD_WIDTH
#else
#define IVP_SIMD_WIDTH  32
#endif

#define IVP_SIMD_WIDTH_LOG2  5

// AX_VDSP_TIME_STAMP macro is used to measure the cycles

#define AX_VDSP_POOL0_SIZE                (240 * 1024)
#define AX_VDSP_POOL1_SIZE                (220 * 1024)
#define AX_VDSP_DMA_DESCR_CNT            (16) // number of DMA decsriptors
#define AX_VDSP_MAX_PIF                  (32)
#define AX_VDSP_INT_ON_COMPLETION  (1)
#define AX_VDSP_HAVE_HISTOGRAM 	1//XCHAL_HAVE_GRIVPEP_HISTOGRAM

#define AX_VDSP_CLIP(a, maxv, minv)		 (((a)>(maxv)) ? (maxv) : (((a) < (minv)) ? (minv) : (a)))
#define AX_VDSP_TIME_STAMP(cycCnt)    	 (cycCnt) = XT_RSR_CCOUNT()

/***********************************
 * Common OFFSET_PTR macro
 **********************************/
#define AX_VDSP_OFFSET_PTR_2NX8U(  ptr, nrows, stride, in_row_offset) ((xb_vec2Nx8U*)   ((uint8_t*) (ptr)+(in_row_offset)+(nrows)*(stride)))
#define AX_VDSP_OFFSET_PTR_N_2X32U(ptr, nrows, stride, in_row_offset) ((xb_vecN_2x32Uv*)((uint32_t*)(ptr)+(in_row_offset)+(nrows)*(stride)))
#define AX_VDSP_OFFSET_PTR_N_2X32( ptr, nrows, stride, in_row_offset) ((xb_vecN_2x32v*) ((int32_t*) (ptr)+(in_row_offset)+(nrows)*(stride)))
#define AX_VDSP_OFFSET_PTR_NX16U(  ptr, nrows, stride, in_row_offset) ((xb_vecNx16U*)   ((uint16_t*)(ptr)+(in_row_offset)+(nrows)*(stride)))
#define AX_VDSP_OFFSET_PTR_NX16(   ptr, nrows, stride, in_row_offset) ((xb_vecNx16*)    ((int16_t*) (ptr)+(in_row_offset)+(nrows)*(stride)))
#define AX_VDSP_OFFSET_PTR_N_2X16( ptr, nrows, stride, in_row_offset) ((xb_vecN_2x16*)  ((int16_t*) (ptr)+(in_row_offset)+(nrows)*(stride)))
#define AX_VDSP_OFFSET_PTR_NX8U(   ptr, nrows, stride, in_row_offset) ((xb_vecNx8U*)    ((uint8_t*) (ptr)+(in_row_offset)+(nrows)*(stride)))
#define AX_VDSP_OFFSET_PTR_NX8(    ptr, nrows, stride, in_row_offset) ((xb_vecNx8*)     ((int8_t*)  (ptr)+(in_row_offset)+(nrows)*(stride)))
#define AX_VDSP_OFFSET_PTR_2NX8(   ptr, nrows, stride, in_row_offset) ((xb_vec2Nx8*)    ((int8_t*)  (ptr)+(in_row_offset)+(nrows)*(stride)))


#endif  //__AX_VDSP_DEF_H__

