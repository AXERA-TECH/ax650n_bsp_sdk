/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_VDSP_TM_H__
#define __AX_VDSP_TM_H__

#include "tileManager.h"
/*
*Tile type define
*/
#define AX_VDSP_MAKETYPE                XV_MAKETYPE
#define AX_VDSP_CUSTOMTYPE              XV_CUSTOMTYPE

#define AX_VDSP_U8                      XV_U8
#define AX_VDSP_U16                     XV_U16
#define AX_VDSP_U32                     XV_U32

#define AX_VDSP_S8                      XV_S8
#define AX_VDSP_S16                     XV_S16
#define AX_VDSP_S32                     XV_S32

#define AX_VDSP_ARRAY_U8                XV_ARRAY_U8
#define AX_VDSP_ARRAY_S8                XV_ARRAY_S8
#define AX_VDSP_ARRAY_U16               XV_ARRAY_U16
#define AX_VDSP_ARRAY_S16               XV_ARRAY_S16
#define AX_VDSP_ARRAY_U32               XV_ARRAY_U32
#define AX_VDSP_ARRAY_S32               XV_ARRAY_S32

#define AX_VDSP_TILE_U8                 XV_TILE_U8
#define AX_VDSP_TILE_S8                 XV_TILE_S8
#define AX_VDSP_TILE_U16                XV_TILE_U16
#define AX_VDSP_TILE_S16                XV_TILE_S16
#define AX_VDSP_TILE_U32                XV_TILE_U32
#define AX_VDSP_TILE_S32                XV_TILE_S32

/*
 * Frame define
 */
#define AX_VDSP_FRAME_GET_BUFF_PTR          XV_FRAME_GET_BUFF_PTR
#define AX_VDSP_FRAME_SET_BUFF_PTR          XV_FRAME_SET_BUFF_PTR

#define AX_VDSP_FRAME_GET_BUFF_SIZE         XV_FRAME_GET_BUFF_SIZE
#define AX_VDSP_FRAME_SET_BUFF_SIZE         XV_FRAME_SET_BUFF_SIZE

#define AX_VDSP_FRAME_GET_DATA_PTR          XV_FRAME_GET_DATA_PTR
#define AX_VDSP_FRAME_SET_DATA_PTR          XV_FRAME_SET_DATA_PTR

#define AX_VDSP_FRAME_GET_WIDTH             XV_FRAME_GET_WIDTH
#define AX_VDSP_FRAME_SET_WIDTH             XV_FRAME_SET_WIDTH

#define AX_VDSP_FRAME_GET_HEIGHT            XV_FRAME_GET_HEIGHT
#define AX_VDSP_FRAME_SET_HEIGHT            XV_FRAME_SET_HEIGHT

#define AX_VDSP_FRAME_GET_PITCH             XV_FRAME_GET_PITCH
#define AX_VDSP_FRAME_SET_PITCH             XV_FRAME_SET_PITCH
#define AX_VDSP_FRAME_GET_PITCH_IN_BYTES    XV_FRAME_GET_PITCH_IN_BYTES

#define AX_VDSP_FRAME_GET_PIXEL_RES         XV_FRAME_GET_PIXEL_RES
#define AX_VDSP_FRAME_SET_PIXEL_RES         XV_FRAME_SET_PIXEL_RES

#define AX_VDSP_FRAME_GET_PIXEL_FORMAT      XV_FRAME_GET_PIXEL_FORMAT
#define AX_VDSP_FRAME_SET_PIXEL_FORMAT      XV_FRAME_SET_PIXEL_FORMAT

#define AX_VDSP_FRAME_GET_EDGE_WIDTH        XV_FRAME_GET_EDGE_WIDTH
#define AX_VDSP_FRAME_SET_EDGE_WIDTH        XV_FRAME_SET_EDGE_WIDTH

#define AX_VDSP_FRAME_GET_EDGE_HEIGHT       XV_FRAME_GET_EDGE_HEIGHT
#define AX_VDSP_FRAME_SET_EDGE_HEIGHT       XV_FRAME_SET_EDGE_HEIGHT

#define AX_VDSP_FRAME_GET_EDGE_LEFT         XV_FRAME_GET_EDGE_LEFT
#define AX_VDSP_FRAME_SET_EDGE_LEFT         XV_FRAME_SET_EDGE_LEFT

#define AX_VDSP_FRAME_GET_EDGE_RIGHT        XV_FRAME_GET_EDGE_RIGHT
#define AX_VDSP_FRAME_SET_EDGE_RIGHT        XV_FRAME_SET_EDGE_RIGHT

#define AX_VDSP_FRAME_GET_EDGE_TOP          XV_FRAME_GET_EDGE_TOP
#define AX_VDSP_FRAME_SET_EDGE_TOP          XV_FRAME_SET_EDGE_TOP

#define AX_VDSP_FRAME_GET_EDGE_BOTTOM       XV_FRAME_GET_EDGE_BOTTOM
#define AX_VDSP_FRAME_SET_EDGE_BOTTOM       XV_FRAME_SET_EDGE_BOTTOM

#define AX_VDSP_FRAME_GET_PADDING_TYPE      XV_FRAME_GET_PADDING_TYPE
#define AX_VDSP_FRAME_SET_PADDING_TYPE      XV_FRAME_SET_PADDING_TYPE

/*
 * Array Access defines
 */
#define AX_VDSP_ARRAY_GET_BUFF_PTR(pArray)              XV_ARRAY_GET_BUFF_PTR
#define AX_VDSP_ARRAY_SET_BUFF_PTR(pArray, pBuff)       XV_ARRAY_SET_BUFF_PTR

#define AX_VDSP_ARRAY_GET_BUFF_SIZE(pArray)             XV_ARRAY_GET_BUFF_SIZE
#define AX_VDSP_ARRAY_SET_BUFF_SIZE(pArray, buffSize)   XV_ARRAY_SET_BUFF_SIZE

#define AX_VDSP_ARRAY_GET_DATA_PTR(pArray)              ((pArray)->pData)//XV_ARRAY_GET_DATA_PTR
#define AX_VDSP_ARRAY_SET_DATA_PTR(pArray, pArrayData)  XV_ARRAY_SET_DATA_PTR

#define AX_VDSP_ARRAY_GET_WIDTH(pArray)                 XV_ARRAY_GET_WIDTH
#define AX_VDSP_ARRAY_SET_WIDTH(pArray, value)          XV_ARRAY_SET_WIDTH

#define AX_VDSP_ARRAY_GET_PITCH(pArray)                 XV_ARRAY_GET_PITCH
//#define AX_VDSP_ARRAY_GET_PITCH(pArray)                 ((pArray)->pitch)

#define AX_VDSP_ARRAY_SET_PITCH(pArray, value)          XV_ARRAY_SET_PITCH

#define AX_VDSP_ARRAY_GET_HEIGHT(pArray)                XV_ARRAY_GET_HEIGHT
#define AX_VDSP_ARRAY_SET_HEIGHT(pArray, value)         XV_ARRAY_SET_HEIGHT

#define AX_VDSP_ARRAY_GET_STATUS_FLAGS(pArray)          XV_ARRAY_GET_STATUS_FLAGS
#define AX_VDSP_ARRAY_SET_STATUS_FLAGS(pArray, value)   XV_ARRAY_SET_STATUS_FLAGS

#define AX_VDSP_ARRAY_GET_TYPE(pArray)                  XV_ARRAY_GET_TYPE
#define AX_VDSP_ARRAY_SET_TYPE(pArray, value)           XV_ARRAY_SET_TYPE

#define AX_VDSP_ARRAY_GET_CAPACITY(pArray)              XV_ARRAY_GET_CAPACITY
#define AX_VDSP_ARRAY_SET_CAPACITY(pArray, value)       XV_ARRAY_SET_CAPACITY

#define AX_VDSP_ARRAY_GET_ELEMENT_TYPE(pArray)          XV_ARRAY_GET_ELEMENT_TYPE
#define AX_VDSP_ARRAY_GET_ELEMENT_SIZE(pArray)          XV_ARRAY_GET_ELEMENT_SIZE
#define AX_VDSP_ARRAY_IS_TILE(pArray)                   XV_ARRAY_IS_TILE
#define AX_VDSP_ARRAY_GET_AREA(pArray)                  XV_ARRAY_GET_AREA

/*
*Tile define
*/
#define AX_VDSP_TILE_GET_BUFF_PTR           XV_TILE_GET_BUFF_PTR
#define AX_VDSP_TILE_SET_BUFF_PTR           XV_TILE_SET_BUFF_PTR

#define AX_VDSP_TILE_GET_BUFF_SIZE          XV_TILE_GET_BUFF_SIZE
#define AX_VDSP_TILE_SET_BUFF_SIZE          XV_TILE_SET_BUFF_SIZE

#define AX_VDSP_TILE_GET_DATA_PTR           XV_TILE_GET_DATA_PTR
#define AX_VDSP_TILE_SET_DATA_PTR           XV_TILE_SET_DATA_PTR

#define AX_VDSP_TILE_GET_WIDTH              XV_TILE_GET_WIDTH
#define AX_VDSP_TILE_SET_WIDTH              XV_TILE_SET_WIDTH

#define AX_VDSP_TILE_GET_PITCH              XV_TILE_GET_PITCH
#define AX_VDSP_TILE_SET_PITCH              XV_TILE_SET_PITCH

#define AX_VDSP_TILE_GET_HEIGHT             XV_TILE_GET_HEIGHT
#define AX_VDSP_TILE_SET_HEIGHT             XV_TILE_SET_HEIGHT

#define AX_VDSP_TILE_GET_STATUS_FLAGS       XV_TILE_GET_STATUS_FLAGS
#define AX_VDSP_TILE_SET_STATUS_FLAGS       XV_TILE_SET_STATUS_FLAGS

#define AX_VDSP_TILE_GET_TYPE               XV_TILE_GET_TYPE
#define AX_VDSP_TILE_SET_TYPE               XV_TILE_SET_TYPE

#define AX_VDSP_TILE_GET_ELEMENT_TYPE       XV_TILE_GET_ELEMENT_TYPE
#define AX_VDSP_TILE_GET_ELEMENT_SIZE       XV_TILE_GET_ELEMENT_SIZE
#define AX_VDSP_TILE_IS_TILE                XV_TILE_IS_TILE

#define AX_VDSP_TYPE_ELEMENT_SIZE           XV_TYPE_ELEMENT_SIZE
#define AX_VDSP_TYPE_ELEMENT_TYPE           XV_TYPE_ELEMENT_TYPE
#define AX_VDSP_TYPE_IS_TILE                XV_TYPE_IS_TILE
#define AX_VDSP_TYPE_IS_SIGNED              XV_TYPE_IS_SIGNED

#define AX_VDSP_TILE_GET_FRAME_PTR          XV_TILE_GET_FRAME_PTR
#define AX_VDSP_TILE_SET_FRAME_PTR          XV_TILE_SET_FRAME_PTR

#define AX_VDSP_TILE_GET_X_COORD            XV_TILE_GET_X_COORD
#define AX_VDSP_TILE_SET_X_COORD            XV_TILE_SET_X_COORD

#define AX_VDSP_TILE_GET_Y_COORD            XV_TILE_GET_Y_COORD
#define AX_VDSP_TILE_SET_Y_COORD            XV_TILE_SET_Y_COORD

#define AX_VDSP_TILE_GET_EDGE_WIDTH         XV_TILE_GET_EDGE_WIDTH
#define AX_VDSP_TILE_SET_EDGE_WIDTH         XV_TILE_SET_EDGE_WIDTH

#define AX_VDSP_TILE_GET_EDGE_HEIGHT        XV_TILE_GET_EDGE_HEIGHT
#define AX_VDSP_TILE_SET_EDGE_HEIGHT        XV_TILE_SET_EDGE_HEIGHT

#define AX_VDSP_TILE_GET_EDGE_LEFT          XV_TILE_GET_EDGE_LEFT
#define AX_VDSP_TILE_SET_EDGE_LEFT          XV_TILE_SET_EDGE_LEFT

#define AX_VDSP_TILE_GET_EDGE_RIGHT         XV_TILE_GET_EDGE_RIGHT
#define AX_VDSP_TILE_SET_EDGE_RIGHT         XV_TILE_SET_EDGE_RIGHT

#define AX_VDSP_TILE_GET_EDGE_TOP           XV_TILE_GET_EDGE_TOP
#define AX_VDSP_TILE_SET_EDGE_TOP           XV_TILE_SET_EDGE_TOP

#define AX_VDSP_TILE_GET_EDGE_BOTTOM        XV_TILE_GET_EDGE_BOTTOM
#define AX_VDSP_TILE_SET_EDGE_BOTTOM        XV_TILE_SET_EDGE_BOTTOM

/*
 * Set up tile by type
 */
#define AX_VDSP_SETUP_TILE SETUP_TILE
/*
 * Pixel pack format
 */
typedef enum {
    AX_VDSP_PIXEL_PACK_FORMAT_ONE    = 0x1,  /*SINGLE_COMPONENT_PLANAR*/
    AX_VDSP_PIXEL_PACK_FORMAT_TWO    = 0x2,  /*TWO_COMPONENT_PACKED*/
    AX_VDSP_PIXEL_PACK_FORMAT_THREE  = 0x3,  /*THREE_COMPONENT_PACKED*/
    AX_VDSP_PIXEL_PACK_FORMAT_FOUR   = 0x4,  /*FOUR_COMPONENT_PACKED*/

    AX_VDSP_PIXEL_PACK_FORMAT_BUTT
} AX_VDSP_PIXEL_PACK_FORMAT_E;

/*
 *Padding Type
 */
typedef enum {
    AX_VDSP_PADDING_TYPE_ZERO    = 0x0,
    AX_VDSP_PADDING_TYPE_EDGE    = 0x1,
    AX_VDSP_PADDING_TYPE_BUTT
} AX_VDSP_PADDING_TYPE_E;

/**
 *Frame
 */
typedef xvFrame AX_VDSP_FRAME_S;
typedef xvFrame AX_VDSP_SRC_FRAME_S;
typedef xvFrame AX_VDSP_DST_FRAME_S;
typedef AX_VDSP_PIXEL_PACK_FORMAT_E  pixelPackFormat_t;

typedef xvArray AX_VDSP_ARRAY_S;
typedef xvTile  AX_VDSP_TILE_S;
typedef xvpTile  AX_VDSP_PTILE_S;
typedef AX_VDSP_ARRAY_S AX_VDSP_SRC_ARRAY_S;
typedef AX_VDSP_ARRAY_S AX_VDSP_DST_ARRAY_S;
typedef AX_VDSP_TILE_S AX_VDSP_SRC_TILE_S;
typedef AX_VDSP_TILE_S AX_VDSP_DST_TILE_S;


#define AX_VDSP_MOVE_X_TO_Y(indX,indY,tileWidth,tileHeight,imageWidth,imageHeight)            \
{                                                                                         \
    (indX)  += (tileWidth);                                                               \
    if (((indX) >= (imageWidth)) || (((indX) + (tileWidth)) > imageWidth))            \
    {                                                                                     \
        (indY) += (tileHeight);                                                           \
        (indX)  = 0;                                                                      \
        if (((indY) >= (imageHeight)) || (((indY) + (tileHeight)) > (imageHeight)))   \
        {                                                                                 \
            (indY) = 0;                                                                   \
        }                                                                                 \
    }                                                                                     \
}

#define AX_VDSP_MOVE(start,len) (start) += (len)

/*****************************************************************************
*   Prototype    : AX_VDSP_TM_Init
*   Description  : DSP Tile manager init.
*   Parameters   : AX_VOID pBuffPools buffer pointers
*                : AX_S32 pBuffSizes buffer size pointers
*
*
*   Return Value : AX_SUCCESS: Success;Error codes: Failure.
*   Spec         :
*
*
*   History:
*
*       1.  Date         : 2021-06-02
*           Author       :
*           Modification : Created function
*
*****************************************************************************/
AX_S32 AX_VDSP_TM_Init(AX_S32 *pBuffPools, AX_S32 *pBuffSizes);

/*****************************************************************************
*   Prototype    : AX_VDSP_TM_Exit
*   Description  : DSP Tile manager uninit.
*   Parameters   :
*
*
*   Return Value : AX_SUCCESS: Success;Error codes: Failure.
*   Spec         :
*
*
*   History:
*
*       1.  Date         : 2021-06-02
*           Author       :
*           Modification : Created function
*
*****************************************************************************/
AX_S32 AX_VDSP_TM_Exit(AX_VOID);
/*****************************************************************************
*   Prototype    : AX_VDSP_CreateFrames
*   Description  : Allocates frames. It does not allocate buffer required for frames data.
*   Parameters   :
*
*
*   Return Value : AX_SUCCESS: Success;Error codes: Failure.
*   Spec         :
*
*
*   History:
*
*       1.  Date         : 2021-06-02
*           Author       :
*           Modification : Created function
*
*****************************************************************************/
AX_S32 AX_VDSP_CreateFrames(AX_VDSP_DST_FRAME_S **ppstFrm, AX_U32 u32Num, uint64_t imgBuff, uint32_t frameBuffSize,
                            int32_t width, \
                            int32_t height, int32_t pitch, uint8_t pixRes, uint8_t numChannels, uint8_t paddingtype, uint32_t paddingVal);

/*****************************************************************************
*   Prototype    : AX_VDSP_AllocateFrames
*   Description  : DSP Allocate frames.
*   Parameters   : AX_VDSP_DST_FRAME_S      **ppstFrm   Frame
*                  AX_U32                   u32Num      Number
*
*   Return Value : AX_SUCCESS: Success;Error codes: Failure.
*   Spec         :
*
*
*   History:
*
*       1.  Date         : 2021-06-02
*           Author       :
*           Modification : Created function
*
*****************************************************************************/
AX_S32 AX_VDSP_AllocateFrames(AX_VDSP_DST_FRAME_S **ppstFrm, AX_U32 u32Num);

/*****************************************************************************
*   Prototype    : AX_VDSP_FreeFrames
*   Description  : DSP Free frames.
*   Parameters   : AX_VDSP_FRAME_S      **ppstFrm   Frame
*                  AX_U32                   u32Num      Number
*
*   Return Value : AX_SUCCESS: Success;Error codes: Failure.
*   Spec         :
*
*
*   History:
*
*       1.  Date         : 2021-06-02
*           Author       :
*           Modification : Created function
*
*****************************************************************************/
AX_S32 AX_VDSP_FreeFrames(AX_VDSP_FRAME_S **ppstFrm, AX_U32 u32Num);

/*****************************************************************************
*   Prototype    : AX_VDSP_FreeAllFrames
*   Description  : DSP Free all frames.
*   Parameters   :
*
*
*   Return Value : AX_SUCCESS: Success;Error codes: Failure.
*   Spec         :
*
*
*   History:
*
*       1.  Date         : 2021-06-02
*           Author       :
*           Modification : Created function
*
*****************************************************************************/
AX_S32 AX_VDSP_FreeAllFrames(AX_VOID);

/*****************************************************************************
*   Prototype    : AX_VDSP_AllocateBuffers
*   Description  : DSP Allocate buffers.
*   Parameters   : AX_VOID        **ppvBuff             Buffer
*                  AX_U32         u32Num                Buffer number
*                  AX_S32         s32BuffSize        Buffer size
*                  AX_S32         s32BuffColor          Bank
*                  AX_S32         s32BuffAlign  align
*
*   Return Value : AX_SUCCESS: Success;Error codes: Failure.
*   Spec         :
*
*
*   History:
*
*       1.  Date         : 2021-06-02
*           Author       :
*           Modification : Created function
*
*****************************************************************************/
AX_S32 AX_VDSP_AllocateBuffers(AX_VOID **ppvBuff, AX_U32 u32Num, AX_S32 s32BuffSize,
                               AX_S32 s32BuffColor, AX_S32 s32BuffAlign);

/*****************************************************************************
*   Prototype    : AX_VDSP_FreeBuffers
*   Description  : DSP Free buffers.
*   Parameters   : AX_VOID            **ppvBuff       Buffer
*                  AX_U32             u32Num          Buffer number
*
*   Return Value : AX_SUCCESS: Success;Error codes: Failure.
*   Spec         :
*
*
*   History:
*
*       1.  Date         : 2021-06-02
*           Author       :
*           Modification : Created function
*
*****************************************************************************/
AX_S32 AX_VDSP_FreeBuffers(AX_VOID **ppvBuff, AX_U32 u32Num);

/*****************************************************************************
*   Prototype    : AX_VDSP_AllocateTiles
*   Description  : DSP Allocate tiles.
*   Parameters   : AX_VDSP_SRC_TILE_S      **ppstTile   Tile
*                  AX_U32                  u32Num       Tile number
*
*   Return Value : AX_SUCCESS: Success;Error codes: Failure.
*   Spec         :
*
*
*   History:
*
*       1.  Date         : 2021-06-02
*           Author       :
*           Modification : Created function
*
*****************************************************************************/
AX_S32 AX_VDSP_AllocateTiles(AX_VDSP_SRC_TILE_S **ppstTile, AX_U32 u32Num);

/*****************************************************************************
*   Prototype    : AX_VDSP_FreeTiles
*   Description  : DSP Free tiles.
*   Parameters   : AX_VDSP_TILE_S      **ppstTile   Tile
*                  AX_U32              u32Num       Tile number
*
*   Return Value : AX_SUCCESS: Success;Error codes: Failure.
*   Spec         :
*
*
*   History:
*
*       1.  Date         : 2021-06-02
*           Author       :
*           Modification : Created function
*
*****************************************************************************/
AX_S32 AX_VDSP_FreeTiles(AX_VDSP_TILE_S **ppstTile, AX_U32 u32Num);

/*****************************************************************************
*   Prototype    : AX_VDSP_FreeAllTiles
*   Description  : DSP Free all tiles.
*   Parameters   :
*
*
*   Return Value : AX_SUCCESS: Success;Error codes: Failure.
*   Spec         :
*
*
*   History:
*
*       1.  Date         : 2021-06-02
*           Author       :
*           Modification : Created function
*
*****************************************************************************/
AX_S32 AX_VDSP_FreeAllTiles(AX_VOID);

/*****************************************************************************
*   Prototype    : AX_VDSP_CopyDataByIdma
*   Description  : DSP Copy data by idma.
*   Parameters   : AX_VOID *          pdst64   Dst address pointer
*                  AX_VOID *          psrc64   Src address pointer
*                  AX_S32           s32Size                     Size
*                  AX_S32           s32IntOnCompletion         Flag
*
*   Return Value : AX_SUCCESS: Success;Error codes: Failure.
*   Spec         :
*
*
*   History:
*
*       1.  Date         : 2021-06-02
*           Author       :
*           Modification : Created function
*
*****************************************************************************/
AX_S32 AX_VDSP_CopyDataByIdma(AX_VOID *pdst64, AX_VOID *psrc64, AX_S32 s32Size, AX_S32 s32IntOnCompletion);



/*****************************************************************************
*   Prototype    : AX_VDSP_ReqTileTransferIn
*   Description  : DSP Request tile transfer from system memory to local memory
*   Parameters   : AX_VDSP_SRC_TILE_S     *pstTile                 Current tile
*                  AX_VDSP_SRC_TILE_S     *pstPrevTile             Prev tile
*                  AX_S32                 s32IntOnCompletion       Flag
*
*   Return Value : AX_SUCCESS: Success;Error codes: Failure.
*   Spec         :
*
*
*   History:
*
*       1.  Date         : 2021-06-02
*           Author       :
*           Modification : Created function
*
*****************************************************************************/
AX_S32 AX_VDSP_ReqTileTransferIn(AX_VDSP_SRC_TILE_S *pstTile, AX_VDSP_SRC_TILE_S *pstPrevTile,
                                 AX_S32 s32IntOnCompletion);

/*****************************************************************************
*   Prototype    : AX_VDSP_ReqTileTransferOut
*   Description  : DSP Request tile transfer from local memory to system memory
*   Parameters   : AX_VDSP_SRC_TILE_S     *pstTile                 Tile
*                  AX_S32                 s32IntOnCompletion       Flag
*
*   Return Value : AX_SUCCESS: Success;Error codes: Failure.
*   Spec         :
*
*
*   History:
*
*       1.  Date         : 2021-06-02
*           Author       :
*           Modification : Created function
*
*****************************************************************************/
AX_S32 AX_VDSP_ReqTileTransferOut(AX_VDSP_SRC_TILE_S *pstTile, AX_S32 s32IntOnCompletion);

/*****************************************************************************
*   Prototype    : AX_VDSP_CheckTileReady
*   Description  : DSP Check tile is finish
*   Parameters   : AX_VDSP_SRC_TILE_S     *pstTile                 Tile
*
*
*   Return Value : AX_SUCCESS: Success;Error codes: Failure.
*   Spec         :
*
*
*   History:
*
*       1.  Date         : 2021-06-02
*           Author       :
*           Modification : Created function
*
*****************************************************************************/
AX_S32 AX_VDSP_CheckTileReady(AX_VDSP_SRC_TILE_S *pstTile);

/*****************************************************************************
*   Prototype    : AX_VDSP_GetErrorInfo
*   Description  : DSP Get error info
*   Parameters   :
*
*
*   Return Value : AX_CHAR*
*   Spec         :
*
*
*   History:
*
*       1.  Date         : 2021-06-02
*           Author       :
*           Modification : Created function
*
*****************************************************************************/
AX_CHAR *AX_VDSP_GetErrorInfo(AX_VOID);

/*****************************************************************************
*   Prototype    : AX_VDSP_WaitForTile
*   Description  : DSP Wait for tile
*   Parameters   : AX_VDSP_SRC_TILE_S     *pstTile                 Tile
*
*
*   Return Value : AX_VOID
*   Spec         :
*
*
*   History:
*
*       1.  Date         : 2021-06-02
*           Author       :
*           Modification : Created function
*
*****************************************************************************/
AX_VOID AX_VDSP_WaitForTile(AX_VDSP_SRC_TILE_S *pstTile);
/*****************************************************************************
*   Prototype    : AX_VDSP_CopyData
*   Description  : Transfer data between external and local memories.
*   Parameters   : AX_U64 src          Input source data address.
*                  AX_U64 dst          Output destination address.
*                  AX_S32 s32Size         Data size
*
*   Return Value : AX_SUCCESS: Success;Error codes: .
*   Spec         :
*   History:
*
*       1.  Date         : 2021-06-02
*           Author       :
*           Modification : Created function
*
*****************************************************************************/
AX_S32 AX_VDSP_CopyData(AX_U64 dst, AX_U64 src, AX_S32 s32Size);
/*****************************************************************************
*   Prototype    : AX_VDSP_CopyDataWideAddr
*   Description  : DSP Copy data by idma.
*   Parameters   : AX_U64          dst                     Dst
*                  AX_U64          src                     Src
*                  AX_S32           s32Size                 size
*                  AX_S32           intOnCompletion         Flag
*
*   Return Value : AX_SUCCESS: Success;Error codes: Failure.
*   Spec         :
*
*
*   History:
*
*       1.  Date         : 2021-07-12
*           Author       :
*           Modification : Created function
*
*****************************************************************************/
AX_S32 AX_VDSP_CopyDataWideAddr(AX_U32 dmaChn, AX_U64 dst, AX_U64 src, AX_S32 s32Size, AX_U32 intOnCompletion);
/*****************************************************************************
*   Prototype    : AX_VDSP_CopyDataWideAddr
*   Description  : DSP Copy data by idma.
*   Parameters   : dmaChn          DMA channel number
*                  AX_U64          dst                     Dst
*                  AX_U64          src                     Src
*                  AX_U32          rowSize
*                  AX_U32          numRows
*                  AX_U32          srcPitchBytes
*                  AX_U32          dstPitchBytes
*                  AX_S32          intOnCompletion         Flag
*
*   Return Value : AX_SUCCESS: Success;Error codes: Failure.
*   Spec         :
*
*
*   History:
*
*       1.  Date         : 2021-07-12
*           Author       :
*           Modification : Created function
*
*****************************************************************************/
AX_S32 AX_VDSP_Copy2DAddr(AX_U32 dmaChn, AX_U64 dst, AX_U64 src, AX_U32 rowSize, AX_U32 numRows, AX_U32 srcPitchBytes,
                          AX_U32 dstPitchBytes, AX_U32 intOnCompletion);
/**********************************************************************************
 * FUNCTION: AX_VDSP_CheckForIdmaIndex()
 *
 * DESCRIPTION:
 *     Checks if DMA transfer for given index is completed
 *
 * INPUTS:
 *     AX_S32       dmaChn              DMA channel number
 *     AX_S32       idmaIndex           Index of the dma transfer request
 *
 * OUTPUTS:
 *     Returns 1 if transfer is complete and 0 if it is not
 *     Returns AX_FAILURE if an error occurs
 *   History:
 *
 *       1.  Date         : 2021-07-12
 *           Author       :
 *           Modification : Created function
 *
 ********************************************************************************** */
AX_S32 AX_VDSP_CheckForIdmaIndex(AX_U32 dmaChn, AX_S32 idmaIndex);
AX_S32 AX_VDSP_TM_Getfreememsize(AX_S32 index);
#endif /* __AX_VDSP_TM_H__ */

