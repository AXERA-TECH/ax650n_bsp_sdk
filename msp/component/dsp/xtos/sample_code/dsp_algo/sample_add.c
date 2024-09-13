/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xtensa/hal.h>
#include <xtensa/xtutil.h>
#include <xtensa/config/core.h>
#include <xtensa/xtbsp.h>
#include <ax_base_type.h>
#include <ax650x_api.h>
#include <ax_dsp_common.h>
#include <xtensa/idma.h>
#include <xtensa/hal.h>
#include <ax_base_type.h>
#include <ax_dsp_def.h>

#include "ax_dsp_tm.h"
#include "ax_dsp_trace.h"

#define TEST_SIZE 0x1000

int AX_DSP_AlgoArrayadd(AX_DSP_MESSAGE_T *msg)
{
    int i;
    char *pSrc0, * pSrc1, * pDst;
    AX_S32 size = TEST_SIZE;
    AX_U64 tmp;
    tmp = msg->u32Body[0] | ((AX_U64)msg->u32Body[1] << 32);
    pSrc0 = (char *)ax_cpu_addr_to_dsp_addr(tmp);
    tmp = msg->u32Body[2] | ((AX_U64)msg->u32Body[3] << 32);
    pSrc1 = (char *)ax_cpu_addr_to_dsp_addr(tmp);
    tmp = msg->u32Body[4] | ((AX_U64)msg->u32Body[5] << 32);
    pDst = (char *)ax_cpu_addr_to_dsp_addr(tmp);
    xthal_dcache_region_invalidate(pSrc0, size);
    xthal_dcache_region_invalidate(pSrc1, size);
    for (i = 0; i < size; i ++) {
        pDst[i] = pSrc0[i] + pSrc1[i];
    }
    xthal_dcache_region_writeback_inv(pDst, size);

//    xt_printf("pSrc0=0x%x",pSrc0);
//    xt_printf("pSrc1=0x%x",pSrc1);
//    xt_printf("pDst=0x%x",pDst);
//    for(i=0;i<8;i++)
//        xt_printf(" %d=0x%x", i, pDst[i]);

    return 0;
}

static AX_S32 SAMPL_iDmaPolling(AX_U32 dmaChn, AX_S32 idmaIndex)
{
    AX_S32 retVal;
    while (1) {
        retVal = AX_VDSP_CheckForIdmaIndex(dmaChn, idmaIndex);
        if (retVal < 0) {
            AX_VDSP_LOG_ERROR("dmaChn: %d, idmaIndex %d, retVal:%x\n", dmaChn, idmaIndex, retVal);
            return -1;
        }
        if (retVal == 1) {
            break;
        }
    }
    return retVal;
}

#define IMG_SIZE (3840*2160)
#define BUF_SIZE (320*320)
#define ROW_BYTES 256
#define NUM_ROWS 400

int AX_DSP_TestFBCDC(AX_DSP_MESSAGE_T *msg)
{
    AX_U64 pSrc, pDst0, pDst1;
    AX_S32 size = 0;
    AX_S32 idmaIndex, retVal;
    AX_VOID *buffers[2];
    AX_U64 srcW;
    AX_U64 dstW;

    pSrc = msg->u32Body[0] | ((AX_U64)msg->u32Body[1] << 32);
    pDst0 = msg->u32Body[2] | ((AX_U64)msg->u32Body[3] << 32);
    pDst1 = msg->u32Body[4] | ((AX_U64)msg->u32Body[5] << 32);

    retVal = AX_VDSP_AllocateBuffers(buffers, 1, BUF_SIZE, XV_MEM_BANK_COLOR_0, 64);
    if (retVal < 0) {
        AX_VDSP_LOG_ERROR("retVal %d\n", retVal);
        return -1;
    }
    retVal = AX_VDSP_AllocateBuffers(&buffers[1], 1, BUF_SIZE, XV_MEM_BANK_COLOR_1, 64);
    if (retVal < 0) {
        AX_VDSP_LOG_ERROR("retVal %d\n", retVal);
        return -1;
    }

    xt_printf("buffers[0] = 0x%x\n", buffers[0]);
    xt_printf("buffers[1] = 0x%x\n", buffers[1]);

    while (size < IMG_SIZE) {
        srcW = pSrc + size;
        dstW = (AX_U64)(AX_U32)buffers[0];
//        idmaIndex = AX_VDSP_Copy2DAddr(0, dstW, srcW, BUF_SIZE, 1, BUF_SIZE, BUF_SIZE, 0);
        idmaIndex = AX_VDSP_Copy2DAddr(0, dstW, srcW, ROW_BYTES, NUM_ROWS, ROW_BYTES, ROW_BYTES, 0);
        if (idmaIndex < 0) {
            AX_VDSP_LOG_ERROR("AX_VDSP_Copy2DAddr fail!\n");
            AX_VDSP_FreeBuffers(buffers, 2);
            return -1;
        }
        if (SAMPL_iDmaPolling(0, idmaIndex) < 0) {
            AX_VDSP_LOG_ERROR("polling fail!\n");
            AX_VDSP_FreeBuffers(buffers, 2);
            return -1;
        }

        //fbc
        dstW = pDst0 + size;
        srcW = (AX_U64)(AX_U32)buffers[0];
//        idmaIndex = AX_VDSP_Copy2DAddr(0, dstW, srcW, BUF_SIZE, 1, BUF_SIZE, BUF_SIZE, 0);
        idmaIndex = AX_VDSP_Copy2DAddr(0, dstW, srcW, ROW_BYTES, NUM_ROWS, ROW_BYTES, ROW_BYTES, 0);
        if (idmaIndex < 0) {
            AX_VDSP_LOG_ERROR("AX_VDSP_Copy2DAddr fail!\n");
            AX_VDSP_FreeBuffers(buffers, 2);
            return -1;
        }
        if (SAMPL_iDmaPolling(0, idmaIndex) < 0) {
            AX_VDSP_LOG_ERROR("polling fail!\n");
            AX_VDSP_FreeBuffers(buffers, 2);
            return -1;
        }

        size += BUF_SIZE;
        xt_printf("index=%d", size / BUF_SIZE);
    }

    size = 0;
    while (size < IMG_SIZE) {
        //fbdc
        srcW = pDst0 + size;
        dstW = (AX_U64)(AX_U32)buffers[1];
//        idmaIndex = AX_VDSP_Copy2DAddr(1, dstW, srcW, BUF_SIZE, 1, BUF_SIZE, BUF_SIZE, 0);
        idmaIndex = AX_VDSP_Copy2DAddr(1, dstW, srcW, ROW_BYTES, NUM_ROWS, ROW_BYTES, ROW_BYTES, 0);
        if (idmaIndex < 0) {
            AX_VDSP_LOG_ERROR("polling fail!\n");
            AX_VDSP_FreeBuffers(buffers, 2);
            return -1;
        }
        if (SAMPL_iDmaPolling(1, idmaIndex) < 0) {
            AX_VDSP_LOG_ERROR("polling fail!\n");
            AX_VDSP_FreeBuffers(buffers, 2);
            return -1;
        }

        dstW = pDst1 + size;
        srcW = (AX_U64)(AX_U32)buffers[1];
//        idmaIndex = AX_VDSP_Copy2DAddr(1, dstW, srcW, BUF_SIZE, 1, BUF_SIZE, BUF_SIZE, 0);
        idmaIndex = AX_VDSP_Copy2DAddr(1, dstW, srcW, ROW_BYTES, NUM_ROWS, ROW_BYTES, ROW_BYTES, 0);
        if (idmaIndex < 0) {
            AX_VDSP_LOG_ERROR("AX_VDSP_Copy2DAddr fail!\n");
            AX_VDSP_FreeBuffers(buffers, 2);
            return -1;
        }
        if (SAMPL_iDmaPolling(1, idmaIndex) < 0) {
            AX_VDSP_LOG_ERROR("polling fail!\n");
            AX_VDSP_FreeBuffers(buffers, 2);
            return -1;
        }

        size += BUF_SIZE;
        xt_printf("index2=%d", size / BUF_SIZE);
    }


ext:
//    xt_printf("pSrc=0x%llx",pSrc);
//    xt_printf("pDst0=0x%llx",pDst0);
//    xt_printf("pDst1=0x%llx",pDst1);
//    for(i=0;i<8;i++)
//        xt_printf(" %d=0x%x", i, pDst[i]);

    AX_VDSP_FreeBuffers(buffers, 2);

    return 0;
}
