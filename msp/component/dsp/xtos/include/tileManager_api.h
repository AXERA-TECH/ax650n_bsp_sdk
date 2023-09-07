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

#ifndef TILE_MANAGER_API_H__
#define TILE_MANAGER_API_H__

#if defined (__cplusplus)
extern "C"
{
#endif


#define DEFAULT_IDMA_CHANNEL  IDMA_CHANNEL_0
#define TM_IDMA_CH0           IDMA_CHANNEL_0
#if defined XCHAL_IDMA_NUM_CHANNELS && (XCHAL_IDMA_NUM_CHANNELS >= 2)
#define TM_IDMA_CH1           IDMA_CHANNEL_1
#else
#define TM_IDMA_CH1           IDMA_CHANNEL_0
#endif

#if defined XCHAL_IDMA_NUM_CHANNELS && (XCHAL_IDMA_NUM_CHANNELS == 4)
#define TM_IDMA_CH2           IDMA_CHANNEL_2
#define TM_IDMA_CH3           IDMA_CHANNEL_3
#else
#define TM_IDMA_CH2           IDMA_CHANNEL_0
#define TM_IDMA_CH3           IDMA_CHANNEL_0
#endif


// MAX limits for number of tiles, frames memory banks and dma queue length
#define MAX_NUM_MEM_BANKS         8
#define MAX_NUM_TILES             32
#define MAX_NUM_FRAMES            8
#define MAX_NUM_DMA_QUEUE_LENGTH  32 // Optimization, multiple of 2
#define MAX_NUM_CHANNEL           4


#if defined(SUPPORT_3D_TILES)
//support fro 3D Frame and Tile
#define MAX_NUM_FRAMES3D          4
#define MAX_NUM_TILES3D           8
#endif


// Bank colors. XV_MEM_BANK_COLOR_ANY is an unlikely enum value
#define XV_MEM_BANK_COLOR_0    0x0
#define XV_MEM_BANK_COLOR_1    0x1
#define XV_MEM_BANK_COLOR_2    0x2
#define XV_MEM_BANK_COLOR_3    0x3
#define XV_MEM_BANK_COLOR_4    0x4
#define XV_MEM_BANK_COLOR_5    0x5
#define XV_MEM_BANK_COLOR_6    0x6
#define XV_MEM_BANK_COLOR_7    0x7
#define XV_MEM_BANK_COLOR_ANY  0xBEEDDEAFU

// Edge padding format
#define FRAME_ZERO_PADDING      0
#define FRAME_CONSTANT_PADDING  1
#define FRAME_EDGE_PADDING      2
#define FRAME_PADDING_MAX       3

#define XVTM_DUMMY_DMA_INDEX    -2
#define XVTM_ERROR              -1
#define XVTM_SUCCESS            0


/*****************************************
*   Individual status flags definitions
*****************************************/

#define XV_TILE_STATUS_DMA_ONGOING                 (0x01 << 0)
#define XV_TILE_STATUS_LEFT_EDGE_PADDING_NEEDED    (0x01 << 1)
#define XV_TILE_STATUS_RIGHT_EDGE_PADDING_NEEDED   (0x01 << 2)
#define XV_TILE_STATUS_TOP_EDGE_PADDING_NEEDED     (0x01 << 3)
#define XV_TILE_STATUS_BOTTOM_EDGE_PADDING_NEEDED  (0x01 << 4)
#define XV_TILE_STATUS_FRONT_EDGE_PADDING_NEEDED   (0x01 << 5)
#define XV_TILE_STATUS_BACK_EDGE_PADDING_NEEDED    (0x01 << 6)

#define XV_TILE_STATUS_LEFT_EDGE_PADDING_ONGOING   (0x01 << 8)
#define XV_TILE_STATUS_RIGHT_EDGE_PADDING_ONGOING  (0x01 << 9)
#define XV_TILE_STATUS_TOP_EDGE_PADDING_ONGOING    (0x01 << 10)
#define XV_TILE_STATUS_BOTTOM_EDGE_PADDING_ONGOING (0x01 << 11)
#define XV_TILE_STATUS_FRONT_EDGE_PADDING_ONGOING  (0x01 << 12)
#define XV_TILE_STATUS_BACK_EDGE_PADDING_ONGOING   (0x01 << 13)
#define XV_TILE_STATUS_DUMMY_DMA_ONGOING           (0x01 << 14)


#define XV_TILE_STATUS_DUMMY_IDMA_INDEX_NEEDED     (0x01 << 15)



#define XV_TILE_STATUS_EDGE_PADDING_NEEDED    \
  (XV_TILE_STATUS_LEFT_EDGE_PADDING_NEEDED |  \
   XV_TILE_STATUS_RIGHT_EDGE_PADDING_NEEDED | \
   XV_TILE_STATUS_TOP_EDGE_PADDING_NEEDED |   \
   XV_TILE_STATUS_BOTTOM_EDGE_PADDING_NEEDED | \
   XV_TILE_STATUS_FRONT_EDGE_PADDING_NEEDED | \
   XV_TILE_STATUS_BACK_EDGE_PADDING_NEEDED)



/*****************************************
*   Data type definitions
*****************************************/

#define XV_TYPE_SIGNED_BIT         (1 << 15)
#define XV_TYPE_TILE_BIT           (1 << 14)

#define XV_TYPE_ELEMENT_SIZE_BITS  10
#define XV_TYPE_ELEMENT_SIZE_MASK  ((1 << XV_TYPE_ELEMENT_SIZE_BITS) - 1)
#define XV_TYPE_CHANNELS_BITS      2
#define XV_TYPE_CHANNELS_MASK      (((1 << XV_TYPE_CHANNELS_BITS) - 1) << XV_TYPE_ELEMENT_SIZE_BITS)

// XV_MAKETYPE accepts 3 parameters
// 1: flag: Denotes whether the entity is a tile (XV_TYPE_TILE_BIT is set) or an array (XV_TYPE_TILE_BIT is not set),
//    and also if the data is a signed(XV_TYPE_SIGNED_BIT is set) or unsigned(XV_TYPE_SIGNED_BIT is not set).
// 2: depth: Denotes number of bytes per pel.
//    1 implies the data is 8bit, 2 implies the data is 16bit and 4 implies the data is 32bit.
// 3: Denotes number of channels.
//    1 implies gray scale, 3 implies RGB

#define XV_MAKETYPE(flags, depth, channels)  (((depth) * (channels)) | (((channels) - 1) << XV_TYPE_ELEMENT_SIZE_BITS) | (flags))
#define XV_CUSTOMTYPE(type)                  XV_MAKETYPE(0, sizeof(type), 1)

#define XV_TYPE_ELEMENT_SIZE(type)           ((type) & (XV_TYPE_ELEMENT_SIZE_MASK))
#define XV_TYPE_ELEMENT_TYPE(type)           ((type) & (XV_TYPE_SIGNED_BIT | XV_TYPE_CHANNELS_MASK | XV_TYPE_ELEMENT_SIZE_MASK))
#define XV_TYPE_IS_TILE(type)                ((type) & (XV_TYPE_TILE_BIT))
#define XV_TYPE_IS_SIGNED(type)              ((type) & (XV_TYPE_SIGNED_BIT))
#define XV_TYPE_CHANNELS(type)               ((((type) & (XV_TYPE_CHANNELS_MASK)) >> (XV_TYPE_ELEMENT_SIZE_BITS)) + 1)

// Common XV_MAKETYPEs
#define XV_U8         XV_MAKETYPE(0, 1, 1)
#define XV_U16        XV_MAKETYPE(0, 2, 1)
#define XV_U32        XV_MAKETYPE(0, 4, 1)

#define XV_S8         XV_MAKETYPE(XV_TYPE_SIGNED_BIT, 1, 1)
#define XV_S16        XV_MAKETYPE(XV_TYPE_SIGNED_BIT, 2, 1)
#define XV_S32        XV_MAKETYPE(XV_TYPE_SIGNED_BIT, 4, 1)

#define XV_ARRAY_U8   XV_U8
#define XV_ARRAY_S8   XV_S8
#define XV_ARRAY_U16  XV_U16
#define XV_ARRAY_S16  XV_S16
#define XV_ARRAY_U32  XV_U32
#define XV_ARRAY_S32  XV_S32

#define XV_TILE_U8    (XV_U8 | XV_TYPE_TILE_BIT)
#define XV_TILE_S8    (XV_S8 | XV_TYPE_TILE_BIT)
#define XV_TILE_U16   (XV_U16 | XV_TYPE_TILE_BIT)
#define XV_TILE_S16   (XV_S16 | XV_TYPE_TILE_BIT)
#define XV_TILE_U32   (XV_U32 | XV_TYPE_TILE_BIT)
#define XV_TILE_S32   (XV_S32 | XV_TYPE_TILE_BIT)

/*****************************************
*    Frame Access Macros
*****************************************/

#define XV_FRAME_GET_BUFF_PTR(pFrame)                   ((pFrame)->pFrameBuff)
#define XV_FRAME_SET_BUFF_PTR(pFrame, pBuff)            (pFrame)->pFrameBuff = ((uint64_t)(pBuff))

#define XV_FRAME_GET_BUFF_SIZE(pFrame)                  ((pFrame)->frameBuffSize)
#define XV_FRAME_SET_BUFF_SIZE(pFrame, buffSize)        (pFrame)->frameBuffSize = ((uint32_t) (buffSize))

#define XV_FRAME_GET_DATA_PTR(pFrame)                   ((pFrame)->pFrameData)
#define XV_FRAME_SET_DATA_PTR(pFrame, pData)            (pFrame)->pFrameData = ((uint64_t)(pData))

#define XV_FRAME_GET_WIDTH(pFrame)                      ((pFrame)->frameWidth)
#define XV_FRAME_SET_WIDTH(pFrame, width)               (pFrame)->frameWidth = ((int32_t) (width))

#define XV_FRAME_GET_HEIGHT(pFrame)                     ((pFrame)->frameHeight)
#define XV_FRAME_SET_HEIGHT(pFrame, height)             (pFrame)->frameHeight = ((int32_t) (height))

#define XV_FRAME_GET_PITCH(pFrame)                      ((pFrame)->framePitch)
#define XV_FRAME_SET_PITCH(pFrame, pitch)               (pFrame)->framePitch = ((int32_t) (pitch))
#define XV_FRAME_GET_PITCH_IN_BYTES(pFrame)             ((pFrame)->framePitch * (pFrame)->pixelRes)

#define XV_FRAME_GET_PIXEL_RES(pFrame)                  ((pFrame)->pixelRes)
#define XV_FRAME_SET_PIXEL_RES(pFrame, pixRes)          (pFrame)->pixelRes = ((uint8_t) (pixRes))

#define XV_FRAME_GET_NUM_CHANNELS(pFrame)               ((pFrame)->numChannels)
#define XV_FRAME_SET_NUM_CHANNELS(pFrame, pixelFormat)  (pFrame)->numChannels = ((uint8_t) (pixelFormat))

#define XV_FRAME_GET_EDGE_WIDTH(pFrame)                 ((pFrame)->leftEdgePadWidth < (pFrame)->rightEdgePadWidth ? (pFrame)->leftEdgePadWidth : (pFrame)->rightEdgePadWidth)
#define XV_FRAME_SET_EDGE_WIDTH(pFrame, padWidth)         \
  {                                                       \
    (pFrame)->leftEdgePadWidth  = ((uint8_t) (padWidth)); \
    (pFrame)->rightEdgePadWidth = ((uint8_t) (padWidth)); \
  }

#define XV_FRAME_GET_EDGE_HEIGHT(pFrame)  ((pFrame)->topEdgePadHeight < (pFrame)->bottomEdgePadHeight ? (pFrame)->topEdgePadHeight : (pFrame)->bottomEdgePadHeight)
#define XV_FRAME_SET_EDGE_HEIGHT(pFrame, padHeight)          \
  {                                                          \
    (pFrame)->topEdgePadHeight    = ((uint8_t) (padHeight)); \
    (pFrame)->bottomEdgePadHeight = ((uint8_t) (padHeight)); \
  }

#define XV_FRAME_GET_EDGE_LEFT(pFrame)               ((pFrame)->leftEdgePadWidth)
#define XV_FRAME_SET_EDGE_LEFT(pFrame, padWidth)     (pFrame)->leftEdgePadWidth = ((uint8_t) (padWidth))

#define XV_FRAME_GET_EDGE_RIGHT(pFrame)              ((pFrame)->rightEdgePadWidth)
#define XV_FRAME_SET_EDGE_RIGHT(pFrame, padWidth)    (pFrame)->rightEdgePadWidth = ((uint8_t) (padWidth))

#define XV_FRAME_GET_EDGE_TOP(pFrame)                ((pFrame)->topEdgePadHeight)
#define XV_FRAME_SET_EDGE_TOP(pFrame, padHeight)     (pFrame)->topEdgePadHeight = ((uint8_t) (padHeight))

#define XV_FRAME_GET_EDGE_BOTTOM(pFrame)             ((pFrame)->bottomEdgePadHeight)
#define XV_FRAME_SET_EDGE_BOTTOM(pFrame, padHeight)  (pFrame)->bottomEdgePadHeight = ((uint8_t) (padHeight))

#define XV_FRAME_GET_PADDING_TYPE(pFrame)            ((pFrame)->paddingType)
#define XV_FRAME_SET_PADDING_TYPE(pFrame, padType)   (pFrame)->paddingType = (padType)

#define XV_FRAME_GET_PADDING_VALUE(pFrame)           ((pFrame)->paddingVal)
#define XV_FRAME_SET_PADDING_VALUE(pFrame, padVal)   (pFrame)->paddingVal = (padVal)

/*****************************************
*    Array Access Macros
*****************************************/

#define XV_ARRAY_GET_BUFF_PTR(pArray)              ((pArray)->pBuffer)
#define XV_ARRAY_SET_BUFF_PTR(pArray, pBuff)       (pArray)->pBuffer = ((void *) (pBuff))

#define XV_ARRAY_GET_BUFF_SIZE(pArray)             ((pArray)->bufferSize)
#define XV_ARRAY_SET_BUFF_SIZE(pArray, buffSize)   (pArray)->bufferSize = ((uint32_t) (buffSize))

#define XV_ARRAY_GET_DATA_PTR(pArray)              ((pArray)->pData)
#define XV_ARRAY_SET_DATA_PTR(pArray, pArrayData)  (pArray)->pData = ((void *) (pArrayData))

#define XV_ARRAY_GET_WIDTH(pArray)                 ((pArray)->width)
#define XV_ARRAY_SET_WIDTH(pArray, value)          (pArray)->width = ((int32_t) (value))

#define XV_ARRAY_GET_PITCH(pArray)                 ((pArray)->pitch)
#define XV_ARRAY_SET_PITCH(pArray, value)          (pArray)->pitch = ((int32_t) (value))

#define XV_ARRAY_GET_HEIGHT(pArray)                ((pArray)->height)
#define XV_ARRAY_SET_HEIGHT(pArray, value)         (pArray)->height = ((uint16_t) (value))

#define XV_ARRAY_GET_STATUS_FLAGS(pArray)          ((pArray)->status)
#define XV_ARRAY_SET_STATUS_FLAGS(pArray, value)   (pArray)->status = ((uint8_t) (value))

#define XV_ARRAY_GET_TYPE(pArray)                  ((pArray)->type)
#define XV_ARRAY_SET_TYPE(pArray, value)           (pArray)->type = ((uint16_t) (value))

#define XV_ARRAY_GET_CAPACITY(pArray)              XV_ARRAY_GET_PITCH(pArray)
#define XV_ARRAY_SET_CAPACITY(pArray, value)       XV_ARRAY_SET_PITCH((pArray), (value))

#define XV_ARRAY_GET_ELEMENT_TYPE(pArray)          XV_TYPE_ELEMENT_TYPE(XV_ARRAY_GET_TYPE(pArray))
#define XV_ARRAY_GET_ELEMENT_SIZE(pArray)          XV_TYPE_ELEMENT_SIZE(XV_ARRAY_GET_TYPE(pArray))
#define XV_ARRAY_IS_TILE(pArray)                   XV_TYPE_IS_TILE(XV_ARRAY_GET_TYPE(pArray) & (XV_TYPE_TILE_BIT))

#define XV_ARRAY_GET_AREA(pArray)                  (((pArray)->width) * ((int32_t) (pArray)->height))

/*****************************************
*    Tile Access Macros
*****************************************/

#define XV_TILE_GET_BUFF_PTR   XV_ARRAY_GET_BUFF_PTR
#define XV_TILE_SET_BUFF_PTR   XV_ARRAY_SET_BUFF_PTR

#define XV_TILE_GET_BUFF_SIZE  XV_ARRAY_GET_BUFF_SIZE
#define XV_TILE_SET_BUFF_SIZE  XV_ARRAY_SET_BUFF_SIZE

#define XV_TILE_GET_DATA_PTR   XV_ARRAY_GET_DATA_PTR
#define XV_TILE_SET_DATA_PTR   XV_ARRAY_SET_DATA_PTR

#define XV_TILE_GET_WIDTH      XV_ARRAY_GET_WIDTH
#define XV_TILE_SET_WIDTH      XV_ARRAY_SET_WIDTH

#define XV_TILE_GET_PITCH      XV_ARRAY_GET_PITCH
#define XV_TILE_SET_PITCH      XV_ARRAY_SET_PITCH
#define XV_TILE_GET_PITCH_IN_BYTES(pTile)  ((pTile)->pitch * (int32_t) ((pTile)->pFrame->pixelRes))

#define XV_TILE_GET_HEIGHT        XV_ARRAY_GET_HEIGHT
#define XV_TILE_SET_HEIGHT        XV_ARRAY_SET_HEIGHT

#define XV_TILE_GET_STATUS_FLAGS  XV_ARRAY_GET_STATUS_FLAGS
#define XV_TILE_SET_STATUS_FLAGS  XV_ARRAY_SET_STATUS_FLAGS

#define XV_TILE_GET_TYPE          XV_ARRAY_GET_TYPE
#define XV_TILE_SET_TYPE          XV_ARRAY_SET_TYPE

#define XV_TILE_GET_ELEMENT_TYPE  XV_ARRAY_GET_ELEMENT_TYPE
#define XV_TILE_GET_ELEMENT_SIZE  XV_ARRAY_GET_ELEMENT_SIZE
#define XV_TILE_IS_TILE           XV_ARRAY_IS_TILE

#define XV_TILE_RESET_DMA_INDEX(pTile)              ((pTile)->dmaIndex = 0)
#define XV_TILE_RESET_PREVIOUS_TILE(pTile)          (pTile)->pPrevTile = ((xvTile *) (NULL))
#define XV_TILE_RESET_REUSE_COUNT(pTile)            ((pTile)->reuseCount = 0)

#define XV_TILE_GET_FRAME_PTR(pTile)                ((pTile)->pFrame)
#define XV_TILE_SET_FRAME_PTR(pTile, ptrFrame)      (pTile)->pFrame = ((xvFrame *) (ptrFrame))

#define XV_TILE_GET_X_COORD(pTile)                  ((pTile)->x)
#define XV_TILE_SET_X_COORD(pTile, xcoord)          (pTile)->x = ((int32_t) (xcoord))

#define XV_TILE_GET_Y_COORD(pTile)                  ((pTile)->y)
#define XV_TILE_SET_Y_COORD(pTile, ycoord)          (pTile)->y = ((int32_t) (ycoord))

#define XV_TILE_GET_EDGE_LEFT(pTile)                ((pTile)->tileEdgeLeft)
#define XV_TILE_SET_EDGE_LEFT(pTile, edgeWidth)     (pTile)->tileEdgeLeft = ((uint16_t) (edgeWidth))

#define XV_TILE_GET_EDGE_RIGHT(pTile)               ((pTile)->tileEdgeRight)
#define XV_TILE_SET_EDGE_RIGHT(pTile, edgeWidth)    (pTile)->tileEdgeRight = ((uint16_t) (edgeWidth))

#define XV_TILE_GET_EDGE_TOP(pTile)                 ((pTile)->tileEdgeTop)
#define XV_TILE_SET_EDGE_TOP(pTile, edgeHeight)     (pTile)->tileEdgeTop = ((uint16_t) (edgeHeight))

#define XV_TILE_GET_EDGE_BOTTOM(pTile)              ((pTile)->tileEdgeBottom)
#define XV_TILE_SET_EDGE_BOTTOM(pTile, edgeHeight)  (pTile)->tileEdgeBottom = ((uint16_t) (edgeHeight))

#define XV_TILE_GET_EDGE_WIDTH(pTile)               (((pTile)->tileEdgeLeft < (pTile)->tileEdgeRight) ? (pTile)->tileEdgeLeft : (pTile)->tileEdgeRight)
#define XV_TILE_SET_EDGE_WIDTH(pTile, edgeWidth)       \
  {                                                    \
    (pTile)->tileEdgeLeft  = ((uint16_t) (edgeWidth)); \
    (pTile)->tileEdgeRight = ((uint16_t) (edgeWidth)); \
  }

#define XV_TILE_GET_EDGE_HEIGHT(pTile)  (((pTile)->tileEdgeTop < (pTile)->tileEdgeBottom) ? (pTile)->tileEdgeTop : (pTile)->tileEdgeBottom)
#define XV_TILE_SET_EDGE_HEIGHT(pTile, edgeHeight)       \
  {                                                      \
    (pTile)->tileEdgeTop    = ((uint16_t) (edgeHeight)); \
    (pTile)->tileEdgeBottom = ((uint16_t) (edgeHeight)); \
  }

#define XV_TILE_CHECK_STATUS_FLAGS_DMA_ONGOING(pTile)          (((pTile)->status & XV_TILE_STATUS_DMA_ONGOING) > 0)
#define XV_TILE_CHECK_STATUS_FLAGS_EDGE_PADDING_NEEDED(pTile)  (((pTile)->status & XV_TILE_STATUS_EDGE_PADDING_NEEDED) > 0)



#if defined(SUPPORT_3D_TILES)

#define XV_TILE_3D_GET_FRAME_3D_PTR(pTile3D)                  ((pTile3D)->pFrame)
#define XV_TILE_3D_SET_FRAME_3D_PTR(pTile3D, ptrFrame3D)      (pTile3D)->pFrame = ((ptrFrame3D))



#define XV_FRAME_GET_DEPTH(pFrame3D)  ((pFrame3D)->frameDepth)
#define XV_FRAME_GET_FRAME_PITCH(pFrame3D) ((pFrame3D)->frame2DFramePitch)


#define XV_FRAME_SET_DEPTH(pFrame3D, numTiles)  ((pFrame3D)->frameDepth = numTiles)
#define XV_FRAME_SET_FRAME_PITCH(pFrame3D, Frame2DPitch)  ((pFrame3D)->frame2DFramePitch = Frame2DPitch)


#define XV_FRAME_SET_EDGE_DEPTH(pFrame3D, edgeDepth)        \
          {                                                 \
              ((pFrame3D)->frontEdgePadDepth = edgeDepth);  \
              ((pFrame3D)->backEdgePadDepth = edgeDepth);   \
          }

#define XV_TILE_GET_DEPTH(pTile3D)                    ((pTile3D)->depth)
#define XV_TILE_SET_DEPTH(pTile3D, Tdepth)            ((pTile3D)->depth = Tdepth)
#define XV_TILE_GET_Z_COORD(pTile3D)                  ((pTile3D)->z)
#define XV_TILE_SET_Z_COORD(pTile3D, zcoord)          (pTile3D)->z = ((int32_t) (zcoord))

#define XV_TILE_GET_EDGE_FRONT(pTile3D)               ((pTile3D)->tileEdgeFront)
#define XV_TILE_GET_EDGE_BACK(pTile3D)                ((pTile3D)->tileEdgeBack)
#define XV_TILE_GET_EDGE_DEPTH(pTile3D)               ((pTile3D)->tileEdgeBack < (pTile3D)->tileEdgeFront ? (pTile3D)->tileEdgeFront : (pTile3D)->tileEdgeBack)


#define XV_TILE_SET_EDGE_FRONT(pTile3D, frontEdgePad)       (pTile3D)->tileEdgeFront = ((int32_t) (frontEdgePad))
#define XV_TILE_SET_EDGE_BACK(pTile3D, backEdgePad)       (pTile3D)->tileEdgeBack = ((int32_t) (backEdgePad))
#define XV_TILE_SET_EDGE_DEPTH(pTile3D, DepthPad)                             \
          {                                                                   \
             (pTile3D)->tileEdgeFront = ((int32_t) (DepthPad));           \
             (pTile3D)->tileEdgeBack = ((int32_t) (DepthPad));            \
          }

#define XV_TILE_SET_TILE_PITCH(pTile3D, Tile2DPitch)  ((pTile3D)->Tile2Dpitch = Tile2DPitch )
#define XV_TILE_GET_TILE_PITCH(pTile3D)  ((pTile3D)->Tile2Dpitch)

#endif


/*Structures and enums*/

typedef enum {
    TILE_UNALIGNED = 0,
    EDGE_ALIGNED_32,
    DATA_ALIGNED_32,
    EDGE_ALIGNED_64,
    DATA_ALIGNED_64,
} buffer_align_type_t;

typedef enum {
    XV_ERROR_SUCCESS            = 0,
    XV_ERROR_TILE_MANAGER_NULL  = 1,
    XV_ERROR_POINTER_NULL       = 2,
    XV_ERROR_FRAME_NULL         = 3,
    XV_ERROR_TILE_NULL          = 4,
    XV_ERROR_BUFFER_NULL        = 5,
    XV_ERROR_ALLOC_FAILED       = 6,
    XV_ERROR_FRAME_BUFFER_FULL  = 7,
    XV_ERROR_TILE_BUFFER_FULL   = 8,
    XV_ERROR_DIMENSION_MISMATCH = 9,
    XV_ERROR_BUFFER_OVERFLOW    = 10,
    XV_ERROR_BAD_ARG            = 11,
    XV_ERROR_FILE_OPEN          = 12,
    XV_ERROR_DMA_INIT           = 13,
    XV_ERROR_XVMEM_INIT         = 14,
    XV_ERROR_IDMA               = 15
} xvError_t;


typedef struct xvFrameStruct {
    uint64_t pFrameBuff;
    uint32_t frameBuffSize;
    uint64_t pFrameData;
    int32_t  frameWidth;
    int32_t  frameHeight;
    int32_t  framePitch;
    uint8_t  pixelRes;
    uint8_t  numChannels;
    uint8_t  leftEdgePadWidth;
    uint8_t  topEdgePadHeight;
    uint8_t  rightEdgePadWidth;
    uint8_t  bottomEdgePadHeight;
    uint8_t  paddingType;
    uint32_t paddingVal;
#if defined(XI_XV_TILE_COMPATIBILITY_TEST)
} __attribute__((packed))  xvFrame, *xvpFrame;
#else
}
xvFrame, *xvpFrame;
#endif




#define XV_ARRAY_FIELDS \
  void     *pBuffer;    \
  uint32_t bufferSize;  \
  void *pData;          \
  int32_t width;        \
  int32_t pitch;        \
  uint32_t status;      \
  uint16_t type;        \
  uint16_t height;






typedef struct xvArrayStruct {
    XV_ARRAY_FIELDS
} xvArray, *xvpArray;


typedef struct xvTileStruct {
    XV_ARRAY_FIELDS
    xvFrame             *pFrame;
    int32_t             x;
    int32_t             y;
    uint16_t            tileEdgeLeft;
    uint16_t            tileEdgeTop;
    uint16_t            tileEdgeRight;
    uint16_t            tileEdgeBottom;
    int32_t             dmaIndex;
    int32_t             reuseCount;
    struct xvTileStruct *pPrevTile;
#if defined(XI_XV_TILE_COMPATIBILITY_TEST)
} __attribute__((packed)) xvTile, *xvpTile;
#else
} xvTile, *xvpTile;
#endif


#if defined (SUPPORT_3D_TILES)


typedef struct xvFrame3DStruct {
    uint64_t pFrameBuff;
    uint32_t frameBuffSize;
    uint64_t pFrameData;
    int32_t  frameWidth;
    int32_t  frameHeight;
    int32_t  framePitch;
    uint8_t  pixelRes;
    uint8_t  numChannels;
    uint16_t leftEdgePadWidth;
    uint16_t rightEdgePadWidth;
    uint16_t topEdgePadHeight;
    uint16_t bottomEdgePadHeight;
    uint16_t frontEdgePadDepth;
    uint16_t backEdgePadDepth;
    uint8_t  paddingType;

    int32_t  frame2DFramePitch;
    int32_t  frameDepth;

    uint32_t paddingVal;

}  xvFrame3D, *xvpFrame3D;



typedef struct xvTileStruct3D {

    void       *pBuffer;
    uint32_t    bufferSize;
    void       *pData;
    int32_t     width;
    int32_t     pitch;
    uint32_t    status;
    uint16_t    type;
    uint16_t    height;
    xvpFrame3D  pFrame;
    int32_t     x;
    int32_t     y;
    uint16_t    tileEdgeLeft;
    uint16_t    tileEdgeTop;
    uint16_t    tileEdgeRight;
    uint16_t    tileEdgeBottom;

    int32_t    Tile2Dpitch;
    uint16_t   depth;
    uint32_t    dataOrder;
    int32_t    z;

    uint16_t  tileEdgeFront;
    uint16_t  tileEdgeBack;
    int32_t   dmaIndex;
    void      *pTemp;
} xvTile3D, *xvpTile3D;

#endif

typedef struct xvTileManagerStruct {
    // iDMA related
    void    *pdmaObj0;
    void    *pdmaObj1;
    void    *pdmaObj2;
    void    *pdmaObj3;

    int32_t tileDMApendingCount[XCHAL_IDMA_NUM_CHANNELS];       // Incremented when new request is added. Decremented when request is completed.
    int32_t tileDMAstartIndex[XCHAL_IDMA_NUM_CHANNELS];         // Incremented when request is completed
    xvTile  *tileProcQueue[XCHAL_IDMA_NUM_CHANNELS][MAX_NUM_DMA_QUEUE_LENGTH];

    // Mem Banks
    int32_t     numMemBanks;                       // Number of memory banks/pools
    xvmem_mgr_t memBankMgr[MAX_NUM_MEM_BANKS];     // xvmem memory manager, one for each bank
    void        *pMemBankStart[MAX_NUM_MEM_BANKS]; // Start address of bank
    int32_t     memBankSize[MAX_NUM_MEM_BANKS];    // size of each bank
    // Tiles and frame allocation
    xvTile    tileArray[MAX_NUM_TILES];
    xvFrame   frameArray[MAX_NUM_FRAMES];
    int32_t   tileAllocFlags[(MAX_NUM_TILES + 31) / 32];      // Each bit of tileAllocFlags and frameAllocFlags
    int32_t   frameAllocFlags[(MAX_NUM_FRAMES + 31) / 32];    // indicates if a particular tile/frame is allocated

    int32_t   tileCount;
    int32_t   frameCount;


    xvError_t errFlag;
    xvError_t idmaErrorFlag[4];               //Allocate for MAX iDMA channels.

#if defined(SUPPORT_3D_TILES)

    xvTile3D  tile3DArray[MAX_NUM_TILES3D];
    xvFrame3D frame3DArray[MAX_NUM_FRAMES3D];
    int32_t   tile3DAllocFlags[(MAX_NUM_TILES3D + 31) / 32];      // Each bit of tile3DAllocFlags and frame3DAllocFlags
    int32_t   frame3DAllocFlags[(MAX_NUM_FRAMES3D + 31) / 32];    // indicates if a particular tile3D/frame3D is allocated

    int32_t tile3DDMApendingCount[XCHAL_IDMA_NUM_CHANNELS];       // Incremented when new request is added. Decremented when request is completed.
    int32_t tile3DDMAstartIndex[XCHAL_IDMA_NUM_CHANNELS];         // Incremented when request is completed
    xvTile3D  *tile3DProcQueue[XCHAL_IDMA_NUM_CHANNELS][MAX_NUM_DMA_QUEUE_LENGTH];

    int32_t   tile3DCount;
    int32_t   frame3DCount;

#endif

} xvTileManager;


/***********************************
*    Function  Prototypes
***********************************/



/**********************************************************************************
 * FUNCTION: xvInitIdmaMultiChannel4CH()
 *
 * DESCRIPTION:
 *     Function to initialize iDMA library. Tile Manager uses iDMA library
 *     in fixed buffer mode. DMA transfer is scheduled as soon as the descriptor
 *     is added.
 *
 *
 * INPUTS:
 *     xvTileManager        *pxvTM              Tile Manager object
 *     idma_buffer_t        *buf0               iDMA library handle; contains descriptors and idma library object for channel0
 *     idma_buffer_t        *buf1               iDMA library handle; contains descriptors and idma library object for channel1. For single channel mode. buf1 can be NULL;.
 *     idma_buffer_t        *buf2               iDMA library handle; contains descriptors and idma library object for channel2  For single channel mode. buf2 can be NULL;.
 *     idma_buffer_t        *buf3               iDMA library handle; contains descriptors and idma library object for channel3  For single channel mode. buf3 can be NULL;.
 *     int32_t              numDescs            Number of descriptors that can be added in buffer
 *     int32_t              maxBlock            Maximum block size allowed
 *     int32_t              maxPifReq           Maximum number of outstanding pif requests
 *     idma_err_callback_fn errCallbackFunc0    Callback for dma transfer error for channel 0
 *     idma_err_callback_fn errCallbackFunc1    Callback for dma transfer error for channel 1
 *     idma_err_callback_fn errCallbackFunc2    Callback for dma transfer error for channel 2
 *     idma_err_callback_fn errCallbackFunc3    Callback for dma transfer error for channel 3
 *     idma_callback_fn     cbFunc0             Callback for dma transfer completion for channel 0
 *     void                 *cbData0            Data needed for completion callback function for channel 0
 *     idma_callback_fn     cbFunc1             Callback for dma transfer completion for channel 1
 *     void                 *cbData1            Data needed for completion callback function for channel 1
 *     idma_callback_fn     cbFunc2             Callback for dma transfer completion for channel 2
 *     void                 *cbData2            Data needed for completion callback function for channel 2
 *     idma_callback_fn     cbFunc3             Callback for dma transfer completion for channel 3
 *     void                 *cbData3            Data needed for completion callback function for channel 3
 *
 * OUTPUTS:
 *     Returns XVTM_ERROR if it encounters an error, else returns XVTM_SUCCESS
 *
 ********************************************************************************** */

int32_t xvInitIdmaMultiChannel4CH(xvTileManager *pxvTM, idma_buffer_t *buf0, idma_buffer_t *buf1, idma_buffer_t *buf2,
                                  idma_buffer_t *buf3,
                                  int32_t numDescs, int32_t maxBlock, int32_t maxPifReq, idma_err_callback_fn errCallbackFunc0,
                                  idma_err_callback_fn errCallbackFunc1, idma_err_callback_fn errCallbackFunc2,  idma_err_callback_fn errCallbackFunc3,
                                  idma_callback_fn cbFunc0, void *cbData0, idma_callback_fn cbFunc1, void *cbData1,
                                  idma_callback_fn cbFunc2, void *cbData2, idma_callback_fn cbFunc3, void *cbData3);


#define xvInitIdmaMultiChannel(pxvTM, buf0, buf1,                                          \
          numDescs, maxBlock, maxPifReq, errCallbackFunc0,                                  \
          errCallbackFunc1,  cbFunc0, cbData0, cbFunc1, cbData1)                            \
                                                                                            \
          xvInitIdmaMultiChannel4CH(pxvTM, buf0, buf1, NULL, NULL,                           \
          numDescs, maxBlock, maxPifReq, errCallbackFunc0, errCallbackFunc1, NULL, NULL,    \
          cbFunc0, cbData0, cbFunc1, cbData1, NULL, NULL, NULL, NULL)                       \




// Initializes Tile Manager
// pxvTM           - Tile Manager object
// buf0, buf1      - iDMA buffer
// buf2, buf3      - iDMA buffer
// Returns XVTM_ERROR if it encounters an error, else returns XVTM_SUCCESS
int32_t xvInitTileManagerMultiChannel4CH(xvTileManager *pxvTM, idma_buffer_t *buf0, idma_buffer_t *buf1,
        idma_buffer_t *buf2, idma_buffer_t *buf3);

#define xvInitTileManagerMultiChannel(pxvTM, buf0, buf1)     xvInitTileManagerMultiChannel4CH(pxvTM, buf0, buf1, NULL, NULL)



// Resets Tile Manager
// pxvTM        - Tile Manager object
// Returns XVTM_ERROR if it encounters an error, else returns XVTM_SUCCESS
int32_t xvResetTileManager(xvTileManager *pxvTM);

// Initializes memory manager
// pxvTM         - Tile Manager object
// numMemBanks   - Number of memory pools
// pBankBuffPool - Array of start addresses of memory bank
// buffPoolSize  - Array of sizes of memory bank
// Returns XVTM_ERROR if it encounters an error, else returns XVTM_SUCCESS
int32_t xvInitMemAllocator(xvTileManager *pxvTM, int32_t numMemBanks, void **pBankBuffPool, int32_t *buffPoolSize);


// Allocates buffer from the pool
// pxvTM         - Tile Manager object
// buffSize      - size of requested buffer
// buffColor     - color/index of requested bufffer
// buffAlignment - Alignment of requested buffer
// Returns the buffer with requested parameters. If an error occurs, returns ((void *)(XVTM_ERROR))
void *xvAllocateBuffer(xvTileManager *pxvTM, int32_t buffSize, int32_t buffColor, int32_t buffAlignment);


// Releases the given buffer
// pxvTM - Tile Manager object
// pBuff - Pointer to buffer that needs to be released
// Returns XVTM_ERROR if it encounters an error, else returns XVTM_SUCCESS
int32_t xvFreeBuffer(xvTileManager *pxvTM, void const *pBuff);


// Releases all buffers. Reinitializes the memory allocator
// pxvTM - Tile Manager object
// Returns XVTM_ERROR if it encounters an error, else returns XVTM_SUCCESS
int32_t xvFreeAllBuffers(xvTileManager *pxvTM);


// Allocates single frame
// pxvTM - Tile Manager object
// Returns the pointer to allocated frame. Does not allocate frame data buffer.
// Returns ((xvFrame *)(XVTM_ERROR)) if it encounters an error.
xvFrame *xvAllocateFrame(xvTileManager *pxvTM);


// Releases given frame
// pxvTM  - Tile Manager object
// pFrame - Frame that needs to be released. Does not release buffer
// Returns XVTM_ERROR if it encounters an error, else returns XVTM_SUCCESS
int32_t xvFreeFrame(xvTileManager *pxvTM, xvFrame const *pFrame);


// Releases all allocated frames
// pxvTM  - Tile Manager object
// Returns XVTM_ERROR if it encounters an error, else returns XVTM_SUCCESS
int32_t  xvFreeAllFrames(xvTileManager *pxvTM);


// Allocates single tile
// pxvTM - Tile Manager object
// Returns the pointer to allocated tile. Does not allocate tile data buffer
// Returns ((xvTile *)(XVTM_ERROR)) if it encounters an error.
xvTile *xvAllocateTile(xvTileManager *pxvTM);


// Releases given tile
// pxvTM  - Tile Manager object
// pFrame - Tile that needs to be released. Does not release buffer
// Returns XVTM_ERROR if it encounters an error, else returns XVTM_SUCCESS
int32_t xvFreeTile(xvTileManager *pxvTM, xvTile const *pTile);


// Releases all allocated tiles
// pxvTM  - Tile Manager object
// Returns XVTM_ERROR if it encounters an error, else returns XVTM_SUCCESS
int32_t xvFreeAllTiles(xvTileManager *pxvTM);


// Add iDMA transfer request
// dmaChannel            - dmaChannel to be used
// pxvTM                 - Tile Manager object
// dst                   - pointer to destination buffer
// src                   - pointer to source buffer
// rowSize               - number of bytes to transfer in a row
// numRows               - number of rows to transfer
// srcPitch              - source buffer's pitch in bytes
// dstPitch              - destination buffer's pitch in bytes
// interruptOnCompletion - if it is set, iDMA will interrupt after completing transfer
// pred_mask             - prediction mask buffer pointer
// Returns dmaIndex for this request. It returns -1 if it encounters an error
int32_t xvAddIdmaRequestMultiChannel_predicated_wide(int32_t dmaChannel, xvTileManager *pxvTM, uint64_t dst,
        uint64_t src, size_t rowSize,
        int32_t numRows, int32_t srcPitch, int32_t dsPitch, int32_t interruptOnCompletion, uint32_t *pred_mask);



// Add iDMA transfer request
// dmaChannel            - dmaChannel to be used
// pxvTM                 - Tile Manager object
// dst                   - pointer to destination buffer
// src                   - pointer to source buffer
// rowSize               - number of bytes to transfer in a row
// numRows               - number of rows to transfer
// srcPitch              - source buffer's pitch in bytes
// dstPitch              - destination buffer's pitch in bytes
// interruptOnCompletion - if it is set, iDMA will interrupt after completing transfer
// Returns dmaIndex for this request. It returns -1 if it encounters an error
int32_t xvAddIdmaRequestMultiChannel(int32_t dmaChannel, xvTileManager *pxvTM, void *dst, void *src, size_t rowSize,
                                     int32_t numRows, int32_t srcPitch, int32_t dsPitch, int32_t interruptOnCompletion);



// Requests data transfer from frame present in system memory to local tile memory
// dmaChannel            - dmaChannel to be used
// pxvTM                 - Tile Manager object
// pTile                 - destination tile
// interruptOnCompletion - if it is set, iDMA will interrupt after completing transfer
// pred_mask             - Predication mask pointer
// Returns XVTM_ERROR if it encounters an error, else it returns XVTM_SUCCESS
int32_t xvReqTileTransferInMultiChannelPredicated(int32_t dmaChannel, xvTileManager *pxvTM, xvTile *pTile,
        int32_t interruptOnCompletion, uint32_t *pred_mask);


// Requests data transfer from frame present in system memory to local tile memory
// dmaChannel            - dmaChannel to be used
// pxvTM                 - Tile Manager object
// pTile                 - destination tile
// pPrevTile             - data is copied from this tile to pTile if the buffer overlaps
// interruptOnCompletion - if it is set, iDMA will interrupt after completing transfer
// Returns XVTM_ERROR if it encounters an error, else it returns XVTM_SUCCESS
int32_t xvReqTileTransferInMultiChannel(int32_t dmaChannel, xvTileManager *pxvTM, xvTile *pTile, xvTile *pPrevTile,
                                        int32_t interruptOnCompletion);


// Requests 8b data transfer from frame present in system memory to local tile memory
// dmaChannel            - dmaChannel to be used
// pxvTM                 - Tile Manager object
// pTile                 - destination tile
// interruptOnCompletion - if it is set, iDMA will interrupt after completing transfer
// Returns XVTM_ERROR if it encounters an error, else it returns XVTM_SUCCESS
int32_t xvReqTileTransferInFastMultiChannel(int32_t dmaChannel, xvTileManager *pxvTM, xvTile *pTile,
        int32_t interruptOnCompletion);

// Requests 16b data transfer from frame present in system memory to local tile memory
// dmaChannel            - dmaChannel to be used
// pxvTM                 - Tile Manager object
// pTile                 - destination tile
// interruptOnCompletion - if it is set, iDMA will interrupt after completing transfer
// Returns XVTM_ERROR if it encounters an error, else it returns XVTM_SUCCESS
int32_t xvReqTileTransferInFast16MultiChannel(int32_t dmaChannel, xvTileManager *pxvTM, xvTile *pTile,
        int32_t interruptOnCompletion);

// Requests data transfer from tile present in local memory to frame in system memory
// dmaChannel            - dmaChannel to be used
// pxvTM                 - Tile Manager object
// pTile                 - source tile
// interruptOnCompletion - if it is set, iDMA will interrupt after completing transfer
// pred_mask             - predication mask pointer.
// Returns XVTM_ERROR if it encounters an error, else it returns XVTM_SUCCESS
int32_t xvReqTileTransferOutMultiChannelPredicated(int32_t dmaChannel, xvTileManager *pxvTM, xvTile *pTile,
        int32_t interruptOnCompletion, uint32_t *pred_mask);

// Requests data transfer from tile present in local memory to frame in system memory
// dmaChannel            - dmaChannel to be used
// pxvTM                 - Tile Manager object
// pTile                 - source tile
// interruptOnCompletion - if it is set, iDMA will interrupt after completing transfer
// Returns XVTM_ERROR if it encounters an error, else it returns XVTM_SUCCESS
int32_t xvReqTileTransferOutMultiChannel(int32_t dmaChannel, xvTileManager *pxvTM, xvTile *pTile,
        int32_t interruptOnCompletion);

// Requests 8b data transfer from tile present in local memory to frame in system memory
// dmaChannel            - dmaChannel to be used
// pxvTM                 - Tile Manager object
// pTile                 - source tile
// interruptOnCompletion - if it is set, iDMA will interrupt after completing transfer
// Returns XVTM_ERROR if it encounters an error, else it returns XVTM_SUCCESS
int32_t xvReqTileTransferOutFastMultiChannel(int32_t dmaChannel, xvTileManager *pxvTM, xvTile *pTile,
        int32_t interruptOnCompletion);

// Requests 16b data transfer from tile present in local memory to frame in system memory
// dmaChannel            - dmaChannel to be used
// pxvTM                 - Tile Manager object
// pTile                 - source tile
// interruptOnCompletion - if it is set, iDMA will interrupt after completing transfer
// Returns XVTM_ERROR if it encounters an error, else it returns XVTM_SUCCESS
int32_t xvReqTileTransferOutFast16MultiChannel(int32_t dmaChannel, xvTileManager *pxvTM, xvTile *pTile,
        int32_t interruptOnCompletion);

//Pads 8b edge of the given tile
// pxvTM - Tile Manager object
// pTile - tile
int32_t xvPadEdges(xvTileManager *pxvTM, xvTile *pTile);


//Pads 16b edge of the given tile
// pxvTM - Tile Manager object
// pTile - tile
int32_t xvPadEdges16(xvTileManager *pxvTM, xvTile *pTile);


// Check if dma transfer is done
// dmaChannel - dmaChannel to be used
// pxvTM      - Tile Manager object
// index      - index for dma transfer request
// Returns XVTM_ERROR if an error occurs
int32_t xvCheckForIdmaIndexMultiChannel(int32_t dmaChannel, xvTileManager *pxvTM, int32_t index);

// Check if tile is ready
// pxvTM - Tile Manager object
// pTile - input tile
// Takes care of padding, tile reuse
// Completes all tile transfers before the input tile
// Returns 1 if tile is ready, else returns 0
// Returns XVTM_ERROR if an error occurs
int32_t xvCheckTileReadyMultiChannel(int32_t dmaChannel, xvTileManager *pxvTM, xvTile const *pTile);

// Check if input tile is free.
// A tile is said to be free if all data transfers pertaining to data resue from this tile is completed
// pxvTM - Tile Manager object
// pTile - output tile
// Returns 1 if tile is free, else returns 0
// Returns -1 if an error occurs
int32_t xvCheckInputTileFree(xvTileManager *pxvTM, xvTile const *pTile);


// Prints the most recent error information.
// It returns the most recent error code.
xvError_t xvGetErrorInfo(xvTileManager const *pxvTM);

// Creates and initializes Tile Manager, Memory Allocator and iDMA.
// Returns XVTM_ERROR if it encounters an error, else returns XVTM_SUCCESS
// pxvTM - Tile Manager object
// buf0, buf1, buf2, buf3 - iDMA object. It should be initialized before calling this function. Contains descriptors and idma library object
// numMemBanks - Number of memory pools
// pBankBuffPool - Array of memory pool start address
// buffPoolSize - Array of memory pool sizes
// numDescs - Number of descriptors that can be added in buffer
// maxBlock - Maximum block size allowed
// maxPifReq - Maximum number of outstanding pif requests
// errCallbackFunc0 - Callback for dma transfer error for channel 0
// errCallbackFunc1 - Callback for dma transfer error for channel 1
// errCallbackFunc2 - Callback for dma transfer error for channel 2
// errCallbackFunc3 - Callback for dma transfer error for channel 3
// cbFunc0 - Callback for dma transfer completion for channel 0
// cbData0 - Data needed for completion callback function for channel 0
// cbFunc1 - Callback for dma transfer completion for channel 1
// cbData1 - Data needed for completion callback function for channel 1
// cbFunc2 - Callback for dma transfer completion for channel 2
// cbData2 - Data needed for completion callback function for channel 2
// cbFunc3 - Callback for dma transfer completion for channel 3
// cbData4 - Data needed for completion callback function for channel 3
// Returns XVTM_SUCCESS on successful initializations,
// else returns XVTM_ERROR in case of error
int32_t xvCreateTileManagerMultiChannel4CH(xvTileManager *pxvTM, void *buf0, void *buf1, void *buf2, void *buf3,
        int32_t numMemBanks, void **pBankBuffPool, int32_t *buffPoolSize,
        idma_err_callback_fn errCallbackFunc0, idma_err_callback_fn errCallbackFunc1,
        idma_err_callback_fn errCallbackFunc2, idma_err_callback_fn errCallbackFunc3,
        idma_callback_fn intrCallbackFunc0, void *cbData0,
        idma_callback_fn intrCallbackFunc1, void *cbData1,
        idma_callback_fn intrCallbackFunc2, void *cbData2,
        idma_callback_fn intrCallbackFunc3, void *cbData3,
        int32_t descCount, int32_t maxBlock, int32_t numOutReq);



#define xvCreateTileManagerMultiChannel(pxvTM, buf0, buf1, numMemBanks, pBankBuffPool, buffPoolSize,   \
                 errCallbackFunc0, errCallbackFunc1,                                                   \
                 intrCallbackFunc0, cbData0,                                                           \
                 intrCallbackFunc1, cbData1,                                                           \
                 descCount, maxBlock, numOutReq)                                                       \
                                                                                                       \
                 xvCreateTileManagerMultiChannel4CH(pxvTM, buf0, buf1, NULL,  NULL,                    \
                 numMemBanks, pBankBuffPool, buffPoolSize,                                             \
                 errCallbackFunc0, errCallbackFunc1,  NULL, NULL,                                      \
                 intrCallbackFunc0, cbData0, intrCallbackFunc1, cbData1,                               \
                 NULL,  NULL,  NULL,  NULL,                                                            \
                 descCount, maxBlock, numOutReq)





// Allocates single frame. It does not allocate buffer required for frame data.
// Initializes the frame elements
// pxvTM - Tile Manager object
// imgBuff - Pointer to image buffer
// frameBuffSize - Size of allocated image buffer
// width - Width of image
// height - Height of image
// pitch - Pitch of image
// pixRes - Pixel resolution of image in bytes
// numChannels - Number of channels in the image
// paddingtype - Supported padding type
// paddingVal - Padding value if padding type is edge extension
// Returns the pointer to allocated frame.
// Returns ((xvFrame *)(XVTM_ERROR)) if it encounters an error.
// Does not allocate frame data buffer.

xvFrame *xvCreateFrame(xvTileManager *pxvTM, uint64_t imgBuff, uint32_t frameBuffSize, int32_t width, int32_t height,
                       int32_t pitch, uint8_t pixRes, uint8_t numChannels, uint8_t paddingtype, uint32_t paddingVal);

// Allocates single tile and associated buffer data.
// Initializes the elements in tile
// pxvTM - Tile Manager object
// tileBuffSize - Size of allocated tile buffer
// width - Width of tile
// height - Height of tile
// pitch - Pitch of tile
// edgeWidth - Edge width of tile
// edgeHeight - Edge height of tile
// color - Memory pool from which the buffer should be allocated
// pFrame - Frame associated with the tile
// tileType - Type of tile
// alignType - Alignment tpye of tile. could be edge aligned of data aligned
// Returns the pointer to allocated tile.
// Returns ((xvTile *)(XVTM_ERROR)) if it encounters an error.
xvTile *xvCreateTile(xvTileManager *pxvTM, int32_t tileBuffSize, int32_t width, uint16_t height, int32_t pitch,
                     uint16_t edgeWidth, uint16_t edgeHeight, int32_t color, xvFrame *pFrame, uint16_t xvTileType, int32_t alignType);



// Waits till DMA transfer for given tile is completed.
// int32_t       ch                       DMA channel
// xvTileManager *pxvTM                   Tile Manager object
// xvTile        *pTile                   Input tile
// Returns ONE if dma transfer for input tile is complete and ZERO if it is not
// Returns XVTM_ERROR if an error occurs
int32_t xvWaitForTileFastMultiChannel(int32_t ch, xvTileManager const *pxvTM, xvTile  *pTile);

// Sleeps till DMA transfer for given tile is completed.
// INPUTS:
// int32_t       ch                       DMA channel
// TileManager *pxvTM                     Tile Manager object
// xvTile        *pTile                   Input tile
// Returns ONE if dma transfer for input tile is complete and ZERO if it is not
// Returns XVTM_ERROR if an error occurs
int32_t xvSleepForTileFastMultiChannel(int32_t ch, xvTileManager const *pxvTM, xvTile *pTile);


// Sleeps till DMA transfer for given DMA index is completed.
// INPUTS:
//     int32_t       ch                       iDMA channel
//     xvTileManager *pxvTM                   Tile Manager object
//     uint32_t      dmaIndex                 DMA index
//
// OUTPUTS:
//     Returns ONE if dma transfer for input tile is complete and ZERO if it is not
//     Returns XVTM_ERROR if an error occurs
int32_t xvSleepForiDMAMultiChannel(int32_t ch, xvTileManager const *pxvTM, uint32_t dmaIndex);


// Waits till DMA transfer for given DMA index is completed.
// INPUTS:
//     int32_t       ch                       iDMA channel
//     xvTileManager *pxvTM                   Tile Manager object
//     uint32_t      dmaIndex                 DMA index
//
// OUTPUTS:
//     Returns ONE if dma transfer for input tile is complete and ZERO if it is not
//     Returns XVTM_ERROR if an error occurs
int32_t xvWaitForiDMAMultiChannel(int32_t ch, xvTileManager const *pxvTM, uint32_t dmaIndex);

//     Waits till DMA transfer for given tile is completed.
//
// INPUTS:
//     int32_t       ch                       iDMA channel
//     xvTileManager *pxvTM                   Tile Manager object
//     xvTile        *pTile                   Input tile
//
// OUTPUTS:
//     Returns ONE if dma transfer for input tile is complete and ZERO if it is not
//     Returns XVTM_ERROR if an error occurs
int32_t xvWaitForTileMultiChannel(int32_t ch, xvTileManager *pxvTM, xvTile const *pTile);


//     Sleeps till DMA transfer for given tile is completed.
//
// INPUTS:
//     int32_t       ch                       iDMA channel
//     xvTileManager *pxvTM                   Tile Manager object
//     xvTile        *pTile                   Input tile
//
// OUTPUTS:
//     Returns ONE if dma transfer for input tile is complete and ZERO if it is not
//     Returns XVTM_ERROR if an error occurs
int32_t xvSleepForTileMultiChannel(int32_t ch, xvTileManager *pxvTM, xvTile const *pTile);

/*******************************************************
*   S I N G L E    C H A N N E L    S U P P O R T
*******************************************************/



#define xvInitIdma(pxvTM, buf0, buf1, numDescs, maxBlock, maxPifReq, errCallbackFunc, cbFunc, cbData) \
  xvInitIdmaMultiChannel(pxvTM, buf0, buf1, numDescs, maxBlock, maxPifReq, errCallbackFunc, NULL, cbFunc, cbData, NULL, NULL)

#define xvInitTileManager(pxvTM, buf0, buf1)  xvInitTileManagerMultiChannel(pxvTM, buf0, buf1)

#define xvAddIdmaRequest(pxvTM, dst, src, rowSize, numRows, srcPitch, dsPitch, interruptOnCompletion) \
  xvAddIdmaRequestMultiChannel(TM_IDMA_CH0, pxvTM, dst, src, rowSize, numRows, srcPitch, dsPitch, interruptOnCompletion)

#define xvCreateTileManager(pxvTM, buf0, numMemBanks, pBankBuffPool, buffPoolSize, errCallbackFunc, intrCallbackFunc, cbData, descCount, maxBlock, numOutReq) \
  xvCreateTileManagerMultiChannel(pxvTM, buf0, NULL, numMemBanks, pBankBuffPool, buffPoolSize, errCallbackFunc, NULL, intrCallbackFunc, cbData, NULL, NULL, descCount, maxBlock, numOutReq)

#define xvReqTileTransferIn(pxvTM, pTile, pPrevTile, interruptOnCompletion) \
  xvReqTileTransferInMultiChannel(TM_IDMA_CH0, pxvTM, pTile, pPrevTile, interruptOnCompletion)

#define xvReqTileTransferInFast(pxvTM, pTile, interruptOnCompletion) \
  xvReqTileTransferInFastMultiChannel(TM_IDMA_CH0, pxvTM, pTile, interruptOnCompletion)

#define xvReqTileTransferInFast16(pxvTM, pTile, interruptOnCompletion) \
  xvReqTileTransferInFast16MultiChannel(TM_IDMA_CH0, pxvTM, pTile, interruptOnCompletion)

#define xvReqTileTransferOut(pxvTM, pTile, interruptOnCompletion) \
  xvReqTileTransferOutMultiChannel(TM_IDMA_CH0, pxvTM, pTile, interruptOnCompletion)

#define xvReqTileTransferOutFast(pxvTM, pTile, interruptOnCompletion) \
  xvReqTileTransferOutFastMultiChannel(TM_IDMA_CH0, pxvTM, pTile, interruptOnCompletion)

#define xvReqTileTransferOutFast16(pxvTM, pTile, interruptOnCompletion) \
  xvReqTileTransferOutFast16MultiChannel(TM_IDMA_CH0, pxvTM, pTile, interruptOnCompletion)


#define  xvReqTileTransferInPredicated(pxvTM, pTile, interruptOnCompletion, pred_mask)\
  xvReqTileTransferInMultiChannelPredicated(TM_IDMA_CH0, pxvTM, pTile, interruptOnCompletion, pred_mask)

#define xvReqTileTransferOutPredicated(pxvTM, pTile, interruptOnCompletion, pred_mask)\
  xvReqTileTransferOutMultiChannelPredicated(TM_IDMA_CH0, pxvTM, pTile, interruptOnCompletion, pred_mask)


#define xvCheckForIdmaIndex(pxvTM, index) \
  xvCheckForIdmaIndexMultiChannel(TM_IDMA_CH0, pxvTM, index)

#define xvCheckTileReady(pxvTM, pTile) \
  xvCheckTileReadyMultiChannel(TM_IDMA_CH0, pxvTM, pTile)


#if defined(SUPPORT_3D_TILES)
/**********************************************************************************
   * FUNCTION: xvAllocateFrame3D()
   *
   * DESCRIPTION:
   *     Allocates single 3D frame. It does not allocate buffer required for 3Dframe data.
   *
   * INPUTS:
   *     xvTileManager *pxvTM      Tile Manager object
   *
   * OUTPUTS:
   *     Returns the pointer to allocated frame.
   *     Returns ((xvFrame *)(XVTM_ERROR)) if it encounters an error.
   *     Does not allocate frame data buffer.
   *
   ********************************************************************************** */
xvFrame3D *xvAllocateFrame3D(xvTileManager *pxvTM);

/**********************************************************************************
   * FUNCTION: xvFreeFrame3D()
   *
   * DESCRIPTION:
   *     Releases the given 3D frame. Does not release associated frame data buffer.
   *
   * INPUTS:
   *     xvTileManager *pxvTM      Tile Manager object
   *     xvFrame3D       *pFrame3D    3D Frame that needs to be released
   *
   * OUTPUTS:
   *     Returns XVTM_ERROR if it encounters an error, else returns XVTM_SUCCESS
   *
   ********************************************************************************** */

int32_t xvFreeFrame3D(xvTileManager *pxvTM, xvFrame3D const *pFrame3D);


/**********************************************************************************
   * FUNCTION: xvFreeAllFrames3D()
   *
   * DESCRIPTION:
   *     Releases all allocated 3D frames.
   *
   * INPUTS:
   *     xvTileManager *pxvTM      Tile Manager object
   *
   * OUTPUTS:
   *     Returns XVTM_ERROR if it encounters an error, else returns XVTM_SUCCESS
   *
   ********************************************************************************** */

int32_t xvFreeAllFrames3D(xvTileManager *pxvTM);


/**********************************************************************************
   * FUNCTION: xvAllocateTile3D()
   *
   * DESCRIPTION:
   *     Allocates single 3D tile. It does not allocate buffer required for tile data.
   *
   * INPUTS:
   *     xvTileManager *pxvTM      Tile Manager object
   *
   * OUTPUTS:
   *     Returns the pointer to allocated tile.
   *     Returns ((xvTile *)(XVTM_ERROR)) if it encounters an error.
   *     Does not allocate tile data buffer
   *
   ********************************************************************************** */
xvTile3D *xvAllocateTile3D(xvTileManager *pxvTM);



/**********************************************************************************
   * FUNCTION: xvFreeTile3D()
   *
   * DESCRIPTION:
   *     Releases the given 3D tile. Does not release associated tile data buffer.
   *
   * INPUTS:
   *     xvTileManager *pxvTM      Tile Manager object
   *     xvTile3D        *pTile3D      3D Tile that needs to be released
   *
   * OUTPUTS:
   *     Returns XVTM_ERROR if it encounters an error, else returns XVTM_SUCCESS
   *
   ********************************************************************************** */
int32_t xvFreeTile3D(xvTileManager *pxvTM, xvTile3D const *pTile3D);



/**********************************************************************************
   * FUNCTION: xvFreeAllTiles3D()
   *
   * DESCRIPTION:
   *     Releases all allocated 3D tiles.
   *
   * INPUTS:
   *     xvTileManager *pxvTM      Tile Manager object
   *
   * OUTPUTS:
   *     Returns XVTM_ERROR if it encounters an error, else returns XVTM_SUCCESS
   *
   ********************************************************************************** */

int32_t xvFreeAllTiles3D(xvTileManager *pxvTM);


/**********************************************************************************
   * FUNCTION: xvReqTileTransferInMultiChannel3D()
   *
   * DESCRIPTION:
   *     Requests data transfer from 3D frame present in system memory to local 3D tile memory.
   *
   *
   * INPUTS:
   *     int32_t        dmaChannel              DMA channel number
   *     xvTileManager *pxvTM                   Tile Manager object
   *     xvTile        *pTile                   Destination tile
   *     int32_t       interruptOnCompletion    If it is set, iDMA will interrupt after completing transfer
   *
   * OUTPUTS:
   *     Returns XVTM_ERROR if it encounters an error, else it returns XVTM_SUCCESS
   *
   ********************************************************************************** */

int32_t xvReqTileTransferInMultiChannel3D(int32_t dmaChannel, xvTileManager *pxvTM, xvTile3D *pTile3D,
        int32_t interruptOnCompletion);


/**********************************************************************************
   * FUNCTION: xvCheckTileReadyMultiChannel3D()
   *
   * DESCRIPTION:
   *     Checks if DMA transfer for given 3D tile is completed.
   *     It checks all the tile in the transfer request buffer
   *     before the given tile and updates their respective
   *     status. It pads edges wherever required.
   *
   * INPUTS:
   *     int32_t        dmaChannel              DMA channel number
   *     xvTileManager *pxvTM                   Tile Manager object
   *     xvTile3D        *pTile3D               Input tile
   *
   * OUTPUTS:
   *     Returns ONE if dma transfer for input tile is complete and ZERO if it is not
   *     Returns XVTM_ERROR if an error occurs
   *
   ********************************************************************************** */

int32_t xvCheckTileReadyMultiChannel3D(int32_t dmaChannel, xvTileManager *pxvTM, xvTile3D const *pTile3D);


/**********************************************************************************
   * FUNCTION: xvCreateFrame3D()
   *
   * DESCRIPTION:
   *     Allocates single 3D frame. It does not allocate buffer required for frame data.
   *     Initializes the frame elements
   *
   * INPUTS:
   *     xvTileManager *pxvTM          Tile Manager object
   *     void          *imgBuff        Pointer to image buffer
   *     uint32_t       frameBuffSize   Size of allocated image buffer
   *     int32_t       width           Width of image
   *     int32_t       height          Height of image
   *     int32_t       pitch           Pitch of image
   *     int32_t       depth           depth of image
   *     int32_t       Tile2Dpitch     2DTile pitch
   *     uint8_t       pixRes          Pixel resolution of image in bytes
   *     uint8_t       numChannels     Number of channels in the image
   *     uint8_t       paddingtype     Supported padding type
   *     uint32_t       paddingVal      Padding value if padding type is edge extension
   *
   * OUTPUTS:
   *     Returns the pointer to allocated 3D frame.
   *     Returns ((xvFrame *)(XVTM_ERROR)) if it encounters an error.
   *     Does not allocate frame data buffer.
   *
   ********************************************************************************** */

xvFrame3D *xvCreateFrame3D(xvTileManager *pxvTM, uint64_t imgBuff, uint32_t frameBuffSize, int32_t width,
                           int32_t height, int32_t pitch, int32_t depth,
                           int32_t Tile2Dpitch, uint8_t pixRes, uint8_t numChannels, uint8_t paddingType, uint32_t paddingVal);

/**********************************************************************************
 * FUNCTION: xvCreateTile3D()
 *
 * DESCRIPTION:
 *     Allocates single 3D tile and associated buffer data.
 *     Initializes the elements in tile
 *
 * INPUTS:
 *     xvTileManager *pxvTM          Tile Manager object
 *     int32_t       tileBuffSize    Size of allocated tile buffer
 *     int32_t       width           Width of tile
 *     uint16_t      height          Height of tile
 *     uint16_t      depth           depth of tile
 *     int32_t       pitch           row pitch of tile
 *     int32_t       pitch2D         Pitch of 2D-tile
 *     uint16_t      edgeWidth       Edge width of tile
 *     uint16_t      edgeHeight      Edge height of tile
 *     uint16_t      edgeHDepth      Edge depth of tile
 *     int32_t       color           Memory pool from which the buffer should be allocated
 *     xvFrame3D     *pFrame3D       3D Frame associated with the 3D tile
 *     uint16_t       tileType        Type of tile
 *     int32_t       alignType       Alignment tpye of tile. could be edge aligned of data aligned
 *
 * OUTPUTS:
 *     Returns the pointer to allocated 3D tile.
 *     Returns ((xvTile *)(XVTM_ERROR)) if it encounters an error.
 *
 ********************************************************************************** */
xvTile3D  *xvCreateTile3D(xvTileManager *pxvTM, int32_t tileBuffSize, int32_t width, uint16_t height, uint16_t depth,
                          int32_t pitch, int32_t pitch2D, uint16_t edgeWidth, uint16_t edgeHeight,
                          uint16_t edgeDepth, int32_t color, xvFrame3D *pFrame3D, uint16_t xvTileType, int32_t alignType);


/**********************************************************************************
   * FUNCTION: xvWaitForTileMultiChannel3D()
   *
   * DESCRIPTION:
   *     Waits till DMA transfer for given 3D tile is completed.
   *
   * INPUTS:
   *     int32_t       ch                       iDMA channel
   *     xvTileManager *pxvTM                   Tile Manager object
   *     xvTile        *pTile3D                 Input 3D tile
   *
   * OUTPUTS:
   *     Returns ONE if dma transfer for input tile is complete and ZERO if it is not
   *     Returns XVTM_ERROR if an error occurs
   *
   ********************************************************************************** */
int32_t xvWaitForTileMultiChannel3D(int32_t ch, xvTileManager *pxvTM, xvTile3D const *pTile3D);



int32_t addIdmaRequestInlineMultiChannel_wide3D(int32_t dmaChannel, xvTileManager *pxvTM, uint64_t pdst64,
        uint64_t psrc64, size_t rowSize,
        int32_t numRows, int32_t srcPitchBytes, int32_t dstPitchBytes, int32_t interruptOnCompletion, int32_t srcTilePitchBytes,
        int32_t dstTilePitchBytes, int32_t numTiles);



/**********************************************************************************
   * FUNCTION: xvReqTileTransferOutMultiChannel3D()
   *
   * DESCRIPTION:
   *     Requests data transfer from 3D tile present in local memory to 3D frame in system memory.
   *
   * INPUTS:
   *     int32_t        dmaChannel              DMA channel number
   *     xvTileManager *pxvTM                   Tile Manager object
   *     xvTile3D        *pTile3D               Source tile
   *     int32_t       interruptOnCompletion    If it is set, iDMA will interrupt after completing transfer
   *
   * OUTPUTS:
   *     Returns XVTM_ERROR if it encounters an error, else it returns XVTM_SUCCESS
   *
   ********************************************************************************** */
int32_t xvReqTileTransferOutMultiChannel3D(int32_t dmaChannel, xvTileManager *pxvTM, xvTile3D *pTile3D,
        int32_t interruptOnCompletion);



#define xvCheckTileReady3D(pxvTM, pTile3D)   xvCheckTileReadyMultiChannel3D(TM_IDMA_CH0, pxvTM, pTile3D)
#define xvReqTileTransferIn3D(pxvTM, pTile3D, interruptOnCompletion)    xvReqTileTransferInMultiChannel3D(TM_IDMA_CH0, pxvTM, pTile3D, interruptOnCompletion)

#endif



#if defined (__cplusplus)
}
#endif
#endif


