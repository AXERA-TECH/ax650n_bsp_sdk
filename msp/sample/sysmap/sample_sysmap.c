/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>
#include <ax_base_type.h>
#include <ax_sys_api.h>

#define MMAP_FILE_NAME "/dev/ax_sysmap"

#define MMAP_TEST_LEN   0x1200000
void *srcTestVirt, *dstTestVirt;
AX_U64 testAddrSrc, testAddrDst;
static int SAMPLE_SYSMAP_Init(void)
{
    int ret;
    AX_SYS_Init();
    ret = AX_SYS_MemAlloc(&testAddrSrc, &srcTestVirt, MMAP_TEST_LEN, 0x4, (AX_S8 *)"ax_sysmap_test");
    if (ret < 0) {
        printf("SAMPLE_SYSMAP_Init alloc src buffer failed %x\n", ret);
        return -1;
    }
    ret = AX_SYS_MemAlloc(&testAddrDst, &dstTestVirt, MMAP_TEST_LEN, 0x4, (AX_S8 *)"ax_sysmap_test");
    if (ret < 0) {
        printf("SAMPLE_SYSMAP_Init alloc dst buffer failed %x\n", ret);
        return -1;
    }
    printf("malloc phy addr: %llx, %llx\n", testAddrSrc, testAddrDst);
    return 0;
}
static int SAMPLE_SYSMAP_DeInit(void)
{
    AX_SYS_MemFree(testAddrSrc, srcTestVirt);
    AX_SYS_MemFree(testAddrDst, srcTestVirt);
    return 0;
}
static int SAMPLE_SYSMAP_Test(AX_BOOL bCached)
{
    char *addrMap0, *addrMap1;
    struct timeval start, end;
    int i;
    int fd;
    if (!bCached) {
        fd = open(MMAP_FILE_NAME, O_RDWR | O_SYNC);
    } else {
        fd = open(MMAP_FILE_NAME, O_RDWR);
    }
    if (fd == -1) {
        printf("open %s fail!\n", MMAP_FILE_NAME);
        return -1;
    }
    addrMap0 = (char *)mmap(0, MMAP_TEST_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, testAddrSrc);
    addrMap1 = (char *)mmap(0, MMAP_TEST_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, testAddrDst);

    if ((addrMap0 == MAP_FAILED) || (addrMap1 == MAP_FAILED)) {
        printf("map fail, %lx, %lxS\n", (AX_LONG)addrMap0, (AX_LONG)addrMap1);
        goto errExit;
    }
    for (i = 0; i < 0x20; i++) {
        memcpy(addrMap0 + i, addrMap1 + i, MMAP_TEST_LEN - i);
        if (memcmp(addrMap0 + i, addrMap1 + i, MMAP_TEST_LEN - i)) {
            printf("memcpy fail, i: %x\n", i);
        }
    }
    gettimeofday(&start, NULL);
    for (i = 0; i < 50; i++) {
        memcpy(addrMap0, addrMap1, MMAP_TEST_LEN);
    }
    gettimeofday(&end, NULL);
    printf("time used: %ld.%ldS\n", (AX_LONG)(end.tv_sec - start.tv_sec),
           (end.tv_sec - start.tv_sec) ? (end.tv_usec - start.tv_usec + 1000000) : (end.tv_usec - start.tv_usec));
errExit:
    if (addrMap0) {
        munmap((void *)addrMap0, MMAP_TEST_LEN);
        addrMap0 = 0;
    }

    if (addrMap1) {
        munmap((void *)addrMap1, MMAP_TEST_LEN);
        addrMap1 = 0;
    }
    close(fd);
    return 0;
}
int main()
{
    if (SAMPLE_SYSMAP_Init() < 0) {
        printf("sample sysmap init failed\n");
        return -1;
    }
    printf("Test uncached\n");
    SAMPLE_SYSMAP_Test(0);
    printf("Test cached\n");
    SAMPLE_SYSMAP_Test(1);
    SAMPLE_SYSMAP_DeInit();
    printf("samp sysmap test pass\n");
    return 0;
}
