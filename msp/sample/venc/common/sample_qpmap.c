/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/


#include "sample_qpmap.h"

#include "sample_global.h"
#include "sample_venc_log.h"

AX_VOID SampleWriteQpValue2Memory(AX_S8 qpDelta, AX_S8* memory, AX_U16 column, AX_U16 row, AX_U16 blockunit,
                                  AX_U16 width, AX_U16 ctb_size, AX_U32 ctb_per_row, AX_U32 ctb_per_column,
                                  AX_S32 qpMapQpType)
{
    AX_U32 blks_per_ctb = ctb_size / 8;
    AX_U32 blks_per_unit = 1 << (3 - blockunit);
    AX_U32 ctb_row_number = row * blks_per_unit / blks_per_ctb;
    AX_U32 ctb_column_number = column * blks_per_unit / blks_per_ctb;
    AX_U32 ctb_row_stride = ctb_per_row * blks_per_ctb * blks_per_ctb;
    AX_U32 xoffset = (column * blks_per_unit) % blks_per_ctb;
    AX_U32 yoffset = (row * blks_per_unit) % blks_per_ctb;
    AX_U32 stride = blks_per_ctb;
    AX_U32 columns, rows, r, c;

    rows = columns = blks_per_unit;

    if (blks_per_ctb < blks_per_unit) {
        rows = MIN(rows, ctb_per_column * blks_per_ctb - row * blks_per_unit);
        columns = MIN(columns, ctb_per_row * blks_per_ctb - column * blks_per_unit);
        rows /= blks_per_ctb;
        columns *= blks_per_ctb;
        stride = ctb_row_stride;
    }

    // ctb addr --> blk addr
    memory += ctb_row_number * ctb_row_stride + ctb_column_number * (blks_per_ctb * blks_per_ctb);
    memory += yoffset * stride + xoffset;
    for (r = 0; r < rows; r++) {
        AX_S8* dst = memory + r * stride;
        for (c = 0; c < columns; c++) {
            if (AX_VENC_QPMAP_QP_DELTA == qpMapQpType || AX_VENC_QPMAP_QP_ABS == qpMapQpType)
                *dst++ = qpDelta;
        }
    }
}

AX_VOID SampleCopyQPDelta2Memory(AX_S32 w, AX_S32 h, AX_S32 maxCuSize, AX_S32 blkUnit, AX_S8* QpmapAddr,
                                 AX_S32 qpMapQpType)
{
    AX_U32 ctb_per_row = (w + maxCuSize - 1) / maxCuSize;
    AX_U32 ctb_per_column = (h + maxCuSize - 1) / maxCuSize;
    AX_U32 block_unit = 0;
    AX_U32 width, height;
    AX_S32 qpDelta = 0;
    AX_U32 blockunit = 0;
    AX_U32 block_size = 0;

    switch (blkUnit) {
    case 0:
        block_unit = 64;
        blockunit = 0;
        break;
    case 1:
        block_unit = 32;
        blockunit = 1;
        break;
    case 2:
        block_unit = 16;
        blockunit = 2;
        break;
    case 3:
        block_unit = 8;
        blockunit = 3;
        break;
    default:
        block_unit = 64;
        blockunit = 0;
        break;
    }


    block_size = TILE_ALIGN(w, maxCuSize) * TILE_ALIGN(h, maxCuSize) / (8 * 8);
    memset(QpmapAddr, 0, block_size);

    width = (((w + maxCuSize - 1) & (~(maxCuSize - 1))) + block_unit - 1) / block_unit;
    height = (((h + maxCuSize - 1) & (~(maxCuSize - 1))) + block_unit - 1) / block_unit;

    AX_U32 ctb_size = maxCuSize;

    for (AX_S32 line_idx = 0; line_idx < height; line_idx++) {
        // delta qp,rang in [-31, 32]
        if (AX_VENC_QPMAP_QP_DELTA == qpMapQpType) {
            if (line_idx % 2 == 0)
                qpDelta = 30;
            else
                qpDelta = -30;

            qpDelta = CLIP3(-31, 32, qpDelta);
            qpDelta = -qpDelta;
            qpDelta &= 0x3f;
            qpDelta = (qpDelta << 1) | 0;
        } else if (AX_VENC_QPMAP_QP_ABS == qpMapQpType) {  // absolute qp, rang in [0, 51]
            if (line_idx % 2 == 0)
                qpDelta = 20;
            else
                qpDelta = 35;

            qpDelta = CLIP3(0, 51, qpDelta);
            qpDelta &= 0x3f;
            qpDelta = (qpDelta << 1) | 1;
        }

        for (AX_S32 i = 0; i < width; i++) {
            SampleWriteQpValue2Memory((AX_S8)qpDelta, QpmapAddr, i, line_idx, blockunit, width, ctb_size, ctb_per_row,
                                      ctb_per_column, qpMapQpType);
        }
    }
}

/* IPCM map setting */
AX_VOID SampleWriteFlags2Memory(AX_CHAR flag, AX_S8* memory, AX_U16 column, AX_U16 row, AX_U16 blockunit, AX_U16 width,
                                AX_U16 ctb_size, AX_U32 ctb_per_row, AX_U32 ctb_per_column)
{
    AX_U32 blks_per_ctb = ctb_size / 8;
    AX_U32 blks_per_unit = 1 << (3 - blockunit);
    AX_U32 ctb_row_number = row * blks_per_unit / blks_per_ctb;
    AX_U32 ctb_column_number = column * blks_per_unit / blks_per_ctb;
    AX_U32 ctb_row_stride = ctb_per_row * blks_per_ctb * blks_per_ctb;
    AX_U32 xoffset = (column * blks_per_unit) % blks_per_ctb;
    AX_U32 yoffset = (row * blks_per_unit) % blks_per_ctb;
    AX_U32 stride = blks_per_ctb;
    AX_U32 columns, rows, r, c;

    rows = columns = blks_per_unit;
    if (blks_per_ctb < blks_per_unit) {
        rows = MIN(rows, ctb_per_column * blks_per_ctb - row * blks_per_unit);
        columns = MIN(columns, ctb_per_row * blks_per_ctb - column * blks_per_unit);
        rows /= blks_per_ctb;
        columns *= blks_per_ctb;
        stride = ctb_row_stride;
    }

    // ctb addr --> blk addr
    memory += ctb_row_number * ctb_row_stride + ctb_column_number * (blks_per_ctb * blks_per_ctb);
    memory += yoffset * stride + xoffset;
    for (r = 0; r < rows; r++) {
        AX_S8* dst = memory + r * stride;
        AX_U8 val;
        for (c = 0; c < columns; c++) {
            val = *dst;
            *dst++ = (val & 0x7f) | (flag << 7);
        }
    }
}

AX_S32 SampleCopyFlagsMap2Memory(AX_S32 w, AX_S32 h, AX_S32 maxCuSize, AX_U16 blockUnit, AX_S8* QpmapAddr,
                                 AX_S32 qpmapBlkType)
{
    AX_U32 ctb_per_row = ((w + maxCuSize - 1) / (maxCuSize));
    AX_U32 ctb_per_column = ((h + maxCuSize - 1) / (maxCuSize));
    AX_U16 ctb_size = maxCuSize;
    AX_U16 block_unit = blockUnit;
    AX_U16 blockUnitMax = AX_VENC_QPMAP_BLOCK_UNIT_16x16;
    AX_U32 i;
    AX_U32 flag = 0;

    AX_U16 width, height, block_unit_size;

    if (AX_VENC_QPMAP_BLOCK_IPCM == qpmapBlkType) {
        /* h265 IPCM map only support 64x64, h264 IPCM map only support 16x16 */
        block_unit = ((maxCuSize == MAX_CU_SIZE) ? AX_VENC_QPMAP_BLOCK_UNIT_64x64 : AX_VENC_QPMAP_BLOCK_UNIT_16x16);
    } else if (AX_VENC_QPMAP_BLOCK_SKIP == qpmapBlkType) {
        if (MAX_CU_SIZE == maxCuSize) {  // h265
            blockUnitMax = AX_VENC_QPMAP_BLOCK_UNIT_32x32;
            block_unit = MIN(blockUnit, blockUnitMax);  // h265 skipmap only support 64x64、32x32
        } else if (MAX_AVC_CU_SIZE == maxCuSize) {      // h264
            blockUnitMax = AX_VENC_QPMAP_BLOCK_UNIT_16x16;
            block_unit = MIN(blockUnit, blockUnitMax);  // h264 skipmap support 64x64、32x32、16x16
        }
    }

    block_unit_size = 8 << (3 - block_unit);
    width = (((w + maxCuSize - 1) & (~(maxCuSize - 1))) + block_unit_size - 1) / block_unit_size;
    height = (((h + maxCuSize - 1) & (~(maxCuSize - 1))) + block_unit_size - 1) / block_unit_size;

    for (AX_S32 line_idx = 0; line_idx < height; line_idx++) {
        /* ipcm/skip map setting, could be changed by user */
        flag = 0;  // disable ipcm/skip map

        if (AX_VENC_QPMAP_BLOCK_SKIP == qpmapBlkType) {
            if (line_idx < (height / 2))
                flag = 1;
        } else if (AX_VENC_QPMAP_BLOCK_IPCM == qpmapBlkType) {
            if (line_idx % 8 == 0)
                flag = 1;
        }

        for (i = 0; i < width; i++)
            SampleWriteFlags2Memory((AX_CHAR)flag, QpmapAddr, i, line_idx, block_unit, width, ctb_size, ctb_per_row,
                                    ctb_per_column);
    }

    return 0;
}
