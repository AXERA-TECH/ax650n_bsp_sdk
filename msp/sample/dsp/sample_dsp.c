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
#include <stdlib.h>
#include "util.h"

//#define AX_DSP_ID_BUTT 1 //..
//#define SAMPLE_DSP_DEBUG
#ifdef SAMPLE_DSP_DEBUG
    #define DBG(fmt, ...) printf("%s %d" fmt,__func__, __LINE__, ##__VA_ARGS__)
#endif

static AX_S32 SAMPLE_DSP_Init(AX_DSP_ID_E dspid, char *itcm, char *sram)
{
    AX_S32 ret;
    ret = AX_DSP_PowerOn(dspid);
    if (ret != AX_DSP_SUCCESS) {
        printf("AX DSP Poweron error %x\n", ret);
        return -1;
    }
    ret = AX_DSP_LoadBin(dspid, sram, AX_DSP_MEM_TYPE_SRAM);
    if (ret != AX_DSP_SUCCESS) {
        printf("AX DSP LoadBin error %x\n", ret);
        return -1;
    }
    ret = AX_DSP_LoadBin(dspid, itcm, AX_DSP_MEM_TYPE_ITCM);
    if (ret != AX_DSP_SUCCESS) {
        printf("AX DSP LoadBin error %x\n", ret);
        return -1;
    }
    ret = AX_DSP_EnableCore(dspid);
    if (ret != AX_DSP_SUCCESS) {
        printf("AX DSP Enable Core error %x\n", ret);
        return -1;
    }
    return 0;
}
static AX_S32 SAMPLE_DSP_DeInit(AX_DSP_ID_E dspid)
{
    AX_DSP_DisableCore(dspid);
    AX_DSP_PowerOff(dspid);

    return 0;
}

#define TEST_SIZE 0x1000
#define ALIGN_SIZE 0x4
void *virt[AX_DSP_ID_BUTT][3];
AX_U64 phyAddr[AX_DSP_ID_BUTT][3];
static int SAMPLE_DSP_ArrayAddTest(AX_DSP_ID_E dspid)
{
    AX_S32 ret, i, j;
    AX_DSP_HANDLE dspHandle;
    AX_DSP_MESSAGE_T msg;
    AX_BOOL bBlock = 1;
    AX_U8 *ptr0, *ptr1, *ptr2;

    for (i = 0; i < 2; i++) {
        ptr0 = (AX_U8 *)virt[dspid][i];
        for (j = 0; j < TEST_SIZE; j++) {
            ptr0[j] = j;
        }
        ret = AX_SYS_MflushCache(phyAddr[dspid][i], virt[dspid][i], TEST_SIZE);
        if (ret != 0) {
            printf("SAMPLE_DSP%d_ArrayAddTest flush error %x %x\n", dspid, i, ret);
            return -1;
        }
    }
    msg.u32CMD = AX_DSP_CMD_TEST_ADD;
    msg.u32Body[0] = phyAddr[dspid][0] & 0xffffffff;
    msg.u32Body[1] = phyAddr[dspid][0] >> 32;
    msg.u32Body[2] = phyAddr[dspid][1] & 0xffffffff;
    msg.u32Body[3] = phyAddr[dspid][1] >> 32;
    msg.u32Body[4] = phyAddr[dspid][2] & 0xffffffff;
    msg.u32Body[5] = phyAddr[dspid][2] >> 32;

    ret = AX_DSP_PRC(&dspHandle, &msg, dspid, AX_DSP_PRI_1);
    if (ret != 0) {
        printf("SAMPLE_DSP%d_ArrayAddTest PRC error %x\n", dspid, ret);
        return -1;
    }

    ret = AX_DSP_Query(dspid, dspHandle, &msg, bBlock);
    if (ret != AX_DSP_SUCCESS) {
        if (bBlock) {
            printf("SAMPLE_DSP%d_ArrayAddTest query error %x\n", dspid, ret);
            return -1;
        }
    }

    AX_SYS_MinvalidateCache(phyAddr[dspid][2], virt[dspid][2], TEST_SIZE);
    for (i = 0; i < TEST_SIZE; i++) {
        ptr0 = (AX_U8 *)virt[dspid][0];
        ptr1 = (AX_U8 *)virt[dspid][1];
        ptr2 = (AX_U8 *)virt[dspid][2];
        if (ptr2[i] != ((ptr0[i] + ptr1[i]) & 0xff)) {
            printf("SAMPLE_DSP%d_ArrayAddTest compare error %x, src0: %x, src1: %x, dst: %x\n", dspid, i, ptr0[i], ptr1[i],
                   ptr2[i]);
            break;
        }
    }
    if (i != TEST_SIZE) {
        printf("SAMPLE_DSP%d_ArrayAddTest FAILED!\n", dspid);
        ret = -1;
    }

    return ret;
}
int SAMPLE_DSP_Test(AX_DSP_ID_E dspid)
{
    AX_S32 ret;
    int i;

    for (i = 0; i < 3; i++) {
        ret = AX_SYS_MemAllocCached(&phyAddr[dspid][i], &virt[dspid][i], TEST_SIZE, ALIGN_SIZE, (AX_S8 *)"dsp_test");
        if (ret != 0) {
            printf("SAMPLE_DSP%d_ArrayAddTest alloc[%d] failed %x\n", dspid, i, ret);
            return -1;
        }
        printf("phyAddr[%d] = 0x%llx, virt[%d] = 0x%llx\n", i, phyAddr[dspid][i], i, (long long unsigned int)virt[dspid][i]);
    }

    {
        ret = SAMPLE_DSP_ArrayAddTest(dspid);
        if (ret != AX_DSP_SUCCESS) {
            printf("AX DSP%d arrayadd test error %x\n", dspid, ret);
            ret = -1;
        }
    }

    for (i = 0; i < 3; i++) {
        AX_SYS_MemFree(phyAddr[dspid][i], virt[dspid][i]);
    }

    return ret;
}
int SAMPLE_DSP_TestCopy(AX_DSP_ID_E dspid)
{
    AX_DSP_HANDLE dspHandle;
    AX_DSP_MESSAGE_T msg;
    AX_BOOL bBlock = 1;
    AX_S32 ret;
    struct timeval tv_start, tv_end;

    gettimeofday(&tv_start, NULL);

    msg.u32CMD = AX_DSP_CMD_TEST_COPY;

    ret = AX_DSP_PRC(&dspHandle, &msg, dspid, AX_DSP_PRI_1);
    if (ret != 0) {
        printf("SAMPLE_DSP%d_TestCopy PRC error %x\n", dspid, ret);
        return -1;
    }

    ret = AX_DSP_Query(dspid, dspHandle, &msg, bBlock);
    if (ret != AX_DSP_SUCCESS) {
        if (bBlock) {
            printf("SAMPLE_DSP%d_TestCopy query error %x\n", dspid, ret);
            return -1;
        }
    }

    gettimeofday(&tv_end, NULL);
    printf("SAMPLE_DSP%d_TestCopy DSP cost time: usec: %ld\n", dspid,
           (tv_end.tv_sec - tv_start.tv_sec) * 1000000 + (tv_end.tv_usec - tv_start.tv_usec));

    ret = msg.u32Body[0];

    printf("ret = %d\n", ret);

    return ret;
}

extern AX_S32 SAMPLE_DSP_GaussianBlur(AX_DSP_ID_E dspid);
extern int SAMPLE_DSP_TestResize(AX_DSP_ID_E dspid);
extern int SAMPLE_DSP_TestCvtColor(AX_DSP_ID_E dspid);
extern int SAMPLE_DSP_jointLR(AX_DSP_ID_E dspid);
extern int SAMPLE_DSP_TestCvtColor_KVM(AX_DSP_ID_E dspid);

//#define THREAD_TEST
#ifndef THREAD_TEST
int main(int argc, AX_CHAR *argv[])
{
    int i;
    int ret;
    if (argc != 3) {
        printf("input argument not correct! please input:\n");
        printf("sample_dsp itcm.bin sram.bin\n");
        return -1;
    }
    printf("itcm file path: %s, sram file path: %s\n", argv[1], argv[2]);

    ret = AX_SYS_Init();
    if (ret != 0) {
        printf("AX_SYS_Init error %x\n", ret);
        return ret;
    }

    for (i = 0; i < AX_DSP_ID_BUTT; i++) {
        ret = SAMPLE_DSP_Init(i, argv[1], argv[2]);
        if (ret != AX_DSP_SUCCESS) {
            printf("AX DSP%d Init error %x\n", i, ret);
            return ret;
        }
    }
    usleep(100000);//wait dsp ready

    for (i = 0; i < AX_DSP_ID_BUTT; i++) {
        ret = SAMPLE_DSP_Test(i);
        if (ret)
            printf("SAMPLE_DSP%d_ArrayAddTest FAILED\n", i);
        else
            printf("SAMPLE_DSP%d_ArrayAddTest PASS!\n", i);
    }
    for (i = 0; i < AX_DSP_ID_BUTT; i++) {
        ret = SAMPLE_DSP_GaussianBlur(i);
        if (ret)
            printf("SAMPLE_DSP%d_GaussianBlur FAILED\n", i);
        else
            printf("SAMPLE_DSP%d_GaussianBlur PASS\n", i);
    }

    for (i = 0; i < AX_DSP_ID_BUTT; i++) {
        ret = SAMPLE_DSP_TestCopy(i);
        if (ret)
            printf("SAMPLE_DSP%d_TestCopy FAILED\n", i);
        else
            printf("SAMPLE_DSP%d_TestCopy PASS\n", i);
    }

    ret = SAMPLE_DSP_TestResize(0);
    if (ret)
        printf("SAMPLE_DSP0_TestResize FAILED\n");

    ret = SAMPLE_DSP_TestCvtColor(1);
    if (ret)
        printf("SAMPLE_DSP1_TestCvtColor FAILED\n");

    ret = SAMPLE_DSP_jointLR(0);
    if (ret)
        printf("SAMPLE_DSP0_jointLR FAILED\n");

    ret = SAMPLE_DSP_TestCvtColor_KVM(1);
    if (ret)
        printf("SAMPLE_DSP1_TestCvtColor_KVM FAILED\n");

    while (1)
        sleep(10);

    for (i = 0; i < AX_DSP_ID_BUTT; i++) {
        SAMPLE_DSP_DeInit(i);
    }

    return 0;
}
#else
#include <pthread.h>
#define DSP_COUNT AX_DSP_ID_BUTT
#define THREAD_TEST_NUM 1024
#define PRI_NUM 4
static int thread_exit = 0;
static int th_proc_cnt[DSP_COUNT][PRI_NUM] = {0,};
struct gbl {
    AX_DSP_HANDLE th_handle;
    int th_hold;
    int th_pri;
    struct timeval th_tv_start;
    struct timeval th_tv_end;
};
struct gbl th_gbl[DSP_COUNT][THREAD_TEST_NUM];
struct th_param {
    int dspid;
    int num;
};
struct th_param th_proc_param[DSP_COUNT][THREAD_TEST_NUM];
pthread_mutex_t th_mutex[DSP_COUNT][PRI_NUM];
static void func_printinfo(int dspid)
{
    int i;

    for (i = 0; i < PRI_NUM; i++) {
        pthread_mutex_lock(&th_mutex[dspid][i]);
        printf("dsp%d proc (pri%d) count %d\n", dspid, i, th_proc_cnt[dspid][i]);
        pthread_mutex_unlock(&th_mutex[dspid][i]);
    }
    thread_exit = 1;
}
static void *func_proc(struct th_param *param)
{
    int ret;
    int k = 0, cnt;
    int dspid = param->dspid;
    int seq = param->num;

    while (!thread_exit) {
        {
            AX_DSP_HANDLE dspHandle;
            AX_DSP_MESSAGE_T msg;
            AX_BOOL bBlock = 1;
            AX_S32 ret;
            int pri = rand() % 4;
            volatile int *ptr;

            gettimeofday(&th_gbl[dspid][seq + k].th_tv_start, NULL);

            msg.u32CMD = AX_DSP_CMD_TEST_COPY;
retry:
            ret = AX_DSP_PRC(&dspHandle, &msg, dspid, pri);
            if (ret != 0) {
                if (ret == AX_DSP_BUSY) {
                    usleep(1000);
                    goto retry;
                }
                printf("SAMPLE_DSP%d_TestCopy PRC error %x\n", dspid, ret);
                func_printinfo(dspid);
                return -1;
            }

            pthread_mutex_lock(&th_mutex[dspid][pri]);
            ptr = &th_proc_cnt[dspid][pri];
            *ptr = *ptr + 1;
            pthread_mutex_unlock(&th_mutex[dspid][pri]);
            if (*ptr >= 32) {
                func_printinfo(dspid);
            }

            th_gbl[dspid][seq + k].th_handle = dspHandle;
            th_gbl[dspid][seq + k].th_hold = 1;
            th_gbl[dspid][seq + k].th_pri = pri;

            ////////////////////////////////////
            int num = seq + k;
            ret = AX_DSP_Query(dspid, dspHandle, &msg, bBlock);
            if (ret != AX_DSP_SUCCESS) {
                if (bBlock) {
                    printf("SAMPLE_DSP%d_TestCopy handle%d query error %x\n", dspid, dspHandle, ret);
                    func_printinfo(dspid);
                    return -1;
                }
            }

            gettimeofday(&th_gbl[dspid][num].th_tv_end, NULL);

            th_gbl[dspid][num].th_hold = 0;

            pthread_mutex_lock(&th_mutex[dspid][pri]);
            ptr = &th_proc_cnt[dspid][th_gbl[dspid][num].th_pri];
            *ptr = *ptr - 1;
            pthread_mutex_unlock(&th_mutex[dspid][pri]);

            ret = msg.u32Body[0];

            if (ret)
                printf("SAMPLE_DSP%d %s FAILED\n", dspid, __func__);
            else {
                printf("SAMPLE_DSP%d_TestCopy DSP cost time: usec: %ld\n", dspid,
                       (th_gbl[dspid][num].th_tv_end.tv_sec - th_gbl[dspid][num].th_tv_start.tv_sec) * 1000000 +
                       (th_gbl[dspid][num].th_tv_end.tv_usec - th_gbl[dspid][num].th_tv_start.tv_usec));
                printf("SAMPLE_DSP%d %s PASS\n", dspid, __func__);
            }

        }
    }
    return (void *)0;
}

int main(int argc, char *argv[])
{
    int i, ret, k;
    pthread_t ptd[AX_DSP_ID_BUTT][THREAD_TEST_NUM];
    int thread_cnt = 16;

    if (argc < 3) {
        printf("input argument not correct! please input:\n");
        printf("sample_dsp itcm.bin sram.bin [thread count]\n");
        return -1;
    }

    if (argc == 4)
        thread_cnt = atoi(argv[3]);

    srand((unsigned int)time(0));

    for (i = 0; i < DSP_COUNT; i++)
        for (k = 0; k < PRI_NUM; k++)
            pthread_mutex_init(&th_mutex[i][k], 0);

    ret = AX_SYS_Init();
    if (ret != 0) {
        printf("AX_SYS_Init error %x\n", ret);
        return ret;
    }

    for (i = 0; i < AX_DSP_ID_BUTT; i++) {
        ret = SAMPLE_DSP_Init(i, argv[1], argv[2]);
        if (ret != AX_DSP_SUCCESS) {
            printf("AX DSP%d Init error %x\n", i, ret);
            return ret;
        }
    }
    usleep(100000);//wait dsp ready

    for (i = 0; i < DSP_COUNT; i++) {
        for (k = 0; k < thread_cnt; k++) {
            th_proc_param[i][k].dspid = i;
            th_proc_param[i][k].num = k;
            pthread_create(&ptd[i][k], 0, (void *(*)(void *))func_proc, &th_proc_param[i][k]);
        }
    }

    while (1) {
        sleep(20);
        //thread_exit = 1;
    }

    for (i = 0; i < AX_DSP_ID_BUTT; i++) {
        SAMPLE_DSP_DeInit(i);
    }

    return 0;
}
#endif
