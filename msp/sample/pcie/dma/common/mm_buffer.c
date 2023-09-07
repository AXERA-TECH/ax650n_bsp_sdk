/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include "ax_base_type.h"


#define AX_MM_DEV "/dev/ax_mmb_dev"


// ioctl cmd
#define PCIE_BASE       'H'
#define PCIE_DMA_GET_PHY_BASE  _IOW(PCIE_BASE, 3, unsigned int)
#define AX_IOC_PCIe_ALLOC_MEMORY   _IOW(PCIE_BASE, 4, unsigned int)

static int g_MmbFd[256];


AX_S32 MM_BufferInit(AX_S32 TargetId, AX_U8 **MM_VirtualAddr, AX_U64 *MM_PhyBaseAddr, AX_U64 Size)
{
    AX_S32 AX_MM_Fd;
    AX_U64 DmaBufferSize;
    AX_S32  Ret = 0;

    if (g_MmbFd[TargetId] > 0) {
        printf("Target %d has applied for buf \n", TargetId);
        return -1;
    }

    AX_MM_Fd = open(AX_MM_DEV, O_RDWR);
    if (AX_MM_Fd == -1) {
        printf("open %s failed!\n", AX_MM_DEV);
        return -1;
    }
    g_MmbFd[TargetId] = AX_MM_Fd;

    DmaBufferSize = Size;
    Ret = ioctl(AX_MM_Fd, AX_IOC_PCIe_ALLOC_MEMORY, &DmaBufferSize);
    if (Ret < 0) {
        printf("alloc MM buffer failed\n");
        return -1;
    }

    Ret = ioctl(AX_MM_Fd, PCIE_DMA_GET_PHY_BASE, MM_PhyBaseAddr);
    if (Ret < 0 || *MM_PhyBaseAddr <= 0) {
        printf("get MM buffer base addr failed\n");
        return -1;
    }
    printf("MM buffer physical base addr:%llx, size:%lld\n", *MM_PhyBaseAddr, DmaBufferSize);

    *MM_VirtualAddr = (AX_U8 *)mmap(NULL, DmaBufferSize, PROT_READ | PROT_WRITE, MAP_SHARED, AX_MM_Fd, 0);
    if (*MM_VirtualAddr <= 0) {
        printf("mmap fail\n");
        return -1;
    }

    printf("MM buffer virtual addr:%lx\n", (long)(*MM_VirtualAddr));
    printf("initial data in MM buffer: %s\n", *MM_VirtualAddr);

    return 0;
}

AX_U8 *MM_Phy2VirtAddr(AX_U64 PhyAddr, AX_U64 BasePhyAddr, AX_U8 *BaseVirtAddr)
{
    return BaseVirtAddr + (PhyAddr - BasePhyAddr);
}

AX_S32 MM_BufferDeInit(AX_S32 TargetId, AX_U8 *MM_VirtualAddr, AX_U64 Size)
{
    if (g_MmbFd[TargetId] <= 0) {
        printf("Target %d has free \n", TargetId);
        return -1;
    }
    munmap(MM_VirtualAddr, Size);
    close(g_MmbFd[TargetId]);
    g_MmbFd[TargetId] = -1;
    return 0;
}