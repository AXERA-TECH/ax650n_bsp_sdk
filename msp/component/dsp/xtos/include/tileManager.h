/*
 * Copyright (c) 2019 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc.  They may be adapted and modified by bona fide
 * purchasers for internal use, but neither the original nor any adapted
 * or modified version may be disclosed or distributed to third parties
 * in any manner, medium, or form, in whole or in part, without the prior
 * written consent of Cadence Design Systems Inc.  This software and its
 * derivatives are to be executed solely on products incorporating a Cadence
 * Design Systems processor.
 */

#ifndef TILE_MANAGER_H__
#define TILE_MANAGER_H__

#if defined (__cplusplus)
extern "C"
{
#endif

//Fix for RI-1 build failure.
#if !defined(XCHAL_IDMA_MAX_OUTSTANDING_REQ)
#define XCHAL_IDMA_MAX_OUTSTANDING_REQ 64
#endif

//#define SUPPORT_3D_TILES  1
#define IDMA_USE_WIDE_API 1
//#if !defined(IDMA_USE_MULTICHANNEL)
#define IDMA_USE_MULTICHANNEL   1
//#endif

//#define SUPPORT_3D_TILES  1
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#define IDMA_USE_WIDE_ADDRESS_COMPILE  //Enable this for cores with >32bit data memory

#if !defined(__XTENSA__)
#include "dummy.h"
#endif
#include "tmUtils.h"



#if defined(__XTENSA__)
#define IDMA_APP_USE_XTOS

#include <xtensa/config/core.h>
#include <xtensa/config/core-isa.h>

#include <xtensa/idma.h>

#if (IDMA_USE_WIDE_API==0)
#if 0
#ifdef __XTENSA__
#define idma_copy_2d_desc64_wide(dmaChannel, pdst, psrc, rowSize, intrCompletionFlag, numRows, srcPitch, dstPitch) \
	idma_copy_2d_desc64(dmaChannel, pdst, psrc, rowSize, intrCompletionFlag, numRows, srcPitch, dstPitch)

#define idma_copy_2d_pred_desc64_wide(dmaChannel, pdst, psrc, rowSize, intrCompletionFlag, pred_mask, numRows, srcPitch, dstPitch) \
	idma_copy_2d_pred_desc64(dmaChannel, pdst, psrc, rowSize, intrCompletionFlag, pred_mask, numRows, srcPitch, dstPitch)

#define idma_copy_3d_desc64_wide(dmaChannel, pdst, psrc, intrCompletionFlag, rowSize, numRows, numTiles, srcPitchBytes, dstPitchBytes,   srcTilePitchBytes, dstTilePitchBytes) \
	idma_copy_3d_desc64(dmaChannel, pdst, psrc, intrCompletionFlag, rowSize, numRows, numTiles, srcPitchBytes, dstPitchBytes,   srcTilePitchBytes, dstTilePitchBytes)
#endif
#endif
#endif
#include <xtensa/tie/xt_misc.h>
#endif //__XTENSA__
#include "tileManager_api.h"




#ifndef IVP_SIMD_WIDTH
#define IVP_SIMD_WIDTH          XCHAL_IVPN_SIMD_WIDTH
#endif
#define IVP_ALIGNMENT           0x1F
#define XVTM_MIN(a, b)  (((a) < (b)) ? (a) : (b))

#define XVTM_DUMMY_DMA_INDEX  -2
#define XVTM_ERROR            -1
#define XVTM_SUCCESS          0

#define XVTM_RAISE_EXCEPTION(pxvTM)      ((pxvTM)->idmaErrorFlag = XV_ERROR_IDMA)
#define XVTM_RESET_EXCEPTION(pxvTM)      ((pxvTM)->idmaErrorFlag = XV_ERROR_SUCCESS)
#define IS_TRANSFER_SUCCESS(pxvTM)  (((pxvTM)->idmaErrorFlag == XV_ERROR_SUCCESS) ? 1 : 0)




/***********************************
*    Other Marcos
***********************************/

#define xvWaitForTileFast(pxvTM, pTile) xvWaitForTileFastMultiChannel(TM_IDMA_CH0, pxvTM, pTile)
#define xvSleepForiDMA(pxvTM, dmaIndex) xvSleepForiDMAMultiChannel(TM_IDMA_CH0, pxvTM, dmaIndex)
#define xvWaitForiDMA(pxvTM, dmaIndex) xvWaitForiDMAMultiChannel(TM_IDMA_CH0, pxvTM, dmaIndex)
#define xvWaitForTile(pxvTM, pTile) xvWaitForTileMultiChannel(TM_IDMA_CH0, pxvTM, pTile)
#define xvSleepForTile(pxvTM, pTile) xvSleepForTileMultiChannel(TM_IDMA_CH0, pxvTM, pTile)
#define xvSleepForTileFast(pxvTM, pTile) xvSleepForTileFastMultiChannel(TM_IDMA_CH0, pxvTM, pTile)
#define WAIT_FOR_TILE_MULTICHANNEL(ch, pxvTM, pTile)  xvWaitForTileMultiChannel(ch, pxvTM, pTile)
#define SLEEP_FOR_TILE_MULTICHANNEL(ch, pxvTM, pTile) xvSleepForTileMultiChannel(ch, pxvTM, pTile)
#define WAIT_FOR_DMA_MULTICHANNEL(ch, pxvTM, dmaIndex) xvWaitForiDMAMultiChannel(ch, pxvTM, dmaIndex)
#define SLEEP_FOR_DMA_MULTICHANNEL(ch, pxvTM, dmaIndex) xvSleepForiDMAMultiChannel(ch, pxvTM, dmaIndex)
#define WAIT_FOR_TILE_FAST_MULTICHANNEL(ch, pxvTM, pTile)   xvWaitForTileFastMultiChannel(ch, pxvTM, pTile)
#define SLEEP_FOR_TILE_FAST_MULTICHANNEL(ch, pxvTM, pTile)  xvSleepForTileFastMultiChannel(ch, pxvTM, pTile)
#define xvAddIdmaRequestMultiChannel_wide(dmaChannel, pxvTM, pdst64, psrc64, rowSize, \
	                                  numRows, srcPitch, dstPitch, interruptOnCompletion)  \
                                                                                          \
	xvAddIdmaRequestMultiChannel_predicated_wide(dmaChannel, pxvTM, pdst64, psrc64, rowSize, \
	                                             numRows, srcPitch, dstPitch, interruptOnCompletion, NULL)

#define XV_TILE_CHECK_VIRTUAL_FRAME(pTile)    ((pTile)->pFrame->pFrameBuff == NULL)
#define XV_FRAME_CHECK_VIRTUAL_FRAME(pFrame)  ((pFrame)->pFrameBuff == NULL)

#define SETUP_TILE(pTile, pBuf, bufSize, pFrame, width, height, pitch, type, edgeWidth, edgeHeight, x, y, alignType)          \
	{                                                                                                                           \
		int32_t tileType, bytesPerPixel, channels, bytesPerPel;                                                                   \
		uint8_t *edgePtr  = (uint8_t *) (pBuf), *dataPtr;                                                                             \
		int32_t alignment = 63;                                                                                                       \
		if (((alignType) == EDGE_ALIGNED_32) || ((alignType) == DATA_ALIGNED_32)) { (alignment) = 31; }                               \
		tileType      = type;                                                                               \
		bytesPerPixel = XV_TYPE_ELEMENT_SIZE(tileType);                                                                           \
		channels      = XV_TYPE_CHANNELS(tileType);                                                                               \
		bytesPerPel   = bytesPerPixel / channels;                                                                                 \
		XV_TILE_SET_FRAME_PTR((xvTile *) (pTile), ((xvFrame *) (pFrame)));                                                        \
		XV_TILE_SET_BUFF_PTR((xvTile *) (pTile), (pBuf));                                                                         \
		XV_TILE_SET_BUFF_SIZE((xvTile *) (pTile), (bufSize));                                                                     \
		if (((alignType) == EDGE_ALIGNED_32) || ((alignType) == EDGE_ALIGNED_64))                                                     \
		{                                                                                                                             \
			edgePtr = &((uint8_t *) (pBuf))[alignment - (((int32_t) (pBuf) + alignment) & (alignment))];                                \
		}                                                                                                                             \
		XV_TILE_SET_DATA_PTR((xvTile *) (pTile), edgePtr + (((edgeHeight) * (pitch) * bytesPerPel)) + ((edgeWidth) * bytesPerPixel)); \
		if (((alignType) == DATA_ALIGNED_32) || ((alignType) == DATA_ALIGNED_64))                                                     \
		{                                                                                                                             \
			dataPtr = (uint8_t *) XV_TILE_GET_DATA_PTR((xvTile *) (pTile));                                                             \
			dataPtr = (uint8_t *) (((long) (dataPtr) + alignment) & (~alignment));                                                  \
			XV_TILE_SET_DATA_PTR((xvTile *) (pTile), dataPtr);                                                                          \
		}                                                                                                                             \
		XV_TILE_SET_WIDTH((xvTile *) (pTile), (width));                                                                           \
		XV_TILE_SET_HEIGHT((xvTile *) (pTile), (height));                                                                         \
		XV_TILE_SET_PITCH((xvTile *) (pTile), (pitch));                                                                           \
		XV_TILE_SET_TYPE((xvTile *) (pTile), (tileType | XV_TYPE_TILE_BIT));                                                      \
		XV_TILE_SET_EDGE_WIDTH((xvTile *) (pTile), (edgeWidth));                                                                  \
		XV_TILE_SET_EDGE_HEIGHT((xvTile *) (pTile), (edgeHeight));                                                                \
		XV_TILE_SET_X_COORD((xvTile *) (pTile), (x));                                                                             \
		XV_TILE_SET_Y_COORD((xvTile *) (pTile), (y));                                                                             \
		XV_TILE_SET_STATUS_FLAGS((xvTile *) (pTile), 0);                                                                          \
		XV_TILE_RESET_DMA_INDEX((xvTile *) (pTile));                                                                              \
		XV_TILE_RESET_PREVIOUS_TILE((xvTile *) (pTile));                                                                          \
		XV_TILE_RESET_REUSE_COUNT((xvTile *) (pTile));                                                                            \
	}

#if defined(SUPPORT_3D_TILES)
#define SETUP_TILE_3D(pTile3D, pBuf, bufSize, pFrame3D, width, height, depth, pitch, pitch2D, type, edgeWidth, edgeHeight, edgeDepth, x, y, z, alignType)   \
	{                                                                                                                                 \
		int32_t tileType, bytesPerPixel, channels, bytesPerPel;                                                                       \
		uint8_t *edgePtr  = (uint8_t *) (pBuf), *dataPtr;                                                                             \
		int32_t alignment = 63;                                                                                                       \
		if (((alignType) == EDGE_ALIGNED_32) || ((alignType) == DATA_ALIGNED_32)) { (alignment) = 31; }                               \
		tileType      = type;                                                                                                         \
		bytesPerPixel = XV_TYPE_ELEMENT_SIZE(tileType);                                                                               \
		channels      = XV_TYPE_CHANNELS(tileType);                                                                                   \
		bytesPerPel   = bytesPerPixel / channels;                                                                                     \
		XV_TILE_3D_SET_FRAME_3D_PTR(pTile3D, pFrame3D);                                                                               \
		XV_TILE_SET_BUFF_PTR((pTile3D), (pBuf));                                                                                      \
		XV_TILE_SET_BUFF_SIZE((pTile3D), (bufSize));                                                                                  \
		if (((alignType) == EDGE_ALIGNED_32) || ((alignType) == EDGE_ALIGNED_64))                                                     \
		{                                                                                                                             \
			edgePtr = &((uint8_t *) (pBuf))[alignment - (((int32_t) (pBuf) + alignment) & (alignment))];                              \
		}                                                                                                                             \
		XV_TILE_SET_DATA_PTR((pTile3D), edgePtr + (((edgeHeight) * (pitch) * bytesPerPel)) + ((edgeWidth) * bytesPerPixel) +          \
		                     ((edgeDepth)*(pitch2D)*bytesPerPel));                                                                    \
		if (((alignType) == DATA_ALIGNED_32) || ((alignType) == DATA_ALIGNED_64))                                                     \
		{                                                                                                                             \
			dataPtr = (uint8_t *) XV_TILE_GET_DATA_PTR(pTile3D);                                                                      \
			dataPtr = (uint8_t *) (((long) (dataPtr) + alignment) & (~alignment));                                                    \
			XV_TILE_SET_DATA_PTR(pTile3D, dataPtr);                                                                                   \
		}                                                                                                                             \
		XV_TILE_SET_WIDTH((pTile3D), (width));                                                                                        \
		XV_TILE_SET_HEIGHT((pTile3D), (height));                                                                                      \
		XV_TILE_SET_PITCH((pTile3D), (pitch));                                                                                        \
		XV_TILE_SET_TYPE((pTile3D), (tileType | XV_TYPE_TILE_BIT));                                                                   \
		XV_TILE_SET_EDGE_WIDTH((pTile3D), (edgeWidth));                                                                               \
		XV_TILE_SET_EDGE_HEIGHT((pTile3D), (edgeHeight));                                                                             \
		XV_TILE_SET_X_COORD((pTile3D), (x));                                                                                          \
		XV_TILE_SET_Y_COORD((pTile3D), (y));                                                                                          \
		XV_TILE_SET_STATUS_FLAGS((pTile3D), 0);                                                                                       \
		XV_TILE_RESET_DMA_INDEX((pTile3D));                                                                                           \
		XV_TILE_SET_Z_COORD(pTile3D, z);                                                                                              \
		XV_TILE_SET_TILE_PITCH(pTile3D, pitch2D);                                                                                     \
		XV_TILE_SET_EDGE_DEPTH(pTile3D, edgeDepth);                                                                                   \
		XV_TILE_SET_DEPTH(pTile3D, depth);                                                                                            \
		pTile3D->pTemp = NULL;                                                                                                        \
	}
#endif


#define SETUP_FRAME(pFrame, pFrameBuffer, buffSize, width, height, pitch, padWidth, padHeight, pixRes, numCh, paddingType, paddingVal)     \
	{                                                                                                                                      \
		XV_FRAME_SET_BUFF_PTR((xvFrame *) (pFrame), (pFrameBuffer));                                                                       \
		XV_FRAME_SET_BUFF_SIZE((xvFrame *) (pFrame), (buffSize));                                                                          \
		XV_FRAME_SET_WIDTH((xvFrame *) (pFrame), (width));                                                                                 \
		XV_FRAME_SET_HEIGHT((xvFrame *) (pFrame), (height));                                                                               \
		XV_FRAME_SET_PITCH((xvFrame *) (pFrame), (pitch));                                                                                 \
		XV_FRAME_SET_PIXEL_RES((xvFrame *) (pFrame), (pixRes));                                                                            \
		XV_FRAME_SET_DATA_PTR((xvFrame *) (pFrame), (uint64_t)(pFrameBuffer) + (uint64_t)((((pitch) * (padHeight)) + ((padWidth) * (numCh))) * (pixRes))); \
		XV_FRAME_SET_EDGE_WIDTH((xvFrame *) (pFrame), (padWidth));                                                                         \
		XV_FRAME_SET_EDGE_HEIGHT((xvFrame *) (pFrame), (padHeight));                                                                       \
		XV_FRAME_SET_NUM_CHANNELS((xvFrame *) (pFrame), (numCh));                                                                          \
		XV_FRAME_SET_PADDING_TYPE((xvFrame *) (pFrame), (paddingType));                                                                    \
		XV_FRAME_SET_PADDING_VALUE((xvFrame *) (pFrame), (paddingVal));                                                                    \
	}


#if defined(SUPPORT_3D_TILES)



#define SETUP_FRAME_3D(pFrame3D, pFrameBuffer, buffSize, width, height, depth, pitch, pitchFrame2D, padWidth, padHeight, padDepth, pixRes, numCh, paddingType, paddingVal)     \
	{                                                                                                                                \
		XV_FRAME_SET_BUFF_PTR((pFrame3D), (pFrameBuffer));                                                                           \
		XV_FRAME_SET_BUFF_SIZE((pFrame3D), (buffSize));                                                                              \
		XV_FRAME_SET_WIDTH((pFrame3D), (width));                                                                                     \
		XV_FRAME_SET_HEIGHT((pFrame3D), (height));                                                                                   \
		XV_FRAME_SET_PITCH((pFrame3D), (pitch));                                                                                     \
		XV_FRAME_SET_PIXEL_RES((pFrame3D), (pixRes));                                                                                \
		XV_FRAME_SET_DATA_PTR((pFrame3D), ((uint64_t)(pFrameBuffer)) + (   (((pitch) * (padHeight)) +                                \
		    ((padWidth) * (numCh))  ) * (pixRes))     + ((pitchFrame2D) * (padDepth)) *(pixRes));                                    \
		XV_FRAME_SET_EDGE_WIDTH((pFrame3D), (padWidth));                                                                             \
		XV_FRAME_SET_EDGE_HEIGHT((pFrame3D), (padHeight));                                                                           \
		XV_FRAME_SET_NUM_CHANNELS((pFrame3D), (numCh));                                                                              \
		XV_FRAME_SET_PADDING_TYPE((pFrame3D), (paddingType));                                                                        \
		XV_FRAME_SET_PADDING_VALUE((pFrame3D), (paddingVal));                                                                        \
		XV_FRAME_SET_EDGE_DEPTH(pFrame3D, padDepth);                                                                                 \
		XV_FRAME_SET_DEPTH(pFrame3D, depth);                                                                                         \
		XV_FRAME_SET_FRAME_PITCH(pFrame3D, pitchFrame2D);                                                                            \
	}
#endif


#define WAIT_FOR_TILE(pxvTM, pTile)      WAIT_FOR_TILE_MULTICHANNEL(TM_IDMA_CH0, pxvTM, pTile)
#define SLEEP_FOR_TILE(pxvTM, pTile)    SLEEP_FOR_TILE_MULTICHANNEL(TM_IDMA_CH0, pxvTM, pTile)
#define WAIT_FOR_DMA(pxvTM, dmaIndex)   WAIT_FOR_DMA_MULTICHANNEL(TM_IDMA_CH0, pxvTM, dmaIndex)
#define SLEEP_FOR_DMA(pxvTM, dmaIndex)  SLEEP_FOR_DMA_MULTICHANNEL(TM_IDMA_CH0, pxvTM, dmaIndex)
#define WAIT_FOR_TILE_FAST(pxvTM, pTile)  WAIT_FOR_TILE_FAST_MULTICHANNEL(TM_IDMA_CH0, pxvTM, pTile)
#define SLEEP_FOR_TILE_FAST(pxvTM, pTile)  SLEEP_FOR_TILE_FAST_MULTICHANNEL(TM_IDMA_CH0, pxvTM, pTile)

#if defined(SUPPORT_3D_TILES)
#define WAIT_FOR_TILE_3D(pxvTM, pTile3D)   WAIT_FOR_TILE_MULTICHANNEL_3D(TM_IDMA_CH0, pxvTM, pTile3D)
#define WAIT_FOR_TILE_MULTICHANNEL_3D(ch, pxvTM, pTile3D)  xvWaitForTileMultiChannel3D(ch, pxvTM, pTile3D)
#endif


// Assumes both top and bottom edges are equal
#define XV_TILE_UPDATE_EDGE_HEIGHT(pTile, newEdgeHeight)                            \
	{                                                                                 \
		uint32_t tileType       = XV_TILE_GET_TYPE(pTile);                              \
		uint32_t bytesPerPixel  = XV_TYPE_ELEMENT_SIZE(tileType);                       \
		uint32_t channels       = XV_TYPE_CHANNELS(tileType);                           \
		uint32_t bytesPerPel    = bytesPerPixel / channels;                             \
		uint16_t currEdgeHeight = (uint16_t) XV_TILE_GET_EDGE_HEIGHT(pTile);            \
		uint32_t tilePitch      = (uint32_t) XV_TILE_GET_PITCH(pTile);                  \
		uint32_t tileHeight     = (uint32_t) XV_TILE_GET_HEIGHT(pTile);                 \
		uint32_t dataU32        = (uint32_t) XV_TILE_GET_DATA_PTR(pTile);               \
		dataU32 = dataU32 + tilePitch * bytesPerPel * ((newEdgeHeight) - currEdgeHeight); \
		XV_TILE_SET_DATA_PTR((pTile), (void *) dataU32);                                  \
		XV_TILE_SET_EDGE_HEIGHT((pTile), (newEdgeHeight));                                \
		XV_TILE_SET_HEIGHT((pTile), tileHeight + 2 * (currEdgeHeight - (newEdgeHeight))); \
	}



// Assumes both left and right edges are equal
#define XV_TILE_UPDATE_EDGE_WIDTH(pTile, newEdgeWidth)                        \
	{                                                                           \
		uint32_t tileType      = (pTile)->type;                                       \
		uint32_t bytesPerPixel = XV_TYPE_ELEMENT_SIZE(tileType);                  \
		uint16_t currEdgeWidth = (uint16_t) XV_TILE_GET_EDGE_WIDTH(pTile);        \
		uint32_t tileWidth     = (uint32_t) XV_TILE_GET_WIDTH(pTile);             \
		uint32_t dataU32       = (uint32_t) XV_TILE_GET_DATA_PTR(pTile);          \
		dataU32 = dataU32 + ((newEdgeWidth) - currEdgeWidth) * bytesPerPixel;         \
		XV_TILE_SET_DATA_PTR((pTile), (void *) dataU32);                              \
		XV_TILE_SET_EDGE_WIDTH((pTile), (newEdgeWidth));                              \
		XV_TILE_SET_WIDTH((pTile), tileWidth + 2 * (currEdgeWidth - (newEdgeWidth))); \
	}

#define XV_TILE_UPDATE_DIMENSIONS(pTile, x, y, w, h, p) \
	{                                                     \
		XV_TILE_SET_X_COORD((pTile), (x));                  \
		XV_TILE_SET_Y_COORD((pTile), (y));                  \
		XV_TILE_SET_WIDTH((pTile), (w));                    \
		XV_TILE_SET_HEIGHT((pTile), (h));                   \
		XV_TILE_SET_PITCH((pTile), (p));                    \
	}

#define XV_TILE_GET_CHANNEL(t)  XV_TYPE_CHANNELS(XV_TILE_GET_TYPE(t))
#define XV_TILE_PEL_SIZE(t)     (XV_TILE_GET_ELEMENT_SIZE(t) / XV_TILE_GET_CHANNEL(t))

#  define XV_CHECK_ERROR(condition, statment, code, wide_description) \
	if (condition) {} \
	else {  \
	    AX_VDSP_LOG_ERROR("Error: %s\n", wide_description); \
	    (statment); return (code); \
	}


#if XCHAL_NUM_DATARAM == 2
#define XV_ARRAY_STARTS_IN_DRAM(t) \
	((((unsigned int) XV_ARRAY_GET_BUFF_PTR(t)) >= (((unsigned int) XCHAL_DATARAM0_VADDR < (unsigned int) XCHAL_DATARAM1_VADDR) ? (unsigned int) XCHAL_DATARAM0_VADDR : (unsigned int) XCHAL_DATARAM1_VADDR)))
#define XV_ARRAY_ENDS_IN_DRAM(t) \
	(((((unsigned int) XV_ARRAY_GET_BUFF_PTR(t)) + XV_ARRAY_GET_BUFF_SIZE(t)) <= ((((unsigned int) XCHAL_DATARAM0_VADDR + (unsigned int) XCHAL_DATARAM0_SIZE) > ((unsigned int) XCHAL_DATARAM1_VADDR + (unsigned int) XCHAL_DATARAM1_SIZE)) ? ((unsigned int) XCHAL_DATARAM0_VADDR + (unsigned int) XCHAL_DATARAM0_SIZE) : ((unsigned int) XCHAL_DATARAM1_VADDR + (unsigned int) XCHAL_DATARAM1_SIZE))))
#define XV_TILE_STARTS_IN_DRAM(t) \
	((((unsigned int) XV_TILE_GET_BUFF_PTR(t)) >= (((unsigned int) XCHAL_DATARAM0_VADDR < (unsigned int) XCHAL_DATARAM1_VADDR) ? (unsigned int) XCHAL_DATARAM0_VADDR : (unsigned int) XCHAL_DATARAM1_VADDR)))
#define XV_TILE_ENDS_IN_DRAM(t) \
	(((((unsigned int) XV_TILE_GET_BUFF_PTR(t)) + XV_TILE_GET_BUFF_SIZE(t)) <= ((((unsigned int) XCHAL_DATARAM0_VADDR + (unsigned int) XCHAL_DATARAM0_SIZE) > ((unsigned int) XCHAL_DATARAM1_VADDR + (unsigned int) XCHAL_DATARAM1_SIZE)) ? ((unsigned int) XCHAL_DATARAM0_VADDR + (unsigned int) XCHAL_DATARAM0_SIZE) : ((unsigned int) XCHAL_DATARAM1_VADDR + (unsigned int) XCHAL_DATARAM1_SIZE))))

#define XV_PTR_START_IN_DRAM(t) \
	((((unsigned int) ((void *) (t))) >= (((unsigned int) XCHAL_DATARAM0_VADDR < (unsigned int) XCHAL_DATARAM1_VADDR) ? (unsigned int) XCHAL_DATARAM0_VADDR : (unsigned int) XCHAL_DATARAM1_VADDR)))

#define XV_PTR_END_IN_DRAM(t) \
	(((unsigned int) ((void *) (t))) <= ((((unsigned int) XCHAL_DATARAM0_VADDR + (unsigned int) XCHAL_DATARAM0_SIZE) > ((unsigned int) XCHAL_DATARAM1_VADDR + (unsigned int) XCHAL_DATARAM1_SIZE)) ? ((unsigned int) XCHAL_DATARAM0_VADDR + (unsigned int) XCHAL_DATARAM0_SIZE) : ((unsigned int) XCHAL_DATARAM1_VADDR + (unsigned int) XCHAL_DATARAM1_SIZE)))

#elif XCHAL_NUM_DATARAM == 1
#define XV_ARRAY_STARTS_IN_DRAM(t) \
	(((unsigned int) XV_ARRAY_GET_BUFF_PTR(t)) >= (unsigned int) XCHAL_DATARAM0_VADDR)
#define XV_ARRAY_ENDS_IN_DRAM(t) \
	((((unsigned int) XV_ARRAY_GET_BUFF_PTR(t)) + XV_ARRAY_GET_BUFF_SIZE(t)) <= ((unsigned int) XCHAL_DATARAM0_VADDR + (unsigned int) XCHAL_DATARAM0_SIZE))
#define XV_TILE_STARTS_IN_DRAM(t) \
	(((unsigned int) XV_TILE_GET_BUFF_PTR(t)) >= (unsigned int) XCHAL_DATARAM0_VADDR)
#define XV_TILE_ENDS_IN_DRAM(t) \
	((((unsigned int) XV_TILE_GET_BUFF_PTR(t)) + XV_TILE_GET_BUFF_SIZE(t)) <= ((unsigned int) XCHAL_DATARAM0_VADDR + (unsigned int) XCHAL_DATARAM0_SIZE))

#define XV_PTR_START_IN_DRAM(t) \
	(((unsigned int) ((void *) (t))) >= (unsigned int) XCHAL_DATARAM0_VADDR)

#define XV_PTR_END_IN_DRAM(t) \
	(((unsigned int) ((void *) (t))) <= ((unsigned int) XCHAL_DATARAM0_VADDR + (unsigned int) XCHAL_DATARAM0_SIZE))
#endif

#define XV_TILE_IS_CONSISTENT(t)                                                                                                                                                                             \
	(((XV_TILE_GET_PITCH(t)) >= ((XV_TILE_GET_WIDTH(t) + XV_TILE_GET_EDGE_LEFT(t) + XV_TILE_GET_EDGE_RIGHT(t)) * XV_TILE_GET_CHANNEL(t))) &&                                                                   \
	 (((uint8_t *) XV_TILE_GET_DATA_PTR(t) - ((XV_TILE_GET_EDGE_LEFT(t) * XV_TILE_GET_ELEMENT_SIZE(t)) + (XV_TILE_GET_PITCH(t) * XV_TILE_GET_EDGE_TOP(t) * XV_TILE_PEL_SIZE(t))))                              \
	  >= ((uint8_t *) XV_TILE_GET_BUFF_PTR(t))) &&                                                                                                                                                             \
	 ((((uint8_t *) XV_TILE_GET_DATA_PTR(t) - (XV_TILE_GET_EDGE_LEFT(t) * XV_TILE_GET_ELEMENT_SIZE(t))) + (XV_TILE_GET_PITCH(t) * (XV_TILE_GET_HEIGHT(t) + XV_TILE_GET_EDGE_BOTTOM(t)) * XV_TILE_PEL_SIZE(t))) \
	  <= ((uint8_t *) XV_TILE_GET_BUFF_PTR(t) + XV_TILE_GET_BUFF_SIZE(t))))

#define XV_TILE_IS_CONSISTENT_3D(t)                                                                                                                                                                          \
	(((XV_TILE_GET_PITCH(t)) >= ((XV_TILE_GET_WIDTH(t) + XV_TILE_GET_EDGE_LEFT(t) + XV_TILE_GET_EDGE_RIGHT(t)) * XV_TILE_GET_CHANNEL(t))) &&                                                                   \
	 ((XV_TILE_GET_TILE_PITCH(t)) >= ((XV_TILE_GET_HEIGHT(t) + XV_TILE_GET_EDGE_TOP(t) + XV_TILE_GET_EDGE_BOTTOM(t)) * XV_TILE_GET_PITCH(t))) &&                                                                \
	 (((uint32_t) XV_TILE_GET_DATA_PTR(t) - ((XV_TILE_GET_EDGE_LEFT(t) * XV_TILE_GET_ELEMENT_SIZE(t)) + (XV_TILE_GET_PITCH(t) * XV_TILE_GET_EDGE_TOP(t) * XV_TILE_PEL_SIZE(t)) +                              \
	                                         (XV_TILE_GET_TILE_PITCH(t) * XV_TILE_GET_EDGE_FRONT(t) * XV_TILE_PEL_SIZE(t)))) >= ((uint32_t) XV_TILE_GET_BUFF_PTR(t))) &&                                                                              \
	 ((((uint32_t) XV_TILE_GET_DATA_PTR(t) - (XV_TILE_GET_EDGE_LEFT(t) * XV_TILE_GET_ELEMENT_SIZE(t)) - XV_TILE_GET_PITCH(t)*XV_TILE_GET_EDGE_TOP(t)*XV_TILE_PEL_SIZE(t))  +                                  \
	   (XV_TILE_GET_TILE_PITCH(t) * (XV_TILE_GET_DEPTH(t) + XV_TILE_GET_EDGE_BACK(t)) * XV_TILE_PEL_SIZE(t)))  <= ((uint32_t) XV_TILE_GET_BUFF_PTR(t) + XV_TILE_GET_BUFF_SIZE(t))))

#define XV_CHECK_POINTER(pointer, statment) \
	XV_CHECK_ERROR((pointer) != (void *) NULL, (statment), XVTM_ERROR, " NULL pointer error")

#define XV_CHECK_TILE(tile, TileMgr)                                                                                                     \
	XV_CHECK_POINTER((tile), (TileMgr)->errFlag = XV_ERROR_BAD_ARG);                                                                     \
	XV_CHECK_ERROR((XV_TILE_IS_TILE(tile) > 0), (TileMgr)->errFlag = XV_ERROR_BAD_ARG, XVTM_ERROR, "The argument is not a tile");          \
	XV_CHECK_ERROR(XV_TILE_STARTS_IN_DRAM(tile), (TileMgr)->errFlag = XV_ERROR_BAD_ARG, XVTM_ERROR, "Data buffer does not start in DRAM"); \
	XV_CHECK_ERROR(XV_TILE_ENDS_IN_DRAM(tile), (TileMgr)->errFlag = XV_ERROR_BAD_ARG, XVTM_ERROR, "Data buffer does not fit in DRAM");     \
	XV_CHECK_ERROR(XV_TILE_IS_CONSISTENT(tile), (TileMgr)->errFlag = XV_ERROR_BAD_ARG, XVTM_ERROR, "Invalid buffer")


#if defined(SUPPORT_3D_TILES)
#define XV_CHECK_TILE_3D(tile3D, TileMgr)    \
	XV_CHECK_POINTER((tile3D), (TileMgr)->errFlag = XV_ERROR_BAD_ARG);                                                                     \
	XV_CHECK_ERROR((XV_TILE_IS_TILE(tile3D) > 0), (TileMgr)->errFlag = XV_ERROR_BAD_ARG, XVTM_ERROR, "The argument is not a tile");          \
	XV_CHECK_ERROR(XV_TILE_STARTS_IN_DRAM(tile3D), (TileMgr)->errFlag = XV_ERROR_BAD_ARG, XVTM_ERROR, "Data buffer does not start in DRAM"); \
	XV_CHECK_ERROR(XV_TILE_ENDS_IN_DRAM(tile3D), (TileMgr)->errFlag = XV_ERROR_BAD_ARG, XVTM_ERROR, "Data buffer does not fit in DRAM");     \
	XV_CHECK_ERROR(XV_TILE_IS_CONSISTENT_3D(tile3D), (TileMgr)->errFlag = XV_ERROR_BAD_ARG, XVTM_ERROR, "Invalid buffer")
#endif


#define XV_IS_TILE_OK(t)                                                                                                                                                                               \
	((t) &&                                                                                                                                                                                              \
	 (XV_TILE_GET_DATA_PTR(t)) &&                                                                                                                                                                        \
	 (XV_TILE_GET_BUFF_PTR(t)) &&                                                                                                                                                                        \
	 ((XV_TILE_GET_PITCH(t)) >= ((XV_TILE_GET_WIDTH(t) + XV_TILE_GET_EDGE_LEFT(t) + XV_TILE_GET_EDGE_RIGHT(t)) * XV_TILE_GET_CHANNEL(t))) &&                                                             \
	 (((uint8_t *) XV_TILE_GET_DATA_PTR(t) - (XV_TILE_GET_EDGE_LEFT(t) * XV_TILE_GET_ELEMENT_SIZE(t) + XV_TILE_GET_PITCH(t) * XV_TILE_GET_EDGE_TOP(t) * XV_TILE_PEL_SIZE(t)))                            \
	  >= ((uint8_t *) XV_TILE_GET_BUFF_PTR(t))) &&                                                                                                                                                       \
	 (((uint8_t *) XV_TILE_GET_DATA_PTR(t) - XV_TILE_GET_EDGE_LEFT(t) * XV_TILE_GET_ELEMENT_SIZE(t) + XV_TILE_GET_PITCH(t) * (XV_TILE_GET_HEIGHT(t) + XV_TILE_GET_EDGE_BOTTOM(t)) * XV_TILE_PEL_SIZE(t)) \
	  <= ((uint8_t *) XV_TILE_GET_BUFF_PTR(t) + XV_TILE_GET_BUFF_SIZE(t))))



#if defined (__cplusplus)
}
#endif
#endif


