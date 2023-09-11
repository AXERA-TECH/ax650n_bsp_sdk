/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#pragma once

typedef enum {
    NALU_TYPE_NON = 0,
    NALU_TYPE_SEI = 1,
    NALU_TYPE_SPS = 2,
    NALU_TYPE_PPS = 3,
    NALU_TYPE_VPS = 4,
    NALU_TYPE_IDR = 5,  // IDR
    NALU_TYPE_OTH = 6   // NO IDR, simpy to classify P, B... to OTH
} STREAM_NALU_TYPE_E;