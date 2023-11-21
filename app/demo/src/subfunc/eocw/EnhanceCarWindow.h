/**********************************************************************************
 *
 * Copyright (c) 2019-2020 Beijing AXera Technology Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Beijing AXera Technology Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Beijing AXera Technology Co., Ltd.
 *
 **********************************************************************************/
#ifndef _ENHANCE_CAR_WINDOW_H_
#define _ENHANCE_CAR_WINDOW_H_

#include "ax_base_type.h"
#include "ax_sys_api.h"
#include "ax_dmadim_api.h"
#include "ax_engine_api.h"
#include "ax_vin_api.h"

#define AX_ENHANCE_MAX_PATH_SIZE (256)
#define AX_ENHANCE_TABLE_CNT (8)
typedef enum {
    IDX_OFFSET_IN       = 0,
    IDX_OFFSET_OUT      = 1,
    IDX_ROI_RAW         = 2,
    IDX_ROI_MASK        = 3,
} AX_SAMPLE_ENHANCE_ENGINE_IN;

typedef enum {
    IDX_ENHANCE_ROI_RAW     = 0,
} AX_SAMPLE_ENHANCE_ENGINE_OUT;

typedef struct _AX_SAMPLE_ENHANCE_CW_RECT_T {
    AX_U32 nX;
    AX_U32 nY;
    AX_U32 nW;
    AX_U32 nH;
} AX_SAMPLE_ENHANCE_CW_RECT_T;

// same as ISensor.hpp/ENHANCE_CONFIG_T
typedef struct _AX_ENHANCE_CONFIG_T {
    AX_U32 nRefValue;
    AX_CHAR szModel[AX_ENHANCE_MAX_PATH_SIZE];
    AX_CHAR szMask[AX_ENHANCE_TABLE_CNT];
} AX_ENHANCE_CONFIG_T, *AX_ENHANCE_CONFIG_PTR;

typedef struct _AX_SAMPLE_ENHANCE_CFG_T {
    AX_S8 *pModelPath;
    AX_S8 *pCarWindowPath;          // for test
    AX_S8 *pCarWindowMaskPath;      // fixed, mask.bin
    AX_S8 *pCarWindowEnhancePath;   // for test
    AX_ENGINE_IO_T tEngineIOData;
} AX_SAMPLE_ENHANCE_CFG_T;

extern AX_S32 gDmaChn;
extern AX_ENGINE_HANDLE gEnginehandle;
extern AX_SAMPLE_ENHANCE_CFG_T gtSampleEnhanceCfg;

AX_BOOL SAMPLE_ENHANCE_Init(AX_ENHANCE_CONFIG_T *tEnhanceCfg, AX_U32 nEnhanceCfgCnt);
AX_BOOL SAMPLE_ENHANCE_DeInit(AX_VOID);
AX_BOOL SAMPLE_ENHANCE_Run(AX_SAMPLE_ENHANCE_CW_RECT_T *pRoi, AX_U32 nRoiSize, const AX_IMG_INFO_T *pImgInfo[]);

#endif /* _ENHANCE_CAR_WINDOW_H_ */
