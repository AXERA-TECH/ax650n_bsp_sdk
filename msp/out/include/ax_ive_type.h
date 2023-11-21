/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_IVE_TYPE_H_
#define _AX_IVE_TYPE_H_

#include "ax_base_type.h"
#include "ax_global_type.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifndef AX_FAILURE
#define AX_FAILURE (-1)
#endif

#ifndef AX_SUCCESS
#define AX_SUCCESS 0
#endif


/*
* The fixed-point data type, will be used to
* represent float data in hardware calculations.
*/

/* u8bit */
typedef unsigned char AX_U1Q7;
typedef unsigned char AX_U4Q4;

/* u16bit */
typedef unsigned short AX_U1Q10;
typedef unsigned short AX_U1Q15;

/* u32bit */
typedef unsigned int AX_U0Q20;
typedef unsigned int AX_U14Q4;

/* s16bit */
typedef short AX_S1Q7;
typedef short AX_S1Q14;

/* s32bit */
typedef int AX_S6Q10;

/*
* IVE handle
*/
typedef AX_S32 AX_IVE_HANDLE;

#define AX_IVE_MAX_REGION_NUM  2048
#define AX_IVE_HIST_NUM        256

/* Type of the AX_IVE_IMAGE_T */
typedef enum axIVE_IMAGE_TYPE_E {
    AX_IVE_IMAGE_TYPE_U8C1 = 0x0,
    AX_IVE_IMAGE_TYPE_S8C1 = 0x1,

    AX_IVE_IMAGE_TYPE_YUV420SP = 0x2, /* YUV420 SemiPlanar */
    AX_IVE_IMAGE_TYPE_YUV422SP = 0x3, /* YUV422 SemiPlanar */
    AX_IVE_IMAGE_TYPE_YUV420P = 0x4,  /* YUV420 Planar */
    AX_IVE_IMAGE_TYPE_YUV422P = 0x5,  /* YUV422 planar */

    AX_IVE_IMAGE_TYPE_S8C2_PACKAGE = 0x6,
    AX_IVE_IMAGE_TYPE_S8C2_PLANAR = 0x7,

    AX_IVE_IMAGE_TYPE_S16C1 = 0x8,
    AX_IVE_IMAGE_TYPE_U16C1 = 0x9,

    AX_IVE_IMAGE_TYPE_U8C3_PACKAGE = 0xa,
    AX_IVE_IMAGE_TYPE_U8C3_PLANAR = 0xb,

    AX_IVE_IMAGE_TYPE_S32C1 = 0xc,
    AX_IVE_IMAGE_TYPE_U32C1 = 0xd,

    AX_IVE_IMAGE_TYPE_S64C1 = 0xe,
    AX_IVE_IMAGE_TYPE_U64C1 = 0xf,

    AX_IVE_IMAGE_TYPE_BUTT

} AX_IVE_IMAGE_TYPE_E;

/* Definition of the AX_IVE_IMAGE_T */
typedef struct axIVE_IMAGE_T {
    AX_U64 au64PhyAddr[3];   /* RW;The physical address of the image */
    AX_U64 au64VirAddr[3];   /* RW;The virtual address of the image */
    AX_U32 au32Stride[3];    /* RW;The stride of the image */
    AX_U32 u32Width;         /* RW;The width of the image */
    AX_U32 u32Height;        /* RW;The height of the image */
    union {
        AX_IVE_IMAGE_TYPE_E enType; /* RW;The type of the image */
        AX_IMG_FORMAT_E enGlbType; /* RW;The type of the global image */
    };
} AX_IVE_IMAGE_T;

typedef AX_IVE_IMAGE_T AX_IVE_SRC_IMAGE_T;
typedef AX_IVE_IMAGE_T AX_IVE_DST_IMAGE_T;

/*
* Definition of the AX_IVE_MEM_INFO_T.
* This struct special purpose for input or ouput, such as Hist, CCL.
*/
typedef struct axIVE_MEM_INFO_T {
    AX_U64 u64PhyAddr; /* RW;The physical address of the memory */
    AX_U64 u64VirAddr; /* RW;The virtual address of the memory */
    AX_U32 u32Size;    /* RW;The size of memory */
} AX_IVE_MEM_INFO_T;
typedef AX_IVE_MEM_INFO_T AX_IVE_SRC_MEM_INFO_T;
typedef AX_IVE_MEM_INFO_T AX_IVE_DST_MEM_INFO_T;

/* Data struct */
typedef struct axIVE_DATA_T {
    AX_U64 u64PhyAddr; /* RW;The physical address of the data */
    AX_U64 u64VirAddr; /* RW;The virtaul address of the data */
    AX_U32 u32Stride; /* RW;The stride of 2D data by byte */
    AX_U32 u32Width;  /* RW;The width of 2D data by byte */
    AX_U32 u32Height; /* RW;The height of 2D data by byte */

    AX_U32 u32Reserved;
} AX_IVE_DATA_T;

typedef AX_IVE_DATA_T AX_IVE_SRC_DATA_T;
typedef AX_IVE_DATA_T AX_IVE_DST_DATA_T;

/*
* Hardware Engine
*/
typedef enum axIVE_HW_ENGINE_E {
    AX_IVE_ENGINE_IVE = 0,
    AX_IVE_ENGINE_TDP,
    AX_IVE_ENGINE_VGP,
    AX_IVE_ENGINE_VPP,
    AX_IVE_ENGINE_GDC,
    AX_IVE_ENGINE_DSP,
    AX_IVE_ENGINE_NPU,
    AX_IVE_ENGINE_CPU,
    AX_IVE_ENGINE_MAU,
    AX_IVE_ENGINE_BUTT
} AX_IVE_ENGINE_E;

/*
* Definition of s16 point
*/
typedef struct axIVE_POINT_S16_T {
    AX_S16 s16X; /* RW;The X coordinate of the point */
    AX_S16 s16Y; /* RW;The Y coordinate of the point */
} AX_IVE_POINT_S16_T;

/* Definition of rectangle */
typedef struct axIVE_RECT_U16_T {
    AX_U16 u16X;      /* RW;The location of X axis of the rectangle */
    AX_U16 u16Y;      /* RW;The location of Y axis of the rectangle */
    AX_U16 u16Width;  /* RW;The width of the rectangle */
    AX_U16 u16Height; /* RW;The height of the rectangle */
} AX_IVE_RECT_U16_T;

typedef enum axIVE_ERR_CODE_E {
    AX_ERR_IVE_OPEN_FAILED = 0x50,     /* IVE open device failed */
    AX_ERR_IVE_INIT_FAILED = 0x51,     /* IVE init failed  */
    AX_ERR_IVE_NOT_INIT = 0x52,        /* IVE not init error */
    AX_ERR_IVE_SYS_TIMEOUT = 0x53,     /* IVE process timeout */
    AX_ERR_IVE_QUERY_TIMEOUT = 0x54,   /* IVE query timeout */
    AX_ERR_IVE_BUS_ERR = 0x55,         /* IVE bus error */

} AX_IVE_ERR_CODE_E;

/* MAU error code */
typedef enum axIVE_MAU_ERR_CODE_E {
    AX_ERR_MAU_CREATE_HANDLE_ERROR = 0x3,   /* Create handle error */
    AX_ERR_MAU_DESTROY_HANDLE_ERROR = 0x4,  /* Destory handle error */
    AX_ERR_MAU_MATMUL_ERROR = 0x5,          /* Matrix multiplication operation error */
    AX_ERR_MAU_DRIVER_ERROR = 0x6,          /* MAU driver error */
    AX_ERR_MAU_TILE_ERROR  = 0x7,           /* Tile error, only for NPU engine */

} AX_IVE_MAU_ERR_CODE_E;

#define AX_ID_IVE_SMOD  0x00
#define AX_ID_MAU_SMOD  0x01
/************************************************IVE error code ***********************************/
/* Invalid device ID */
#define AX_ERR_IVE_INVALID_DEVID AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_INVALID_MODID)
/* Invalid channel ID */
#define AX_ERR_IVE_INVALID_CHNID AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_INVALID_CHNID)
/* At least one parameter is illegal. For example, an illegal enumeration value exists. */
#define AX_ERR_IVE_ILLEGAL_PARAM AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_ILLEGAL_PARAM)
/* The channel exists. */
#define AX_ERR_IVE_EXIST AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_EXIST)
/* The UN exists. */
#define AX_ERR_IVE_UNEXIST AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_UNEXIST)
/* A null point is used. */
#define AX_ERR_IVE_NULL_PTR AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_NULL_PTR)
/* Try to enable or initialize the system, device, or channel before configuring attributes. */
#define AX_ERR_IVE_NOT_CONFIG AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_NOT_CONFIG)
/* The operation is not supported currently. */
#define AX_ERR_IVE_NOT_SURPPORT AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_NOT_SUPPORT)
/* The operation, changing static attributes for example, is not permitted. */
#define AX_ERR_IVE_NOT_PERM AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_NOT_PERM)
/* A failure caused by the malloc memory occurs. */
#define AX_ERR_IVE_NOMEM AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_NOMEM)
/* A failure caused by the malloc buffer occurs. */
#define AX_ERR_IVE_NOBUF AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_NOBUF)
/* The buffer is empty. */
#define AX_ERR_IVE_BUF_EMPTY AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_BUF_EMPTY)
/* No buffer is provided for storing new data. */
#define AX_ERR_IVE_BUF_FULL AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_BUF_FULL)
/* The system is not ready because it may be not initialized or loaded.
 * The error code is returned when a device file fails to be opened. */
#define AX_ERR_IVE_NOTREADY AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_SYS_NOTREADY)
/* The source address or target address is incorrect during the operations such as calling
copy_from_user or copy_to_user. */
#define AX_ERR_IVE_BADADDR AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_BADADDR)
/* The resource is busy during the operations such as destroying a VENC channel
without deregistering it. */
#define AX_ERR_IVE_BUSY AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_BUSY)
/* IVE open device error: */
#define AX_ERR_IVE_OPEN_FAILED AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_IVE_OPEN_FAILED)
/* IVE init error: */
#define AX_ERR_IVE_INIT_FAILED AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_IVE_INIT_FAILED)
/* IVE not init error:  */
#define AX_ERR_IVE_NOT_INIT AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_IVE_NOT_INIT)
/* IVE process timeout: */
#define AX_ERR_IVE_SYS_TIMEOUT AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_IVE_SYS_TIMEOUT)
/* IVE query timeout: */
#define AX_ERR_IVE_QUERY_TIMEOUT AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_IVE_QUERY_TIMEOUT)
/* IVE Bus error: */
#define AX_ERR_IVE_BUS_ERR AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_IVE_BUS_ERR)
/* IVE unknow error: */
#define AX_ERR_IVE_UNKNOWN AX_DEF_ERR(AX_ID_IVE, AX_ID_IVE_SMOD, AX_ERR_UNKNOWN)

/* Create handle error: */
#define AX_ERR_IVE_MAU_CREATE_HANDLE AX_DEF_ERR(AX_ID_IVE, AX_ID_MAU_SMOD, AX_ERR_MAU_CREATE_HANDLE_ERROR)
/* Destory handle error: */
#define AX_ERR_IVE_MAU_DESTROY_HANDLE AX_DEF_ERR(AX_ID_IVE, AX_ID_MAU_SMOD, AX_ERR_MAU_DESTROY_HANDLE_ERROR)
/* Matrix multiplication operation error: */
#define AX_ERR_IVE_MAU_MATMUL AX_DEF_ERR(AX_ID_IVE, AX_ID_MAU_SMOD, AX_ERR_MAU_MATMUL_ERROR)
/* MAU driver error: */
#define AX_ERR_IVE_MAU_DRIVER AX_DEF_ERR(AX_ID_IVE, AX_ID_MAU_SMOD, AX_ERR_MAU_DRIVER_ERROR)
/* Tile error, only for NPU engine: */
#define AX_ERR_IVE_MAU_TILE AX_DEF_ERR(AX_ID_IVE, AX_ID_MAU_SMOD, AX_ERR_MAU_TILE_ERROR)


/*
* DMA mode
*/
typedef enum axIVE_DMA_MODE_E {
    AX_IVE_DMA_MODE_DIRECT_COPY = 0x0,
    AX_IVE_DMA_MODE_INTERVAL_COPY = 0x1,
    AX_IVE_DMA_MODE_SET_3BYTE = 0x2,
    AX_IVE_DMA_MODE_SET_8BYTE = 0x3,
    AX_IVE_DMA_MODE_BUTT
} AX_IVE_DMA_MODE_E;

/*
* DMA control parameter
*/
typedef struct axIVE_DMA_CTRL_T {
    AX_IVE_DMA_MODE_E enMode;
    AX_U64 u64Val;      /* Used in memset mode */
    AX_U8 u8HorSegSize; /* Used in interval-copy mode, every row was segmented by u8HorSegSize bytes */
    AX_U8 u8ElemSize;   /* Used in interval-copy mode, the valid bytes copied in front of every segment
                        in a valid row, which 0<u8ElemSize<u8HorSegSize */
    AX_U8 u8VerSegRows; /* Used in interval-copy mode, copy one row in every u8VerSegRows */
    AX_U16 u16CrpX0;    /* Used in direct-copy mode, crop start point x-coordinate */
    AX_U16 u16CrpY0;    /* Used in direct-copy mode, crop start point y-coordinate */
} AX_IVE_DMA_CTRL_T;

/*
* Add control parameters
*/
typedef struct axIVE_ADD_CTRL_T {
    AX_U1Q7 u1q7X; /* x of "xA+yB" */
    AX_U1Q7 u1q7Y; /* y of "xA+yB" */
} AX_IVE_ADD_CTRL_T;

/*
* Type of the Sub output results
*/
typedef enum axIVE_SUB_MODE_E {
    AX_IVE_SUB_MODE_ABS = 0x0,   /* Absolute value of the difference */
    AX_IVE_SUB_MODE_SHIFT = 0x1, /* The output result is obtained by shifting the result one digit right
                                to reserve the signed bit. */
    AX_IVE_SUB_MODE_BUTT
} AX_IVE_SUB_MODE_E;

/*
* Sub control parameters
*/
typedef struct axIVE_SUB_CTRL_T {
    AX_IVE_SUB_MODE_E enMode;
} AX_IVE_SUB_CTRL_T;

/*
* Mse control parameters
*/
typedef struct axIVE_MSE_CTRL_T {
    AX_U1Q15 u1q15MseCoef; /* MSE coef, range: [0,65535]*/
} AX_IVE_MSE_CTRL_T;

/*
* HysEdge control struct
*/
typedef struct axIVE_HYS_EDGE_CTRL_T {
    AX_U16 u16LowThr;
    AX_U16 u16HighThr;
} AX_IVE_HYS_EDGE_CTRL_T;

/*
* CannyEdge control struct
*/
typedef struct axIVE_CANNY_EDGE_CTRL_T {
    AX_U8 u8Thr;
} AX_IVE_CANNY_EDGE_CTRL_T;

/*
* Region struct
*/
typedef struct axIVE_REGION_T {
    AX_U8 u8LabelStatus; /* 0: Labeled failed ; 1: Labeled successfully */
    AX_U32 u32Area;   /* Represented by the pixel number */
    AX_U16 u16Left;   /* Circumscribed rectangle left border */
    AX_U16 u16Right;  /* Circumscribed rectangle right border */
    AX_U16 u16Top;    /* Circumscribed rectangle top border */
    AX_U16 u16Bottom; /* Circumscribed rectangle bottom border */
} AX_IVE_REGION_T;

/*
* CCBLOB struct
*/
typedef struct axIVE_CCBLOB_T {
    AX_U16 u16RegionNum; /* Number of valid region */
    AX_IVE_REGION_T astRegion[AX_IVE_MAX_REGION_NUM]; /* Valid regions with 'u32Area>0' and 'label = ArrayIndex+1' */
} AX_IVE_CCBLOB_T;

/*
* Type of the CCL
*/
typedef enum axIVE_CCL_MODE_E {
    AX_IVE_CCL_MODE_4C = 0x0, /* 4-connected */
    AX_IVE_CCL_MODE_8C = 0x1, /* 8-connected */

    AX_IVE_CCL_MODE_BUTT
} AX_IVE_CCL_MODE_E;
/*
* CCL control struct
*/
typedef struct axIVE_CCL_CTRL_T {
    AX_IVE_CCL_MODE_E enMode; /* Mode */
} AX_IVE_CCL_CTRL_T;

/*
* Erode control parameter
*/
typedef struct axIVE_ERODE_CTRL_T {
    AX_U8 au8Mask[25]; /* The template parameter value must be 0 or 255. */
} AX_IVE_ERODE_CTRL_T;

/*
* Dilate control parameters
*/
typedef struct axIVE_DILATE_CTRL_T {
    AX_U8 au8Mask[25]; /* The template parameter value must be 0 or 255. */
} AX_IVE_DILATE_CTRL_T;

/*
* Filter control parameters
* You need to set these parameters when using the filter operator.
*/
typedef struct axIVE_FILTER_CTRL_T {
    AX_S6Q10 as6q10Mask[25]; /* Template parameter filter coefficient */
} AX_IVE_FILTER_CTRL_T;

/*
* Equalizehist control parameters
*/
typedef struct axIVE_EQUALIZE_HIST_CTRL_T {
    AX_U0Q20 u0q20HistEqualCoef; /* range: [0,1048575] */
} AX_IVE_EQUALIZE_HIST_CTRL_T;

/*
* Type of the Integ output results
*/
typedef enum axIVE_INTEG_OUT_CTRL_E {
    AX_IVE_INTEG_OUT_CTRL_COMBINE = 0x0,
    AX_IVE_INTEG_OUT_CTRL_SUM = 0x1,
    AX_IVE_INTEG_OUT_CTRL_SQSUM = 0x2,
    AX_IVE_INTEG_OUT_CTRL_BUTT
} AX_IVE_INTEG_OUT_CTRL_E;

/*
* Integ control parameters
*/
typedef struct axIVE_INTEG_CTRL_T {
    AX_IVE_INTEG_OUT_CTRL_E enOutCtrl;
} AX_IVE_INTEG_CTRL_T;

/*
* SOBEL control parameter
*/
typedef struct axIVE_SOBEL_CTRL_T {
    AX_S6Q10 as6q10Mask[25]; /* Template parameter sobel coefficient */
} AX_IVE_SOBEL_CTRL_T;

/*
* GMM control struct
*/
typedef struct axIVE_GMM_CTRL_T {
    AX_U14Q4 u14q4InitVar;  /* Initial Variance, range: [0,262143] */
    AX_U14Q4 u14q4MinVar;   /* Min  Variance, range: [0,262143] */
    AX_U1Q10 u1q10InitWeight; /* Initial Weight, range: [0,1024] */
    AX_U1Q7 u1q7LearnRate;    /* Learning rate, range: [0,128] */
    AX_U1Q7 u1q7BgRatio;      /* Background ratio, range: [0,128] */
    AX_U4Q4 u4q4VarThr;       /* Variance Threshold, range: [0,255] */
    AX_U8 u8Thr;              /* Output Threshold, range: [1,255] */
} AX_IVE_GMM_CTRL_T;

/*
* GMM2 control struct
*/
typedef struct axIVE_GMM2_CTRL_T {
    AX_U14Q4 u14q4InitVar; /* Initial Variance, range: [0,262143] */
    AX_U14Q4 u14q4MinVar;  /* Min  Variance, range: [0,262143] */
    AX_U14Q4 u14q4MaxVar;  /* Max  Variance, range: [0,262143] */
    AX_U1Q7 u1q7LearnRate;   /* Learning rate, range: [0,128] */
    AX_U1Q7 u1q7BgRatio;     /* Background ratio, range: [0,128] */
    AX_U4Q4 u4q4VarThr;      /* Variance Threshold, range: [0,255] */
    AX_U4Q4 u4q4VarThrCheck; /* Variance Threshold Check, range: [0,255] */
    AX_S1Q7 s1q7CT;          /* range: [-255,255] */
    AX_U8 u8Thr;             /* Output Threshold, range: [1,255] */
} AX_IVE_GMM2_CTRL_T;

/*
* Type of the Thresh mode.
*/
typedef enum axIVE_THRESH_MODE_E {
    AX_IVE_THRESH_MODE_BINARY = 0x0,    /* srcVal <= lowThr, dstVal = minVal; srcVal > lowThr, dstVal = maxVal. */
    AX_IVE_THRESH_MODE_TRUNC = 0x1,     /* srcVal <= lowThr, dstVal = srcVal; srcVal > lowThr, dstVal = maxVal. */
    AX_IVE_THRESH_MODE_TO_MINVAL = 0x2, /* srcVal <= lowThr, dstVal = minVal; srcVal > lowThr, dstVal = srcVal. */

    AX_IVE_THRESH_MODE_MIN_MID_MAX = 0x3, /* srcVal <= lowThr, dstVal = minVal;  lowThr < srcVal <= highThr,
                                       dstVal = midVal; srcVal > highThr, dstVal = maxVal. */
    AX_IVE_THRESH_MODE_ORI_MID_MAX = 0x4, /* srcVal <= lowThr, dstVal = srcVal;  lowThr < srcVal <= highThr,
                                       dstVal = midVal; srcVal > highThr, dstVal = maxVal. */
    AX_IVE_THRESH_MODE_MIN_MID_ORI = 0x5, /* srcVal <= lowThr, dstVal = minVal;  lowThr < srcVal <= highThr,
                                       dstVal = midVal; srcVal > highThr, dstVal = srcVal. */
    AX_IVE_THRESH_MODE_MIN_ORI_MAX = 0x6, /* srcVal <= lowThr, dstVal = minVal;  lowThr < srcVal <= highThr,
                                       dstVal = srcVal; srcVal > highThr, dstVal = maxVal. */
    AX_IVE_THRESH_MODE_ORI_MID_ORI = 0x7, /* srcVal <= lowThr, dstVal = srcVal;  lowThr < srcVal <= highThr,
                                       dstVal = midVal; srcVal > highThr, dstVal = srcVal. */

    AX_IVE_THRESH_MODE_BUTT
} AX_IVE_THRESH_MODE_E;

/*
* Thresh control parameters.
*/
typedef struct axIVE_THRESH_CTRL_T {
    AX_IVE_THRESH_MODE_E enMode;
    AX_U8 u8LowThr;  /* user-defined threshold,  0<=u8LowThr<=255 */
    AX_U8 u8HighThr; /* user-defined threshold, if enMode<AX_IVE_THRESH_MODE_MIN_MID_MAX, u8HighThr is not used,
                      else 0<=u8LowThr<=u8HighThr<=255; */
    AX_U8 u8MinVal;  /* Minimum value when tri-level thresholding */
    AX_U8 u8MidVal;  /* Middle value when tri-level thresholding, if enMode<2, u32MidVal is not used; */
    AX_U8 u8MaxVal;  /* Maxmum value when tri-level thresholding */
} AX_IVE_THRESH_CTRL_T;

/*
* Mode of 16BitTo8Bit
*/
typedef enum axIVE_16BIT_TO_8BIT_MODE_E {
    AX_IVE_16BIT_TO_8BIT_MODE_S16_TO_S8 = 0x0,
    AX_IVE_16BIT_TO_8BIT_MODE_S16_TO_U8_ABS = 0x1,
    AX_IVE_16BIT_TO_8BIT_MODE_S16_TO_U8_BIAS = 0x2,
    AX_IVE_16BIT_TO_8BIT_MODE_U16_TO_U8 = 0x3,

    AX_IVE_16BIT_TO_8BIT_MODE_BUTT
} AX_IVE_16BIT_TO_8BIT_MODE_E;

/*
* 16BitTo8Bit control parameters
*/
typedef struct axIVE_16BIT_TO_8BIT_CTRL_T {
    AX_IVE_16BIT_TO_8BIT_MODE_E enMode;
    AX_S1Q14 s1q14Gain; /* range: [-16383,16383] */
    AX_S16 s16Bias; /* range: [-16384,16383] */
} AX_IVE_16BIT_TO_8BIT_CTRL_T;

/*
* Crop image control parameter
*/
typedef struct axIVE_CROP_IMAGE_CTRL_T {
    AX_U16 u16Num;
} AX_IVE_CROP_IMAGE_CTRL_T;

/*
* Aspect ratio align mode
*/
typedef enum axIVE_ASPECT_RATIO_ALIGN_MODE_E {
    AX_IVE_ASPECT_RATIO_FORCE_RESIZE = 0, /* Without keep aspect ratio, others keep aspect ratio. */
    AX_IVE_ASPECT_RATIO_HORIZONTAL_LEFT = 1, /* Border on the right of the image. */
    AX_IVE_ASPECT_RATIO_HORIZONTAL_CENTER = 2, /* Border on both sides of the image. */
    AX_IVE_ASPECT_RATIO_HORIZONTAL_RIGHT = 3, /* Border on the left of the image. */
    AX_IVE_ASPECT_RATIO_VERTICAL_TOP = AX_IVE_ASPECT_RATIO_HORIZONTAL_LEFT, /* Border on the bottom of the image. */
    AX_IVE_ASPECT_RATIO_VERTICAL_CENTER = AX_IVE_ASPECT_RATIO_HORIZONTAL_CENTER, /* Border on both sides of the image. */
    AX_IVE_ASPECT_RATIO_VERTICAL_BOTTOM = AX_IVE_ASPECT_RATIO_HORIZONTAL_RIGHT, /* Border on the top of the image. */

    AX_IVE_ASPECT_RATIO_BUTT
} AX_IVE_ASPECT_RATIO_ALIGN_MODE_E;

/*
* CropResize control parameter
*/
typedef struct axIVE_CROP_RESIZE_CTRL_T {
    AX_U16 u16Num;
    AX_IVE_ASPECT_RATIO_ALIGN_MODE_E enAlign[2];
    AX_U32 u32BorderColor;
} AX_IVE_CROP_RESIZE_CTRL_T;

/*
* MAU and NPU data structure
*/

/*
* MatMul handle
*/
typedef AX_VOID *AX_IVE_MATMUL_HANDLE;

/*
* MAU engine id
*/
typedef enum axIVE_MAU_ID_E {
    AX_IVE_MAU_ID_0 = 0,

    AX_IVE_MAU_ID_BUTT
} AX_IVE_MAU_ID_E;

/*
* MAU order
*/
typedef enum axIVE_MAU_ORDER_E {
    AX_IVE_MAU_ORDER_ASCEND  = 0,
    AX_IVE_MAU_ORDER_DESCEND = 1,

    AX_IVE_MAU_ORDER_BUTT
} AX_IVE_MAU_ORDER_E;

/*
* Data type for matrix manipulation
*/
typedef enum axIVE_MAU_DATA_TYPE_E {
    AX_IVE_MAU_DT_UNKNOWN  = 0,
    AX_IVE_MAU_DT_UINT8    = 1,
    AX_IVE_MAU_DT_UINT16   = 2,
    AX_IVE_MAU_DT_FLOAT32  = 3,
    AX_IVE_MAU_DT_SINT16   = 4,
    AX_IVE_MAU_DT_SINT8    = 5,
    AX_IVE_MAU_DT_SINT32   = 6,
    AX_IVE_MAU_DT_UINT32   = 7,
    AX_IVE_MAU_DT_FLOAT64  = 8,
    AX_IVE_MAU_DT_FLOAT16  = 9,
    AX_IVE_MAU_DT_UINT64   = 10,
    AX_IVE_MAU_DT_SINT64   = 11,
    AX_IVE_MAU_DT_BFLOAT16 = 12,

    AX_IVE_MAU_DT_BUTT
} AX_IVE_MAU_DATA_TYPE_E;

/*
* MatMul control parameter for NPU engine
*/
typedef struct axIVE_NPU_MATMUL_CTRL_T {
    AX_CHAR *pchModelDir;
    AX_IVE_MAU_DATA_TYPE_E enDataType;
    AX_S32 s32KSize;
} AX_IVE_NPU_MATMUL_CTRL_T;

/*
* MatMul control parameter for MAU engine
*/
typedef struct axIVE_MAU_MATMUL_CTRL_T {
    AX_IVE_MAU_ID_E enMauId;
    AX_S32 s32DdrReadBandwidthLimit;
    AX_BOOL bEnableMulRes;
    AX_BOOL bEnableTopNRes;
    AX_IVE_MAU_ORDER_E enOrder;
    AX_S32 s32TopN;
} AX_IVE_MAU_MATMUL_CTRL_T;

/*
* Blob data(tensor) for matrix manipulation
*/
typedef struct axIVE_MAU_BLOB_T {
    AX_U64 u64PhyAddr;
    AX_VOID *pVirAddr;
    AX_S32 *pShape;
    AX_U8 u8ShapeSize;
    AX_IVE_MAU_DATA_TYPE_E enDataType;
} AX_IVE_MAU_BLOB_T;

/*
* Input data for MatMul
*/
typedef struct axIVE_MAU_MATMUL_INPUT_T {
    AX_IVE_MAU_BLOB_T stMatQ;
    AX_IVE_MAU_BLOB_T stMatB;
} AX_IVE_MAU_MATMUL_INPUT_T;

/*
* Output data for MatMul
*/
typedef struct axIVE_MAU_MATMUL_OUTPUT_T {
    AX_IVE_MAU_BLOB_T stMulRes;
    AX_IVE_MAU_BLOB_T stTopNRes; /* NPU engine no need */
} AX_IVE_MAU_MATMUL_OUTPUT_T;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif/*_AX_IVE_TYPE_H_*/