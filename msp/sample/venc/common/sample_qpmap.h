/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __SAMPLE_QPMAP_H__
#define __SAMPLE_QPMAP_H__

#include "ax_global_type.h"
#include "ax_venc_api.h"

AX_VOID SampleWriteQpValue2Memory(AX_S8 qpDelta, AX_S8* memory, AX_U16 column, AX_U16 row, AX_U16 blockunit,
                                  AX_U16 width, AX_U16 ctb_size, AX_U32 ctb_per_row, AX_U32 ctb_per_column,
                                  AX_S32 qpmapType);

AX_VOID SampleCopyQPDelta2Memory(AX_S32 w, AX_S32 h, AX_S32 maxCuSize, AX_S32 blkUnit, AX_S8* QpmapAddr,
                                 AX_S32 qpmapType);

AX_VOID SampleWriteFlags2Memory(AX_CHAR flag, AX_S8* memory, AX_U16 column, AX_U16 row, AX_U16 blockunit, AX_U16 width,
                                AX_U16 ctb_size, AX_U32 ctb_per_row, AX_U32 ctb_per_column);

AX_S32 SampleCopyFlagsMap2Memory(AX_S32 w, AX_S32 h, AX_S32 maxCuSize, AX_U16 blockUnit, AX_S8* QpmapAddr,
                                 AX_S32 qpmapType);

#endif