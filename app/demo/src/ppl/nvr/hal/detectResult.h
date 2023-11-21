/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#pragma once

#include "ax_base_type.h"
#include "ax_skel_api.h"

#define MAX_DETECT_RESULT_COUNT (32)

typedef struct {
    AX_U64 nSeqNum{0};
    AX_U32 nGrpId{0};
    AX_U32 nChnId{0};
    AX_U32 nSkelChn{0};
} SKEL_FRAME_PRIVATE_DATA_T;

typedef enum {
    DETECT_TYPE_UNKNOWN = 0,
    DETECT_TYPE_FACE = 1,
    DETECT_TYPE_BODY = 2,
    DETECT_TYPE_VEHICLE = 3,
    DETECT_TYPE_PLATE = 4,
    DETECT_TYPE_CYCLE = 5,
    DETECT_TYPE_BUTT
} DETECT_TYPE_E;

typedef struct {
    DETECT_TYPE_E eType;
    AX_SKEL_RECT_T tBox;
} DETECT_RESULT_ITEM_T;

typedef struct DETECT_RESULT_S {
    AX_U64 nSeqNum{0};
    AX_U32 nW{0};  // detect Ooriginal resolution
    AX_U32 nH{0};  // detect original resolution
    AX_U32 nCount{0};
    AX_U32 nGrpId{0};
    AX_U32 nChn{0};
    DETECT_RESULT_ITEM_T item[MAX_DETECT_RESULT_COUNT];
} DETECT_RESULT_T;
