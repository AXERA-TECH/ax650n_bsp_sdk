/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include <xtensa/config/core.h>
#include <xtensa/hal.h>
#include <string.h>
#if defined BOARD
    #include <xtensa/xtbsp.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <xtensa/xtutil.h>
#include <ax_base_type.h>
#include <ax_dsp_common.h>
#include <ax_base_type.h>
#include <ax650x_api.h>
#include <xtensa/idma.h>
#include <xtensa/hal.h>

#include "ax_dsp_def.h"
#include "ax_dsp_tm.h"
#include "ax_dsp_trace.h"

#define TEST_SIZE 0x1000
ALIGNDCACHE char testSrc[TEST_SIZE];
ALIGNDCACHE char testDst[TEST_SIZE];

static AX_VOID DataInit(AX_CHAR *src, AX_S32 size)
{
    int i;
    for (i = 0 ; i < size; i++) {
        src[i] = (rand() + 1) & 0xFF;
    }
}
static AX_S32 DataCompare(AX_CHAR *src, AX_CHAR *dst, AX_S32 size)
{
    int i;
    for (i = 0 ; i < size; i++)
        if (src[i] != dst[i])
            return -1;
    return 0;
}
static AX_S32 SAMPLE_VDSPCopyBlock(AX_VOID)
{
    AX_VOID *buffers[2];
    AX_S32 retVal;
    AX_U64 srcW;
    AX_U64 dstW;
    AX_VDSP_LOG_INFO("enter testSrc = 0x%x", testSrc);
    retVal = AX_VDSP_AllocateBuffers(buffers, 2, TEST_SIZE, XV_MEM_BANK_COLOR_0, 64);
    if (retVal < 0) {
        AX_VDSP_LOG_ERROR("retVal %d\n", retVal);
        return -1;
    }
    DataInit(testSrc, TEST_SIZE);
    memset(buffers[0], 0, TEST_SIZE);
    xthal_dcache_region_writeback_inv(testSrc, TEST_SIZE);
    srcW = ax_dsp_addr_to_idma_addr((unsigned int)testSrc);
    dstW = (AX_U64)(AX_U32)buffers[0];

    AX_VDSP_LOG_INFO("%llx, %llx\n", srcW, dstW);
    retVal = AX_VDSP_CopyData(dstW, srcW, TEST_SIZE);
    if (retVal < 0) {
        AX_VDSP_LOG_ERROR("retVal %d\n", retVal);
        AX_VDSP_FreeBuffers(buffers, 2);
        return -1;
    }
    if (DataCompare(testSrc, buffers[0], TEST_SIZE) != 0) {
        AX_VDSP_LOG_ERROR("compare error\n");
        AX_VDSP_FreeBuffers(buffers, 2);
        return -1;
    }
    AX_VDSP_LOG_INFO("DDR to DTCM Pass!");
    srcW = (AX_U64)(AX_U32)buffers[0];
    dstW = (AX_U64)(AX_U32)buffers[1];
    memset(buffers[1], 0, TEST_SIZE);
    AX_VDSP_LOG_INFO("%llx, %llx\n", srcW, dstW);
    retVal = AX_VDSP_CopyData(dstW, srcW, TEST_SIZE);
    if (retVal < 0) {
        AX_VDSP_LOG_ERROR("retVal %d\n", retVal);
        AX_VDSP_FreeBuffers(buffers, 2);
        return -1;
    }
    if (DataCompare(buffers[0], buffers[1], TEST_SIZE) != 0) {
        AX_VDSP_LOG_ERROR("compare error\n");
        AX_VDSP_FreeBuffers(buffers, 2);
        return -1;
    }
    AX_VDSP_LOG_INFO("DTCM to DTCM Pass!");
    srcW = (AX_U64)(AX_U32)buffers[1];
    dstW = ax_dsp_addr_to_idma_addr((unsigned int)testDst);
    memset(testDst, 0, TEST_SIZE);
    AX_VDSP_LOG_INFO("%llx, %llx\n", srcW, dstW);
    retVal = AX_VDSP_CopyData(dstW, srcW, TEST_SIZE);
    if (retVal < 0) {
        AX_VDSP_LOG_ERROR("retVal %d\n", retVal);
        AX_VDSP_FreeBuffers(buffers, 2);
        return -1;
    }
    xthal_dcache_region_invalidate(testDst, TEST_SIZE);
    if (DataCompare(testDst, buffers[1], TEST_SIZE) != 0) {
        AX_VDSP_LOG_ERROR("compare error\n");
        AX_VDSP_FreeBuffers(buffers, 2);
        return -1;
    }
    AX_VDSP_FreeBuffers(buffers, 2);
    AX_VDSP_LOG_INFO("DTCM to DDR Pass!");
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
AX_S32 SAMPLE_1dCopy(AX_VOID)
{
    AX_VOID *buffers[2];
    AX_S32 retVal;
    AX_U64 srcW;
    AX_U64 dstW;
    AX_S32 idmaIndex;
    AX_S32 i;
    AX_VDSP_LOG_INFO("enter");
    retVal = AX_VDSP_AllocateBuffers(buffers, 2, TEST_SIZE, XV_MEM_BANK_COLOR_0, 64);
    if (retVal < 0) {
        AX_VDSP_LOG_ERROR("retVal %d\n", retVal);
        return -1;
    }
    DataInit(testSrc, TEST_SIZE);
    xthal_dcache_region_writeback_inv(testSrc, TEST_SIZE);
    srcW = ax_dsp_addr_to_idma_addr((unsigned int)testSrc);
    dstW = (AX_U64)(AX_U32)buffers[0];

    AX_VDSP_LOG_INFO("%llx, %llx\n", srcW, dstW);
    for (i = 0; i < 2; i++) {
        memset(buffers[0], 0, TEST_SIZE);
        idmaIndex = AX_VDSP_CopyDataWideAddr(i, dstW, srcW, TEST_SIZE, 0);
        if (idmaIndex < 0) {
            AX_VDSP_LOG_ERROR("idmaIndex %d\n", idmaIndex);
            AX_VDSP_FreeBuffers(buffers, 2);
            return -1;
        }
        if (SAMPL_iDmaPolling(i, idmaIndex) < 0) {
            AX_VDSP_LOG_ERROR("polling fail!\n");
            AX_VDSP_FreeBuffers(buffers, 2);
            return -1;
        }
        if (DataCompare(testSrc, buffers[0], TEST_SIZE) != 0) {
            AX_VDSP_LOG_ERROR("compare error\n");
            AX_VDSP_FreeBuffers(buffers, 2);
            return -1;
        }
        AX_VDSP_LOG_INFO("PASS idmaIndex %d\n", idmaIndex);
    }
    AX_VDSP_FreeBuffers(buffers, 2);
    AX_VDSP_LOG_INFO("Exit");
    return 0;
}
AX_S32 SAMPLE_2dCopy(AX_VOID)
{
    AX_VOID *buffers[2];
    AX_S32 retVal;
    AX_U64 srcW;
    AX_U64 dstW;
    AX_S32 idmaIndex;
    AX_S32 rowSize = 512;
    AX_S32 numRows = 8;
    AX_S32 srcPitchBytes = 512;
    AX_S32 dstPitchBytes = 512;
    AX_S32 i;

    AX_VDSP_LOG_INFO("enter");
    retVal = AX_VDSP_AllocateBuffers(buffers, 2, TEST_SIZE, XV_MEM_BANK_COLOR_0, 64);
    if (retVal < 0) {
        AX_VDSP_LOG_ERROR("retVal %d\n", retVal);
        return -1;
    }
    DataInit(testSrc, TEST_SIZE);
    xthal_dcache_region_writeback_inv(testSrc, TEST_SIZE);
    srcW = ax_dsp_addr_to_idma_addr((unsigned int)testSrc);
    dstW = (AX_U64)(AX_U32)buffers[0];

    AX_VDSP_LOG_INFO("%llx, %llx\n", srcW, dstW);
    for (i = 0; i < 2; i++) {
        memset(buffers[0], 0, TEST_SIZE);
        idmaIndex = AX_VDSP_Copy2DAddr(i, dstW, srcW, rowSize, numRows, srcPitchBytes, dstPitchBytes, 0);
        AX_VDSP_LOG_INFO("idmaIndex %d\n", idmaIndex);
        if (idmaIndex < 0) {
            AX_VDSP_LOG_ERROR("idmaIndex %d\n", idmaIndex);
            AX_VDSP_FreeBuffers(buffers, 2);
            return -1;
        }
        if (SAMPL_iDmaPolling(i, idmaIndex) < 0) {
            AX_VDSP_LOG_ERROR("polling fail!\n");
            AX_VDSP_FreeBuffers(buffers, 2);
            return -1;
        }
        if (DataCompare(testSrc, buffers[0], TEST_SIZE) != 0) {
            AX_VDSP_LOG_ERROR("compare error\n");
            AX_VDSP_FreeBuffers(buffers, 2);
            return -1;
        }
    }
    AX_VDSP_FreeBuffers(buffers, 2);
    AX_VDSP_LOG_INFO("Exit");
    return 0;
}
AX_S32 SAMPLE_VDSPCopy()
{
    AX_S32 ret;

    ret = SAMPLE_VDSPCopyBlock();
    ret += SAMPLE_1dCopy();
    ret += SAMPLE_2dCopy();

    return ret;
}

#define PERF_SIZE 0x30000
ALIGNDCACHE char testPerf[PERF_SIZE];
#define READ_DIR (1<<1)
#define WRITE_DIR (1<<0)
int AX_DSP_TestIdmaPerf(int isread)
{
    AX_U32 c0, c1;
    AX_VOID *buffers[1];
    AX_S32 retVal;
    AX_U64 srcW;
    AX_U64 dstW;
    AX_S32 idmaIndex;
    int rowSize = 0x1000;
    int numRows = 0x30;
    static int bh = 0;
    AX_U32 rdcnt = 0, wrcnt = 0;
    int rdn = 0, wrn = 0;

    retVal = AX_VDSP_AllocateBuffers(buffers, 1, PERF_SIZE, XV_MEM_BANK_COLOR_0, 64);
    if (retVal < 0) {
        AX_VDSP_LOG_ERROR("retVal %d\n", retVal);
        return -1;
    }
    DataInit(testSrc, PERF_SIZE);
    xthal_dcache_region_writeback_inv(testSrc, PERF_SIZE);

    while (1) {
        if (!bh)
            AX_VDSP_LOG_ERROR("isread = 0x%x", isread);
        if (isread & READ_DIR) {
            srcW = ax_dsp_addr_to_idma_addr((unsigned int)testPerf);
            dstW = (AX_U64)(AX_U32)buffers[0];
            if (!bh)
                xt_printf("src = 0x%llx, dst = 0x%llx", srcW, dstW);
            c0 = xthal_get_ccount();
            idmaIndex = AX_VDSP_Copy2DAddr(0, dstW, srcW, rowSize, numRows, rowSize, rowSize, 0);
            if (SAMPL_iDmaPolling(0, idmaIndex) < 0) {
                AX_VDSP_LOG_ERROR("polling fail!\n");
                AX_VDSP_FreeBuffers(buffers, 1);
                return -1;
            }
            c1 = xthal_get_ccount();
            if (c1 - c0 > 0)
                rdcnt += (c1 - c0);
            rdn++;
            if (!bh)AX_VDSP_LOG_ERROR("R=> 0x%x\n", c1 - c0);
            if (rdcnt > 4000000000) {
                AX_VDSP_LOG_ERROR("R=> %f\n", (1.0 * rdn * 0x30000 * 800000000) / rdcnt);
                rdn = rdcnt = 0;
            }
        }
        if (isread & WRITE_DIR) {
            srcW = (AX_U64)(AX_U32)buffers[0];
            dstW = ax_dsp_addr_to_idma_addr((unsigned int)testPerf);
            if (!bh)
                xt_printf("src = 0x%llx, dst = 0x%llx", srcW, dstW);
            c0 = xthal_get_ccount();
            idmaIndex = AX_VDSP_Copy2DAddr(0, dstW, srcW, rowSize, numRows, rowSize, rowSize, 0);
            if (idmaIndex < 0)
                xt_printf("AX_VDSP_Copy2DAddr return %d", idmaIndex);
            if (SAMPL_iDmaPolling(0, idmaIndex) < 0) {
                AX_VDSP_LOG_ERROR("polling fail!\n");
                AX_VDSP_FreeBuffers(buffers, 1);
                return -1;
            }
            c1 = xthal_get_ccount();
            if (c1 - c0 > 0)
                wrcnt += (c1 - c0);
            wrn++;
            if (!bh)
                AX_VDSP_LOG_ERROR("W=> 0x%x\n", c1 - c0);
            if (wrcnt > 4000000000) {
                AX_VDSP_LOG_ERROR("W=> %f\n", (1.0 * wrn * 0x30000 * 800000000) / wrcnt);
                wrn = wrcnt = 0;
            }
        }
        if (!bh)bh = 1;
    }

    AX_VDSP_FreeBuffers(buffers, 1);
    return 0;
}
