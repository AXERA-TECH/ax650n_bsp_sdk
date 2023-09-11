/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __OS08A20_AE_PARAMS_H__
#define __OS08A20_AE_PARAMS_H__

typedef struct _SNSOS08A20_OBJ_T_ {
    AX_U32 hts;
    AX_U32 vs_hts;
    AX_U32 vts;
    AX_F32 sclk;
    AX_F32 line_period;
} SNSOS08A20_OBJ_T;

#endif  //end __OS08A20_AE_PARAMS_H__
