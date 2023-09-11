/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_MIPI_API__
#define __AX_MIPI_API__

#include "ax_base_type.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
    AX_MIPI_TX_SRC_SNS_0,
    AX_MIPI_TX_SRC_SNS_1,
    AX_MIPI_TX_SRC_SNS_2,
    AX_MIPI_TX_SRC_RAW_SCALAR_2,
    AX_MIPI_TX_SRC_RAW_SCALAR_3,
    AX_MIPI_TX_SRC_RLTM,
    AX_MIPI_TX_SRC_RLTM_INFO,
} AX_MIPI_TX_SRC_E;

typedef enum {
    AX_MIPI_TX_DATA_LANE_1 = 1,
    AX_MIPI_TX_DATA_LANE_2 = 2,
    AX_MIPI_TX_DATA_LANE_3 = 3,
    AX_MIPI_TX_DATA_LANE_4 = 4,
} AX_MIPI_TX_DATA_LANE_NUM_E;

typedef enum {
    AX_MIPI_VC_0 = 0,
    AX_MIPI_VC_1,
    AX_MIPI_VC_2,
    AX_MIPI_VC_3,
    AX_MIPI_VC_4,
    AX_MIPI_VC_5,
    AX_MIPI_VC_6,
    AX_MIPI_VC_7,
    AX_MIPI_VC_8,
    AX_MIPI_VC_9,
    AX_MIPI_VC_10,
    AX_MIPI_VC_11,
    AX_MIPI_VC_12,
    AX_MIPI_VC_13,
    AX_MIPI_VC_14,
    AX_MIPI_VC_15,
    AX_MIPI_VC_MAX
} AX_MIPI_VC_NUM_E;

typedef enum {
    AX_MIPI_DT_YUV420_8BIT_FBC_LOSSY = 0,
    AX_MIPI_DT_YUV420_10BIT_FBC_LOSSY = 1,
    AX_MIPI_DT_YUV420_8BIT_NV12   = 2,
    AX_MIPI_DT_YUV420_8BIT_NV21   = 3,
    AX_MIPI_DT_YUV420_10BIT_NV12_LEGACY   = 4,
    AX_MIPI_DT_YUV420_10BIT_NV12_TIGHT   = 5,
    AX_MIPI_DT_YUV420_10BIT_NV21_LEAGCY   = 6,
    AX_MIPI_DT_YUV420_10BIT_NV21_TIGHT   = 7,
    AX_MIPI_DT_RAW8     = 8,
    AX_MIPI_DT_MAX
} AX_MIPI_TX_TYPE_E;

typedef enum {
    AX_MIPI_DOL_1 = 1,
    AX_MIPI_DOL_2,
    AX_MIPI_DOL_3,
    AX_MIPI_DOL_MAX
} AX_MIPI_DOL_NUM_E;

typedef struct {
    //AX_MIPI_TX_SRC_E                 eInputSrc;
    AX_MIPI_TX_DATA_LANE_NUM_E       eLaneNum;                /*only 4*/
    AX_U32                           nDataRateMbps;           /*dphy datarate(Mbps)*/
    AX_U32                           nImgWidth;
    AX_U32                           nImgHeight;
    AX_MIPI_TX_TYPE_E                eImgDataType;            /*val(8, 10, 12, 14)*/
    AX_MIPI_VC_NUM_E                 eImgVC;
    AX_U32                           nfps;
    AX_U64                           y_or_yuv_addr;  /*1planar or 2planar y*/
    AX_U64                           uv_addr; /*2planar  uv*/
    AX_U8                            nLaneMap[5];
} AX_MIPI_TX_ATTR_S;

AX_S32 AX_MIPI_TX_Init(AX_VOID);
AX_S32 AX_MIPI_TX_DeInit(AX_VOID);
AX_S32 AX_MIPI_TX_Reset(AX_VOID);
AX_S32 AX_MIPI_TX_SetAttr(AX_MIPI_TX_ATTR_S *pMipiAttr);
AX_S32 AX_MIPI_TX_GetAttr(AX_MIPI_TX_ATTR_S *pMipiAttr);
AX_S32 AX_MIPI_TX_Start(AX_VOID);
AX_S32 AX_MIPI_TX_Stop(AX_VOID);

#ifdef __cplusplus
}
#endif

#endif // __AX_MIPI_API__
