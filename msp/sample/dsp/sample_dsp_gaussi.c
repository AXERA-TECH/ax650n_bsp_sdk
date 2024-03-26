/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <ax_base_type.h>
#include <ax_dsp_api.h>
#include <ax_sys_api.h>
#include <pthread.h>
#include "util.h"

#define PIC_SRC_NAME "/opt/data/dsp/noisyImage_YUV_512x512_8b.yuv"
#define PIC_DST_NAME "/opt/data/dsp/noisyImage_YUV_512x512_8b_dst.yuv"
#define ALIGN_SIZE 4
#define PIC_SIZE (512*512)

static AX_S32 SAMPLE_DSP_GaussianBlurProc(AX_DSP_ID_E dspid, AX_DSP_MESSAGE_T *msg)
{
    int ret;
    AX_DSP_HANDLE dspHandle;
    struct timeval tv_start, tv_end;

    gettimeofday(&tv_start, NULL);
    ret = AX_DSP_PRC(&dspHandle, msg, dspid, AX_DSP_PRI_1);
    if (ret != 0) {
        printf("%s PRC error %x\n", __func__, ret);
        return -1;
    }
    ret = AX_DSP_Query(dspid, dspHandle, msg, 1);
    if (ret != AX_DSP_SUCCESS) {
        printf("%s query fail %x\n", __func__, ret);
        return -1;
    }
    gettimeofday(&tv_end, NULL);
    printf("SAMPLE_DSP%d_GaussianBlurProc DSP cost time: usec: %ld\n", dspid,
           (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + (tv_end.tv_usec - tv_start.tv_usec));

    return 0;
}
static AX_S32 DSP_DataCheck(AX_VOID *data, AX_S32 size, AX_S32 checkSum)
{
    AX_S32 i;
    AX_S32 *pData = data;
    AX_S32 count;
    AX_S32 sum;
    if ((size & 3) != 0) {
        printf("data size is not 4 bytes algined, size is %x.\n", size);
        return -1;
    }
    count = size / 4;
    sum = 0;
    for (i = 0; i < count; i++) {
        sum += pData[i];
    }
    if (sum != 0xbf1920d6) {
        return -1;
    }
    return 0;
}
AX_S32 SAMPLE_DSP_GaussianBlur(AX_DSP_ID_E dspid)
{
    AX_S32 ret;
    AX_S32 i;
    AX_U64 phySrc, phyDst;
    AX_VOID *vSrc, *vDst;
    AX_DSP_MESSAGE_T msg;
    AX_S32 size;

    ret = AX_SYS_MemAlloc(&phySrc, &vSrc, PIC_SIZE, ALIGN_SIZE, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        printf("%s alloc failed %x\n", __func__, ret);
        return -1;
    }
    ret = AX_SYS_MemAlloc(&phyDst, &vDst, PIC_SIZE, ALIGN_SIZE, (AX_S8 *)"dsp_test");
    if (ret < 0) {
        AX_SYS_MemFree(phySrc, vSrc);
        printf("%s alloc failed %x\n", __func__, ret);
        return -1;
    }
    size = DSP_LoadPic(PIC_SRC_NAME, vSrc);
    if (size < 0) {
        AX_SYS_MemFree(phySrc, vSrc);
        AX_SYS_MemFree(phyDst, vDst);
        printf("%s load pic error %d\n", __func__, size);
        return -1;
    }
    msg.u32CMD = AX_DSP_CMD_TEST_GAUSSIBLUR;
    msg.u32Body[0] = phySrc & 0xffffffff;
    msg.u32Body[1] = phySrc >> 32;
    msg.u32Body[2] = phyDst & 0xffffffff;
    msg.u32Body[3] = phyDst >> 32;
    msg.u32Body[4] = size;
    for (i = 0; i < 1; i++) {
        memset(vDst, 0, size);
        ret = SAMPLE_DSP_GaussianBlurProc(dspid, &msg);
        if (ret != 0) {
            AX_SYS_MemFree(phySrc, vSrc);
            AX_SYS_MemFree(phyDst, vDst);
            printf("%s PRC error %x\n", __func__, ret);
            return -1;
        }
        DSP_SavePic(PIC_DST_NAME, vDst, size);
        if (DSP_DataCheck(vDst, size, 0) < 0) {
            AX_SYS_MemFree(phySrc, vSrc);
            AX_SYS_MemFree(phyDst, vDst);
            printf("%s data check fail, i = %d\n", __func__, i);
            return -1;
        }
    }
    AX_SYS_MemFree(phySrc, vSrc);
    AX_SYS_MemFree(phyDst, vDst);
    return 0;
}
